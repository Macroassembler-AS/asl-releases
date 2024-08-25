/* strutil.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* haeufig benoetigte String-Funktionen                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "dynstr.h"
#include "striter.h"
#include "strutil.h"
#undef strlen   /* VORSICHT, Rekursion!!! */

char HexStartCharacter;	    /* characters to use for 10,11,...35 */
char SplitByteCharacter;    /* output large numbers per-byte with given split char */

/*--------------------------------------------------------------------------*/
/* eine bestimmte Anzahl Leerzeichen liefern */

const char *Blanks(int cnt)
{
  static const char *BlkStr = "                                                                                                           ";
  static int BlkStrLen = 0;

  if (!BlkStrLen)
    BlkStrLen = strlen(BlkStr);

  if (cnt < 0)
    cnt = 0;
  if (cnt > BlkStrLen)
    cnt = BlkStrLen;

  return BlkStr + (BlkStrLen - cnt);
}

/*!------------------------------------------------------------------------
 * \fn     SysString(char *pDest, size_t DestSize, LargeWord i, int System, int Stellen, Boolean ForceLeadZero, char StartCharacter, char SplitCharacter)
 * \brief  convert number to string in given number system, leading zeros
 * \param  pDest where to write
 * \param  DestSize size of dest buffer
 * \param  i number to convert
 * \param  Stellen minimum length of output
 * \param  ForceLeadZero prepend zero if first character is no number
 * \param  System number system
 * \param  StartCharacter 'a' or 'A' for hex digits
 * \param  SplitCharacter split bytes if not NUL
 * ------------------------------------------------------------------------ */

char *SysStringCore(char *pDest, char *pDestCurr, LargeWord Num, int System, int Stellen, char StartCharacter)
{
  LargeWord Digit;

  do
  {
    if (pDestCurr <= pDest)
      break;
    Digit = Num % System;
    if (Digit < 10)
      *(--pDestCurr) = Digit + '0';
    else
      *(--pDestCurr) = Digit - 10 + StartCharacter;
    Num /= System;
    Stellen--;
  }
  while ((Stellen > 0) || Num);
  return pDestCurr;
}

int SysString(char *pDest, size_t DestSize, LargeWord Num, int System, int Stellen, Boolean ForceLeadZero, char StartCharacter, char SplitCharacter)
{
  int Len = 0;
  char *pDestCurr, *pDestNext;

  if (DestSize < 1)
    return 0;

  if (Stellen > (int)DestSize - 1)
    Stellen = DestSize - 1;

  pDestCurr = pDest + DestSize - 1;
  *pDestCurr = '\0';
  if (SplitCharacter)
  {
    LargeWord Part;
    int ThisLen;
    static int SystemByteLen[37];

    if (!SystemByteLen[System])
    {
      char Dummy[50];

      SystemByteLen[System] = SysString(Dummy, sizeof(Dummy), 0xff, System, 0, False, StartCharacter, False);
    }

    do
    {
      Part = Num % 256;
      Num = Num / 256;
      pDestNext = SysStringCore(pDest, pDestCurr, Part, System, Num ? SystemByteLen[System] : Stellen, StartCharacter);
      ThisLen = pDestCurr - pDestNext;
      Len += ThisLen;
      pDestCurr = pDestNext;
      Stellen -= ThisLen;
      if (Num)
      {
        if (pDestCurr <= pDest)
          break;
        *(--pDestCurr) = SplitCharacter;
        Len++;
      }
    }
    while ((Stellen > 0) || Num);
  }
  else
  {
    pDestNext = SysStringCore(pDest, pDestCurr, Num, System, Stellen, StartCharacter);
    Len += pDestCurr - pDestNext;
    pDestCurr = pDestNext;
  }

  if (ForceLeadZero && !isdigit(*pDestCurr) && (pDestCurr > pDest))
  {
    *(--pDestCurr) = '0';
    Len++;
  }

  if (pDestCurr != pDest)
    strmov(pDest, pDestCurr);
  return Len;
}

/*---------------------------------------------------------------------------*/
/* strdup() is not part of ANSI C89 */

char *as_strdup(const char *s)
{
  char *ptr;

  if (!s)
    return NULL;
  ptr = (char *) malloc(strlen(s) + 1);
#ifdef CKMALLOC
  if (!ptr)
  {
    fprintf(stderr, "strdup: out of memory?\n");
    exit(255);
  }
#endif
  if (ptr != 0)
    strcpy(ptr, s);
  return ptr;
}
/*---------------------------------------------------------------------------*/
/* ...so is snprintf... */

typedef struct
{
  char *p_dest;
  size_t dest_remlen;
  as_dynstr_t *p_dynstr;
} dest_format_context_t;

/*!------------------------------------------------------------------------
 * \fn     as_format_context_reset(as_format_ctx_t *p_context)
 * \brief  reset format context to idle (not within format spec)
 * \param  p_context context to reset
 * ------------------------------------------------------------------------ */

void as_format_context_reset(as_format_ctx_t *p_context)
{
  int z;

  for (z = 0; z < 3; z++)
  {
    p_context->arg[z] = 0;
    p_context->arg_state[z] = e_not_set;
  }
  p_context->curr_arg = 0;
  p_context->int_size = 0;
  p_context->in_format =
  p_context->lead_zero =
  p_context->force_lead_zero =
  p_context->is_signed =
  p_context->left_align =
  p_context->add_plus =
  p_context->force_upper = False;
}

/*!------------------------------------------------------------------------
 * \fn     as_format_context_consume(as_format_ctx_t *p_context, char ch)
 * \brief  core of updating format context by next character
 * \param  p_context context to update
 * \param  ch next character in format string
 * \return True if character consumed
 * ------------------------------------------------------------------------ */

Boolean as_format_context_consume(as_format_ctx_t *p_context, char ch)
{
  if (p_context->in_format)
    switch (ch)
    {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      {
        if (!p_context->curr_arg && !p_context->arg_state[p_context->curr_arg] && (ch == '0'))
          p_context->lead_zero = True;
        p_context->arg[p_context->curr_arg] = (p_context->arg[p_context->curr_arg] * 10) + (ch - '0');
        p_context->arg_state[p_context->curr_arg] = e_set;
        return True;
      }
      case '-':
        if (!p_context->curr_arg && !p_context->arg_state[p_context->curr_arg])
          p_context->left_align = True;
        return True;
      case '+':
        p_context->add_plus = True;
        return True;
      case '~':
        p_context->force_lead_zero = True;
        return True;
      case '.':
        if (p_context->curr_arg < 3)
          p_context->curr_arg++;
        return True;
      default:
        return False;
    }
  else
    return (p_context->in_format = (ch == '%'));
}

/*!------------------------------------------------------------------------
 * \fn     limit_minus_one(dest_format_context_t *p_dest_ctx, size_t cnt)
 * \brief  check if space is left to append given # of characters, plus trailing NUL
 * \param  p_dest_ctx destination context
 * \param  cnt requested # of characters to append
 * \return actual # that can be appended
 * ------------------------------------------------------------------------ */

static size_t limit_minus_one(dest_format_context_t *p_dest_ctx, size_t cnt)
{
  /* anyway still enough space? */

  if (p_dest_ctx->dest_remlen > cnt)
    return cnt;

  /* not enough space: try to realloc dynamic string dest */

  if (p_dest_ctx->p_dynstr)
  {
    size_t curr_len = p_dest_ctx->p_dest - p_dest_ctx->p_dynstr->p_str;
    size_t new_capacity = as_dynstr_roundup_len(curr_len + cnt + 1);

    /* if realloc successful, pointer into string buffer must be adapted: */

    if (!as_dynstr_realloc(p_dest_ctx->p_dynstr, new_capacity))
    {
      p_dest_ctx->p_dest = p_dest_ctx->p_dynstr->p_str + curr_len;
      p_dest_ctx->dest_remlen = p_dest_ctx->p_dynstr->capacity - curr_len;
    }
  }

  /* pathological case... */

  if (!p_dest_ctx->dest_remlen)
    return 0;

  /* truncation */

  else
    return (cnt >= p_dest_ctx->dest_remlen) ? p_dest_ctx->dest_remlen - 1 : cnt;
}

/*!------------------------------------------------------------------------
 * \fn     append_pad(dest_format_context_t *p_dest_ctx, char src, size_t cnt)
 * \brief  append given character n times
 * \param  p_dest_ctx destination context
 * \param  src character to append
 * \param  cnt # of times to append
 * \return actual # of characters appended
 * ------------------------------------------------------------------------ */

static size_t append_pad(dest_format_context_t *p_dest_ctx, char src, size_t cnt)
{
  cnt = limit_minus_one(p_dest_ctx, cnt);

  if (cnt > 0)
  {
    memset(p_dest_ctx->p_dest, src, cnt);
    p_dest_ctx->p_dest += cnt;
    p_dest_ctx->dest_remlen -= cnt;
  }
  return cnt;
}

#if 0
static int FloatConvert(char *pDest, size_t DestSize, as_float_t Src, int Digits, Boolean TruncateTrailingZeros, char FormatType)
{
  int DecPt;
  int Sign, Result = 0;
  char *pBuf, *pEnd, *pRun;

  (void)FormatType;

  if (DestSize < Digits + 6)
  {
    *pDest = '\0';
    return Result;
  }

  if (Digits < 0)
    Digits = 6;

  pBuf = ecvt(Src, Digits + 1, &DecPt, &Sign);
  puts(pBuf);
  pEnd = pBuf + strlen(pBuf) - 1;
  if (TruncateTrailingZeros)
  {
    for (; pEnd > pBuf + 1; pEnd--)
      if (*pEnd != '0')
        break;
  }

  pRun = pDest;
  if (Sign)
    *pRun++ = '-';
  *pRun++ = *pBuf;
  *pRun++ = '.';
  memcpy(pRun, pBuf + 1, pEnd - pBuf); pRun += pEnd - pBuf;
  *pRun = '\0';
  Result = pRun - pDest;
  Result += as_snprintf(pRun, DestSize - Result, "e%+02d", DecPt - 1);
  return Result;
}
#else
static int FloatConvert(char *pDest, size_t DestSize, as_float_t Src, int Digits, Boolean TruncateTrailingZeros, char FormatType)
{
  char Format[10];
  size_t l;

  (void)DestSize;
  (void)TruncateTrailingZeros;
  strcpy(Format, "%0.*" XPRIas_float_t);
  l = strlen(Format);
  Format[l++] = (HexStartCharacter == 'a') ? FormatType : toupper(FormatType);
  Format[l] = '\0';
  sprintf(pDest, Format, Digits, Src);
  return strlen(pDest);
}
#endif

/*!------------------------------------------------------------------------
 * \fn     append(dest_format_context_t *p_dest_ctx, const char *p_src, size_t cnt, as_format_ctx_t *pFormatContext)
 * \brief  append given data, with possible left/right padding
 * \param  p_dest_ctx destination context
 * \param  p_src data to append
 * \param  cnt length of data to append
 * \param  pFormatContext formatting context
 * \return actual # of characters appended
 * ------------------------------------------------------------------------ */

static size_t append(dest_format_context_t *p_dest_ctx, const char *p_src, size_t cnt, as_format_ctx_t *pFormatContext)
{
  size_t pad_len, result = 0;

  pad_len = (pFormatContext->arg[0] > (int)cnt) ? pFormatContext->arg[0] - cnt : 0;

  if ((pad_len > 0) && !pFormatContext->left_align)
    result += append_pad(p_dest_ctx, ' ', pad_len);

  cnt = limit_minus_one(p_dest_ctx, cnt);
  if (cnt > 0)
  {
    memcpy(p_dest_ctx->p_dest, p_src, cnt);
    p_dest_ctx->p_dest += cnt;
    p_dest_ctx->dest_remlen -= cnt;
  }

  if ((pad_len > 0) && pFormatContext->left_align)
    result += append_pad(p_dest_ctx, ' ', pad_len);

  if (pFormatContext->in_format)
    as_format_context_reset(pFormatContext);

  return result + cnt;
}

/*!------------------------------------------------------------------------
 * \fn     vsprcatf_core(dest_format_context_t *p_dest_ctx, const char *pFormat, va_list ap)
 * \brief  The actual core routine to process the format string
 * \param  p_dest_ctx context describing destination
 * \param  pFormat format specifier
 * \param  ap format arguments
 * \return # of characters appended
 * ------------------------------------------------------------------------ */

static int vsprcatf_core(dest_format_context_t *p_dest_ctx, const char *pFormat, va_list ap)
{
  const char *pFormatStart = pFormat;
  int Result = 0;
  size_t OrigLen = strlen(p_dest_ctx->p_dest);
  as_format_ctx_t FormatContext;
  LargeWord UIntArg;
  Boolean UIntArgNeg;

  if (p_dest_ctx->dest_remlen > OrigLen)
    p_dest_ctx->dest_remlen -= OrigLen;
  else
    p_dest_ctx->dest_remlen = 0;
  p_dest_ctx->p_dest += OrigLen;

  as_format_context_reset(&FormatContext);
  for (; *pFormat; pFormat++)
    if (as_format_context_consume(&FormatContext, *pFormat));
    else if (FormatContext.in_format)
    {
      switch (*pFormat)
      {
        case '*':
          FormatContext.arg[FormatContext.curr_arg] = va_arg(ap, int);
          FormatContext.arg_state[FormatContext.curr_arg] = e_finished;
          break;
        case 'c':
        {
          char ch = va_arg(ap, int);

          Result += append(p_dest_ctx, &ch, 1, &FormatContext);
          break;
        }
        case '%':
          Result += append(p_dest_ctx, "%", 1, &FormatContext);
          break;
        case 'l':
        {
          FormatContext.int_size++;
          FormatContext.curr_arg = 2;
          break;
        }
        case 'd':
        {
          LargeInt IntArg;

          if (FormatContext.int_size >= 3)
            IntArg = va_arg(ap, LargeInt);
          else
#if AS_HAS_LONGLONG
          if (FormatContext.int_size >= 2)
            IntArg = va_arg(ap, long long);
          else
#endif
          if (FormatContext.int_size >= 1)
            IntArg = va_arg(ap, long);
          else
            IntArg = va_arg(ap, int);
          FormatContext.arg[1] = 10;
          FormatContext.is_signed = True;
          UIntArgNeg = (IntArg < 0);
          UIntArg = UIntArgNeg ? 0 - IntArg : IntArg;
          goto IntCommon;
        }
        case 'u':
        {
          if (FormatContext.int_size >= 3)
            UIntArg = va_arg(ap, LargeWord);
          else
#if AS_HAS_LONGLONG
          if (FormatContext.int_size >= 2)
            UIntArg = va_arg(ap, unsigned long long);
          else
#endif
          if (FormatContext.int_size >= 1)
            UIntArg = va_arg(ap, unsigned long);
          else
            UIntArg = va_arg(ap, unsigned);
          UIntArgNeg = False;
          goto IntCommon;
        }
        case 'x':
        case 'X':
        {
          if (FormatContext.int_size >= 3)
            UIntArg = va_arg(ap, LargeWord);
          else
#if AS_HAS_LONGLONG
          if (FormatContext.int_size >= 2)
            UIntArg = va_arg(ap, unsigned long long);
          else
#endif
          if (FormatContext.int_size)
            UIntArg = va_arg(ap, unsigned long);
          else
            UIntArg = va_arg(ap, unsigned);
          FormatContext.arg[1] = 16;
          UIntArgNeg = False;
          FormatContext.force_upper = as_isupper(*pFormat);
          goto IntCommon;
        }
        IntCommon:
        {
          char Str[100], *pStr = Str;
          int Cnt;
          int NumPadZeros = 0;

          if (FormatContext.is_signed)
          {
            if (UIntArgNeg)
              *pStr++ = '-';
            else if (FormatContext.add_plus)
              *pStr++ = '+';
          }
          if (FormatContext.lead_zero)
          {
            NumPadZeros = FormatContext.arg[0];
            FormatContext.arg[0] = 0;
          }
          Cnt = (pStr - Str)
              + SysString(pStr, sizeof(Str) - (pStr - Str), UIntArg,
                          FormatContext.arg[1] ? FormatContext.arg[1] : 10,
                          NumPadZeros, FormatContext.force_lead_zero,
                          FormatContext.force_upper ? 'A' : HexStartCharacter,
                          SplitByteCharacter);
          if (Cnt > (int)sizeof(Str))
            Cnt = sizeof(Str);
          Result += append(p_dest_ctx, Str, Cnt, &FormatContext);
          break;
        }
        case 'e':
        case 'f':
        case 'g':
        {
          char Str[100];
          int Cnt;

          Cnt = FloatConvert(Str, sizeof(Str),
                             (FormatContext.int_size >= 3) ? va_arg(ap, as_float_t) : va_arg(ap, double),
                             FormatContext.arg[1], False, *pFormat);
          if (Cnt > (int)sizeof(Str))
            Cnt = sizeof(Str);
          Result += append(p_dest_ctx, Str, Cnt, &FormatContext);
          break;
        }
        case 's':
        {
          const char *pStr = va_arg(ap, char*);
          size_t cnt = FormatContext.arg[1]
                     ? as_strnlen(pStr, FormatContext.arg[1])
                     : strlen(pStr);

          Result += append(p_dest_ctx, pStr, cnt, &FormatContext);
          break;
        }
        default:
          fprintf(stderr, "invalid format: '%c' in '%s'\n", *pFormat, pFormatStart);
          exit(255);
      }
    }
    else
      Result += append(p_dest_ctx, pFormat, 1, &FormatContext);

  if (p_dest_ctx->dest_remlen > 0)
    *(p_dest_ctx->p_dest++) = '\0';
  return Result;
}

/*!------------------------------------------------------------------------
 * \fn     as_vsdprcatf(as_dynstr_t *p_dest, const char *pFormat, va_list ap)
 * \brief  append to dynamic string by format
 * \param  p_dest string to be appended to
 * \param  pFormat format specifier
 * \param  ap format arguments
 * \return # of characters appended
 * ------------------------------------------------------------------------ */

int as_vsdprcatf(as_dynstr_t *p_dest, const char *pFormat, va_list ap)
{
  dest_format_context_t ctx;

  ctx.p_dest = p_dest->p_str;
  ctx.dest_remlen = p_dest->capacity;
  ctx.p_dynstr = p_dest;
  return vsprcatf_core(&ctx, pFormat, ap);
}

/*!------------------------------------------------------------------------
 * \fn     as_vsdprintf(as_dynstr_t *p_dest, const char *pFormat, va_list ap)
 * \brief  print to dynamic string by format
 * \param  p_dest string to be appended to
 * \param  pFormat format specifier
 * \param  ap format arguments
 * \return # of characters written
 * ------------------------------------------------------------------------ */

int as_vsdprintf(as_dynstr_t *p_dest, const char *pFormat, va_list ap)
{
  if (p_dest->capacity > 0)
    p_dest->p_str[0] = '\0';
  return as_vsdprcatf(p_dest, pFormat, ap);
}

/*!------------------------------------------------------------------------
 * \fn     as_sdprcatf(as_dynstr_t *p_dest, const char *pFormat, ...)
 * \brief  append to dynamic string by format
 * \param  p_dest string to be appended to
 * \param  pFormat format specifier
 * \param  ... format arguments
 * \return # of characters written
 * ------------------------------------------------------------------------ */

int as_sdprcatf(as_dynstr_t *p_dest, const char *pFormat, ...)
{
  va_list ap;
  int ret;

  va_start(ap, pFormat);
  ret = as_vsdprcatf(p_dest, pFormat, ap);
  va_end(ap);
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_sdprintf(as_dynstr_t *p_dest, const char *pFormat, ...)
 * \brief  print to dynamic string by format
 * \param  p_dest string to be appended to
 * \param  pFormat format specifier
 * \param  ... format arguments
 * \return # of characters written
 * ------------------------------------------------------------------------ */

int as_sdprintf(as_dynstr_t *p_dest, const char *pFormat, ...)
{
  va_list ap;
  int ret;

  va_start(ap, pFormat);
  ret = as_vsdprintf(p_dest, pFormat, ap);
  va_end(ap);
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_vsnprcatf(char *pDest, size_t DestSize, const char *pFormat, va_list ap)
 * \brief  append to string by format
 * \param  pDest string to be appended to
 * \param  DestSize capacity of string
 * \param  pFormat format specifier
 * \param  ap format arguments
 * \return # of characters appended
 * ------------------------------------------------------------------------ */

int as_vsnprcatf(char *pDest, size_t DestSize, const char *pFormat, va_list ap)
{
  dest_format_context_t ctx;

  if (DestSize == sizeof(char*))
  {
    fprintf(stderr, "pointer size passed to as_vsnprcatf\n");
    exit(2);
  }

  ctx.p_dest = pDest;
  ctx.dest_remlen = DestSize;
  ctx.p_dynstr = NULL;
  return vsprcatf_core(&ctx, pFormat, ap);
}

/*!------------------------------------------------------------------------
 * \fn     as_vsnprintf(char *pDest, size_t DestSize, const char *pFormat, va_list ap)
 * \brief  print to string by format
 * \param  pDest string to be appended to
 * \param  DestSize capacity of string
 * \param  pFormat format specifier
 * \param  ap format arguments
 * \return # of characters written
 * ------------------------------------------------------------------------ */

int as_vsnprintf(char *pDest, size_t DestSize, const char *pFormat, va_list ap)
{
  if (DestSize > 0)
    *pDest = '\0';
  return as_vsnprcatf(pDest, DestSize, pFormat, ap);
}

/*!------------------------------------------------------------------------
 * \fn     as_snprintf(char *pDest, size_t DestSize, const char *pFormat, ...)
 * \brief  print to string by format
 * \param  pDest string to be appended to
 * \param  DestSize capacity of string
 * \param  pFormat format specifier
 * \param  ... format arguments
 * \return # of characters written
 * ------------------------------------------------------------------------ */

int as_snprintf(char *pDest, size_t DestSize, const char *pFormat, ...)
{
  va_list ap;
  int Result;

  va_start(ap, pFormat);
  if (DestSize > 0)
    *pDest = '\0';
  Result = as_vsnprcatf(pDest, DestSize, pFormat, ap);
  va_end(ap);
  return Result;
}

/*!------------------------------------------------------------------------
 * \fn     as_snprcatf(char *pDest, size_t DestSize, const char *pFormat, ...)
 * \brief  append to string by format
 * \param  pDest string to be appended to
 * \param  DestSize capacity of string
 * \param  pFormat format specifier
 * \param  ... format arguments
 * \return # of characters appended
 * ------------------------------------------------------------------------ */

int as_snprcatf(char *pDest, size_t DestSize, const char *pFormat, ...)
{
  va_list ap;
  int Result;

  va_start(ap, pFormat);
  Result = as_vsnprcatf(pDest, DestSize, pFormat, ap);
  va_end(ap);
  return Result;
}

int as_strcasecmp(const char *src1, const char *src2)
{
  if (!src1)
    src1 = "";
  if (!src2)
    src2 = "";
  while (tolower(*src1) == tolower(*src2))
  {
    if ((!*src1) && (!*src2))
      return 0;
    src1++;
    src2++;
  }
  return ((int) tolower(*src1)) - ((int) tolower(*src2));
}	

int as_strncasecmp(const char *src1, const char *src2, size_t len)
{
  if (!src1)
    src1 = "";
  if (!src2)
    src2 = "";
  while (tolower(*src1) == tolower(*src2))
  {
    if (--len == 0)
      return 0;
    if ((!*src1) && (!*src2))
      return 0;
    src1++;
    src2++;
  }
  return ((int) tolower(*src1)) - ((int) tolower(*src2));
}	

#ifdef NEEDS_STRSTR
char *strstr(const char *haystack, const char *needle)
{
  int lh = strlen(haystack), ln = strlen(needle);
  int z;
  char *p;

  for (z = 0; z <= lh - ln; z++)
    if (strncmp(p = haystack + z, needle, ln) == 0)
      return p;
  return NULL;
}
#endif

/*!------------------------------------------------------------------------
 * \fn     strrmultchr(const char *haystack, const char *needles)
 * \brief  find the last occurence of either character in string
 * \param  haystack string to search in
 * \param  needles characters to search for
 * \return last occurence or NULL
 * ------------------------------------------------------------------------ */

char *strrmultchr(const char *haystack, const char *needles)
{
  const char *pPos;

  for (pPos = haystack + strlen(haystack) - 1; pPos >= haystack; pPos--)
    if (strchr(needles, *pPos))
      return (char*)pPos;
  return NULL;
}

/*---------------------------------------------------------------------------*/
/* das originale strncpy plaettet alle ueberstehenden Zeichen mit Nullen */

size_t strmaxcpy(char *dest, const char *src, size_t Max)
{
  size_t cnt = strlen(src);

  /* leave room for terminating NUL */

  if (!Max)
    return 0;
  if (cnt + 1 > Max)
    cnt = Max - 1;
  memcpy(dest, src, cnt);
  dest[cnt] = '\0';
  return cnt;
}

/*---------------------------------------------------------------------------*/
/* einfuegen, mit Begrenzung */

size_t strmaxcat(char *Dest, const char *Src, size_t MaxLen)
{
  int TLen = strlen(Src);
  size_t DLen = strlen(Dest);

  if (TLen > (int)MaxLen - 1 - (int)DLen)
    TLen = MaxLen - DLen - 1;
  if (TLen > 0)
  {
    memcpy(Dest + DLen, Src, TLen);
    Dest[DLen + TLen] = '\0';
    return DLen + TLen;
  }
  else
    return DLen;
}

void strprep(char *Dest, const char *Src)
{
  memmove(Dest + strlen(Src), Dest, strlen(Dest) + 1);
  memmove(Dest, Src, strlen(Src));
}

/*!------------------------------------------------------------------------
 * \fn     strmaxprep(char *p_dest, const char *p_src, size_t max_len)
 * \brief  prepend as much as possible from src to dest
 * \param  p_dest string to be prepended
 * \param  p_src string to prepend
 * \param  max_len capacity of p_dest
 * ------------------------------------------------------------------------ */

void strmaxprep(char *p_dest, const char *p_src, size_t max_len)
{
  size_t src_len = strlen(p_src),
         dest_len = strlen(p_dest);

  assert(dest_len + 1 <= max_len);
  if (src_len > max_len - dest_len - 1)
    src_len = max_len - dest_len - 1;
  memmove(p_dest + src_len, p_dest, dest_len + 1);
  memmove(p_dest, p_src, src_len);
}

/*!------------------------------------------------------------------------
 * \fn     strmaxprep2(char *p_dest, const char *p_src, size_t max_len)
 * \brief  prepend as much as possible from src to dest, and possibly truncate dest by that
 * \param  p_dest string to be prepended
 * \param  p_src string to prepend
 * \param  max_len capacity of p_dest
 * ------------------------------------------------------------------------ */

void strmaxprep2(char *p_dest, const char *p_src, size_t max_len)
{
  size_t src_len = strlen(p_src),
         dest_len = strlen(p_dest);

  assert(max_len > 0);
  if (src_len >= max_len)
    src_len = max_len - 1;
  max_len -= src_len;
  if (dest_len >= max_len)
    dest_len = max_len - 1;
  memmove(p_dest + src_len, p_dest, dest_len + 1);
  memmove(p_dest, p_src, src_len);
}

void strins(char *Dest, const char *Src, int Pos)
{
  memmove(Dest + Pos + strlen(Src), Dest + Pos, strlen(Dest) + 1 - Pos);
  memmove(Dest + Pos, Src, strlen(Src));
}

void strmaxins(char *Dest, const char *Src, int Pos, size_t MaxLen)
{
  size_t RLen;

  RLen = strlen(Src);
  if (RLen > MaxLen - strlen(Dest))
    RLen = MaxLen - strlen(Dest);
  memmove(Dest + Pos + RLen, Dest + Pos, strlen(Dest) + 1 - Pos);
  memmove(Dest + Pos, Src, RLen);
}

int strlencmp(const char *pStr1, unsigned Str1Len,
              const char *pStr2, unsigned Str2Len)
{
  const char *p1, *p2, *p1End, *p2End;
  int Diff;

  for (p1 = pStr1, p1End = p1 + Str1Len,
       p2 = pStr2, p2End = p2 + Str2Len;
       p1 < p1End && p2 < p2End; p1++, p2++)
  {
    Diff = ((int)*p1) - ((int)*p2);
    if (Diff)
      return Diff;
  }
  return ((int)Str1Len) - ((int)Str2Len);
}

unsigned fstrlenprint(FILE *pFile, const char *pStr, unsigned StrLen)
{
  unsigned Result = 0;
  const char *pRun, *pEnd;

  for (pRun = pStr, pEnd = pStr + StrLen; pRun < pEnd; pRun++)
    if ((*pRun == '\\') || (*pRun == '"') || (*pRun == ' ') || (!isprint(*pRun)))
    {
      fprintf(pFile, "\\%03d", *pRun);
      Result += 4;
    }
    else
    {
      fputc(*pRun, pFile);
      Result++;
    }

  return Result;
}

size_t as_strnlen(const char *pStr, size_t MaxLen)
{
  size_t Res = 0;

  for (; (MaxLen > 0); MaxLen--, pStr++, Res++)
    if (!*pStr)
      break;
  return Res;
}

/*!------------------------------------------------------------------------
 * \fn     strreplace(char *pHaystack, const char *pFrom, const char *pTo, size_t ToMaxLen, size_t HaystackSize)
 * \brief  replaces all occurences of From to To in Haystack
 * \param  pHaystack string to search in
 * \param  pFrom what to find
 * \param  pFrom what to find
 * \param  pTo what to replace it with
 * \param  ToMaxLen if not -1, max. length of pTo (not NUL-terminated)
 * \param  HaystackSize buffer capacity
 * \return # of occurences
 * ------------------------------------------------------------------------ */

int strreplace(char *pHaystack, const char *pFrom, const char *pTo, size_t ToMaxLen, size_t HaystackSize)
{
  int HaystackLen = -1, FromLen = -1, ToLen = -1, Count = 0;
  int HeadLen, TailLen;
  char *pSearch, *pPos;

  pSearch = pHaystack;
  while (True)
  {
    /* find an occurence */

    pPos = strstr(pSearch, pFrom);
    if (!pPos)
      return Count;

    /* compute some stuff upon first occurence when needed */

    if (FromLen < 0)
    {
      HaystackLen = strlen(pHaystack);
      FromLen = strlen(pFrom);
    }
    ToLen = (ToMaxLen > 0) ? as_strnlen(pTo, ToMaxLen) : strlen(pTo);

    /* See how much of the remainder behind 'To' still fits into buffer after replacement,
       and move accordingly: */

    HeadLen = pPos - pHaystack;
    TailLen = HaystackLen - HeadLen - FromLen;
    if (HeadLen + ToLen + TailLen >= (int)HaystackSize)
    {
      TailLen = HaystackSize - 1 - HeadLen - ToLen;
      if (TailLen < 0)
        TailLen = 0;
    }
    if (TailLen > 0)
      memmove(pPos + ToLen, pPos + FromLen, TailLen);

    /* See how much of 'To' still fits into buffer, and set accordingly: */

    if (HeadLen + ToLen >= (int)HaystackSize)
    {
      ToLen = HaystackSize - 1 - ToLen;
      if (ToLen < 0)
        ToLen = 0;
    }
    if (ToLen > 0)
      memcpy(pPos, pTo, ToLen);

    /* Update length & terminate new string */

    HaystackLen = HeadLen + ToLen + TailLen;
    pHaystack[HaystackLen] = '\0';

    /* continue searching behind replacement: */

    pSearch = &pHaystack[HeadLen + ToLen];

    Count++;
  }
}

/*---------------------------------------------------------------------------*/
/* Bis Zeilenende lesen */

void ReadLn(FILE *Datei, char *Zeile)
{
  char *ptr;
  int l;

  *Zeile = '\0';
  ptr = fgets(Zeile, 256, Datei);
  if ((!ptr) && (ferror(Datei) != 0))
    *Zeile = '\0';
  l = strlen(Zeile);
  if ((l > 0) && (Zeile[l - 1] == '\n'))
    Zeile[--l] = '\0';
  if ((l > 0) && (Zeile[l - 1] == '\r'))
    Zeile[--l] = '\0';
  if ((l > 0) && (Zeile[l - 1] == 26))
    Zeile[--l] = '\0';
}

#if 0

static void dump(const char *pLine, unsigned Cnt)
{
  unsigned z;

  fputc('\n', stderr);
  for (z = 0; z < Cnt; z++)
  {
    fprintf(stderr, " %02x", pLine[z]);
    if ((z & 15) == 15)
      fputc('\n', stderr);
  }
  fputc('\n', stderr);
}

#endif

/*!------------------------------------------------------------------------
 * \fn     ReadLnCont(FILE *Datei, as_dynstr_t *p_line)
 * \brief  read line, regarding \ continuation characters
 * \param  Datei where to read from
 * \param  pLine dest buffer
 * \return # of lines read
 * ------------------------------------------------------------------------ */

size_t ReadLnCont(FILE *Datei, as_dynstr_t *p_line)
{
  char *ptr, *pDest;
  size_t l, Count, LineCount;
  Boolean Terminated;

  /* read from input until no continuation is present */

  pDest = p_line->p_str;
  LineCount = Count = 0;
  while (1)
  {
    /* get a line from file, possibly reallocating until everything up to \n fits */

    while (1)
    {
      if (p_line->capacity - Count < 128)
        as_dynstr_realloc(p_line, p_line->capacity + 128);

      pDest = p_line->p_str + Count;
      *pDest = '\0';
      ptr = fgets(pDest, p_line->capacity - Count, Datei);
      if (!ptr)
      {
        if (ferror(Datei) != 0)
          *pDest = '\0';
        break;
      }

      /* If we have a trailing \n, we read up to end of line: */

      l = strlen(pDest);
      Terminated = ((l > 0) && (pDest[l - 1] == '\n'));

      /* srtrip possible CR preceding LF: */

      if (Terminated)
      {
        /* strip LF, and possible CR, and bail out: */

        pDest[--l] = '\0';
        if ((l > 0) && (pDest[l - 1] == '\r'))
          pDest[--l] = '\0';
      }

      Count += l;
      pDest += l;

      if (Terminated)
        break;
    }

    LineCount++;
    if ((Count > 0) && (p_line->p_str[Count - 1] == 26))
      p_line->p_str[--Count] = '\0';

    /* optional line continuation */

    if ((Count > 0) && (p_line->p_str[Count - 1] == '\\'))
      p_line->p_str[--Count] = '\0';
    else
      break;
  }

  return LineCount;
}

/*!------------------------------------------------------------------------
 * \fn     DigitVal(char ch, int Base)
 * \brief  get value of hex digit
 * \param  ch digit
 * \param  Base Number System
 * \return 0..Base-1 or -1 if no valid digit
 * ------------------------------------------------------------------------ */

int DigitVal(char ch, int Base)
{
  int Result;

  /* Ziffern 0..9 ergeben selbiges */

  if ((ch >= '0') && (ch <= '9'))
    Result = ch - '0';

  /* Grossbuchstaben fuer Hexziffern */

  else if ((ch >= 'A') && (ch <= 'Z'))
    Result = ch - 'A' + 10;

  /* Kleinbuchstaben nicht vergessen...! */

  else if ((ch >= 'a') && (ch <= 'z'))
    Result = ch - 'a' + 10;

  /* alles andere ist Schrott */

  else
    Result = -1;

  return (Result >= Base) ? -1 : Result;
}

/*--------------------------------------------------------------------*/
/* Zahlenkonstante umsetzen: $ hex, % binaer, @ oktal */
/* inp: Eingabezeichenkette */
/* erg: Zeiger auf Ergebnis-Longint */
/* liefert TRUE, falls fehlerfrei, sonst FALSE */

LargeInt ConstLongInt(const char *inp, Boolean *pErr, LongInt Base)
{
  static const char Prefixes[4] = { '$', '@', '%', '\0' }; /* die moeglichen Zahlensysteme */
  static const char Postfixes[4] = { 'H', 'O', '\0', '\0' };
  static const LongInt Bases[3] = { 16, 8, 2 };            /* die dazugehoerigen Basen */
  LargeInt erg, val;
  int z, vorz = 1;  /* Vermischtes */
  int InpLen = strlen(inp);

  /* eventuelles Vorzeichen abspalten */

  if (*inp == '-')
  {
    vorz = -1;
    inp++;
    InpLen--;
  }

  /* Sonderbehandlung 0x --> $ */

  if ((InpLen >= 2)
   && (*inp == '0')
   && (as_toupper(inp[1]) == 'X'))
  {
    inp += 2;
    InpLen -= 2;
    Base = 16;
  }

  /* Jetzt das Zahlensystem feststellen.  Vorgabe ist dezimal, was
     sich aber durch den Initialwert von Base jederzeit aendern
     laesst.  Der break-Befehl verhindert, dass mehrere Basenzeichen
     hintereinander eingegeben werden koennen */

  else if (InpLen > 0)
  {
    for (z = 0; z < 3; z++)
      if (*inp == Prefixes[z])
      {
        Base = Bases[z];
        inp++;
        InpLen--;
        break;
      }
      else if (as_toupper(inp[InpLen - 1]) == Postfixes[z])
      {
        Base = Bases[z];
        InpLen--;
        break;
      }
  }

  /* jetzt die Zahlenzeichen der Reihe nach durchverwursten */

  erg = 0;
  *pErr = False;
  for(; InpLen > 0; inp++, InpLen--)
  {
    val = DigitVal(*inp, 16);
    if (val < -0)
      break;

    /* entsprechend der Basis zulaessige Ziffer ? */

    if (val >= Base)
      break;

    /* Zahl linksschieben, zusammenfassen, naechster bitte */

    erg = erg * Base + val;
  }

  /* bis zum Ende durchgelaufen ? */

  if (!InpLen)
  {
    /* Vorzeichen beruecksichtigen */

    erg *= vorz;
    *pErr = True;
  }

  return erg;
}

/*--------------------------------------------------------------------------*/
/* fuehrende Leerzeichen loeschen */

int KillPrefBlanks(char *s)
{
  char *z = s;

  while ((*z != '\0') && as_isspace(*z))
    z++;
  if (z != s)
    strmov(s, z);
  return z - s;
}

/*--------------------------------------------------------------------------*/
/* anhaengende Leerzeichen loeschen */

int KillPostBlanks(char *s)
{
  char *z = s + strlen(s) - 1;
  int count = 0;

  while ((z >= s) && as_isspace(*z))
  {
    *(z--) = '\0';
    count++;
  }
  return count;
}

/*--------------------------------------------------------------------------*/

int strqcmp(const char *s1, const char *s2)
{
  int erg = (*s1) - (*s2);

  return (erg != 0) ? erg : strcmp(s1, s2);
}

/*--------------------------------------------------------------------------*/

/* we need a strcpy() with a defined behaviour in case of overlapping source
   and destination: */

char *strmov(char *pDest, const char *pSrc)
{
  memmove(pDest, pSrc, strlen(pSrc) + 1);
  return pDest;
}

#ifdef __GNUC__

#ifdef strcpy
# undef strcpy
#endif
char *strcpy(char *pDest, const char *pSrc)
{
  int l = strlen(pSrc) + 1;
  int Overlap = 0;

  if (pSrc < pDest)
  {
    if (pSrc + l > pDest)
      Overlap = 1;
  }
  else if (pSrc > pDest)
  {
    if (pDest + l > pSrc)
      Overlap = 1;
  }
  else if (l > 0)
  {
    Overlap = 1;
  }

  if (Overlap)
  {
    fprintf(stderr, "overlapping strcpy() called from address %p, resolve this address with addr2line and report to author\n",
            __builtin_return_address(0));
    abort();
  }

  return strmov(pDest, pSrc);
}

#endif

/*!------------------------------------------------------------------------
 * \fn     strmemcpy(char *pDest, size_t DestSize, const char *pSrc, size_t SrcLen)
 * \brief  copy string with length limitation
 * \param  pDest where to write
 * \param  DestSize destination capacity
 * \param  pSrc copy source
 * \param  SrcLen # of characters to copy at most
 * \return actual, possibly limited length
 * ------------------------------------------------------------------------ */

int strmemcpy(char *pDest, size_t DestSize, const char *pSrc, size_t SrcLen)
{
  if (DestSize < SrcLen + 1)
    SrcLen = DestSize - 1;
  memmove(pDest, pSrc, SrcLen);
  pDest[SrcLen] = '\0';
  return SrcLen;
}

/*--------------------------------------------------------------------------*/

char *ParenthPos(char *pHaystack, char Needle)
{
  char *pRun;
  int Level = 0;

  for (pRun = pHaystack; *pRun; pRun++)
  {
    switch (*pRun)
    {
      case '(':
        Level++;
        break;
      case ')':
        if (Level < 1)
          return NULL;
        Level--;
        break;
      default:
        if (*pRun == Needle && !Level)
          return pRun;
    }
  }
  return NULL;
}

/*!------------------------------------------------------------------------
 * \fn     TabCompressed(char in)
 * \brief  replace TABs with spaces for error display
 * \param  in character to compress
 * \return compressed result
 * ------------------------------------------------------------------------ */

char TabCompressed(char in)
{
  return (in == '\t') ? ' ' : (as_isprint(in) ? in : '*');
}

/*--------------------------------------------------------------------------*/

void strutil_init(void)
{
  HexStartCharacter = 'A';
}
