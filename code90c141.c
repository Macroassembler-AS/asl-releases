/* code90c141.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Toshiba TLCS-90                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code90c141.h"

typedef struct
{
  const char *Name;
  Byte Code;
} Condition;

#define AccReg 6
#define HLReg 2

#define COND_CODE_TRUE 8

enum
{
  ModNone = -1,
  ModReg8 = 0,
  ModReg16 = 1,
  ModIReg16 = 2,
  ModIndReg = 3,
  ModIdxReg = 4,
  ModDir = 5,
  ModMem = 6,
  ModImm = 7
};

#define MModReg8   (1 << ModReg8)
#define MModReg16  (1 << ModReg16)
#define MModIReg16 (1 << ModIReg16)
#define MModIndReg (1 << ModIndReg)
#define MModIdxReg (1 << ModIdxReg)
#define MModDir    (1 << ModDir)
#define MModMem    (1 << ModMem)
#define MModImm    (1 << ModImm)

static ShortInt AdrType;
static Byte AdrMode;
static ShortInt OpSize;
static Byte AdrVals[10];
static Boolean MinOneIs0;

static Condition *Conditions;

static CPUVar CPU90C141;

/*---------------------------------------------------------------------------*/

static void SetOpSize(ShortInt New)
{
  if (OpSize == -1)
    OpSize = New;
  else if (OpSize != New)
  {
    WrError(ErrNum_ConfOpSizes);
    AdrType = ModNone;
    AdrCnt = 0;
  }
}

static void DecodeAdr(const tStrComp *pArg, Byte Erl)
{
  static const char Reg8Names[][2] = { "B", "C", "D", "E", "H", "L", "A" };
  static const int Reg8Cnt = sizeof(Reg8Names) / sizeof(*Reg8Names);
  static const char Reg16Names[][3] = { "BC", "DE", "HL", "\0", "IX", "IY", "SP" };
  static const int Reg16Cnt = sizeof(Reg16Names) / sizeof(*Reg16Names);
  static const char IReg16Names[][3] =  {"IX", "IY", "SP" };
  static const int IReg16Cnt = sizeof(IReg16Names) / sizeof(*IReg16Names);

  int z;
  char *p;
  LongInt DispAcc, DispVal;
  Byte OccFlag, BaseReg;
  Boolean ok, fnd, NegFlag, NNegFlag, Unknown;

  AdrType = ModNone; AdrCnt = 0;

  /* 1. 8-Bit-Register */

  for (z = 0; z < Reg8Cnt; z++)
    if (!as_strcasecmp(pArg->str.p_str, Reg8Names[z]))
    {
      AdrType = ModReg8;
      AdrMode = z;
      SetOpSize(0);
      goto chk;
    }

  /* 2. 16-Bit-Register, indiziert */

  if (Erl & MModIReg16)
  {
    for (z = 0; z < IReg16Cnt; z++)
      if (!as_strcasecmp(pArg->str.p_str, IReg16Names[z]))
      {
        AdrType = ModIReg16;
        AdrMode = z;
        SetOpSize(1);
        goto chk;
      }
  }

  /* 3. 16-Bit-Register, normal */

  for (z = 0; z < Reg16Cnt; z++)
   if (!as_strcasecmp(pArg->str.p_str, Reg16Names[z]))
   {
     AdrType = ModReg16;
     AdrMode = z;
     SetOpSize(1);
     goto chk;
   }

  /* Speicheradresse */

  if (IsIndirect(pArg->str.p_str))
  {
    tStrComp Arg, Remainder;

    OccFlag = 0;
    BaseReg = 0;
    DispAcc = 0;
    ok = True;
    NegFlag = False;
    Unknown = False;
    StrCompRefRight(&Arg, pArg, 1);
    StrCompShorten(&Arg, 1);

    do
    {
      p = QuotMultPos(Arg.str.p_str, "+-");
      NNegFlag = p && (*p == '-');
      if (p)
        StrCompSplitRef(&Arg, &Remainder, &Arg, p);

      KillPrefBlanksStrComp(&Arg);
      KillPostBlanksStrComp(&Arg);
      fnd = False;

      if (!as_strcasecmp(Arg.str.p_str, "A"))
      {
        fnd = True;
        ok = ((!NegFlag) && (!(OccFlag & 1)));
        if (ok)
          OccFlag += 1;
        else
          WrError(ErrNum_InvAddrMode);
      }

      if (!fnd)
      {
        for (z = 0; z < Reg16Cnt; z++)
        {
          if (!as_strcasecmp(Arg.str.p_str, Reg16Names[z]))
          {
            fnd = True;
            BaseReg = z;
            ok = ((!NegFlag) && (!(OccFlag & 2)));
            if (ok)
              OccFlag += 2;
            else
              WrError(ErrNum_InvAddrMode);
          }
        }
      }

      if (!fnd)
      {
        tSymbolFlags Flags;

        DispVal = EvalStrIntExpressionWithFlags(&Arg, Int32, &ok, &Flags);
        if (ok)
        {
          DispAcc = NegFlag ? DispAcc - DispVal : DispAcc + DispVal;
          if (mFirstPassUnknown(Flags))
            Unknown = True;
        }
      }

      NegFlag = NNegFlag;
      if (p)
        Arg = Remainder;
    }
    while (p && ok);

    if (!ok)
      return;
    if (Unknown)
      DispAcc &= 0x7f;

    switch (OccFlag)
    {
      case 1:
        WrError(ErrNum_InvAddrMode);
        break;
      case 3:
        if ((BaseReg != 2) || (DispAcc!=0)) WrError(ErrNum_InvAddrMode);
        else
        {
          AdrType = ModIdxReg;
          AdrMode = 3;
        }
        break;
      case 2:
        if ((DispAcc > 127) || (DispAcc < -128)) WrError(ErrNum_OverRange);
        else if (DispAcc == 0)
        {
          AdrType = ModIndReg;
          AdrMode = BaseReg;
        }
        else if (BaseReg < 4) WrError(ErrNum_InvAddrMode);
        else
        {
          AdrType = ModIdxReg;
          AdrMode = BaseReg - 4;
          AdrCnt = 1;
          AdrVals[0] = DispAcc & 0xff;
        }
        break;
      case 0:
        if (DispAcc > 0xffff) WrError(ErrNum_AdrOverflow);
        else if ((Hi(DispAcc) == 0xff) && (Erl & MModDir))
        {
          AdrType = ModDir;
          AdrCnt = 1;
          AdrVals[0] = Lo(DispAcc);
        }
        else
        {
          AdrType = ModMem;
          AdrCnt = 2;
          AdrVals[0] = Lo(DispAcc);
          AdrVals[1] = Hi(DispAcc);
        }
        break;
    }
  }

  /* immediate */

  else
  {
    if ((OpSize == -1) && (MinOneIs0))
      OpSize = 0;
    switch (OpSize)
    {
      case -1:
        WrError(ErrNum_InvOpSize);
        break;
      case 0:
        AdrVals[0] = EvalStrIntExpression(pArg, Int8, &ok);
        if (ok)
        {
          AdrType = ModImm;
          AdrCnt = 1;
        }
        break;
      case 1:
        DispVal = EvalStrIntExpression(pArg, Int16, &ok);
        if (ok)
        {
          AdrType = ModImm;
          AdrCnt = 2;
          AdrVals[0] = Lo(DispVal);
          AdrVals[1] = Hi(DispVal);
        }
        break;
    }
  }

  /* gefunden */

chk:
  if ((AdrType != ModNone) && (!((1 << AdrType) & Erl)))
  {
    WrError(ErrNum_InvAddrMode);
    AdrType = ModNone;
    AdrCnt = 0;
  }
}

static Boolean ArgPair(const char *Arg1, const char *Arg2)
{
  return  (((!as_strcasecmp(ArgStr[1].str.p_str, Arg1)) && (!as_strcasecmp(ArgStr[2].str.p_str, Arg2)))
        || ((!as_strcasecmp(ArgStr[1].str.p_str, Arg2)) && (!as_strcasecmp(ArgStr[2].str.p_str, Arg1))));
}

/*!------------------------------------------------------------------------
 * \fn     decode_condition(const char *p_cond_str, Byte *p_cond_code)
 * \brief  parse condition code
 * \param  p_cond_str source argument
 * \param  p_cond_code returns machine code of condition if found
 * \return True if found
 * ------------------------------------------------------------------------ */

static Boolean decode_condition(const char *p_cond_str, Byte *p_cond_code)
{
  int z;

  for (z = 0; Conditions[z].Name; z++)
    if (!as_strcasecmp(p_cond_str, Conditions[z].Name))
    {
      *p_cond_code = Conditions[z].Code;
      return True;
    }
  return False;
}

/*-------------------------------------------------------------------------*/

/* ohne Argument */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
}

static void DecodeMove(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 2;
    BAsmCode[0] = 0xfe;
    BAsmCode[1] = Code;
  }
}

static void DecodeShift(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], (Hi(Code) ? MModReg8 : 0) | MModIndReg | MModIdxReg | MModMem | MModDir);
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 2;
        BAsmCode[0] = 0xf8 + AdrMode;
        BAsmCode[1] = Lo(Code);
        if (AdrMode == AccReg)
          WrError(ErrNum_ShortAddrPossible);
        break;
      case ModIndReg:
        CodeLen = 2;
        BAsmCode[0] = 0xe0 + AdrMode;
        BAsmCode[1] = Lo(Code);
        break;
      case ModIdxReg:
        CodeLen = 2 + AdrCnt;
        BAsmCode[0] = 0xf0 + AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[1 + AdrCnt] = Lo(Code);
        break;
      case ModDir:
        CodeLen = 3;
        BAsmCode[0] = 0xe7;
        BAsmCode[1] = AdrVals[0];
        BAsmCode[2] = Lo(Code);
        break;
      case ModMem:
        CodeLen = 4;
        BAsmCode[0] = 0xe3;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[3] = Lo(Code);
        break;
    }
  }
}

/* Logik */

static void DecodeBit(Word Code)
{
  Byte BitPos;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    BitPos = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
    if (OK)
    {
      DecodeAdr(&ArgStr[2], MModReg8 | MModIndReg | MModIdxReg | MModMem | MModDir);
      switch (AdrType)
      {
        case ModReg8:
          CodeLen = 2;
          BAsmCode[0] = 0xf8 + AdrMode;
          BAsmCode[1] = Code + BitPos;
          break;
        case ModIndReg:
          CodeLen = 2;
          BAsmCode[0] = 0xe0 + AdrMode;
          BAsmCode[1] = Code + BitPos;
          break;
        case ModIdxReg:
          CodeLen = 2 + AdrCnt;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[0] = 0xf0 + AdrMode;
          BAsmCode[1 + AdrCnt] = Code + BitPos;
          break;
        case ModMem:
          CodeLen = 4;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[0] = 0xe3;
          BAsmCode[1 + AdrCnt] = Code + BitPos;
          break;
        case ModDir:
          BAsmCode[1] = AdrVals[0];
          if (Code == 0x18)
          {
            BAsmCode[0] = 0xe7;
            BAsmCode[2] = Code + BitPos;
            CodeLen = 3;
          }
          else
          {
            BAsmCode[0] = Code + BitPos;
            CodeLen = 2;
          }
          break;
      }
    }
  }
}

static void DecodeAcc(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
}

static void DecodeALU2(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModIdxReg | MModIndReg | MModDir | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        DecodeAdr(&ArgStr[2], MModImm | (((HReg = AdrMode) == AccReg) ? MModReg8 | MModIndReg | MModIdxReg | MModDir | MModMem:0));
        switch(AdrType)
        {
          case ModReg8:
            CodeLen = 2;
            BAsmCode[0] = 0xf8 | AdrMode;
            BAsmCode[1] = 0x60 | Code;
            break;
          case ModIndReg:
            CodeLen = 2;
            BAsmCode[0] = 0xe0 | AdrMode; BAsmCode[1] = 0x60 | Code;
            break;
          case ModIdxReg:
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0xf0 | AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x60 | Code;
            break;
          case ModDir:
            CodeLen = 2;
            BAsmCode[0] = 0x60 | Code;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModMem:
            CodeLen = 4;
            BAsmCode[0] = 0xe3;
            BAsmCode[3] = 0x60 | Code;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            break;
          case ModImm:
            if (HReg == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x68 | Code;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xf8 | HReg;
              BAsmCode[1] = 0x68 | Code;
              BAsmCode[2] = AdrVals[0];
            }
            break;
        }
        break;
      case ModReg16:
        if ((AdrMode == 2) || ((Code == 0) && (AdrMode >= 4)))
        {
          HReg = AdrMode;
          DecodeAdr(&ArgStr[2], MModReg16 | MModIndReg | MModIdxReg | MModDir | MModMem | MModImm);
          switch (AdrType)
          {
            case ModReg16:
              CodeLen = 2;
              BAsmCode[0] = 0xf8 | AdrMode;
              BAsmCode[1] = (HReg >= 4) ? 0x14 + HReg - 4 : 0x70 + Code;
              break;
            case ModIndReg:
              CodeLen = 2;
              BAsmCode[0] = 0xe0 | AdrMode;
              BAsmCode[1] = (HReg >= 4) ? 0x14 + HReg - 4 : 0x70 + Code;
              break;
            case ModIdxReg:
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0xf0 | AdrMode;
              memcpy(BAsmCode + 1,AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = (HReg >= 4) ? 0x14 + HReg - 4 : 0x70 + Code;
              break;
            case ModDir:
              if (HReg >= 4)
              {
                CodeLen = 3;
                BAsmCode[0] = 0xe7;
                BAsmCode[1] = AdrVals[0];
                BAsmCode[2] = 0x10 | HReg;
              }
              else
              {
                CodeLen = 2;
                BAsmCode[0] = 0x70 | Code;
                BAsmCode[1] = AdrVals[0];
              }
              break;
            case ModMem:
              CodeLen = 4;
              BAsmCode[0] = 0xe3;
              memcpy(BAsmCode + 1, AdrVals, 2);
              BAsmCode[3] = (HReg >= 4) ? 0x14 + HReg - 4 : 0x70 + Code;
              break;
            case ModImm:
              CodeLen = 3;
              BAsmCode[0] = (HReg >= 4) ? 0x14 + HReg - 4 : 0x78 + Code;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              break;
          }
        }
        else WrError(ErrNum_InvAddrMode);
        break;
      case ModIndReg:
      case ModIdxReg:
      case ModDir:
      case ModMem:
        OpSize = 0;
        switch (AdrType)
        {
          case ModIndReg:
            HReg = 3;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0x68 | Code;
            break;
          case ModIdxReg:
            HReg = 3 + AdrCnt;
            BAsmCode[0] = 0xf4 | AdrMode;
            BAsmCode[1 + AdrCnt] = 0x68 | Code;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            break;
          case ModDir:
            HReg = 4;
            BAsmCode[0] = 0xef;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = 0x68 | Code;
            break;
          case ModMem:
            HReg = 5;
            BAsmCode[0] = 0xeb;
            memcpy(BAsmCode + 1, AdrVals, 2);
            BAsmCode[3] = 0x68 | Code;
            break;
          default:
            HReg = 0;
        }
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType == ModImm)
        {
          BAsmCode[HReg-1] = AdrVals[0];
          CodeLen = HReg;
        }
        break;
    }
  }
}

static void DecodeLD(Word Code)
{
  Byte HReg;

  if (Hi(Code))
    SetOpSize(1);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModIndReg | MModIdxReg | MModDir | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModIndReg | MModIdxReg | MModDir | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg8:
            if (HReg == AccReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x20 | AdrMode;
            }
            else if (AdrMode == AccReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x28 | HReg;
            }
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0xf8 | AdrMode;
              BAsmCode[1] = 0x30 | HReg;
            }
            break;
          case ModIndReg:
            CodeLen = 2;
            BAsmCode[0] = 0xe0 | AdrMode;
            BAsmCode[1] = 0x28 | HReg;
            break;
          case ModIdxReg:
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0xf0 | AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x28 | HReg;
            break;
          case ModDir:
            if (HReg == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x27;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xe7;
              BAsmCode[1] = AdrVals[0];
              BAsmCode[2] = 0x28 | HReg;
            }
            break;
          case ModMem:
            CodeLen = 4;
            BAsmCode[0] = 0xe3;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[3] = 0x28 | HReg;
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0x30 | HReg;
            BAsmCode[1] = AdrVals[0];
            break;
        }
        break;
      case ModReg16:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg16 | MModIndReg | MModIdxReg | MModDir | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg16:
            if (HReg == HLReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x40 | AdrMode;
            }
            else if (AdrMode == HLReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x48 | HReg;
            }
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0xf8 | AdrMode;
              BAsmCode[1] = 0x38 | HReg;
            }
            break;
          case ModIndReg:
            CodeLen = 2;
            BAsmCode[0] = 0xe0 | AdrMode;
            BAsmCode[1] = 0x48 | HReg;
            break;
          case ModIdxReg:
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0xf0 | AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x48 | HReg;
            break;
          case ModDir:
            if (HReg == HLReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x47;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xe7;
              BAsmCode[1] = AdrVals[0];
              BAsmCode[2] = 0x48 | HReg;
            }
            break;
          case ModMem:
            CodeLen = 4;
            BAsmCode[0] = 0xe3;
            BAsmCode[3] = 0x48 | HReg;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            break;
          case ModImm:
            CodeLen = 3;
            BAsmCode[0] = 0x38 | HReg;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            break;
        }
        break;
      case ModIndReg:
      case ModIdxReg:
      case ModDir:
      case ModMem:
        MinOneIs0 = True;
        HReg = AdrCnt;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        switch (AdrType)
        {
          case ModIndReg:
            BAsmCode[0] = 0xe8 | AdrMode;
            break;
          case ModIdxReg:
            BAsmCode[0] = 0xf4 | AdrMode;
            break;
          case ModMem:
            BAsmCode[0] = 0xeb;
            break;
          case ModDir:
            BAsmCode[0] = 0x0f;
            break;
        }
        DecodeAdr(&ArgStr[2], MModReg16 | MModReg8 | MModImm);
        if (BAsmCode[0] == 0x0f)
         switch (AdrType)
         {
           case ModReg8:
             if (AdrMode == AccReg)
             {
               CodeLen = 2;
               BAsmCode[0] = 0x2f;
             }
             else
             {
               CodeLen = 3;
               BAsmCode[0] = 0xef;
               BAsmCode[2] = 0x20 | AdrMode;
             }
             break;
           case ModReg16:
             if (AdrMode == HLReg)
             {
               CodeLen = 2;
               BAsmCode[0] = 0x4f;
             }
             else
             {
               CodeLen = 3;
               BAsmCode[0] = 0xef;
               BAsmCode[2] = 0x40 | AdrMode;
             }
             break;
           case ModImm:
             CodeLen = 3 + OpSize;
             BAsmCode[0] = 0x37 | (OpSize << 3);
             memcpy(BAsmCode + 2, AdrVals, AdrCnt);
             break;
         }
         else
         {
           switch (AdrType)
           {
             case ModReg8:
               BAsmCode[1 + HReg] = 0x20 | AdrMode;
               break;
             case ModReg16:
               BAsmCode[1 + HReg] = 0x40 | AdrMode;
               break;
             case ModImm:
               BAsmCode[1 + HReg] = 0x37 | (OpSize << 3);
               break;
           }
           memcpy(BAsmCode + 2 + HReg, AdrVals, AdrCnt);
           CodeLen = 1 + HReg + 1 + AdrCnt;
         }
        break;
    }
  }
}

static void DecodePUSH_POP(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "AF"))
    {
      CodeLen = 1;
      BAsmCode[0] = 0x56 | Code;
    }
    else
    {
      DecodeAdr(&ArgStr[1], MModReg16);
      if (AdrType == ModReg16)
      {
        if (AdrMode == 6) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 1;
          BAsmCode[0] = 0x50 | Code | AdrMode;
        }
      }
    }
  }
}

static void DecodeLDA(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrType == ModReg16)
    {
      HReg = 0x38 + AdrMode;
      DecodeAdr(&ArgStr[2], MModIndReg | MModIdxReg);
      switch (AdrType)
      {
        case ModIndReg:
          if (AdrMode < 4) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 3;
            BAsmCode[0] = 0xf0 | AdrMode;
            BAsmCode[1] = 0;
            BAsmCode[2] = HReg;
          }
          break;
        case ModIdxReg:
          CodeLen = 2 + AdrCnt;
          BAsmCode[0] = 0xf4 + AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = HReg;
          break;
      }
    }
  }
}

static void DecodeLDAR(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "HL")) WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;
    Integer AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK) - (EProgCounter() + 2);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x17;
      BAsmCode[1] = Lo(AdrInt);
      BAsmCode[2] = Hi(AdrInt);
    }
  }
}

static void DecodeEX(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  /* work around the parser problem related to the ' character */

  if (!as_strncasecmp(ArgStr[2].str.p_str, "AF\'", 3))
    ArgStr[2].str.p_str[3] = '\0';

  if (!ChkArgCnt(2, 2));
  else if (ArgPair("DE", "HL"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x08;
  }
  else if ((ArgPair("AF", "AF\'")) || (ArgPair("AF", "AF`")))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x09;
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg16 | MModIndReg | MModIdxReg | MModMem | MModDir);
    switch (AdrType)
    {
      case ModReg16:
        HReg = 0x50 | AdrMode;
        DecodeAdr(&ArgStr[2], MModIndReg | MModIdxReg | MModMem | MModDir);
        switch (AdrType)
        {
          case ModIndReg:
            CodeLen = 2;
            BAsmCode[0] = 0xe0 | AdrMode;
            BAsmCode[1] = HReg;
            break;
          case ModIdxReg:
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0xf0 | AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = HReg;
            break;
          case ModDir:
            CodeLen = 3;
            BAsmCode[0] = 0xe7;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = HReg;
            break;
          case ModMem:
            CodeLen = 4;
            BAsmCode[0] = 0xe3;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[3] = HReg;
            break;
        }
        break;
      case ModIndReg:
      case ModIdxReg:
      case ModDir:
      case ModMem:
        switch (AdrType)
        {
          case ModIndReg:
            BAsmCode[0] = 0xe0 | AdrMode;
            break;
          case ModIdxReg:
            BAsmCode[0] = 0xf0 | AdrMode;
            break;
          case ModDir:
            BAsmCode[0] = 0xe7;
            break;
          case ModMem:
            BAsmCode[0] = 0xe3;
            break;
        }
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        HReg = 2 + AdrCnt;
        DecodeAdr(&ArgStr[2], MModReg16);
        if (AdrType == ModReg16)
        {
          BAsmCode[HReg - 1] = 0x50 | AdrMode;
          CodeLen = HReg;
        }
        break;
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  if (Hi(Code))
    SetOpSize(1);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModIndReg | MModIdxReg | MModDir | MModMem);
    if (OpSize==-1)
      OpSize = 0;
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 1;
        BAsmCode[0] = 0x80 | Lo(Code) | AdrMode;
        break;
      case ModReg16:
        CodeLen = 1;
        BAsmCode[0] = 0x90 | Lo(Code) | AdrMode;
        break;
      case ModIndReg:
        CodeLen = 2;
        BAsmCode[0] = 0xe0 | AdrMode;
        BAsmCode[1] = 0x87 | (OpSize << 4) | Lo(Code);
        break;
      case ModIdxReg:
        CodeLen = 2 + AdrCnt;
        BAsmCode[0] = 0xf0 | AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[1 + AdrCnt] = 0x87 | (OpSize << 4) | Lo(Code);
        break;
      case ModDir:
        CodeLen = 2;
        BAsmCode[0] = 0x87 | (OpSize << 4) | Lo(Code);
        BAsmCode[1] = AdrVals[0];
        break;
      case ModMem:
        CodeLen = 4;
        BAsmCode[0] = 0xe3;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[3] = 0x87 | (OpSize << 4) | Lo(Code);
        BAsmCode[1] = AdrVals[0];
        break;
    }
  }
}

static void DecodeINCX_DECX(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModDir);
    if (AdrType == ModDir)
    {
      CodeLen = 2;
      BAsmCode[0] = Code;
      BAsmCode[1] = AdrVals[0];
    }
  }
}

static void DecodeMUL_DIV(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "HL")) WrError(ErrNum_InvAddrMode);
  else
  {
    OpSize = 0;
    DecodeAdr(&ArgStr[2], MModReg8 | MModIndReg | MModIdxReg | MModDir | MModMem | MModImm);
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 2;
        BAsmCode[0] = 0xf8 + AdrMode;
        BAsmCode[1] = Code;
        break;
      case ModIndReg:
        CodeLen = 2;
        BAsmCode[0] = 0xe0 + AdrMode;
        BAsmCode[1] = Code;
        break;
      case ModIdxReg:
        CodeLen = 2 + AdrCnt;
        BAsmCode[0] = 0xf0 + AdrMode;
        BAsmCode[1 + AdrCnt] = Code;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        break;
      case ModDir:
        CodeLen = 3;
        BAsmCode[0] = 0xe7;
        BAsmCode[1] = AdrVals[0];
        BAsmCode[2] = Code;
        break;
      case ModMem:
        CodeLen = 4;
        BAsmCode[0] = 0xe3;
        BAsmCode[3] = Code;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        break;
      case ModImm:
        CodeLen = 2;
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrVals[0];
        break;
    }
  }
}

static void DecodeJR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    Boolean OK;
    tSymbolFlags Flags;
    Integer AdrInt;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int16, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0xc0 | cond_code;
        BAsmCode[1] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeCALL_JP(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    OpSize = 1;
    DecodeAdr(&ArgStr[ArgCnt], MModIndReg | MModIdxReg | MModMem |MModImm);
    if (AdrType == ModImm)
      AdrType = ModMem;
    switch (AdrType)
    {
      case ModIndReg:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = 0xc0 | (Code << 4) | cond_code;
        break;
      case ModIdxReg:
        CodeLen = 2 + AdrCnt;
        BAsmCode[0] = 0xf4 | AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[1 + AdrCnt] = 0xc0 | (Code << 4) | cond_code;
        break;
      case ModMem:
        if (cond_code == COND_CODE_TRUE)
        {
          CodeLen = 3;
          BAsmCode[0] = 0x1a + (Code << 1);
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        }
        else
        {
          CodeLen = 4;
          BAsmCode[0] = 0xeb;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[3] = 0xc0 | (Code << 4) | cond_code;
        }
        break;
    }
  }
}

static void DecodeRET(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, 1))
  {
    Byte cond_code;

    if (ArgCnt == 0)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    if (cond_code == COND_CODE_TRUE)
    {
      CodeLen = 1;
      BAsmCode[0] = 0x1e;
    }
    else
    {
      CodeLen = 2;
      BAsmCode[0] = 0xfe;
      BAsmCode[1] = 0xd0 | cond_code;
    }
  }
}

static void DecodeDJNZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    if (ArgCnt == 1)
    {
      AdrType = ModReg8;
      AdrMode = 0;
      OpSize = 0;
    }
    else
      DecodeAdr(&ArgStr[1], MModReg8 | MModReg16);
    if (AdrType != ModNone)
    {
      if (AdrMode != 0) WrError(ErrNum_InvAddrMode);
      else
      {
        Boolean OK;
        tSymbolFlags Flags;
        Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int16, &OK, &Flags) - (EProgCounter() + 2);

        if (OK)
        {
          if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0x18 | OpSize;
            BAsmCode[1] = AdrInt & 0xff;
          }
        }
      }
    }
  }
}

static void DecodeJRL_CALR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Integer AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK) - (EProgCounter() + 2);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = Code;
      if ((Code == 0x1b)
       && (AdrInt >= -128)
       && (AdrInt <= 127))
        WrError(ErrNum_ShortJumpPossible);
      BAsmCode[1] = Lo(AdrInt);
      BAsmCode[2] = Hi(AdrInt);
    }
  }
}

/*-------------------------------------------------------------------------*/

static void AddW(const char *Name, Word Code, InstProc Proc)
{
  char Str[20];

  AddInstTable(InstTable, Name, Code, Proc);
  as_snprintf(Str, sizeof(Str), "%sW", Name);
  AddInstTable(InstTable, Str, Code | 0x100, Proc);
}

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddMove(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeMove);
}

static void AddShift(const char *NName, Word NCode, Boolean NMay)
{
  AddInstTable(InstTable, NName, NCode | (NMay ? 0x100 : 0), DecodeShift);
}

static void AddBit(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void AddAcc(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAcc);
}

static void AddCondition(const char *NName, Byte NCode)
{
  order_array_rsv_end(Conditions, Condition);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(207);
  SetDynamicInstTable(InstTable);
  AddW("LD", 0, DecodeLD);
  AddInstTable(InstTable, "PUSH", 0, DecodePUSH_POP);
  AddInstTable(InstTable, "POP" , 8, DecodePUSH_POP);
  AddInstTable(InstTable, "LDA", 0, DecodeLDA);
  AddInstTable(InstTable, "LDAR", 0, DecodeLDAR);
  AddInstTable(InstTable, "EX", 0, DecodeEX);
  AddW("INC", 0, DecodeINC_DEC);
  AddW("DEC", 8, DecodeINC_DEC);
  AddInstTable(InstTable, "INCX", 0x07, DecodeINCX_DECX);
  AddInstTable(InstTable, "DECX", 0x0f, DecodeINCX_DECX);
  AddInstTable(InstTable, "MUL", 0x12, DecodeMUL_DIV);
  AddInstTable(InstTable, "DIV", 0x13, DecodeMUL_DIV);
  AddInstTable(InstTable, "JR", 0, DecodeJR);
  AddInstTable(InstTable, "CALL", 1, DecodeCALL_JP);
  AddInstTable(InstTable, "JP", 0, DecodeCALL_JP);
  AddInstTable(InstTable, "RET", 0, DecodeRET);
  AddInstTable(InstTable, "DJNZ", 0, DecodeDJNZ);
  AddInstTable(InstTable, "JRL", 0x1b, DecodeJRL_CALR);
  AddInstTable(InstTable, "CALR", 0x1d, DecodeJRL_CALR);

  AddFixed("EXX" , 0x0a); AddFixed("CCF" , 0x0e);
  AddFixed("SCF" , 0x0d); AddFixed("RCF" , 0x0c);
  AddFixed("NOP" , 0x00); AddFixed("HALT", 0x01);
  AddFixed("DI"  , 0x02); AddFixed("EI"  , 0x03);
  AddFixed("SWI" , 0xff); AddFixed("RLCA", 0xa0);
  AddFixed("RRCA", 0xa1); AddFixed("RLA" , 0xa2);
  AddFixed("RRA" , 0xa3); AddFixed("SLAA", 0xa4);
  AddFixed("SRAA", 0xa5); AddFixed("SLLA", 0xa6);
  AddFixed("SRLA", 0xa7); AddFixed("RETI", 0x1f);

  AddMove("LDI" , 0x58);
  AddMove("LDIR", 0x59);
  AddMove("LDD" , 0x5a);
  AddMove("LDDR", 0x5b);
  AddMove("CPI" , 0x5c);
  AddMove("CPIR", 0x5d);
  AddMove("CPD" , 0x5e);
  AddMove("CPDR", 0x5f);

  AddShift("RLC", 0xa0, True );
  AddShift("RRC", 0xa1, True );
  AddShift("RL" , 0xa2, True );
  AddShift("RR" , 0xa3, True );
  AddShift("SLA", 0xa4, True );
  AddShift("SRA", 0xa5, True );
  AddShift("SLL", 0xa6, True );
  AddShift("SRL", 0xa7, True );
  AddShift("RLD", 0x10, False);
  AddShift("RRD", 0x11, False);

  AddBit("BIT" , 0xa8);
  AddBit("SET" , 0xb8);
  AddBit("RES" , 0xb0);
  AddBit("TSET", 0x18);

  AddAcc("DAA", 0x0b);
  AddAcc("CPL", 0x10);
  AddAcc("NEG", 0x11);

  InstrZ = 0;
  AddInstTable(InstTable, "ADD", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "ADC", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "SUB", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "SBC", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "AND", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "XOR", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "OR" , InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "CP" , InstrZ++, DecodeALU2);

  InstrZ = 0;
  AddCondition("F"  ,  0); AddCondition("T"  , COND_CODE_TRUE);
  AddCondition("Z"  ,  6); AddCondition("NZ" , 14);
  AddCondition("C"  ,  7); AddCondition("NC" , 15);
  AddCondition("PL" , 13); AddCondition("MI" ,  5);
  AddCondition("P"  , 13); AddCondition("M"  ,  5);
  AddCondition("NE" , 14); AddCondition("EQ" ,  6);
  AddCondition("OV" ,  4); AddCondition("NOV", 12);
  AddCondition("PE" ,  4); AddCondition("PO" , 12);
  AddCondition("GE" ,  9); AddCondition("LT" ,  1);
  AddCondition("GT" , 10); AddCondition("LE" ,  2);
  AddCondition("UGE", 15); AddCondition("ULT",  7);
  AddCondition("UGT", 11); AddCondition("ULE",  3);
  AddCondition(NULL ,  0);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(Conditions);
}

static void MakeCode_90C141(void)
{
  CodeLen = 0; DontPrint = False; OpSize = -1;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*-------------------------------------------------------------------------*/

static Boolean IsDef_90C141(void)
{
  return False;
}

static void SwitchFrom_90C141(void)
{
  DeinitFields();
}

static Boolean ChkMoreOneArg(void)
{
  return (ArgCnt > 1);
}

static void SwitchTo_90C141(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = ChkMoreOneArg;

  PCSymbol = "$";
  HeaderID = 0x53;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_90C141;
  IsDef = IsDef_90C141;
  SwitchFrom = SwitchFrom_90C141;
  InitFields();
}

void code90c141_init(void)
{
  CPU90C141 = AddCPU("90C141", SwitchTo_90C141);
}
