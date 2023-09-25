/* code65.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator 65xx/MELPS740                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code65.h"

/*---------------------------------------------------------------------------*/

enum
{
  ModZA    = 0,   /* aa */
  ModA     = 1,   /* aabb */
  ModZIX   = 2,   /* aa,X */
  ModIX    = 3,   /* aabb,X */
  ModZIY   = 4,   /* aa,Y */
  ModIY    = 5,   /* aabb,Y */
  ModIndIX = 6,   /* (aa,X) */
  ModIndOX = 7,   /* (aa),X */
  ModIndOY = 8,   /* (aa),Y */
  ModIndOZ = 9,   /* (aa),Z (65CE02-specific) */
  ModInd16 =10,   /* (aabb) */
  ModImm   =11,   /* #aa */
  ModAcc   =12,   /* A */
  ModNone  =13,   /* */
  ModInd8  =14,   /* (aa) */
  ModIndSPY=15,   /* (aa,SP),Y (65CE02-specific) */
  ModSpec = 16    /* \aabb */
};

typedef struct
{
  Word CPUFlag;
  Byte Code;
} FixedOrder;

typedef struct
{
  LongInt Codes[ModSpec + 1];
} NormOrder;

typedef struct
{
  Word CPUFlag;
  Byte CodeShort, CodeLong;
} CondOrder;

typedef struct
{
  ShortInt ErgMode;
  int AdrCnt;
  Byte AdrVals[2];
} tAdrResult;

/* NOTE: keep in the same order as in registration in code65_init()! */

#define M_6502      (1 << 0)
#define M_65SC02    (1 << 1)
#define M_65C02     (1 << 2)
#define M_65CE02    (1 << 3)
#define M_W65C02S   (1 << 4)
#define M_65C19     (1 << 5)
#define M_MELPS740  (1 << 6)
#define M_HUC6280   (1 << 7)
#define M_6502U     (1 << 8)

static Boolean This_CLI_SEI_Flag, This_ADC_SBC_Flag,
               Last_CLI_SEI_Flag, Last_ADC_SBC_Flag;

static FixedOrder *FixedOrders;
static NormOrder *NormOrders;
static CondOrder *CondOrders;

static CPUVar CPU6502, CPU65SC02, CPU65C02, CPU65CE02, CPUW65C02S, CPU65C19, CPUM740, CPUHUC6280, CPU6502U;
static LongInt SpecPage, RegB, MPR[8];

/*---------------------------------------------------------------------------*/

static unsigned ChkZero(const tStrComp *pArg, Byte *erg)
{
 if ((strlen(pArg->str.p_str) > 1) && ((*pArg->str.p_str == '<') || (*pArg->str.p_str == '>')))
 {
   *erg = Ord(*pArg->str.p_str == '<') + 1;
   return 1;
 }
 else
 {
   *erg = 0;
   return 0;
 }
}

static Boolean CPUAllowed(Word Flag)
{
  return (((Flag >> (MomCPU - CPU6502)) & 1) || False);
}

static void InsNOP(void)
{
  memmove(BAsmCode, BAsmCode + 1, CodeLen);
  CodeLen++;
  BAsmCode[0] = NOPCode;
}

static Boolean IsAllowed(LongInt Val)
{
  return (CPUAllowed(Val >> 8) && (Val != -1));
}

static void ChkZeroMode(tAdrResult *pResult, const NormOrder *pOrder, ShortInt ZeroMode)
{
  if (pOrder && (IsAllowed(pOrder->Codes[ZeroMode])))
  {
    pResult->ErgMode = ZeroMode;
    pResult->AdrCnt--;
  }
}

/*---------------------------------------------------------------------------*/

static Word EvalAddress(const tStrComp *pArg, IntType Type, Boolean *pOK)
{
  /* for the HUC6280, check if the address is within one of the selected pages */

  if (MomCPU == CPUHUC6280)
  {
    Word Page;
    LongWord AbsAddress;
    tSymbolFlags Flags;

    /* get the absolute address */

    AbsAddress = EvalStrIntExpressionWithFlags(pArg, UInt21, pOK, &Flags);
    if (!*pOK)
      return 0;

    /* within one of the 8K pages? */

    for (Page = 0; Page < 8; Page++)
      if ((LargeInt)((AbsAddress >> 13) & 255) == MPR[Page])
        break;
    if (Page >= 8)
    {
      WrError(ErrNum_InAccPage);
      AbsAddress &= 0x1fff;
    }
    else
      AbsAddress = (AbsAddress & 0x1fff) | (Page << 13);

    /* short address requested? */

    if ((Type != UInt16) && (Type != Int16) && Hi(AbsAddress))
    {
      if (mFirstPassUnknown(Flags))
        AbsAddress &= 0xff;
      else
      {
        *pOK = False;
        WrError(ErrNum_OverRange);
      }
    }
    return AbsAddress;
  }

  /* for the 65CE02, regard basepage register */

  else if (MomCPU == CPU65CE02)
  {
    Word Address;
    tSymbolFlags Flags;

    /* alwys get a full 16 bit address */

    Address = EvalStrIntExpressionWithFlags(pArg, UInt16, pOK, &Flags);
    if (!*pOK)
      return 0;

    /* short address requested? */

    if ((Type != UInt16) && (Type != Int16))
    {
      if ((Hi(Address) != RegB) && !mFirstPassUnknown(Flags))
      {
        *pOK = False;
        WrError(ErrNum_OverRange);
      }
      if (*pOK)
        Address &= 0xff;
    }
    return Address;
  }

  else
    return EvalStrIntExpression(pArg, Type, pOK);
}

static Boolean IsBasePage(Byte Page)
{
  return (MomCPU == CPU65CE02) ? (Page == RegB) : !Page;
}

static void DecodeAdr(tAdrResult *pResult, const NormOrder *pOrder)
{
  Word AdrWord;
  Boolean ValOK;
  Byte ZeroMode;

  /* normale Anweisungen: Adressausdruck parsen */

  pResult->ErgMode = -1;

  if (ArgCnt == 0)
  {
    pResult->AdrCnt = 0;
    pResult->ErgMode = ModNone;
  }

  else if (ArgCnt == 1)
  {
    /* 1. Akkuadressierung */

    if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
    {
      pResult->AdrCnt = 0;
      pResult->ErgMode = ModAcc;
    }

    /* 2. immediate ? */

    else if (*ArgStr[1].str.p_str == '#')
    {
      IntType Type = Memo("PHW") ? Int16 : Int8;
      Word Value;

      Value = EvalStrIntExpressionOffs(&ArgStr[1], 1, Type, &ValOK);
      if (ValOK)
      {
        pResult->ErgMode = ModImm;
        pResult->AdrVals[0] = Lo(Value);
        pResult->AdrCnt = 1;
        if (Int16 == Type)
          pResult->AdrVals[pResult->AdrCnt++] = Hi(Value);
      }
    }

    /* 3. Special Page ? */

    else if (*ArgStr[1].str.p_str == '\\')
    {
      tSymbolFlags Flags;

      AdrWord = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, UInt16, &ValOK, &Flags);
      if (ValOK)
      {
        if (mFirstPassUnknown(Flags))
          AdrWord = (SpecPage << 8) | Lo(AdrWord);
        if (Hi(AdrWord) != SpecPage) WrError(ErrNum_UnderRange);
        else
        {
          pResult->ErgMode = ModSpec;
          pResult->AdrVals[0] = Lo(AdrWord);
          pResult->AdrCnt = 1;
        }
      }
    }

    /* 4. X-inner-indirekt ? */

    else if ((strlen(ArgStr[1].str.p_str) >= 5) && (!as_strcasecmp(ArgStr[1].str.p_str + strlen(ArgStr[1].str.p_str) - 3, ",X)")))
    {
      if (*ArgStr[1].str.p_str != '(') WrError(ErrNum_InvAddrMode);
      else
      {
        tStrComp AddrArg;

        StrCompRefRight(&AddrArg, &ArgStr[1], 1);
        StrCompShorten(&AddrArg, 3);
        StrCompIncRefLeft(&AddrArg, ChkZero(&AddrArg, &ZeroMode));
        if (Memo("JMP") || Memo("JSR"))
        {
          AdrWord = EvalAddress(&AddrArg, UInt16, &ValOK);
          if (ValOK)
          {
            pResult->AdrVals[0] = Lo(AdrWord);
            pResult->AdrVals[1] = Hi(AdrWord);
            pResult->ErgMode = ModIndIX;
            pResult->AdrCnt = 2;
          }
        }
        else
        {
          pResult->AdrVals[0] = EvalAddress(&AddrArg, UInt8, &ValOK);
          if (ValOK)
          {
            pResult->ErgMode = ModIndIX;
            pResult->AdrCnt = 1;
          }
        }
      }
    }

    else
    {
      /* 5. indirekt absolut ? */

      if (IsIndirect(ArgStr[1].str.p_str))
      {
        tStrComp AddrArg;

        StrCompRefRight(&AddrArg, &ArgStr[1], 1);
        StrCompShorten(&AddrArg, 1);
        StrCompIncRefLeft(&AddrArg, ChkZero(&AddrArg, &ZeroMode));
        if (ZeroMode == 2)
        {
          pResult->AdrVals[0] = EvalAddress(&AddrArg, UInt8, &ValOK);
          if (ValOK)
          {
            pResult->ErgMode = ModInd8;
            pResult->AdrCnt = 1;
          }
        }
        else
        {
          AdrWord = EvalAddress(&AddrArg, UInt16, &ValOK);
          if (ValOK)
          {
            pResult->ErgMode = ModInd16;
            pResult->AdrCnt = 2;
            pResult->AdrVals[0] = Lo(AdrWord);
            pResult->AdrVals[1] = Hi(AdrWord);
            if ((ZeroMode == 0) && IsBasePage(pResult->AdrVals[1]))
              ChkZeroMode(pResult, pOrder, ModInd8);
          }
        }
      }

      /* 6. absolut */

      else
      {
        tStrComp AddrArg;

        StrCompRefRight(&AddrArg, &ArgStr[1], ChkZero(&ArgStr[1], &ZeroMode));
        if (ZeroMode == 2)
        {
          pResult->AdrVals[0] = EvalAddress(&AddrArg, UInt8, &ValOK);
          if (ValOK)
          {
            pResult->ErgMode = ModZA;
            pResult->AdrCnt = 1;
          }
        }
        else
        {
          AdrWord = EvalAddress(&AddrArg, UInt16, &ValOK);
          if (ValOK)
          {
            pResult->ErgMode = ModA;
            pResult->AdrCnt = 2;
            pResult->AdrVals[0] = Lo(AdrWord);
            pResult->AdrVals[1] = Hi(AdrWord);
            if ((ZeroMode == 0) && IsBasePage(pResult->AdrVals[1]))
              ChkZeroMode(pResult, pOrder, ModZA);
          }
        }
      }
    }
  }

  else if (ArgCnt == 2)
  {
    Boolean Indir1 = IsIndirect(ArgStr[1].str.p_str);

    /* 7. stack-relative (65CE02-specific) ? */

    if (Indir1 && !as_strcasecmp(ArgStr[2].str.p_str, "Y") && (!as_strcasecmp(ArgStr[1].str.p_str + strlen(ArgStr[1].str.p_str) - 4, ",SP)")))
    {
      if (*ArgStr[1].str.p_str != '(') WrError(ErrNum_InvAddrMode);
      else
      {
        tStrComp DistArg;

        StrCompRefRight(&DistArg, &ArgStr[1], 1);
        StrCompShorten(&DistArg, 4);
        pResult->AdrVals[0] = EvalStrIntExpression(&DistArg, UInt8, &ValOK);
        if (ValOK)
        {
          pResult->ErgMode = ModIndSPY;
          pResult->AdrCnt = 1;
        }
      }
    }

    /* 8. X,Y,Z-outer-indirekt ? */

    else if (Indir1
          && (!as_strcasecmp(ArgStr[2].str.p_str, "X")
           || !as_strcasecmp(ArgStr[2].str.p_str, "Y")
           || !as_strcasecmp(ArgStr[2].str.p_str, "Z")))
    {
      int Mode = toupper(ArgStr[2].str.p_str[0]) - 'X';
      tStrComp AddrArg;

      StrCompRefRight(&AddrArg, &ArgStr[1], 1);
      StrCompShorten(&AddrArg, 1);
      StrCompIncRefLeft(&AddrArg, ChkZero(&AddrArg, &ZeroMode));
      pResult->AdrVals[0] = EvalAddress(&AddrArg, UInt8, &ValOK);
      if (ValOK)
      {
        pResult->ErgMode = ModIndOX + Mode;
        pResult->AdrCnt = 1;
      }
    }

    /* 9. X,Y-indiziert ? */

    else
    {
      tStrComp AddrArg;

      StrCompRefRight(&AddrArg, &ArgStr[1], ChkZero(&ArgStr[1], &ZeroMode));
      if (ZeroMode == 2)
      {
        pResult->AdrVals[0] = EvalAddress(&AddrArg, UInt8, &ValOK);
        if (ValOK)
        {
          pResult->AdrCnt = 1;
          if (!as_strcasecmp(ArgStr[2].str.p_str, "X"))
            pResult->ErgMode = ModZIX;
          else if (!as_strcasecmp(ArgStr[2].str.p_str, "Y"))
            pResult->ErgMode = ModZIY;
          else
            WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
        }
      }
      else
      {
        AdrWord = EvalAddress(&AddrArg, Int16, &ValOK);
        if (ValOK)
        {
          pResult->AdrCnt = 2;
          pResult->AdrVals[0] = Lo(AdrWord);
          pResult->AdrVals[1] = Hi(AdrWord);
          if (!as_strcasecmp(ArgStr[2].str.p_str, "X"))
            pResult->ErgMode = ModIX;
          else if (!as_strcasecmp(ArgStr[2].str.p_str, "Y"))
            pResult->ErgMode = ModIY;
          else
            WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
          if (pResult->ErgMode != -1)
          {
            if (IsBasePage(pResult->AdrVals[1]) && (ZeroMode == 0))
              ChkZeroMode(pResult, pOrder, (!as_strcasecmp(ArgStr[2].str.p_str, "X")) ? ModZIX : ModZIY);
          }
        }
      }
    }
  }

  else
    (void)ChkArgCnt(0, 2);
}

static int ImmStart(const char *pArg)
{
  return !!(*pArg == '#');
}

/*---------------------------------------------------------------------------*/

/* Anweisungen ohne Argument */

static void DecodeFixed(Word Index)
{
  const FixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && (ChkExactCPUMask(pOrder->CPUFlag, CPU6502) >= 0))
  {
    CodeLen = 1;
    BAsmCode[0] = pOrder->Code;
    if (MomCPU == CPUM740)
    {
      if (Memo("PLP"))
        BAsmCode[CodeLen++] = NOPCode;
      if ((Last_ADC_SBC_Flag) && (Memo("SEC") || Memo("CLC") || Memo("CLD")))
        InsNOP();
    }
  }
}

/* All right, guys, this really makes tool developers' lives difficult: you can't
   seem to agree on whether BRK is a single or two byte instruction.  Always adding
   a NOP is obviously not what suits the majority of people, so I'll change it to
   an optional argument.  I hope this ends the discussion about BRK.  No, wait, it
   *will* end discussion because I will not answer any further requests about this
   instruction... */

static void DecodeBRK(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(0, 1))
  {
    BAsmCode[0] = 0x00;
    if (ArgCnt > 0)
    {
      Boolean OK;

      BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), Int8, &OK);
      if (OK)
        CodeLen = 2;
    }
    else
      CodeLen = 1;
  }
}

static void DecodeSEB_CLB(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkExactCPU(CPUM740))
  {
    Boolean ValOK;
    Byte BitNo = EvalStrIntExpression(&ArgStr[1], UInt3, &ValOK);

    if (ValOK)
    {
      BAsmCode[0] = Code + (BitNo << 5);
      if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
        CodeLen = 1;
      else
      {
        BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], UInt8, &ValOK);
        if (ValOK)
        {
          CodeLen = 2;
          BAsmCode[0] += 4;
        }
      }
    }
  }
}

static void DecodeBBC_BBS(Word Code)
{
  Boolean ValOK;
  tSymbolFlags Flags;
  int b;

  if (ChkArgCnt(3, 3)
   && ChkExactCPU(CPUM740))
  {
    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt3, &ValOK);
    if (ValOK)
    {
      BAsmCode[0] = (BAsmCode[0] << 5) + Code;
      b = (as_strcasecmp(ArgStr[2].str.p_str, "A") != 0);
      if (!b)
        ValOK = True;
      else
      {
        BAsmCode[0] += 4;
        BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], UInt8, &ValOK);
      }
      if (ValOK)
      {
        Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], Int16, &ValOK, &Flags) - (EProgCounter() + 2 + Ord(b) + Ord(Last_CLI_SEI_Flag));

        if (ValOK)
        {
          if (((AdrInt > 127) || (AdrInt < -128)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            CodeLen = 2 + Ord(b);
            BAsmCode[CodeLen - 1] = AdrInt & 0xff;
            if (Last_CLI_SEI_Flag)
              InsNOP();
          }
        }
      }
    }
  }
}

static void DecodeBBR_BBS(Word Code)
{
  Boolean ValOK;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_HUC6280, CPU6502) >= 0))
  {
    Byte ForceSize;
    unsigned Offset = ChkZero(&ArgStr[1], &ForceSize);

    if (ForceSize == 1) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    else
    {
      BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], Offset, UInt8, &ValOK);
      if (ValOK)
      {
        Integer AdrInt;
        tSymbolFlags Flags;

        BAsmCode[0] = Code;
        AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt16, &ValOK, &Flags) - (EProgCounter() + 3);
        if (ValOK)
        {
          if (((AdrInt > 127) || (AdrInt < -128)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            CodeLen = 3;
            BAsmCode[2] = AdrInt & 0xff;
          }
        }
      }
    }
  }
}

static void DecodeRMB_SMB(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_HUC6280, CPU6502) >= 0))
  {
    Byte ForceSize;
    unsigned Offset = ChkZero(&ArgStr[1], &ForceSize);

    if (ForceSize == 1) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    else
    {
      Boolean ValOK;

      BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], Offset, UInt8, &ValOK);
      if (ValOK)
      {
        BAsmCode[0] = Code;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeRBA_SBA(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkExactCPU(CPU65C19))
  {
    Boolean OK;
    Word Addr;

    Addr = EvalStrIntExpression(&ArgStr[1], UInt16, &OK);
    if (OK)
    {
      BAsmCode[3] = EvalStrIntExpressionOffs(&ArgStr[2], ImmStart(ArgStr[2].str.p_str), UInt8, &OK);
      if (OK)
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = Lo(Addr);
        BAsmCode[2] = Hi(Addr);
        CodeLen = 4;
      }
    }
  }
}

static void DecodeBAR_BAS(Word Code)
{
  if (ChkArgCnt(3, 3)
   && ChkExactCPU(CPU65C19))
  {
    Boolean OK;
    Word Addr;

    Addr = EvalStrIntExpression(&ArgStr[1], UInt16, &OK);
    if (OK)
    {
      BAsmCode[3] = EvalStrIntExpressionOffs(&ArgStr[2], ImmStart(ArgStr[2].str.p_str), UInt8, &OK);
      if (OK)
      {
        tSymbolFlags Flags;
        Integer Dist = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt16, &OK, &Flags) - (EProgCounter() + 5);

        if (OK)
        {
          if (((Dist > 127) || (Dist < -128)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[0] = Code;
            BAsmCode[1] = Lo(Addr);
            BAsmCode[2] = Hi(Addr);
            BAsmCode[4] = Dist & 0xff;
            CodeLen = 5;
          }
        }
      }
    }
  }
}

static void DecodeSTI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && ChkExactCPU(CPU65C19))
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
    if (OK)
    {
      BAsmCode[2] = EvalStrIntExpressionOffs(&ArgStr[2], ImmStart(ArgStr[2].str.p_str), UInt8, &OK);
      if (OK)
      {
        BAsmCode[0] = 0xb2;
        CodeLen = 3;
      }
    }
  }
}

static void DecodeLDM(Word Code)
{
  Boolean ValOK;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && ChkExactCPU(CPUM740))
  {
    BAsmCode[0] = 0x3c;
    BAsmCode[2] = EvalStrIntExpression(&ArgStr[2], UInt8, &ValOK);
    if (ValOK)
    {
      if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int8, &ValOK);
        if (ValOK)
          CodeLen = 3;
      }
    }
  }
}

static void DecodeJSB(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkExactCPU(CPU65C19))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Word Addr;

    Addr = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      Addr = 0xffe0;
    if (OK)
    {
      if ((Addr & 0xffe1) != 0xffe0) WrError(ErrNum_JmpTargOnDiffPage);
      else
      {
        BAsmCode[0] = 0x0b | ((Addr & 0x000e) << 3);
        CodeLen = 1;
      }
    }
  }
}

static void DecodeAUG(Word Code)
{
  if (ChkArgCnt(0, 0) && ChkExactCPU(CPU65CE02))
  {
    BAsmCode[0] = Code;
    BAsmCode[1] =
    BAsmCode[2] =
    BAsmCode[3] = 0x00;
    CodeLen = 4;
  }
}

static void DecodeNorm(Word Index)
{
  const NormOrder *pOrder = NormOrders + Index;
  tAdrResult AdrResult;

  DecodeAdr(&AdrResult, pOrder);
  if (AdrResult.ErgMode != -1)
  {
    if (pOrder->Codes[AdrResult.ErgMode] == -1)
    {
      if (AdrResult.ErgMode == ModZA) AdrResult.ErgMode = ModA;
      if (AdrResult.ErgMode == ModZIX) AdrResult.ErgMode = ModIX;
      if (AdrResult.ErgMode == ModZIY) AdrResult.ErgMode = ModIY;
      if (AdrResult.ErgMode == ModInd8) AdrResult.ErgMode = ModInd16;
      AdrResult.AdrVals[AdrCnt++] = 0;
    }
    if (pOrder->Codes[AdrResult.ErgMode] == -1) WrError(ErrNum_InvAddrMode);
    else if (ChkExactCPUMask(pOrder->Codes[AdrResult.ErgMode] >> 8, CPU6502) >= 0)
    {
      BAsmCode[0] = Lo(pOrder->Codes[AdrResult.ErgMode]);
      memcpy(BAsmCode + 1, AdrResult.AdrVals, AdrResult.AdrCnt);
      CodeLen = AdrResult.AdrCnt + 1;
      if ((AdrResult.ErgMode == ModInd16) && (MomCPU != CPU65C02) && (BAsmCode[1] == 0xff))
      {
        WrError(ErrNum_NotOnThisAddress);
        CodeLen = 0;
      }
    }
  }
}

static void DecodeTST(Word Index)
{
  Byte ImmVal = 0;

  /* split off immediate argument for HUC6280 TST? */

  if (MomCPU == CPUHUC6280)
  {
    Boolean OK;
    int z;

    if (!ChkArgCnt(1, ArgCntMax))
      return;

    ImmVal = EvalStrIntExpressionOffs(&ArgStr[1], ImmStart(ArgStr[1].str.p_str), Int8, &OK);
    if (!OK)
      return;
    for (z = 1; z <= ArgCnt - 1; z++)
      StrCompCopy(&ArgStr[z], &ArgStr[z + 1]);
    ArgCnt--;
  }

  /* generic generation */

  DecodeNorm(Index);

  /* if succeeded, insert immediate value */

  if ((CodeLen > 0) && (MomCPU == CPUHUC6280))
  {
    memmove(BAsmCode + 2, BAsmCode + 1, CodeLen - 1);
    BAsmCode[1] = ImmVal;
  }
}

/* relativer Sprung ? */

static void DecodeCond(Word Index)
{
  const CondOrder *pOrder = CondOrders + Index;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUFlag, CPU6502) >= 0))
  {
    Integer AdrInt;
    Boolean ValOK;
    tSymbolFlags Flags;
    const Boolean MayShort = !!pOrder->CodeShort,
                  MayLong = !!pOrder->CodeLong && (MomCPU == CPU65CE02);
    Byte ForceSize;

    AdrInt = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], ChkZero(&ArgStr[1], &ForceSize), UInt16, &ValOK, &Flags);
    if (!ValOK)
      return;
    if (!ForceSize)
    {
      if (!MayLong)
        ForceSize = 2;
      else if (!MayShort)
        ForceSize = 1;
      else
        ForceSize = RangeCheck(AdrInt - (EProgCounter() + 2), SInt8) ? 2 : 1;
    }

    AdrInt -= EProgCounter() + (4 - ForceSize);
    switch (ForceSize)
    {
      case 2:
        if (!MayShort) WrError(ErrNum_InvAddrMode);
        else if (((AdrInt > 127) || (AdrInt < -128)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = pOrder->CodeShort;
          BAsmCode[1] = AdrInt & 0xff;
          CodeLen = 2;
        }
        break;
      case 1:
        if (!MayLong) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] = pOrder->CodeLong;
          BAsmCode[1] = AdrInt & 0xff;
          BAsmCode[2] = (AdrInt >> 8) & 0xff;
          CodeLen = 3;
        }
        break;
    }
  }
}

static void DecodeTransfer(Word Code)
{
  if (ChkArgCnt(3, 3)
   && ChkExactCPU(CPUHUC6280))
  {
    Boolean OK;
    Word Address;
    int z;

    for (z = 1; z <= 3; z++)
    {
      Address = EvalStrIntExpression(&ArgStr[z], UInt16, &OK);
      if (!OK)
        return;
      BAsmCode[z * 2 - 1] = Lo(Address);
      BAsmCode[z * 2    ] = Hi(Address);
    }
    BAsmCode[0] = Code;
    CodeLen = 7;
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NFlag, Byte NCode)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].CPUFlag = NFlag;
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddNorm(const char *NName, LongWord ZACode, LongWord ACode, LongWord ZIXCode,
                    LongWord IXCode, LongWord ZIYCode, LongWord IYCode, LongWord IndIXCode,
                    LongWord IndOXCode, LongWord IndOYCode, LongWord IndOZCode, LongWord Ind16Code, LongWord ImmCode, LongWord AccCode,
                    LongWord NoneCode, LongWord Ind8Code, LongWord IndSPYCode, LongWord SpecCode)
{
  order_array_rsv_end(NormOrders, NormOrder);
  NormOrders[InstrZ].Codes[ModZA] = ZACode;
  NormOrders[InstrZ].Codes[ModA] = ACode;
  NormOrders[InstrZ].Codes[ModZIX] = ZIXCode;
  NormOrders[InstrZ].Codes[ModIX] = IXCode;
  NormOrders[InstrZ].Codes[ModZIY] = ZIYCode;
  NormOrders[InstrZ].Codes[ModIY] = IYCode;
  NormOrders[InstrZ].Codes[ModIndIX] = IndIXCode;
  NormOrders[InstrZ].Codes[ModIndOX] = IndOXCode;
  NormOrders[InstrZ].Codes[ModIndOY] = IndOYCode;
  NormOrders[InstrZ].Codes[ModIndOZ] = IndOZCode;
  NormOrders[InstrZ].Codes[ModInd16] = Ind16Code;
  NormOrders[InstrZ].Codes[ModImm] = ImmCode;
  NormOrders[InstrZ].Codes[ModAcc] = AccCode;
  NormOrders[InstrZ].Codes[ModNone] = NoneCode;
  NormOrders[InstrZ].Codes[ModInd8] = Ind8Code;
  NormOrders[InstrZ].Codes[ModIndSPY] = IndSPYCode;
  NormOrders[InstrZ].Codes[ModSpec] = SpecCode;
  AddInstTable(InstTable, NName, InstrZ++, strcmp(NName, "TST") ? DecodeNorm : DecodeTST);
}

static void AddCond(const char *NName, Word NFlag, Byte NCodeShort, Byte NCodeLong)
{
  order_array_rsv_end(CondOrders, CondOrder);
  CondOrders[InstrZ].CPUFlag = NFlag;
  CondOrders[InstrZ].CodeShort = NCodeShort;
  CondOrders[InstrZ].CodeLong = NCodeLong;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCond);
}

static LongWord MkMask(Word CPUMask, Byte Code)
{
  return (((LongWord)CPUMask) << 8) | Code;
}

static void InitFields(void)
{
  Boolean Is740 = (MomCPU == CPUM740),
          Is65C19 = (MomCPU == CPU65C19);
  int Bit;
  char Name[20];

  InstTable = CreateInstTable(237);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "SEB", 0x0b, DecodeSEB_CLB);
  AddInstTable(InstTable, "CLB", 0x1b, DecodeSEB_CLB);
  AddInstTable(InstTable, "BBC", 0x13, DecodeBBC_BBS);
  AddInstTable(InstTable, "BBS", 0x03, DecodeBBC_BBS);
  AddInstTable(InstTable, "RBA", 0xc2, DecodeRBA_SBA);
  AddInstTable(InstTable, "SBA", 0xd2, DecodeRBA_SBA);
  AddInstTable(InstTable, "BAR", 0xe2, DecodeBAR_BAS);
  AddInstTable(InstTable, "BAS", 0xf2, DecodeBAR_BAS);
  AddInstTable(InstTable, "STI", 0, DecodeSTI);
  AddInstTable(InstTable, "BRK", 0, DecodeBRK);
  for (Bit = 0; Bit < 8; Bit++)
  {
    as_snprintf(Name, sizeof(Name), "BBR%d", Bit);
    AddInstTable(InstTable, Name, (Bit << 4) + 0x0f, DecodeBBR_BBS);
    as_snprintf(Name, sizeof(Name), "BBS%d", Bit);
    AddInstTable(InstTable, Name, (Bit << 4) + 0x8f, DecodeBBR_BBS);
    as_snprintf(Name, sizeof(Name), "RMB%d", Bit);
    AddInstTable(InstTable, Name, (Bit << 4) + 0x07, DecodeRMB_SMB);
    as_snprintf(Name, sizeof(Name), "SMB%d", Bit);
    AddInstTable(InstTable, Name, (Bit << 4) + 0x87, DecodeRMB_SMB);
  }
  AddInstTable(InstTable, "LDM", 0, DecodeLDM);
  AddInstTable(InstTable, "JSB", 0, DecodeJSB);
  AddInstTable(InstTable, "AUG", 0x5c, DecodeAUG);

  AddInstTable(InstTable, "TAI"  , 0xf3, DecodeTransfer);
  AddInstTable(InstTable, "TDD"  , 0xc3, DecodeTransfer);
  AddInstTable(InstTable, "TIA"  , 0xe3, DecodeTransfer);
  AddInstTable(InstTable, "TII"  , 0x73, DecodeTransfer);
  AddInstTable(InstTable, "TIN"  , 0xd3, DecodeTransfer);

  InstrZ = 0;
  AddFixed("RTS", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x60);
  AddFixed("RTI", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x40);
  AddFixed("TAX", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xaa);
  AddFixed("TXA", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x8a);
  AddFixed("TAY", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa8);
  AddFixed("TYA", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x98);
  AddFixed("TXS", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x9a);
  AddFixed("TSX", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xba);
  AddFixed("TSY",                               M_65CE02                                                         , 0x0b);
  AddFixed("TYS",                               M_65CE02                                                         , 0x2b);
  AddFixed("TAZ",                               M_65CE02                                                         , 0x4b);
  AddFixed("TAB",                               M_65CE02                                                         , 0x5b);
  AddFixed("TZA",                               M_65CE02                                                         , 0x6b);
  AddFixed("TBA",                               M_65CE02                                                         , 0x7b);
  AddFixed("DEX", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xca);
  AddFixed("DEY", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x88);
  AddFixed("DEZ",                               M_65CE02                                                         , 0x3b);
  AddFixed("INX", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xe8);
  AddFixed("INY", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xc8);
  AddFixed("INZ",                               M_65CE02                                                         , 0x1b);
  AddFixed("PHA", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x48);
  AddFixed("PLA", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x68);
  AddFixed("PHP", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x08);
  AddFixed("PLP", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x28);
  AddFixed("PHX",          M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19              | M_HUC6280          , 0xda);
  AddFixed("PLX",          M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19              | M_HUC6280          , 0xfa);
  AddFixed("PHY",          M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19              | M_HUC6280          , 0x5a);
  AddFixed("PLY",          M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19              | M_HUC6280          , 0x7a);
  AddFixed("PHZ",                               M_65CE02                                                         , 0xdb);
  AddFixed("PLZ",                               M_65CE02                                                         , 0xfb);
  AddFixed("STP",                                          M_W65C02S |           M_MELPS740                      , Is740 ? 0x42 : 0xdb);
  AddFixed("WAI",                                          M_W65C02S                                             , 0xcb);
  AddFixed("SLW",                                                                M_MELPS740                      , 0xc2);
  AddFixed("FST",                                                                M_MELPS740                      , 0xe2);
  AddFixed("WIT",                                                                M_MELPS740                      , 0xc2);
  AddFixed("CLI", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x58);
  AddFixed("SEI", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x78);
  AddFixed("CLC", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x18);
  AddFixed("SEC", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x38);
  AddFixed("CLD", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xd8);
  AddFixed("SED", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xf8);
  AddFixed("CLE",                               M_65CE02                                                         , 0x02);
  AddFixed("SEE",                               M_65CE02                                                         , 0x03);
  AddFixed("CLV", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xb8);
  AddFixed("CLT",                                                                M_MELPS740                      , 0x12);
  AddFixed("SET",                                                                M_MELPS740 | M_HUC6280          , Is740 ? 0x32 : 0xf4); /* !!! for HUC6280, is prefix for (x) instead of ACC as dest */
  AddFixed("JAM",                                                                                         M_6502U, 0x02);
  AddFixed("CRS",                                                                                         M_6502U, 0x02);
  AddFixed("KIL",                                                                                         M_6502U, 0x02);
  AddFixed("CLW",                                                      M_65C19                                   , 0x52);
  AddFixed("MPY",                                                      M_65C19                                   , 0x02);
  AddFixed("MPA",                                                      M_65C19                                   , 0x12);
  AddFixed("PSH",                                                      M_65C19                                   , 0x22);
  AddFixed("PUL",                                                      M_65C19                                   , 0x32);
  AddFixed("PLW",                                                      M_65C19                                   , 0x33);
  AddFixed("RND",                                                      M_65C19                                   , 0x42);
  AddFixed("TAW",                                                      M_65C19                                   , 0x62);
  AddFixed("TWA",                                                      M_65C19                                   , 0x72);
  AddFixed("NXT",                                                      M_65C19                                   , 0x8b);
  AddFixed("LII",                                                      M_65C19                                   , 0x9b);
  AddFixed("LAI",                                                      M_65C19                                   , 0xeb);
  AddFixed("INI",                                                      M_65C19                                   , 0xbb);
  AddFixed("PHI",                                                      M_65C19                                   , 0xcb);
  AddFixed("PLI",                                                      M_65C19                                   , 0xdb);
  AddFixed("TIP",                                                      M_65C19                                   , 0x03);
  AddFixed("PIA",                                                      M_65C19                                   , 0xfb);
  AddFixed("LAN",                                                      M_65C19                                   , 0xab);
  AddFixed("CLA",                                                                             M_HUC6280          , 0x62);
  AddFixed("CLX",                                                                             M_HUC6280          , 0x82);
  AddFixed("CLY",                                                                             M_HUC6280          , 0xc2);
  AddFixed("CSL",                                                                             M_HUC6280          , 0x54);
  AddFixed("CSH",                                                                             M_HUC6280          , 0xd4);
  AddFixed("SAY",                                                                             M_HUC6280          , 0x42);
  AddFixed("SXY",                                                                             M_HUC6280          , 0x02);

  InstrZ = 0;
  AddNorm("NOP",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0x04),
  /* A     */ MkMask(                                                                                        M_6502U, 0x0c),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0x14),
  /* IX    */ MkMask(                                                                                        M_6502U, 0x1c),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0x80),
  /* ACC   */     -1,
  /* NON   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xea),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LDA",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa5),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xad),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xb5),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xbd),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xb9),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0xa1),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0xb1),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0xb1),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0xb2),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa9),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, Is65C19 ? 0xa1 : 0xb2),
  /*(n,SP),y*/MkMask(                              M_65CE02                                                         , 0xe2),
  /* spec  */     -1);
  AddNorm("LDX",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa6),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xae),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xb6),
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xbe),
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa2),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LDY",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa4),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xac),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xb4),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xbc),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xa0),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LDZ",
  /* ZA    */     -1,
  /* A     */ MkMask(                              M_65CE02                                                         , 0xab),
  /* ZIX   */     -1,
  /* IX    */ MkMask(                              M_65CE02                                                         , 0xbb),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                              M_65CE02                                                         , 0xa3),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("STA",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x85),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x8d),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x95),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x9d),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x99),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x81),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0x91),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x91),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0x92),
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0x81 : 0x92),
  /*(n,SP),y*/MkMask(                              M_65CE02                                                         , 0x82),
  /* spec  */     -1);
  AddNorm("ST0",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                            M_HUC6280          , 0x03),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ST1",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                            M_HUC6280          , 0x13),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ST2",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                            M_HUC6280          , 0x23),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("STX",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x86),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x8e),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x96),
  /* IY    */ MkMask(                              M_65CE02                                                         , 0x9b),
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("STY",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x84),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x8c),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x94),
  /* IX    */ MkMask(                              M_65CE02                                                         , 0x8b),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("STZ",
  /* ZA    */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x64),
  /* A     */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x9c),
  /* ZIX   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x74),
  /* IX    */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x9e),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ADC",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x65),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x6d),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x75),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x7d),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x79),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x61),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0x71),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x71),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0x72),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x69),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0x61 : 0x72),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ADD",
  /* ZA    */ MkMask(                                                     M_65C19                                   , 0x64),
  /* A     */     -1,
  /* ZIX   */ MkMask(                                                     M_65C19                                   , 0x74),
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                     M_65C19                                   , 0x89),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SBC",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xe5),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xed),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xf5),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xfd),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xf9),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0xe1),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0xf1),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0xf1),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0xf2),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xe9),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0xe1 : 0xf2),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("MUL",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */ MkMask(                                                               M_MELPS740                      , 0x62),
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("DIV",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */ MkMask(                                                               M_MELPS740                      , 0xe2),
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("AND",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x25),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x2d),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x35),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x3d),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x39),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x21),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0x31),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x31),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0x32),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x29),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0x21 : 0x32),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ORA",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x05),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x0d),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x15),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x1d),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x19),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x01),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0x11),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x11),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0x12),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x09),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0x01 : 0x12),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("EOR",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x45),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x4d),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x55),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x5d),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x59),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x41),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0x51),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0x51),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0x52),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x49),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0x41 : 0x52),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("COM",
  /* ZA    */ MkMask(                                                               M_MELPS740                      , 0x44),
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("BIT",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x24),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x2c),
  /* ZIX   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x34),
  /* IX    */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x3c),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S                        | M_HUC6280          , 0x89),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("TST", /* TODO: 6280 */
  /* ZA    */ MkMask(                                                               M_MELPS740 | M_HUC6280          , Is740 ? 0x64 : 0x83),
  /* A     */ MkMask(                                                                            M_HUC6280          , 0x93),
  /* ZIX   */ MkMask(                                                                            M_HUC6280          , 0xa3),
  /* IX    */ MkMask(                                                                            M_HUC6280          , 0xb3),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ASL",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x06),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x0e),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x16),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x1e),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x0a),
  /* NON   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x0a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ASR",
  /* ZA    */ MkMask(                              M_65CE02                                                         , 0x44),
  /* A     */     -1,
  /* ZIX   */ MkMask(                              M_65CE02                                                         , 0x54),
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0x4b),
  /* ACC   */ MkMask(                              M_65CE02             | M_65C19                                   , (MomCPU == CPU65CE02) ? 0x43 : 0x3a),
  /* NON   */ MkMask(                              M_65CE02             | M_65C19                                   , (MomCPU == CPU65CE02) ? 0x43 : 0x3a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ASW",
  /* ZA    */     -1,
  /* A     */ MkMask(                              M_65CE02                                                         , 0xcb),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LAB",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(                                                     M_65C19                                   , 0x13),
  /* NON   */ MkMask(                                                     M_65C19                                   , 0x13),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("NEG",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(                              M_65CE02             | M_65C19                                   , (MomCPU == CPU65CE02) ? 0x42 : 0x1a),
  /* NON   */ MkMask(                              M_65CE02             | M_65C19                                   , (MomCPU == CPU65CE02) ? 0x42 : 0x1a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LSR",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x46),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x4e),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x56),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x5e),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x4a),
  /* NON   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x4a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ROL",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x26),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x2e),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x36),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x3e),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x2a),
  /* NON   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x2a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ROW",
  /* ZA    */     -1,
  /* A     */ MkMask(                              M_65CE02                                                         , 0xeb),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ROR",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x66),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x6e),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x76),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x7e),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x6a),
  /* NON   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x6a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("RRF",
  /* ZA    */ MkMask(                                                               M_MELPS740                      , 0x82),
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("TSB",
  /* ZA    */ MkMask(                   M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S              | M_HUC6280          , 0x04),
  /* A     */ MkMask(                   M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S              | M_HUC6280          , 0x0c),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("TRB",
  /* ZA    */ MkMask(                   M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S              | M_HUC6280          , 0x14),
  /* A     */ MkMask(                   M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S              | M_HUC6280          , 0x1c),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("INC",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xe6),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xee),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xf6),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xfe),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280          , Is740 ? 0x3a : 0x1a),
  /* NON   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280          , Is740 ? 0x3a : 0x1a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("INW",
  /* ZA    */ MkMask(                              M_65CE02                                                         , 0xe3),
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("DEC",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xc6),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xce),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xd6),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xde),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280          , Is740 ? 0x1a : 0x3a),
  /* NON   */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280          , Is740 ? 0x1a : 0x3a),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("DEW",
  /* ZA    */ MkMask(                              M_65CE02                                                         , 0xc3),
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("CMP",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xc5),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xcd),
  /* ZIX   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xd5),
  /* IX    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xdd),
  /* ZIY   */     -1,
  /* IY    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xd9),
  /* (n,X) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0xc1),
  /* (n),X */ MkMask(                                                     M_65C19                                   , 0xd1),
  /* (n),Y */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S           | M_MELPS740 | M_HUC6280 | M_6502U, 0xd1),
  /* (n),Z */ MkMask(                              M_65CE02                                                         , 0xd2),
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xc9),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(         M_65SC02 | M_65C02            | M_W65C02S | M_65C19              | M_HUC6280          , Is65C19 ? 0xc1 : 0xd2),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("CPX",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xe4),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xec),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xe0),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("CPY",
  /* ZA    */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xc4),
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xcc),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xc0),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("CPZ",
  /* ZA    */ MkMask(                              M_65CE02                                                         , 0xd4),
  /* A     */ MkMask(                              M_65CE02                                                         , 0xdc),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                              M_65CE02                                                         , 0xc2),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("JMP",
  /* ZA    */     -1,
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x4c),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */ MkMask(         M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19              | M_HUC6280          , 0x7c),
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x6c),
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(                                                               M_MELPS740                      , 0xb2),
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("JPI",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */ MkMask(                                                     M_65C19                                   , 0x0c),
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("JSR",
  /* ZA    */     -1,
  /* A     */ MkMask(M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x20),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */ MkMask(                              M_65CE02                                                         , 0x23),
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */ MkMask(                              M_65CE02                                                         , 0x22),
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */ MkMask(                                                               M_MELPS740                      , 0x02),
  /*(n,SP),y*/    -1,
  /* spec  */ MkMask(                                                               M_MELPS740                      , 0x22));
  AddNorm("RTN",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                              M_65CE02                                                         , 0x62),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SLO",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0x07),
  /* A     */ MkMask(                                                                                        M_6502U, 0x0f),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0x17),
  /* IX    */ MkMask(                                                                                        M_6502U, 0x1f),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x1b),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0x03),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0x13),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ANC",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0x0b),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("RLA",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0x27),
  /* A     */ MkMask(                                                                                        M_6502U, 0x2f),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0x37),
  /* IX    */ MkMask(                                                                                        M_6502U, 0x3f),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x3b),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0x23),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0x33),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SRE",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0x47),
  /* A     */ MkMask(                                                                                        M_6502U, 0x4f),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0x57),
  /* IX    */ MkMask(                                                                                        M_6502U, 0x5f),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x5b),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0x43),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0x53),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("RRA",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0x67),
  /* A     */ MkMask(                                                                                        M_6502U, 0x6f),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0x77),
  /* IX    */ MkMask(                                                                                        M_6502U, 0x7f),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x7b),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0x63),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0x73),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */    -1);
  AddNorm("ARR",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0x6b),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SAX",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0x87),
  /* A     */ MkMask(                                                                                        M_6502U, 0x8f),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */ MkMask(                                                                                        M_6502U, 0x97),
  /* IY    */     -1,
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0x83),
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */ MkMask(                                                                            M_HUC6280          , 0x22),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ANE",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0x8b),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SHA",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */ MkMask(                                                                                        M_6502U, 0x93),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x9f),
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SHS",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x9b),
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SHY",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0x9c),
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SHX",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */ MkMask(                                                                                        M_6502U, 0x9e),
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LAX",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0xa7),
  /* A     */ MkMask(                                                                                        M_6502U, 0xaf),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */ MkMask(                                                                                        M_6502U, 0xb7),
  /* IY    */ MkMask(                                                                                        M_6502U, 0xbf),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0xa3),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0xb3),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LXA",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0xab),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("LAE",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0xbb),
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("DCP",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0xc7),
  /* A     */ MkMask(                                                                                        M_6502U, 0xcf),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0xd7),
  /* IX    */ MkMask(                                                                                        M_6502U, 0xdf),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0xdb),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0xc3),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0xd3),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("SBX",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                                        M_6502U, 0xcb),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("ISB",
  /* ZA    */ MkMask(                                                                                        M_6502U, 0xe7),
  /* A     */ MkMask(                                                                                        M_6502U, 0xef),
  /* ZIX   */ MkMask(                                                                                        M_6502U, 0xf7),
  /* IX    */ MkMask(                                                                                        M_6502U, 0xff),
  /* ZIY   */     -1,
  /* IY    */ MkMask(                                                                                        M_6502U, 0xfb),
  /* (n,X) */ MkMask(                                                                                        M_6502U, 0xe3),
  /* (n),X */     -1,
  /* (n),Y */ MkMask(                                                                                        M_6502U, 0xf3),
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("EXC",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */ MkMask(                                                     M_65C19                                   , 0xd4),
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */     -1,
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("TAM",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                            M_HUC6280          , 0x53),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("TMA",
  /* ZA    */     -1,
  /* A     */     -1,
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                                                                            M_HUC6280          , 0x43),
  /* ACC   */     -1,
  /* NON   */     -1,
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);
  AddNorm("PHW",
  /* ZA    */     -1,
  /* A     */ MkMask(                              M_65CE02                                                         , 0xfc),
  /* ZIX   */     -1,
  /* IX    */     -1,
  /* ZIY   */     -1,
  /* IY    */     -1,
  /* (n,X) */     -1,
  /* (n),X */     -1,
  /* (n),Y */     -1,
  /* (n),Z */     -1,
  /* (n16) */     -1,
  /* imm   */ MkMask(                              M_65CE02                                                         , 0xf4),
  /* ACC   */     -1,
  /* NON   */ MkMask(                                                     M_65C19                                   , 0x23),
  /* (n8)  */     -1,
  /*(n,SP),y*/    -1,
  /* spec  */     -1);

  InstrZ = 0;
  AddCond("BEQ", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xf0, 0xf3);
  AddCond("BNE", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xd0, 0xd3);
  AddCond("BPL", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x10, 0x13);
  AddCond("BMI", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x30, 0x33);
  AddCond("BCC", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x90, 0x93);
  AddCond("BCS", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0xb0, 0xb3);
  AddCond("BVC", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x50, 0x53);
  AddCond("BVS", M_6502 | M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280 | M_6502U, 0x70, 0x73);
  AddCond("BRA",          M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280          , 0x80, 0x83);
  AddCond("BRU",          M_65SC02 | M_65C02 | M_65CE02 | M_W65C02S | M_65C19 | M_MELPS740 | M_HUC6280          , 0x80, 0x83);
  AddCond("BSR",                               M_65CE02                                    | M_HUC6280          ,
          (MomCPU == CPUHUC6280) ? 0x44 : 0x00,
          (MomCPU == CPU65CE02) ? 0x63 : 0x00);

  init_moto8_pseudo(InstTable, e_moto_8_le | e_moto_8_db | e_moto_8_dw | e_moto_8_ds | e_moto_8_ddb | e_moto_8_dcm);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(FixedOrders);
  order_array_free(NormOrders);
  order_array_free(CondOrders);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_65(void)
{
  CodeLen = 0;
  DontPrint = False;

  This_CLI_SEI_Flag = (Memo("CLI") || Memo("SEI"));
  This_ADC_SBC_Flag = (Memo("ADC") || Memo("SBC"));

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);

  Last_CLI_SEI_Flag = This_CLI_SEI_Flag;
  Last_ADC_SBC_Flag = This_ADC_SBC_Flag;
}

static void InitCode_65(void)
{
  int z;

  Last_CLI_SEI_Flag = False;
  Last_ADC_SBC_Flag = False;
  for (z = 0; z < 8; z++)
    MPR[z] = z;
}

static Boolean IsDef_65(void)
{
  return False;
}

static void SwitchFrom_65(void)
{
  DeinitFields();
}

static Boolean ChkZeroArgs(void)
{
  return (0 == ArgCnt);
}

static void SwitchTo_65(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x11;
  NOPCode = 0xea;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = (MomCPU == CPUHUC6280) ? 0x1fffff : 0xffff;

  if (MomCPU == CPUM740)
  {
    static ASSUMERec ASSUME740s[] =
    {
      { "SP", &SpecPage, 0, 0xff, -1, NULL }
    };

    pASSUMERecs = ASSUME740s;
    ASSUMERecCnt = (sizeof(ASSUME740s) / sizeof(*ASSUME740s));
    SetIsOccupiedFnc = ChkZeroArgs;
  }
  else if (MomCPU == CPUHUC6280)
  {
    static ASSUMERec ASSUME6280s[] =
    {
      { "MPR0", MPR + 0, 0, 0xff, -1, NULL },
      { "MPR1", MPR + 1, 0, 0xff, -1, NULL },
      { "MPR2", MPR + 2, 0, 0xff, -1, NULL },
      { "MPR3", MPR + 3, 0, 0xff, -1, NULL },
      { "MPR4", MPR + 4, 0, 0xff, -1, NULL },
      { "MPR5", MPR + 5, 0, 0xff, -1, NULL },
      { "MPR6", MPR + 6, 0, 0xff, -1, NULL },
      { "MPR7", MPR + 7, 0, 0xff, -1, NULL },
    };

    pASSUMERecs = ASSUME6280s;
    ASSUMERecCnt = (sizeof(ASSUME6280s) / sizeof(*ASSUME6280s));
    SetIsOccupiedFnc = ChkZeroArgs;
  }
  else if (MomCPU == CPU65CE02)
  {
    static ASSUMERec ASSUME65CE02s[] =
    {
      { "B", &RegB, 0, 0xff, 0, NULL }
    };

    pASSUMERecs = ASSUME65CE02s;
    ASSUMERecCnt = (sizeof(ASSUME65CE02s) / sizeof(*ASSUME65CE02s));
  }
  else
    SetIsOccupiedFnc = NULL;

  MakeCode = MakeCode_65;
  IsDef = IsDef_65;
  SwitchFrom = SwitchFrom_65;
  InitFields();
}

void code65_init(void)
{
  CPU6502    = AddCPU("6502"     , SwitchTo_65);
  CPU65SC02  = AddCPU("65SC02"   , SwitchTo_65);
  CPU65C02   = AddCPU("65C02"    , SwitchTo_65);
  CPU65CE02  = AddCPU("65CE02"   , SwitchTo_65);
  CPUW65C02S = AddCPU("W65C02S"  , SwitchTo_65);
  CPU65C19   = AddCPU("65C19"    , SwitchTo_65);
  CPUM740    = AddCPU("MELPS740" , SwitchTo_65);
  CPUHUC6280 = AddCPU("HUC6280"  , SwitchTo_65);
  CPU6502U   = AddCPU("6502UNDOC", SwitchTo_65);

  AddInitPassProc(InitCode_65);
}
