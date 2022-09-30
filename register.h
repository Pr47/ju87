#ifndef JU87_REGISTER_H
#define JU87_REGISTER_H

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  JU87_AL = 0,
  JU87_CL,
  JU87_DL,
  JU87_BL,
  JU87_AH,
  JU87_CH,
  JU87_DH,
  JU87_BH
} ju87_reg8;

typedef enum
{
  JU87_AX = 0,
  JU87_CX,
  JU87_DX,
  JU87_BX,
  JU87_SP,
  JU87_BP,
  JU87_SI,
  JU87_DI
} ju87_reg16;

typedef enum
{
  JU87_EAX = 0,
  JU87_ECX,
  JU87_EDX,
  JU87_EBX,
  JU87_ESP,
  JU87_EBP,
  JU87_ESI,
  JU87_EDI
} ju87_reg32;

typedef enum
{
  JU87_RAX = 0,
  JU87_RCX,
  JU87_RDX,
  JU87_RBX,
  JU87_RSP,
  JU87_RBP,
  JU87_RSI,
  JU87_RDI,
  JU87_R8 = 8,
  JU87_R9,
  JU87_R10,
  JU87_R11,
  JU87_R12,
  JU87_R13,
  JU87_R14,
  JU87_R15
} ju87_reg64;

typedef enum
{
  JU87_REG_NONE = 0,
  JU87_REG_8BIT,
  JU87_REG_16BIT,
  JU87_REG_32BIT,
  JU87_REG_64BIT
} ju87_reg_type;

typedef struct
{
  ju87_reg_type type : 3;
  unsigned value : 5;
} ju87_reg;

#define JU87_NO_REG ((ju87_reg) { JU87_REG_NONE, 0 })

#define JU87_REG8(REG) ((ju87_reg) { JU87_REG_8BIT, (REG) })
#define JU87_REG16(REG) ((ju87_reg) { JU87_REG_16BIT, (REG) })
#define JU87_REG32(REG) ((ju87_reg) { JU87_REG_32BIT, (REG) })
#define JU87_REG64(REG) ((ju87_reg) { JU87_REG_64BIT, (REG) })

#define JU87_REG_IS_EXTEND(REG) ((REG).value >= JU87_R8)
#define JU87_REG_NORMALIZE(REG) ((REG).value % JU87_R8)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JU87_REGISTER_H */
