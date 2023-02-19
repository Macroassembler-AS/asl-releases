/* codest6.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator ST6-Familie                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmstructs.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"
#include "intformat.h"

#include "codest6.h"

#define ModNone (-1)
#define ModAcc 0
#define MModAcc (1 << ModAcc)
#define ModDir 1
#define MModDir (1 << ModDir)
#define ModInd 2
#define MModInd (1 << ModInd)


static Byte AdrMode;
static ShortInt AdrType;
static Byte AdrVal;

static LongInt WinAssume, PRPRVal;

typedef struct
{
  const char *pName;
  IntType CodeAdrInt;
} tCPUProps;

#define ASSUMEST6Count 2
static ASSUMERec ASSUMEST6s[ASSUMEST6Count] =
{
  { "PRPR",    &PRPRVal  , 0, 0x03, 0x04, NULL },
  { "ROMBASE", &WinAssume, 0, 0x3f, 0x40, NULL },
};
static const tCPUProps *pCurrCPUProps;

/*---------------------------------------------------------------------------------*/
/* Helper Functions */

static void ResetAdr(void)
{
  AdrType = ModNone; AdrCnt = 0;
}

static void DecodeAdr(const tStrComp *pArg, Byte Mask)
{
  Integer AdrInt;
  tEvalResult EvalResult;

  ResetAdr();

  if ((!as_strcasecmp(pArg->str.p_str, "A")) && (Mask & MModAcc))
  {
    AdrType = ModAcc;
    goto chk;
  }

  if (!as_strcasecmp(pArg->str.p_str, "(X)"))
  {
    AdrType = ModInd;
    AdrMode = 0;
    goto chk;
  }

  if (!as_strcasecmp(pArg->str.p_str, "(Y)"))
  {
    AdrType = ModInd;
    AdrMode = 1;
    goto chk;
  }

  AdrInt = EvalStrIntExpressionWithResult(pArg, UInt16, &EvalResult);
  if (EvalResult.OK)
  {
    if (EvalResult.AddrSpaceMask & (1 << SegCode))
    {
      AdrType = ModDir;
      AdrVal = (AdrInt & 0x3f) + 0x40;
      AdrCnt=1;
      if (!mFirstPassUnknown(EvalResult.Flags))
        if (WinAssume != (AdrInt >> 6)) WrError(ErrNum_InAccPage);
    }
    else
    {
      if (mFirstPassUnknown(EvalResult.Flags)) AdrInt = Lo(AdrInt);
      if (AdrInt > 0xff) WrError(ErrNum_OverRange);
      else
      {
        AdrType = ModDir;
        AdrVal = AdrInt;
        goto chk;
      }
    }
  }

chk:
  if ((AdrType != ModNone) && (!(Mask & (1 << AdrType))))
  {
    ResetAdr(); WrError(ErrNum_InvAddrMode);
  }
}

static Boolean IsReg(Byte Adr)
{
  return ((Adr & 0xfc) == 0x80);
}

static Byte MirrBit(Byte inp)
{
  return (((inp & 1) << 2) + (inp & 2) + ((inp & 4) >> 2));
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits in symbol table:
 * bits 0..2: bit position
 * bits 3...10: register address in DATA/SFR space
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
 * \fn     AssembleBitSymbol(Byte BitPos, Word Address)
 * \brief  build the compact internal representation of a bit symbol
 * \param  BitPos bit position in word
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, Word Address)
{
  return (BitPos & 7)
       | (((LongWord)Address & 0xff) << 3);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg)
{
  Boolean OK;
  LongWord Addr;
  Byte BitPos;

  BitPos = EvalBitPosition(pBitArg, &OK);
  if (!OK)
    return False;

  Addr = EvalStrIntExpression(pRegArg, UInt8, &OK);
  if (!OK)
    return False;

  *pResult = AssembleBitSymbol(BitPos, Addr);

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

    *pResult = EvalStrIntExpressionWithResult(&ArgStr[Start], UInt11, &EvalResult);
    if (EvalResult.OK)
      ChkSpace(SegBData, EvalResult.AddrSpaceMask);
    return EvalResult.OK;
  }

  /* register & bit position are given as separate arguments */

  else if (Stop == Start + 1)
    return DecodeBitArg2(pResult, &ArgStr[Stop], &ArgStr[Start]);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos)
 * \brief  transform compact represenation of bit (field) symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress (I/O) register address
 * \param  pBitPos (start) bit position
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos)
{
  *pAddress = (BitSymbol >> 3) & 0xffff;
  *pBitPos = BitSymbol & 7;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_ST6(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_ST6(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  Word Address;

  DissectBitSymbol(Inp, &Address, &BitPos);

  as_snprintf(pDest, DestSize, "%02.*u%s.%u",
              ListRadixBase, (unsigned)Address, GetIntConstIntelSuffix(ListRadixBase),
              (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandST6Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandST6Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
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
    if (!ChkRange(Address, 0, 0xff)
     || !ChkRange(pStructElem->BitPos, 0, 7))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*---------------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
}

static void DecodeLD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModDir | MModInd);
    switch (AdrType)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModDir | MModInd);
        switch (AdrType)
        {
          case ModDir:
            if (IsReg(AdrVal))
            {
              CodeLen = 1;
              BAsmCode[0] = 0x35 + ((AdrVal & 3) << 6);
            }
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x1f;
              BAsmCode[1] = AdrVal;
            }
            break;
          case ModInd:
            CodeLen = 1;
            BAsmCode[0] = 0x07 + (AdrMode << 3);
            break;
        }
        break;
      case ModDir:
        DecodeAdr(&ArgStr[2], MModAcc);
        if (AdrType != ModNone)
        {
          if (IsReg(AdrVal))
          {
            CodeLen = 1;
            BAsmCode[0] = 0x3d + ((AdrVal & 3) << 6);
          }
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0x9f;
            BAsmCode[1] = AdrVal;
          }
        }
        break;
      case ModInd:
        DecodeAdr(&ArgStr[2], MModAcc);
        if (AdrType != ModNone)
        {
          CodeLen = 1;
          BAsmCode[0] = 0x87 + (AdrMode << 3);
        }
        break;
    }
  }
}

static void DecodeLDI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean OK;

    Integer AdrInt = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      DecodeAdr(&ArgStr[1], MModAcc | MModDir);
      switch (AdrType)
      {
        case ModAcc:
          CodeLen = 2;
          BAsmCode[0] = 0x17;
          BAsmCode[1] = Lo(AdrInt);
          break;
        case ModDir:
          CodeLen = 3;
          BAsmCode[0] = 0x0d;
          BAsmCode[1] = AdrVal;
          BAsmCode[2] = Lo(AdrInt);
          break;
      }
    }
  }
}

static void DecodeRel(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags) - (EProgCounter() + 1);

    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt < -16) || (AdrInt > 15))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = Code + ((AdrInt << 3) & 0xf8);
      }
    }
  }
}

static void DecodeJP_CALL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrInt;
    tSymbolFlags Flags;

    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], pCurrCPUProps->CodeAdrInt, &OK, &Flags);
    if (OK)
    {
      Word DestPage = AdrInt >> 11;

      /* CPU program space's page 1 (800h...0fffh) always accesses ROM space page 1.
         CPU program space's page 0 (000h...7ffh) accesses 2K ROM space pages as defined by PRPR. */

      if (!mFirstPassUnknown(Flags) && (DestPage != 1))
      {
        Word SrcPage = EProgCounter() >> 11;

        /* Jump from page 1 is allowed to page defined by PRPR.
           Jump from any other page is only allowed back to page 1 or within same page. */

        if (DestPage != ((SrcPage == 1) ? PRPRVal : SrcPage)) WrError(ErrNum_InAccPage);

        AdrInt &= 0x7ff;
      }
      CodeLen = 2;
      BAsmCode[0] = Code + ((AdrInt & 0x00f) << 4);
      BAsmCode[1] = AdrInt >> 4;
    }
  }
}

static void DecodeALU(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc);
    if (AdrType != ModNone)
    {
      DecodeAdr(&ArgStr[2], MModDir | MModInd);
      switch (AdrType)
      {
        case ModDir:
          CodeLen = 2;
          BAsmCode[0] = Code + 0x18;
          BAsmCode[1] = AdrVal;
          break;
        case ModInd:
          CodeLen = 1;
          BAsmCode[0] = Code + (AdrMode << 3);
          break;
      }
    }
  }
}

static void DecodeALUImm(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc);
    if (AdrType != ModNone)
    {
      Boolean OK;
      BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
      if (OK)
      {
        CodeLen = 2;
        BAsmCode[0] = Code + 0x10;
      }
    }
  }
}

static void DecodeCLR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModDir);
    switch (AdrType)
    {
      case ModAcc:
        CodeLen = 2;
        BAsmCode[0] = 0xdf;
        BAsmCode[1] = 0xff;
        break;
      case ModDir:
        CodeLen = 3;
        BAsmCode[0] = 0x0d;
        BAsmCode[1] = AdrVal;
        BAsmCode[2] = 0;
        break;
    }
  }
}

static void DecodeAcc(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModAcc);
    if (AdrType != ModNone)
    {
      BAsmCode[CodeLen++] = Lo(Code);
      if (Hi(Code))
        BAsmCode[CodeLen++] = Hi(Code);
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModDir | MModInd);
    switch (AdrType)
    {
      case ModDir:
        if (IsReg(AdrVal))
        {
          CodeLen = 1;
          BAsmCode[0] = Code + 0x15 + ((AdrVal & 3) << 6);
        }
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0x7f + (Code << 4);
          BAsmCode[1] = AdrVal;
        }
        break;
      case ModInd:
        CodeLen = 1;
        BAsmCode[0] = 0x67 + (AdrMode << 3) + (Code << 4);
        break;
    }
  }
}

static void DecodeSET_RES(Word Code)
{
  LongWord PackedAddr;

  if (ChkArgCnt(1, 2)
   && DecodeBitArg(&PackedAddr, 1, ArgCnt))
  {
    Word RegAddr;
    Byte BitPos;

    DissectBitSymbol(PackedAddr, &RegAddr, &BitPos);
    BAsmCode[0] = (MirrBit(BitPos) << 5) | Code;
    BAsmCode[1] = RegAddr;
    CodeLen = 2;
  }
}

static void DecodeJRR_JRS(Word Code)
{
  LongWord PackedAddr;

  if (ChkArgCnt(2, 3)
   && DecodeBitArg(&PackedAddr, 1, ArgCnt - 1))
  {
    Word RegAddr;
    Byte BitPos;
    Boolean OK;
    Integer AdrInt;
    tSymbolFlags Flags;

    DissectBitSymbol(PackedAddr, &RegAddr, &BitPos);
    BAsmCode[0] = (MirrBit(BitPos) << 5) | Code;
    BAsmCode[1] = RegAddr;
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], UInt16, &OK, &Flags) - (EProgCounter() + 3);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 3;
        BAsmCode[2] = Lo(AdrInt);
      }
    }
  }
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);
  CodeEquate(SegData, 0, 0xff);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeASCII_ASCIZ(Word IsZ)
 * \brief  handle ASCII/ASCIZ instructions
 * \param  IsZ 1 if it's ASCIZ
 * ------------------------------------------------------------------------ */

static void DecodeASCII_ASCIZ(Word IsZ)
{
  if (ChkArgCnt(1, ArgCntMax))
  {
    int l;
    Boolean OK = True;
    tStrComp *pArg;
    TempResult t;

    as_tempres_ini(&t);
    forallargs(pArg, OK)
    {
      EvalStrExpression(pArg, &t);
      switch (t.Typ)
      {
        case TempString:
        {
          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            OK = False;
          else
          {
            l = t.Contents.str.len;
            if (SetMaxCodeLen(CodeLen + l + IsZ))
            {
              WrStrErrorPos(ErrNum_CodeOverflow, pArg); OK = False;
            }
            else
            {
              memcpy(BAsmCode + CodeLen, t.Contents.str.p_str, l);
              CodeLen += l;
              if (IsZ)
                BAsmCode[CodeLen++] = 0;
            }
          }
          break;
        }
        case TempNone:
          OK = False;
          break;
        default:
          WrStrErrorPos(ErrNum_ExpectString, pArg);
          OK = False;
      }
    }
    as_tempres_free(&t);
    if (!OK)
      CodeLen = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBIT(Word Code)
 * \brief  decode BIT instruction
 * ------------------------------------------------------------------------ */

static void DecodeBIT(Word Code)
{
  LongWord BitSpec;

  UNUSED(Code);

  /* if in structure definition, add special element to structure */

  if (ActPC == StructSeg)
  {
    Boolean OK;
    Byte BitPos;
    PStructElem pElement;

    if (!ChkArgCnt(2, 2))
      return;
    BitPos = EvalBitPosition(&ArgStr[2], &OK);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = eSymbolSize8Bit;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandST6Bit;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (DecodeBitArg(&BitSpec, 1, ArgCnt))
    {
      *ListLine = '=';
      DissectBit_ST6(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*---------------------------------------------------------------------------------*/
/* code table handling */

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddRel(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel);
}

static void AddALU(const char *NName, const char *NNameImm, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU);
  AddInstTable(InstTable, NNameImm, NCode, DecodeALUImm);
}

static void AddAcc(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAcc);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "LDI", 0, DecodeLDI);
  AddInstTable(InstTable, "JP", 0x09, DecodeJP_CALL);
  AddInstTable(InstTable, "CALL", 0x01, DecodeJP_CALL);
  AddInstTable(InstTable, "CLR", 0, DecodeCLR);
  AddInstTable(InstTable, "INC", 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 8, DecodeINC_DEC);
  AddInstTable(InstTable, "SET", 0x1b, DecodeSET_RES);
  AddInstTable(InstTable, "RES", 0x0b, DecodeSET_RES);
  AddInstTable(InstTable, "JRR", 0x03, DecodeJRR_JRS);
  AddInstTable(InstTable, "JRS", 0x13, DecodeJRR_JRS);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "ASCII", 0, DecodeASCII_ASCIZ);
  AddInstTable(InstTable, "ASCIZ", 1, DecodeASCII_ASCIZ);
  AddInstTable(InstTable, "BYTE", 0, DecodeMotoBYT);
  AddInstTable(InstTable, "WORD", 0, DecodeMotoADR);
  AddInstTable(InstTable, "BLOCK", 0, DecodeMotoDFS);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);

  AddFixed("NOP" , 0x04);
  AddFixed("RET" , 0xcd);
  AddFixed("RETI", 0x4d);
  AddFixed("STOP", 0x6d);
  AddFixed("WAIT", 0xed);

  AddRel("JRZ" , 0x04);
  AddRel("JRNZ", 0x00);
  AddRel("JRC" , 0x06);
  AddRel("JRNC", 0x02);

  AddALU("ADD" , "ADDI" , 0x47);
  AddALU("AND" , "ANDI" , 0xa7);
  AddALU("CP"  , "CPI"  , 0x27);
  AddALU("SUB" , "SUBI" , 0xc7);

  AddAcc("COM", 0x002d);
  AddAcc("RLC", 0x00ad);
  AddAcc("SLA", 0xff5f);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*!------------------------------------------------------------------------
 * \fn     MakeCode_ST6(void)
 * \brief  entry point to decode machine instructions
 * ------------------------------------------------------------------------ */

static void MakeCode_ST6(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     InitCode_ST6(void)
 * \brief  per-pass initializations for ST6
 * ------------------------------------------------------------------------ */

static void InitCode_ST6(void)
{
  WinAssume = 0x40;
  PRPRVal = 0;
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_ST6(void)
 * \brief  does instruction consume label field?
 * \return true if to be consumed
 * ------------------------------------------------------------------------ */

static Boolean IsDef_ST6(void)
{
  return Memo("SFR") || Memo("BIT");
}

/*!------------------------------------------------------------------------
 * \fn     SwitchFrom_ST6(void)
 * \brief  cleanup after switching away from target
 * ------------------------------------------------------------------------ */

static void SwitchFrom_ST6(void)
{
  DeinitFields();
}

static Boolean TrueFnc(void)
{
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_ST6(char *pArg, TempResult *pErg)
 * \brief  check for built-in symbols
 * \param  pAsc ASCII repr. of symbol
 * \param  pErg result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_ST6(char *pArg, TempResult *pErg)
{
  int z;
#define RegCnt 5
  static const char RegNames[RegCnt + 1][2] = {"A", "V", "W", "X", "Y"};
  static const Byte RegCodes[RegCnt + 1] = {0xff, 0x82, 0x83, 0x80, 0x81};

  for (z = 0; z < RegCnt; z++)
    if (!as_strcasecmp(pArg, RegNames[z]))
    {
      as_tempres_set_int(pErg, RegCodes[z]);
      pErg->AddrSpaceMask |= (1 << SegData);
    }
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_ST6(void)
 * \brief  switch to target
 * ------------------------------------------------------------------------ */

static void SwitchTo_ST6(void *pUser)
{
  int ASSUMEOffset;

  pCurrCPUProps = (const tCPUProps*)pUser;
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = TrueFnc;

  PCSymbol = "PC"; HeaderID = 0x78; NOPCode = 0x04;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[pCurrCPUProps->CodeAdrInt].Max;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  SegLimits[SegData] = 0xff;

  ASSUMEOffset = (SegLimits[SegCode] > 0xfff) ? 0 : 1;
  pASSUMERecs = ASSUMEST6s + ASSUMEOffset;
  ASSUMERecCnt = ASSUMEST6Count - ASSUMEOffset;

  MakeCode = MakeCode_ST6;
  IsDef = IsDef_ST6;
  SwitchFrom = SwitchFrom_ST6;
  DissectBit = DissectBit_ST6;
  InternSymbol = InternSymbol_ST6;

  InitFields();
}

/*!------------------------------------------------------------------------
 * \fn     codest6_init(void)
 * \brief  register ST6 target
 * ------------------------------------------------------------------------ */

static const tCPUProps CPUProps[] =
{
  { "ST6200", UInt12 },
  { "ST6201", UInt12 },
  { "ST6203", UInt12 },
  { "ST6208", UInt12 },
  { "ST6209", UInt12 },
  { "ST6210", UInt12 },
  { "ST6215", UInt12 },
  { "ST6218", UInt13 },
  { "ST6220", UInt12 },
  { "ST6225", UInt12 },
  { "ST6228", UInt13 },
  { "ST6230", UInt13 },
  { "ST6232", UInt13 },
  { "ST6235", UInt13 },
  { "ST6240", UInt13 },
  { "ST6242", UInt13 },
  { "ST6245", UInt12 },
  { "ST6246", UInt12 },
  { "ST6252", UInt12 },
  { "ST6253", UInt12 },
  { "ST6255", UInt12 },
  { "ST6260", UInt12 },
  { "ST6262", UInt12 },
  { "ST6263", UInt12 },
  { "ST6265", UInt12 },
  { "ST6280", UInt13 },
  { "ST6285", UInt13 },
  { NULL    , (IntType)0 },
};

void codest6_init(void)
{
  const tCPUProps *pProp;

  for (pProp = CPUProps; pProp->pName; pProp++)
    (void)AddCPUUser(pProp->pName, SwitchTo_ST6, (void*)pProp, NULL);

  AddInitPassProc(InitCode_ST6);
}
