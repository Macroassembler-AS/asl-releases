/* code85.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 8080/8085                                                   */
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
#include "asmallg.h"
#include "asmcode.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "errmsg.h"
#include "headids.h"

#include "code85.h"

#define set_guessed(flags, offs, len, value) \
        if (mFirstPassUnknownOrQuestionable(flags)) set_basmcode_guessed(offs, len, value);

/*--------------------------------------------------------------------------------------------------*/

typedef enum
{
  ModNone = 0xff,
  ModReg8 = 0,
  ModReg16 = 1,
  ModIReg16 = 2,
  ModAbs = 3,
  ModImm = 4,
  ModIM = 5
} tAdrMode;

typedef struct
{
  Byte values[2];
  unsigned count;
  tSymbolFlags flags;
} adr_vals_t;

#define MModReg8 (1 << ModReg8)
#define MModReg16 (1 << ModReg16)
#define MModIReg16 (1 << ModIReg16)
#define MModAbs (1 << ModAbs)
#define MModImm (1 << ModImm)
#define MModIM (1 << ModIM)

static CPUVar CPU8080, CPUV30EMU, CPU8085, CPU8085U;
static tSymbolSize OpSize;

/*---------------------------------------------------------------------------*/

static const Byte AccReg = 7;

static Boolean DecodeReg8(const char *Asc, tZ80Syntax Syntax, Byte *Erg)
{
  static const char RegNames[] = "BCDEHLMA";
  const char *p;

  if (strlen(Asc) != 1) return False;
  else
  {
    p = strchr(RegNames, as_toupper(*Asc));
    if (!p) return False;
    else
    {
      *Erg = p - RegNames;
      if ((!(Syntax & eSyntax808x)) && (*Erg == 6))
        return False;
      return True;
    }
  }
}

static const Byte BCReg = 0;
static const Byte DEReg = 1;
static const Byte HLReg = 2;
static const Byte SPReg = 3;

static Boolean DecodeReg16(char *pAsc, tZ80Syntax Syntax, Byte *pResult)
{
  static const char RegNames[8][3] = { "B", "D", "H", "SP", "BC", "DE", "HL", "SP" };

  for (*pResult = (Syntax & eSyntax808x) ? 0 : 4;
       *pResult < ((Syntax & eSyntaxZ80) ? 8 : 4);
       (*pResult)++)
    if (!as_strcasecmp(pAsc, RegNames[*pResult]))
    {
      *pResult &= 3;
      break;
    }

  return ((*pResult) < 4);
}

static const char Conditions[][4] =
{
  "NZ", "Z", "NC", "C", "PO", "PE", "P", "M", "NX5", "X5"
};

static Boolean DecodeCondition(const char *pAsc, Byte *pResult, Boolean WithX5)
{
  int ConditionCnt = sizeof(Conditions) / sizeof(*Conditions);

  if (!WithX5 || (MomCPU!= CPU8085U))
    ConditionCnt -= 2;
  for (*pResult = 0; *pResult < ConditionCnt; (*pResult)++)
    if (!as_strcasecmp(pAsc, Conditions[*pResult]))
    {
      *pResult = (*pResult < 8) ? (*pResult << 3) : (0x1b + ((*pResult & 1) << 5));
      return True;
    }
  return False;
}

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->count = 0;
  p_vals->flags = eSymbolFlag_None;
}

static void append_adr_vals(const adr_vals_t *p_vals)
{
  set_guessed(p_vals->flags, CodeLen, p_vals->count, 0xff);
  memcpy(&BAsmCode[CodeLen], p_vals->values, p_vals->count);
  CodeLen += p_vals->count;
}

static tAdrMode DecodeAdr_Z80(tStrComp *pArg, adr_vals_t *p_vals, Word Mask)
{
  Boolean OK;
  tAdrMode AdrMode;
  int ArgLen = strlen(pArg->str.p_str);

  AdrMode = ModNone;
  reset_adr_vals(p_vals);

  if (DecodeReg8(pArg->str.p_str, eSyntaxZ80, &p_vals->values[0]))
  {
    AdrMode = ModReg8;
    goto AdrFound;
  }

  if ((Mask & MModIM) && !as_strcasecmp(pArg->str.p_str, "IM"))
  {
    AdrMode = ModIM;
    goto AdrFound;
  }

  if ((ArgLen == 2) && DecodeReg16(pArg->str.p_str, eSyntaxZ80, &p_vals->values[0]))
  {
    AdrMode = ModReg16;
    OpSize = eSymbolSize16Bit;
    goto AdrFound;
  }

  if (IsIndirect(pArg->str.p_str))
  {
    tStrComp IArg;

    StrCompRefRight(&IArg, pArg, 1);
    StrCompShorten(&IArg, 1);

    if (DecodeReg16(IArg.str.p_str, eSyntaxZ80, &p_vals->values[0]))
    {
      AdrMode = ModIReg16;
      goto AdrFound;
    }
    else
    {
      Word Addr = EvalStrIntExpressionWithFlags(&IArg, UInt16, &OK, &p_vals->flags);

      if (OK)
      {
        p_vals->values[0] = Lo(Addr);
        p_vals->values[1] = Hi(Addr);
        p_vals->count = 2;
        AdrMode = ModAbs;
      }
    }
  }
  else if (OpSize == eSymbolSize16Bit)
  {
    Word Val = EvalStrIntExpressionWithFlags(pArg, Int16, &OK, &p_vals->flags);

    if (OK)
    {
      p_vals->values[0] = Lo(Val);
      p_vals->values[1] = Hi(Val);
      p_vals->count = 2;
      AdrMode = ModImm;
    }
  }
  else
  {
    p_vals->values[0] = EvalStrIntExpressionWithFlags(pArg, Int8, &OK, &p_vals->flags);
    if (OK)
    {
      AdrMode = ModImm;
      p_vals->count = 1;
    }
  }

AdrFound:

  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    reset_adr_vals(p_vals);
    AdrMode = ModNone;
  }
  return AdrMode;
}

/*---------------------------------------------------------------------------*/

/* Anweisungen ohne Operanden */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0)
   && ChkZ80Syntax((tZ80Syntax)Hi(Code)))
  {
    CodeLen = 1;
    BAsmCode[0] = Lo(Code);
  }
}

/* ein 16-Bit-Operand */

static void DecodeOp16(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkZ80Syntax((tZ80Syntax)Hi(Code)))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], Int16, &EvalResult);

    if (EvalResult.OK)
    {
      CodeLen = 3;
      BAsmCode[0] = Lo(Code);
      BAsmCode[1] = Lo(AdrWord);
      BAsmCode[2] = Hi(AdrWord);
      set_guessed(EvalResult.Flags, 1, 2, 0xff);
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeOp8(Word Code)
{
  Boolean OK;
  Byte AdrByte;

  if (ChkArgCnt(1, 1)
   && ChkZ80Syntax((tZ80Syntax)Hi(Code)))
  {
    tSymbolFlags flags;

    AdrByte = EvalStrIntExpressionWithFlags(&ArgStr[1], Int8, &OK, &flags);
    if (OK)
    {
      CodeLen = 2;
      BAsmCode[0] = Lo(Code);
      set_guessed(flags, 1, 1, 0xff);
      BAsmCode[1] = AdrByte;
    }
  }
}

static void DecodeALU(Word Code)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!ChkZ80Syntax((tZ80Syntax)Hi(Code)));
  else if (!DecodeReg8(ArgStr[1].str.p_str, (tZ80Syntax)Hi(Code), &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = Code + Reg;
  }
}

static void DecodeMOV(Word Index)
{
  Byte Dest;

  UNUSED(Index);

  if (!ChkArgCnt(2,  2));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg8(ArgStr[1].str.p_str, eSyntax808x, &Dest)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
  else if (!DecodeReg8(ArgStr[2].str.p_str, eSyntax808x, BAsmCode + 0)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
  else
  {
    BAsmCode[0] += 0x40 + (Dest << 3);
    if (BAsmCode[0] == 0x76)
      WrError(ErrNum_InvRegPair);
    else
      CodeLen = 1;
  }
}

static void DecodeMVI(Word Index)
{
  Boolean OK;
  Byte Reg;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntax808x))
  {
    tSymbolFlags flags;

    BAsmCode[1] = EvalStrIntExpressionWithFlags(&ArgStr[2], Int8, &OK, &flags);
    if (OK)
    {
      if (!DecodeReg8(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
      else
      {
        BAsmCode[0] = 0x06 + (Reg << 3);
        set_guessed(flags, 1, 1, 0xff);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeLXI(Word Index)
{
  Boolean OK;
  Word AdrWord;
  Byte Reg;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntax808x))
  {
    tSymbolFlags flags;

    AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[2], Int16, &OK, &flags);
    if (OK)
    {
      if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
      else
      {
        BAsmCode[0] = 0x01 + (Reg << 4);
        set_guessed(flags, 1, 2, 0xff);
        BAsmCode[1] = Lo(AdrWord);
        BAsmCode[2] = Hi(AdrWord);
        CodeLen = 3;
      }
    }
  }
}

static void DecodeLDAX_STAX(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
  else
  {
    switch (Reg)
    {
      case 3:                             /* SP */
        WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        break;
      case 2:                             /* H --> MOV A,M oder M,A */
        CodeLen = 1;
        BAsmCode[0] = 0x77 + (Index * 7);
        break;
      default:
        CodeLen = 1;
        BAsmCode[0] = 0x02 + (Reg << 4) + (Index << 3);
        break;
    }
  }
}

static void DecodePUSH_POP(Word Index)
{
  Byte Reg = 0;
  Boolean OK = False;

  if (ChkArgCnt(1, 1))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "PSW"))
    {
      if (ChkZ80Syntax(eSyntax808x))
      {
        Reg = 3;
        OK = True;
      }
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "AF"))
    {
      if (ChkZ80Syntax(eSyntaxZ80))
      {
        Reg = 3;
        OK = True;
      }
    }
    else if (DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg))
      OK = (Reg != 3);
    else
      OK = False;
    if (!OK) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
    else
    {
      CodeLen = 1;
      BAsmCode[0] = 0xc1 + (Reg << 4) + Index;
    }
  }
}

static void DecodeRST(Word Index)
{
  Byte AdrByte;
  Boolean OK;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if ((MomCPU >= CPU8085U) && (!as_strcasecmp(ArgStr[1].str.p_str, "V")))
  {
    if (ChkZ80Syntax(eSyntaxZ80))
    {
      CodeLen = 1;
      BAsmCode[0] = 0xcb;
    }
  }
  else
  {
    tSymbolFlags Flags;

    AdrByte = EvalStrIntExpressionWithFlags(&ArgStr[1], (CurrZ80Syntax & eSyntaxZ80) ? UInt6 : UInt3, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      AdrByte = 0;
    if (OK)
    {
      tZ80Syntax Syntax = (CurrZ80Syntax == eSyntaxBoth) ? ((AdrByte < 8) ? eSyntax808x : eSyntaxZ80): CurrZ80Syntax;

      if (Syntax == eSyntax808x)
      {
        set_guessed(Flags, 0, 1, 0x38);
        BAsmCode[CodeLen++] = 0xc7 + (AdrByte << 3);
      }
      else if (AdrByte & 7)
        WrStrErrorPos(ErrNum_NotAligned, &ArgStr[1]);
      else
      {
        set_guessed(Flags, 0, 1, 0x38);
        BAsmCode[CodeLen++] = 0xc7 + (AdrByte & 0x38);
      }
    }
  }
}

static void DecodeINR_DCR(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg8(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x04 + (Reg << 3) + Index;
  }
}

static void DecodeINX_DCX(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x03 + (Reg << 4) + Index;
  }
}

static void DecodeDAD(Word Index)
{
  Byte Reg;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x09 + (Reg << 4);
  }
}

static void DecodeDSUB(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(0, 1) && ChkMinCPU(CPU8085U) && ChkZ80Syntax(eSyntax808x))
  {
    Byte Reg;

    if ((ArgCnt == 1)
     && (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg) || (Reg != BCReg))) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
    else
    {
      CodeLen = 1;
      BAsmCode[0] = 0x08;
    }
  }
}

static void DecodeLHLX_SHLX(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(0, 1) && ChkMinCPU(CPU8085U) && ChkZ80Syntax(eSyntax808x))
  {
    Byte Reg;

    if ((ArgCnt == 1)
     && (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg) || (Reg != DEReg))) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
    else
    {
      CodeLen = 1;
      BAsmCode[0] = Index ? 0xed: 0xd9;
    }
  }
}

static void DecodeLD(Word Code)
{
  Word Mask;
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntaxZ80))
  {
    adr_vals_t dest_adr_vals, src_adr_vals;

    Mask = MModReg8 | MModIReg16 | MModAbs | MModReg16;
    if (MomCPU >= CPU8085)
      Mask |= MModIM;
    switch (DecodeAdr_Z80(&ArgStr[1], &dest_adr_vals, Mask))
    {
      case ModReg8:
        Mask = MModReg8 | MModIReg16 | MModImm;
        if (dest_adr_vals.values[0] == AccReg)
        {
          Mask |= MModAbs;
          if (MomCPU >= CPU8085)
            Mask |= MModIM;
        }
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, Mask))
        {
          case ModReg8:
            BAsmCode[CodeLen++] = 0x40 | (dest_adr_vals.values[0] << 3) | src_adr_vals.values[0];
            break;
          case ModIReg16:
            if (src_adr_vals.values[0] == HLReg)
              BAsmCode[CodeLen++] = 0x46 | (dest_adr_vals.values[0] << 3);
            else if ((dest_adr_vals.values[0] == AccReg) && (src_adr_vals.values[0] != SPReg))
              BAsmCode[CodeLen++] = 0x0a | (src_adr_vals.values[0] << 4);
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          case ModAbs:
            BAsmCode[CodeLen++] = 0x3a;
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0x06 | (dest_adr_vals.values[0] << 3);
            append_adr_vals(&src_adr_vals);
            break;
          case ModIM:
            BAsmCode[CodeLen++] = 0x20;
            break;
          default:
            break;
        }
        break;
      case ModReg16:
      {
        Word Mask = MModImm;

        if (dest_adr_vals.values[0] == HLReg)
        {
          Mask |= MModAbs;
          if (MomCPU == CPU8085U)
            Mask |= MModIReg16;
        }
        if (dest_adr_vals.values[0] == SPReg)
          Mask |= MModReg16;
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, Mask))
        {
          case ModImm:
            BAsmCode[CodeLen++] = 0x01 | (dest_adr_vals.values[0] << 4);
            append_adr_vals(&src_adr_vals);
            break;
          case ModAbs:
            BAsmCode[CodeLen++] = 0x2a;
            append_adr_vals(&src_adr_vals);
            break;
          case ModReg16:
            if (src_adr_vals.values[0] == HLReg)
              BAsmCode[CodeLen++] = 0xf9;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          case ModIReg16:
            if (src_adr_vals.values[0] == DEReg)
              BAsmCode[CodeLen++] = 0xed;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
        break;
      }
      case ModIReg16:
      {
        Word Mask = MModReg8;

        if (dest_adr_vals.values[0] == HLReg)
          Mask |= MModImm;
        if ((dest_adr_vals.values[0] == DEReg) && (MomCPU == CPU8085U))
          Mask |= MModReg16;
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, Mask))
        {
          case ModReg8:
            if (dest_adr_vals.values[0] == HLReg)
              BAsmCode[CodeLen++] = 0x70 | src_adr_vals.values[0];
            else if ((src_adr_vals.values[0] == AccReg) && (dest_adr_vals.values[0] != SPReg))
              BAsmCode[CodeLen++] = 0x02 | (dest_adr_vals.values[0] << 4);
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0x36;
            append_adr_vals(&src_adr_vals);
            break;
          case ModReg16:
            if (src_adr_vals.values[0] == HLReg)
              BAsmCode[CodeLen++] = 0xd9;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
        break;
      }
      case ModAbs:
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg8 | MModReg16))
        {
          case ModReg8:
            if (src_adr_vals.values[0] != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x32;
              append_adr_vals(&dest_adr_vals);
            }
            break;
          case ModReg16:
            if (src_adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x22;
              append_adr_vals(&dest_adr_vals);
            }
            break;
          default:
            break;
        }
        break;
      case ModIM:
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg8))
        {
          case ModReg8:
            if (src_adr_vals.values[0] != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
              BAsmCode[CodeLen++] = 0x30;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeEX(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntaxZ80))
  {
    adr_vals_t dest_adr_vals, src_adr_vals;

    switch (DecodeAdr_Z80(&ArgStr[1], &dest_adr_vals, MModReg16 | MModIReg16))
    {
      case ModReg16:
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg16 | MModIReg16))
        {
          case ModReg16:
            if (((dest_adr_vals.values[0] == DEReg) && (src_adr_vals.values[0] == HLReg))
             || ((dest_adr_vals.values[0] == HLReg) && (src_adr_vals.values[0] == DEReg)))
              BAsmCode[CodeLen++] = 0xeb;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          case ModIReg16:
            if ((dest_adr_vals.values[0] == HLReg) && (src_adr_vals.values[0] == SPReg))
              BAsmCode[CodeLen++] = 0xe3;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
        break;
      case ModIReg16:
        switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg16))
        {
          case ModReg16:
            if ((dest_adr_vals.values[0] == SPReg) && (src_adr_vals.values[0] == HLReg))
              BAsmCode[CodeLen++] = 0xe3;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeADD(Word Code)
{
  int MinArg, MaxArg;

  UNUSED(Code);

  MinArg = (CurrZ80Syntax & eSyntax808x) ? 1 : 2;
  if (CurrZ80Syntax & eSyntaxZ80)
    MaxArg = (MomCPU == CPU8085U) ? 3 : 2;
  else
    MaxArg = 1;
  if (!ChkArgCnt(MinArg, MaxArg))
    return;

  switch (ArgCnt)
  {
    case 1: /* 8080 style - 8 bit register src only, dst is implicitly ACC */
    {
      Byte Reg;

      if (!DecodeReg8(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
      else
        BAsmCode[CodeLen++] = 0x80 | Reg;
      break;
    }
    case 2: /* Z80 style - dst may be HL or A and is first arg */
    {
      adr_vals_t dest_adr_vals, src_adr_vals;

      switch (DecodeAdr_Z80(&ArgStr[1], &dest_adr_vals, MModReg8 | MModReg16))
      {
        case ModReg8:
          if (dest_adr_vals.values[0] != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else
          {
            switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg8 | MModIReg16 | MModImm))
            {
              case ModReg8:
                BAsmCode[CodeLen++] = 0x80 | src_adr_vals.values[0];
                break;
              case ModIReg16:
                if (src_adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
                else
                  BAsmCode[CodeLen++] = 0x86;
                break;
              case ModImm:
                BAsmCode[CodeLen++] = 0xc6;
                append_adr_vals(&src_adr_vals);
                break;
              default:
                break;
            }
          }
          break;
        case ModReg16:
          if (dest_adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else
          {
            switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg16))
            {
              case ModReg16:
                BAsmCode[CodeLen++] = 0x09 | (src_adr_vals.values[0] << 4);
                break;
              default:
                break;
            }
          }
          break;
        default:
          break;
      }
      break;
    }
    case 3: /* Z80 style of undoc 8085 : DE <- (HL,SP) + m */
    {
      Byte Reg;

      if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg) || (Reg != DEReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else if (!DecodeReg16(ArgStr[2].str.p_str, CurrZ80Syntax, &Reg) || (Reg < 2)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      else
      {
        Boolean OK;
        tSymbolFlags flags;

        BAsmCode[1] = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt8, &OK, &flags);
        if (OK)
        {
          BAsmCode[0] = 0x08 | (Reg << 4);
          set_guessed(flags, 1, 1, 0xff);
          CodeLen = 2;
        }
      }
      break;
    }
  }
}

static void DecodeADC(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt((CurrZ80Syntax & eSyntax808x) ? 1 : 2, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
      return;

  switch (ArgCnt)
  {
    case 1: /* 8080 style - 8 bit register src only */
    {
      Byte Reg;

      if (!DecodeReg8(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
      else
        BAsmCode[CodeLen++] = 0x88 | Reg;
      break;
    }
    case 2:
    {
      adr_vals_t dest_adr_vals, src_adr_vals;

      switch (DecodeAdr_Z80(&ArgStr[1], &dest_adr_vals, MModReg8))
      {
        case ModReg8:
          if (dest_adr_vals.values[0] != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else
          {
            switch (DecodeAdr_Z80(&ArgStr[2], &src_adr_vals, MModReg8 | MModIReg16 | MModImm))
            {
              case ModReg8:
                BAsmCode[CodeLen++] = 0x88 | src_adr_vals.values[0];
                break;
              case ModIReg16:
                if (src_adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
                else
                  BAsmCode[CodeLen++] = 0x8e;
                break;
              case ModImm:
                BAsmCode[CodeLen++] = 0xce;
                append_adr_vals(&src_adr_vals);
                break;
              default:
                break;
            }
          }
          break;
        default:
          break;
      }
      break;
    }
  }
}

static void DecodeSUB(Word Code)
{
  Byte Reg;
  tAdrMode AdrMode;
  adr_vals_t adr_vals;

  UNUSED(Code);

  /* dest operand is always A and also optional for Z80 mode, so min arg cnt is always 1! */

  if (!ChkArgCnt(1, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  /* For Z80, optionally allow A as dest */

  if (ArgCnt == 2)
  {
    switch ((AdrMode = DecodeAdr_Z80(&ArgStr[1], &adr_vals, MModReg8 | ((MomCPU == CPU8085U) ? MModReg16 : 0))))
    {
      case ModNone:
        return;
      case ModReg8:
        if (adr_vals.values[0] != AccReg)
          goto error;
        break;
      case ModReg16:
        if (adr_vals.values[0] != HLReg)
          goto error;
        break;
      default:
      error:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        return;
    }
  }
  else
    AdrMode = ModReg8;

  if (DecodeReg8(ArgStr[ArgCnt].str.p_str, CurrZ80Syntax, &Reg)) /* 808x style incl. M, Z80 style excl. (HL) */
  {
    BAsmCode[CodeLen++] = 0x90 | Reg;
    return;
  }

  /* rest is Z80 style ( (HL) or immediate) */

  if (!(CurrZ80Syntax & eSyntaxZ80))
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], &adr_vals, MModImm | MModIReg16 | ((AdrMode == ModReg16) ? MModReg16 : 0)))
  {
    case ModReg16:
      if (adr_vals.values[0] != BCReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
        BAsmCode[CodeLen++] = 0x08;
      break;
    case ModIReg16:
      if (adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
        BAsmCode[CodeLen++] = 0x96;
      break;
    case ModImm:
      BAsmCode[CodeLen++] = 0xd6;
      append_adr_vals(&adr_vals);
      break;
    default:
      break;
  }
}

static void DecodeALU8_Z80(Word Code)
{
  adr_vals_t adr_vals;

  if (!ChkZ80Syntax(eSyntaxZ80)
   || !ChkArgCnt(1, 2))
    return;

  if (ArgCnt == 2) /* A as dest */
  {
    switch (DecodeAdr_Z80(&ArgStr[1], &adr_vals, MModReg8))
    {
      case ModNone:
        return;
      case ModReg8:
        if (adr_vals.values[0] == AccReg)
          break;
        /* else fall-through */
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        return;
    }
  }

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], &adr_vals, MModImm | MModIReg16 | MModReg8))
  {
    case ModReg8:
      BAsmCode[CodeLen++] = 0x80 | (Code << 3) | adr_vals.values[0];
      break;
    case ModIReg16:
      if (adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
        BAsmCode[CodeLen++] = 0x86 | (Code << 3);
      break;
    case ModImm:
      BAsmCode[CodeLen++] = 0xc6 | (Code << 3);
      append_adr_vals(&adr_vals);
      break;
    default:
      break;
  }
}

static void DecodeINCDEC(Word Code)
{
  if (ChkZ80Syntax(eSyntaxZ80)
   && ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr_Z80(&ArgStr[1], &adr_vals, MModReg8 | MModReg16 | MModIReg16))
    {
      case ModReg8:
        BAsmCode[CodeLen++] = 0x04 | Code | (adr_vals.values[0] << 3);
        break;
      case ModReg16:
        BAsmCode[CodeLen++] = 0x03 | (Code << 3) | (adr_vals.values[0] << 4);
        break;
      case ModIReg16:
        if (adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
          BAsmCode[CodeLen++] = 0x34 | Code;
      default:
        break;
    }
  }
}

static void DecodeCP(Word Code)
{
  adr_vals_t adr_vals;

  UNUSED(Code);

  if (!ChkArgCnt(1, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  /* 2 arguments -> check for A as dest, and compare is meant
     (syntax is either Z80 or Z80+808x implicitly due to previous check) */

  if (ArgCnt == 2) /* A as dest */
  {
    switch (DecodeAdr_Z80(&ArgStr[1], &adr_vals, MModReg8))
    {
      case ModNone:
        return;
      case ModReg8:
        if (adr_vals.values[0] == AccReg)
          break;
        /* else fall-through */
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        return;
    }
    OpSize = eSymbolSize8Bit;
  }

  /* 1 argument -> must be compared anyway in pure Z80 syntax mode, otherwise assume 808x call-on-positive */

  else
    OpSize = (CurrZ80Syntax == eSyntaxZ80) ? eSymbolSize8Bit : eSymbolSize16Bit;

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], &adr_vals, MModImm | ((CurrZ80Syntax & eSyntaxZ80) ? (MModIReg16 | MModReg8) : 0)))
  {
    case ModReg8:
      BAsmCode[CodeLen++] = 0xb8 | adr_vals.values[0];
      break;
    case ModIReg16:
      if (adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
        BAsmCode[CodeLen++] = 0xbe;
      break;
    case ModImm:
      if (eSymbolSize16Bit == OpSize) /* see comment above */
      {
        BAsmCode[CodeLen++] = 0xf4;
        append_adr_vals(&adr_vals);
      }
      else
      {
        BAsmCode[CodeLen++] = 0xfe;
        append_adr_vals(&adr_vals);
      }
      break;
    default:
      break;
  }
}

static void DecodeJP(Word Code)
{
  adr_vals_t adr_vals;
  Byte Condition;
  UNUSED(Code);

  if (!ChkArgCnt(1, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  /* if two arguments, first one is (Z80) condition */

  if (ArgCnt == 2)
  {
    if (!DecodeCondition(ArgStr[1].str.p_str, &Condition, True))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }
  }

  /* if one argument, it's unconditional JP in Z80 mode, or jump-if positive */

  else
    Condition = (CurrZ80Syntax == eSyntaxZ80) ? 0xff : 6 << 3;

  OpSize = eSymbolSize16Bit;
  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], &adr_vals, MModImm | (((ArgCnt == 1) && (CurrZ80Syntax & eSyntaxZ80)) ? MModIReg16 : 0)))
  {
    case ModIReg16:
      if (adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
        BAsmCode[CodeLen++] = 0xe9;
      break;
    case ModImm:
      BAsmCode[CodeLen++] = (0xff == Condition) ? 0xc3 : 0xc2 + Condition;
      append_adr_vals(&adr_vals);
    default:
      break;
  }
}

static void DecodeJ(Word Code)
{
  adr_vals_t adr_vals;
  Byte Condition;
  UNUSED(Code);

  if (!ChkZ80Syntax(eSyntaxZ80))
    return;

  switch (ArgCnt)
  {
    /* if two arguments, first one is (Z80) condition */

    case 2:
      if (!DecodeCondition(ArgStr[1].str.p_str, &Condition, True))
      {
        WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        return;
      }
      break;

    /* if one argument, it's unconditional JP */

    case 1:
      Condition = 0x01;
      break;

    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  OpSize = eSymbolSize16Bit;
  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], &adr_vals, MModImm | ((ArgCnt == 1) ? MModIReg16 : 0)))
  {
    case ModIReg16:
      if (adr_vals.values[0] != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
        BAsmCode[CodeLen++] = 0xe9;
      break;
    case ModImm:
      BAsmCode[CodeLen++] = 0xc2 + Condition;
      append_adr_vals(&adr_vals);
    default:
      break;
  }
}

static void DecodeCALL(Word Code)
{
  adr_vals_t adr_vals;
  Byte Condition;
  UNUSED(Code);

  if (!ChkArgCnt(1, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  if (ArgCnt == 2) /* Z80-style with condition */
  {
    if (!DecodeCondition(ArgStr[1].str.p_str, &Condition, False))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }
  }
  else
    Condition = 0xff;

  OpSize = eSymbolSize16Bit;
  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], &adr_vals, MModImm))
  {
    case ModImm:
      BAsmCode[CodeLen++] = (1 == ArgCnt) ? 0xcd : 0xc4 | Condition;
      append_adr_vals(&adr_vals);
    default:
      break;
  }
}

static void DecodeRET(Word Code)
{
  Byte Condition;
  UNUSED(Code);

  if (!ChkArgCnt(0, (CurrZ80Syntax & eSyntaxZ80) ? 1 : 0))
    return;

  if (ArgCnt == 1) /* Z80-style with condition */
  {
    if (!DecodeCondition(ArgStr[1].str.p_str, &Condition, False)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
    else
      BAsmCode[CodeLen++] = 0xc0 | Condition;
  }
  else
   BAsmCode[CodeLen++] = 0xc9;
}

static void DecodeINOUT(Word Code)
{
  tEvalResult EvalResult;

  if (!ChkArgCnt((CurrZ80Syntax & eSyntax808x) ? 1 : 2, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  if (ArgCnt == 2) /* Z80-style with A */
  {
    adr_vals_t adr_vals;
    tStrComp *p_reg_arg = &ArgStr[Code == 0xdb ? 1 : 2];

    switch (DecodeAdr_Z80(p_reg_arg, &adr_vals, MModReg8))
    {
      case ModReg8:
        if (adr_vals.values[0] != AccReg)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, p_reg_arg);
          return;
        }
        break;
      default:
        return;
    }
  }
  BAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[((Code == 0xdb) && (ArgCnt == 2)) ? 2 : 1], UInt8, &EvalResult);
  if (EvalResult.OK)
  {
    ChkSpace(SegIO, EvalResult.AddrSpaceMask);
    set_guessed(EvalResult.Flags, 1, 1, 0xff);
    BAsmCode[0] = Lo(Code);
    CodeLen = 2;
  }
}

static void DecodeSRA(Word Code)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU8085U));
  else if (!ChkZ80Syntax(eSyntaxZ80));
  else if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg) || (Reg != HLReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
   BAsmCode[CodeLen++] = Lo(Code);
}

static void DecodeRLC(Word Code)
{
  if (!ChkArgCnt((CurrZ80Syntax & eSyntax808x) ? 0 : 1, (CurrZ80Syntax & eSyntaxZ80) ? 1 : 0))
    return;

  switch (ArgCnt)
  {
    case 0:
      BAsmCode[CodeLen++] = Code;
      break;
    case 1:
    {
      Byte Reg;

      if (!DecodeReg16(ArgStr[1].str.p_str, CurrZ80Syntax, &Reg) || (Reg != DEReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
        BAsmCode[CodeLen++] = 0x18;
      break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALLN(Word code)
 * \brief  handle CALLN instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCALLN(Word code)
{
  if (ChkArgCnt(1, 1) && ChkExactCPU(CPUV30EMU))
  {
    Boolean ok;
    tSymbolFlags flags;

    BAsmCode[2] = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt8, &ok, &flags);
    if (ok)
    {
      BAsmCode[0] = Hi(code);
      BAsmCode[1] = Lo(code);
      set_guessed(flags, 2, 1, 0xff);
      CodeLen = 3;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRETEM(Word code)
 * \brief  handle RETEM instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeRETEM(Word code)
{
  if (ChkArgCnt(0, 0) && ChkExactCPU(CPUV30EMU))
  {
    BAsmCode[0] = Hi(code);
    BAsmCode[1] = Lo(code);
    CodeLen = 2;
  }
}

static void DecodePORT(Word Index)
{
  UNUSED(Index);

  code_equate_type(SegIO, UInt8);
}

/*--------------------------------------------------------------------------------------------------------*/

static void AddFixed(const char *NName, CPUVar NMinCPU, Byte NCode, Word SyntaxMask)
{
  if (MomCPU >= NMinCPU)
    AddInstTable(InstTable, NName, (SyntaxMask << 8) | NCode, DecodeFixed);
}

static void AddOp16(const char *NName, CPUVar NMinCPU, Byte NCode, Word SyntaxMask)
{
  if (MomCPU >= NMinCPU)
    AddInstTable(InstTable, NName, (SyntaxMask << 8) | NCode, DecodeOp16);
}

static void AddOp8(const char *NName, CPUVar NMinCPU, Word NCode, Word SyntaxMask)
{
  if (MomCPU >= NMinCPU)
    AddInstTable(InstTable, NName, (SyntaxMask << 8) | NCode, DecodeOp8);
}

static void AddALU(const char *NName, Byte NCode, Word SyntaxMask)
{
  AddInstTable(InstTable, NName, (SyntaxMask << 8) | NCode, DecodeALU);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  add_null_pseudo(InstTable);

  AddInstTable(InstTable, "MOV" , 0, DecodeMOV);
  AddInstTable(InstTable, "MVI" , 0, DecodeMVI);
  AddInstTable(InstTable, "LXI" , 0, DecodeLXI);
  AddInstTable(InstTable, "STAX", 0, DecodeLDAX_STAX);
  AddInstTable(InstTable, "LDAX", 1, DecodeLDAX_STAX);
  AddInstTable(InstTable, "SHLX", 0, DecodeLHLX_SHLX);
  AddInstTable(InstTable, "LHLX", 1, DecodeLHLX_SHLX);
  AddInstTable(InstTable, "PUSH", 4, DecodePUSH_POP);
  AddInstTable(InstTable, "POP" , 0, DecodePUSH_POP);
  AddInstTable(InstTable, "RST" , 0, DecodeRST);
  AddInstTable(InstTable, "INR" , 0, DecodeINR_DCR);
  AddInstTable(InstTable, "DCR" , 1, DecodeINR_DCR);
  AddInstTable(InstTable, "INX" , 0, DecodeINX_DCX);
  AddInstTable(InstTable, "DCX" , 8, DecodeINX_DCX);
  AddInstTable(InstTable, "DAD" , 0, DecodeDAD);
  AddInstTable(InstTable, "DSUB", 0, DecodeDSUB);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);

  AddFixed("XCHG", CPU8080 , 0xeb, eSyntax808x);
  AddFixed("XTHL", CPU8080 , 0xe3, eSyntax808x);
  AddFixed("SPHL", CPU8080 , 0xf9, eSyntax808x);
  AddFixed("PCHL", CPU8080 , 0xe9, eSyntax808x);
  AddFixed("RC"  , CPU8080 , 0xd8, eSyntax808x);
  AddFixed("RNC" , CPU8080 , 0xd0, eSyntax808x);
  AddFixed("RZ"  , CPU8080 , 0xc8, eSyntax808x);
  AddFixed("RNZ" , CPU8080 , 0xc0, eSyntax808x);
  AddFixed("RP"  , CPU8080 , 0xf0, eSyntax808x);
  AddFixed("RM"  , CPU8080 , 0xf8, eSyntax808x);
  AddFixed("RPE" , CPU8080 , 0xe8, eSyntax808x);
  AddFixed("RPO" , CPU8080 , 0xe0, eSyntax808x);
  /* RLC needs special handling */
  AddFixed("RLCA", CPU8080 , 0x07, eSyntaxZ80);
  AddFixed("RRC" , CPU8080 , 0x0f, eSyntax808x);
  AddFixed("RRCA", CPU8080 , 0x0f, eSyntaxZ80);
  AddFixed("RAL" , CPU8080 , 0x17, eSyntax808x);
  AddFixed("RLA" , CPU8080 , 0x17, eSyntaxZ80);
  AddFixed("RAR" , CPU8080 , 0x1f, eSyntax808x);
  AddFixed("RRA" , CPU8080 , 0x1f, eSyntaxZ80);
  AddFixed("CMA" , CPU8080 , 0x2f, eSyntax808x);
  AddFixed("CPL" , CPU8080 , 0x2f, eSyntaxZ80);
  AddFixed("STC" , CPU8080 , 0x37, eSyntax808x);
  AddFixed("SCF" , CPU8080 , 0x37, eSyntaxZ80);
  AddFixed("CMC" , CPU8080 , 0x3f, eSyntax808x);
  AddFixed("CCF" , CPU8080 , 0x3f, eSyntaxZ80);
  AddFixed("DAA" , CPU8080 , 0x27, eSyntax808x | eSyntaxZ80);
  AddFixed("EI"  , CPU8080 , 0xfb, eSyntax808x | eSyntaxZ80);
  AddFixed("DI"  , CPU8080 , 0xf3, eSyntax808x | eSyntaxZ80);
  AddFixed("NOP" , CPU8080 , 0x00, eSyntax808x | eSyntaxZ80);
  AddFixed("HLT" , CPU8080 , 0x76, eSyntax808x);
  AddFixed("HALT", CPU8080 , 0x76, eSyntaxZ80);
  AddFixed("RIM" , CPU8085 , 0x20, eSyntax808x);
  AddFixed("SIM" , CPU8085 , 0x30, eSyntax808x);
  AddFixed("ARHL", CPU8085U, 0x10, eSyntax808x);
  AddFixed("RDEL", CPU8085U, 0x18, eSyntax808x | eSyntaxZ80); /* TODO: find LD equivalent */
  AddFixed("RSTV", CPU8085U, 0xcb, eSyntax808x);


  AddOp16("STA" , CPU8080 , 0x32, eSyntax808x);
  AddOp16("LDA" , CPU8080 , 0x3a, eSyntax808x);
  AddOp16("SHLD", CPU8080 , 0x22, eSyntax808x);
  AddOp16("LHLD", CPU8080 , 0x2a, eSyntax808x);
  AddOp16("JMP" , CPU8080 , 0xc3, eSyntax808x);
  AddOp16("JC"  , CPU8080 , 0xda, eSyntax808x);
  AddOp16("JNC" , CPU8080 , 0xd2, eSyntax808x);
  AddOp16("JZ"  , CPU8080 , 0xca, eSyntax808x);
  AddOp16("JNZ" , CPU8080 , 0xc2, eSyntax808x);
  /* JP needs special handling */
  AddOp16("JM"  , CPU8080 , 0xfa, eSyntax808x);
  AddOp16("JPE" , CPU8080 , 0xea, eSyntax808x);
  AddOp16("JPO" , CPU8080 , 0xe2, eSyntax808x);
  /* CALL needs special handling */
  AddOp16("CC"  , CPU8080 , 0xdc, eSyntax808x);
  AddOp16("CNC" , CPU8080 , 0xd4, eSyntax808x);
  AddOp16("CZ"  , CPU8080 , 0xcc, eSyntax808x);
  AddOp16("CNZ" , CPU8080 , 0xc4, eSyntax808x);
  /* CP needs special handling */
  AddOp16("CM"  , CPU8080 , 0xfc, eSyntax808x);
  AddOp16("CPE" , CPU8080 , 0xec, eSyntax808x);
  AddOp16("CPO" , CPU8080 , 0xe4, eSyntax808x);
  AddOp16("JNX5", CPU8085U, 0xdd, eSyntax808x);
  AddOp16("JX5" , CPU8085U, 0xfd, eSyntax808x);

  AddOp8("ADI" , CPU8080 , 0xc6, eSyntax808x);
  AddOp8("ACI" , CPU8080 , 0xce, eSyntax808x);
  AddOp8("SUI" , CPU8080 , 0xd6, eSyntax808x);
  AddOp8("SBI" , CPU8080 , 0xde, eSyntax808x);
  AddOp8("ANI" , CPU8080 , 0xe6, eSyntax808x);
  AddOp8("XRI" , CPU8080 , 0xee, eSyntax808x);
  AddOp8("ORI" , CPU8080 , 0xf6, eSyntax808x);
  AddOp8("CPI" , CPU8080 , 0xfe, eSyntax808x);
  AddOp8("LDHI", CPU8085U, 0x28, eSyntax808x | eSyntaxZ80); /* TODO: find LD equivalent */
  AddOp8("LDSI", CPU8085U, 0x38, eSyntax808x | eSyntaxZ80);

  AddALU("SBB" , 0x98, eSyntax808x);
  AddALU("ANA" , 0xa0, eSyntax808x);
  AddALU("XRA" , 0xa8, eSyntax808x);
  AddALU("ORA" , 0xb0, eSyntax808x);
  AddALU("CMP" , 0xb8, eSyntax808x);

  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "EX", 0, DecodeEX);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "ADC", 0, DecodeADC);
  AddInstTable(InstTable, "SUB", 0, DecodeSUB);
  AddInstTable(InstTable, "SBC", 3, DecodeALU8_Z80);
  AddInstTable(InstTable, "INC", 0, DecodeINCDEC);
  AddInstTable(InstTable, "DEC", 1, DecodeINCDEC);
  AddInstTable(InstTable, "AND", 4, DecodeALU8_Z80);
  AddInstTable(InstTable, "XOR", 5, DecodeALU8_Z80);
  AddInstTable(InstTable, "OR" , 6, DecodeALU8_Z80);
  AddInstTable(InstTable, "CP" , 0, DecodeCP);
  AddInstTable(InstTable, "JP" , 0, DecodeJP);
  AddInstTable(InstTable, "J" , 0, DecodeJ);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "RET", 0, DecodeRET);
  AddInstTable(InstTable, "IN", 0xdb, DecodeINOUT);
  AddInstTable(InstTable, "OUT", 0xd3, DecodeINOUT);
  AddInstTable(InstTable, "SRA", 0x10, DecodeSRA);
  AddInstTable(InstTable, "RLC", 0x07, DecodeRLC);

  AddInstTable(InstTable, "CALLN", 0xeded, DecodeCALLN);
  AddInstTable(InstTable, "RETEM", 0xedfd, DecodeRETEM);

  AddZ80Syntax(InstTable);
  AddIntelPseudo(InstTable, eIntPseudoFlag_LittleEndian);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------------------------------------*/

static void MakeCode_85(void)
{
  OpSize = eSymbolSize8Bit;

  /* suchen */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_85(void)
{
  return Memo("PORT");
}

static void SwitchFrom_85(void)
{
  DeinitFields();
}

static void SwitchTo_85(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("8080/8085");
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegIO);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;
  Grans[SegIO  ] = 1; ListGrans[SegIO  ] = 1; SegInits[SegIO  ] = 0;
  SegLimits[SegIO  ] = 0xff;

  MakeCode = MakeCode_85;
  IsDef = IsDef_85;
  SwitchFrom = SwitchFrom_85;
  InitFields();
}

void code85_init(void)
{
  CPU8080 = AddCPU("8080", SwitchTo_85);
  CPUV30EMU = AddCPU("V30EMU", SwitchTo_85);
  CPU8085 = AddCPU("8085", SwitchTo_85);
  CPU8085U = AddCPU("8085UNDOC", SwitchTo_85);
}
