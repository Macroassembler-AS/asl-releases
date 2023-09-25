/* code870c.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TLCS-870/C                                                  */
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
#include "headids.h"
#include "errmsg.h"

#include "code870c.h"

typedef struct
{
  const char *Name;
  Word Code;
} CondRec;

enum
{
  ModNone = -1,
  ModReg8 = 0,
  ModReg16 = 1,
  ModImm = 2,
  ModMem = 3
};

#define MModReg8 (1 << ModReg8)
#define MModReg16 (1 << ModReg16)
#define MModImm (1 << ModImm)
#define MModAbs (1 << ModAbs)
#define MModMem (1 << ModMem)

#define AccReg 0
#define WAReg 0

#define COND_CODE_TRUE 0xde

#define Reg8Cnt 8
static const char Reg8Names[] = "AWCBEDLH";

static CPUVar CPU870C;
static ShortInt OpSize;
static Byte AdrVals[4];
static ShortInt AdrType;
static Byte AdrMode;

static CondRec *Conditions;

/*--------------------------------------------------------------------------*/

static Boolean DecodeRegDisp(tStrComp *pArg, Byte *pRegFlag, LongInt *pDispAcc, Boolean *pFirstFlag)
{
  static const char AdrRegs[][3] =
  {
    "DE", "HL", "IX", "IY", "SP", "C"
  };
  static const int AdrRegCnt = sizeof(AdrRegs) / sizeof(*AdrRegs);
  Boolean OK, NegFlag, NNegFlag;
  char *EPos;
  LongInt DispPart;
  int z;
  tStrComp Remainder;

  *pRegFlag = 0;
  *pDispAcc = 0;
  NegFlag = False;
  OK = True;
  *pFirstFlag = False;
  do
  {
    EPos = QuotMultPos(pArg->str.p_str, "+-");
    NNegFlag = EPos && (*EPos == '-');
    if (EPos)
      StrCompSplitRef(pArg, &Remainder, pArg, EPos);

    for (z = 0; z < AdrRegCnt; z++)
      if (!as_strcasecmp(pArg->str.p_str, AdrRegs[z]))
        break;
    if (z >= AdrRegCnt)
    {
      tSymbolFlags Flags;

      DispPart = EvalStrIntExpressionWithFlags(pArg, Int32, &OK, &Flags);
      *pFirstFlag = *pFirstFlag || mFirstPassUnknown(Flags);
      *pDispAcc = NegFlag ? *pDispAcc - DispPart :  *pDispAcc + DispPart;
    }
    else if ((NegFlag) || (*pRegFlag & (1 << z)))
    {
      WrError(ErrNum_InvAddrMode);
      OK = False;
    }
    else
      *pRegFlag |= 1 << z;

    NegFlag = NNegFlag;
    if (EPos)
      *pArg = Remainder;
  }
  while (OK && EPos);
  if (*pDispAcc != 0)
    *pRegFlag |= 1 << AdrRegCnt;

  return OK;
}

static void DecodeAdr(const tStrComp *pArg, Byte Erl, Boolean IsDest)
{
  static const char Reg16Names[][3] =
  {
    "WA", "BC", "DE", "HL", "IX", "IY", "SP"
  };
  static const int Reg16Cnt = sizeof(Reg16Names) / sizeof(*Reg16Names);

  int z;
  Byte RegFlag;
  LongInt DispAcc;
  Boolean OK, FirstFlag;

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
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    StrCompShorten(&Arg, 1);

    if ((!as_strcasecmp(Arg.str.p_str, "+SP")) && (!IsDest))
    {
      AdrType = ModMem;
      AdrMode = 0xe6;
      goto chk;
    }
    if ((!as_strcasecmp(Arg.str.p_str, "SP-")) && (IsDest))
    {
      AdrType = ModMem;
      AdrMode = 0xe6;
      goto chk;
    }
    if ((!as_strcasecmp(Arg.str.p_str, "PC+A")) && (!IsDest))
    {
      AdrType = ModMem;
      AdrMode = 0x4f;
      goto chk;
    }

    if (DecodeRegDisp(&Arg, &RegFlag, &DispAcc, &FirstFlag))
      switch (RegFlag)
      {
        case 0x40: /* (nnnn) (nn) */
          AdrType = ModMem;
          AdrVals[0] = DispAcc & 0xff;
          if (DispAcc > 0xff)
          {
            AdrMode = 0xe1;
            AdrCnt = 2;
            AdrVals[1] = (DispAcc >> 8) & 0xff;
          }
          else
          {
            AdrMode = 0xe0;
            AdrCnt = 1;
          }
          break;
        case 0x08: /* (IY) */
          AdrType = ModMem;
          AdrMode = 0xe5;
          break;
        case 0x04: /* (IX) */
          AdrType = ModMem;
          AdrMode = 0xe4;
          break;
        case 0x02: /* (HL) */
          AdrType = ModMem;
          AdrMode = 0xe3;
          break;
        case 0x01: /* (DE) */
          AdrType = ModMem;
          AdrMode = 0xe2;
          break;
        case 0x50: /* (SP+dd) */
          if (FirstFlag)
            DispAcc &= 0x7f;
          if (ChkRange(DispAcc, -128, 127))
          {
            AdrType = ModMem;
            AdrMode = 0xd6;
            AdrCnt = 1;
            AdrVals[0] = DispAcc & 0xff;
          }
          break;
        case 0x48: /* (IY+dd) */
          if (FirstFlag)
            DispAcc &= 0x7f;
          if (ChkRange(DispAcc, -128, 127))
          {
            AdrType = ModMem;
            AdrMode = 0xd5;
            AdrCnt = 1;
            AdrVals[0] = DispAcc & 0xff;
          }
          break;
        case 0x44: /* (IX+dd) */
          if (FirstFlag)
            DispAcc &= 0x7f;
          if (ChkRange(DispAcc, -128, 127))
          {
            AdrType = ModMem;
            AdrMode = 0xd4;
            AdrCnt = 1;
            AdrVals[0] = DispAcc & 0xff;
          }
          break;
        case 0x42: /* (HL+dd) */
          if (FirstFlag)
            DispAcc &= 0x7f;
          if (ChkRange(DispAcc, -128, 127))
          {
            AdrType = ModMem;
            AdrMode = 0xd7;
            AdrCnt = 1;
            AdrVals[0] = DispAcc & 0xff;
          }
          break;
        case 0x22:  /* (HL+c) */
          AdrType = ModMem;
          AdrMode = 0xe7;
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
    goto chk;
  }
  else
  {
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
  }

chk:
  if ((AdrType != ModNone) && (!((1<<AdrType) & Erl)))
  {
    AdrType = ModNone;
    AdrCnt = 0;
    WrError(ErrNum_InvAddrMode);
  }
}

static Byte MakeDestMode(Byte AdrMode)
{
  if ((AdrMode & 0xf0) == 0xe0)
    return AdrMode + 0x10;
  else
    return AdrMode - 0x80;
}

static Boolean DecodeSPDisp(const tStrComp *pArg, Byte *pDisp, Boolean *pDispNeg)
{
  Boolean OK;
  LongInt DispAcc;
  tSymbolFlags Flags;

  *pDisp = 0;
  *pDispNeg = False;

  /* avoid ambiguities - LD SP,SP should be coded as LD rr,rr */

  if (IsIndirect(pArg->str.p_str))
    return False;
  if (as_strncasecmp(pArg->str.p_str, "SP", 2))
    return False;
  if (strlen(pArg->str.p_str) < 3)
    return False;

  DispAcc = EvalStrIntExpressionOffsWithFlags(pArg, 2, Int16, &OK, &Flags);
  if (!OK)
    return False;

  if (mFirstPassUnknown(Flags))
    DispAcc &= 0xff;
  if (ChkRange(DispAcc, -255, 255))
  {
    *pDispNeg = DispAcc < 0;
    *pDisp = *pDispNeg ? -DispAcc : DispAcc;
  }
  return True; /* return True even if disp is out of range, addressing mode was properly detected */
}

static Boolean SplitBit(tStrComp *pArg, Byte *Erg)
{
  tStrComp BitArg;
  char *p;

  p = RQuotPos(pArg->str.p_str, '.');
  if (!p)
    return False;
  StrCompSplitRef(pArg, &BitArg, pArg, p);

  if (strlen(BitArg.str.p_str) != 1) return False;
  else if ((*BitArg.str.p_str >= '0') && (*BitArg.str.p_str <= '7'))
  {
    *Erg = *BitArg.str.p_str - '0';
    return True;
  }
  else if (toupper(*BitArg.str.p_str) == 'A')
  {
    *Erg = 8;
    return True;
  }
  else
    return False;
}

static void CodeMem(Byte Entry, Byte Opcode)
{
  BAsmCode[0] = Entry + AdrMode;
  memcpy(BAsmCode + 1, AdrVals, AdrCnt);
  BAsmCode[1 + AdrCnt] = Opcode;
}

/*!------------------------------------------------------------------------
 * \fn     decode_condition(const char *p_cond_str, Word *p_cond_code)
 * \brief  parse condition code
 * \param  p_cond_str source argument
 * \param  p_cond_code machine code if found
 * \return True if found
 * ------------------------------------------------------------------------ */

static Boolean decode_condition(const char *p_cond_str, Word *p_cond_code)
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
 * \fn     cond_code_tf(Word cond_code)
 * \brief  check if condition is true or false
 * \param  cond_code condition code to check
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean cond_code_tf(Word cond_code)
{
  return (cond_code == COND_CODE_TRUE)
      || (cond_code == 0xdf);
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
  Byte HReg, Bit, HCnt, HMode, HVals[2];
  Boolean OK, NegFlag;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "PSW"))
  {
    BAsmCode[2] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0xe8;
      BAsmCode[1] = 0xde;
      CodeLen = 3;
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "RBS"))
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], UInt1, &OK) << 1;
    if (OK)
    {
      BAsmCode[0] = 0xf9;
      CodeLen = 2;
    }
  }
  else if ((!as_strcasecmp(ArgStr[1].str.p_str, "SP")) && (DecodeSPDisp(&ArgStr[2], BAsmCode + 1, &NegFlag)))
  {
    BAsmCode[0] = NegFlag ? 0x3f : 0x37;
    CodeLen = 2;
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    if (!SplitBit(&ArgStr[2], &Bit)) WrError(ErrNum_InvBitPos);
    else
    {
      DecodeAdr(&ArgStr[2], (Bit < 8 ? MModReg8 : 0) | MModMem, False);
      switch (AdrType)
      {
        case ModReg8:
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = 0x58 | Bit;
          break;
        case ModMem:
          if ((Bit < 8) && (AdrMode == 0xe0))
          {
            CodeLen = 2;
            BAsmCode[0] = 0x58 | Bit;
            BAsmCode[1] = AdrVals[0];
          }
          else if (Bit < 8)
          {
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0x58 | Bit);
          }
          else
          {
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0xfc);
          }
          break;
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "CF"))
  {
    if (!SplitBit(&ArgStr[1], &Bit)) WrError(ErrNum_InvBitPos);
    else
    {
      DecodeAdr(&ArgStr[1], (Bit < 8 ? MModReg8 : 0) | MModMem, False);
      switch (AdrType)
      {
        case ModReg8:
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = 0xe8 | Bit;
          break;
        case ModMem:
          if (Bit < 8)
          {
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0xe8 | Bit);
          }
          else
          {
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0xf3);
          }
          break;
      }
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModMem, TRUE);
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModMem | MModImm, FALSE);
        switch (AdrType)
        {
          case ModReg8:
            if (HReg == AccReg)
            {
              CodeLen = 1; /* OK */
              BAsmCode[0] = 0x10 | AdrMode;
            }
            else if (AdrMode == AccReg)
            {
              CodeLen = 1; /* OK */
              BAsmCode[0] = 0x40 | HReg;
            }
            else
            {
              CodeLen = 2; /* OK */
              BAsmCode[0] = 0xe8 | AdrMode;
              BAsmCode[1] = 0x40 | HReg;
            }
            break;
          case ModMem:
            if ((HReg == AccReg) && (AdrMode == 0xe3))   /* A,(HL) */
            {
              CodeLen = 1; /* OK */
              BAsmCode[0] = 0x0d;
            }
            else if ((HReg == AccReg) && (AdrMode == 0xe0)) /* A,(nn) */
            {
              CodeLen = 2; /* OK */
              BAsmCode[0] = 0x0c;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 2 + AdrCnt; /* OK */
              CodeMem(0x00, 0x40 | HReg);
            }
            break;
          case ModImm:
            CodeLen = 2; /* OK */
            BAsmCode[0] = 0x18 | HReg;
            BAsmCode[1] = AdrVals[0];
            break;
        }
        break;
      case ModReg16:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg16 | MModMem | MModImm, FALSE);
        switch (AdrType)
        {
          case ModReg16:
            CodeLen = 2; /* OK */
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0x18 | HReg;
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt; /* OK */
            BAsmCode[0] = AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x48 + HReg;
            break;
          case ModImm:
            CodeLen = 3; /* OK */
            BAsmCode[0] = 0x48 | HReg;
            memcpy(BAsmCode + 1, AdrVals, 2);
            break;
        }
        break;
      case ModMem:
        memcpy(HVals, AdrVals, AdrCnt);
        HCnt = AdrCnt;
        HMode = AdrMode;
        OpSize = 0;
        DecodeAdr(&ArgStr[2], MModReg8 | MModReg16 | MModImm, FALSE);
        switch (AdrType)
        {
          case ModReg8:
            if ((HMode == 0xe3) && (AdrMode == AccReg))   /* (HL),A */
            {
              CodeLen = 1; /* OK */
              BAsmCode[0] = 0x0f;
            }
            else if ((HMode == 0xe0) && (AdrMode == AccReg))
            {
              CodeLen = 2; /* OK */
              BAsmCode[0] = 0x0e;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 2 + HCnt; /* OK */
              BAsmCode[0] = MakeDestMode(HMode);
              memcpy(BAsmCode + 1, HVals, HCnt);
              BAsmCode[1 + HCnt] = 0x78 | AdrMode;
            }
            break;
          case ModReg16:
            CodeLen = 2 + HCnt; /* OK */
            BAsmCode[0] = MakeDestMode(HMode);
            memcpy(BAsmCode + 1, HVals, HCnt);
            BAsmCode[1 + HCnt] = 0x68 | AdrMode;
            break;
          case ModImm:
            if (HMode == 0xe0) /* (nn),nn */
            {
              CodeLen = 3;
              BAsmCode[0] = 0x0a;
              BAsmCode[1] = HVals[0];
              BAsmCode[2] = AdrVals[0];
            }
            else
            {
              CodeLen = 1 + HCnt + 1 + AdrCnt;
              BAsmCode[0] = MakeDestMode(HMode);
              memcpy(BAsmCode + 1, HVals, HCnt);
              BAsmCode[1 + HCnt] = 0xf9;
              BAsmCode[2 + HCnt] = AdrVals[0];
            }
            break;
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
      DecodeAdr(&ArgStr[1], MModReg16 | MModMem, TRUE);
      switch (AdrType)
      {
        case ModReg16:
          CodeLen = 3;
          BAsmCode[0] = 0x48 | AdrMode;
          BAsmCode[1] = AdrInt & 0xff;
          BAsmCode[2] = AdrInt >> 8;
          break;
        case ModMem:
          if (AdrMode == 0xe3) /* HL */
          {
            CodeLen = 3;
            BAsmCode[0] = 0x09;
            BAsmCode[1] = AdrInt & 0xff;
            BAsmCode[2] = AdrInt >> 8;
          }
          else if (AdrMode != 0xe0) WrError(ErrNum_InvAddrMode);  /* (nn) */
          else
          {
            CodeLen = 3;
            BAsmCode[0] = 0x08;
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
    CodeLen = 2;
    BAsmCode[0] = 0xe8;
    BAsmCode[1] = 0xdc | Code;
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg16, False);
    if (AdrType != ModNone)
    {
      if (AdrMode < 4)
      {
        CodeLen = 1;
        BAsmCode[0] = (Code << 7) | 0x50 | AdrMode;
      }
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = 0xd8 | Code;
      }
    }
  }
}

static void DecodeXCH(Word Code)
{
  Byte HReg, HCnt;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModMem, FALSE); /* set IsDest FALSE for mirrored MemOp */
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModMem, FALSE);
        switch (AdrType)
        {
          case ModReg8:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0x70 | HReg;
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0x70 | HReg);
            break;
        }
        break;
      case ModReg16:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg16 | MModMem, FALSE);
        switch (AdrType)
        {
          case ModReg16:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0x78 | HReg;
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0xd8 | HReg);
            break;
        }
        break;
      case ModMem:
        BAsmCode[0] = AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        HCnt = AdrCnt;
        DecodeAdr(&ArgStr[2], MModReg8 | MModReg16, FALSE);
        switch (AdrType)
        {
          case ModReg8:
            CodeLen = 2 + HCnt;
            BAsmCode[1 + HCnt] = 0x70 | AdrMode;
            break;
          case ModReg16:
            CodeLen = 2 + HCnt;
            BAsmCode[1 + HCnt] = 0xd8 | AdrMode;
            break;
        }
        break;
    }
  }
}

static void DecodeALU(Word Code)
{
  Byte HReg, HLen;

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    Byte Bit;

    if (Code != 5) WrError(ErrNum_InvAddrMode); /* XOR only */
    else if (!SplitBit(&ArgStr[2], &Bit)) WrError(ErrNum_InvBitPos);
    else if (Bit >= 8) WrError(ErrNum_InvAddrMode); /* only fixed bit # */
    else
    {
      DecodeAdr(&ArgStr[2], MModReg8 | MModMem, False);
      switch (AdrType)
      {
        case ModReg8:
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = 0x50 | Bit;
          break;
        case ModMem:
          CodeLen = 2 + AdrCnt;
          CodeMem(0x00, 0x50 | Bit);
          break;
      }
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModMem, FALSE); /* (+SP) allowed as dest mem op instead of (SP-) */
    switch (AdrType)
    {
      case ModReg8:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8 | MModMem | MModImm, FALSE);
        switch (AdrType)
        {
          case ModReg8:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = (HReg << 3) | Code;
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, (HReg << 3) | Code);
            break;
          case ModImm:
            if (HReg == AccReg)
            {
              CodeLen = 2;
              BAsmCode[0] = 0x60 | Code;
              BAsmCode[1] = AdrVals[0];
            }
            else
            {
              CodeLen = 3;
              BAsmCode[0] = 0xe8 | HReg;
              BAsmCode[1] = 0x60 | Code;
              BAsmCode[2] = AdrVals[0];
            }
            break;
        }
        break;
      case ModReg16:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModImm | MModMem | MModReg16, FALSE);
        switch (AdrType)
        {
          case ModImm:
            CodeLen = 4;
            BAsmCode[0] = 0xe8 | HReg;
            BAsmCode[1] = 0x68 | Code;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt;
            CodeMem(0x00, 0x80 | (HReg << 3) | Code);
            break;
          case ModReg16:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | AdrMode;
            BAsmCode[1] = 0x80 | (HReg << 3) | Code;
            break;
        }
        break;
      case ModMem:
        if ((0xe0 == AdrMode) && (Code == 7))
        {
          BAsmCode[0] = Code;
          BAsmCode[1] = AdrVals[0];
          HLen = 2;
        }
        else
        {
          CodeMem(0x00, 0x60 | Code);
          HLen = 2 + AdrCnt;
        }
        OpSize = 0;
        DecodeAdr(&ArgStr[2], MModImm, FALSE);
        if (AdrType == ModImm)
        {
          BAsmCode[HLen] = AdrVals[0];
          CodeLen = HLen + 1;
        }
        break;
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModReg16 | MModMem, False);
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 1;
        BAsmCode[0] = 0x20 | Code | AdrMode;
        break;
      case ModReg16:
        CodeLen = 1;
        BAsmCode[0] = 0x30 | Code | AdrMode;
        break;
      case ModMem:
        CodeLen = 2 + AdrCnt;
        CodeMem(0x00, 0xf0 | Code);
        break;
    }
  }
}

static void DecodeReg(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8, False);
    if (AdrType != ModNone)
    {
      CodeLen = 1;
      BAsmCode[0] = Lo(Code) | AdrMode;
      if (Hi(Code))
        BAsmCode[CodeLen++] = Hi(Code);
    }
  }
}

static void DecodeReg16(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg16, False);
    if (AdrType != ModNone)
    {
      CodeLen = 1;
      BAsmCode[0] = Lo(Code) | AdrMode;
      if (Hi(Code))
        BAsmCode[CodeLen++] = Hi(Code);
    }
  }
}

static void DecodeMUL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8, False);
    if (AdrType == ModReg8)
    {
      Byte HReg = AdrMode;
      DecodeAdr(&ArgStr[2], MModReg8, False);
      if (AdrType == ModReg8)
      {
        if ((HReg ^ AdrMode) != 1) WrError(ErrNum_InvRegPair);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | (HReg >> 1);
          BAsmCode[1] = 0xf2;
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
    DecodeAdr(&ArgStr[1], MModReg16, False);
    if (AdrType == ModReg16)
    {
      if ((AdrMode == 1) || (AdrMode > 3)) WrError(ErrNum_InvAddrMode); /* WA DE HL */
      else
      {
        Byte HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg8, False);
        if (AdrType == ModReg8)
        {
          if (AdrMode != 2) WrError(ErrNum_InvAddrMode);  /* C */
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xe8 | HReg;
            BAsmCode[1] = 0xf3;
          }
        }
      }
    }
  }
}

static void DecodeNEG(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CS")) WrError(ErrNum_InvAddrMode);
  else
  {
    DecodeAdr(&ArgStr[2], MModReg16, False);
    if (AdrType == ModReg16)
    {
      CodeLen = 2;
      BAsmCode[0] = 0xe8 | AdrMode;
      BAsmCode[1] = 0xfa;
    }
  }
}

static void DecodeROLD_RORD(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrError(ErrNum_InvAddrMode);
  else
  {
    DecodeAdr(&ArgStr[2], MModMem, False);
    if (AdrType != ModNone)
    {
      CodeLen = 2 + AdrCnt;
      CodeMem(0x00, Code);
    }
  }
}

static void DecodeTEST_CPL_SET_CLR(Word Code)
{
  Byte Bit;

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CF"))
  {
    switch (Lo(Code))
    {
      case 0xc0:
        BAsmCode[0] = 0xc5;
        CodeLen = 1;
        break;
      case 0xc8:
        BAsmCode[0] = 0xc4;
        CodeLen = 1;
        break;
      case 0xe0:
        BAsmCode[0] = 0xc6;
        CodeLen = 1;
        break;
      default:
        WrError(ErrNum_InvAddrMode);
    }
  }
  else if (!SplitBit(&ArgStr[1], &Bit)) WrError(ErrNum_InvBitPos);
  else
  {
    DecodeAdr(&ArgStr[1], (Bit < 8 ? MModReg8 : 0) | MModMem, False);
    switch (AdrType)
    {
      case ModReg8:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = Lo(Code) | Bit;
        break;
      case ModMem:
        if ((Bit < 8) && (AdrMode == 0xe0) && (Lo(Code) != 0xe0)) /* no short addr. for CPL */
        {
          CodeLen = 2;
          BAsmCode[0] = Lo(Code) | Bit;
          BAsmCode[1] = AdrVals[0];
        }
        else if (Bit < 8)
        {
          CodeLen = 2 + AdrCnt;
          CodeMem(0x00, Lo(Code) | Bit);
        }
        else
        {
          CodeLen = 2 + AdrCnt;
          CodeMem(0x00, Hi(Code));
        }
        break;
    }
  }
}

static void DecodeJRS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Integer AdrInt;
    Word cond_code;
    Boolean OK;
    tSymbolFlags Flags;

    /* only T/F allowed */
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
          BAsmCode[0] = 0x80 | ((cond_code - 0xde) << 5) | (AdrInt & 0x1f);
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
    Word cond_code;
    Integer AdrInt;
    int Delta;
    Boolean OK;
    tSymbolFlags Flags;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

      Delta = ((ArgCnt == 1) || (!Hi(cond_code))) ? 2 : 3;

      AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int16, &OK, &Flags) - (EProgCounter() + Delta);
      if (OK)
      {
        if (((AdrInt < -128) || (AdrInt > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          if (3 == Delta)
            BAsmCode[CodeLen++] = Hi(cond_code);
          BAsmCode[CodeLen++] = (ArgCnt == 1) ?  0xfc : Lo(cond_code);
          BAsmCode[CodeLen++] = AdrInt & 0xff;
        }
      }
  }
}

static void DecodeJ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Word cond_code;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    OpSize = 1;
    DecodeAdr(&ArgStr[ArgCnt], MModReg16 | MModMem | MModImm, False);
    switch (AdrType)
    {
      case ModReg16: /* -> JP */
        if (cond_code != COND_CODE_TRUE) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xe8 | AdrMode;
          BAsmCode[1] = Code;
        }
        break;
      case ModMem: /* -> JP */
        if (cond_code != COND_CODE_TRUE) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        else
        {
          CodeLen = 2 + AdrCnt;
          CodeMem(0x00, Code);
        }
        break;
      case ModImm:
      {
        Word Adr = ((Word)(AdrVals[1] << 8)) | AdrVals[0];
        Integer Dist = Adr - (EProgCounter() + 2);
        int Delta = ((ArgCnt == 1) || !Hi(cond_code)) ? 2 : 3;

        /* TODO: the ArgCnt != 1 check is only necessary to get same
           encoding as previous versions.  Encoding 'J xxx' as 'JRS T,XXX'
           if possible would actually be smarter: */

        if ((Dist >= -16) && (Dist < 15) && (ArgCnt != 1) && cond_code_tf(cond_code)) /* JRS T/F */
        {
          CodeLen = 1;
          BAsmCode[0] = 0x80 | ((cond_code - 0xde) << 5) | (Dist & 0x1f);
        }
        else if ((Dist >= -128) && (Dist < 127))
        {
          if (ArgCnt == 1) /* JR dist */
          {
            BAsmCode[CodeLen++] = 0xfc;
            BAsmCode[CodeLen++] = Dist & 0xff;
          }
          else /* JR cc, dist */
          {
            if (3 == Delta)
              BAsmCode[CodeLen++] = Hi(cond_code);
            BAsmCode[CodeLen++] = Lo(cond_code);
            BAsmCode[CodeLen++] = Dist & 0xff;
          }
        }
        else
        {
          if (ArgCnt == 1) /* JP dest */
          {
            BAsmCode[CodeLen++] = 0xfe;
            BAsmCode[CodeLen++] = Lo(Adr);
            BAsmCode[CodeLen++] = Hi(Adr);
          }
          else /* JR !cc, JP dest */
          {
            cond_code ^= 1;
            if (3 == Delta)
              BAsmCode[CodeLen++] = Hi(cond_code);
            BAsmCode[CodeLen++] = Lo(cond_code);
            BAsmCode[CodeLen++] = 3;
            BAsmCode[CodeLen++] = 0xfe;
            BAsmCode[CodeLen++] = Lo(Adr);
            BAsmCode[CodeLen++] = Hi(Adr);
          }
        }
        break;
      }
    }
  }
}

static void DecodeJP_CALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    OpSize = 1;
    DecodeAdr(&ArgStr[1], MModReg16 | MModMem | MModImm, False);
    switch (AdrType)
    {
      case ModReg16:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 | AdrMode;
        BAsmCode[1] = Code;
        break;
      case ModMem:
        CodeLen = 2 + AdrCnt;
        CodeMem(0x00, Code);
        break;
      case ModImm:
        CodeLen = 3;
        BAsmCode[0] = Code;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
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
      BAsmCode[0] = 0x70 | (HVal & 15);
    }
  }
}

/*--------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddCond(const char *NName, Word NCode)
{
  order_array_rsv_end(Conditions, CondRec);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ++].Code = NCode;
}

static void AddReg(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeReg);
}

static void AddReg16(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeReg16);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "LDW", 0, DecodeLDW);
  AddInstTable(InstTable, "PUSH", 0, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 1, DecodePUSH_POP);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "INC", 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 8, DecodeINC_DEC);
  AddInstTable(InstTable, "MUL", 0, DecodeMUL);
  AddInstTable(InstTable, "DIV", 0, DecodeDIV);
  AddInstTable(InstTable, "NEG", 0, DecodeNEG);
  AddInstTable(InstTable, "ROLD", 0xf6, DecodeROLD_RORD);
  AddInstTable(InstTable, "RORD", 0xf7, DecodeROLD_RORD);
  AddInstTable(InstTable, "CLR", 0xfac8, DecodeTEST_CPL_SET_CLR);
  AddInstTable(InstTable, "TEST", 0xfc58, DecodeTEST_CPL_SET_CLR);
  AddInstTable(InstTable, "CPL", 0xfbe0, DecodeTEST_CPL_SET_CLR);
  AddInstTable(InstTable, "SET", 0xf2c0, DecodeTEST_CPL_SET_CLR);
  AddInstTable(InstTable, "JR", 0, DecodeJR);
  AddInstTable(InstTable, "JRS", 0, DecodeJRS);
  AddInstTable(InstTable, "JP", 0xfe, DecodeJP_CALL);
  AddInstTable(InstTable, "J", 0, DecodeJ);
  AddInstTable(InstTable, "CALL", 0xfd, DecodeJP_CALL);
  AddInstTable(InstTable, "CALLV", 0, DecodeCALLV);

  AddFixed("DI"  , 0xc83a);
  AddFixed("EI"  , 0xc03a);
  AddFixed("RET" , 0x00fa);
  AddFixed("RETI", 0x00fb);
  AddFixed("RETN", 0xe8fb);
  AddFixed("SWI" , 0x00ff);
  AddFixed("NOP" , 0x0000);

  InstrZ = 0;
  AddCond("EQ" , 0x00d8); AddCond("Z"  , 0x00d8);
  AddCond("NE" , 0x00d9); AddCond("NZ" , 0x00d9);
  AddCond("CS" , 0x00da); AddCond("LT" , 0x00da);
  AddCond("CC" , 0x00db); AddCond("GE" , 0x00db);
  AddCond("LE" , 0x00dc); AddCond("GT" , 0x00dd);
  AddCond("M"  , 0xe8d0); AddCond("P"  , 0xe8d1);
  AddCond("SLT", 0xe8d2); AddCond("SGE", 0xe8d3);
  AddCond("SLE", 0xe8d4); AddCond("SGT", 0xe8d5);
  AddCond("VS" , 0xe8d6); AddCond("VC" , 0xe8d7);
  AddCond("T"  , COND_CODE_TRUE); AddCond("F"  , 0x00df);
  AddCond(NULL , 0);

  AddReg("DAA" , 0xdae8);  AddReg("DAS" , 0xdbe8);
  AddReg("SHLC", 0xf4e8);  AddReg("SHRC", 0xf5e8);
  AddReg("ROLC", 0xf6e8);  AddReg("RORC", 0xf7e8);
  AddReg("SWAP", 0xffe8);

  AddReg16("SHLCA", 0xf0e8); AddReg16("SHRCA", 0xf1e8);

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

static void MakeCode_870C(void)
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

static Boolean IsDef_870C(void)
{
  return False;
}

static void SwitchFrom_870C(void)
{
  DeinitFields();
}

static Boolean TrueFnc(void)
{
  return True;
}

static void SwitchTo_870C(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("TLCS-870/C");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = TrueFnc;

  PCSymbol = "$";
  HeaderID = FoundDescr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_870C;
  IsDef = IsDef_870C;
  SwitchFrom = SwitchFrom_870C;
  InitFields();
}

void code870c_init(void)
{
  CPU870C = AddCPU("TLCS-870/C", SwitchTo_870C);
}
