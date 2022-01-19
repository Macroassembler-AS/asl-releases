/* codesx20.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator Parallax SX20                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codef8.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPUSX20, CPUSX28;

static LongInt Reg_FSR, Reg_STATUS;

/*---------------------------------------------------------------------------*/

static Boolean DecodeRegLinear(const tStrComp *pArg, int Offset, Word *pResult, tEvalResult *pEvalResult)
{
  *pResult = EvalStrIntExpressionOffsWithResult(pArg, Offset, UInt8, pEvalResult);
  if (pEvalResult->OK)
    ChkSpace(SegData, pEvalResult->AddrSpaceMask);
  return pEvalResult->OK;
}

static Word Lin2PagedRegAddr(const tStrComp *pArg, Word LinAddr, tSymbolFlags Flags)
{
  if (!mFirstPassUnknown(Flags) && (LinAddr & 0x10))
  {
    if ((Reg_FSR & 0xf0) != (LinAddr & 0xf0))
      WrStrErrorPos(ErrNum_InAccPage, pArg);
  }
  return LinAddr & 0x1f;
}

static Boolean DecodeReg(const tStrComp *pArg, int Offset, Word *pResult)
{
  tEvalResult EvalResult;

  if (!DecodeRegLinear(pArg, Offset, pResult, &EvalResult))
    return False;
  *pResult = Lin2PagedRegAddr(pArg, *pResult, EvalResult.Flags);
  return True;
}

static Boolean DecodeRegAppendix(tStrComp *pArg, const char *pAppendix, Word *pResult)
{
  int ArgLen = strlen(pArg->str.p_str), AppLen = strlen(pAppendix);
  Boolean Result = False;

  if ((ArgLen > AppLen) && !as_strcasecmp(pArg->str.p_str + (ArgLen - AppLen), pAppendix))
  {
    char Save = pArg->str.p_str[ArgLen - AppLen];

    pArg->str.p_str[ArgLen - AppLen] = '\0';
    pArg->Pos.Len -= AppLen;
    Result = DecodeReg(pArg, 0, pResult);
    pArg->str.p_str[ArgLen - AppLen] = Save;
    pArg->Pos.Len += AppLen;
  }
  return Result;
}

/* NOTE: bit symbols have bit position in lower three bits, opposed
   to machine coding.  Done this way because incrementing makes more
   sense this way. */

static Word AssembleBitSymbol(Word RegAddr, Word BitPos)
{
  return ((RegAddr & 0xff) << 3) | (BitPos & 7);
}

static void SplitBitSymbol(Word BitSymbol, Word *pRegAddr, Word *pBitPos)
{
  *pRegAddr = (BitSymbol >> 3) & 0xff;
  *pBitPos = BitSymbol & 7;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_SX20(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_SX20(char *pDest, size_t DestSize, LargeWord Inp)
{
  Word BitPos, Address;

  SplitBitSymbol(Inp, &Address, &BitPos);

  as_snprintf(pDest, DestSize, "$%x.%u", (unsigned)Address, (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitSymbol(const tStrComp *pArg, Word *pBitSymbol)
 * \brief  decode a bit expression and return in internal format
 * \param  pArg bit spec in source code
 * \param  pBitSymbol returns bit in internal format
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitSymbol(const tStrComp *pArg, Word *pBitSymbol, tEvalResult *pEvalResult)
{
  char *pSplit;

  pSplit = strchr(pArg->str.p_str, '.');
  if (!pSplit)
  {
    *pBitSymbol = EvalStrIntExpressionWithResult(pArg, UInt11, pEvalResult);
    if (pEvalResult->OK)
      ChkSpace(SegBData, pEvalResult->AddrSpaceMask);
    return pEvalResult->OK;
  }
  else
  {
    tStrComp RegArg, BitArg;
    Boolean BitOK, RegOK;
    Word BitPos, RegAddr;

    StrCompSplitRef(&RegArg, &BitArg, pArg, pSplit);
    BitPos = EvalStrIntExpression(&BitArg, UInt3, &BitOK);
    RegOK = DecodeRegLinear(&RegArg, 0, &RegAddr, pEvalResult);
    *pBitSymbol = AssembleBitSymbol(RegAddr, BitPos);
    return BitOK && RegOK;
  }
}

static Boolean DecodeRegBitPacked(const tStrComp *pArg, Word *pPackedResult)
{
  Word BitSymbol, RegAddr, BitPos;
  tEvalResult EvalResult;

  if (!DecodeBitSymbol(pArg, &BitSymbol, &EvalResult))
    return False;
  SplitBitSymbol(BitSymbol, &RegAddr, &BitPos);
  *pPackedResult = (BitPos << 5) | Lin2PagedRegAddr(pArg, RegAddr, EvalResult.Flags);
  return True;
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    WAsmCode[CodeLen++] = Code;
}

static void DecodeOneReg(Word Code)
{
  Word Reg;

  if (ChkArgCnt(1, 1)
   && DecodeReg(&ArgStr[1], 0, &Reg))
    WAsmCode[CodeLen++] = Code | Reg;
}

static void DecodeNOT(Word Code)
{
  if (!ChkArgCnt(1, 1))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "W"))
    WAsmCode[CodeLen++] = 0xfff;
  else
    DecodeOneReg(Code);
}

static void DecodeMOV(Word Code)
{
  Word Reg;
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "W"))
  {
    if (*ArgStr[2].str.p_str == '#')
    {
      Reg = EvalStrIntExpressionOffs(&ArgStr[2], 1, Int8, &OK);
      if (OK)
        WAsmCode[CodeLen++] = 0xc00 | (Reg & 0xff);
    }
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "M"))
      WAsmCode[CodeLen++] = 0x042;
    else if (!strncmp(ArgStr[2].str.p_str, "/", 1) && DecodeReg(&ArgStr[2], 1, &Reg))
      WAsmCode[CodeLen++] = 0x240 | Reg;
    else if (!strncmp(ArgStr[2].str.p_str, "--", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x0c0 | Reg;
    else if (!strncmp(ArgStr[2].str.p_str, "++", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x280 | Reg;
    else if (!strncmp(ArgStr[2].str.p_str, "<<", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x340 | Reg;
    else if (!strncmp(ArgStr[2].str.p_str, ">>", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x300 | Reg;
    else if (!strncmp(ArgStr[2].str.p_str, "<>", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x380 | Reg;
    else if (DecodeRegAppendix(&ArgStr[2], "-W", &Reg))
      WAsmCode[CodeLen++] = 0x080 | Reg;
    else if (DecodeReg(&ArgStr[2], 0, &Reg))
      WAsmCode[CodeLen++] = 0x200 | Reg;
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "W"))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "!OPTION"))
      WAsmCode[CodeLen++] = 0x002;
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "M"))
      WAsmCode[CodeLen++] = 0x003;
    else if (!strncmp(ArgStr[1].str.p_str, "!", 1) && DecodeReg(&ArgStr[1], 1, &Reg))
    {
      if (ChkRange(Reg, 0, 7))
        WAsmCode[CodeLen++] = 0x000 | Reg;
    }
    else if (DecodeReg(&ArgStr[1], 0, &Reg))
      WAsmCode[CodeLen++] = 0x020 | Reg;
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "M"))
  {
    if (*ArgStr[2].str.p_str == '#')
    {
      Reg = EvalStrIntExpressionOffs(&ArgStr[2], 1, Int4, &OK);
      if (OK)
        WAsmCode[CodeLen++] = 0x050 | (Reg & 0xf);
    }
    else
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeMOVSZ(Word Code)
{
  Word Reg;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "W"))
  {
    if (!strncmp(ArgStr[2].str.p_str, "--", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x2c0 | Reg;
    else if (!strncmp(ArgStr[2].str.p_str, "++", 2) && DecodeReg(&ArgStr[2], 2, &Reg))
      WAsmCode[CodeLen++] = 0x3c0 | Reg;
    else
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
  }
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

static void DecodeLogic(Word Code)
{
  Word Arg;

  if (!ChkArgCnt(2, 2))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "W"))
  {
    if (*ArgStr[2].str.p_str == '#')
    {
      Word ImmCode = Code & 0x0f00;

      if (ImmCode)
      {
        Boolean OK;

        Arg = EvalStrIntExpressionOffs(&ArgStr[2], 1, Int8, &OK);
        if (OK)
          WAsmCode[CodeLen++] = ImmCode | (Arg & 0xff);
      }
      else
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
    }
    else if (DecodeReg(&ArgStr[2], 0, &Arg))
      WAsmCode[CodeLen++] = (Lo(Code) << 4) | Arg;
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "W"))
  {
    if (DecodeReg(&ArgStr[1], 0, &Arg))
      WAsmCode[CodeLen++] = (Lo(Code) << 4) | 0x20 | Arg;
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeCLR(Word Code)
{
  Word Arg;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "W"))
    WAsmCode[CodeLen++] = 0x040;
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "!WDT"))
    WAsmCode[CodeLen++] = 0x004;
  else if (DecodeReg(&ArgStr[1], 0, &Arg))
    WAsmCode[CodeLen++] = 0x060 | Arg;
}

static void DecodeSUB(Word Code)
{
  Word Arg;

  if (!ChkArgCnt(2, 2))
    return;

  if (!as_strcasecmp(ArgStr[2].str.p_str, "W"))
  {
    if (DecodeReg(&ArgStr[1], 0, &Arg))
      WAsmCode[CodeLen++] = (Lo(Code) << 4) | 0x20 | Arg;
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeBit(Word Code)
{
  Word BitArg;

  if (!ChkArgCnt(1, 1))
    return;

  if (DecodeRegBitPacked(&ArgStr[1], &BitArg))
    WAsmCode[CodeLen++] = Code | BitArg;
}

static void DecodeJMP_CALL(Word Code)
{
  Word Addr;
  Boolean IsCall = (Code == 0x900);
  tEvalResult EvalResult;

  if (!ChkArgCnt(1, 1))
    return;

  if (!IsCall)
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "W"))
    {
      WAsmCode[CodeLen++] = 0x022;
      return;
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "PC+W"))
    {
      WAsmCode[CodeLen++] = 0x1e2;
      return;
    }
  }

  Addr = EvalStrIntExpressionWithResult(&ArgStr[1], UInt11, &EvalResult);
  if (!EvalResult.OK)
    return;

  if (!mFirstPassUnknown(EvalResult.Flags) && IsCall && (Addr & 0x100)) WrStrErrorPos(ErrNum_NotFromThisAddress, &ArgStr[1]);
  else
  {
    if (!mFirstPassUnknown(EvalResult.Flags) && ((Reg_STATUS & 0xe0) != ((Addr >> 4) & 0xe0))) WrStrErrorPos(ErrNum_InAccPage, &ArgStr[1]);
    WAsmCode[CodeLen++] = Code | (Addr & 0x1ff);
    ChkSpace(SegCode, EvalResult.AddrSpaceMask);
  }
}

static void DecodeRETW(Word Code)
{
  Boolean OK;
  Word Arg;

  if (!ChkArgCnt(1, 1))
    return;

  Arg = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
  if (OK)
    WAsmCode[CodeLen++] = Code | Arg;
}

static void DecodeBANK(Word Code)
{
  Word Arg;
  tEvalResult EvalResult;

  if (!ChkArgCnt(1, 1))
    return;

  Arg = EvalStrIntExpressionWithResult(&ArgStr[1], UInt8, &EvalResult);
  if (EvalResult.OK)
  {
    WAsmCode[CodeLen++] = Code | ((Arg >> 5) & 7);
    ChkSpace(SegData, EvalResult.AddrSpaceMask);
  }
}

static void DecodePAGE(Word Code)
{
  tEvalResult EvalResult;
  Word Arg;

  if (!ChkArgCnt(1, 1))
    return;

  Arg = EvalStrIntExpressionWithResult(&ArgStr[1], UInt12, &EvalResult);
  if (EvalResult.OK)
  {
    WAsmCode[CodeLen++] = Code | ((Arg >> 9) & 7);
    ChkSpace(SegCode, EvalResult.AddrSpaceMask);
  }
}

static void DecodeMODE(Word Code)
{
  tEvalResult EvalResult;
  Word Arg;

  if (!ChkArgCnt(1, 1))
    return;

  Arg = EvalStrIntExpressionWithResult(&ArgStr[1], Int4, &EvalResult);
  if (EvalResult.OK)
  {
    WAsmCode[CodeLen++] = Code | Arg;
    ChkSpace(SegCode, EvalResult.AddrSpaceMask);
  }
}

static void DecodeSKIP(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(0, 0))
    return;

  WAsmCode[CodeLen++] = EProgCounter() & 1 ? 0x702 : 0x602;
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);
  CodeEquate(SegData, 0, 0xff);
}

static void DecodeBIT(Word Code)
{
  Word BitSymbol;
  tEvalResult EvalResult;

  UNUSED(Code);
  if (ChkArgCnt(1, 1)
   && DecodeBitSymbol(&ArgStr[1], &BitSymbol, &EvalResult))
  {
    *ListLine = '=';
    DissectBit_SX20(ListLine + 1, STRINGSIZE - 3, BitSymbol);
    PushLocHandle(-1);
    EnterIntSymbol(&LabPart, BitSymbol, SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

static void DecodeDATA_SX20(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int12, Int8);
}

static void DecodeZERO(Word Code)
{
  Word Size;
  Boolean ValOK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &ValOK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (ValOK && !mFirstPassUnknown(Flags))
    {
      if (SetMaxCodeLen(Size << 1)) WrError(ErrNum_CodeOverflow);
      else
      {
        CodeLen = Size;
        memset(WAsmCode, 0, 2 * Size);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

static void InitFields(void)
{
  InstTable = CreateInstTable(103);

  AddInstTable(InstTable, "NOP"  , NOPCode, DecodeFixed);
  AddInstTable(InstTable, "RET"  , 0x00c, DecodeFixed);
  AddInstTable(InstTable, "RETP" , 0x00d, DecodeFixed);
  AddInstTable(InstTable, "RETI" , 0x00e, DecodeFixed);
  AddInstTable(InstTable, "RETIW", 0x00f, DecodeFixed);
  AddInstTable(InstTable, "IREAD", 0x041, DecodeFixed);
  AddInstTable(InstTable, "SLEEP", 0x003, DecodeFixed);
  AddInstTable(InstTable, "CLC"  , 0x403, DecodeFixed);
  AddInstTable(InstTable, "CLZ"  , 0x443, DecodeFixed);
  AddInstTable(InstTable, "SEC"  , 0x503, DecodeFixed);
  AddInstTable(InstTable, "SEZ"  , 0x543, DecodeFixed);
  AddInstTable(InstTable, "SC"   , 0x703, DecodeFixed);
  AddInstTable(InstTable, "SZ"   , 0x743, DecodeFixed);

  AddInstTable(InstTable, "DEC"  , 0x0e0, DecodeOneReg);
  AddInstTable(InstTable, "INC"  , 0x2a0, DecodeOneReg);
  AddInstTable(InstTable, "DECSZ", 0x2e0, DecodeOneReg);
  AddInstTable(InstTable, "INCSZ", 0x3e0, DecodeOneReg);
  AddInstTable(InstTable, "RL"   , 0x360, DecodeOneReg);
  AddInstTable(InstTable, "RR"   , 0x320, DecodeOneReg);
  AddInstTable(InstTable, "SWAP" , 0x3a0, DecodeOneReg);
  AddInstTable(InstTable, "TEST" , 0x220, DecodeOneReg);
  AddInstTable(InstTable, "NOT"  , 0x260, DecodeNOT);

  AddInstTable(InstTable, "MOV"  , 0, DecodeMOV);
  AddInstTable(InstTable, "MOVSZ", 0, DecodeMOVSZ);

  AddInstTable(InstTable, "AND"  , 0x0e14, DecodeLogic);
  AddInstTable(InstTable, "OR"   , 0x0d10, DecodeLogic);
  AddInstTable(InstTable, "XOR"  , 0x0f18, DecodeLogic);
  AddInstTable(InstTable, "ADD"  , 0x001c, DecodeLogic);
  AddInstTable(InstTable, "SUB"  , 0x000a, DecodeSUB);
  AddInstTable(InstTable, "CLR"  , 0, DecodeCLR);

  AddInstTable(InstTable, "CLRB" , 0x0400, DecodeBit);
  AddInstTable(InstTable, "SB"   , 0x0700, DecodeBit);
  AddInstTable(InstTable, "SETB" , 0x0500, DecodeBit);
  AddInstTable(InstTable, "SNB"  , 0x0600, DecodeBit);

  AddInstTable(InstTable, "CALL" , 0x0900, DecodeJMP_CALL);
  AddInstTable(InstTable, "JMP"  , 0x0a00, DecodeJMP_CALL);
  AddInstTable(InstTable, "SKIP" , 0     , DecodeSKIP);

  AddInstTable(InstTable, "RETW" , 0x0800, DecodeRETW);
  AddInstTable(InstTable, "BANK" , 0x0800, DecodeBANK);
  AddInstTable(InstTable, "PAGE" , 0x0010, DecodePAGE);
  AddInstTable(InstTable, "MODE" , 0x0050, DecodeMODE);

  AddInstTable(InstTable, "SFR"  , 0, DecodeSFR);
  AddInstTable(InstTable, "BIT"  , 0, DecodeBIT);
  AddInstTable(InstTable, "DATA" , 0, DecodeDATA_SX20);
  AddInstTable(InstTable, "ZERO" , 0, DecodeZERO);
  AddInstTable(InstTable, "RES"  , 0, DecodeRES);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_SX20(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void SwitchFrom_SX20(void)
{
  DeinitFields();
}

static Boolean IsDef_SX20(void)
{
  return Memo("SFR") || Memo("BIT");
}

static void SwitchTo_SX20(void)
{
  const TFamilyDescr *pDescr = FindFamilyByName("SX20");
#define ASSUMESX20Count (sizeof(ASSUMESX20s) / sizeof(*ASSUMESX20s))
  static const ASSUMERec ASSUMESX20s[] =
  {
    { "FSR"   , &Reg_FSR   , 0,  0xff, 0, NULL },
    { "STATUS", &Reg_STATUS, 0,  0xff, 0, NULL },
  };

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = pDescr->Id;
  NOPCode = 0x000;
  DivideChars = ",";
  HasAttrs = False;
  PageIsOccupied = True;

  ValidSegs = (1 << SegCode) + (1 << SegData);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 2047;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegCode] = 0;
  SegLimits[SegData] = 0xff;

  MakeCode = MakeCode_SX20;
  IsDef = IsDef_SX20;
  SwitchFrom = SwitchFrom_SX20;
  DissectBit = DissectBit_SX20;
  InitFields();

  pASSUMERecs = ASSUMESX20s;
  ASSUMERecCnt = ASSUMESX20Count;
}

void codesx20_init(void)
{
  CPUSX20 = AddCPU("SX20", SwitchTo_SX20);
  CPUSX28 = AddCPU("SX28", SwitchTo_SX20);
}
