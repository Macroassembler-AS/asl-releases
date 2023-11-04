/* code960.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Makroassembler AS                                                         */
/*                                                                           */
/* Codegenerator i960-Familie                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "be_le.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "codevars.h"
#include "intpseudo.h"
#include "headids.h"
#include "errmsg.h"

#include "code960.h"

/*--------------------------------------------------------------------------*/

enum
{
  ModNone = -1,
  ModReg = 0,
  ModFReg = 1,
  ModImm = 2
};

#define MModReg (1 << ModReg)
#define MModFReg (1 << ModFReg)
#define MModImm (1 << ModImm)

typedef enum
{
  NoneOp, IntOp, LongOp, QuadOp, SingleOp, DoubleOp, ExtOp, OpCnt
} OpType;

static LongWord OpMasks[OpCnt] =
{
  0xffffffff, 0, 1, 3, 0, 1, 3
};

typedef struct
{
  LongWord Code;
} FixedOrder;

typedef struct
{
  LongWord Code;
  Boolean HasSrc;
} CobrOrder;

typedef struct
{
  LongWord Code;
  OpType Src1Type, Src2Type, DestType;
  Boolean Imm1, Imm2, Privileged;
} RegOrder;

typedef struct
{
  LongWord Code;
  OpType Type;
  ShortInt RegPos;
} MemOrder;

typedef struct
{
  char Name[4];
  LongWord Code;
} SpecReg;

static FixedOrder *FixedOrders;
static RegOrder *RegOrders;
static CobrOrder *CobrOrders;
static FixedOrder *CtrlOrders;
static MemOrder *MemOrders;
static const SpecReg SpecRegs[] =
{
  { "FP" , 31 },
  { "PFP",  0 },
  { "SP" ,  1 },
  { "RIP",  2 }
};

static CPUVar CPU80960;

/*--------------------------------------------------------------------------*/

static Boolean ChkAdr(int AMode, Byte Mask, LongWord *Erg, LongWord *Mode)
{
  UNUSED(Erg);

  if (!(Mask & (1 << AMode)))
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  else
  {
    *Mode = (AMode != ModReg);
    return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIRegCore(const char *pArg, LongWord *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeIRegCore(const char *pArg, LongWord *pResult)
{
  size_t z;
  LongWord Offs;

  for (z = 0; z < as_array_size(SpecRegs); z++)
   if (!as_strcasecmp(pArg, SpecRegs[z].Name))
   {
     *pResult = REGSYM_FLAG_ALIAS | SpecRegs[z].Code;
     return True;
   }

  switch (as_toupper(*pArg))
  {
    case 'G':
      Offs = 16;
      goto eval;
    case 'R':
      Offs = 0;
      goto eval;
    default:
      return False;
    eval:
    {
      char *pEnd;

      *pResult = strtoul(pArg + 1, &pEnd, 10);
      if (!*pEnd && (*pResult <= 15))
      {
        *pResult += Offs;
        return True;
      }
    }
  }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPRegCore(const char *pArg, LongWord *pResult)
 * \brief  check whether argument is an FPU register
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeFPRegCore(const char *pArg, LongWord *pResult)
{
  if (!as_strncasecmp(pArg, "FP", 2))
  {
    char *pEnd;

    *pResult = strtoul(pArg + 2, &pEnd, 10);
    if (!*pEnd && (*pResult <= 3))
      return True;
  }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_960(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - i960 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_960(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
    {
      size_t z;

      for (z = 0; z < as_array_size(SpecRegs); z++)
        if (Value == (REGSYM_FLAG_ALIAS | SpecRegs[z].Code))
        {
          as_snprintf(pDest, DestSize, "%s", SpecRegs[z].Name);
          return;
        }
      as_snprintf(pDest, DestSize, "%c%u", Value & 16 ? 'G' : 'R', (unsigned)(Value & 15));
      break;
    }
    case eSymbolSizeFloat64Bit:
      as_snprintf(pDest, DestSize, "FP%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIReg(const tStrComp *pArg, LongWord *pResult, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \param  MustBeReg True if register is expected
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeIReg(const tStrComp *pArg, LongWord *pResult, Boolean MustBeReg)
{
  if (DecodeIRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return eIsReg;
  }
  else
  {
    tRegDescr RegDescr;
    tEvalResult EvalResult;
    tRegEvalResult RegEvalResult;

    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
    if (eIsReg == RegEvalResult)
      *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
    return RegEvalResult;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIOrFPReg(const tStrComp *pArg, LongWord *pResult, tSymbolSize *pSize, Boolean MustBeReg)
 * \brief  check whether argument is a CPU/FPU register or register alias
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \param  pSize returns register size/type
 * \param  MustBeReg True if register is expected
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeIOrFPReg(const tStrComp *pArg, LongWord *pResult, tSymbolSize *pSize, Boolean MustBeReg)
{
  if (DecodeIRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    *pSize = eSymbolSize32Bit;
    return eIsReg;
  }
  else if (DecodeFPRegCore(pArg->str.p_str, pResult))
  {
    *pSize = eSymbolSizeFloat64Bit;
    return eIsReg;
  }
  else
  {
    tRegDescr RegDescr;
    tEvalResult EvalResult;
    tRegEvalResult RegEvalResult;

    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
    if (eIsReg == RegEvalResult)
    {
      *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
      *pSize = EvalResult.DataSize;
    }
    return RegEvalResult;
  }
}

static Boolean DecodeAdr(const tStrComp *pArg, Byte Mask, OpType Type, LongWord *Erg, LongWord *Mode)
{
  Double FVal;
  tEvalResult EvalResult;
  tSymbolSize DataSize;

  *Mode = ModNone;
  *Erg = 0;

  switch (DecodeIOrFPReg(pArg, Erg, &DataSize, False))
  {
    case eIsReg:
      switch (DataSize)
      {
        case eSymbolSize32Bit:
          if ((*Erg) & OpMasks[Type])
          {
            WrStrErrorPos(ErrNum_InvRegPair, pArg);
            return False;
          }
          else
            return ChkAdr(ModReg, Mask, Erg, Mode);
        case eSymbolSizeFloat64Bit:
          return ChkAdr(ModFReg, Mask, Erg, Mode);
        default:
          break;
      }
      break;
    case eRegAbort:
      return False;
    case eIsNoReg:
      break;
  }

  if (Type != IntOp)
  {
    FVal = EvalStrFloatExpressionWithResult(pArg, Float64, &EvalResult);
    if (EvalResult.OK)
    {
      if (mFirstPassUnknown(EvalResult.Flags))
        FVal = 0.0;
      if (FVal == 0.0)
        *Erg = 16;
      else if (FVal == 1.0)
        *Erg = 22;
      else
      {
        WrError(ErrNum_OverRange);
        EvalResult.OK = False;
      }
      if (EvalResult.OK)
        return ChkAdr(ModImm, Mask, Erg, Mode);
    }
  }
  else
  {
    *Erg = EvalStrIntExpressionWithResult(pArg, UInt5, &EvalResult);
    if (EvalResult.OK)
      return ChkAdr(ModImm, Mask, Erg, Mode);
  }
  return False;
}

#define NOREG 33
#define IPREG 32

static int AddrError(tErrorNum Num)
{
  WrError(Num);
  return -1;
}

static int DecodeMem(const tStrComp *pArg, LongWord *Erg, LongWord *Ext)
{
  LongInt DispAcc;
  LongWord Base, Index, Scale, Mode;
  Boolean Done;
  int ArgLen, Scale2;
  char *p, *p2, *end;
  Boolean OK;
  tStrComp Arg = *pArg, RegArg, ScaleArg;

  Base = Index = NOREG;
  Scale = 0;

  /* Register abhobeln */

  Done = FALSE;
  do
  {
    ArgLen = strlen(Arg.str.p_str);
    if (ArgLen == 0)
      Done = True;
    else switch (Arg.str.p_str[ArgLen - 1])
    {
      case ']':
        if (Index != NOREG) return AddrError(ErrNum_InvAddrMode);
        for (p = Arg.str.p_str + ArgLen - 1; p >= Arg.str.p_str; p--)
          if (*p == '[')
            break;
        if (p < Arg.str.p_str) return AddrError(ErrNum_BrackErr);
        StrCompShorten(&Arg, 1);
        StrCompSplitRef(&Arg, &RegArg, &Arg, p);
        p2 = strchr(RegArg.str.p_str, '*');
        if (p2)
        {
          StrCompSplitRef(&RegArg, &ScaleArg, &RegArg, p2);
          Scale2 = strtol(ScaleArg.str.p_str, &end, 10);
          if (*end != '\0') return AddrError(ErrNum_InvAddrMode);
          for (Scale = 0; Scale < 5; Scale++, Scale2 = Scale2 >> 1)
            if (Odd(Scale2))
              break;
          if (Scale2 != 1) return AddrError(ErrNum_InvAddrMode);
        }
        if (DecodeIReg(&RegArg, &Index, True) != eIsReg)
          return -1;

        break;
      case ')':
        if (Base != NOREG) return AddrError(ErrNum_InvAddrMode);
        for (p = Arg.str.p_str + ArgLen - 1; p >= Arg.str.p_str; p--)
          if (*p == '(')
            break;
        if (p < Arg.str.p_str) return AddrError(ErrNum_BrackErr);
        StrCompShorten(&Arg, 1);
        StrCompSplitRef(&Arg, &RegArg, &Arg, p);
        if (!as_strcasecmp(RegArg.str.p_str, "IP"))
          Base = IPREG;
        else if (DecodeIReg(&RegArg, &Base, True) != eIsReg)
          return -1;
        break;
      default:
        Done = True;
    }
  }
  while (!Done);

  DispAcc = EvalStrIntExpression(&Arg, Int32, &OK);

  if (Base == IPREG)
  {
    DispAcc -= EProgCounter() + 8;
    if (Index != NOREG) return AddrError(ErrNum_InvAddrMode);
    else
    {
      *Erg = (5 << 10);
      *Ext = DispAcc;
      return 1;
    }
  }
  else if ((Index == NOREG) && (DispAcc >= 0) && (DispAcc <= 4095))
  {
    *Erg = DispAcc;
    if (Base != NOREG)
      *Erg += 0x2000 + (Base << 14);
    return 0;
  }
  else
  {
    Mode = (Ord(DispAcc != 0) << 3) + 4 + (Ord(Index != NOREG) << 1) + Ord(Base != NOREG);
    if ((Mode & 9) == 0)
      Mode += 8;
    if (Mode == 5)
      Mode--;
    *Erg = (Mode << 10);
    if (Base != NOREG)
      *Erg += Base << 14;
    if (Index != NOREG)
      *Erg += Index + (Scale << 7);
    if (Mode < 8) return 0;
    else
    {
      *Ext = DispAcc;
      return 1;
    }
  }
}

/*--------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  FixedOrder *Op = FixedOrders + Index;

  if (ChkArgCnt(0, 0))
  {
    DAsmCode[0] = Op->Code;
    CodeLen = 4;
  }
}

static void DecodeReg(Word Index)
{
  RegOrder *Op = RegOrders + Index;
  LongWord DReg = 0, DMode = 0;
  LongWord S1Reg = 0, S1Mode = 0;
  LongWord S2Reg = 0, S2Mode = 0;
  unsigned NumArgs = 1 + Ord(Op->Src2Type != NoneOp) + Ord(Op->DestType != NoneOp), ActArgCnt;
  tStrComp *pDestArg = NULL;

  /* if destination required, but too few args, assume the last op is also destination */

  ActArgCnt = ArgCnt;
  if (Op->DestType != NoneOp)
  {
    if (ArgCnt == 1 + Ord(Op->Src2Type != NoneOp))
      ActArgCnt++;
    pDestArg = &ArgStr[ArgCnt];
  }

  if (!ChkArgCntExt(ActArgCnt, NumArgs, NumArgs));
  else if (((Op->DestType >= SingleOp) || (Op->Src1Type >= SingleOp)) && (!FPUAvail)) WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
  else if (((Op->DestType == NoneOp) || (DecodeAdr(pDestArg, MModReg | (Op->DestType >= SingleOp ? MModFReg : 0), Op->DestType, &DReg, &DMode)))
        && (DecodeAdr(&ArgStr[1], MModReg | (Op->Src1Type >= SingleOp ? MModFReg : 0) | (Op->Imm1 ? MModImm : 0 ), Op->Src1Type, &S1Reg, &S1Mode))
        && ((Op->Src2Type == NoneOp) || (DecodeAdr(&ArgStr[2], MModReg | (Op->Src2Type >= SingleOp ? MModFReg : 0) | (Op->Imm2 ? MModImm : 0), Op->Src2Type, &S2Reg, &S2Mode))))
  {
    DAsmCode[0] = ((Op->Code & 0xff0) << 20)
                + ((Op->Code & 0xf) << 7)
                + (S1Reg)
                + (S2Reg << 14)
                + (DReg << 19)
                + (S1Mode << 11)
                + (S2Mode << 12)
                + (DMode << 13);
    CodeLen = 4;
    if ((Op->Privileged) && (!SupAllowed)) WrError(ErrNum_PrivOrder);
  }
}

static void DecodeCobr(Word Index)
{
  CobrOrder *Op = CobrOrders + Index;
  LongWord S1Reg, S1Mode;
  LongWord S2Reg = 0, S2Mode = 0;
  LongInt AdrInt;
  Boolean OK;
  tSymbolFlags Flags;
  unsigned NumArgs = 1 + 2 * Ord(Op->HasSrc);

  if (!ChkArgCnt(NumArgs, NumArgs));
  else if ((DecodeAdr(&ArgStr[1], MModReg | (Op->HasSrc ? MModImm : 0), IntOp, &S1Reg, &S1Mode))
        && ((!Op->HasSrc) || (DecodeAdr(&ArgStr[2], MModReg, IntOp, &S2Reg, &S2Mode))))
  {
    OK = True;
    Flags = eSymbolFlag_None;
    AdrInt = (Op->HasSrc) ? EvalStrIntExpressionWithFlags(&ArgStr[3], UInt32, &OK, &Flags) - EProgCounter() : 0;
    if (mFirstPassUnknown(Flags))
      AdrInt &= (~3);
    if (OK)
    {
      if (AdrInt & 3) WrError(ErrNum_NotAligned);
      else if (!mSymbolQuestionable(Flags) && ((AdrInt < -4096) || (AdrInt > 4090))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        DAsmCode[0] = (Op->Code << 24)
                    + (S1Reg << 19)
                    + (S2Reg << 14)
                    + (S1Mode << 13)
                    + (AdrInt & 0x1ffc);
        CodeLen = 4;
      }
    }
  }
}

static void DecodeCtrl(Word Index)
{
  FixedOrder *Op = CtrlOrders + Index;
  LongInt AdrInt;
  tSymbolFlags Flags;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt32, &OK, &Flags) - EProgCounter();
    if (mFirstPassUnknown(Flags)) AdrInt &= (~3);
    if (OK)
    {
      if (AdrInt & 3) WrError(ErrNum_NotAligned);
      else if (!mSymbolQuestionable(Flags) && ((AdrInt < -8388608) || (AdrInt > 8388604))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        DAsmCode[0] = (Op->Code << 24) + (AdrInt & 0xfffffc);
        CodeLen = 4;
      }
    }
  }
}

static void DecodeMemO(Word Index)
{
  MemOrder *Op = MemOrders + Index;
  LongWord Reg = 0, Mem = 0;
  int MemType;
  ShortInt MemPos = (Op->RegPos > 0) ? 3 - Op->RegPos : 1;
  unsigned NumArgs = 1 + Ord(Op->RegPos > 0);

  if (!ChkArgCnt(NumArgs, NumArgs));
  else if ((Op->RegPos > 0) && (DecodeIReg(&ArgStr[Op->RegPos], &Reg, True) != eIsReg));
  else if (Reg & OpMasks[Op->Type]) WrStrErrorPos(ErrNum_InvReg, &ArgStr[Op->RegPos]);
  else if ((MemType = DecodeMem(&ArgStr[MemPos], &Mem,DAsmCode + 1)) >= 0)
  {
    DAsmCode[0] = (Op->Code << 24) + (Reg << 19) + Mem;
    CodeLen = (1 + MemType) << 2;
  }
}

static void DecodeWORD(Word Code)
{
  Boolean OK;
  int z;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    z = 1;
    while ((z <= ArgCnt) && (OK))
    {
      DAsmCode[z - 1] = EvalStrIntExpression(&ArgStr[z], Int32, &OK);
      z++;
    }
    if (OK)
      CodeLen = 4 * ArgCnt;
  }
}

static void DecodeSPACE(Word Code)
{
  Boolean OK;
  LongWord Size;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (OK && !mFirstPassUnknown(Flags))
    {
      DontPrint = True;
      if (!Size) WrError(ErrNum_NullResMem);
      CodeLen = Size;
      BookKeeping();
    }
  }
}

/*--------------------------------------------------------------------------*/

static void MakeCode_960(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* Nullanweisung */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  /* Befehlszaehler nicht ausgerichtet? */

  if (EProgCounter() & 3)
    WrError(ErrNum_AddrNotAligned);

  /* CPU-Anweisungen */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*--------------------------------------------------------------------------*/

static void AddFixed(const char *NName, LongWord NCode)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddReg(const char *NName, LongWord NCode,
                   OpType NSrc1, OpType NSrc2, OpType NDest,
                   Boolean NImm1, Boolean NImm2, Boolean NPriv)
{
  order_array_rsv_end(RegOrders, RegOrder);
  RegOrders[InstrZ].Code = NCode;
  RegOrders[InstrZ].Src1Type = NSrc1;
  RegOrders[InstrZ].Src2Type = NSrc2;
  RegOrders[InstrZ].DestType = NDest;
  RegOrders[InstrZ].Imm1 = NImm1;
  RegOrders[InstrZ].Imm2 = NImm2;
  RegOrders[InstrZ].Privileged = NPriv;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg);
}

static void AddCobr(const char *NName, LongWord NCode, Boolean NHas)
{
  order_array_rsv_end(CobrOrders, CobrOrder);
  CobrOrders[InstrZ].Code = NCode;
  CobrOrders[InstrZ].HasSrc = NHas;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCobr);
}

static void AddCtrl(const char *NName, LongWord NCode)
{
  order_array_rsv_end(CtrlOrders, FixedOrder);
  CtrlOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCtrl);
}

static void AddMem(const char *NName, LongWord NCode, OpType NType, int NPos)
{
  order_array_rsv_end(MemOrders, MemOrder);
  MemOrders[InstrZ].Code = NCode;
  MemOrders[InstrZ].Type = NType;
  MemOrders[InstrZ].RegPos = NPos;
  AddInstTable(InstTable, NName, InstrZ++, DecodeMemO);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(301);

  InstrZ = 0;
  AddFixed("FLUSHREG", 0x66000680);
  AddFixed("FMARK"   , 0x66000600);
  AddFixed("MARK"    , 0x66000580);
  AddFixed("RET"     , 0x0a000000);
  AddFixed("SYNCF"   , 0x66000780);
  AddFixed("FAULTNO" , 0x18000000);
  AddFixed("FAULTG"  , 0x19000000);
  AddFixed("FAULTE"  , 0x1a000000);
  AddFixed("FAULTGE" , 0x1b000000);
  AddFixed("FAULTL"  , 0x1c000000);
  AddFixed("FAULTNE" , 0x1d000000);
  AddFixed("FAULTLE" , 0x1e000000);
  AddFixed("FAULTO"  , 0x1f000000);

  InstrZ = 0;
      /*  Name       OpCode Src1Type  Src2Type  DestType  Imm1   Imm2 */
  AddReg("ADDC"    , 0x5b0, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ADDI"    , 0x591, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ADDO"    , 0x590, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ADDR"    , 0x78f, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("ADDRL"   , 0x79f, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("ALTERBIT", 0x58f, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("AND"     , 0x581, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ANDNOT"  , 0x582, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ATADD"   , 0x612, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ATANR"   , 0x680, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("ATANRL"  , 0x690, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("ATMOD"   , 0x610, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("CALLS"   , 0x660, IntOp   , NoneOp  , NoneOp  , True , False, False);
  AddReg("CHKBIT"  , 0x5ae, IntOp   , IntOp   , NoneOp  , True , True , False);
  AddReg("CLASSR"  , 0x68f, SingleOp, NoneOp  , NoneOp  , True , False, False);
  AddReg("CLASSRL" , 0x69f, DoubleOp, NoneOp  , NoneOp  , True , False, False);
  AddReg("CLRBIT"  , 0x58c, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("CMPDECI" , 0x5a7, IntOp   , IntOp   , IntOp   , True , False, False);
  AddReg("CMPDECO" , 0x5a6, IntOp   , IntOp   , IntOp   , True , False, False);
  AddReg("CMPI"    , 0x5a1, IntOp   , IntOp   , NoneOp  , True , True , False);
  AddReg("CMPO"    , 0x5a0, IntOp   , IntOp   , NoneOp  , True , True , False);
  AddReg("CMPINCI" , 0x5a5, IntOp   , IntOp   , IntOp   , True , False, False);
  AddReg("CMPINCO" , 0x5a4, IntOp   , IntOp   , IntOp   , True , False, False);
  AddReg("CMPOR"   , 0x684, SingleOp, SingleOp, NoneOp  , True , True , False);
  AddReg("CMPORL"  , 0x694, DoubleOp, DoubleOp, NoneOp  , True , True , False);
  AddReg("CMPR"    , 0x685, SingleOp, SingleOp, NoneOp  , True , True , False);
  AddReg("CMPRL"   , 0x695, DoubleOp, DoubleOp, NoneOp  , True , True , False);
  AddReg("CONCMPI" , 0x5a3, IntOp   , IntOp   , NoneOp  , True , True , False);
  AddReg("CONCMPO" , 0x5a2, IntOp   , IntOp   , NoneOp  , True , True , False);
  AddReg("COSR"    , 0x68d, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("COSRL"   , 0x69d, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("CPYSRE"  , 0x6e2, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("CPYRSRE" , 0x6e3, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("CVTIR"   , 0x674, IntOp,    NoneOp  , SingleOp, True , False, False);
  AddReg("CVTILR"  , 0x675, LongOp,   NoneOp  , DoubleOp, True , False, False);
  AddReg("CVTRI"   , 0x6c0, SingleOp, NoneOp  , IntOp   , True , False, False);
  AddReg("CVTRIL"  , 0x6c1, SingleOp, NoneOp  , LongOp  , True , False, False);
  AddReg("CVTZRI"  , 0x6c2, IntOp   , NoneOp  , IntOp   , True , False, False);
  AddReg("CVTZRIL" , 0x6c2, LongOp  , NoneOp  , LongOp  , True , False, False);
      /*  Name       OpCode Src1Type  Src2Type  DestType  Imm1   Imm2 */
  AddReg("DADDC"   , 0x642, IntOp   , IntOp   , IntOp   , False, False, False);
  AddReg("DIVI"    , 0x74b, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("DIVO"    , 0x70b, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("DIVR"    , 0x78b, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("DIVRL"   , 0x79b, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("DMOVT"   , 0x644, IntOp   , NoneOp  , IntOp   , False, False, False);
  AddReg("DSUBC"   , 0x643, IntOp   , IntOp   , IntOp   , False, False, False);
  AddReg("EDIV"    , 0x671, IntOp   , LongOp  , LongOp  , True , True , False);
  AddReg("EMUL"    , 0x670, IntOp   , IntOp   , LongOp  , True , True , False);
  AddReg("EXPR"    , 0x689, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("EXPRL"   , 0x699, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("EXTRACT" , 0x651, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("LOGBNR"  , 0x68a, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("LOGBNRL" , 0x69a, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("LOGEPR"  , 0x681, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("LOGEPRL" , 0x691, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("LOGR"    , 0x682, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("LOGRL"   , 0x692, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("MODAC"   , 0x645, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("MODI"    , 0x749, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("MODIFY"  , 0x650, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("MODPC"   , 0x655, IntOp   , IntOp   , IntOp   , True , True , True );
  AddReg("MODTC"   , 0x654, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("MOV"     , 0x5cc, IntOp   , NoneOp  , IntOp   , True , False, False);
  AddReg("MOVL"    , 0x5dc, LongOp  , NoneOp  , LongOp  , True , False, False);
  AddReg("MOVT"    , 0x5ec, QuadOp  , NoneOp  , QuadOp  , True , False, False);
  AddReg("MOVQ"    , 0x5fc, QuadOp  , NoneOp  , QuadOp  , True , False, False);
  AddReg("MOVR"    , 0x6c9, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("MOVRL"   , 0x6d9, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("MOVRE"   , 0x6e1, ExtOp   , NoneOp  , ExtOp   , True , False, False);
  AddReg("MULI"    , 0x741, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("MULO"    , 0x701, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("MULR"    , 0x78c, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("MULRL"   , 0x79c, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("NAND"    , 0x58e, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("NOR"     , 0x588, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("NOT"     , 0x58a, IntOp   , NoneOp  , IntOp   , True , False, False);
  AddReg("NOTAND"  , 0x584, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("NOTBIT"  , 0x580, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("NOTOR"   , 0x58d, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("OR"      , 0x587, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ORNOT"   , 0x58b, IntOp   , IntOp   , IntOp   , True , True , False);
      /*  Name       OpCode TwoSrc HDest  Float     Imm1   Imm2 */
  AddReg("REMI"    , 0x748, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("REMO"    , 0x708, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("REMR"    , 0x683, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("REMRL"   , 0x693, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("ROTATE"  , 0x59d, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("ROUNDR"  , 0x68b, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("ROUNDRL" , 0x69b, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("SCALER"  , 0x677, IntOp   , SingleOp, SingleOp, True , True , False);
  AddReg("SCALERL" , 0x676, IntOp   , DoubleOp, DoubleOp, True , True , False);
  AddReg("SCANBIT" , 0x641, IntOp   , NoneOp  , IntOp   , True , False, False);
  AddReg("SCANBYTE", 0x5ac, IntOp   , NoneOp  , IntOp   , True , False, False);
  AddReg("SETBIT"  , 0x583, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SHLO"    , 0x59c, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SHRO"    , 0x598, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SHLI"    , 0x59e, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SHRI"    , 0x59B, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SHRDI"   , 0x59a, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SINR"    , 0x68c, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("SINRL"   , 0x69c, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("SPANBIT" , 0x640, IntOp   , NoneOp  , IntOp   , True , False, False);
  AddReg("SQRTR"   , 0x688, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("SQRTRL"  , 0x698, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("SUBC"    , 0x5b2, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SUBI"    , 0x593, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SUBO"    , 0x592, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("SUBR"    , 0x78d, SingleOp, SingleOp, SingleOp, True , True , False);
  AddReg("SUBRL"   , 0x79d, DoubleOp, DoubleOp, DoubleOp, True , True , False);
  AddReg("SYNLD"   , 0x615, IntOp   , NoneOp  , IntOp   , False, False, False);
  AddReg("SYNMOV"  , 0x600, IntOp   , NoneOp  , IntOp   , False, False, False);
  AddReg("SYNMOVL" , 0x601, IntOp   , NoneOp  , IntOp   , False, False, False);
  AddReg("SYNMOVQ" , 0x602, IntOp   , NoneOp  , IntOp   , False, False, False);
  AddReg("TANR"    , 0x68e, SingleOp, NoneOp  , SingleOp, True , False, False);
  AddReg("TANRL"   , 0x69e, DoubleOp, NoneOp  , DoubleOp, True , False, False);
  AddReg("XOR"     , 0x589, IntOp   , IntOp   , IntOp   , True , True , False);
  AddReg("XNOR"    , 0x589, IntOp   , IntOp   , IntOp   , True , True , False);

  InstrZ = 0;
  AddCobr("BBC"    , 0x30, True ); AddCobr("BBS"    , 0x37, True );
  AddCobr("CMPIBE" , 0x3a, True ); AddCobr("CMPOBE" , 0x32, True );
  AddCobr("CMPIBNE", 0x3d, True ); AddCobr("CMPOBNE", 0x35, True );
  AddCobr("CMPIBL" , 0x3c, True ); AddCobr("CMPOBL" , 0x34, True );
  AddCobr("CMPIBLE", 0x3e, True ); AddCobr("CMPOBLE", 0x36, True );
  AddCobr("CMPIBG" , 0x39, True ); AddCobr("CMPOBG" , 0x31, True );
  AddCobr("CMPIBGE", 0x3b, True ); AddCobr("CMPOBGE", 0x33, True );
  AddCobr("CMPIBO" , 0x3f, True ); AddCobr("CMPIBNO", 0x38, True );
  AddCobr("TESTE"  , 0x22, False); AddCobr("TESTNE" , 0x25, False);
  AddCobr("TESTL"  , 0x24, False); AddCobr("TESTLE" , 0x26, False);
  AddCobr("TESTG"  , 0x21, False); AddCobr("TESTGE" , 0x23, False);
  AddCobr("TESTO"  , 0x27, False); AddCobr("TESTNO" , 0x27, False);

  InstrZ = 0;
  AddCtrl("B"   , 0x08); AddCtrl("CALL", 0x09);
  AddCtrl("BAL" , 0x0b); AddCtrl("BNO" , 0x19);
  AddCtrl("BG"  , 0x11); AddCtrl("BE"  , 0x12);
  AddCtrl("BGE" , 0x13); AddCtrl("BL"  , 0x14);
  AddCtrl("BNE" , 0x15); AddCtrl("BLE" , 0x16);
  AddCtrl("BO"  , 0x17);

  InstrZ = 0;
  AddMem("LDOB" , 0x80, IntOp   , 2);
  AddMem("STOB" , 0x82, IntOp   , 1);
  AddMem("BX"   , 0x84, IntOp   , 0);
  AddMem("BALX" , 0x85, IntOp   , 2);
  AddMem("CALLX", 0x86, IntOp   , 0);
  AddMem("LDOS" , 0x88, IntOp   , 2);
  AddMem("STOS" , 0x8a, IntOp   , 1);
  AddMem("LDA"  , 0x8c, IntOp   , 2);
  AddMem("LD"   , 0x90, IntOp   , 2);
  AddMem("ST"   , 0x92, IntOp   , 1);
  AddMem("LDL"  , 0x98, LongOp  , 2);
  AddMem("STL"  , 0x9a, LongOp  , 1);
  AddMem("LDT"  , 0xa0, QuadOp  , 2);
  AddMem("STT"  , 0xa2, QuadOp  , 1);
  AddMem("LDQ"  , 0xb0, QuadOp  , 2);
  AddMem("STQ"  , 0xb2, QuadOp  , 1);
  AddMem("LDIB" , 0xc0, IntOp   , 2);
  AddMem("STIB" , 0xc2, IntOp   , 1);
  AddMem("LDIS" , 0xc8, IntOp   , 2);
  AddMem("STIS" , 0xca, IntOp   , 1);

  AddInstTable(InstTable, "WORD", 0, DecodeWORD);
  AddInstTable(InstTable, "SPACE", 0, DecodeSPACE);
  AddInstTable(InstTable, "REG", 0, CodeREG);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(RegOrders);
  order_array_free(CobrOrders);
  order_array_free(CtrlOrders);
}

/*--------------------------------------------------------------------------*/

static Boolean IsDef_960(void)
{
  return Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_960(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on i960
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_960(char *pArg, TempResult *pResult)
{
  LongWord Reg;

  if (DecodeIRegCore(pArg, &Reg))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_960;
    pResult->Contents.RegDescr.compare = NULL;
  }
  else if (DecodeFPRegCore(pArg, &Reg))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSizeFloat64Bit;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_960;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void SwitchTo_960(void)
{
  const TFamilyDescr *FoundId;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  FoundId = FindFamilyByName("i960");
  if (!FoundId)
    exit(255);
  PCSymbol = "$";
  HeaderID = FoundId->Id;
  NOPCode = 0x000000000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs=(1 << SegCode);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 4;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

  MakeCode = MakeCode_960;
  IsDef = IsDef_960;
  InternSymbol = InternSymbol_960;
  DissectReg = DissectReg_960;
  SwitchFrom = DeinitFields;
  onoff_fpu_add();
  onoff_supmode_add();

  InitFields();
}

void code960_init(void)
{
  CPU80960 = AddCPU("80960", SwitchTo_960);
}
