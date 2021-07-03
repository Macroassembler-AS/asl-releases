/* codepdk.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS - Target Padauk MCUs                                                   */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "bpemu.h"
#include "strutil.h"
#include "cpulist.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmstructs.h"
#include "codevars.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codepdk.h"

typedef enum
{
  eCoreNone,
  eCorePDK13,
  eCorePDK14,
  eCorePDK15,
  eCorePDK16
} tCPUCore;

enum
{
  eInstCapCOMP_AM_MA = 1 << 0,
  eInstCapNADD_AM_MA = 1 << 1,
  eInstCapMUL = 1 << 2,
  eInstCapXOR_IOA = 1 << 3,
  eInstCap_SWAPM = 1 << 4,
  eInstCapNMOV_AM_MA = 1 << 5,
  eInstCapPUSHW_POPW = 1 << 6,
  eInstCapPMODE = 1 << 7
};

typedef struct
{
  const char *pName;
  Word FlashEndD16;
  Byte RAMEnd, IOAreaEnd;
  tCPUCore Core;
  Word InstCaps;
} tCPUProps;

typedef enum
{
  ModNone = 0,
  ModMem = 1,
  ModIO = 2,
  ModImm = 3,
  ModAcc = 4
} tAdrMode;

#define MModMem (1 << ModMem)
#define MModIO (1 << ModIO)
#define MModImm (1 << ModImm)
#define MModAcc (1 << ModAcc)

static const tCPUProps *pCurrCPUProps;

static IntType CodeAdrIntType,
               CodeWordIntType,
               DataAdrIntType,
               DataBitAdrIntType,
               IOAdrIntType;
static Byte DataMemBits,
            DataBitMemBits,
            CodeMemBits,
            IOMemBits;
static Word AccInstOffs;
static ShortInt OpSize;

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(tStrComp *pArg, Word Mask, IntType MemIntType, Word *pResult)
 * \brief  decode address expression
 * \param  pArg string argument in source
 * \param  Mask bit mask of allowed modes
 * \param  MemIntType integer range for memory addresses
 * \param  pResult resulting address for memory/IO
 * \return decoded address mode or none
 * ------------------------------------------------------------------------ */

static tAdrMode DecodeAdr(tStrComp *pArg, Word Mask, IntType MemIntType, Word *pResult)
{
  tAdrMode AdrMode = ModNone;
  Boolean OK;
  IntType AutoIntType;
  LongInt AddrOrImm;
  int ArgLen;
  tEvalResult EvalResult;

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    AdrMode = ModAcc;
    goto check;
  }

  /* explicit memory: disp[addr], [addr], addr[idx] */

  ArgLen = strlen(pArg->str.p_str);
  if ((ArgLen >= 2) && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    tStrComp Part1, Part2;
    LongInt Num1, Num2;
    Boolean EitherUnknown;
    char *pSep;
    tEvalResult EvalResult1, EvalResult2;

    StrCompShorten(pArg, 1);
    pSep = RQuotPos(pArg->str.p_str, '[');
    if (!pSep)
    {
      WrStrErrorPos(ErrNum_BrackErr, pArg);
      goto check;
    }

    StrCompSplitRef(&Part1, &Part2, pArg, pSep);
    EitherUnknown = False;

    if (*Part1.str.p_str)
    {
      Num1 = EvalStrIntExpressionWithResult(&Part1, MemIntType, &EvalResult1);
      if (!EvalResult1.OK)
        goto check;
      EitherUnknown = EitherUnknown || mFirstPassUnknown(EvalResult1.Flags);
    }
    else
    {
      EvalResult1.OK = False;
      EvalResult1.Flags = eSymbolFlag_None;
      EvalResult1.AddrSpaceMask = 0;
      Num1 = 0;
    }

    Num2 = EvalStrIntExpressionWithResult(&Part2, MemIntType, &EvalResult2);
    if (!EvalResult2.OK)
      goto check;
    EitherUnknown = EitherUnknown || mFirstPassUnknown(EvalResult2.Flags);
    Num1 += Num2;

    if (EitherUnknown)
    {
      Num1 &= SegLimits[SegData];
      if (OpSize == eSymbolSize16Bit)
        Num1 &= ~1;
    }

    if ((OpSize == eSymbolSize16Bit) && (Num1 & 1)) WrStrErrorPos(ErrNum_NotAligned, pArg);
    else if (ChkRange(Num1, 0, SegLimits[SegData]))
    {
      ChkSpace(SegData, EvalResult1.AddrSpaceMask | EvalResult2.AddrSpaceMask);
      AdrMode = ModMem;
      *pResult = Num1;
    }
    goto check;
  }

  /* explicit I/O */

  if (!as_strncasecmp(pArg->str.p_str, "IO", 2) && IsIndirect(pArg->str.p_str + 2))
  {
    *pResult = EvalStrIntExpressionOffsWithResult(pArg, 2, IOAdrIntType, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      AdrMode = ModIO;
    }
    goto check;
  }

  /* explicit immediate */

  if (*pArg->str.p_str == '#')
  {
    *pResult = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK) & 0xff;
    if (OK)
      AdrMode = ModImm;
    goto check;
  }

  /* OK, guess what is meant... */

  AutoIntType = Int8;
  if (Lo(IntTypeDefs[IOAdrIntType].SignAndWidth) > Lo(IntTypeDefs[AutoIntType].SignAndWidth))
    AutoIntType = IOAdrIntType;
  if (Lo(IntTypeDefs[MemIntType].SignAndWidth) > Lo(IntTypeDefs[AutoIntType].SignAndWidth))
    AutoIntType = MemIntType;

  AddrOrImm = EvalStrIntExpressionWithResult(pArg, AutoIntType, &EvalResult);
  if (EvalResult.OK)
  {
    if (EvalResult.AddrSpaceMask == 1 << SegIO)
    {
      if (mFirstPassUnknown(EvalResult.Flags) && (AddrOrImm > (LongInt)SegLimits[SegIO]))
        AddrOrImm &= SegLimits[SegIO];
      if (AddrOrImm > (LongInt)SegLimits[SegIO])
        WrStrErrorPos(ErrNum_OverRange, pArg);
      else
      {
        AdrMode = ModIO;
        *pResult = AddrOrImm;
      }
    }
    else if (EvalResult.AddrSpaceMask == 1 << SegData)
    {
      if (mFirstPassUnknown(EvalResult.Flags) && (AddrOrImm > (LongInt)SegLimits[SegData]))
        AddrOrImm &= SegLimits[SegData];
      if (AddrOrImm > (LongInt)SegLimits[SegData])
        WrStrErrorPos(ErrNum_OverRange, pArg);
      else
      {
        AdrMode = ModMem;
        *pResult = AddrOrImm;
      }
    }
    else
    {
      if (mFirstPassUnknown(EvalResult.Flags) && ((AddrOrImm > 0xff) || (AddrOrImm < -128)))
        AddrOrImm &= 0xff;
      if (!ChkRange(AddrOrImm, -128, 255))
        WrStrErrorPos(ErrNum_OverRange, pArg);
      else
      {
        AdrMode = ModImm;
        *pResult = AddrOrImm & 0xff;
      }
    }
  }

check:
  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    AdrMode = ModNone;
  }
  return AdrMode;
}

static Boolean CoreMask(Word Mask)
{
  return !!(Mask & (1 << pCurrCPUProps->Core));
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits in symbol table:
 * bits 0..2: bit position
 * bits 3...n: I/O or memory address
 * bit 15: 1 for I/O, 0 for memory
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
  return EvalStrIntExpression(pArg, UInt3, pOK);
}

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, Word Address, Boolean IsIO)
 * \brief  build the compact internal representation of a bit symbol
 * \param  BitPos bit position in word
 * \param  Address register address
 * \param  IsIO true for bit in I/O space
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, Word Address, Boolean IsIO)
{
  return (BitPos & 7)
       | (((LongWord)Address & 0x1ff) << 3)
       | (IsIO ? 0x8000u : 0x0000);
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos, Boolean *pIO)
 * \brief  transform compact representation of bit symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress I/O or memory address
 * \param  pBitPos bit position
 * \param  pIO 1 for I/O, 0 for memory address
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos, Boolean *pIO)
{
  *pAddress = (BitSymbol >> 3) & 0x1ff;
  *pBitPos = BitSymbol & 7;
  *pIO = !!(BitSymbol & 0x8000u);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, tStrComp *pAddrArg, const tStrComp *pBitArg)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pAddrArg memory/IO address argument
 * \param  pBitArg bit argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, tStrComp *pAddrArg, const tStrComp *pBitArg)
{
  Boolean OK;
  Word Address;
  Byte BitPos;
  Boolean IsIO;

  BitPos = EvalBitPosition(pBitArg, &OK);
  if (!OK)
    return False;

  switch (DecodeAdr(pAddrArg, MModMem | MModIO, DataBitAdrIntType, &Address))
  {
    case ModMem:
      IsIO = False;
      break;
    case ModIO:
      IsIO = True;
      break;
    default:
      return False;
  }

  *pResult = AssembleBitSymbol(BitPos, Address, IsIO);

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
    char *pSep = strchr(ArgStr[Start].str.p_str, '.');

    if (pSep)
    {
      tStrComp AddressComp, PosComp;

      StrCompSplitRef(&AddressComp, &PosComp, &ArgStr[Start], pSep);
      return DecodeBitArg2(pResult, &AddressComp, &PosComp);
    }
    else
    {
      tEvalResult EvalResult;

      *pResult = EvalStrIntExpressionWithResult(&ArgStr[Start], UInt16, &EvalResult);
      if (EvalResult.OK)
        ChkSpace(SegBData, EvalResult.AddrSpaceMask);
      return EvalResult.OK;
    }
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
 * \fn     DissectBit_Padauk(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_Padauk(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  Word Address;
  Boolean IsIO;

  DissectBitSymbol(Inp, &Address, &BitPos, &IsIO);

  if (IsIO)
    as_snprintf(pDest, DestSize, "IO(0x%x).%u", (unsigned)Address, (unsigned)BitPos);
  else
    as_snprintf(pDest, DestSize, "[0x%x].%u", (unsigned)Address, (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandBit_Padauk(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandBit_Padauk(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
{
  LongWord Address = Base + pStructElem->Offset;
  Boolean IsIO = (ActPC == SegIO);

  if (!ChkRange(Address, 0, IntTypeDefs[IsIO ? IOAdrIntType : DataBitAdrIntType].Max)
   || !ChkRange(pStructElem->BitPos, 0, 7))
    return;

  PushLocHandle(-1);
  EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, Address, IsIO), SegBData, False);
  PopLocHandle();
  /* TODO: MakeUseList? */
}

/*!------------------------------------------------------------------------
 * \fn     ImmCode(Word Code13)
 * \brief  translate PDK13 arithmetic op code to actual code code for immediate src
 * \param  Code13 PDK13 code
 * \return Immediate Opcode
 * ------------------------------------------------------------------------ */

static Word ImmCode(Word Code13)
{
  switch (pCurrCPUProps->Core)
  {
    case eCorePDK14:
      return 0x20 | ((Code13 & 0x10) >> 1) | (Code13 & 7);
    case eCorePDK15:
      return 0x40 | Code13;
    case eCorePDK16:
      return 0x08 | Code13;
    default:
      return Code13;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AMCode(Word Code13, Boolean AccDest)
 * \brief  translate PDK13 arithmetic op code to actual code code for Acc<->[M]
 * \param  Code13 PDK13 code
 * \param  AccDest: true if A<-M
 * \return Opcode
 * ------------------------------------------------------------------------ */

static Word AMCode(Word Code13, Boolean AccDest)
{
  switch (pCurrCPUProps->Core)
  {
    case eCorePDK16:
      return (Code13 << 1) | (AccDest ? 1 : 0);
    default:
      return Code13 | (AccDest ? 8 : 0);
  }
}

/*!------------------------------------------------------------------------
 * \fn     MCode(Word Code13)
 * \brief  translate PDK13 arithmetic op code to actual code code for [M]
 * \param  Code13 PDK13 code
 * \return Opcode
 * ------------------------------------------------------------------------ */

static Word MCode(Word Code13)
{
  switch (pCurrCPUProps->Core)
  {
    case eCorePDK16:
      return 0x30 | Code13;
    default:
      return 0x20 | Code13;
  }
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    WAsmCode[CodeLen++] = Code;
}

static void DecodeACC(Word Code)
{
  Word Address;

  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModAcc, DataAdrIntType, &Address))
    WAsmCode[CodeLen++] = Code;
}

static void DecodeSWAP(Word Code)
{
  Word Address;

  if (ChkArgCnt(1, 1))
    switch (DecodeAdr(&ArgStr[1], MModAcc | ((pCurrCPUProps->InstCaps & eInstCap_SWAPM) ? MModMem : 0), DataAdrIntType, &Address))
    {
      case ModAcc:
        WAsmCode[CodeLen++] = Code;
        break;
      case ModMem:
        WAsmCode[CodeLen++] = ((pCurrCPUProps->Core == eCorePDK16 ? 0x3e: 0x0a) << DataMemBits) | Address;
        break;
      default:
        break;
    }
}

static void DecodeADDC_SUBC(Word Code)
{
  Word Address;

  switch (ArgCnt)
  {
    case 1:
      switch (DecodeAdr(&ArgStr[1], MModAcc | MModMem, DataAdrIntType, &Address))
      {
        case ModAcc:
          WAsmCode[CodeLen++] = (AccInstOffs + 0) | Code;
          break;
        case ModMem:
          WAsmCode[CodeLen++] = (MCode(Code) << DataMemBits) | Address;
          break;
        default:
          break;
      }
      break;
    case 2:
      if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      {
        if (DecodeAdr(&ArgStr[2], MModMem, DataAdrIntType, &Address))
          WAsmCode[CodeLen++] = (AMCode(0x12 + Code, True) << DataMemBits) | Address;
      }
      else if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
      {
        if (DecodeAdr(&ArgStr[1], MModMem, DataAdrIntType, &Address))
          WAsmCode[CodeLen++] = (AMCode(0x12 + Code, False) << DataMemBits) | Address;
      }
      break;
    default:
      (void)ChkArgCnt(1, 2);
  }
}

static void DecodeACCOrM(Word Code)
{
  Word Address;

  if (ChkArgCnt(1, 1))
    switch (DecodeAdr(&ArgStr[1], MModMem | MModAcc, DataAdrIntType, &Address))
    {
      case ModAcc:
        WAsmCode[CodeLen++] = AccInstOffs + Code;
        break;
      case ModMem:
        WAsmCode[CodeLen++] = (MCode(Code) << DataMemBits) | Address;
        break;
      default:
        break;
    }
}

static void DecodeDELAY(Word Code)
{
  Word Address;

  if (ChkArgCnt(1, 1))
    switch (DecodeAdr(&ArgStr[1], MModMem | MModAcc | MModImm, DataAdrIntType, &Address))
    {
      case ModAcc:
        WAsmCode[CodeLen++] = AccInstOffs + Code;
        break;
      case ModMem:
        WAsmCode[CodeLen++] = (MCode(Code) << DataMemBits) | Address;
        break;
      case ModImm:
        WAsmCode[CodeLen++] = 0x0e00 | Address;
        break;
      default:
        break;
    }
}

static void DecodeM(Word Code)
{
  Word Address;

  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModMem, DataAdrIntType, &Address))
    WAsmCode[CodeLen++] = (MCode(Code) << DataMemBits) | Address;
}

static void DecodeRET(Word Code)
{
  Word Value;

  UNUSED(Code);

  switch (ArgCnt)
  {
    case 0:
      WAsmCode[CodeLen++] = (AccInstOffs == 0x60) ? 0x7a : 0x3a;
      break;
    case 1:
      if (DecodeAdr(&ArgStr[1], MModImm, DataAdrIntType, &Value))
        WAsmCode[CodeLen++] = Code | Value;
      break;
    default:
      (void)ChkArgCnt(0, 1);
  }
}

static void DecodeXOR(Word Code)
{
  Word DestAddress, SrcAddress,
       OpCode = Lo(Code),
       IOCode = Hi(Code);

  if (ChkArgCnt(2,2))
    switch (DecodeAdr(&ArgStr[1], MModAcc | MModMem | ((pCurrCPUProps->InstCaps & eInstCapXOR_IOA) ? MModIO : 0), DataAdrIntType, &DestAddress))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], MModMem | MModImm | ((pCurrCPUProps->Core >= eCorePDK16) ? MModIO : 0), DataAdrIntType, &SrcAddress))
        {
          case ModMem:
            WAsmCode[CodeLen++] = (AMCode(OpCode, True) << DataMemBits) | SrcAddress;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = (ImmCode(OpCode) << 8) | SrcAddress;
            break;
          case ModIO:
            WAsmCode[CodeLen++] = ((IOCode + 1) << IOMemBits) | SrcAddress;
            break;
          default:
            break;
        }
        break;
      case ModMem:
        if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (AMCode(OpCode, False) << DataMemBits) | DestAddress;
        break;
      case ModIO:
        if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (IOCode << IOMemBits) | DestAddress;
        break;
      default:
        break;
    }
}

static void DecodeAccToM(Word Code)
{
  Word DestAddress, SrcAddress;

  if (ChkArgCnt(2, 2))
    switch (DecodeAdr(&ArgStr[1], MModAcc | MModMem, DataAdrIntType, &DestAddress))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], MModMem | MModImm, DataAdrIntType, &SrcAddress))
        {
          case ModMem:
            WAsmCode[CodeLen++] = (AMCode(Code, True) << DataMemBits) | SrcAddress;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = (ImmCode(Code) << 8) | SrcAddress;
            break;
          default:
            break;
        }
        break;
      case ModMem:
        if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (AMCode(Code, False) << DataMemBits) | DestAddress;
        break;
      default:
        break;
    }
}

static void DecodeCOMP_NADD_NMOV(Word Code)
{
  Word DestAddress, SrcAddress;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
    switch (DecodeAdr(&ArgStr[1], MModAcc | MModMem, DataAdrIntType, &DestAddress))
    {
      case ModAcc:
        if (DecodeAdr(&ArgStr[2], MModMem, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (((Code << 1) + 0) << DataMemBits) | SrcAddress;
        break;
      case ModMem:
        if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (((Code << 1) + 1) << DataMemBits) | DestAddress;
        break;
      default:
        break;
    }
}

static void DecodeMOV(Word Code)
{
  Word DestAddress, SrcAddress,
       OpCode = Lo(Code),
       IOCode = Hi(Code) & 15;

  if (ChkArgCnt(2,2))
    switch (DecodeAdr(&ArgStr[1], MModAcc | MModMem | MModIO, DataAdrIntType, &DestAddress))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], MModMem | MModImm | MModIO, DataAdrIntType, &SrcAddress))
        {
          case ModMem:
            WAsmCode[CodeLen++] = (AMCode(OpCode, True) << DataMemBits) | SrcAddress;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = (ImmCode(OpCode) << 8) | SrcAddress;
            break;
          case ModIO:
            WAsmCode[CodeLen++] = ((IOCode + 1) << IOMemBits) | SrcAddress;
            break;
          default:
            break;
        }
        break;
      case ModMem:
        if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (AMCode(OpCode, False) << DataMemBits) | DestAddress;
        break;
      case ModIO:
        if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
          WAsmCode[CodeLen++] = (IOCode << IOMemBits) | DestAddress;
        break;
      default:
        break;
    }
}

static void DecodeGOTO_CALL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Address = EvalStrIntExpressionWithResult(&ArgStr[1], CodeAdrIntType, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      WAsmCode[CodeLen++] = (Code << CodeMemBits) | Address;
    }
  }
}

static void DecodeCnEQSN(Word Code)
{
  Word SrcAddress, DestAddress;

  if (ChkArgCnt(2, 2))
   switch (DecodeAdr(&ArgStr[1], MModAcc | ((pCurrCPUProps->Core >= eCorePDK16) ? MModMem : 0), DataAdrIntType, &DestAddress))
   {
     case ModAcc:
       switch (DecodeAdr(&ArgStr[2], MModMem | MModImm, DataAdrIntType, &SrcAddress))
       {
         case ModMem:
           WAsmCode[CodeLen++] = (Code << DataMemBits) | SrcAddress;
           break;
         case ModImm:
           WAsmCode[CodeLen++] = (ImmCode(0x12 + (Code & 1)) << 8) | SrcAddress;
           break;
         default:
           break;
       }
       break;
     case ModMem:
       if (DecodeAdr(&ArgStr[2], MModAcc, DataAdrIntType, &SrcAddress))
         WAsmCode[CodeLen++] = ((Code ^ 1) << DataMemBits) | DestAddress;
       break;
     default:
       break;
   }
}

static void DecodeEvenAddr(tStrComp *pArg, Word Code1, Word Code2)
{
  Word Address;

  OpSize = eSymbolSize16Bit;

  if (DecodeAdr(pArg, MModMem, (pCurrCPUProps->Core == eCorePDK13) ? UInt5 : DataAdrIntType, &Address))
    WAsmCode[CodeLen++] = (Code1 << ((pCurrCPUProps->Core == eCorePDK13) ? 5 : DataMemBits)) | Code2 | Address;
}

static void DecodeSTT16_LDT16(Word Code)
{
  if (ChkArgCnt(1, 1))
    DecodeEvenAddr(&ArgStr[1], Hi(Code), Lo(Code));
}

static void DecodeLDTAB(Word Code)
{
  if (ChkArgCnt(1, 1))
    DecodeEvenAddr(&ArgStr[1], 0x05, Code);
}

static void DecodeIDXM(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      DecodeEvenAddr(&ArgStr[2], Code, 1);
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
      DecodeEvenAddr(&ArgStr[1], Code, 0);
  }
}

static void DecodeBitOp(Word Code)
{
  LongWord BitSpec;
  Word OpCode = Lo(Code);

  if (DecodeBitArg(&BitSpec, 1, ArgCnt))
  {
    Word Address;
    Byte BitPos;
    Boolean IsIO;

    DissectBitSymbol(BitSpec, &Address, &BitPos, &IsIO);
    if (IsIO)
    {
      if (pCurrCPUProps->Core == eCorePDK16)
        WAsmCode[CodeLen++] = 0x2000 | (OpCode << (3 + IOMemBits)) | (BitPos << IOMemBits) | Address;
      else
        WAsmCode[CodeLen++] = (3 << (5 + IOMemBits)) | (OpCode << (3 + IOMemBits)) | (BitPos << IOMemBits) | Address;
    }
    else if (pCurrCPUProps->Core == eCorePDK13)
      WAsmCode[CodeLen++] = (Code & 0xff00u) | ((OpCode & 2) << (4 - 1 + DataBitMemBits)) | (BitPos << (1 + DataBitMemBits)) | ((Code & 1) << DataBitMemBits) | Address;
    else
      WAsmCode[CodeLen++] = (Code & 0xff00u) | (OpCode << (3 + DataBitMemBits)) | (BitPos << DataBitMemBits) | Address;
  }
}

static void DecodeBitOpIO(Word Code)
{
  LongWord BitSpec;

  if (DecodeBitArg(&BitSpec, 1, ArgCnt))
  {
    Word Address;
    Byte BitPos;
    Boolean IsIO;

    DissectBitSymbol(BitSpec, &Address, &BitPos, &IsIO);
    if (IsIO)
      WAsmCode[CodeLen++] = (Code << (3 + IOMemBits)) | (BitPos << IOMemBits) | Address;
    else
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeConstU5(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Num = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), UInt5, &OK);

    if (OK)
      WAsmCode[CodeLen++] = Code | (Num & 0x1f);
  }
}

static void DecodeConstU4(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Num = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), UInt4, &OK);

    if (OK)
      WAsmCode[CodeLen++] = Code | (Num & 0x0f);
  }
}

static void DecodeBIT(Word Code)
{
  UNUSED(Code);

  /* if in structure definition, add special element to structure */

  if (ActPC == StructSeg)
  {
    Boolean OK;
    Byte BitPos;
    PStructElem pElement;
    tStrComp BitComp, *pBitComp, AddrComp, *pAddrComp;

    switch (ArgCnt)
    {
      case 1:
      {
        char *pSep = strchr(ArgStr[1].str.p_str, '.');
        if (!pSep)
          goto fail;
        StrCompSplitRef(&AddrComp, &BitComp, &ArgStr[1], pSep);
        pBitComp = &BitComp;
        pAddrComp = &AddrComp;
        break;
      }
      case 2:
        pAddrComp = &ArgStr[1];
        pBitComp = &ArgStr[2];
        break;
      default:
      fail:
        WrError(ErrNum_WrongArgCnt);
        return;
    }

    BitPos = EvalBitPosition(pBitComp, &OK);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    pElement->pRefElemName = as_strdup(pAddrComp->str.p_str);
    pElement->OpSize = eSymbolSize8Bit;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandBit_Padauk;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    LongWord BitSpec;

    if (DecodeBitArg(&BitSpec, 1, ArgCnt))
    {
      *ListLine = '=';
      DissectBit_Padauk(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

static void DecodeDATA_Padauk(Word Code)
{
  UNUSED(Code);

  DecodeDATA(CodeWordIntType, DataAdrIntType);
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

/*---------------------------------------------------------------------------*/

static void InitFields(void)
{
  Word Base, MovIOCode, RETCode, SWAPCCode, BitOpCode, XORIOCode, STT16_LDT16Code, IDXMCode, CEQSNCode, CNEQSNCode;
  InstTable = CreateInstTable(203);

  switch (pCurrCPUProps->Core)
  {
    case eCorePDK13:
      MovIOCode = 0x0400;
      RETCode = 0x0100;
      SWAPCCode = 0;
      BitOpCode = 0x0200;
      XORIOCode = 0x0300;
      STT16_LDT16Code = 0x0600;
      IDXMCode = 7;
      CEQSNCode = 0x2e;
      CNEQSNCode = 0x2f;
      break;
    case eCorePDK14:
      MovIOCode = 0x0600;
      RETCode = 0x0200;
      SWAPCCode = 2;
      BitOpCode = 0x2000;
      XORIOCode = 0x0300;
      STT16_LDT16Code = 0x0600;
      IDXMCode = 7;
      CEQSNCode = 0x2e;
      CNEQSNCode = 0x2f;
      break;
    case eCorePDK15:
      MovIOCode = 0x0200;
      RETCode = 0x0200;
      SWAPCCode = 0x17;
      BitOpCode = 0x4000;
      XORIOCode = 0x0100;
      STT16_LDT16Code = 0x0600;
      IDXMCode = 7;
      CEQSNCode = 0x2e;
      CNEQSNCode = 0x2f;
      break;
    case eCorePDK16:
      MovIOCode = 0x0200;
      RETCode = 0x0f00;
      SWAPCCode = 0x17;
      BitOpCode = 0x8000;
      XORIOCode = 0x4000;
      STT16_LDT16Code = 0x0100;
      IDXMCode = 4;
      CNEQSNCode = 0x0b;
      CEQSNCode = 0x1c;
      break;
    default:
      WrError(ErrNum_InternalError);
      exit(255);
  }

  AddInstTable(InstTable, "NOP", NOPCode, DecodeFixed);
  if (pCurrCPUProps->Core <= eCorePDK14)
  {
    AddInstTable(InstTable, "LDSPTL", 0x0006, DecodeFixed);
    AddInstTable(InstTable, "LDSPTH", 0x0007, DecodeFixed);
  }
  else
  {
    AddInstTable(InstTable, "LDTABL", 0, DecodeLDTAB);
    AddInstTable(InstTable, "LDTABH", 1, DecodeLDTAB);
  }
  Base = (AccInstOffs == 0x60) ? 0x70 : 0x30;
  AddInstTable(InstTable, "WDRESET", Base + 0x00, DecodeFixed);
  AddInstTable(InstTable, "PUSHAF" , Base + 0x02, DecodeFixed);
  AddInstTable(InstTable, "POPAF"  , Base + 0x03, DecodeFixed);
  AddInstTable(InstTable, "RESET"  , Base + 0x05, DecodeFixed);
  AddInstTable(InstTable, "STOPSYS", Base + 0x06, DecodeFixed);
  AddInstTable(InstTable, "STOPEXE", Base + 0x07, DecodeFixed);
  AddInstTable(InstTable, "ENGINT" , Base + 0x08, DecodeFixed);
  AddInstTable(InstTable, "DISGINT", Base + 0x09, DecodeFixed);
  AddInstTable(InstTable, "RETI"   , Base + 0x0b, DecodeFixed);
  if (pCurrCPUProps->InstCaps & eInstCapMUL)
    AddInstTable(InstTable, "MUL"    , Base + 0x0c, DecodeFixed);

  AddInstTable(InstTable, "RET"    , RETCode, DecodeRET);

  AddInstTable(InstTable, "ADDC", 0, DecodeADDC_SUBC);
  AddInstTable(InstTable, "SUBC", 1, DecodeADDC_SUBC);
  AddInstTable(InstTable, "IZSN", 2, DecodeACCOrM);
  AddInstTable(InstTable, "DZSN", 3, DecodeACCOrM);
  AddInstTable(InstTable, "NOT" , 8, DecodeACCOrM);
  AddInstTable(InstTable, "NEG" , 9, DecodeACCOrM);
  AddInstTable(InstTable, "SR"  ,10, DecodeACCOrM);
  AddInstTable(InstTable, "SL"  ,11, DecodeACCOrM);
  AddInstTable(InstTable, "SRC" ,12, DecodeACCOrM);
  AddInstTable(InstTable, "SLC" ,13, DecodeACCOrM);
  if (pCurrCPUProps->Core >= eCorePDK16)
    AddInstTable(InstTable, "DELAY", 15, DecodeDELAY);

  AddInstTable(InstTable, "PCADD", AccInstOffs +  7, DecodeACC);
  AddInstTable(InstTable, "SWAP",  AccInstOffs + 14, DecodeSWAP);

  AddInstTable(InstTable, "INC"  , 4, DecodeM);
  AddInstTable(InstTable, "DEC"  , 5, DecodeM);
  AddInstTable(InstTable, "CLEAR", 6, DecodeM);
  AddInstTable(InstTable, "XCH"  , 7, DecodeM);

  if (pCurrCPUProps->InstCaps & eInstCapNADD_AM_MA)
    AddInstTable(InstTable, "COMP", pCurrCPUProps->Core >= eCorePDK16 ? 0x0f : 0x06, DecodeCOMP_NADD_NMOV);
  if (pCurrCPUProps->InstCaps & eInstCapCOMP_AM_MA)
    AddInstTable(InstTable, "NADD", pCurrCPUProps->Core >= eCorePDK16 ? 0x0d : 0x07, DecodeCOMP_NADD_NMOV);
  if (pCurrCPUProps->InstCaps & eInstCapNMOV_AM_MA)
    AddInstTable(InstTable, "NMOV", pCurrCPUProps->Core >= eCorePDK16 ? 0x0c : 0x04, DecodeCOMP_NADD_NMOV);
  AddInstTable(InstTable, "ADD", 0x10, DecodeAccToM);
  AddInstTable(InstTable, "SUB", 0x11, DecodeAccToM);
  AddInstTable(InstTable, "AND", 0x14, DecodeAccToM);
  AddInstTable(InstTable, "OR" , 0x15, DecodeAccToM);
  AddInstTable(InstTable, "XOR", XORIOCode | 0x16, DecodeXOR);
  AddInstTable(InstTable, "MOV", MovIOCode | 0x17, DecodeMOV);
  AddInstTable(InstTable, "CEQSN", CEQSNCode, DecodeCnEQSN);
  if (pCurrCPUProps->Core >= eCorePDK14)
    AddInstTable(InstTable, "CNEQSN", CNEQSNCode | 1, DecodeCnEQSN);

  AddInstTable(InstTable, "STT16", STT16_LDT16Code | 0, DecodeSTT16_LDT16);
  AddInstTable(InstTable, "LDT16", STT16_LDT16Code | 1, DecodeSTT16_LDT16);
  if (pCurrCPUProps->InstCaps & eInstCapPUSHW_POPW)
  {
    AddInstTable(InstTable, "POPW", 0x0200, DecodeSTT16_LDT16);
    AddInstTable(InstTable, "PUSHW", 0x0201, DecodeSTT16_LDT16);
  }
  if (pCurrCPUProps->Core >= eCorePDK16)
  {
    AddInstTable(InstTable, "IGOTO", 0x0300, DecodeSTT16_LDT16);
    AddInstTable(InstTable, "ICALL", 0x0301, DecodeSTT16_LDT16);
  }
  AddInstTable(InstTable, "IDXM" , IDXMCode, DecodeIDXM);

  AddInstTable(InstTable, "GOTO", 6, DecodeGOTO_CALL);
  AddInstTable(InstTable, "CALL", 7, DecodeGOTO_CALL);

  AddInstTable(InstTable, "T0SN", BitOpCode | 0, DecodeBitOp);
  AddInstTable(InstTable, "T1SN", BitOpCode | 1, DecodeBitOp);
  AddInstTable(InstTable, "SET0", BitOpCode | 2, DecodeBitOp);
  AddInstTable(InstTable, "SET1", BitOpCode | 3, DecodeBitOp);
  if (pCurrCPUProps->Core >= eCorePDK16)
  {
    AddInstTable(InstTable, "TOG", 0x14, DecodeBitOpIO);
    AddInstTable(InstTable, "WAIT0", 0x15, DecodeBitOpIO);
    AddInstTable(InstTable, "WAIT1", 0x16, DecodeBitOpIO);
  }
  if (SWAPCCode)
    AddInstTable(InstTable, "SWAPC", SWAPCCode, DecodeBitOpIO);

  if (pCurrCPUProps->InstCaps & eInstCapPMODE)
    AddInstTable(InstTable, "PMODE", 0x0040, DecodeConstU5);
  if (pCurrCPUProps->InstCaps & eInstCapPUSHW_POPW)
  {
    AddInstTable(InstTable, "POPWPC" , 0x0060, DecodeConstU4);
    AddInstTable(InstTable, "PUSHWPC", 0x0070, DecodeConstU4);
  }

  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_Padauk);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "RES", 0, DecodeRES);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_Padauk(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = eSymbolSize8Bit;

  if (Memo("")) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_Padauk(void)
{
  return Memo("BIT")
      || Memo("SFR");
}

static void SwitchFrom_Padauk(void)
{
  DeinitFields();
}

static void SwitchTo_Padauk(void *pUser)
{
  static const char CommentLeadIn[] = { ';', '\0', '/', '/', '\0', '\0' };

  pCurrCPUProps = (const tCPUProps*)pUser;

  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "*";
  HeaderID = 0x3b;
  NOPCode = 0x0000;
  DivideChars = ",";
  pCommentLeadIn = CommentLeadIn;
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0; SegLimits[SegData] = pCurrCPUProps->RAMEnd;
  Grans[SegIO  ] = 1; ListGrans[SegIO  ] = 1; SegInits[SegIO  ] = 0;  SegLimits[SegIO] = pCurrCPUProps->IOAreaEnd;

  SegLimits[SegCode] = ((LongWord)pCurrCPUProps->FlashEndD16) << 4 | 0xf;
  CodeAdrIntType = GetSmallestUIntType(SegLimits[SegCode]);
  CodeWordIntType = GetUIntTypeByBits(13 + (pCurrCPUProps->Core - eCorePDK13));
  DataAdrIntType = GetSmallestUIntType(SegLimits[SegData]);
  DataBitAdrIntType = (pCurrCPUProps->Core == eCorePDK13) ? UInt4 : DataAdrIntType;
  IOAdrIntType   = GetSmallestUIntType(SegLimits[SegIO  ]);

#if 0
  fprintf(stderr, "Data 0x%lx DataBit 0x%lx Code 0x%lx IO 0x%lx\n",
          IntTypeDefs[DataAdrIntType].Max,
          IntTypeDefs[DataBitAdrIntType].Max,
          IntTypeDefs[CodeAdrIntType].Max,
          IntTypeDefs[IOAdrIntType].Max);
#endif

  switch (pCurrCPUProps->Core)
  {
    case eCorePDK13:
      DataBitAdrIntType = UInt4;
      IOMemBits = 5;
      break;
    case eCorePDK14:
      DataBitAdrIntType = UInt6;
      IOMemBits = 6;
      break;
    case eCorePDK15:
      DataBitAdrIntType = UInt7;
      IOMemBits = 7;
      break;
    case eCorePDK16:
      DataBitAdrIntType = UInt9;
      IOMemBits = 6;
      break;
    default:
      break;
  }
  DataBitMemBits = Lo(IntTypeDefs[DataBitAdrIntType].SignAndWidth);

  DataMemBits = 6 + (pCurrCPUProps->Core - eCorePDK13);
  CodeMemBits = 10 + (pCurrCPUProps->Core - eCorePDK13);

  AccInstOffs = CoreMask((1 << eCorePDK14) | (1 << eCorePDK15)) ? 0x0060 : 0x0010;

  MakeCode = MakeCode_Padauk;
  IsDef = IsDef_Padauk;
  SwitchFrom = SwitchFrom_Padauk;
  DissectBit = DissectBit_Padauk;
  InitFields();
}

static const tCPUProps CPUProps[] =
{
  { "PMC150"  , 0x3f, 0x3f, 0x1f, eCorePDK13,                                                         eInstCapXOR_IOA                                                                            },
  { "PMS150"  , 0x3f, 0x3f, 0x1f, eCorePDK13,                                                         eInstCapXOR_IOA                                                                            },
  { "PFS154"  , 0x7f, 0x7f, 0x3f, eCorePDK14,                                                         eInstCapXOR_IOA                                                                            },
  { "PMC131"  , 0x5f, 0x57, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA                                                                                              },
  { "PMS130"  , 0x5f, 0x57, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA                                                                                              },
  { "PMS131"  , 0x5f, 0x57, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA                                                                                              },
  { "PMS132"  , 0x7f, 0x7f, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA                                                                            },
  { "PMS132B" , 0x7f, 0x7f, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA                                                                            },
  { "PMS152"  , 0x4f, 0x4f, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA                                                                            },
  { "PMS154B" , 0x7f, 0x7f, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA                                                                            },
  { "PMS154C" , 0x7f, 0x7f, 0x3f, eCorePDK14, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA                                                                            },
  { "PFS173"  , 0xbf, 0xff, 0x7f, eCorePDK15, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA                                                                                              },
  { "PMS133"  , 0xbf, 0xff, 0x7f, eCorePDK15, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMS134"  , 0xff, 0xff, 0x7f, eCorePDK15, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "DF69"    , 0xff, 0xcf, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM | eInstCapPUSHW_POPW | eInstCapPMODE },
  { "MCS11"   , 0xff, 0xcf, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM | eInstCapPUSHW_POPW | eInstCapPMODE },
  { "PMC232"  , 0x7f, 0x9f, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMC234"  , 0xff, 0xcf, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMC251"  , 0x3f, 0x3f, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMC271"  , 0x3f, 0x3f, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMC884"  , 0xff, 0xff, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA | eInstCapMUL | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM | eInstCapPUSHW_POPW | eInstCapPMODE },
  { "PMS232"  , 0x7f, 0x9f, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMS234"  , 0xff, 0xcf, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { "PMS271"  , 0x3f, 0x3f, 0x3f, eCorePDK16, eInstCapCOMP_AM_MA               | eInstCapNADD_AM_MA | eInstCapXOR_IOA | eInstCapNMOV_AM_MA | eInstCap_SWAPM                                      },
  { NULL      , 0x00, 0x00, 0x00, eCoreNone , 0 }
};

void codepdk_init(void)
{
  const tCPUProps *pProp;

  for (pProp = CPUProps; pProp->pName; pProp++)
    (void)AddCPUUser(pProp->pName, SwitchTo_Padauk, (void*)pProp, NULL);
}
