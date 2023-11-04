/* intpseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Pseudo Instructions used by some 4 bit devices                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "bpemu.h"
#include "be_le.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "errmsg.h"
#include "chartrans.h"

#include "fourpseudo.h"

void DecodeRES(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean ValOK;
    tSymbolFlags Flags;
    Word Size;

    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &ValOK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (ValOK && !mFirstPassUnknown(Flags))
    {
      DontPrint = True;
      if (!Size) WrError(ErrNum_NullResMem);
      CodeLen = Size;
      BookKeeping();
    }
  }
}

void DecodeDATA(IntType CodeIntType, IntType DataIntType)
{
  TempResult t;
  IntType ValIntType = (ActPC == SegData) ? DataIntType : CodeIntType;
  LargeWord ValMask = IntTypeDefs[ValIntType].Mask;
  Word MaxMultCharLen = (Lo(IntTypeDefs[ValIntType].SignAndWidth) + 7) / 8;
  LargeWord UnknownMask = ValMask / 2;

  as_tempres_ini(&t);
  if (ChkArgCnt(1, ArgCntMax))
  {
    Boolean ValOK = True;
    tStrComp *pArg;

    forallargs(pArg, ValOK)
    {
      EvalStrExpression(pArg, &t);
      if ((t.Typ == TempInt) && mFirstPassUnknown(t.Flags))
        t.Contents.Int &= UnknownMask;

      switch (t.Typ)
      {
        case TempFloat:
          WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
          ValOK = False;
          break;
        case TempString:
        {
          unsigned char *cp;
          unsigned z2;
          int bpos;
          LongWord TransCh;

          if (MultiCharToInt(&t, MaxMultCharLen))
            goto ToInt;

          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            ValOK = False;
          else
            for (z2 = 0, cp = (unsigned char *)t.Contents.str.p_str, bpos = 0;
                 z2 < t.Contents.str.len;
                 z2++, cp++)
            {
              TransCh = *cp & 0xff;

              /* word width 24..31 bits: pack three characters into one dword */

              if (ValMask >= 0xfffffful)
              {
                if (!bpos)
                  DAsmCode[CodeLen++] = TransCh;
                else if (1 == bpos)
                  DAsmCode[CodeLen - 1] |= TransCh << 8;
                else
                  DAsmCode[CodeLen - 1] |= TransCh << 16;
                if (++bpos >= 3)
                  bpos = 0;
              }

              /* word width 17..23 bits: pack two characters into one dword */

              else if (ValMask > 0xfffful)
              {
                if (!bpos)
                  DAsmCode[CodeLen++] = TransCh;
                else
                  DAsmCode[CodeLen - 1] |= TransCh << 8;
                if (++bpos >= 2)
                  bpos = 0;
              }

              /* word width 16 bits: pack two characters into one word */

              else if (ValMask == 0xfffful)
              {
                if (!bpos)
                  WAsmCode[CodeLen++] = TransCh;
                else
                  WAsmCode[CodeLen - 1] |= TransCh << 8;
                if (++bpos >= 2)
                  bpos = 0;
              }

              /* word width 9..15 bits: pack one character into one word */

              else if (ValMask > 0xff)
                WAsmCode[CodeLen++] = TransCh;

              /* word width 8 bits: pack one character into one byte */

              else if (ValMask == 0xff)
                BAsmCode[CodeLen++] = TransCh;

              /* word width 4...7 bits: pack one character into two nibbles */

              else
              {
                BAsmCode[CodeLen++] = TransCh >> 4;
                BAsmCode[CodeLen++] = TransCh & 15;
              }
            }
          break;
        }
        case TempInt:
        ToInt:
          if (!mSymbolQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, ValIntType))
          {
            WrStrErrorPos(ErrNum_OverRange, pArg);
            ValOK = False;
          }
          else if (ValMask <= 0xff)
            BAsmCode[CodeLen++] = t.Contents.Int & ValMask;
          else if (ValMask <= 0xfffful)
            WAsmCode[CodeLen++] = t.Contents.Int & ValMask;
          else
            DAsmCode[CodeLen++] = t.Contents.Int & ValMask;
          break;
        default:
          ValOK = False;
      }
    }
    if (!ValOK)
      CodeLen = 0;
  }
  as_tempres_free(&t);
}
