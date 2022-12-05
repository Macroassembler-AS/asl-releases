/* codekenbak.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator KENBAK(-1)                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include "stdinc.h"
#include "strutil.h"
#include "intformat.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmstructs.h"
#include "headids.h"

#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"

#include "codekenbak.h"

/*---------------------------------------------------------------------------*/

/* do not change enum values, they match the machine codings: */

enum
{
  ModNone = 0,
  ModImm = 3,
  ModMemory = 4,
  ModIndirect = 5,
  ModIndexed = 6,
  ModIndirectIndexed = 7
};

#define MModImm (1 << ModImm)
#define MModMemory (1 << ModMemory)
#define MModIndirect (1 << ModIndirect)
#define MModIndexed (1 << ModIndexed)
#define MModIndirectIndexed (1 << ModIndirectIndexed)

#define MModAll (MModImm | MModMemory | MModIndirect | MModIndexed | MModIndirectIndexed)

typedef struct
{
  Byte Mode, Val;
} tAdrData;

static const char Regs[5] = "ABXP";

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Byte *pResult)
 * \brief  check whether argument describes a CPU register
 * \param  pArg argument
 * \param  pValue resulting register # if yes
 * \return true if argument is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Byte *pResult)
{
  const char *pPos;

  if (strlen(pArg) != 1)
    return False;
  pPos = strchr(Regs, as_toupper(*pArg));
  if (pPos)
    *pResult = pPos - Regs;
  return !!pPos;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_KENBAK(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - KENBAK variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_KENBAK(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      if (Value <= 3)
      {
        as_snprintf(pDest, DestSize, "%c", Regs[Value]);
        break;
      }
      /* else fall-thru */
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Byte *pResult, Word RegMask, Boolean MustBeReg)
 * \brief  check whether argument is CPU register, including register aliases
 * \param  pArg source code argument
 * \param  pResult register # if it's a register
 * \param  RegMask bit mask of allowed registers
 * \param  MustBeReg expecting register anyway
 * \return eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Byte *pResult, Word RegMask, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult))
    RegEvalResult = eIsReg;
  else
  {
    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize8Bit, MustBeReg);
    *pResult = RegDescr.Reg;
  }

  if ((RegEvalResult == eIsReg)
   && !(1 & (RegMask >> *pResult)))
  {
    WrStrErrorPos(ErrNum_InvReg, pArg);
    RegEvalResult = eRegAbort;
  }

  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegWithMemOpt(const tStrComp *pArg, Byte *pResult, Word RegMask)
 * \brief  check whether argument is CPU register, register alias, or memory location representing a register
 * \param  pArg source code argument
 * \param  pResult register # if it's a register
 * \param  RegMask bit mask of allowed registers
 * \return True if argument can be interpreted as register in some way
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegWithMemOpt(const tStrComp *pArg, Byte *pResult, Word RegMask)
{
  switch (DecodeReg(pArg, pResult, RegMask, False))
  {
    case eIsReg:
      return True;
    case eRegAbort:
      return False;
    default:
    {
      tEvalResult EvalResult;

      *pResult = EvalStrIntExpressionWithResult(pArg, UInt8, &EvalResult);
      if (!EvalResult.OK)
        return False;
      if (!(1 & (RegMask >> *pResult)))
      {
        WrStrErrorPos(ErrNum_InvReg, pArg);
        return False;
      }
      return True;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, tAdrData *pAdrData, Word ModeMask)
 * \brief  decode address expression
 * \param  pArg1 1st argument
 * \param  pArg1 optional 2nd argument (may be NULL)
 * \param  pAdrData result buffer
 * \return true if successfully decoded
 * ------------------------------------------------------------------------ */

static Boolean ChkMode(Word ModeMask, tAdrData *pAdrData)
{
  return !!((ModeMask >> pAdrData->Mode) & 1);
}

static Boolean DecodeAdr(tStrComp *pArg1, tStrComp *pArg2, tAdrData *pAdrData, Word ModeMask)
{
  tStrComp Arg;

  pAdrData->Mode = 0;

  if (pArg2)
  {
    Byte IndexReg;

    if (DecodeReg(pArg2, &IndexReg, 4, True) != eIsReg)
      return False;
    pAdrData->Mode |= 2;
  }
  else
  {
    if (*pArg1->str.p_str == '#')
    {
      Boolean OK;

      pAdrData->Mode = 3;
      pAdrData->Val = EvalStrIntExpressionOffs(pArg1, 1, Int8, &OK);
      return OK && ChkMode(ModeMask, pAdrData);
    }
  }

  if ((ModeMask & (MModIndirect | MModIndirectIndexed)) && IsIndirect(pArg1->str.p_str))
  {
    StrCompShorten(pArg1, 1);
    StrCompRefRight(&Arg, pArg1, 1);
    pAdrData->Mode |= 1;
  }
  else
    StrCompRefRight(&Arg, pArg1, 0);
  switch (DecodeReg(&Arg, &pAdrData->Val, 15, False))
  {
    case eIsReg:
      pAdrData->Mode |= 4;
      return ChkMode(ModeMask, pAdrData);
    case eRegAbort:
      return False;
    default:
    {
      Boolean OK;

      pAdrData->Val = EvalStrIntExpression(&Arg, UInt8, &OK);
      pAdrData->Mode |= 4;
      return OK && ChkMode(ModeMask, pAdrData);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAddrKeyword(const char *pKeyword, Byte *pMode)
 * \brief  deduce addressing mode form key word
 * \param  pKeyword keyword from source
 * \param  pMode resulting mode if yes
 * \return True if valid key word
 * ------------------------------------------------------------------------ */

static Boolean DecodeAddrKeyword(const char *pKeyword, Byte *pMode)
{
  if (!as_strcasecmp(pKeyword, "Constant"))
  {
    *pMode = ModImm;
    return True;
  }
  if (!as_strcasecmp(pKeyword, "Memory"))
  {
    *pMode = ModMemory;
    return True;
  }
  if (!as_strcasecmp(pKeyword, "Indexed"))
  {
    *pMode = ModIndexed;
    return True;
  }
  if (!as_strcasecmp(pKeyword, "Indirect"))
  {
    *pMode = ModIndirect;
    return True;
  }
  if (!as_strcasecmp(pKeyword, "Indirect-Indexed"))
  {
    *pMode = ModIndirectIndexed;
    return True;
  }
  return False;
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits in symbol table:
 * Bits 10...3: Absolute Address
 * Bits 0..2: Bit Position
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
  tAdrData AdrData;

  BitPos = EvalBitPosition(pBitArg, &OK);
  if (!OK)
    return False;

  if (!DecodeAdr(pRegArg, NULL, &AdrData, MModMemory))
    return False;

  *pResult = AssembleBitSymbol(BitPos, AdrData.Val);
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

static Boolean DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos)
{
  *pAddress = (BitSymbol >> 3) & 0xfful;
  *pBitPos = BitSymbol & 7;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_KENBAK(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_KENBAK(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  Word Address;

  DissectBitSymbol(Inp, &Address, &BitPos);

  as_snprintf(pDest, DestSize, "%~02.*u%s,%u",
              ListRadixBase, (unsigned)Address, GetIntConstIntelSuffix(ListRadixBase),
              (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandBit_KENBAK(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandBit_KENBAK(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
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
    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     CodeGen(Word Code)
 * \brief  decode generic 2-operand instructions
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeGen(Word Code)
{
  if (ChkArgCnt(2, 3))
  {
    int RegArg;
    Boolean AddrOK;
    Byte Reg;
    tAdrData AdrData;

    /* addressing mode is either given by keyword or by addressing syntax: */

    if ((ArgCnt == 3) && DecodeAddrKeyword(ArgStr[1].str.p_str, &AdrData.Mode))
    {
      AdrData.Val = EvalStrIntExpression(&ArgStr[3], Int8, &AddrOK);
      RegArg = 2;
    }
    else
    {
      AddrOK = DecodeAdr(&ArgStr[2], (ArgCnt == 3) ? &ArgStr[3] : NULL, &AdrData, MModAll);
      RegArg = 1;
    }

    if (AddrOK && DecodeRegWithMemOpt(&ArgStr[RegArg], &Reg, Code & 0xc0 ? 1 : 7))
    {
      BAsmCode[CodeLen++] = (Reg << 6) | Code | AdrData.Mode;
      BAsmCode[CodeLen++] = AdrData.Val;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeClear(Word Code)
 * \brief  decode CLEAR instruction
 * ------------------------------------------------------------------------ */

static void CodeClear(Word Code)
{
  Byte Reg;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && DecodeRegWithMemOpt(&ArgStr[1], &Reg, 7))
  {
    /* SUB reg,reg */

    BAsmCode[CodeLen++] = (Reg << 6) | 0x0c;
    BAsmCode[CodeLen++] = Reg;
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeJump(Word Code)
 * \brief  decode jump instructions (direct/indirect in mnemonic)
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeJumpCommon(Word Code, const tAdrData *pAdrData)
{
  Byte Reg;
  int Cond;
  static const char ShortConds[5][4] = { "NZ", "Z", "N", "P", "PNZ" };
  static const char *LongConds[5] = { "Non-zero", "Zero", "Negative", "Positive", "Positive-Non-zero" };

  if ((ArgCnt == 1) || !as_strcasecmp(ArgStr[1].str.p_str, "Unconditional"))
    Reg = 3;
  else if (!DecodeRegWithMemOpt(&ArgStr[1], &Reg, 7))
    return;

  if (ArgCnt == 1)
    Cond = 0;
  else
  {
    for (Cond = 0; Cond < 5; Cond++)
      if (!as_strcasecmp(ArgStr[ArgCnt - 1].str.p_str, ShortConds[Cond])
       || !as_strcasecmp(ArgStr[ArgCnt - 1].str.p_str, LongConds[Cond]))
        break;
    if (Cond >= 5)
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[ArgCnt - 1]);
      return;
    }
  }
  BAsmCode[CodeLen++] = (Reg << 6) | Code | (Cond + 3);
  BAsmCode[CodeLen++] = pAdrData->Val;
}

static void CodeJump(Word Code)
{
  tAdrData AdrData;

  if ((ArgCnt != 1) && (ArgCnt != 3))
  {
    WrError(ErrNum_WrongArgCnt);
    return;
  }
  if (DecodeAdr(&ArgStr[ArgCnt], NULL, &AdrData, MModMemory))
    CodeJumpCommon(Code, &AdrData);
}

/*!------------------------------------------------------------------------
 * \fn     CodeJumpGen(Word Code)
 * \brief  decode jump instructions (direct/indirect in addressing mode argument)
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeJumpGen(Word Code)
{
  tAdrData AdrData;

  if ((ArgCnt != 1) && (ArgCnt != 3))
  {
    WrError(ErrNum_WrongArgCnt);
      return;
  }

  /* transport the addressing mode's indirect bit (bit 0) to the
     corresponding instruction code's bit (bit 3): */

  if (DecodeAdr(&ArgStr[ArgCnt], NULL, &AdrData, MModMemory | MModIndirect))
    CodeJumpCommon(Code | ((AdrData.Mode & 1) << 3), &AdrData);
}

/*!------------------------------------------------------------------------
 * \fn     CodeSkip(Word Code)
 * \brief  decode skip instructions
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeSkipCore(Word Code, int ArgOffs)
{
  LongWord BitSpec;
  tEvalResult DestEvalResult;
  int HasDest;
  Word Dest;

  /* For two operands, we do not know whether it's <addr>,<bit>
     or <bitsym>,<dest>.  If the third op is from code segment, we
     assume it's the second: */

  switch (ArgCnt - ArgOffs)
  {
    case 1:
      HasDest = 0;
      break;
    case 3:
      HasDest = 1;
      Dest = EvalStrIntExpressionWithResult(&ArgStr[ArgOffs + 3], UInt8, &DestEvalResult);
      if (!DestEvalResult.OK)
        return;
      break;
    case 2:
    {
      Dest = EvalStrIntExpressionWithResult(&ArgStr[ArgOffs + 2], UInt8, &DestEvalResult);
      if (!DestEvalResult.OK)
        return;
      HasDest = !!(DestEvalResult.AddrSpaceMask & (1 << SegCode));
      break;
    }
    default:
      (void)ChkArgCnt(1 + ArgOffs, 3 + ArgOffs);
      return;
  }

  if (DecodeBitArg(&BitSpec, 1 + ArgOffs, ArgCnt - HasDest))
  {
    Word Address;
    Byte BitPos;

    DissectBitSymbol(BitSpec, &Address, &BitPos);

    if (HasDest)
    {
      if (!(DestEvalResult.Flags & (eSymbolFlag_FirstPassUnknown | eSymbolFlag_Questionable))
       && (Dest != EProgCounter() + 4))
      {
        WrStrErrorPos(ErrNum_SkipTargetMismatch, &ArgStr[ArgCnt]);
        return;
      }
    }
    BAsmCode[CodeLen++] = Code | (BitPos << 3);
    BAsmCode[CodeLen++] = Address;
  }
}

static void CodeSkip(Word Code)
{
  CodeSkipCore(Code, 0);
}

static void CodeSkip2(Word Code)
{
  if (ChkArgCnt(2, 4))
  {
    Boolean OK;
    Byte Value = EvalStrIntExpression(&ArgStr[1], UInt1, &OK);
    if (OK)
      CodeSkipCore(Code | (Value << 6), 1);
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeReg(Word Code)
 * \brief  decode shift/rotate instructions
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeShiftCore(Word Code, int ArgOffs)
{
  Byte Reg;

  if (DecodeRegWithMemOpt(&ArgStr[ArgCnt], &Reg, 3))
  {
    Byte Count = 1;

    if (ArgCnt > ArgOffs)
    {
      tEvalResult EvalResult;

      Count = EvalStrIntExpressionWithResult(&ArgStr[ArgOffs], UInt3, &EvalResult);
      if (!EvalResult.OK)
        return;
      if (mFirstPassUnknown(EvalResult.Flags))
        Count = 1;
      if ((Count < 1) || (Count > 4))
      {
        WrStrErrorPos(ErrNum_InvShiftArg, &ArgStr[ArgOffs]);
        return;
      }
    }
    BAsmCode[CodeLen++] = Code | (Reg << 5) | ((Count & 3) << 3);
  }
}

static void CodeShift(Word Code)
{
  if (ChkArgCnt(1, 2))
    CodeShiftCore(Code, 1);
}

static void CodeShift2(Word Code)
{
  if (ChkArgCnt(2, 3))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "LEFT"))
      CodeShiftCore(Code | 0x80, 2);
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "RIGHT"))
      CodeShiftCore(Code, 2);
    else
      WrStrErrorPos(ErrNum_InvShiftArg, &ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeBit(Word Code)
 * \brief  decode bit instructions
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeBitCore(Word Code, int BitArgStart)
{
  LongWord BitSpec;

  if (DecodeBitArg(&BitSpec, BitArgStart, ArgCnt))
  {
    Word Address;
    Byte BitPos;

    DissectBitSymbol(BitSpec, &Address, &BitPos);
    BAsmCode[CodeLen++] = Code | (BitPos << 3);
    BAsmCode[CodeLen++] = Address;
  }
}

static void CodeBit(Word Code)
{
  if (ChkArgCnt(1, 2))
    CodeBitCore(Code, 1);
}

static void CodeBit2(Word Code)
{
  if (ChkArgCnt(2, 3))
  {
    Boolean OK;
    Byte Value = EvalStrIntExpression(&ArgStr[1], UInt1, &OK);
    if (OK)
      CodeBitCore(Code | (Value << 6), 2);
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeFixed(Word Code)
 * \brief  decode instructions witohut argument
 * \param  Code instruction code in 1st byte
 * ------------------------------------------------------------------------ */

static void CodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    BAsmCode[CodeLen++] = Code;
}

/*!------------------------------------------------------------------------
 * \fn     CodeBIT(Word Code)
 * \brief  handle BIT instruction
 * ------------------------------------------------------------------------ */

static void CodeBIT(Word Code)
{
  UNUSED(Code);

  /* if in structure definition, add special element to structure */

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
    pElement->ExpandFnc = ExpandBit_KENBAK;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    LongWord BitSpec;

    if (DecodeBitArg(&BitSpec, 1, ArgCnt))
    {
      *ListLine = '=';
      DissectBit_KENBAK(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  fill instruction hash table
 * ------------------------------------------------------------------------ */

static void InitFields(void)
{
  InstTable = CreateInstTable(51);

  AddInstTable(InstTable, "ADD"  , 0 << 3, CodeGen);
  AddInstTable(InstTable, "SUB"  , 1 << 3, CodeGen);
  AddInstTable(InstTable, "LOAD" , 2 << 3, CodeGen);
  AddInstTable(InstTable, "STORE", 3 << 3, CodeGen);

  AddInstTable(InstTable, "AND"  , 0xd0, CodeGen);
  AddInstTable(InstTable, "OR"   , 0xc0, CodeGen);
  AddInstTable(InstTable, "LNEG" , 0xd8, CodeGen);

  AddInstTable(InstTable, "JPD"  , 0x20, CodeJump);
  AddInstTable(InstTable, "JPI"  , 0x28, CodeJump);
  AddInstTable(InstTable, "JMD"  , 0x30, CodeJump);
  AddInstTable(InstTable, "JMI"  , 0x38, CodeJump);

  AddInstTable(InstTable, "JP"   , 0x20, CodeJumpGen);
  AddInstTable(InstTable, "JM"   , 0x30, CodeJumpGen);

  AddInstTable(InstTable, "SKP0" , 0x82, CodeSkip);
  AddInstTable(InstTable, "SKP1" , 0xc2, CodeSkip);
  AddInstTable(InstTable, "SKP"  , 0x82, CodeSkip2);
  AddInstTable(InstTable, "SKIP" , 0x82, CodeSkip2);

  AddInstTable(InstTable, "SET0" , 0x02, CodeBit);
  AddInstTable(InstTable, "SET1" , 0x42, CodeBit);
  AddInstTable(InstTable, "SET"  , 0x02, CodeBit2);

  AddInstTable(InstTable, "SFTL"  , 0x81, CodeShift);
  AddInstTable(InstTable, "SFTR"  , 0x01, CodeShift);
  AddInstTable(InstTable, "ROTL"  , 0xc1, CodeShift);
  AddInstTable(InstTable, "ROTR"  , 0x41, CodeShift);
  AddInstTable(InstTable, "SHIFT" , 0x01, CodeShift2);
  AddInstTable(InstTable, "ROTATE", 0x41, CodeShift2);

  AddInstTable(InstTable, "CLEAR" , 0x00, CodeClear);

  AddInstTable(InstTable, "NOOP" , 0x80, CodeFixed);
  AddInstTable(InstTable, "HALT" , 0x00, CodeFixed);

  AddInstTable(InstTable, "REG"  , 0   , CodeREG);
  AddInstTable(InstTable, "BIT"  , 0   , CodeBIT);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_KENBAK(void)
{
  CodeLen = 0; DontPrint = False;

  /* to be ignored */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InternSymbol_KENBAK(char *pArg, TempResult *pResult)
{
  Byte RegNum;

  if (DecodeRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize8Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_KENBAK;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static Boolean IsDef_KENBAK(void)
{
  return Memo("REG")
      || Memo("BIT");
}

static void SwitchFrom_KENBAK(void)
{
  DeinitFields();
}

static Boolean TrueFnc(void)
{
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_KENBAK(void)
 * \brief  initialize for KENBAK as target
 * ------------------------------------------------------------------------ */

static void SwitchTo_KENBAK(void)
{
  const TFamilyDescr *Descr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  Descr = FindFamilyByName("KENBAK");
  PCSymbol = "$";
  HeaderID = Descr->Id;
  NOPCode = 0x80;
  DivideChars = ",";
  HasAttrs = False;
  SetIsOccupiedFnc = TrueFnc;
  ShiftIsOccupied = True;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegLimits[SegCode] = 0xff;
  SegInits[SegCode] = 0;

  MakeCode = MakeCode_KENBAK;
  IsDef = IsDef_KENBAK;
  DissectReg = DissectReg_KENBAK;
  DissectBit = DissectBit_KENBAK;
  SwitchFrom = SwitchFrom_KENBAK;
  InternSymbol = InternSymbol_KENBAK;
  InitFields();
}

void codekenbak_init(void)
{
  (void)AddCPU("KENBAK", SwitchTo_KENBAK);
}
