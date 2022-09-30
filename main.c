#include <stdio.h>
#include "encode.h"

main(argc, argv)
  int argc;
  char **argv;
{
  ju87_encode_ctx ctx;
  ju87_encode_ctx_init(&ctx, 8192, JU87_ENDIAN_LITTLE, false);

  /* movabsq $1145141919810, %rax */
  ju87_mov_imm_reg(&ctx,
                   ju87_addr_imm(0xA0B0C0D010203040ULL, JU87_W_QWORD).value.imm,
                   JU87_REG64(JU87_RAX));
  /* repz ret */
  ju87_repz_ret(&ctx);

  size_t i;
  for (i = 0; i < ctx.offset; i++)
    {
      uint8_t byte = ctx.buf[i];
      fprintf(stderr, "%02x ", byte);
    }

  return 0;
}
