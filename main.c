#include <stdio.h>
#include "encode.h"

main(argc, argv)
  int argc;
  char **argv;
{
  ju87_encode_ctx ctx;
  ju87_encode_ctx_init(&ctx, 8192, JU87_ENDIAN_LITTLE, false);

  ju87_mov_reg_mem(&ctx,
                   JU87_REG64(JU87_RAX),
                   (ju87_mem) { JU87_REG64(JU87_RCX), JU87_REG64(JU87_RDX), 4, 360 });

  ju87_dbg_dump_buf(&ctx);
  ju87_dbg_clr_buf(&ctx);

  ju87_mov_reg_mem(&ctx,
                   JU87_REG64(JU87_R12),
                   (ju87_mem) { JU87_REG64(JU87_R8), JU87_REG64(JU87_R9), 8, 60 });
  ju87_dbg_dump_buf(&ctx);
  ju87_dbg_clr_buf(&ctx);

  return 0;
}
