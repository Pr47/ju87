#ifndef JU87_ENCODE_H
#define JU87_ENCODE_H

#include <stdbool.h>
#include <stddef.h>

#include "addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  JU87_ENDIAN_BIG    = 0,
  JU87_ENDIAN_LITTLE = 1
} ju87_endian;

typedef struct
{
  unsigned char *buf;
  size_t offset;
  size_t bufsiz;

  ju87_endian endian: 1;
  bool verbose : 1;
} ju87_encode_ctx;

void ju87_encode_ctx_init(ju87_encode_ctx *ctx,
                          size_t bufsiz_init,
                          ju87_endian endian,
                          bool verbose);
void ju87_encode_ctx_deinit(ju87_encode_ctx *ctx);

bool ju87_mov(ju87_encode_ctx *ctx, ju87_addr src, ju87_addr dst);

bool ju87_mov_imm_reg(ju87_encode_ctx *ctx, ju87_imm src, ju87_reg dst);

bool ju87_mov_imm_mem(ju87_encode_ctx *ctx, ju87_imm imm, ju87_mem mem);

bool ju87_mov_reg_mem(ju87_encode_ctx *ctx, ju87_reg reg, ju87_mem mem);

bool ju87_mov_mem_reg(ju87_encode_ctx *ctx, ju87_mem mem, ju87_reg reg);

bool ju87_mov_reg(ju87_encode_ctx *ctx, ju87_reg src, ju87_reg dst);

void ju87_repz_ret(ju87_encode_ctx *ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JU87_ENCODE_H */
