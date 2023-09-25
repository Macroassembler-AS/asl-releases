/* codemic8.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator LatticeMico8                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmallg.h"
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"
#include "codepseudo.h"

#include "codemic8.h"

/* define as needed by address space */

#define CodeAddrInt UInt12
#define DataAddrInt UInt5

typedef struct
{
  LongWord Code;
} FixedOrder;

typedef struct
{
  LongWord Code;
  Boolean MayImm;
} ALUOrder;

typedef struct
{
  LongWord Code;
  Byte Space;
} MemOrder;

static FixedOrder *FixedOrders, *ShortBranchOrders, *RegOrders, *LongBranchOrders;
static MemOrder *MemOrders;
static ALUOrder *ALUOrders;

static CPUVar CPUMico8_05, CPUMico8_V3, CPUMico8_V31;

/*--------------------------------------------------------------------------
 * Address Expression Parsing
 *--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     IsWRegCore(const char *pArg, LongWord *pValue)
 * \brief  check whether argument is a CPU register
 * \param  pArg argument to check
 * \param  pValue register number if it is a register
 * \return True if it is a register
 * ------------------------------------------------------------------------ */

static Boolean IsWRegCore(const char *pArg, LongWord *pValue)
{
  Boolean OK;

  if ((strlen(pArg) < 2) || (as_toupper(*pArg) != 'R'))
    return False;

  *pValue = ConstLongInt(pArg + 1, &OK, 10);
  if (!OK)
    return False;

  return (*pValue < 32);
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_Mico8(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - MICO8 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_Mico8(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     IsWReg(const tStrComp *pArg, LongWord *pValue, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg argument to check
 * \param  pValue register number if it is a register
 * \param  MustBeReg expecting register as arg?
 * \return register parse result
 * ------------------------------------------------------------------------ */

static tRegEvalResult IsWReg(const tStrComp *pArg, LongWord *pValue, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (IsWRegCore(pArg->str.p_str, pValue))
    return eIsReg;

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize8Bit, MustBeReg);
  *pValue = RegDescr.Reg;
  return RegEvalResult;
}

/*--------------------------------------------------------------------------
 * Code Handlers
 *--------------------------------------------------------------------------*/

static void DecodePort(Word Index)
{
  UNUSED(Index);

  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

static void DecodeFixed(Word Index)
{
  FixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0))
  {
    DAsmCode[0] = pOrder->Code;
    CodeLen = 1;
  }
}

static void DecodeALU(Word Index)
{
  ALUOrder *pOrder = ALUOrders + Index;
  LongWord Src, DReg;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &DReg, True))
    switch (IsWReg(&ArgStr[2], &Src, True))
    {
      case eIsReg:
        DAsmCode[0] = pOrder->Code | (DReg << 8) | (Src << 3);
        CodeLen = 1;
        break;
      case eIsNoReg:
        if (!pOrder->MayImm) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          Boolean OK;

          Src = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
          if (OK)
          {
            DAsmCode[0] = pOrder->Code | (1 << 13) | (DReg << 8) | (Src & 0xff);
           CodeLen = 1;
          }
        }
        break;
      default:
        break;
    }
}

static void DecodeALUI(Word Index)
{
  ALUOrder *pOrder = ALUOrders + Index;
  LongWord Src, DReg;
  Boolean OK;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &DReg, True))
  {
    Src = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      DAsmCode[0] = pOrder->Code | (1 << 13) | (DReg << 8) | (Src & 0xff);
      CodeLen = 1;
    }
  }
}

static void DecodeShortBranch(Word Index)
{
  FixedOrder *pOrder = ShortBranchOrders + Index;
  LongInt Dest;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1))
  {
    Dest = EvalStrIntExpressionWithFlags(&ArgStr[1], CodeAddrInt, &OK, &Flags);
    if (OK)
    {
      Dest -= EProgCounter();
      if (((Dest < -512) || (Dest > 511)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        DAsmCode[0] = pOrder->Code | (Dest & 0x3ff);
        CodeLen = 1;
      }
    }
  }
}

static void DecodeLongBranch(Word Index)
{
  FixedOrder *pOrder = LongBranchOrders + Index;
  LongInt Dest;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1))
  {
    Dest = EvalStrIntExpressionWithFlags(&ArgStr[1], CodeAddrInt, &OK, &Flags);
    if (OK)
    {
      Dest -= EProgCounter();
      if (((Dest < -2048) || (Dest > 2047)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        DAsmCode[0] = pOrder->Code | (Dest & 0xfff);
        CodeLen = 1;
      }
    }
  }
}

static void DecodeMem(Word Index)
{
  MemOrder *pOrder = MemOrders + Index;
  LongWord DReg, Src;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &DReg, True))
    switch (IsWReg(&ArgStr[2], &Src, False))
    {
      case eIsReg:
        DAsmCode[0] = pOrder->Code | (DReg << 8) | ((Src & 0x1f) << 3) | 2;
        CodeLen = 1;
        break;
      case eIsNoReg:
      {
        tEvalResult EvalResult;

        Src = EvalStrIntExpressionWithResult(&ArgStr[2], DataAddrInt, &EvalResult);
        if (EvalResult.OK)
        {
          ChkSpace(pOrder->Space, EvalResult.AddrSpaceMask);
          DAsmCode[0] = pOrder->Code | (DReg << 8) | ((Src & 0x1f) << 3);
          CodeLen = 1;
        }
        break;
      }
      default:
        break;
    }
}

static void DecodeMemI(Word Index)
{
  MemOrder *pOrder = MemOrders + Index;
  LongWord DReg, SReg;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &DReg, True)
   && IsWReg(&ArgStr[2], &SReg, True))
  {
    DAsmCode[0] = pOrder->Code | (DReg << 8) | (SReg << 3) | 2;
    CodeLen = 1;
  }
}

static void DecodeReg(Word Index)
{
  FixedOrder *pOrder = RegOrders + Index;
  LongWord Reg = 0;

  if (!ChkArgCnt(1, 1));
  else if (IsWReg(&ArgStr[1], &Reg, True))
  {
    DAsmCode[0] = pOrder->Code | (Reg << 8);
    CodeLen = 1;
  }
}

/*--------------------------------------------------------------------------
 * Instruction Table Handling
 *--------------------------------------------------------------------------*/

static void AddFixed(const char *NName, LongWord NCode)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddALU(const char *NName, const char *NImmName, LongWord NCode)
{
  order_array_rsv_end(ALUOrders, ALUOrder);
  ALUOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ, DecodeALU);
  ALUOrders[InstrZ].MayImm = NImmName != NULL;
  if (ALUOrders[InstrZ].MayImm)
    AddInstTable(InstTable, NImmName, InstrZ, DecodeALUI);
  InstrZ++;
}

static void AddShortBranch(const char *NName, LongWord NCode)
{
  order_array_rsv_end(ShortBranchOrders, FixedOrder);
  ShortBranchOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeShortBranch);
}

static void AddLongBranch(const char *NName, LongWord NCode)
{
  order_array_rsv_end(LongBranchOrders, FixedOrder);
  LongBranchOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeLongBranch);
}

static void AddMem(const char *NName, const char *NImmName, LongWord NCode, Byte NSpace)
{
  order_array_rsv_end(MemOrders, MemOrder);
  MemOrders[InstrZ].Code = NCode;
  MemOrders[InstrZ].Space = NSpace;
  AddInstTable(InstTable, NName, InstrZ, DecodeMem);
  AddInstTable(InstTable, NImmName, InstrZ, DecodeMemI);
  InstrZ++;
}

static void AddReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(RegOrders, FixedOrder);
  RegOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(97);

  InstrZ = 0;
  AddFixed("CLRC"  , 0x2c000);
  AddFixed("SETC"  , 0x2c001);
  AddFixed("CLRZ"  , 0x2c002);
  AddFixed("SETZ"  , 0x2c003);
  AddFixed("CLRI"  , 0x2c004);
  AddFixed("SETI"  , 0x2c005);
  if (MomCPU == CPUMico8_05)
  {
    AddFixed("RET"   , 0x3a000);
    AddFixed("IRET"  , 0x3a001);
  }
  else if (MomCPU == CPUMico8_V3)
  {
    AddFixed("RET"   , 0x38000);
    AddFixed("IRET"  , 0x39000);
  }
  else if (MomCPU == CPUMico8_V31)
  {
    AddFixed("RET"   , 0x39000);
    AddFixed("IRET"  , 0x3a000);
  }
  AddFixed("NOP"   , 0x10000);

  InstrZ = 0;
  AddALU("ADD"    , "ADDI"  ,   2UL << 14);
  AddALU("ADDC"   , "ADDIC" ,   3UL << 14);
  AddALU("SUB"    , "SUBI"  ,   0UL << 14);
  AddALU("SUBC"   , "SUBIC" ,   1UL << 14);
  AddALU("MOV"    , "MOVI"  ,   4UL << 14);
  AddALU("AND"    , "ANDI"  ,   5UL << 14);
  AddALU("OR"     , "ORI"   ,   6UL << 14);
  AddALU("XOR"    , "XORI"  ,   7UL << 14);
  AddALU("CMP"    , "CMPI"  ,   8UL << 14);
  AddALU("TEST"   , "TESTI" ,   9UL << 14);
  AddALU("ROR"    , NULL    , (10UL << 14) | 0); /* Note: The User guide (Feb '08) differs  */
  AddALU("ROL"    , NULL    , (10UL << 14) | 1); /* from the actual implementation in */
  AddALU("RORC"   , NULL    , (10UL << 14) | 2); /* decoding the last 3 bits of the Rotate */
  AddALU("ROLC"   , NULL    , (10UL << 14) | 3); /* instructions. These values are correct. */

  InstrZ = 0;
  AddReg("INC"    , (2UL << 14)  | (1UL << 13) | 1);
  AddReg("DEC"    , (0UL << 14)  | (1UL << 13) | 1);

  InstrZ = 0;
  if (MomCPU != CPUMico8_V31)
  {
    AddShortBranch("BZ"    , 0x32000);
    AddShortBranch("BNZ"   , 0x32400);
    AddShortBranch("BC"    , 0x32800);
    AddShortBranch("BNC"   , 0x32c00);
    AddShortBranch("CALLZ" , 0x36000);
    AddShortBranch("CALLNZ", 0x36400);
    AddShortBranch("CALLC" , 0x36800);
    AddShortBranch("CALLNC", 0x36c00);
  }

  /* AcQ/MA: a group for unconditional branches, which can support
   *         larger branches then the conditional branches (not supported
   *         in the earliest versions of the Mico8 processor). The branch
   *         range is +2047 to -2048 instead of +511 to -512. */
  InstrZ = 0;
  if (MomCPU != CPUMico8_05)
  {
    if (MomCPU == CPUMico8_V31)
    {
      AddLongBranch("BZ"    , 0x30000);
      AddLongBranch("BNZ"   , 0x31000);
      AddLongBranch("BC"    , 0x32000);
      AddLongBranch("BNC"   , 0x33000);
      AddLongBranch("CALLZ" , 0x34000);
      AddLongBranch("CALLNZ", 0x35000);
      AddLongBranch("CALLC" , 0x36000);
      AddLongBranch("CALLNC", 0x37000);
      AddLongBranch("CALL"  , 0x38000);
      AddLongBranch("B"     , 0x3b000);
    }
    else
    {
      AddLongBranch("B"     , 0x33000);
      AddLongBranch("CALL"  , 0x37000);
    }
  }

  InstrZ = 0;
  if (MomCPU == CPUMico8_V31)
  {
    AddMem("INP"    , "INPI"   , (23UL << 13) | 1, SegIO);
    AddMem("IMPORT" , "IMPORTI", (23UL << 13) | 1, SegIO);
    AddMem("OUTP"   , "OUTPI"  , (23UL << 13) | 0, SegIO);
    AddMem("EXPORT" , "EXPORTI", (23UL << 13) | 0, SegIO);
    AddMem("LSP"    , "LSPI"   , (23UL << 13) | 5, SegData);
    AddMem("SSP"    , "SSPI"   , (23UL << 13) | 4, SegData);
  }
  else
  {
    if (MomCPU == CPUMico8_V3)
    {
      AddMem("INP"    , "INPI"   , (15UL << 14) | 1, SegIO);
      AddMem("OUTP"   , "OUTPI"  , (15UL << 14) | 0, SegIO);
    }
    AddMem("IMPORT" , "IMPORTI", (15UL << 14) | 1, SegIO);
    AddMem("EXPORT" , "EXPORTI", (15UL << 14) | 0, SegIO);
    AddMem("LSP"    , "LSPI"   , (15UL << 14) | 5, SegData);
    AddMem("SSP"    , "SSPI"   , (15UL << 14) | 4, SegData);
  }

  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "PORT", 0, DecodePort);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(ALUOrders);
  order_array_free(LongBranchOrders);
  order_array_free(ShortBranchOrders);
  order_array_free(MemOrders);
  order_array_free(RegOrders);
}

/*--------------------------------------------------------------------------
 * Semipublic Functions
 *--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_Mico8(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for MICO8
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_Mico8(char *pArg, TempResult *pResult)
{
  LongWord RegNum;

  if (IsWRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize8Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_Mico8;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static Boolean IsDef_Mico8(void)
{
   return (Memo("REG")) || (Memo("PORT"));
}

static void SwitchFrom_Mico8(void)
{
   DeinitFields();
}

static void MakeCode_Mico8(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

   if (Memo("")) return;

   /* Pseudoanweisungen */

   if (DecodeIntelPseudo(True)) return;

   if (!LookupInstTable(InstTable, OpPart.str.p_str))
     WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void SwitchTo_Mico8(void)
{
   const TFamilyDescr *FoundDescr;

   FoundDescr = FindFamilyByName("Mico8");

   TurnWords = True;
   SetIntConstMode(eIntConstModeC);

   PCSymbol = "$"; HeaderID = FoundDescr->Id;

   /* NOP = mov R0,R0 */

   NOPCode = 0x10000;
   DivideChars = ","; HasAttrs = False;

   ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegXData) | (1 << SegIO);
   Grans[SegCode] = 4; ListGrans[SegCode] = 4; SegInits[SegCode] = 0;
   SegLimits[SegCode] = IntTypeDefs[CodeAddrInt].Max;
   Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
   SegLimits[SegData] = IntTypeDefs[DataAddrInt].Max;
   Grans[SegXData] = 1; ListGrans[SegXData] = 1; SegInits[SegXData] = 0;
   SegLimits[SegXData] = 0xff;
   Grans[SegIO] = 1; ListGrans[SegIO] = 1; SegInits[SegIO] = 0;
   SegLimits[SegIO] = 0xff;

   MakeCode = MakeCode_Mico8;
   IsDef = IsDef_Mico8;
   InternSymbol = InternSymbol_Mico8;
   DissectReg = DissectReg_Mico8;
   SwitchFrom = SwitchFrom_Mico8; InitFields();
}

/*--------------------------------------------------------------------------
 * Initialization
 *--------------------------------------------------------------------------*/

void codemico8_init(void)
{
   CPUMico8_05  = AddCPU("Mico8_05" , SwitchTo_Mico8);
   CPUMico8_V3  = AddCPU("Mico8_V3" , SwitchTo_Mico8);
   CPUMico8_V31 = AddCPU("Mico8_V31", SwitchTo_Mico8);
}
