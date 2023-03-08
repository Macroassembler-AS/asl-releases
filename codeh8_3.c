/* codeh8_3.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator H8/300(L/H)                                                 */
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
#include "onoff_common.h"
#include "asmstructs.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codeh8_3.h"

#define ModNone (-1)
#define ModReg 0
#define MModReg (1 << ModReg)
#define ModImm 1
#define MModImm (1 << ModImm)
#define ModAbs8 2
#define MModAbs8 (1 << ModAbs8)
#define ModAbs16 3
#define MModAbs16 (1 << ModAbs16)
#define ModAbs24 4
#define MModAbs24 (1 << ModAbs24)
#define MModAbs (MModAbs8 | MModAbs16 | MModAbs24)
#define ModIReg 5
#define MModIReg (1 << ModIReg)
#define ModPreDec 6
#define MModPreDec (1 << ModPreDec)
#define ModPostInc 7
#define MModPostInc (1 << ModPostInc)
#define ModInd16 8
#define MModInd16 (1 << ModInd16)
#define ModInd24 9
#define MModInd24 (1 << ModInd24)
#define ModIIAbs 10
#define MModIIAbs (1 << ModIIAbs)
#define MModInd (MModInd16 | MModInd24)

/* keep in the same order as in registration */

#define M_CPUH8_300L  (1 << 0)
#define M_CPU6413308  (1 << 1)
#define M_CPUH8_300   (1 << 2)
#define M_CPU6413309  (1 << 3)
#define M_CPUH8_300H  (1 << 4)

#define REG_SP 7

static tSymbolSize OpSize, MomSize;
static ShortInt AdrMode;    /* Ergebnisadressmodus */
static Byte AdrPart;        /* Adressierungsmodusbits im Opcode */
static Word AdrVals[6];     /* Adressargument */

static CPUVar CPUH8_300L;
static CPUVar CPU6413308,CPUH8_300;
static CPUVar CPU6413309,CPUH8_300H;
static Boolean CPU16;       /* keine 32-Bit-Register */

/*-------------------------------------------------------------------------*/
/* Adressparsing */

static void SetOpSize(tSymbolSize Size)
{
  if (OpSize == eSymbolSizeUnknown)
    OpSize = Size;
  else if (Size != OpSize)
  {
    WrError(ErrNum_ConfOpSizes);
    AdrMode = ModNone;
    AdrCnt = 0;
  }
}

static Boolean IsNum(char Inp, Byte *Erg)
{
  if ((Inp < '0') || (Inp > '7'))
    return False;
  else
  {
    *Erg = Inp - AscOfs;
    return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(char *pArg, Byte *pResult, tSymbolSize *pSize)
 * \brief  check whether argument is a CPU register
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \param  pSize register size if yes
 * \return True if it is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(char *pArg, Byte *pResult, tSymbolSize *pSize)
{
  if (!as_strcasecmp(pArg, "SP"))
  {
    *pResult = REG_SP | REGSYM_FLAG_ALIAS;
    *pSize = MaxMode ? eSymbolSize32Bit : eSymbolSize16Bit;
    return True;
  }

  switch (strlen(pArg))
  {
    case 2:
      if (IsNum(pArg[1], pResult))
      {
        if (as_toupper(*pArg) == 'R')
        {
          *pSize = eSymbolSize16Bit;
          return True;
        }
        else if (as_toupper(*pArg) == 'E')
        {
          *pResult += 8;
          *pSize = eSymbolSize16Bit;
          return True;
        }
      }
      break;
    case 3:
      if ((as_toupper(*pArg) == 'R') && IsNum(pArg[1], pResult))
      {
        if (as_toupper(pArg[2]) == 'L')
        {
          *pResult += 8;
          *pSize = eSymbolSize8Bit;
          return True;
        }
        else if (as_toupper(pArg[2]) == 'H')
        {
          *pSize = eSymbolSize8Bit;
          return True;
        }
      }
      else if ((as_toupper(*pArg) == 'E') && (as_toupper(pArg[1]) == 'R') && IsNum(pArg[2], pResult))
      {
        *pSize = eSymbolSize32Bit;
        return True;
      }
  }

  *pSize = eSymbolSizeUnknown;
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_H8_3(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - C16x variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_H8_3(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%u%c", (unsigned)(Value & 7), "HL"[(Value >> 3) & 1]);
      break;
    case eSymbolSize16Bit:
      if (Value == (REG_SP | REGSYM_FLAG_ALIAS))
        as_snprintf(pDest, DestSize, "SP");
      else
        as_snprintf(pDest, DestSize, "%c%u", "RE"[(Value >> 3) & 1], (unsigned)(Value & 7));
      break;
    case eSymbolSize32Bit:
      if (Value == (REG_SP | REGSYM_FLAG_ALIAS))
        as_snprintf(pDest, DestSize, "SP");
      else
        as_snprintf(pDest, DestSize, "ER%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Byte *pResult, tSymbolSize ReqSize, tSymbolSize *pSize)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \param  SizeMask requested register size(s)
 * \param  pSize register size if yes
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Byte *pResult, unsigned SizeMask, tSymbolSize *pSize, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult, pSize))
  {
    RegEvalResult = eIsReg;
    *pResult &= ~REGSYM_FLAG_ALIAS;
  }
  else
  {
    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
    *pSize = EvalResult.DataSize;
    *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  }

  if ((RegEvalResult == eIsReg)
   && !((SizeMask >> *pSize) & 1))
  {
    WrStrErrorPos(ErrNum_InvOpSize, pArg);
    return MustBeReg ? eIsNoReg : eRegAbort;
  }

  return RegEvalResult;
}

static void CutSize(tStrComp *pArg)
{
  int ArgLen = strlen(pArg->str.p_str);

  if ((ArgLen >= 2) && !strcmp(pArg->str.p_str + ArgLen - 2, ":8"))
  {
    StrCompShorten(pArg, 2);
    MomSize = eSymbolSize8Bit;
  }
  else if ((ArgLen >= 3) && !strcmp(pArg->str.p_str + ArgLen - 3, ":16"))
  {
    StrCompShorten(pArg, 3);
    MomSize = eSymbolSize16Bit;
  }
  else if ((ArgLen >= 3) && !strcmp(pArg->str.p_str + ArgLen - 3, ":24"))
  {
    StrCompShorten(pArg, 3);
    MomSize = eSymbolSize24Bit;
  }
}

static Boolean ChkCPU32(tErrorNum ErrorNum)
{
  return ChkMinCPUExt(CPU6413309, ErrorNum);
}

static Byte DecodeBaseReg(const tStrComp *pArg, Byte *Erg, Boolean MustBeReg)
{
  tSymbolSize HSize;
  Word Mask;

  switch (DecodeReg(pArg, Erg, (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), &HSize, MustBeReg))
  {
    case eIsNoReg:
      return 0;
    case eRegAbort:
      return 1;
    default:
      break;
  }
  if ((HSize == eSymbolSize16Bit) && (*Erg > 7))
  {
    WrStrErrorPos(ErrNum_InvReg, pArg);
    return 1;
  }
  Mask = (HSize == eSymbolSize16Bit) ? (M_CPUH8_300L | M_CPU6413308 | M_CPUH8_300) : (M_CPU6413309 | M_CPUH8_300H);
  if (ChkExactCPUMaskExt(Mask, CPUH8_300L, ErrNum_AddrModeNotSupported) < 0)
    return 1;
  return 2;
}

static Boolean Is8(LongInt Address)
{
  if (CPU16)
    return (((Address >> 8) & 0xff) == 0xff);
  else
    return (((Address >> 8) & 0xffff) == 0xffff);
}

static Boolean Is16(LongInt Address)
{
  return (CPU16) ? (True) : (((Address >= 0) && (Address <= 0x7fff)) || ((Address >= 0xff8000) && (Address <= 0xffffff)));
}

static void DecideVAbsolute(LongInt Address, Word Mask)
{
  /* bei Automatik Operandengroesse festlegen */

  if (MomSize == eSymbolSizeUnknown)
  {
    if (Is8(Address))
      MomSize = eSymbolSize8Bit;
    else if (Is16(Address))
      MomSize = eSymbolSize16Bit;
    else
      MomSize = eSymbolSize24Bit;
  }

  /* wenn nicht vorhanden, eins rauf */

  if ((MomSize == eSymbolSize8Bit) && ((Mask & MModAbs8) == 0))
    MomSize = eSymbolSize16Bit;
  if ((MomSize == eSymbolSize16Bit) && ((Mask & MModAbs16) == 0))
    MomSize = eSymbolSize24Bit;

  /* entsprechend Modus Bytes ablegen */

  switch (MomSize)
  {
    case eSymbolSize8Bit:
      if (!Is8(Address)) WrError(ErrNum_AdrOverflow);
      else
      {
        AdrCnt = 2;
        AdrVals[0] = Address & 0xff;
        AdrMode = ModAbs8;
      }
      break;
    case eSymbolSize16Bit:
      if (!Is16(Address)) WrError(ErrNum_AdrOverflow);
      else
      {
        AdrCnt = 2;
        AdrVals[0] = Address & 0xffff;
        AdrMode = ModAbs16;
      }
      break;
    case eSymbolSize24Bit:
      AdrCnt = 4;
      AdrVals[1] = Address & 0xffff;
      AdrVals[0] = Lo(Address >> 16);
      AdrMode = ModAbs24;
      break;
    default:
      WrError(ErrNum_InternalError);
  }
}

static void DecideAbsolute(const tStrComp *pArg, Word Mask)
{
  LongInt Addr;
  Boolean OK;

  Addr = EvalStrIntExpression(pArg, Int32, &OK);
  if (OK)
    DecideVAbsolute(Addr, Mask);
}


static void DecodeAdr(tStrComp *pArg, Word Mask)
{
  tSymbolSize HSize;
  Byte HReg;
  LongInt HLong;
  Boolean OK;
  char *p;
  LongInt DispAcc;
  int ArgLen;

  AdrMode = ModNone;
  AdrCnt = 0;
  MomSize = eSymbolSizeUnknown;

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case eSymbolSizeUnknown:
        WrError(ErrNum_UndefOpSizes);
        break;
      case eSymbolSize8Bit:
        HReg = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        if (OK)
        {
          AdrCnt = 2;
          AdrVals[0] = HReg;
          AdrMode = ModImm;
        }
        break;
      case eSymbolSize16Bit:
        AdrVals[0] = EvalStrIntExpressionOffs(pArg, 1, Int16, &OK);
        if (OK)
        {
          AdrCnt = 2;
          AdrMode = ModImm;
        }
        break;
      case eSymbolSize32Bit:
        HLong = EvalStrIntExpressionOffs(pArg, 1, Int32, &OK);
        if (OK)
        {
          AdrCnt = 4;
          AdrVals[0] = HLong >> 16;
          AdrVals[1] = HLong & 0xffff;
          AdrMode = ModImm;
        }
        break;
      default:
        WrError(ErrNum_InvOpSize);
    }
    goto chk;
  }

  /* Register ? */

  switch (DecodeReg(pArg, &HReg, (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), &HSize, False))
  {
    case eIsReg:
      AdrMode = ModReg;
      AdrPart = HReg;
      SetOpSize(HSize);
      goto chk;
    case eRegAbort:
      return;
    case eIsNoReg:
      break;
  }

  /* indirekt ? */

  if (*pArg->str.p_str == '@')
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);

    if (*Arg.str.p_str == '@')
    {
      AdrVals[0] = EvalStrIntExpressionOffs(&Arg, 1, UInt8, &OK) & 0xff;
      if (OK)
      {
        AdrCnt = 1;
        AdrMode = ModIIAbs;
      }
      goto chk;
    }

    switch (DecodeBaseReg(&Arg, &AdrPart, False))
    {
      case 1:
        goto chk;
      case 2:
        AdrMode = ModIReg;
        goto chk;
    }

    if (*Arg.str.p_str == '-')
    {
      tStrComp Reg;

      StrCompRefRight(&Reg, &Arg, 1);
      switch (DecodeBaseReg(&Reg, &AdrPart, True))
      {
        case 1:
          goto chk;
        case 2:
          AdrMode = ModPreDec;
          goto chk;
      }
    }

    ArgLen = strlen(Arg.str.p_str);
    if (*Arg.str.p_str && (Arg.str.p_str[ArgLen - 1] == '+'))
    {
      StrCompShorten(&Arg, 1);
      switch (DecodeBaseReg(&Arg, &AdrPart, True))
      {
        case 1:
          goto chk;
        case 2:
         AdrMode = ModPostInc;
         goto chk;
      }
      Arg.str.p_str[ArgLen - 1] = '+'; Arg.Pos.Len++;
    }

    if (IsIndirect(Arg.str.p_str))
    {
      tStrComp Part, Remainder;

      StrCompRefRight(&Part, &Arg, 1);
      StrCompShorten(&Part, 1);

      AdrPart = 0xff;
      DispAcc = 0;
      do
      {
        p = QuotPos(Part.str.p_str, ',');
        if (p)
          StrCompSplitRef(&Part, &Remainder, &Part, p);
        switch (DecodeBaseReg(&Part, &HReg, False))
        {
          case 2:
            if (AdrPart != 0xff)
            {
              WrError(ErrNum_InvAddrMode);
              goto chk;
            }
            else
              AdrPart = HReg;
            break;
          case 1:
            goto chk;
          case 0:
            CutSize(&Part);
            DispAcc += EvalStrIntExpression(&Part, Int32, &OK);
            if (!OK)
            {
              goto chk;
            }
            break;
        }
        if (p)
          Part = Remainder;
      }
      while (p);

      if (AdrPart == 0xff)
        DecideVAbsolute(DispAcc, Mask);
      else
      {
        if ((CPU16) && ((DispAcc & 0xffff8000) == 0x8000))
          DispAcc += 0xffff0000;
        if (MomSize == eSymbolSizeUnknown)
          MomSize = ((DispAcc >= -32768) && (DispAcc <= 32767)) ? eSymbolSize16Bit : eSymbolSize24Bit;
        switch (MomSize)
        {
          case eSymbolSize8Bit:
            WrError(ErrNum_InvOpSize);
            break;
          case eSymbolSize16Bit:
            if (ChkRange(DispAcc, -32768, 32767))
            {
              AdrCnt = 2;
              AdrVals[0] = DispAcc & 0xffff;
              AdrMode = ModInd16;
            }
            break;
          case eSymbolSize24Bit:
            AdrVals[1] = DispAcc & 0xffff;
            AdrVals[0] = Lo(DispAcc >> 16);
            AdrCnt = 4;
            AdrMode = ModInd24;
            break;
          default:
            WrError(ErrNum_InternalError);
        }
      }
    }
    else
    {
      CutSize(&Arg);
      DecideAbsolute(&Arg, Mask);
    }
    goto chk;
  }

  CutSize(pArg);
  DecideAbsolute(pArg, Mask);

chk:
  if (((AdrMode == ModReg) && (OpSize == eSymbolSize32Bit))
   || ((AdrMode == ModReg) && (OpSize == eSymbolSize16Bit) && (AdrPart > 7))
   || (AdrMode == ModAbs24)
   || (AdrMode == ModInd24))
  {
    if (!ChkMinCPUExt(CPU6413309, ErrNum_AddrModeNotSupported))
    {
      AdrMode = ModNone;
      AdrCnt = 0;
    }
  }
  if ((AdrMode != ModNone) && ((Mask & (1 << AdrMode)) == 0))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
    AdrCnt = 0;
  }
}

static LongInt ImmVal(void)
{
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      return Lo(AdrVals[0]);
    case eSymbolSize16Bit:
      return AdrVals[0];
    case eSymbolSize32Bit:
      return (((LongInt)AdrVals[0]) << 16) + AdrVals[1];
    default:
      WrError(ErrNum_InternalError);
      return 0;
  }
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits in symbol table:
 * Bits 10...3: Absolute address (8-bit value for range $FFxx or $FFFFxx)
 * Bits 0..2: Bit position
 */

/*!------------------------------------------------------------------------
 * \fn     EvalBitPosition(const tStrComp *pArg, Boolean *pOK)
 * \brief  evaluate bit position
 * \param  bit position argument (with or without #)
 * \param  pOK parsing OK?
 * \return numeric bit position
 * ------------------------------------------------------------------------ */

static LongWord EvalBitPosition(const tStrComp *pArg, Boolean *pOK)
{
  return EvalStrIntExpressionOffs(pArg, !!(*pArg->str.p_str == '#'), UInt3, pOK);
}

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, LongWord Address)
 * \brief  build the compact internal representation of a bit symbol
 * \param  BitPos bit position in byte
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, Word Address)
{
  return
    (Address << 3)
  | (BitPos << 0);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pBitArg, tStrComp *pRegArg)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pBitArg, tStrComp *pRegArg)
{
  Boolean OK;
  LongWord BitPos;

  BitPos = EvalBitPosition(pBitArg, &OK);
  if (!OK)
    return False;

  DecideAbsolute(pRegArg, MModAbs8);
  if (AdrMode != ModAbs8)
    return False;

  *pResult = AssembleBitSymbol(BitPos, AdrVals[0]);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg(LongWord *pResult, int Start, int Stop)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg(LongWord *pResult, int Start, int Stop)
{
  *pResult = 0;

  /* Just one argument -> parse as bit argument */

  if (Start == Stop)
  {
    tEvalResult EvalResult;

    *pResult = EvalStrIntExpressionWithResult(&ArgStr[Start], UInt32, &EvalResult);
    if (EvalResult.OK)
      ChkSpace(SegBData, EvalResult.AddrSpaceMask);
    return EvalResult.OK;
  }

  /* register & bit position are given as separate arguments */

  else if (Stop == Start + 1)
    return DecodeBitArg2(pResult, &ArgStr[Start], &ArgStr[Stop]);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos)
 * \brief  transform compact representation of bit (field) symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress register address
 * \param  pBitPos bit position
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, LongWord *pAddress, Byte *pBitPos)
{
  *pAddress = (BitSymbol >> 3) & 0xfful;
  *pBitPos = BitSymbol & 7;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_H8_3(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_H8_3(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  LongWord Address;

  DissectBitSymbol(Inp, &Address, &BitPos);

  as_snprintf(pDest, DestSize, "#%u,$%s%x", BitPos,
              (HexStartCharacter == 'A') ? (CPU16 ? "FF" : "FFFF") : (CPU16 ? "ff" : "ffff"),
              (unsigned)Address);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandBit_H8_3(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandBit_H8_3(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
{
  LongWord Address = Base + pStructElem->Offset;

  if (pInnermostNamedStruct)
  {
    PStructElem pElem = CloneStructElem(pVarName, pStructElem);

    if (!pElem)
      return;
    pElem->Offset = Address;
    AddStructElem(pInnermostNamedStruct->StructRec, pElem);
  }
  else
  {
    if (!ChkRange(Address, SegLimits[SegCode] - 0xff, SegLimits[SegCode])
     || !ChkRange(pStructElem->BitPos, 0, 7))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*-------------------------------------------------------------------------*/
/* Code Generators */

static void DecodeFixed(Word Code)
{
  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    CodeLen = 2;
    WAsmCode[0] = Code;
  }
}

static void DecodeEEPMOV(Word Code)
{
  UNUSED(Code);

  if (OpSize == eSymbolSizeUnknown)
    OpSize = CPU16 ? eSymbolSize8Bit : eSymbolSize16Bit;
  if (OpSize > eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
  else if (!ChkArgCnt(0, 0));
  else if ((OpSize == eSymbolSize16Bit) && !ChkCPU32(ErrNum_AddrModeNotSupported));
  else
  {
    CodeLen = 4;
    WAsmCode[0] = (OpSize == eSymbolSize8Bit) ? 0x7b5c : 0x7bd4;
    WAsmCode[1] = 0x598f;
  }
}

static void DecodeMOV(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModPreDec | MModInd | MModAbs);
    switch (AdrMode)
    {
      case ModReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModPostInc | MModInd | MModImm | ((OpSize == eSymbolSize8Bit) ? MModAbs : (MModAbs16 | MModAbs24)));
        switch (AdrMode)
        {
          case ModReg:
          {
            int z = (OpSize == eSymbolSize32Bit) ? 3 : OpSize;

            CodeLen = 2;
            WAsmCode[0] = 0x0c00 + (z << 8) + (AdrPart << 4) + HReg;
            if (OpSize == eSymbolSize32Bit)
              WAsmCode[0] += 0x80;
            break;
          }
          case ModIReg:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 2;
                WAsmCode[0] = 0x6800 + (AdrPart << 4) + HReg;
                break;
              case eSymbolSize16Bit:
                CodeLen = 2;
                WAsmCode[0] = 0x6900 + (AdrPart << 4) + HReg;
                break;
              case eSymbolSize32Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x0100;
                WAsmCode[1] = 0x6900 + (AdrPart << 4) + HReg;
                break;
              default:
                break;
            }
            break;
          case ModPostInc:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 2;
                WAsmCode[0] = 0x6c00 + (AdrPart << 4) + HReg;
                break;
              case eSymbolSize16Bit:
                CodeLen = 2;
                WAsmCode[0] = 0x6d00 + (AdrPart << 4) + HReg;
                break;
              case eSymbolSize32Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x0100;
                WAsmCode[1] = 0x6d00 + (AdrPart << 4) + HReg;
                break;
              default:
                break;
            }
            break;
          case ModInd16:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x6e00 + (AdrPart << 4) + HReg;
                WAsmCode[1] = AdrVals[0];
                break;
              case eSymbolSize16Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x6f00 + (AdrPart << 4) + HReg;
                WAsmCode[1] = AdrVals[0];
                break;
              case eSymbolSize32Bit:
                CodeLen = 6;
                WAsmCode[0] = 0x0100;
                WAsmCode[1] = 0x6f00 + (AdrPart << 4) + HReg;
                WAsmCode[2] = AdrVals[0];
                break;
              default:
                break;
            }
            break;
          case ModInd24:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 8;
                WAsmCode[0] = 0x7800 + (AdrPart << 4);
                WAsmCode[1] = 0x6a20 + HReg;
                memcpy(WAsmCode + 2, AdrVals, AdrCnt);
                break;
              case eSymbolSize16Bit:
                CodeLen = 8;
                WAsmCode[0] = 0x7800 + (AdrPart << 4);
                WAsmCode[1] = 0x6b20 + HReg;
                memcpy(WAsmCode + 2, AdrVals, AdrCnt);
                break;
              case eSymbolSize32Bit:
                CodeLen = 10;
                WAsmCode[0] = 0x0100;
                WAsmCode[1] = 0x7800 + (AdrPart << 4);
                WAsmCode[2] = 0x6b20 + HReg;
                memcpy(WAsmCode + 3, AdrVals, AdrCnt);
                break;
              default:
                break;
            }
            break;
          case ModAbs8:
            CodeLen = 2;
            WAsmCode[0] = 0x2000 + (((Word)HReg) << 8) + Lo(AdrVals[0]);
            break;
          case ModAbs16:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x6a00 + HReg;
                WAsmCode[1] = AdrVals[0];
                break;
              case eSymbolSize16Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x6b00 + HReg;
                WAsmCode[1] = AdrVals[0];
                break;
              case eSymbolSize32Bit:
                CodeLen = 6;
                WAsmCode[0] = 0x0100;
                WAsmCode[1] = 0x6b00 + HReg;
                WAsmCode[2] = AdrVals[0];
                break;
              default:
                break;
            }
            break;
          case ModAbs24:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 6;
                WAsmCode[0] = 0x6a20 + HReg;
                memcpy(WAsmCode + 1, AdrVals, AdrCnt);
                break;
              case eSymbolSize16Bit:
                CodeLen = 6;
                WAsmCode[0] = 0x6b20 + HReg;
                memcpy(WAsmCode + 1, AdrVals, AdrCnt);
                break;
              case eSymbolSize32Bit:
                CodeLen = 8;
                WAsmCode[0] = 0x0100;
                WAsmCode[1] = 0x6b20 + HReg;
                memcpy(WAsmCode + 2, AdrVals, AdrCnt);
                break;
              default:
                break;
            }
            break;
          case ModImm:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 2;
                WAsmCode[0] = 0xf000 + (((Word)HReg) << 8) + Lo(AdrVals[0]);
                break;
              case eSymbolSize16Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x7900 + HReg;
                WAsmCode[1] = AdrVals[0];
                break;
              case eSymbolSize32Bit:
                CodeLen = 6;
                WAsmCode[0] = 0x7a00 + HReg;
                memcpy(WAsmCode + 1, AdrVals, AdrCnt);
                break;
              default:
                break;
            }
            break;
        }
        break;
      case ModIReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen = 2;
              WAsmCode[0] = 0x6880 + (HReg << 4) + AdrPart;
              break;
            case eSymbolSize16Bit:
              CodeLen = 2;
              WAsmCode[0] = 0x6980 + (HReg << 4) + AdrPart;
              break;
            case eSymbolSize32Bit:
              CodeLen = 4;
              WAsmCode[0] = 0x0100;
              WAsmCode[1] = 0x6980 + (HReg << 4) + AdrPart;
              break;
            default:
              break;
          }
        }
        break;
      case ModPreDec:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen=2; WAsmCode[0]=0x6c80+(HReg << 4)+AdrPart;
              break;
            case eSymbolSize16Bit:
              CodeLen=2; WAsmCode[0]=0x6d80+(HReg << 4)+AdrPart;
              break;
            case eSymbolSize32Bit:
              CodeLen=4; WAsmCode[0]=0x0100;
              WAsmCode[1]=0x6d80+(HReg << 4)+AdrPart;
              break;
            default:
              break;
          }
        }
        break;
      case ModInd16:
        HReg = AdrPart;
        WAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode!=ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen = 4;
              WAsmCode[0] = 0x6e80 + (HReg << 4) + AdrPart;
              break;
            case eSymbolSize16Bit:
              CodeLen = 4;
              WAsmCode[0] = 0x6f80 + (HReg << 4) + AdrPart;
              break;
            case eSymbolSize32Bit:
              CodeLen = 6;
              WAsmCode[0] = 0x0100;
              WAsmCode[2] = WAsmCode[1];
              WAsmCode[1] = 0x6f80 + (HReg << 4) + AdrPart;
              break;
            default:
              break;
          }
        }
        break;
      case ModInd24:
        HReg = AdrPart;
        memcpy(WAsmCode + 2, AdrVals, 4);
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen = 8;
              WAsmCode[0] = 0x7800 + (HReg << 4);
              WAsmCode[1] = 0x6aa0 + AdrPart;
              break;
            case eSymbolSize16Bit:
              CodeLen = 8;
              WAsmCode[0] = 0x7800 + (HReg << 4);
              WAsmCode[1] = 0x6ba0 + AdrPart;
              break;
            case eSymbolSize32Bit:
              CodeLen = 10;
              WAsmCode[0] = 0x0100;
              WAsmCode[4] = WAsmCode[3];
              WAsmCode[3] = WAsmCode[2];
              WAsmCode[1] = 0x7800 + (HReg << 4);
              WAsmCode[2] = 0x6ba0 + AdrPart;
              break;
            default:
              break;
          }
        }
        break;
      case ModAbs8:
        HReg = Lo(AdrVals[0]);
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen = 2;
              WAsmCode[0] = 0x3000 + (((Word)AdrPart) << 8) + HReg;
              break;
            case eSymbolSize16Bit:
              CodeLen = 4;
              WAsmCode[0] = 0x6b80 + AdrPart;
              WAsmCode[1] = 0xff00 + HReg;
              break;
            case eSymbolSize32Bit:
              CodeLen = 6;
              WAsmCode[0] = 0x0100;
              WAsmCode[1] = 0x6b80 + AdrPart;
              WAsmCode[2] = 0xff00 + HReg;
              break;
            default:
              break;
          }
        }
        break;
      case ModAbs16:
        WAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen = 4;
              WAsmCode[0] = 0x6a80 + AdrPart;
              break;
            case eSymbolSize16Bit:
              CodeLen = 4;
              WAsmCode[0] = 0x6b80 + AdrPart;
              break;
            case eSymbolSize32Bit:
              CodeLen = 6;
              WAsmCode[0] = 0x0100;
              WAsmCode[2] = WAsmCode[1];
              WAsmCode[1] = 0x6b80 + AdrPart;
              break;
            default:
              break;
          }
        }
        break;
      case ModAbs24:
        memcpy(WAsmCode + 1, AdrVals, 4);
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              CodeLen = 6;
              WAsmCode[0] = 0x6aa0 + AdrPart;
              break;
            case eSymbolSize16Bit:
              CodeLen = 6;
              WAsmCode[0] = 0x6ba0 + AdrPart;
              break;
            case eSymbolSize32Bit:
              CodeLen = 8;
              WAsmCode[0] = 0x0100;
              WAsmCode[3] = WAsmCode[2];
              WAsmCode[2] = WAsmCode[1];
              WAsmCode[1] = 0x6ba0 + AdrPart;
              break;
            default:
              break;
          }
        }
        break;
    }
  }
}

static void DecodeMOVTPE_MOVFPE(Word CodeTPE)
{
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU6413308))
  {
    tStrComp *pRegArg = CodeTPE ? &ArgStr[1] : &ArgStr[2],
             *pMemArg = CodeTPE ? &ArgStr[2] : &ArgStr[1];

    DecodeAdr(pRegArg, MModReg);
    if (AdrMode != ModNone)
    {
      if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else
      {
        Byte HReg = AdrPart;
        DecodeAdr(pMemArg, MModAbs16);
        if (AdrMode != ModNone)
        {
          CodeLen = 4;
          WAsmCode[0] = 0x6a40 + CodeTPE + HReg;
          WAsmCode[1] = AdrVals[0];
        }
      }
    }
  }
}

static void DecodePUSH_POP(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode != ModNone)
    {
      if (OpSize == eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else if ((OpSize != eSymbolSize32Bit) || ChkCPU32(ErrNum_AddrModeNotSupported))
      {
        if (OpSize == eSymbolSize32Bit)
          WAsmCode[0] = 0x0100;
        CodeLen = 2 * OpSize;
        WAsmCode[(CodeLen - 2) >> 1] = Code + AdrPart;
      }
    }
  }
}

static void DecodeLDC_STC(Word CodeIsSTC)
{
  if (ChkArgCnt(2, 2))
  {
    tStrComp *pRegArg = CodeIsSTC ? &ArgStr[1] : &ArgStr[2],
             *pMemArg = CodeIsSTC ? &ArgStr[2] : &ArgStr[1];

    if (as_strcasecmp(pRegArg->str.p_str, "CCR")) WrError(ErrNum_InvAddrMode);
    else
    {
       SetOpSize(eSymbolSize8Bit);
       DecodeAdr(pMemArg, MModReg | MModIReg | MModInd | MModAbs16 | MModAbs24 | (CodeIsSTC ? MModPreDec : (MModImm | MModPostInc)));
       switch (AdrMode)
       {
         case ModReg:
           CodeLen = 2;
           WAsmCode[0] = 0x0300 + AdrPart - (CodeIsSTC << 1);
           break;
         case ModIReg:
           CodeLen = 4;
           WAsmCode[0] = 0x0140;
           WAsmCode[1] = 0x6900 + CodeIsSTC + (AdrPart << 4);
           break;
         case ModPostInc:
         case ModPreDec:
           CodeLen = 4;
           WAsmCode[0] = 0x0140;
           WAsmCode[1] = 0x6d00 + CodeIsSTC + (AdrPart << 4);
           break;
         case ModInd16:
           CodeLen = 6;
           WAsmCode[0] = 0x0140;
           WAsmCode[2] = AdrVals[0];
           WAsmCode[1] = 0x6f00 + CodeIsSTC + (AdrPart << 4);
           break;
         case ModInd24:
           CodeLen = 10;
           WAsmCode[0] = 0x0140;
           WAsmCode[1] = 0x7800 + (AdrPart << 4);
           WAsmCode[2] = 0x6b20 + CodeIsSTC;
           memcpy(WAsmCode + 3, AdrVals, AdrCnt);
           break;
         case ModAbs16:
           CodeLen = 6;
           WAsmCode[0] = 0x0140;
           WAsmCode[2] = AdrVals[0];
           WAsmCode[1] = 0x6b00 + CodeIsSTC;
           break;
         case ModAbs24:
           CodeLen = 8;
           WAsmCode[0] = 0x0140;
           WAsmCode[1] = 0x6b20 + CodeIsSTC;
           memcpy(WAsmCode + 2, AdrVals, AdrCnt);
           break;
         case ModImm:
           CodeLen = 2;
           WAsmCode[0] = 0x0700 + Lo(AdrVals[0]);
           break;
       }
    }
  }
}

static void DecodeADD_SUB(Word IsSUB)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode != ModNone)
    {
      Byte HReg = AdrPart;
      DecodeAdr(&ArgStr[1], MModReg | MModImm);
      if (AdrMode != ModNone)
      {
        if (((OpSize > eSymbolSize16Bit) || ((OpSize == eSymbolSize16Bit) && (AdrMode == ModImm))) && !ChkCPU32(ErrNum_AddrModeNotSupported));
        else
        {
          switch (AdrMode)
          {
            case ModImm:
              switch (OpSize)
              {
                case eSymbolSize8Bit:
                  if (IsSUB) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    CodeLen = 2;
                    WAsmCode[0] = 0x8000 + (((Word)HReg) << 8) + Lo(AdrVals[0]);
                  }
                  break;
                case eSymbolSize16Bit:
                  CodeLen = 4;
                  WAsmCode[1] = AdrVals[0];
                  WAsmCode[0] = 0x7910 + (IsSUB << 5) + HReg;
                  break;
                case eSymbolSize32Bit:
                  CodeLen = 6;
                  memcpy(WAsmCode + 1, AdrVals, 4);
                  WAsmCode[0] = 0x7a10 + (IsSUB << 5) + HReg;
                  break;
                default:
                  break;
              }
              break;
            case ModReg:
              switch (OpSize)
              {
                case eSymbolSize8Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0x0800 + (IsSUB << 12) + (AdrPart << 4) + HReg;
                  break;
                case eSymbolSize16Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0x0900 + (IsSUB << 12) + (AdrPart << 4) + HReg;
                  break;
                case eSymbolSize32Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0x0a00 + (IsSUB << 12) + 0x80 + (AdrPart << 4) + HReg;
                  break;
                default:
                  break;
              }
              break;
          }
        }
      }
    }
  }
}

static void DecodeCMP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode != ModNone)
    {
      Byte HReg = AdrPart;
      DecodeAdr(&ArgStr[1], MModReg | MModImm);
      if (AdrMode != ModNone)
      {
        if (((OpSize > eSymbolSize16Bit) || ((OpSize == eSymbolSize16Bit) && (AdrMode == ModImm))) && !ChkCPU32(ErrNum_AddrModeNotSupported));
        else
        {
          switch (AdrMode)
          {
            case ModImm:
              switch (OpSize)
              {
                case eSymbolSize8Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0xa000 + (((Word)HReg) << 8) + Lo(AdrVals[0]);
                  break;
                case eSymbolSize16Bit:
                  CodeLen = 4;
                  WAsmCode[1] = AdrVals[0];
                  WAsmCode[0] = 0x7920 + HReg;
                  break;
                case eSymbolSize32Bit:
                  CodeLen = 6;
                  memcpy(WAsmCode + 1, AdrVals, 4);
                  WAsmCode[0] = 0x7a20 + HReg;
                default:
                  break;
              }
              break;
            case ModReg:
              switch (OpSize)
              {
                case eSymbolSize8Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0x1c00 + (AdrPart << 4) + HReg;
                  break;
                case eSymbolSize16Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0x1d00 + (AdrPart << 4) + HReg;
                  break;
                case eSymbolSize32Bit:
                  CodeLen = 2;
                  WAsmCode[0] = 0x1f80 + (AdrPart << 4) + HReg;
                  break;
                default:
                  break;
              }
              break;
          }
        }
      }
    }
  }
}

static void DecodeLogic(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode != ModNone)
    {
      if ((OpSize <= eSymbolSize8Bit) || ChkCPU32(ErrNum_AddrModeNotSupported))
      {
        Byte HReg = AdrPart;
        DecodeAdr(&ArgStr[1], MModImm | MModReg);
        switch (AdrMode)
        {
          case ModImm:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 2;
                WAsmCode[0] = 0xc000 + (Code << 12) + (((Word)HReg) << 8) + Lo(AdrVals[0]);
                break;
              case eSymbolSize16Bit:
                CodeLen = 4;
                WAsmCode[1] = AdrVals[0];
                WAsmCode[0] = 0x7940 + (Code << 4) + HReg;
                break;
              case eSymbolSize32Bit:
                CodeLen = 6;
                memcpy(WAsmCode + 1, AdrVals, AdrCnt);
                WAsmCode[0] = 0x7a40 + (Code << 4) + HReg;
                break;
              default:
                break;
            }
            break;
          case ModReg:
            switch (OpSize)
            {
              case eSymbolSize8Bit:
                CodeLen = 2;
                WAsmCode[0] = 0x1400 + (Code << 8) + (AdrPart << 4) + HReg;
                break;
              case eSymbolSize16Bit:
                CodeLen = 2;
                WAsmCode[0] = 0x6400 + (Code << 8) + (AdrPart << 4) + HReg;
                break;
              case eSymbolSize32Bit:
                CodeLen = 4;
                WAsmCode[0] = 0x01f0;
                WAsmCode[1] = 0x6400 + (Code << 8) + (AdrPart << 4) + HReg;
                break;
              default:
                break;
            }
            break;
        }
      }
    }
  }
}

static void DecodeLogicBit(Word Code)
{
  SetOpSize(eSymbolSize8Bit);
  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[2].str.p_str, "CCR")) WrError(ErrNum_InvAddrMode);
  else
  {
    DecodeAdr(&ArgStr[1], MModImm);
    if (AdrMode != ModNone)
    {
      CodeLen = 2;
      WAsmCode[0] = 0x0400 + (Code << 8) + Lo(AdrVals[0]);
    }
  }
}

static void DecodeADDX_SUBX(Word IsSUBX)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode != ModNone)
    {
      if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else
      {
        Byte HReg = AdrPart;
        DecodeAdr(&ArgStr[1], MModImm | MModReg);
        switch (AdrMode)
        {
          case ModImm:
            CodeLen = 2;
            WAsmCode[0] = 0x9000 + (((Word)HReg) << 8) + Lo(AdrVals[0]) + (IsSUBX << 13);
            break;
          case ModReg:
            CodeLen = 2;
            WAsmCode[0] = 0x0e00 + (AdrPart << 4) + HReg + (IsSUBX << 12);
            break;
        }
      }
    }
  }
}

static void DecodeADDS_SUBS(Word IsSUBS)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode != ModNone)
    {
      if (((CPU16) && (OpSize != eSymbolSize16Bit)) || ((!CPU16) && (OpSize != eSymbolSize32Bit))) WrError(ErrNum_InvOpSize);
      else
      {
        Byte HReg = AdrPart;
        DecodeAdr(&ArgStr[1], MModImm);
        if (AdrMode != ModNone)
        {
          LongInt AdrLong = ImmVal();
          if ((AdrLong != 1) && (AdrLong != 2) && (AdrLong != 4)) WrError(ErrNum_OverRange);
          else
          {
            switch (AdrLong)
            {
              case 1: WAsmCode[0] = 0x0b00; break;
              case 2: WAsmCode[0] = 0x0b80; break;
              case 4: WAsmCode[0] = 0x0b90; break;
            }
            CodeLen = 2;
            WAsmCode[0] += HReg + IsSUBS;
          }
        }
      }
    }
  }
}

static void DecodeMul(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    if (OpSize != eSymbolSizeUnknown) OpSize++;
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode != ModNone)
    {
      if (OpSize == eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else if ((OpSize != eSymbolSize32Bit) || ChkCPU32(ErrNum_AddrModeNotSupported))
      {
        Byte HReg = AdrPart;
        OpSize--;
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrMode != ModNone)
        {
          if ((Code & 2) == 2)
          {
            CodeLen = 4;
            WAsmCode[0] = 0x01c0;
            if ((Code & 1) == 1)
              WAsmCode[0] += 0x10;
          }
          else
            CodeLen=2;
          WAsmCode[CodeLen >> 2] = 0x5000
                                 + (((Word)OpSize) << 9)
                                 + ((Code & 1) << 8)
                                 + (AdrPart << 4) + HReg;
        }
      }
    }
  }
}

static void DecodeBit1(Word Code)
{
  Word OpCode = 0x60 + (Code & 0x7f);
  Byte BitPos;

  switch (ArgCnt)
  {
    case 1:
    {
      LongWord BitSpec;

      if (DecodeBitArg(&BitSpec, 1, 1))
      {
        LongWord Addr;

        DissectBitSymbol(BitSpec, &Addr, &BitPos);
        AdrMode = ModAbs8;
        AdrVals[0] = Addr & 0xff;
        AdrCnt = 2;
        goto common;
      }
      break;
    }
    case 2:
    {
      Boolean OK;

      BitPos = EvalBitPosition(&ArgStr[1], &OK);
      if (OK)
      {
        DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModAbs8);
        if (AdrMode != ModNone)
          goto common;
      }
      break;
    }
    common:
      if (OpSize > eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else switch (AdrMode)
      {
        case ModReg:
          CodeLen = 2;
          WAsmCode[0] = (OpCode << 8) + (Code & 0x80) + (BitPos << 4) + AdrPart;
          break;
        case ModIReg:
          CodeLen = 4;
          WAsmCode[0] = 0x7c00 + (AdrPart << 4);
          WAsmCode[1] = (OpCode << 8) + (Code & 0x80) + (BitPos << 4);
          if (OpCode < 0x70)
            WAsmCode[0] += 0x100;
          break;
        case ModAbs8:
          CodeLen = 4;
          WAsmCode[0] = 0x7e00 + Lo(AdrVals[0]);
          WAsmCode[1] = (OpCode << 8) + (Code & 0x80) + (BitPos << 4);
          if (OpCode < 0x70)
            WAsmCode[0] += 0x100;
          break;
      }
      break;
    default:
      (void)ChkArgCnt(1, 2);
  }
}

static void DecodeBit2(Word Code)
{
  Word OpCode;
  Byte Bit;
  Boolean OK;
  tSymbolSize HSize;

  switch (ArgCnt)
  {
    case 1:
    {
      LongWord BitSpec;

      if (DecodeBitArg(&BitSpec, 1, 1))
      {
        LongWord Addr;

        DissectBitSymbol(BitSpec, &Addr, &Bit);
        OpCode = Code + 0x70;
        AdrMode = ModAbs8;
        AdrVals[0] = Addr & 0xff;
        AdrCnt = 2;
        goto common;
      }
      break;
    }
    case 2:
      switch (DecodeReg(&ArgStr[1], &Bit, 1 << eSymbolSize8Bit, &HSize, False))
      {
        case eIsReg:
          OpCode = Code + 0x60;
          OK = True;
          break;
        case eIsNoReg:
          OpCode = Code + 0x70;
          Bit = EvalBitPosition(&ArgStr[1], &OK);
          break;
        default: /* eRegAbort */
          return;
      }
      if (OK)
      {
        DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModAbs8);
        if (AdrMode != ModNone)
          goto common;
        break;
      }
    common:
      if (OpSize > eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else
      {
        switch (AdrMode)
        {
          case ModReg:
            CodeLen = 2;
            WAsmCode[0] = (OpCode << 8) + (Bit << 4) + AdrPart;
            break;
          case ModIReg:
            CodeLen = 4;
            WAsmCode[0] = 0x7d00 + (AdrPart << 4);
            WAsmCode[1] = (OpCode << 8) + (Bit << 4);
            if (Code == 3)
              WAsmCode[0] -= 0x100;
            break;
          case ModAbs8:
            CodeLen = 4;
            WAsmCode[0] = 0x7f00 + Lo(AdrVals[0]);
            WAsmCode[1] = (OpCode << 8) + (Bit << 4);
            if (Code == 3)
              WAsmCode[0] -= 0x100;
            break;
        }
      }
      break;
    default:
      (void)ChkArgCnt(1, 2);
  }
}

static void DecodeINC_DEC(Word Code)
{
  Boolean OK;
  int z;
  Byte HReg;

  if (ChkArgCnt(1, 2))
  {
    DecodeAdr(&ArgStr[ArgCnt], MModReg);
    if (AdrMode != ModNone)
    {
      if ((OpSize <= eSymbolSize8Bit) || ChkCPU32(ErrNum_AddrModeNotSupported))
      {
        HReg = AdrPart;
        if (ArgCnt == 1)
        {
          OK = True;
          z = 1;
        }
        else
        {
          DecodeAdr(&ArgStr[1], MModImm);
          OK = (AdrMode == ModImm);
          if (OK)
          {
            z = ImmVal();
            if (z < 1)
            {
              WrError(ErrNum_UnderRange);
              OK = False;
            }
            else if (((OpSize == eSymbolSize8Bit) && (z > 1)) || (z > 2))
            {
              WrError(ErrNum_OverRange);
              OK = False;
            }
          }
        }
        if (OK)
        {
          CodeLen = 2;
          z--;
          switch (OpSize)
          {
            case eSymbolSize8Bit:
              WAsmCode[0] = Code + 0x0a00 + HReg;
              break;
            case eSymbolSize16Bit:
              WAsmCode[0] = Code + 0x0b50 + HReg + (z << 7);
              break;
            case eSymbolSize32Bit:
              WAsmCode[0] = Code + 0x0b70 + HReg + (z << 7);
              break;
            default:
              break;
          }
        }
      }
    }
  }
}

static void DecodeShift(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode != ModNone)
    {
      if ((OpSize <= eSymbolSize8Bit) || ChkCPU32(ErrNum_AddrModeNotSupported))
      {
        CodeLen = 2;
        switch (OpSize)
        {
          case eSymbolSize8Bit:
            WAsmCode[0] = Code + AdrPart;
            break;
          case eSymbolSize16Bit:
            WAsmCode[0] = Code + AdrPart + 0x10;
            break;
          case eSymbolSize32Bit:
            WAsmCode[0] = Code + AdrPart + 0x30;
            break;
          default:
            break;
        }
      }
    }
  }
}

static void DecodeNEG_NOT(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode != ModNone)
    {
      if ((OpSize <= eSymbolSize8Bit) || ChkCPU32(ErrNum_AddrModeNotSupported))
      {
        CodeLen = 2;
        switch (OpSize)
        {
          case eSymbolSize8Bit:
            WAsmCode[0] = Code + 0x1700 + AdrPart;
            break;
          case eSymbolSize16Bit:
            WAsmCode[0] = Code + 0x1710 + AdrPart;
            break;
          case eSymbolSize32Bit:
            WAsmCode[0] = Code + 0x1730 + AdrPart;
            break;
          default:
            break;
        }
      }
    }
  }
}

static void DecodeEXTS_EXTU(Word IsEXTS)
{
  if (ChkArgCnt(1, 1)
   && ChkCPU32(ErrNum_InstructionNotSupported))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode != ModNone)
    {
      if ((OpSize != eSymbolSize16Bit) && (OpSize != 2)) WrError(ErrNum_InvOpSize);
      else
      {
        CodeLen = 2;
        switch (OpSize)
        {
          case eSymbolSize16Bit:
            WAsmCode[0] = IsEXTS ? 0x17d0 : 0x1750;
            break;
          case eSymbolSize32Bit:
            WAsmCode[0] = IsEXTS ? 0x17f0 : 0x1770;
            break;
          default:
            break;
        }
        WAsmCode[0] += AdrPart;
      }
    }
  }
}

static void DecodeDAA_DAS(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode != ModNone)
    {
      if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else
      {
        CodeLen = 2;
        WAsmCode[0] = Code + AdrPart;
      }
    }
  }
}

static void DecodeCond(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if ((OpSize != eSymbolSizeUnknown) && (OpSize != eSymbolSizeFloat32Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], Int24, &OK, &Flags) - (EProgCounter() + 2);

    if (OK)
    {
      if (OpSize == eSymbolSizeUnknown)
      {
        if ((AdrLong >= -128) && (AdrLong <= 127))
          OpSize = eSymbolSizeFloat32Bit;
        else
        {
          OpSize = eSymbolSize32Bit;
          AdrLong -= 2;
        }
      }
      else if (OpSize == eSymbolSize32Bit)
        AdrLong -= 2;
      if (OpSize == eSymbolSize32Bit)
      {
        if (!mSymbolQuestionable(Flags) && ((AdrLong < -32768) || (AdrLong > 32767))) WrError(ErrNum_JmpDistTooBig);
        else if (ChkCPU32(ErrNum_AddrModeNotSupported))
        {
          CodeLen = 4;
          WAsmCode[0] = 0x5800 + (Code << 4);
          WAsmCode[1] = AdrLong & 0xffff;
        }
      }
      else
      {
        if (!mSymbolQuestionable(Flags) && ((AdrLong < -128) || (AdrLong > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 2;
          WAsmCode[0] = 0x4000 + (Code << 8) + (AdrLong & 0xff);
        }
      }
    }
  }
}

static void DecodeJMP_JSR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModIReg | ((CPU16) ? MModAbs16 : MModAbs24) | MModIIAbs);
    switch (AdrMode)
    {
      case ModIReg:
        CodeLen = 2;
        WAsmCode[0] = 0x5900 + Code + (AdrPart << 4);
        break;
      case ModAbs16:
        CodeLen = 4;
        WAsmCode[0] = 0x5a00 + Code;
        WAsmCode[1] = AdrVals[0];
        break;
      case ModAbs24:
        CodeLen = 4;
        WAsmCode[0] = 0x5a00 + Code + Lo(AdrVals[0]);
        WAsmCode[1] = AdrVals[1];
        break;
      case ModIIAbs:
        CodeLen = 2;
        WAsmCode[0] = 0x5b00 + Code + Lo(AdrVals[0]);
        break;
    }
  }
}

static void DecodeBSR(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if ((OpSize != eSymbolSizeUnknown) && (OpSize != eSymbolSizeFloat32Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], Int24, &OK, &Flags) - (EProgCounter() + 2);

    if (OK)
    {
      if (OpSize == eSymbolSizeUnknown)
      {
        if ((AdrLong >= -128) && (AdrLong <= 127))
          OpSize = eSymbolSizeFloat32Bit;
        else
        {
          OpSize = eSymbolSize32Bit;
          AdrLong -= 2;
        }
      }
      else
      {
        if (OpSize == eSymbolSize32Bit)
          AdrLong -= 2;
      }
      if (OpSize == eSymbolSize32Bit)
      {
        if (!mSymbolQuestionable(Flags) && ((AdrLong < -32768) || (AdrLong > 32767))) WrError(ErrNum_JmpDistTooBig);
        else if (ChkCPU32(ErrNum_AddrModeNotSupported))
        {
          CodeLen = 4;
          WAsmCode[0] = 0x5c00;
          WAsmCode[1] = AdrLong & 0xffff;
        }
      }
      else
      {
        if ((AdrLong < -128) || (AdrLong > 127)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 2;
          WAsmCode[0] = 0x5500 + (AdrLong & 0xff);
        }
      }
    }
  }
}

static void DecodeTRAPA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU6413309))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), UInt2, &OK) << 4;
    if (OK)
    {
      WAsmCode[0] += 0x5700;
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBIT(Word Code)
 * \brief  handle BIT instruction
 * ------------------------------------------------------------------------ */

static void DecodeBIT(Word Code)
{
  UNUSED(Code);

  /* if in structure definition, add special element to structure */

  if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSizeUnknown))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }
  if (ActPC == StructSeg)
  {
    Boolean OK;
    Byte BitPos;
    PStructElem pElement;

    if (!ChkArgCnt(2, 2))
      return;
    BitPos = EvalBitPosition(&ArgStr[1], &OK);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[2].str.p_str);
    pElement->OpSize = eSymbolSize8Bit;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandBit_H8_3;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    LongWord BitSpec;

    if (DecodeBitArg(&BitSpec, 1, ArgCnt))
    {
      *ListLine = '=';
      DissectBit_H8_3(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*-------------------------------------------------------------------------*/
/* dynamische Belegung/Freigabe Codetabellen */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddCond(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeCond);
}

static void AddShift(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeShift);
}

static void AddLogic(const char *NName, const char *NNameBit, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLogic);
  AddInstTable(InstTable, NNameBit, NCode, DecodeLogicBit);
}

static void AddMul(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeMul);
}

static void AddBit1(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit1);
}

static void AddBit2(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit2);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "EEPMOV", 0, DecodeEEPMOV);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MOVTPE", 0x80, DecodeMOVTPE_MOVFPE);
  AddInstTable(InstTable, "MOVFPE", 0x00, DecodeMOVTPE_MOVFPE);
  AddInstTable(InstTable, "PUSH", 0x6df0, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 0x6d70, DecodePUSH_POP);
  AddInstTable(InstTable, "LDC", 0x00, DecodeLDC_STC);
  AddInstTable(InstTable, "STC", 0x80, DecodeLDC_STC);
  AddInstTable(InstTable, "ADD", 0, DecodeADD_SUB);
  AddInstTable(InstTable, "SUB", 1, DecodeADD_SUB);
  AddInstTable(InstTable, "CMP", 0, DecodeCMP);
  AddInstTable(InstTable, "ADDX", 0, DecodeADDX_SUBX);
  AddInstTable(InstTable, "SUBX", 1, DecodeADDX_SUBX);
  AddInstTable(InstTable, "ADDS", 0x0000, DecodeADDS_SUBS);
  AddInstTable(InstTable, "SUBS", 0x1000, DecodeADDS_SUBS);
  AddInstTable(InstTable, "INC", 0x0000, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 0x1000, DecodeINC_DEC);
  AddInstTable(InstTable, "NEG", 0x80, DecodeNEG_NOT);
  AddInstTable(InstTable, "NOT", 0x00, DecodeNEG_NOT);
  AddInstTable(InstTable, "EXTS", 1, DecodeEXTS_EXTU);
  AddInstTable(InstTable, "EXTU", 0, DecodeEXTS_EXTU);
  AddInstTable(InstTable, "DAA", 0x0f00, DecodeDAA_DAS);
  AddInstTable(InstTable, "DAS", 0x1f00, DecodeDAA_DAS);
  AddInstTable(InstTable, "JMP", 0x0000, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR", 0x0400, DecodeJMP_JSR);
  AddInstTable(InstTable, "BSR", 0, DecodeBSR);
  AddInstTable(InstTable, "TRAPA", 0, DecodeTRAPA);

  AddFixed("NOP", 0x0000); AddFixed("RTE"  , 0x5670);
  AddFixed("RTS", 0x5470); AddFixed("SLEEP", 0x0180);

  AddCond("BRA", 0x0); AddCond("BT" , 0x0);
  AddCond("BRN", 0x1); AddCond("BF" , 0x1);
  AddCond("BHI", 0x2); AddCond("BLS", 0x3);
  AddCond("BCC", 0x4); AddCond("BHS", 0x4);
  AddCond("BCS", 0x5); AddCond("BLO", 0x5);
  AddCond("BNE", 0x6); AddCond("BEQ", 0x7);
  AddCond("BVC", 0x8); AddCond("BVS", 0x9);
  AddCond("BPL", 0xa); AddCond("BMI", 0xb);
  AddCond("BGE", 0xc); AddCond("BLT", 0xd);
  AddCond("BGT", 0xe); AddCond("BLE", 0xf);

  AddShift("ROTL" , 0x1280); AddShift("ROTR" , 0x1380);
  AddShift("ROTXL", 0x1200); AddShift("ROTXR", 0x1300);
  AddShift("SHAL" , 0x1080); AddShift("SHAR" , 0x1180);
  AddShift("SHLL" , 0x1000); AddShift("SHLR" , 0x1100);

  AddLogic("OR", "ORC", 0);
  AddLogic("XOR", "XORC", 1);
  AddLogic("AND", "ANDC", 2);

  AddMul("DIVXS", 3);
  AddMul("DIVXU", 1);
  AddMul("MULXS", 2);
  AddMul("MULXU", 0);

  AddBit1("BAND", 0x16); AddBit1("BIAND", 0x96);
  AddBit1("BOR" , 0x14); AddBit1("BIOR" , 0x94);
  AddBit1("BXOR", 0x15); AddBit1("BIXOR", 0x95);
  AddBit1("BLD" , 0x17); AddBit1("BILD" , 0x97);
  AddBit1("BST" , 0x07); AddBit1("BIST" , 0x87);

  AddBit2("BCLR", 2);
  AddBit2("BNOT", 1);
  AddBit2("BSET", 0);
  AddBit2("BTST", 3);

  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_H8_3(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on H8/300
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_H8_3(char *pArg, TempResult *pResult)
{
  Byte Result;
  tSymbolSize Size;

  if (DecodeRegCore(pArg, &Result, &Size))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = Size;
    pResult->Contents.RegDescr.Reg = Result;
    pResult->Contents.RegDescr.Dissect = DissectReg_H8_3;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static Boolean DecodeAttrPart_H8_3(void)
{
  if (*AttrPart.str.p_str)
  {
    if (strlen(AttrPart.str.p_str) != 1)
    {
      WrStrErrorPos(ErrNum_TooLongAttr, &AttrPart);
      return False;
    }
    if (!DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False))
      return False;
  }
  return True;
}

static void MakeCode_H8_3(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  OpSize = eSymbolSizeUnknown;
  if (AttrPartOpSize[0] != eSymbolSizeUnknown)
    SetOpSize(AttrPartOpSize[0]);

  if (DecodeMoto16Pseudo(OpSize, True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_H8_3(void)
{
  return Memo("REG")
      || Memo("BIT");
}

static void SwitchTo_H8_3(void)
{
  TurnWords = True;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x68;
  NOPCode = 0x0000;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 2;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = (MomCPU <= CPUH8_300) ? 0xffff : 0xffffffl;

  DecodeAttrPart = DecodeAttrPart_H8_3;
  MakeCode = MakeCode_H8_3;
  IsDef = IsDef_H8_3;
  InternSymbol = InternSymbol_H8_3;
  DissectReg = DissectReg_H8_3;
  DissectBit = DissectBit_H8_3;
  QualifyQuote = QualifyQuote_SingleQuoteConstant;
  IntConstModeIBMNoTerm = True;
  SwitchFrom = DeinitFields;
  InitFields();
  onoff_maxmode_add();
  AddMoto16PseudoONOFF(False);

  CPU16 = (MomCPU <= CPUH8_300);
}

void codeh8_3_init(void)
{
  CPUH8_300L = AddCPU("H8/300L"   , SwitchTo_H8_3);
  CPU6413308 = AddCPU("HD6413308" , SwitchTo_H8_3);
  CPUH8_300  = AddCPU("H8/300"    , SwitchTo_H8_3);
  CPU6413309 = AddCPU("HD6413309" , SwitchTo_H8_3);
  CPUH8_300H = AddCPU("H8/300H"   , SwitchTo_H8_3);
}
