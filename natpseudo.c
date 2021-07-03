/* natpseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Pseudo Instructions used for National Semiconductor CPUs                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "bpemu.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "errmsg.h"

#include "natpseudo.h"

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

static void DecodeSFR(Word Code)
{
  UNUSED(Code);
  CodeEquate(SegData, 0, 0xff);
}

static void DecodeDSx(Word Shift)
{
  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;
    Boolean ValOK;
    Word Size = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &ValOK, &Flags) << Shift;

    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    else if (ValOK)
    {
      DontPrint = True;
      if (!Size) WrError(ErrNum_NullResMem);
      CodeLen = Size;
      BookKeeping();
    }
  }
}

static void DecodeFx(Word Shift)
{
  if (ChkArgCnt(2, 2))
  {
    tSymbolFlags Flags;
    Boolean ValOK;
    Word Size = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &ValOK, &Flags);

    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    else if (ValOK)
    {
      if (SetMaxCodeLen(Size << Shift)) WrError(ErrNum_CodeOverflow);
      else
      {
        Word Value = EvalStrIntExpression(&ArgStr[2], Shift ? Int16 : Int8, &ValOK);

        if (ValOK)
        {
          Word z;

          for (z = 0; z < Size; z++)
          {
            BAsmCode[CodeLen++] = Lo(Value);
            if (Shift)
              BAsmCode[CodeLen++] = Hi(Value);
          }
        }
      }
    }
  }
}

Boolean DecodeNatPseudo(void)
{
  static PInstTable InstTable = NULL;

  if (!InstTable)
  {
    InstTable = CreateInstTable(31);

    AddInstTable(InstTable, "SFR"  , 0     , DecodeSFR);
    AddInstTable(InstTable, "ADDR" , eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt , DecodeIntelDB);
    AddInstTable(InstTable, "ADDRW", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt , DecodeIntelDW);
    AddInstTable(InstTable, "BYTE" , eIntPseudoFlag_AllowInt , DecodeIntelDB);
    AddInstTable(InstTable, "WORD" , eIntPseudoFlag_AllowInt , DecodeIntelDW);
    AddInstTable(InstTable, "DSB"  , 0     , DecodeDSx);
    AddInstTable(InstTable, "DSW"  , 1     , DecodeDSx);
    AddInstTable(InstTable, "FB"   , 0     , DecodeFx);
    AddInstTable(InstTable, "FW"   , 1     , DecodeFx);
  }

  return LookupInstTable(InstTable, OpPart.str.p_str);
}
