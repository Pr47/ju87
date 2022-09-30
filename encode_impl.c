#include "encode.h"

#include <string.h>

static void _NP_incbufsz(ctx, expected_size)
  ju87_encode_ctx *ctx;
  size_t expected_size;
{
  if (ctx->bufsiz - ctx->offset < expected_size)
    {
      size_t new_bufsiz = ctx->bufsiz * 2;
      void *new_buf = malloc(new_bufsiz);
      memcpy(new_buf, ctx->buf, ctx->bufsiz);

      free(ctx->buf);
      ctx->buf = new_buf;
      ctx->bufsiz = new_bufsiz;
    }
}

static void encode_ctx_push_byte(ctx, byte)
  ju87_encode_ctx *ctx;
  unsigned char byte;
{
  _NP_incbufsz(ctx, 1);

  ctx->buf[ctx->offset] = byte;
  ctx->offset += 1;
}

static void encode_ctx_push_word(ctx, word)
  ju87_encode_ctx *ctx;
  unsigned short word;
{
  _NP_incbufsz(ctx, 2);

  unsigned char high_byte = word >> 8;
  unsigned char low_byte = word;

  if (ctx->endian == JU87_ENDIAN_BIG)
    {
      ctx->buf[ctx->offset++] = high_byte;
      ctx->buf[ctx->offset++] = low_byte;
    }
  else
    {
      ctx->buf[ctx->offset++] = low_byte;
      ctx->buf[ctx->offset++] = high_byte;
    }
}

static unsigned short _NP_detector = 0xFFFE;
#define _NP_detect_endian \
  ( \
    (*(unsigned char*)(&_NP_detector) == 0xFF) ? \
      JU87_ENDIAN_LITTLE : \
      JU87_ENDIAN_BIG \
  )

static void _NP_push_mulibyte(ctx, bytes, cnt)
  ju87_encode_ctx *ctx;
  uint8_t *bytes;
  size_t cnt;
{
  _NP_incbufsz(ctx, cnt);
  size_t i;
  if (ctx->endian != _NP_detect_endian)
    {
      for (i = 0; i < cnt; i++)
        {
          ctx->buf[ctx->offset + i] = bytes[i];
        }
    }
  else
    {
      for (i = 0; i < cnt; i++)
        {
          ctx->buf[ctx->offset + i] = bytes[cnt - i - 1];
        }
    }
  ctx->offset += cnt;
}

static void encode_ctx_push_dword(ctx, dword)
  ju87_encode_ctx *ctx;
  uint32_t dword;
{
  _NP_push_mulibyte(ctx, (uint8_t*)&dword, 4);
}

static void encode_ctx_push_qword(ctx, qword)
  ju87_encode_ctx *ctx;
  uint64_t qword;
{
  _NP_push_mulibyte(ctx, (uint8_t*)&qword, 8);
}
