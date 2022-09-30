#include "addr.h"

#include <assert.h>

ju87_addr ju87_addr_imm(imm, width)
  uint64_t imm;
  ju87_width width;
{
  ju87_addr ret;
  ret.mode = JU87_ADDR_IMM;
  ret.value.imm.imm_value = imm;
  ret.value.imm.width = width;
  return ret;
}

ju87_addr ju87_addr_reg(reg)
  ju87_reg reg;
{
  assert(reg.type);

  ju87_addr ret;
  ret.mode = JU87_ADDR_REG;
  ret.value.reg = reg;
  return ret;
}

ju87_addr ju87_addr_mem_absl(absolute)
  uint32_t absolute;
{
  ju87_addr ret;
  ret.mode = JU87_ADDR_MEM;
  ret.value.mem.base = JU87_NO_REG;
  ret.value.mem.index = JU87_NO_REG;
  ret.value.mem.scale = 0;
  ret.value.mem.offset = absolute;
  return ret;
}

ju87_addr ju87_addr_conv(base, index, scale, offset)
  ju87_reg base;
  ju87_reg index;
  uint8_t scale;
  uint32_t offset;
{
  assert(base.type);
  assert(scale <= 8 && (scale == 1 || scale % 2 == 0));

  if (index.type && scale == 0) {
    scale = 1;
  }

  ju87_addr ret;
  ret.mode = JU87_ADDR_MEM;
  ret.value.mem.base = base;
  ret.value.mem.index = index;
  ret.value.mem.scale = scale;
  ret.value.mem.offset = offset;
  return ret;
}
