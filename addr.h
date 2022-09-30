#ifndef JU87_ADDR_H
#define JU87_ADDR_H

#include <stdint.h>
#include <stdbool.h>

#include "register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  JU87_ADDR_IMM,
  JU87_ADDR_REG,
  JU87_ADDR_MEM
} ju87_addr_mode;

typedef enum
{
  JU87_SCALE_0  = 0,
  JU87_SCALE_1  = 1,
  JU87_SCALE_2  = 2,
  JU87_SCALE_4  = 4,
  JU87_SCALE_8  = 8
} ju87_scale;

typedef enum {
  JU87_W_BYTE = JU87_REG_8BIT,
  JU87_W_WORD = JU87_REG_16BIT,
  JU87_W_DWORD = JU87_REG_32BIT,
  JU87_W_QWORD = JU87_REG_64BIT
} ju87_width;

typedef struct
{
  uint64_t imm_value;
  ju87_width width;
} ju87_imm;

typedef struct
{
  ju87_reg base;
  ju87_reg index;
  ju87_scale scale;
  uint32_t offset;
} ju87_mem;

typedef struct
{
  union
  {
    ju87_imm imm;
    ju87_reg reg;
    ju87_mem mem;
  } value;
  ju87_addr_mode mode;
} ju87_addr;

ju87_addr ju87_addr_imm(uint64_t imm, ju87_width width);
ju87_addr ju87_addr_reg(ju87_reg reg);
ju87_addr ju87_addr_mem_absl(uint32_t absolute);
ju87_addr ju87_addr_mem_conv(ju87_reg base,
                             ju87_reg index,
                             uint8_t scale,
                             uint32_t offset);

#define JU87_ADDR_IS_IMM(ADDR) (JU87_ADDR_IMM == (ADDR).mode)
#define JU87_ADDR_IS_REG(ADDR) (JU87_ADDR_REG == (ADDR).mode)
#define JU87_ADDR_IS_MEM(ADDR) (JU87_ADDR_MEM == (ADDR).mode)
#define JU87_ADDR_IS_ABSL(ADDR) \
  (JU87_ADDR_IS_MEM(ADDR) \
   && (!(ADDR).value.conv.base.type) \
   && (!(ADDR).value.conv.index.type) \
   && (!(ADDR).value.conv.scale) \
   && (ADDR).value.conv.offset)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JU87_ADDR_H */
