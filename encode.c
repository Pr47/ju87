#include "encode.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "encode_impl.c"

#ifndef DEFAULT_BUFSIZ_INIT
#define DEFAULT_BUFSIZ_INIT 4096
#endif

void ju87_encode_ctx_init(ctx, bufsiz_init, endian, verbose)
  ju87_encode_ctx *ctx;
  size_t bufsiz_init;
  ju87_endian endian;
  bool verbose;
{
  if (!bufsiz_init)
    {
      bufsiz_init = DEFAULT_BUFSIZ_INIT;
    }

  ctx->buf = malloc(bufsiz_init);
  ctx->bufsiz = bufsiz_init;
  ctx->offset = 0;
  ctx->endian = endian;
  ctx->verbose = verbose;
}

void ju87_encode_ctx_deinit(ctx)
  ju87_encode_ctx *ctx;
{
  free(ctx->buf);
  ctx->buf = NULL;
  ctx->bufsiz = 0;
  ctx->offset = 0;
}

typedef struct {
  /* prefixes */
  uint8_t lock;
  uint8_t mandatory;
  uint8_t op_override;

  uint8_t rex;
  uint8_t modrm;
  uint8_t sib;

  bool has_lock : 1;
  bool has_op_override : 1;
  bool has_mandatory : 1;
  bool has_modrm : 1;
  bool has_sib : 1;
  unsigned : 2;
} prefix_modrm_sib;

#define REX_FIXED_BITS (4 << 4)
#define REX_W_BIT 0x8
#define REX_R_BIT 0x4
#define REX_X_BIT 0x2
#define REX_B_BIT 0x1

#define MOD_BITS_BASE_ONLY (0 << 6)
#define MOD_BITS_BASE_DISP8 (1 << 6)
#define MOD_BITS_BASE_DISP32 (2 << 6)
#define MOD_BITS_REG (3 << 6)

#define MOD_REG_BITS(REG) ((REG) << 3)
#define MOD_RM_BITS(REG) (REG)

#define SIB_SCALE_BITS(SCALE) ((SCALE) << 6)
#define SIB_INDEX_BITS(IDX) ((IDX) << 3)
#define SIB_BASE_BITS(BASE) (BASE)

static void encode_prefix(ctx, pms)
  ju87_encode_ctx *ctx;
  prefix_modrm_sib *pms;
{
  if (pms->has_lock)
    {
      encode_ctx_push_byte(ctx, pms->lock);
    }
  if (pms->has_op_override)
    {
      encode_ctx_push_byte(ctx, pms->op_override);
    }
  if (pms->rex & REX_FIXED_BITS)
    {
      uint8_t rex = *(uint8_t*)&pms->rex;
      encode_ctx_push_byte(ctx, rex);
    }
}

static void encode_modrm_sib(ctx, pms)
  ju87_encode_ctx *ctx;
  prefix_modrm_sib *pms;
{
  if (pms->has_modrm)
    {
      uint8_t modrm = *(uint8_t*)&pms->modrm;
      encode_ctx_push_byte(ctx, modrm);
    }
  if (pms->has_sib)
    {
      uint8_t sib = *(uint8_t*)&pms->sib;
      encode_ctx_push_byte(ctx, sib);
    }
}

static void clr_pms(pms)
  prefix_modrm_sib *pms;
{
  memset(pms, 0, sizeof(prefix_modrm_sib));
}

static bool make_pms_imm_reg(pms, imm, reg)
  prefix_modrm_sib *pms;
  ju87_imm imm;
  ju87_reg reg;
{
  if (reg.type == JU87_REG_16BIT)
    {
      /* operand override prefix for 16bit dest register */
      pms->op_override = 0x66;
      pms->has_op_override = true;
    }

  if (reg.type == JU87_REG_64BIT)
    {
      pms->rex |= (REX_FIXED_BITS | REX_W_BIT);
      if (JU87_REG_IS_EXTEND(reg))
        {
          pms->rex |= REX_B_BIT;
        }

      if (imm.width == JU87_W_QWORD)
        {
          /* do nothing for MOVABSQ */
        }
      else
        {
          pms->has_modrm = true;
          /* 0x3 = 0b11, register addressing */
          pms->modrm |= MOD_BITS_REG;
          pms->modrm |= MOD_RM_BITS(JU87_REG_NORMALIZE(reg));
        }
    }

  return true;
}

static uint8_t scale_log2(scale)
  ju87_scale scale;
{
  switch (scale)
    {
      case JU87_SCALE_0:
      case JU87_SCALE_1: return 0;
      case JU87_SCALE_2: return 1;
      case JU87_SCALE_4: return 2;
      case JU87_SCALE_8: return 3;
      default: assert(false);
    }
}

static bool make_pms_mem(pms, mem)
  prefix_modrm_sib *pms;
  ju87_mem mem;
{
  ju87_reg base = mem.base;
  ju87_reg index = mem.index;
  ju87_scale scale = mem.scale;
  uint32_t offset = mem.offset;
  if (base.type && index.type && base.type != index.type)
    {
      /* base-index size mismatch */
      return false;
    }

  if (base.type && base.type < JU87_REG_32BIT)
    {
      /* 8bit/16bit registers cannot be used */
      return false;
    }

  if (base.value == JU87_BP)
    {
      /* not supported yet because I don't know how to organize code */
      return false;
    }

  if (base.type == JU87_REG_32BIT)
    {
      /* operand override prefix for 32bit base/index */
      pms->has_op_override = true;
      pms->op_override = 0x67;
    }

  if (base.type == JU87_REG_64BIT)
    {
      pms->rex |= (REX_FIXED_BITS | REX_W_BIT);
    }

  if (JU87_REG_IS_EXTEND(base))
    {
      pms->rex |= (REX_FIXED_BITS | REX_B_BIT);
    }

  pms->has_modrm = true;
  if (index.type || base.value == JU87_SP || offset)
    {
      /* use SIP mode */
      pms->modrm |= MOD_RM_BITS(0x4);

      if (!scale)
        {
          scale = JU87_SCALE_1;
        }
      pms->has_sib = true;

      if (base.type)
        {
          /* base, base + offset, or (base + index * scale) + offset */
          if (JU87_REG_IS_EXTEND(index))
            {
              pms->rex |= (REX_FIXED_BITS | REX_X_BIT);
            }

          pms->sib |= SIB_BASE_BITS(JU87_REG_NORMALIZE(base));
          if (index.type)
            {
              pms->sib |= SIB_INDEX_BITS(JU87_REG_NORMALIZE(index));
            }
          else
            {
              assert(base.value == JU87_SP);
              pms->sib |= SIB_INDEX_BITS(JU87_REG_NORMALIZE(base));
            }
        }
      else
        {
          /* absolute addressing */
          pms->sib |= (SIB_INDEX_BITS(0x04) | SIB_BASE_BITS(0x05));
        }

      pms->sib |= SIB_SCALE_BITS(scale_log2(scale));
    }
  else
    {
      /* modrm only */
      pms->modrm |= MOD_RM_BITS(JU87_REG_NORMALIZE(base));
    }

  if (offset)
    {
      if (offset <= UINT8_MAX)
        {
          pms->modrm |= MOD_BITS_BASE_DISP8;
        }
      else
        {
          pms->modrm |= MOD_BITS_BASE_DISP32;
        }
    }
  else
    {
      pms->modrm |= MOD_BITS_BASE_ONLY;
    }

  return true;
}

static bool make_pms_reg_mem(pms, reg, mem)
  prefix_modrm_sib *pms;
  ju87_reg reg;
  ju87_mem mem;
{
  if (!make_pms_mem(pms, mem))
    {
      return false;
    }

  if (reg.type == JU87_REG_64BIT)
    {
      pms->rex |= (REX_FIXED_BITS | REX_W_BIT);
      if (JU87_REG_IS_EXTEND(reg))
        {
          pms->rex |= REX_R_BIT;
        }
    }
  pms->modrm |= MOD_REG_BITS(reg.value);
  return true;
}

static bool make_pms_imm_mem(pms, imm, mem)
  prefix_modrm_sib *pms;
  ju87_imm imm;
  ju87_mem mem;
{
  (void)imm;
  return make_pms_mem(pms, mem);
}

bool ju87_mov(ctx, src, dst)
  ju87_encode_ctx *ctx;
  ju87_addr src;
  ju87_addr dst;
{
  if (src.mode == JU87_ADDR_IMM)
    {
      if (dst.mode == JU87_ADDR_REG)
        {
          return ju87_mov_imm_reg(ctx, src.value.imm, dst.value.reg);
        }
      else if (dst.mode == JU87_ADDR_MEM)
        {
          return ju87_mov_imm_mem(ctx, src.value.imm, dst.value.mem);
        }
    }
  else if (src.mode == JU87_ADDR_REG && dst.mode == JU87_ADDR_MEM)
    {
      return ju87_mov_reg_mem(ctx, src.value.reg, dst.value.mem);
    }
  else if (src.mode == JU87_ADDR_MEM && dst.mode == JU87_ADDR_REG)
    {
      return ju87_mov_mem_reg(ctx, src.value.mem, dst.value.reg);
    }
  else if (src.mode == JU87_ADDR_REG && dst.mode == JU87_ADDR_REG)
    {
      return ju87_mov_reg(ctx, src.value.reg, dst.value.reg);
    }

  return false;
}

bool ju87_mov_imm_reg(ctx, src, dst)
  ju87_encode_ctx *ctx;
  ju87_imm src;
  ju87_reg dst;
{
  prefix_modrm_sib pms;
  clr_pms(&pms);
  make_pms_imm_reg(&pms, src, dst);

  if (dst.type == JU87_REG_64BIT)
    {
      if (src.width == JU87_W_QWORD)
        {
          /* generate MOVABSQ */
          encode_prefix(ctx, &pms);
          encode_ctx_push_byte(ctx, 0xb8 + JU87_REG_NORMALIZE(dst));
          /* there shouldn't be modrm or SIB, grass, go, ignore! */
          encode_ctx_push_qword(ctx, src.imm_value);
        }
      else
        {
          /* generate conventional MOVQ */
          encode_prefix(ctx, &pms);
          encode_ctx_push_byte(ctx, 0xc7);
          encode_modrm_sib(ctx, &pms);
          encode_ctx_push_dword(ctx, (uint32_t)src.imm_value);
        }
    }
  else
    {
      if (src.width != (ju87_width)dst.type)
        {
          /* size mismatch */
          return false;
        }

      uint8_t opcode;
      switch (src.width)
        {
          case JU87_W_BYTE:
            opcode = 0xb0;
            break;
          case JU87_W_WORD:
          case JU87_W_DWORD:
            opcode = 0xb8;
            break;
          default:
            assert(false);
        }

      encode_prefix(ctx, &pms);
      encode_ctx_push_byte(ctx, opcode + dst.value);
      /* there shouldn't be any modrm or SIB, grass, go, ignore! */
      switch (src.width)
        {
          case JU87_W_BYTE:
            encode_ctx_push_byte(ctx, (uint8_t)src.imm_value);
            break;
          case JU87_W_WORD:
            encode_ctx_push_word(ctx, (uint16_t)src.imm_value);
            break;
          case JU87_W_DWORD:
            encode_ctx_push_dword(ctx, (uint32_t)src.imm_value);
            break;
          default:
            assert(false);
        }
    }

  return true;
}

bool ju87_mov_imm_mem(ctx, imm, mem)
  ju87_encode_ctx *ctx;
  ju87_imm imm;
  ju87_mem mem;
{
  if (imm.width == JU87_W_QWORD)
    {
      /* not allowed, use MOVABSQ first and then MOVQ */
      return false;
    }

  prefix_modrm_sib pms;
  clr_pms(&pms);
  if (!make_pms_imm_mem(&pms, imm, mem))
    {
      return false;
    }

  encode_prefix(ctx, &pms);

  switch (imm.width)
    {
      case JU87_W_BYTE:
        encode_ctx_push_byte(ctx, 0xC6);
        break;
      case JU87_W_WORD:
      case JU87_W_DWORD:
        encode_ctx_push_byte(ctx, 0xC7);
        break;
      default:
        assert(false);
    }

  encode_modrm_sib(ctx, &pms);
  if (mem.offset)
    {
      encode_ctx_push_dword(ctx, mem.offset);
    }

  switch (imm.width)
    {
      case JU87_W_BYTE:
        encode_ctx_push_byte(ctx, (uint8_t)imm.imm_value);
        break;
      case JU87_W_WORD:
        encode_ctx_push_word(ctx, (uint16_t)imm.imm_value);
        break;
      case JU87_W_DWORD:
        encode_ctx_push_dword(ctx, (uint32_t)imm.imm_value);
        break;
      default:
        assert(false);
    }

  return true;
}

bool ju87_mov_reg_mem(ctx, reg, mem)
  ju87_encode_ctx *ctx;
  ju87_reg reg;
  ju87_mem mem;
{
  prefix_modrm_sib pms;
  clr_pms(&pms);
  if (!make_pms_reg_mem(&pms, reg, mem))
    {
      return false;
    }

  encode_prefix(ctx, &pms);
  switch (reg.type)
    {
      case JU87_REG_8BIT:
        encode_ctx_push_byte(ctx, 0x88);
        break;
      case JU87_REG_16BIT:
      case JU87_REG_32BIT:
      case JU87_REG_64BIT:
        encode_ctx_push_byte(ctx, 0x89);
        break;
      default:
        assert(false);
    }

  encode_modrm_sib(ctx, &pms);
  if (mem.offset)
    {
      if (mem.offset <= UINT8_MAX)
        {
          encode_ctx_push_byte(ctx, (uint8_t)mem.offset);
        }
      else
        {
          encode_ctx_push_dword(ctx, mem.offset);
        }
    }

  return true;
}

bool ju87_mov_mem_reg(ctx, mem, reg)
  ju87_encode_ctx *ctx;
  ju87_mem mem;
  ju87_reg reg;
{
  prefix_modrm_sib pms;
  clr_pms(&pms);
  if (!make_pms_reg_mem(&pms, reg, mem))
    {
      return false;
    }

  encode_prefix(ctx, &pms);
  switch (reg.type)
    {
      case JU87_REG_8BIT:
        encode_ctx_push_byte(ctx, 0x8a);
      break;
      case JU87_REG_16BIT:
      case JU87_REG_32BIT:
      case JU87_REG_64BIT:
        encode_ctx_push_byte(ctx, 0x8b);
      break;
      default:
        assert(false);
    }

  encode_modrm_sib(ctx, &pms);
  if (mem.offset)
    {
      if (mem.offset <= UINT8_MAX)
        {
          encode_ctx_push_byte(ctx, (uint8_t)mem.offset);
        }
      else
        {
          encode_ctx_push_dword(ctx, mem.offset);
        }
    }

  return true;
}

bool ju87_mov_reg(ju87_encode_ctx *ctx, ju87_reg src, ju87_reg dst) {}

void ju87_repz_ret(ctx)
  ju87_encode_ctx *ctx;
{
  encode_ctx_push_byte(ctx, 0xF3);
  encode_ctx_push_byte(ctx, 0xC3);
}

void ju87_dbg_dump_buf(ctx)
  ju87_encode_ctx *ctx;
{
  size_t i;
  for (i = 0; i < ctx->offset; i++)
    {
      uint8_t byte = ctx->buf[i];
      fprintf(stderr, "%02x ", byte);
    }
  fputc('\n', stderr);
}

void ju87_dbg_clr_buf(ctx)
  ju87_encode_ctx *ctx;
{
  ctx->offset = 0;
  memset(ctx->buf, 0, ctx->bufsiz);
}
