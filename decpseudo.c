/* decpseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Commonly Used VAX/PDP-11 Pseudo Instructions                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "strutil.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmerr.h"
#include "errmsg.h"

#include "decpseudo.h"

/*****************************************************************************
 * Global Types
 *****************************************************************************/

/*!------------------------------------------------------------------------
 * \fn     decode_dec_packed(Word index)
 * \brief  encode number or decimal string in packed decimal format
 * ------------------------------------------------------------------------ */

static void append_digit(Byte digit, Boolean *p_half)
{
  if (*p_half)
  {
    BAsmCode[CodeLen] |= (digit & 15);
    CodeLen++;
    switch (ListGrans[ActPC])
    {
      case 1:
        break;
      case 2:
        if (!(CodeLen & 1))
          WAsmCode[(CodeLen / 2) - 1] = ((Word)BAsmCode[CodeLen - 1]) << 8 | BAsmCode[CodeLen - 2];
        break;
    }
  }
  else
  {
    SetMaxCodeLen(CodeLen + 1);
    BAsmCode[CodeLen] = (digit & 15) << 4;
  }
  *p_half = !*p_half;
}

void decode_dec_packed(Word index)
{
  TempResult value;
  const char *p_str = NULL;
  char tmp_dec_str[33];

  UNUSED(index);

  if (!ChkArgCnt(1, 2))
    return;

  as_tempres_ini(&value);
  EvalStrExpression(&ArgStr[1], &value);
  switch(value.Typ)
  {
    case TempNone:
      break;
    case TempInt:
      as_snprintf(tmp_dec_str, sizeof(tmp_dec_str), "%lld", value.Contents.Int);
      p_str = tmp_dec_str;
      goto writeout;
    case TempString:
      p_str = value.Contents.str.p_str;
      goto writeout;
    case TempFloat:
      WrStrErrorPos(ErrNum_StringOrIntButFloat, &ArgStr[1]);
      break;
    default:
      WrStrErrorPos(ErrNum_ExpectIntOrString, &ArgStr[1]);
      break;
    writeout:
    {
      size_t num_digits = 0, l;
      Boolean half, negative;

      switch (*p_str)
      {
        case '-':
          negative = True;
          p_str++;
          break;
        case '+':
          negative = False;
          p_str++;
          break;
        default:
          negative = False;
      }

      l = strlen(p_str);
      half = False;
      /* leading zero padding digit is not counted into num_digits: */
      if (!(l & 1))
        append_digit(0, &half);
      for (; *p_str; p_str++)
      {
        if (!isdigit(*p_str))
        {
          WrStrErrorPos(ErrNum_InvalidDecDigit, &ArgStr[1]);
          CodeLen = 0; half = False;
          break;
        }
        append_digit(*p_str - '0', &half);
        if (++num_digits > 31)
        {
          WrStrErrorPos(ErrNum_DecStringTooLong, &ArgStr[1]);
          CodeLen = 0; half = False;
          break;
        }
      }
      if (CodeLen || half)
      {
        append_digit(negative ? 13 : 12, &half);
        if (2 == ArgCnt)
          EnterIntSymbol(&ArgStr[2], num_digits, SegNone, False);
      }
    }
  }

  as_tempres_free(&value);
}
