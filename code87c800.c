/* code87c800.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TLCS-870                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <ctype.h>
#include <string.h>

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

#include "code87c800.h"

typedef struct
{
  const char *Name;
  Byte Code;
} CondRec;

enum
{
  ModNone = -1,
  ModReg8 = 0,
  ModReg16 = 1,
  ModImm = 2,
  ModAbs = 3,
  ModMem = 4
};

#define MModReg8 (1 << ModReg8)
#define MModReg16 (1 << ModReg16)
#define MModImm (1 << ModImm)
#define MModAbs (1 << ModAbs)
#define MModMem (1 << ModMem)

#define AccReg 0
#define WAReg 0

#define COND_CODE_TRUE 6

#define Reg8Cnt 8
static const char Reg8Names[] = "AWCBEDLH";

static CPUVar CPU87C00, CPU87C20, CPU87C40, CPU87C70;
static ShortInt OpSize;
static Byte AdrVals[4];
static ShortInt AdrType;
static Byte AdrMode;

static CondRec *Conditions;

/*--------------------------------------------------------------------------*/

static void DecodeAdr(const tStrComp *pArg, Byte Erl)
{
  static const char Reg16Names[][3] =
  {
    "WA", "BC", "DE", "HL"
  };
  static const int Reg16Cnt = sizeof(Reg16Names) / sizeof(*Reg16Names);
  static const char AdrRegs[][3] =
  {
    "HL", "DE", "C", "PC", "A"
  };
  static const int AdrRegCnt = sizeof(AdrRegs) / sizeof(*AdrRegs);

  int z;
  Byte RegFlag;
  Boolean OK;
  LongInt DispAcc;

  AdrType = ModNone;
  AdrCnt = 0;

  if (strlen(pArg->str.p_str) == 1)
  {
    for (z = 0; z < Reg8Cnt; z++)
      if (as_toupper(*pArg->str.p_str) == Reg8Names[z])
      {
        AdrType = ModReg8;
        OpSize = 0;
        AdrMode = z;
        goto chk;
      }
  }

  for (z = 0; z < Reg16Cnt; z++)
    if (!as_strcasecmp(pArg->str.p_str, Reg16Names[z]))
    {
      AdrType = ModReg16;
      OpSize = 1;
      AdrMode = z;
      goto chk;
    }

  if (IsIndirect(pArg->str.p_str))
  {
    tStrComp Arg, Remainder;
    char *EPos;
    Boolean NegFlag, NNegFlag, FirstFlag;
    LongInt DispPart;

    StrCompRefRight(&Arg, pArg, 1);
    StrCompShorten(&Arg, 1);

    if (!as_strcasecmp(Arg.str.p_str, "-HL"))
    {
      AdrType = ModMem;
      AdrMode = 7;
      goto chk;
    }
    if (!as_strcasecmp(Arg.str.p_str, "HL+"))
    {
      AdrType = ModMem;
      AdrMode = 6;
      goto chk;
    }

    RegFlag = 0;
    DispAcc = 0;
    NegFlag = False;
    OK = True;
    FirstFlag = False;
    do
    {
      EPos = QuotMultPos(Arg.str.p_str, "+-");
      NNegFlag = EPos && (*EPos == '-');
      if (EPos)
        StrCompSplitRef(&Arg, &Remainder, &Arg, EPos);

      for (z = 0; z < AdrRegCnt; z++)
        if (!as_strcasecmp(Arg.str.p_str, AdrRegs[z]))
          break;
      if (z >= AdrRegCnt)
      {
        tSymbolFlags Flags;

        DispPart = EvalStrIntExpressionWithFlags(&Arg, Int32, &OK, &Flags);
        FirstFlag |= mFirstPassUnknown(Flags);
        DispAcc = NegFlag ? DispAcc - DispPart :  DispAcc + DispPart;
      }
      else if ((NegFlag) || (RegFlag & (1 << z)))
      {
        WrError(ErrNum_InvAddrMode);
        OK = False;
      }
      else
        RegFlag |= 1 << z;

      NegFlag = NNegFlag;
      if (EPos)
        Arg = Remainder;
    }
    while (EPos && OK);
    if (DispAcc != 0)
      RegFlag |= 1 << AdrRegCnt;
    if (OK)
     switch (RegFlag)
     {
       case 0x20:
         if (FirstFlag)
           DispAcc &= 0xff;
         if (DispAcc > 0xff) WrError(ErrNum_OverRange);
         else
         {
           AdrType = ModAbs;
           AdrMode = 0;
           AdrCnt = 1;
           AdrVals[0] = DispAcc & 0xff;
         }
         break;
       case 0x02:
         AdrType = ModMem;
         AdrMode = 2;
         break;
       case 0x01:
         AdrType = ModMem;
         AdrMode = 3;
         break;
       case 0x21:
         if (FirstFlag)
           DispAcc &= 0x7f;
         if (ChkRange(DispAcc, -128, 127))
         {
           AdrType = ModMem;
           AdrMode = 4;
           AdrCnt = 1;
           AdrVals[0] = DispAcc & 0xff;
         }
         break;
       case 0x05:
         AdrType = ModMem;
         AdrMode = 5;
         break;
       case 0x18:
         AdrType = ModMem;
         AdrMode = 1;
         break;
       default:
         WrError(ErrNum_InvAddrMode);
     }
    goto chk;
  }
  else
   switch (OpSize)
   {
     case -1:
       WrError(ErrNum_UndefOpSizes);
       break;
     case 0:
       AdrVals[0] = EvalStrIntExpression(pArg, Int8, &OK);
       if (OK)
       {
         AdrType = ModImm;
         AdrCnt = 1;
       }
       break;
     case 1:
       DispAcc = EvalStrIntExpression(pArg, Int16, &OK);
       if (OK)
       {
         AdrType = ModImm;
         AdrCnt = 2;
         AdrVals[0] = DispAcc & 0xff;
         AdrVals[1] = (DispAcc >> 8) & 0xff;
       }
       break;
   }

chk:
  if ((AdrType != ModNone) && (!((1 << AdrType) & Erl)))
  {
    AdrType = ModNone;
    AdrCnt = 0;
    WrError(ErrNum_InvAddrMode);
  }
}

static Boolean SplitBit(tStrComp *pArg, Byte *Erg)
{
  char *p;
  tStrComp BitArg;

  p = RQuotPos(pArg->str.p_str, '.');
  if (!p)
    return False;

  StrCompSplitRef(pArg, &BitArg, pArg, p);

  if (strlen(BitArg.str.p_str) != 1) return False;
  else
   if ((*BitArg.str.p_str >= '0') && (*BitArg.str.p_str <= '7'))
   {
     *Erg = *BitArg.str.p_str - '0';
     return True;
   }
   else
   {
     for (*Erg = 0; *Erg < Reg8Cnt; (*Erg)++)
       if (as_toupper(*BitArg.str.p_str) == Reg8Names[*Erg])
         break;
     if (*Erg < Reg8Cnt)
     {
       *Erg += 8;
       return True;
     }
     else
       return False;
   }
}

static void CodeMem(Byte Entry, Byte Opcode)
{
  BAsmCode[0] = Entry + AdrMode;
  memcpy(BAsmCode + 1, AdrVals, AdrCnt);
  BAsmCode[1 + AdrCnt] = Opcode;
}

/*!------------------------------------------------------------------------
 * \fn     decode_condition(const char *p_cond_str, Byte *p_cond_code)
 * \brief  parse condition code
 * \param  p_cond_str source argument
 * \param  p_cond_code returns code if found
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

/*!------------------------------------------------------------------------
 * \fn     cond_code_tf(Byte cond_code)
 * \brief  is condition code True or False?
 * \param  cond_code code to check
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean cond_code_tf(Byte cond_code)
{
  return (cond_code == COND_CODE_TRUE)
      || (cond_code == 7);
}

/*--------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 0;
    if (Hi(Code) != 0)
      BAsmCode[CodeLen++] = Hi(Code);
    BAsmCode[CodeLen++] = Lo(Code);
  }
}

static void DecodeLD(Word Code)
{
  Boolean OK;
  Byte HReg, HCnt, HMode, HVal;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "SP"))
  {
    OpSize=1;
    DecodeAdr(&ArgStr[2], MModImm+MModReg16);
    switch (AdrType)
    {
      case ModReg16:
        CodeLen = 2; BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = 0xfa;
        break;
      case ModImm:
        CodeLen = 3;
        BAsmCode[0] = 0xfa;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        break;
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "SP"))
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    switch (AdrType)
    {
      case ModReg16:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 + AdrMode;
        BAsmCode[1] = 0xfb;
        break;
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "RBS"))
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int4, &OK);
    if (OK)
    {
      CodeLen = 2;
      BAsmCode[0] = 0x0f;
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    if (!SplitBit(&ArgStr[2], &HReg)) WrError(ErrNum_InvBitPos);
    else
    {
      DecodeAdr(&ArgStr[2], MModReg8 | MModAbs | MModMem);
      switch (AdrType)
      {
        case ModReg8:
          if (HReg >= 8) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0xd8 | HReg;
          }
          break;
        case ModAbs:
          if (HReg >= 8) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xd8 | HReg;
            BAsmCode[1] = AdrVals[0];
          }
          break;
        case ModMem:
          if (HReg < 8)
          {
            CodeLen = 2 + AdrCnt;
            CodeMem(0xe0, 0xd8 | HReg);
          }
          else if ((AdrMode != 2) && (AdrMode != 3)) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xe0 | HReg;
            BAsmCode[1] = 0x9c | AdrMode;
          }
          break;
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "CF"))
  {
    if (!SplitBit(&ArgStr[1], &HReg)) WrError(ErrNum_InvBitPos);
    else
    {
      DecodeAdr(&ArgStr[1], MModReg8 | MModAbs | MModMem);
      switch (AdrType)
      {
        case ModReg8:
          if (HReg >= 8) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0xc8 | HReg;
          }
          break;
        case ModAbs:
        case ModMem:
          if (HReg < 8)
          {
            CodeLen = 2 + AdrCnt;
            CodeMem(0xe0, 0xc8 | HReg);
          }
          else if ((AdrMode != 2) && (AdrMode != 3)) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xe0 | HReg;
            BAsmCode[1] = 0x98 | AdrMode;
          }
          break;
      }
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModAbs | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModAbs | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg8:
            if (HReg == AccReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x50 | AdrMode;
            }
            else if (AdrMode == AccReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x58 | HReg;
            }
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0xe8 | AdrMode;
              BAsmCode[1] = 0x58 | HReg;
            }
            break;
          case ModAbs:
            if (HReg == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x22;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xe0;
              BAsmCode[1] = AdrVals[0];
              BAsmCode[2] = 0x58 | HReg;
            }
            break;
          case ModMem:
            if ((HReg == AccReg) && (AdrMode == 3))   /* A,(HL) */
            {
              CodeLen = 1;
              BAsmCode[0] = 0x23;
            }
            else
            {
              CodeLen = 2 + AdrCnt;
              CodeMem(0xe0, 0x58 | HReg);
              if ((HReg >= 6) && (AdrMode == 6)) WrError(ErrNum_Unpredictable);
            }
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
        DecodeAdr(&ArgStr[2], MModReg16 | MModAbs | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg16:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0x14 | HReg;
            break;
          case ModAbs:
            CodeLen = 3;
            BAsmCode[0] = 0xe0;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = 0x14 | HReg;
            break;
          case ModMem:
            if (AdrMode > 5) WrError(ErrNum_InvAddrMode);   /* (-HL), (HL+) */
            else
            {
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0xe0 | AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0x14 + HReg;
            }
            break;
          case ModImm:
            CodeLen = 3;
            BAsmCode[0] = 0x14 | HReg;
            memcpy(BAsmCode + 1, AdrVals, 2);
            break;
        }
        break;
      case ModAbs:
        HReg = AdrVals[0];
        OpSize = 0;
        DecodeAdr(&ArgStr[2], MModReg8 | MModReg16 | MModAbs | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg8:
            if (AdrMode == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x2a;
              BAsmCode[1] = HReg;
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xf0;
              BAsmCode[1] = HReg;
              BAsmCode[2] = 0x50 | AdrMode;
            }
            break;
          case ModReg16:
            CodeLen = 3;
            BAsmCode[0] = 0xf0;
            BAsmCode[1] = HReg;
            BAsmCode[2] = 0x10 | AdrMode;
            break;
          case ModAbs:
            CodeLen = 3;
            BAsmCode[0] = 0x26;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = HReg;
            break;
          case ModMem:
            if (AdrMode > 5) WrError(ErrNum_InvAddrMode);      /* (-HL),(HL+) */
            else
            {
              CodeLen = 3 + AdrCnt;
              BAsmCode[0] = 0xe0 | AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0x26;
              BAsmCode[2 + AdrCnt] = HReg;
            }
            break;
          case ModImm:
            CodeLen = 3;
            BAsmCode[0] = 0x2c;
            BAsmCode[1] = HReg;
            BAsmCode[2] = AdrVals[0];
            break;
        }
        break;
      case ModMem:
        HVal = AdrVals[0];
        HCnt = AdrCnt;
        HMode = AdrMode;
        OpSize = 0;
        DecodeAdr(&ArgStr[2], MModReg8 | MModReg16 | MModAbs | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg8:
            if ((HMode == 3) && (AdrMode == AccReg))   /* (HL),A */
            {
              CodeLen = 1;
              BAsmCode[0] = 0x2b;
            }
            else if ((HMode == 1) || (HMode == 5)) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 2 + HCnt;
              BAsmCode[0] = 0xf0 | HMode;
              memcpy(BAsmCode + 1, &HVal, HCnt);
              BAsmCode[1 + HCnt] = 0x50 | AdrMode;
              if ((HMode == 6) && (AdrMode >= 6)) WrError(ErrNum_Unpredictable);
            }
            break;
          case ModReg16:
            if ((HMode < 2) || (HMode > 4)) WrError(ErrNum_InvAddrMode);  /* (HL),(DE),(HL+d) */
            else
            {
              CodeLen = 2 + HCnt;
              BAsmCode[0] = 0xf0 | HMode;
              memcpy(BAsmCode + 1, &HVal, HCnt);
              BAsmCode[1 + HCnt] = 0x10 | AdrMode;
            }
            break;
          case ModAbs:
            if (HMode != 3) WrError(ErrNum_InvAddrMode);  /* (HL) */
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xe0;
              BAsmCode[1] = AdrVals[0];
              BAsmCode[2] = 0x27;
            }
            break;
          case ModMem:
            if (HMode != 3) WrError(ErrNum_InvAddrMode);         /* (HL) */
            else if (AdrMode > 5) WrError(ErrNum_InvAddrMode);   /* (-HL),(HL+) */
            else
            {
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0xe0 | AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0x27;
            }
            break;
          case ModImm:
            if ((HMode == 1) || (HMode == 5)) WrError(ErrNum_InvAddrMode);  /* (HL+C),(PC+A) */
            else if (HMode == 3)               /* (HL) */
            {
              CodeLen = 2;
              BAsmCode[0] = 0x2d;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3 + HCnt;
              BAsmCode[0] = 0xf0 + HMode;
              memcpy(BAsmCode + 1, &HVal, HCnt);
              BAsmCode[1 + HCnt] = 0x2c;
              BAsmCode[2 + HCnt] = AdrVals[0];
            }
            break;
        }
        break;
    }
  }
}

static void DecodeXCH(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModAbs | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModAbs | MModMem);
        switch (AdrType)
        {
          case ModReg8:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0xa8 | HReg;
            break;
          case ModAbs:
          case ModMem:
            CodeLen = 2 + AdrCnt;
            CodeMem(0xe0, 0xa8 | HReg);
            if ((HReg >= 6) && (AdrMode == 6)) WrError(ErrNum_Unpredictable);
            break;
        }
        break;
      case ModReg16:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg16);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = 0x10 | HReg;
        }
        break;
      case ModAbs:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModReg8);
        if (AdrType != ModNone)
        {
          CodeLen = 3;
          BAsmCode[0] = 0xe0;
          BAsmCode[2] = 0xa8 | AdrMode;
        }
        break;
      case ModMem:
        BAsmCode[0] = 0xe0 | AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        HReg = AdrCnt;
        DecodeAdr(&ArgStr[2], MModReg8);
        if (AdrType != ModNone)
        {
          CodeLen = 2 + HReg;
          BAsmCode[1 + HReg] = 0xa8 | AdrMode;
          if ((AdrMode >= 6) && ((BAsmCode[0] & 0x0f) == 6)) WrError(ErrNum_Unpredictable);
        }
        break;
    }
  }
}

static void DecodeCLR(Word Code)
{
  Byte HReg;;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x0c;
  }
  else if (SplitBit(&ArgStr[1], &HReg))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModAbs | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        if (HReg >= 8) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = 0x48 | HReg;
        }
        break;
      case ModAbs:
        if (HReg >= 8) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0x48 | HReg;
          BAsmCode[1] = AdrVals[0];
        }
        break;
      case ModMem:
        if (HReg <= 8)
        {
          CodeLen = 2 + AdrCnt;
          CodeMem(0xe0, 0x48 | HReg);
        }
        else if ((AdrMode != 2) && (AdrMode != 3)) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe0 | HReg;
          BAsmCode[1] = 0x88 | AdrMode;
        }
        break;
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModAbs | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 2;
        BAsmCode[0] = 0x30 | AdrMode;
        BAsmCode[1] = 0;
        break;
      case ModReg16:
        CodeLen = 3;
        BAsmCode[0] = 0x14 | AdrMode;
        BAsmCode[1] = 0;
        BAsmCode[2] = 0;
        break;
      case ModAbs:
        CodeLen = 2;
        BAsmCode[0] = 0x2e;
        BAsmCode[1] = AdrVals[0];
        break;
      case ModMem:
        if ((AdrMode == 5) || (AdrMode == 1)) WrError(ErrNum_InvAddrMode);  /* (PC+A, HL+C) */
        else if (AdrMode == 3)     /* (HL) */
        {
          CodeLen = 1;
          BAsmCode[0] = 0x2f;
        }
        else
        {
          CodeLen = 3 + AdrCnt;
          BAsmCode[0] = 0xf0 | AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = 0x2c;
          BAsmCode[2 + AdrCnt] = 0;
        }
        break;
    }
  }
}

static void DecodeLDW(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    Integer AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      DecodeAdr(&ArgStr[1], MModReg16 | MModAbs | MModMem);
      switch (AdrType)
      {
        case ModReg16:
          CodeLen = 3;
          BAsmCode[0] = 0x14 | AdrMode;
          BAsmCode[1] = AdrInt & 0xff;
          BAsmCode[2] = AdrInt >> 8;
          break;
        case ModAbs:
          CodeLen = 4;
          BAsmCode[0] = 0x24;
          BAsmCode[1] = AdrVals[0];
          BAsmCode[2] = AdrInt & 0xff;
          BAsmCode[3] = AdrInt >> 8;
          break;
        case ModMem:
          if (AdrMode != 3) WrError(ErrNum_InvAddrMode);  /* (HL) */
          else
          {
            CodeLen = 3;
            BAsmCode[0] = 0x25;
            BAsmCode[1] = AdrInt & 0xff;
            BAsmCode[2] = AdrInt >> 8;
          }
          break;
      }
    }
  }
}

static void DecodePUSH_POP(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "PSW"))
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrType != ModNone)
    {
      CodeLen = 2;
     BAsmCode[0] = 0xe8 | AdrMode;
     BAsmCode[1] = Code;
    }
  }
}

static void DecodeTEST_CPL_SET(Word Code)
{
  Byte HReg;

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    if (Code == 0xd8) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 1;
      BAsmCode[0] = 0x0d + Ord(Code == 0xc0);
    }
  }
  else if (!SplitBit(&ArgStr[1], &HReg)) WrError(ErrNum_InvBitPos);
  else
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModAbs | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        if (HReg >= 8) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = Code | HReg;
        }
        break;
      case ModAbs:
        if (HReg >= 8) WrError(ErrNum_InvAddrMode);
        else if (Code == 0xc0)
        {
          CodeLen = 3;
          CodeMem(0xe0, Code | HReg);
        }
        else
        {
          CodeLen = 2;
          BAsmCode[0] = Code | HReg;
          BAsmCode[1] = AdrVals[0];
        }
        break;
      case ModMem:
        if (HReg < 8)
        {
          CodeLen = 2 + AdrCnt;
          CodeMem(0xe0, Code | HReg);
        }
        else if ((AdrMode != 2) && (AdrMode != 3)) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe0 | HReg;
          BAsmCode[1] = ((Code & 0x18) >> 1) | ((Code & 0x80) >> 3) | 0x80 | AdrMode;
        }
        break;
    }
  }
}

static void DecodeReg(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8);
    if (AdrType != ModNone)
    {
      if (AdrMode == AccReg)
      {
        CodeLen = 1;
        BAsmCode[0] = Code;
      }
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = Code;
      }
    }
  }
}

static void DecodeALU(Word Code)
{
  Byte HReg;
  Boolean OK;

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    if (Code != 5) WrError(ErrNum_InvAddrMode); /* XOR */
    else if (!SplitBit(&ArgStr[2], &HReg)) WrError(ErrNum_InvBitPos);
    else if (HReg >= 8) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(&ArgStr[2], MModReg8 | MModAbs | MModMem);
      switch (AdrType)
      {
        case ModReg8:
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = 0xd0 | HReg;
          break;
        case ModAbs:
        case ModMem:
          CodeLen = 2 + AdrCnt;
          CodeMem(0xe0, 0xd0 | HReg);
          break;
      }
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModMem | MModAbs);
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModMem | MModAbs | MModImm);
        switch (AdrType)
        {
          case ModReg8:
            if (HReg == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0xe8 | AdrMode;
              BAsmCode[1] = 0x60 | Code;
            }
            else if (AdrMode == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0xe8 | HReg;
              BAsmCode[1] = 0x68 | Code;
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModMem:
            if (HReg != AccReg) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0xe0 | AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0x78 | Code;
            }
            break;
          case ModAbs:
            if (HReg != AccReg) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x78 | Code;
              BAsmCode[1] = AdrVals[0];
            }
            break;
          case ModImm:
            if (HReg == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x70 | Code;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xe8 | HReg;
              BAsmCode[1] = 0x70 | Code;
              BAsmCode[2] = AdrVals[0];
            }
            break;
        }
        break;
      case ModReg16:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModImm | MModReg16);
        switch (AdrType)
        {
          case ModImm:
            CodeLen = 4;
            BAsmCode[0] = 0xe8 | HReg;
            BAsmCode[1] = 0x38 | Code;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            break;
          case ModReg16:
            if (HReg != WAReg) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0xe8 | AdrMode;
              BAsmCode[1] = 0x30 | Code;
            }
            break;
        }
        break;
      case ModAbs:
        if (!as_strcasecmp(ArgStr[2].str.p_str, "(HL)"))
        {
          CodeLen = 3;
          BAsmCode[0] = 0xe0;
          BAsmCode[1] = AdrVals[0];
          BAsmCode[2] = 0x60 | Code;
        }
        else
        {
          BAsmCode[3] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
          if (OK)
          {
            CodeLen = 4;
            BAsmCode[0] = 0xe0;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = 0x70 | Code;
          }
        }
        break;
      case ModMem:
        if (!as_strcasecmp(ArgStr[2].str.p_str, "(HL)"))
        {
          CodeLen = 2 + AdrCnt;
          BAsmCode[0] = 0xe0 | AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = 0x60 | Code;
        }
        else
        {
          BAsmCode[2 + AdrCnt] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
          if (OK)
          {
            CodeLen = 3 + AdrCnt;
            BAsmCode[0] = 0xe0 | AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x70 | Code;
          }
        }
        break;
    }
  }
}

static void DecodeMCMP(Word Code)
{
  Byte HReg;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    HReg = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      DecodeAdr(&ArgStr[1], MModMem | MModAbs);
      if (AdrType != ModNone)
      {
        CodeLen = 3 + AdrCnt;
        CodeMem(0xe0, 0x2f);
        BAsmCode[2 + AdrCnt] = HReg;
      }
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModAbs | MModMem);
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 1;
        BAsmCode[0] = 0x60 | Code | AdrMode;
        break;
      case ModReg16:
        CodeLen = 1;
        BAsmCode[0] = 0x10 | Code | AdrMode;
        break;
      case ModAbs:
        CodeLen = 2;
        BAsmCode[0] = 0x20 | Code;
        BAsmCode[1] = AdrVals[0];
        break;
      case ModMem:
        if (AdrMode == 3)     /* (HL) */
        {
          CodeLen = 1;
          BAsmCode[0] = 0x21 | Code;
        }
        else
        {
          CodeLen = 2 + AdrCnt;
          BAsmCode[0] = 0xe0 | AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = 0x20 | Code;
        }
        break;
    }
  }
}

static void DecodeMUL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8);
    if (AdrType == ModReg8)
    {
      Byte HReg = AdrMode;
      DecodeAdr(&ArgStr[2], MModReg8);
      if (AdrType == ModReg8)
      {
        if ((HReg ^ AdrMode) != 1) WrError(ErrNum_InvRegPair);
        else
        {
          HReg = HReg >> 1;
          if (HReg == 0)
          {
            CodeLen = 1;
            BAsmCode[0] = 0x02;
          }
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | HReg;
            BAsmCode[1] = 0x02;
          }
        }
      }
    }
  }
}

static void DecodeDIV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrType == ModReg16)
    {
      Byte HReg = AdrMode;
      DecodeAdr(&ArgStr[2], MModReg8);
      if (AdrType == ModReg8)
      {
        if (AdrMode != 2) WrError(ErrNum_InvAddrMode);  /* C */
        else if (HReg == 0)
        {
          CodeLen = 1;
          BAsmCode[0] = 0x03;
        }
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | HReg;
          BAsmCode[1] = 0x03;
          if (HReg == 1)
            WrError(ErrNum_Unpredictable);
        }
      }
    }
  }
}

static void DecodeROLD_RORD(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrError(ErrNum_InvAddrMode);
  else
  {
    DecodeAdr(&ArgStr[2], MModAbs | MModMem);
    if (AdrType != ModNone)
    {
      CodeLen = 2 + AdrCnt;
      CodeMem(0xe0, Code);
      if (AdrMode == 1)
        WrError(ErrNum_Unpredictable);
    }
  }
}

static void DecodeJRS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Integer AdrInt;
    Byte cond_code;
    Boolean OK;
    tSymbolFlags Flags;

    if (!decode_condition(ArgStr[1].str.p_str, &cond_code) || !cond_code_tf(cond_code)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
    else
    {
      AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], Int16, &OK, &Flags) - (EProgCounter() + 2);
      if (OK)
      {
        if (((AdrInt < -16) || (AdrInt > 15)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 1;
          BAsmCode[0] = ((cond_code - 2) << 5) | (AdrInt & 0x1f);
        }
      }
    }
  }
}

static void DecodeJR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Integer AdrInt;
    Byte cond_code;
    Boolean OK;
    tSymbolFlags Flags;

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
      if (((AdrInt < -128) || (AdrInt > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = (ArgCnt == 1) ?  0xfb : 0xd0 | cond_code;
        BAsmCode[1] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeJP_CALL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    OpSize = 1;
    DecodeAdr(&ArgStr[1], MModReg16 | MModAbs | MModMem | MModImm);
    switch (AdrType)
    {
      case ModReg16:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = Code;
        break;
      case ModAbs:
        CodeLen = 3;
        BAsmCode[0] = 0xe0;
        BAsmCode[1] = AdrVals[0];
        BAsmCode[2] = Code;
        break;
      case ModMem:
        if (AdrMode > 5) WrError(ErrNum_InvAddrMode);
        else
        {
          CodeLen = 2 + AdrCnt;
          BAsmCode[0] = 0xe0 | AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = Code;
        }
        break;
      case ModImm:
        if ((AdrVals[1] == 0xff) && (Code == 0xfc))
        {
          CodeLen = 2;
          BAsmCode[0] = 0xfd;
          BAsmCode[1] = AdrVals[0];
        }
        else
        {
          CodeLen = 3;
          BAsmCode[0] = Code;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        }
        break;
    }
  }
}

static void DecodeCALLV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Byte HVal = EvalStrIntExpression(&ArgStr[1], Int4, &OK);
    if (OK)
    {
      CodeLen = 1;
      BAsmCode[0] = 0xc0 | (HVal & 15);
    }
  }
}

static void DecodeCALLP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Integer AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      if ((Hi(AdrInt) != 0xff) && (Hi(AdrInt) != 0)) WrError(ErrNum_OverRange);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0xfd;
        BAsmCode[1] = Lo(AdrInt);
      }
    }
  }
}

/*--------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddCond(const char *NName, Byte NCode)
{
  order_array_rsv_end(Conditions, CondRec);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ++].Code = NCode;
}

static void AddReg(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeReg);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "CLR", 0, DecodeCLR);
  AddInstTable(InstTable, "LDW", 0, DecodeLDW);
  AddInstTable(InstTable, "PUSH", 7, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 6, DecodePUSH_POP);
  AddInstTable(InstTable, "TEST", 0xd8, DecodeTEST_CPL_SET);
  AddInstTable(InstTable, "CPL", 0xc0, DecodeTEST_CPL_SET);
  AddInstTable(InstTable, "SET", 0x40, DecodeTEST_CPL_SET);
  AddInstTable(InstTable, "MCMP", 0, DecodeMCMP);
  AddInstTable(InstTable, "INC", 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 8, DecodeINC_DEC);
  AddInstTable(InstTable, "MUL", 0, DecodeMUL);
  AddInstTable(InstTable, "DIV", 0, DecodeDIV);
  AddInstTable(InstTable, "ROLD", 8, DecodeROLD_RORD);
  AddInstTable(InstTable, "RORD", 9, DecodeROLD_RORD);
  AddInstTable(InstTable, "JRS", 0, DecodeJRS);
  AddInstTable(InstTable, "JR", 0, DecodeJR);
  AddInstTable(InstTable, "JP", 0xfe, DecodeJP_CALL);
  AddInstTable(InstTable, "CALL", 0xfc, DecodeJP_CALL);
  AddInstTable(InstTable, "CALLV", 0, DecodeCALLV);
  AddInstTable(InstTable, "CALLP", 0, DecodeCALLP);

  AddFixed("DI"  , 0x483a);
  AddFixed("EI"  , 0x403a);
  AddFixed("RET" , 0x0005);
  AddFixed("RETI", 0x0004);
  AddFixed("RETN", 0xe804);
  AddFixed("SWI" , 0x00ff);
  AddFixed("NOP" , 0x0000);

  InstrZ = 0;
  AddCond("EQ", 0); AddCond("Z" , 0);
  AddCond("NE", 1); AddCond("NZ", 1);
  AddCond("CS", 2); AddCond("LT", 2);
  AddCond("CC", 3); AddCond("GE", 3);
  AddCond("LE", 4); AddCond("GT", 5);
  AddCond("T" , COND_CODE_TRUE); AddCond("F" , 7);
  AddCond(NULL, 0);

  AddReg("DAA" , 0x0a);  AddReg("DAS" , 0x0b);
  AddReg("SHLC", 0x1c);  AddReg("SHRC", 0x1d);
  AddReg("ROLC", 0x1e);  AddReg("RORC", 0x1f);
  AddReg("SWAP", 0x01);

  InstrZ = 0;
  AddInstTable(InstTable, "ADDC", InstrZ++, DecodeALU);
  AddInstTable(InstTable, "ADD" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "SUBB", InstrZ++, DecodeALU);
  AddInstTable(InstTable, "SUB" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "AND" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "XOR" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "OR"  , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "CMP" , InstrZ++, DecodeALU);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(Conditions);
}

/*--------------------------------------------------------------------------*/

static void MakeCode_87C800(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = -1;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_87C800(void)
{
  return False;
}

static void SwitchFrom_87C800(void)
{
  DeinitFields();
}

static Boolean TrueFnc(void)
{
  return True;
}

static void SwitchTo_87C800(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = TrueFnc;

  PCSymbol = "$";
  HeaderID = 0x54;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_87C800;
  IsDef = IsDef_87C800;
  SwitchFrom = SwitchFrom_87C800;
  InitFields();
}

void code87c800_init(void)
{
  CPU87C00 = AddCPU("87C00", SwitchTo_87C800);
  CPU87C20 = AddCPU("87C20", SwitchTo_87C800);
  CPU87C40 = AddCPU("87C40", SwitchTo_87C800);
  CPU87C70 = AddCPU("87C70", SwitchTo_87C800);
}
