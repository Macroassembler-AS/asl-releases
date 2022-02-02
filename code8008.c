/* code8008.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Intel 8008                                                  */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "headids.h"
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "errmsg.h"

#include "code8008.h"

/*---------------------------------------------------------------------------*/
/* Types */

typedef enum
{
  eModNone = 0,
  eModReg8 = 1,
  eModIHL = 2,
  eModImm = 3,
  eModReg16 = 4
} tAdrMode;

#define AccReg 0

#define MModReg8 (1 << eModReg8)
#define MModReg16 (1 << eModReg16)
#define MModImm (1 << eModImm)
#define MModIHL (1 << eModIHL)
#define MModReg16 (1 << eModReg16)

typedef struct
{
  tAdrMode Mode;
  Byte Val;
} tAdrVals;

/*---------------------------------------------------------------------------*/
/* Variablen */

static CPUVar CPU8008, CPU8008New;

static const char RegNames[] = "ABCDEHLM";

/*---------------------------------------------------------------------------*/
/* Parser */

static Boolean DecodeReg(char *Asc, tZ80Syntax Syntax, Byte *pErg)
{
  const char *p;

  if (strlen(Asc) != 1)
    return False;

  p = strchr(RegNames, as_toupper(*Asc));
  if (!p)
    return False;

  *pErg = p - RegNames;
  if ((!(Syntax & eSyntax808x)) && (*pErg == 7))
    return False;
  return True;
}

static Boolean DecodeReg16(char *Asc, Byte *pErg)
{
  if (strlen(Asc) != 2)
    return False;

  for (*pErg = 1; *pErg < 7; *pErg += 2)
    if ((as_toupper(Asc[0]) == RegNames[*pErg])
     && (as_toupper(Asc[1]) == RegNames[*pErg + 1]))
    {
      *pErg /= 2;
      return True;
    }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCondition_Z80(const char *pAsc, Byte *pResult)
 * \brief  decode Z80-style conditions for JP CALL RET
 * \param  pAsc source argument
 * \param  dest buffer
 * \return True if success
 * ------------------------------------------------------------------------ */

static const char Conditions[][4] =
{
  "NC", "NZ", "P", "PO", "C", "Z", "M", "PE"
};

static Boolean DecodeCondition_Z80(const char *pAsc, Byte *pResult)
{
  for (*pResult = 0; *pResult < sizeof(Conditions) / sizeof(*Conditions); (*pResult)++)
    if (!as_strcasecmp(pAsc, Conditions[*pResult]))
    {
      *pResult = *pResult << 3;
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr_Z80(const tStrComp *pArg, Word Mask, tAdrVals *pVals)
 * \brief  decode address expression in Z80 style syntax
 * \param  pArg source argument
 * \param  Mask bit mask of allowed addressig modes
 * \param  pVals dest buffer
 * \return resulting addressing mode
 * ------------------------------------------------------------------------ */

static void ResetAdrVals(tAdrVals *pVals)
{
  pVals->Mode = eModNone;
  pVals->Val = 0;
}

static tAdrMode DecodeAdr_Z80(const tStrComp *pArg, Word Mask, tAdrVals *pVals)
{
  Boolean OK;

  ResetAdrVals(pVals);

  if (DecodeReg(pArg->str.p_str, eSyntaxZ80, &pVals->Val))
  {
    pVals->Mode = eModReg8;
    goto AdrFound;
  }

  if (DecodeReg16(pArg->str.p_str, &pVals->Val))
  {
    pVals->Mode = eModReg16;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "(HL)"))
  {
    pVals->Mode = eModIHL;
    goto AdrFound;
  }

  pVals->Val = EvalStrIntExpression(pArg, Int8, &OK);
  if (OK)
    pVals->Mode = eModImm;

AdrFound:

  if ((pVals->Mode != eModNone) && (!(Mask & (1 << pVals->Mode))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    ResetAdrVals(pVals);
  }
  return pVals->Mode;
}

/*!------------------------------------------------------------------------
 * \fn     CheckAcc_Z80(const tStrComp *pArg)
 * \brief  check whether (optional first) argument is accumulator
 * \param  pArg source argument
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean CheckAcc_Z80(const tStrComp *pArg)
{
  tAdrVals AdrVals;

  switch (DecodeAdr_Z80(pArg, MModReg8, &AdrVals))
  {
    case eModReg8:
      if (AdrVals.Val != AccReg)
      {
        WrStrErrorPos(ErrNum_InvReg, pArg);
        return False;
      }
      break;
    default:
      return False;
  }
  return True;
}

/*---------------------------------------------------------------------------*/
/* Instruction Decoders */

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  handle instructions without argument
 * \param  Word instruction code & syntax flag
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0)
   && ChkZ80Syntax((tZ80Syntax)Hi(Code)))
  {
    BAsmCode[0] = Lo(Code);
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  handle instructions with one (8 bit) immediate argument
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeImm(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkZ80Syntax((tZ80Syntax)Hi(Code)))
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = Lo(Code);
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJmp(Word Index)
 * \brief  decode jump/call instructions taking one address argument
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeJmpCore(Byte Code)
{
  tEvalResult EvalResult;
  Word AdrWord;

  AdrWord = EvalStrIntExpressionWithResult(&ArgStr[ArgCnt], UInt14, &EvalResult);
  if (EvalResult.OK)
  {
    BAsmCode[0] = Code;
    BAsmCode[1] = Lo(AdrWord);
    BAsmCode[2] = Hi(AdrWord) & 0x3f;
    CodeLen = 3;
    ChkSpace(SegCode, EvalResult.AddrSpaceMask);
  }
}

static void DecodeJmp(Word Index)
{
  if (ChkArgCnt(1, 1)
   && ChkZ80Syntax(eSyntax808x))
    DecodeJmpCore(Lo(Index));
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJP(Word Code)
 * \brief  decode JP instruction
 * ------------------------------------------------------------------------ */

static void DecodeJP(Word Code)
{
  UNUSED(Code);

  /* one argument, new 8008 insn set & not exclusively Z80: -> jump on positive or parity even */

  if ((CurrZ80Syntax != eSyntaxZ80) && (ArgCnt == 1))
  {
    DecodeJmp((MomCPU == CPU8008New) ? 0x50 : 0x78);
    return;
  }

  /* for all other stuff, Z80 syntax must be enabled */

  if (!ChkArgCnt(1, 2)
   || !ChkZ80Syntax(eSyntaxZ80))
    return;

  if (ArgCnt == 2)
  {
    Byte Cond;

    if (DecodeCondition_Z80(ArgStr[1].str.p_str, &Cond))
      DecodeJmpCore(0x40 | Cond);
  }
  else
    DecodeJmpCore(0x44);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALL(Word Code)
 * \brief  decode CALL instruction
 * ------------------------------------------------------------------------ */

static void DecodeCALL(Word Code)
{
  UNUSED(Code);

  /* single-arg CALL is only NOT allowed on old 8008 in pure  808x mode : */

  if ((CurrZ80Syntax != eSyntaxZ80) && (ArgCnt == 1) && (MomCPU == CPU8008New))
  {
    DecodeJmp(0x46);
    return;
  }

  if (!ChkArgCnt(1, 2)
   || !ChkZ80Syntax(eSyntaxZ80))
    return;

  if (ArgCnt == 2)
  {
    Byte Cond;

    if (DecodeCondition_Z80(ArgStr[1].str.p_str, &Cond))
      DecodeJmpCore(0x42 | Cond);
  }
  else
    DecodeJmpCore(0x46);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRET(Word Code)
 * \brief  handle RET instruction
 * ------------------------------------------------------------------------ */

static void DecodeRET(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, (CurrZ80Syntax != eSyntax808x) ? 1 : 0))
  {
    if (1 == ArgCnt)
    {
      Byte Cond;

      if (DecodeCondition_Z80(ArgStr[1].str.p_str, &Cond))
        BAsmCode[CodeLen++] = 0x03 | Cond;
    }
    else
      BAsmCode[CodeLen++] = 0x07;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRST(Word Index)
 * \brief  handle RST instruction
 * ------------------------------------------------------------------------ */

static void DecodeRST(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    Word AdrWord;
    tEvalResult EvalResult;

    AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt14, &EvalResult);
    if (mFirstPassUnknown(EvalResult.Flags)) AdrWord &= 0x38;
    if (EvalResult.OK)
    {
      if (ChkRange(AdrWord, 0, 0x38))
      {
        if (AdrWord < 8)
        {
          BAsmCode[0] = 0x05 | (AdrWord << 3);
          CodeLen = 1;
          ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        }
        else if ((AdrWord & 7) != 0) WrError(ErrNum_NotAligned);
        else
        {
          BAsmCode[0] = AdrWord + 0x05;
          CodeLen = 1;
          ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        }
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINP_OUT(Word Index)
 * \brief  Handle IN(P)/OUT Instructions
 * \param  IsIN True if IN
 * ------------------------------------------------------------------------ */

static void DecodeINP_OUT(Word IsIN)
{
  tEvalResult EvalResult;
  Byte Addr, AddrArgIdx;

  if (Memo("INP") && !ChkZ80Syntax(eSyntax808x))
    return;
  if (Memo("IN") && (MomCPU == CPU8008) && !ChkZ80Syntax(eSyntaxZ80))
    return;

  if (!ChkArgCnt((CurrZ80Syntax & eSyntax808x) ? 1 : 2, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  if ((ArgCnt == 2) /* Z80-style with A */
   && !CheckAcc_Z80(&ArgStr[IsIN ? 1 : 2]))
    return;

  AddrArgIdx = ((ArgCnt == 2) && IsIN) ? 2 : 1;
  if (IsIN)
    Addr = EvalStrIntExpressionWithResult(&ArgStr[AddrArgIdx], UInt3, &EvalResult);
  else
  {
    Addr = EvalStrIntExpressionWithResult(&ArgStr[AddrArgIdx], UInt5, &EvalResult);
    if (mFirstPassUnknown(EvalResult.Flags))
      Addr |= 0x08;
    if ((EvalResult.OK) && (Addr < 8))
    {
      WrStrErrorPos(ErrNum_UnderRange, &ArgStr[AddrArgIdx]);
      EvalResult.OK = False;
    }
  }

  if (EvalResult.OK)
  {
    BAsmCode[0] = 0x41 | (Addr << 1);
    CodeLen = 1;
    ChkSpace(SegIO, EvalResult.AddrSpaceMask);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOV(Word Index)
 * \brief  handle MOV instruction
 * ------------------------------------------------------------------------ */

static void DecodeMOV(Word Index)
{
  Byte SReg, DReg;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg(ArgStr[1].str.p_str, eSyntax808x, &DReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (!DecodeReg(ArgStr[2].str.p_str, eSyntax808x, &SReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if ((DReg == 7) && (SReg == 7)) WrError(ErrNum_InvRegPair); /* MOV M,M not allowed - asame opcode as HLT */
  else
  {
    BAsmCode[0] = 0xc0 | (DReg << 3) | SReg;
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMVI(Word Index)
 * \brief  handle MVI instruction
 * ------------------------------------------------------------------------ */

static void DecodeMVI(Word Index)
{
  Byte DReg;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg(ArgStr[1].str.p_str, eSyntax808x, &DReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x06 | (DReg << 3);
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLXI(Word Index)
 * \brief  handle (pseudo) LXI instruction
 * ------------------------------------------------------------------------ */

static void BuildLXI(Byte DReg, Word Val)
{
  BAsmCode[2] = 0x06 | ((DReg + 1) << 3);
  BAsmCode[3] = Lo(Val);
  BAsmCode[0] = 0x06 | (DReg << 3);
  BAsmCode[1] = Hi(Val);
  CodeLen = 4;
}

static void DecodeLXI(Word Index)
{
  Byte DReg;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg(ArgStr[1].str.p_str, eSyntax808x, &DReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if ((DReg != 1) && (DReg != 3) && (DReg != 5)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    Boolean OK;
    Word Val;

    Val = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
      BuildLXI(DReg, Val);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLD(Word Index)
 * \brief  handle LD instruction
 * ------------------------------------------------------------------------ */

static void DecodeLD(Word Index)
{
  UNUSED(Index);
  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntaxZ80))
  {
    tAdrVals DestAdrVals, SrcAdrVals;

    switch (DecodeAdr_Z80(&ArgStr[1], MModReg8 | MModReg16 | MModIHL, &DestAdrVals))
    {
      case eModReg8:
        switch (DecodeAdr_Z80(&ArgStr[2], MModReg8 | MModReg16 | MModIHL | MModImm, &SrcAdrVals))
        {
          case eModReg8:
            BAsmCode[CodeLen++] = 0xc0 | (DestAdrVals.Val << 3) | SrcAdrVals.Val;
            break;
          case eModIHL:
            BAsmCode[CodeLen++] = 0xc7 | (DestAdrVals.Val << 3);
            break;
          case eModImm:
            BAsmCode[CodeLen++] = 0x06 | (DestAdrVals.Val << 3);
            BAsmCode[CodeLen++] = SrcAdrVals.Val;
            break;
          default:
            break;
        }
        break;
      case eModReg16:
      {
        Boolean OK;
        Word Arg = EvalStrIntExpression(&ArgStr[2], Int16, &OK);

        if (OK)
          BuildLXI((DestAdrVals.Val << 1) + 1, Arg);
        break;
      }
      case eModIHL:
        switch (DecodeAdr_Z80(&ArgStr[2], MModReg8 | MModImm, &SrcAdrVals))
        {
          case eModReg8:
            BAsmCode[CodeLen++] = 0xf8 | SrcAdrVals.Val;
            break;
          case eModImm:
            BAsmCode[CodeLen++] = 0x3e;
            BAsmCode[CodeLen++] = SrcAdrVals.Val;
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

/*!------------------------------------------------------------------------
 * \fn     DecodeADD(Word Code)
 * \brief  handle ADD instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeADD(Word Code)
{
  int MinArgCnt = (CurrZ80Syntax & eSyntax808x) ? ((MomCPU == CPU8008New) ? 1 : 0) : 2,
      MaxArgCnt = (CurrZ80Syntax & eSyntaxZ80) ? 2 : ((MomCPU == CPU8008New) ? 1 : 0);

  if (!ChkArgCnt(MinArgCnt, MaxArgCnt))
    return;

  switch (ArgCnt)
  {
    case 0: /* 8008 old style - src is implicit D, dst is implicitly ACC */
      BAsmCode[CodeLen++] = Code | 0x03;
      break;
    case 1: /* 8008 (new) style - 8 bit register src only, dst is implicitly ACC */
      if (MomCPU == CPU8008) (void)ChkArgCnt(0, 0);
      else
      {
        Byte Reg;

        if (!DecodeReg(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
          else
        BAsmCode[CodeLen++] = Code | Reg;
      }
      break;
    case 2: /* Z80 style - dst must be A and is first arg */
    {
      tAdrVals SrcAdrVals;

      if (!CheckAcc_Z80(&ArgStr[1]))
        return;
      switch (DecodeAdr_Z80(&ArgStr[2], MModReg8 | MModIHL | MModImm, &SrcAdrVals))
      {
        case eModReg8:
          BAsmCode[CodeLen++] = Code | SrcAdrVals.Val;
          break;
        case eModIHL:
          BAsmCode[CodeLen++] = Code | 0x07;
          break;
        case eModImm:
          BAsmCode[CodeLen++] = 0x04;
          BAsmCode[CodeLen++] = SrcAdrVals.Val;
          break;
        default:
          break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeADC(Word Code)
 * \brief  handle ADC instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeADC(Word Code)
{
  int MinArgCnt = (CurrZ80Syntax & eSyntax808x) ? ((MomCPU == CPU8008New) ? 1 : 0) : 2,
      MaxArgCnt = (CurrZ80Syntax & eSyntaxZ80) ? 2 : ((MomCPU == CPU8008New) ? 1 : 0);

  if (!ChkArgCnt(MinArgCnt, MaxArgCnt))
    return;

  switch (ArgCnt)
  {
    case 0: /* 8008 add(!) old style - src is implicit C, dst is implicitly ACC */
      BAsmCode[CodeLen++] = 0x82;
      break;
    case 1: /* 8008 (new) style - 8 bit register src only, dst is implicitly ACC */
    {
      Byte Reg;

      if (!DecodeReg(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
        else
      BAsmCode[CodeLen++] = Code | Reg;
      break;
    }
    case 2: /* Z80 style - dst must be A and is first arg */
    {
      tAdrVals SrcAdrVals;

      if (!CheckAcc_Z80(&ArgStr[1]))
        return;
      switch (DecodeAdr_Z80(&ArgStr[2], MModReg8 | MModIHL | MModImm, &SrcAdrVals))
      {
        case eModReg8:
          BAsmCode[CodeLen++] = Code | SrcAdrVals.Val;
          break;
        case eModIHL:
          BAsmCode[CodeLen++] = Code | 0x07;
          break;
        case eModImm:
          BAsmCode[CodeLen++] = 0x0c;
          BAsmCode[CodeLen++] = SrcAdrVals.Val;
          break;
        default:
          break;
      }
      break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSUB(Word Code)
 * \brief  handle SUB instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeSUB(Word Code)
{
  Byte Reg;
  int MinArgCnt, MaxArgCnt;
  tAdrVals SrcAdrVals;

  if (CurrZ80Syntax & eSyntaxZ80)
    MaxArgCnt = 2;
  else
    MaxArgCnt = (MomCPU == CPU8008New) ? 1 : 0;

  /* dest operand is always A and also optional for Z80 mode, so min arg cnt is
     also 1 for pure Z80 mode: */

  if ((CurrZ80Syntax & eSyntax808x) && (MomCPU == CPU8008))
    MinArgCnt = 0;
  else
    MinArgCnt = 1;
  if (!ChkArgCnt(MinArgCnt, MaxArgCnt))
    return;

  /* For Z80, optionally allow A as dest */

  if ((ArgCnt == 2) && !CheckAcc_Z80(&ArgStr[1]))
    return;

  /* no arg (SUB -> B register) */

  if (!ArgCnt)
  {
    BAsmCode[CodeLen++] = Code | 0x01;
    return;
  }

  /* 8 bit register as source
     808x style incl. M, Z80 style excl. (HL) */

  if (DecodeReg(ArgStr[ArgCnt].str.p_str, CurrZ80Syntax, &Reg)) /* 808x style incl. M, Z80 style excl. (HL) */
  {
    BAsmCode[CodeLen++] = Code | Reg;
    return;
  }

  /* rest is Z80 style ( (HL) or immediate) */

  if (!(CurrZ80Syntax & eSyntaxZ80))
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], MModImm | MModIHL, &SrcAdrVals))
  {
    case eModIHL:
      BAsmCode[CodeLen++] = Code | 0x07;
      break;
    case eModImm:
      BAsmCode[CodeLen++] = 0x14;
      BAsmCode[CodeLen++] = SrcAdrVals.Val;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSBC(Word Code)
 * \brief  handle SBC instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeSBC(Word Code)
{
  tAdrVals SrcAdrVals;

  /* special case of 8008 SBC: */

  if ((CurrZ80Syntax & eSyntax808x) && (ArgCnt == 0) && (MomCPU == CPU8008))
  {
    BAsmCode[CodeLen++] = Code | 0x02;
    return;
  }

  /* everything else is Z80-specific: */

  if (!ChkArgCnt(1, 2) || !ChkZ80Syntax(eSyntaxZ80))
    return;

  /* dest operand is always A and also optional, since 8008 can only SUB from A: */

  if ((ArgCnt == 2) && !CheckAcc_Z80(&ArgStr[1]))
    return;

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], MModImm | MModReg8 | MModIHL, &SrcAdrVals))
  {
    case eModReg8:
      BAsmCode[CodeLen++] = Code | SrcAdrVals.Val;
      break;
    case eModIHL:
      BAsmCode[CodeLen++] = Code | 0x07;
      break;
    case eModImm:
      BAsmCode[CodeLen++] = 0x1c;
      BAsmCode[CodeLen++] = SrcAdrVals.Val;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeALU8_Z80(Word Code)
 * \brief  arithmetic instructions only available in Z80 style
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeALU8_Z80(Word Code)
{
  tAdrVals SrcAdrVals;

  if (!ChkZ80Syntax(eSyntaxZ80)
   || !ChkArgCnt(1, 2))
    return;

  if ((ArgCnt == 2) /* A as dest */
   && !CheckAcc_Z80(&ArgStr[1]))
    return;

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], MModImm | MModIHL | MModReg8, &SrcAdrVals))
  {
    case eModReg8:
      BAsmCode[CodeLen++] = Lo(Code) | SrcAdrVals.Val;
      break;
    case eModIHL:
      BAsmCode[CodeLen++] = Lo(Code) | 0x07;
      break;
    case eModImm:
      BAsmCode[CodeLen++] = Hi(Code);
      BAsmCode[CodeLen++] = SrcAdrVals.Val;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCP(Word Code)
 * \brief  handle CP instruction
 * ------------------------------------------------------------------------ */

static void DecodeCP(Word Code)
{
  tAdrVals SrcAdrVals;
  Boolean IsCall;

  UNUSED(Code);

  if (!ChkArgCnt(1, (CurrZ80Syntax & eSyntaxZ80) ? 2 : 1))
    return;

  /* 2 arguments -> check for A as dest, and compare is meant
     (syntax is either Z80 or Z80+808x implicitly due to previous check) */

  if (ArgCnt == 2) /* A as dest */
  {
    if (!CheckAcc_Z80(&ArgStr[1]))
      return;
    IsCall = False;
  }

  /* 1 argument -> must be compare anyway in pure Z80 syntax mode, otherwise assume 808x call-on-positive */

  else
    IsCall = CurrZ80Syntax != eSyntaxZ80;

  if (IsCall)
  {
    DecodeJmpCore((MomCPU == CPU8008New) ? 0x52 : 0x7a);
    return;
  }

  switch (DecodeAdr_Z80(&ArgStr[ArgCnt], MModImm | MModIHL | MModReg8, &SrcAdrVals))
  {
    case eModReg8:
      BAsmCode[CodeLen++] = 0xb8 | SrcAdrVals.Val;
      break;
    case eModIHL:
      BAsmCode[CodeLen++] = 0xbf;
      break;
    case eModImm:
      BAsmCode[CodeLen++] = 0x3c;
      BAsmCode[CodeLen++] = SrcAdrVals.Val;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINCDEC(Word Code)
 * \brief  handle (Z80 style) INC/DEC
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeINCDEC(Word Code)
{
  /* INC aka INC C on old 8008: */

  if ((MomCPU == CPU8008) && (CurrZ80Syntax & eSyntax808x) && (!ArgCnt) && !Code)
  {
    BAsmCode[CodeLen++] = Code | (0x02 << 3);
    return;
  }

  if (ChkZ80Syntax(eSyntaxZ80)
   && ChkArgCnt(1, 1))
  {
    tAdrVals AdrVals;

    switch (DecodeAdr_Z80(&ArgStr[1], MModReg8, &AdrVals))
    {
      case eModReg8:
        if (AdrVals.Val == AccReg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        else
          BAsmCode[CodeLen++] = Code | (AdrVals.Val << 3);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSingleReg(Word Index)
 * \brief  handle instructions taking a single register as argument
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeSingleReg(Word Index)
{
  Byte Reg, Opcode = Lo(Index), Shift = Hi(Index) & 7;
  Boolean NoAM = (Index & 0x8000) || False;

  if (!ChkArgCnt(1, 1));
  else if (!ChkZ80Syntax(eSyntax808x));
  else if (!DecodeReg(ArgStr[1].str.p_str, eSyntax808x, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (NoAM && ((Reg == 0) || (Reg == 7))) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Opcode | (Reg << Shift);
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePORT(Word Index)
 * \brief  handle PORT instruction
 * ------------------------------------------------------------------------ */

static void DecodePORT(Word Index)
{
  UNUSED(Index);

  CodeEquate(SegIO, 0, 0x7);
}

/*---------------------------------------------------------------------------*/
/* Codetabellenverwaltung */

static const char FlagNames[] = "CZSP";

static void AddFixed(const char *NName, Byte NCode, Word SyntaxMask)
{
  AddInstTable(InstTable, NName, (SyntaxMask << 8) | NCode, DecodeFixed);
}

static void AddFixeds(const char *NName, Byte NCode, int Shift, Byte RegMask)
{
  char Memo[10], *p;
  int Reg;

  strcpy(Memo, NName); p = strchr(Memo, '*');
  for (Reg = 0; Reg < 8; Reg++)
    if ((1 << Reg) & RegMask)
    {
      *p = RegNames[Reg];
      AddFixed(Memo, NCode + (Reg << Shift), eSyntax808x);
    }
}

static void AddImm(const char *NName, Byte NCode, Word SyntaxMask)
{
  AddInstTable(InstTable, NName, (SyntaxMask << 8) | NCode, DecodeImm);
}

static void AddImms(const char *NName, Byte NCode, int Pos)
{
  char Memo[10], *p;
  int z;

  strcpy(Memo, NName); p = strchr(Memo, '*');
  for (z = 0; z < 8; z++)
  {
    *p = RegNames[z];
    AddImm(Memo, NCode + (z << Pos), eSyntax808x);
  }
}

static void AddJmp(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void AddJmps(const char *NName, Byte NCode, int Pos)
{
  char Memo[10], *p;
  int z;

  strcpy(Memo, NName); p = strchr(Memo, '*');
  for (z = 0; z < 4; z++)
  {
    *p = FlagNames[z];
    AddJmp(Memo, NCode + (z << Pos));
  }
}

static void InitFields(void)
{
  Boolean New = (MomCPU == CPU8008New);

  SetDynamicInstTable(InstTable = CreateInstTable(503));

  AddFixed("HLT" , 0x00, eSyntax808x);
  AddFixed("HALT", 0x00, eSyntaxZ80);
  AddFixed("NOP" , 0xc0, eSyntaxBoth); /* = MOV A,A */

  if (!New)
    AddInstTable(InstTable, "INP", True, DecodeINP_OUT);
  AddInstTable(InstTable, "IN" , True , DecodeINP_OUT);
  AddInstTable(InstTable, "OUT", False, DecodeINP_OUT);

  AddInstTable(InstTable, "JP", 0, DecodeJP);
  AddJmp ("JMP", 0x44);
  if (New)
  {
    AddJmp("JNC", 0x40);
    AddJmp("JNZ", 0x48);
    AddJmp("JPO", 0x58);
    AddJmp("JM" , 0x70);
    AddJmp("JPE", 0x78);
  }
  else
  {
    AddJmp("JS" , 0x70);
    AddJmps("JF*", 0x40, 3);
    AddJmps("JT*", 0x60, 3);
  }
  AddJmp("JC" , 0x60);
  AddJmp("JZ" , 0x68);

  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "CP", 0, DecodeCP);
  if (New)
  {
    AddJmp("CNC", 0x42);
    AddJmp("CNZ", 0x4a);
    AddJmp("CPO", 0x5a);
    AddJmp("CM" , 0x72);
    AddJmp("CPE", 0x7a);
  }
  else
  {
    AddJmp("CAL", 0x46);
    AddJmp("CS" , 0x72);
    AddJmps("CF*", 0x42, 3);
    AddJmps("CT*", 0x62, 3);
  }
  AddJmp ("CC" , 0x62);
  AddJmp ("CZ" , 0x6a);

  AddInstTable(InstTable, "RET", 0x07, DecodeRET);
  if (New)
  {
    AddFixed("RNC", 0x03, eSyntax808x);
    AddFixed("RNZ", 0x0b, eSyntax808x);
    AddFixed("RP" , 0x13, eSyntax808x);
    AddFixed("RPO", 0x1b, eSyntax808x);
    AddFixed("RM" , 0x33, eSyntax808x);
    AddFixed("RPE", 0x3b, eSyntax808x);
  }
  else
  {
    AddFixed("RFC", 0x03, eSyntax808x);
    AddFixed("RFZ", 0x0b, eSyntax808x);
    AddFixed("RFS", 0x13, eSyntax808x);
    AddFixed("RFP", 0x1b, eSyntax808x);
    AddFixed("RS" , 0x33, eSyntax808x);
    AddFixed("RP" , 0x3b, eSyntax808x);
    AddFixed("RTC", 0x23, eSyntax808x);
    AddFixed("RTZ", 0x2b, eSyntax808x);
    AddFixed("RTS", 0x33, eSyntax808x);
    AddFixed("RTP", 0x3b, eSyntax808x);
  }
  AddFixed("RC" , 0x23, eSyntax808x);
  AddFixed("RZ" , 0x2b, eSyntax808x);

  AddInstTable(InstTable, "RST", 0, DecodeRST); /* TODO: Z80 */

  if (New)
    AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  else
  {
    AddFixeds("L*A", 0xc0, 3, 0xff);
    AddFixeds("L*B", 0xc1, 3, 0xff);
    AddFixeds("L*C", 0xc2, 3, 0xff);
    AddFixeds("L*D", 0xc3, 3, 0xff);
    AddFixeds("L*E", 0xc4, 3, 0xff);
    AddFixeds("L*H", 0xc5, 3, 0xff);
    AddFixeds("L*L", 0xc6, 3, 0xff);
    AddFixeds("L*M", 0xc7, 3, 0x7f); /* forbid LMM - would be opcode for HLT */
  }

  if (New)
  {
    AddInstTable(InstTable, "MVI", 0, DecodeMVI);
    AddInstTable(InstTable, "LXI", 0, DecodeLXI);
  }
  else
    AddImms("L*I", 0x06, 3);

  AddInstTable(InstTable, "LD", 0, DecodeLD);

  AddInstTable(InstTable, "ADD", 0x0080, DecodeADD);
  AddInstTable(InstTable, "ADC", 0x0088, DecodeADC);
  AddInstTable(InstTable, "SUB", 0x0090, DecodeSUB);
  AddInstTable(InstTable, "SBC", 0x0098, DecodeSBC);
  AddInstTable(InstTable, "AND", 0x24a0, DecodeALU8_Z80);
  AddInstTable(InstTable, "XOR", 0x2ca8, DecodeALU8_Z80);
  AddInstTable(InstTable, "OR" , 0x34b0, DecodeALU8_Z80);
  AddInstTable(InstTable, "INC", 0x00, DecodeINCDEC);
  AddInstTable(InstTable, "DEC", 0x01, DecodeINCDEC);
  if (New)
  {
    AddInstTable(InstTable, "SBB", 0x0098, DecodeSingleReg);
    AddInstTable(InstTable, "ANA", 0x00a0, DecodeSingleReg);
    AddInstTable(InstTable, "XRA", 0x00a8, DecodeSingleReg);
    AddInstTable(InstTable, "ORA", 0x00b0, DecodeSingleReg);
    AddInstTable(InstTable, "CMP", 0x00b8, DecodeSingleReg);
    AddInstTable(InstTable, "INR", 0x8300, DecodeSingleReg);
    AddInstTable(InstTable, "DCR", 0x8301, DecodeSingleReg);
  }
  else
  {
    AddFixeds("AD*", 0x80, 0, 0xf3); /* ADC/ADD handled separately */
    AddFixeds("AC*", 0x88, 0, 0xff);
    AddFixeds("SU*", 0x90, 0, 0xfd); /* SUB handled separately */
    AddFixeds("SB*", 0x98, 0, 0xfb); /* SBC handled separately */
    AddFixeds("NR*", 0xa0, 0, 0xff);
    AddFixeds("ND*", 0xa0, 0, 0xff);
    AddFixeds("XR*", 0xa8, 0, 0xff);
    AddFixeds("OR*", 0xb0, 0, 0xff);
    AddFixeds("CP*", 0xb8, 0, 0xff);
    AddFixeds("IN*", 0x00, 3, 0x7a); /* no INA/INM, INC handled separately */
    AddFixeds("DC*", 0x01, 3, 0x7e); /* no DCA/DCM */
  }

  AddImm ("ADI", 0x04, eSyntax808x);
  AddImm ("ACI", 0x0c, eSyntax808x);
  AddImm ("SUI", 0x14, eSyntax808x);
  AddImm ("SBI", 0x1c, eSyntax808x);
  AddImm (New ? "ANI" : "NDI", 0x24, eSyntax808x);
  AddImm ("XRI", 0x2c, eSyntax808x);
  AddImm ("ORI", 0x34, eSyntax808x);
  AddImm ("CPI", 0x3c, eSyntax808x);

  AddFixed ("RLC" , 0x02, eSyntax808x);
  AddFixed ("RLCA", 0x02, eSyntaxZ80);
  AddFixed ("RRC" , 0x0a, eSyntax808x);
  AddFixed ("RRCA", 0x0a, eSyntaxZ80);
  AddFixed ("RAL" , 0x12, eSyntax808x);
  AddFixed ("RLA" , 0x12, eSyntaxZ80);
  AddFixed ("RAR" , 0x1a, eSyntax808x);
  AddFixed ("RRA" , 0x1a, eSyntaxZ80);

  AddInstTable(InstTable, "PORT", 0, DecodePORT);
  AddZ80Syntax(InstTable);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/
/* Callbacks */

static void MakeCode_8008(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  /* der Rest */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_8008(void)
{
  return Memo("PORT");
}

static void SwitchFrom_8008(void)
{
  DeinitFields();
}

static void SwitchTo_8008(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("8008");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = FoundDescr->Id; NOPCode = 0xc0;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegIO);
  Grans[SegCode ] = 1; ListGrans[SegCode ] = 1; SegInits[SegCode ] = 0;
  SegLimits[SegCode] = 0x3fff;
  Grans[SegIO   ] = 1; ListGrans[SegIO   ] = 1; SegInits[SegIO   ] = 0;
  SegLimits[SegIO] = 7;

  MakeCode = MakeCode_8008;
  IsDef = IsDef_8008;
  SwitchFrom = SwitchFrom_8008;

  InitFields();
}

/*---------------------------------------------------------------------------*/
/* Initialisierung */

void code8008_init(void)
{
  CPU8008    = AddCPU("8008"   , SwitchTo_8008);
  CPU8008New = AddCPU("8008NEW", SwitchTo_8008);
}
