/* codemcore.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator MCORE-Familie                                               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "nls.h"
#include "endian.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "errmsg.h"

#include "codemcore.h"

/*--------------------------------------------------------------------------*/
/* Variablen */

#define REG_SP 0
#define REG_LR 15

typedef struct
{
  Word Code;
  Boolean Priv;
} FixedOrder;

typedef struct
{
  Word Code;
  Word Min,Ofs;
} ImmOrder;

typedef struct
{
  const char *Name;
  Word Code;
} CReg;

static CPUVar CPUMCORE;
static tSymbolSize OpSize;

static FixedOrder *FixedOrders;
static FixedOrder *OneRegOrders;
static FixedOrder *TwoRegOrders;
static ImmOrder *UImm5Orders;
static FixedOrder *LJmpOrders;
static CReg *CRegs;

/*--------------------------------------------------------------------------*/
/* Hilfsdekoder */

static const Word AllRegMask = 0xffff;

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Word *pResult)
 * \brief  check whether argument is a register
 * \param  pArg argument
 * \param  pResult register number if it is
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Word *pResult)
{
  if (!as_strcasecmp(pArg, "SP"))
    *pResult = REGSYM_FLAG_ALIAS | REG_SP;
  else if (!as_strcasecmp(pArg, "LR"))
    *pResult = REGSYM_FLAG_ALIAS | REG_LR;
  else if (as_toupper(*pArg) != 'R')
    return False;
  else
  {
    char *endptr;

    *pResult = strtol(pArg + 1, &endptr, 10);
    if (*endptr || (*pResult > 15))
      return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_MCORE(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - M-CORE variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_MCORE(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
      switch (Value)
      {
        case REGSYM_FLAG_ALIAS | REG_SP:
          as_snprintf(pDest, DestSize, "SP");
          break;
        case REGSYM_FLAG_ALIAS | REG_LR:
          as_snprintf(pDest, DestSize, "LR");
          break;
        default:
          as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      }
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Word *pResult, Word Mask, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg argument
 * \param  pResult register number if it is
 * \param  Mask bit mask of allowed registers
 * \param  MustBeReg operand is expected to be a register
 * \return True if it is an allowed register
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Word *pResult, Word Mask, Boolean MustBeReg)
{
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult))
    RegEvalResult = eIsReg;
  else
  {
    tRegDescr RegDescr;
    tEvalResult EvalResult;

    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
    *pResult = RegDescr.Reg;
  }
  *pResult &= ~REGSYM_FLAG_ALIAS;
  if ((RegEvalResult == eIsReg) && !(Mask & (1 << *pResult)))
  {
    RegEvalResult = MustBeReg ? eRegAbort : eIsNoReg;
    WrStrErrorPos(ErrNum_InvReg, pArg);
  }
  return RegEvalResult;
}

static Boolean DecodeArgReg(int Index, Word *pErg, Word Mask)
{
  return (DecodeReg(&ArgStr[Index], pErg, Mask, True) == eIsReg);
}

static Boolean DecodeArgIReg(int Index, Word *pErg, Word Mask)
{
  tStrComp RegComp;
  const char *pArg = ArgStr[Index].str.p_str;
  int l = strlen(pArg);

  if ((l <= 3) || (pArg[0] != '(') || (pArg[l - 1] != ')'))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[Index]);
    return False;
  }
  StrCompRefRight(&RegComp, &ArgStr[Index], 1);
  StrCompShorten(&RegComp, 1);
  return (DecodeReg(&RegComp, pErg, Mask, True) == eIsReg);
}

static Boolean DecodeArgRegPair(int Index, Word *pFrom, Word FromMask, Word *pTo, Word ToMask)
{
  tStrComp FromComp, ToComp;
  char *pSep = strchr(ArgStr[Index].str.p_str, '-');

  if (!pSep)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[Index]);
    return False;
  }

  StrCompSplitRef(&FromComp, &ToComp, &ArgStr[Index], pSep);
  return (DecodeReg(&FromComp, pFrom, FromMask, True) == eIsReg)
      && (DecodeReg(&ToComp, pTo, ToMask, True) == eIsReg);
}

static Boolean DecodeCReg(char *Asc, Word *Erg)
{
  char *endptr;
  int z;

  for (z = 0; CRegs[z].Name; z++)
    if (!as_strcasecmp(Asc, CRegs[z].Name))
    {
      *Erg = CRegs[z].Code;
      return True;
    }

  if ((as_toupper(*Asc) != 'C') || (as_toupper(Asc[1]) != 'R'))
    return False;
  else
  {
    *Erg = strtol(Asc + 2, &endptr, 10);
    return ((*endptr == '\0') && (*Erg <= 31));
  }
}

static Boolean DecodeArgCReg(int Index, Word *pErg)
{
  Boolean Result = DecodeCReg(ArgStr[Index].str.p_str, pErg);

  if (!Result)
    WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[Index]);
  return Result;
}

static Boolean DecodeAdr(const tStrComp *pArg, Word *Erg)
{
  Word Base = 0xff, Tmp;
  LongInt DispAcc = 0, DMask = (1 << OpSize) - 1, DMax = 15 << OpSize;
  Boolean OK, FirstFlag = False;
  tSymbolFlags Flags;
  char *Pos;
  tStrComp Arg, Remainder;

  if (!IsIndirect(pArg->str.p_str))
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }

  StrCompRefRight(&Arg,pArg, 1);
  StrCompShorten(&Arg, 1);
  do
  {
    Pos = QuotPos(Arg.str.p_str,',');
    if (Pos)
      StrCompSplitRef(&Arg, &Remainder, &Arg, Pos);
    switch (DecodeReg(&Arg, &Tmp, AllRegMask, False))
    {
      case eIsReg:
        if (Base == 0xff) Base = Tmp;
        else
        {
          WrError(ErrNum_InvAddrMode);
          return False;
        }
        break;
      case eIsNoReg:
        DispAcc += EvalStrIntExpressionWithFlags(&Arg, Int32, &OK, &Flags);
        if (mFirstPassUnknown(Flags)) FirstFlag = True;
        if (!OK)
          return False;
        break;
      default:
        return False;
    }
    if (Pos)
      Arg = Remainder;
  }
  while (Pos);

  if (Base == 0xff)
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }

  if (FirstFlag)
  {
    DispAcc -= DispAcc & DMask;
    if (DispAcc < 0) DispAcc = 0;
    if (DispAcc > DMax) DispAcc = DMax;
  }

  if ((DispAcc & DMask) != 0)
  {
    WrError(ErrNum_NotAligned);
    return False;
  }
  if (!ChkRange(DispAcc, 0, DMax))
    return False;
  *Erg = Base + ((DispAcc >> OpSize) << 4);
  return True;
}

static void DecodeFixed(Word Index)
{
  FixedOrder *Instr = FixedOrders + Index;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(0, 0))
  {
    if ((Instr->Priv) && (!SupAllowed)) WrError(ErrNum_PrivOrder);
    WAsmCode[0] = Instr->Code;
    CodeLen = 2;
  }
}

static void DecodeOneReg(Word Index)
{
  FixedOrder *Instr = OneRegOrders + Index;
  Word RegX;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(1, 1) && DecodeArgReg(1, &RegX, AllRegMask))
  {
    if ((Instr->Priv) && (!SupAllowed)) WrError(ErrNum_PrivOrder);
    WAsmCode[0] = Instr->Code + RegX;
    CodeLen = 2;
  }
}

static void DecodeTwoReg(Word Index)
{
  FixedOrder *Instr = TwoRegOrders + Index;
  Word RegX, RegY;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2)
        && DecodeArgReg(1, &RegX, AllRegMask)
        && DecodeArgReg(2, &RegY, AllRegMask))
  {
    if ((Instr->Priv) && (!SupAllowed)) WrError(ErrNum_PrivOrder);
    WAsmCode[0] = Instr->Code + (RegY << 4) + RegX;
    CodeLen = 2;
  }
}

static void DecodeUImm5(Word Index)
{
  ImmOrder *Instr = UImm5Orders + Index;
  Word RegX, ImmV;
  Boolean OK;
  tSymbolFlags Flags;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2) && DecodeArgReg(1, &RegX, AllRegMask))
  {
    ImmV = EvalStrIntExpressionWithFlags(&ArgStr[2], (Instr->Ofs > 0) ? UInt6 : UInt5, &OK, &Flags);
    if ((Instr->Min > 0) && (ImmV < Instr->Min))
    {
      if (mFirstPassUnknown(Flags)) ImmV = Instr->Min;
      else
      {
        WrError(ErrNum_UnderRange); OK = False;
      }
    }
    if ((Instr->Ofs > 0) && ((ImmV < Instr->Ofs) || (ImmV > 31 + Instr->Ofs)))
    {
      if (mFirstPassUnknown(Flags)) ImmV = Instr->Ofs;
      else
      {
        WrError((ImmV < Instr->Ofs) ? ErrNum_UnderRange : ErrNum_OverRange);
        OK = False;
      }
    }
    if (OK)
    {
      WAsmCode[0] = Instr->Code + ((ImmV - Instr->Ofs) << 4) + RegX;
      CodeLen = 2;
    }
  }
}

static void DecodeLJmp(Word Index)
{
  FixedOrder *Instr = LJmpOrders + Index;
  LongInt Dest;
  Boolean OK;
  tSymbolFlags Flags;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(1, 1))
  {
    Dest = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt32, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && (Dest & 1)) WrError(ErrNum_DistIsOdd);
      else if (!mSymbolQuestionable(Flags) && ((Dest > 2046) || (Dest < -2048))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        if ((Instr->Priv) && (!SupAllowed)) WrError(ErrNum_PrivOrder);
        WAsmCode[0] = Instr->Code + ((Dest >> 1) & 0x7ff);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeSJmp(Word Index)
{
  LongInt Dest;
  Boolean OK;
  tSymbolFlags Flags;
  int l = 0;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if ((*ArgStr[1].str.p_str != '[') || (ArgStr[1].str.p_str[l = strlen(ArgStr[1].str.p_str) - 1] != ']')) WrError(ErrNum_InvAddrMode);
  else
  {
    ArgStr[1].str.p_str[l] = '\0';
    Dest = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, UInt32, &OK, &Flags);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && (Dest & 3)) WrError(ErrNum_NotAligned);
      else
      {
        Dest = (Dest - (EProgCounter() + 2)) >> 2;
        if ((EProgCounter() & 3) < 2) Dest++;
        if (!mSymbolQuestionable(Flags) && ((Dest < 0) || (Dest > 255))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          WAsmCode[0] = 0x7000 + (Index << 8) + (Dest & 0xff);
          CodeLen = 2;
        }
      }
    }
  }
}

static void DecodeBGENI(Word Index)
{
  Word RegX, ImmV;
  Boolean OK;
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2) && DecodeArgReg(1, &RegX, AllRegMask))
  {
    ImmV = EvalStrIntExpression(&ArgStr[2], UInt5, &OK);
    if (OK)
    {
      if (ImmV > 6)
        WAsmCode[0] = 0x3200 + (ImmV << 4) + RegX;
      else
        WAsmCode[0] = 0x6000 + (1 << (4 + ImmV)) + RegX;
      CodeLen = 2;
    }
  }
}

static void DecodeBMASKI(Word Index)
{
  Word RegX, ImmV;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2) && DecodeArgReg(1, &RegX, AllRegMask))
  {
    ImmV = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt6, &OK, &Flags);
    if (mFirstPassUnknown(Flags) && ((ImmV < 1) || (ImmV > 32))) ImmV = 8;
    if (OK)
    {
      if (ChkRange(ImmV, 1, 32))
      {
        ImmV &= 31;
        if ((ImmV < 1) || (ImmV > 7))
          WAsmCode[0] = 0x2c00 + (ImmV << 4) + RegX;
        else
          WAsmCode[0] = 0x6000 + (((1 << ImmV) - 1) << 4) + RegX;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeLdSt(Word Index)
{
  Word RegX, RegZ, NSize;

  if (*AttrPart.str.p_str && (Lo(Index) != 0xff)) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(2, 2));
  else if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else
  {
    if (Lo(Index) != 0xff) OpSize = (tSymbolSize)Lo(Index);
    if (DecodeArgReg(1, &RegZ, AllRegMask) && DecodeAdr(&ArgStr[2], &RegX))
    {
      NSize = (OpSize == eSymbolSize32Bit) ? 0 : OpSize + 1;
      WAsmCode[0] = 0x8000 + (NSize << 13) + (Hi(Index) << 12) + (RegZ << 8) + RegX;
      CodeLen = 2;
    }
  }
}

static void DecodeLdStm(Word Index)
{
  Word RegF, RegL, RegI;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2)
        && DecodeArgIReg(2, &RegI, 0x0001)
        && DecodeArgRegPair(1, &RegF, 0x7ffe, &RegL, 0x8000))
  {
    WAsmCode[0] = 0x0060 + (Index << 4) + RegF;
    CodeLen = 2;
  }
}

static void DecodeLdStq(Word Index)
{
  Word RegF, RegL, RegX;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2)
        && DecodeArgIReg(2, &RegX, 0xff0f)
        && DecodeArgRegPair(1, &RegF, 0x0010, &RegL, 0x0080))
  {
    WAsmCode[0] = 0x0040 + (Index << 4) + RegX;
    CodeLen = 2;
  }
}

static void DecodeLoopt(Word Index)
{
  Word RegY;
  LongInt Dest;
  Boolean OK;
  tSymbolFlags Flags;
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2) && DecodeArgReg(1, &RegY, AllRegMask))
  {
    Dest = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt32, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && (Dest & 1)) WrError(ErrNum_DistIsOdd);
      else if (!mSymbolQuestionable(Flags) && ((Dest > -2) || (Dest <- 32))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        WAsmCode[0] = 0x0400 + (RegY << 4) + ((Dest >> 1) & 15);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeLrm(Word Index)
{
  LongInt Dest;
  Word RegZ;
  Boolean OK;
  tSymbolFlags Flags;
  int l = 0;
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(2, 2));
  else if (!DecodeArgReg(1, &RegZ, 0x7ffe));
  else if ((*ArgStr[2].str.p_str != '[') || (ArgStr[2].str.p_str[l = strlen(ArgStr[2].str.p_str) - 1] != ']')) WrError(ErrNum_InvAddrMode);
  else
  {
    ArgStr[2].str.p_str[l] = '\0';
    Dest = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], 1, UInt32, &OK, &Flags);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && (Dest & 3)) WrError(ErrNum_NotAligned);
      else
      {
        Dest = (Dest - (EProgCounter() + 2)) >> 2;
        if ((EProgCounter() & 3) < 2) Dest++;
        if (!mSymbolQuestionable(Flags) && ((Dest < 0) || (Dest > 255))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          WAsmCode[0] = 0x7000 + (RegZ << 8) + (Dest & 0xff);
          CodeLen = 2;
        }
      }
    }
  }
}

static void DecodeMcr(Word Index)
{
  Word RegX,CRegY;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2)
        && DecodeArgReg(1, &RegX, AllRegMask)
        && DecodeArgCReg(2, &CRegY))
  {
    if (!SupAllowed) WrError(ErrNum_PrivOrder);
    WAsmCode[0] = 0x1000 + (Index << 11) + (CRegY << 4) + RegX;
    CodeLen = 2;
  }
}

static void DecodeMovi(Word Index)
{
  Word RegX, ImmV;
  Boolean OK;
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2) && DecodeArgReg(1, &RegX, AllRegMask))
  {
    ImmV = EvalStrIntExpression(&ArgStr[2], UInt7, &OK);
    if (OK)
    {
      WAsmCode[0] = 0x6000 + ((ImmV & 127) << 4) + RegX;
      CodeLen = 2;
    }
  }
}

static void DecodeTrap(Word Index)
{
  Word ImmV;
  Boolean OK;
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    ImmV = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt2, &OK);
    if (OK)
    {
      WAsmCode[0] = 0x0008 + ImmV;
      CodeLen = 2;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Codetabellenverwaltung */

static void AddFixed(const char *NName, Word NCode, Boolean NPriv)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].Priv = NPriv;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddOneReg(const char *NName, Word NCode, Boolean NPriv)
{
  order_array_rsv_end(OneRegOrders, FixedOrder);
  OneRegOrders[InstrZ].Code = NCode;
  OneRegOrders[InstrZ].Priv = NPriv;
  AddInstTable(InstTable, NName, InstrZ++, DecodeOneReg);
}

static void AddTwoReg(const char *NName, Word NCode, Boolean NPriv)
{
  order_array_rsv_end(TwoRegOrders, FixedOrder);
  TwoRegOrders[InstrZ].Code = NCode;
  TwoRegOrders[InstrZ].Priv = NPriv;
  AddInstTable(InstTable, NName, InstrZ++, DecodeTwoReg);
}

static void AddUImm5(const char *NName, Word NCode, Word NMin, Word NOfs)
{
  order_array_rsv_end(UImm5Orders, ImmOrder);
  UImm5Orders[InstrZ].Code = NCode;
  UImm5Orders[InstrZ].Min = NMin;
  UImm5Orders[InstrZ].Ofs = NOfs;
  AddInstTable(InstTable, NName, InstrZ++, DecodeUImm5);
}

static void AddLJmp(const char *NName, Word NCode, Boolean NPriv)
{
  order_array_rsv_end(LJmpOrders, FixedOrder);
  LJmpOrders[InstrZ].Code = NCode;
  LJmpOrders[InstrZ].Priv = NPriv;
  AddInstTable(InstTable, NName, InstrZ++, DecodeLJmp);
}

static void AddCReg(const char *NName, Word NCode)
{
  order_array_rsv_end(CRegs, CReg);
  CRegs[InstrZ].Name = NName;
  CRegs[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "REG", 0, CodeREG);

  InstrZ = 0;
  AddFixed("BKPT" , 0x0000, False);
  AddFixed("DOZE" , 0x0006, True );
  AddFixed("RFI"  , 0x0003, True );
  AddFixed("RTE"  , 0x0002, True );
  AddFixed("STOP" , 0x0004, True );
  AddFixed("SYNC" , 0x0001, False);
  AddFixed("WAIT" , 0x0005, True );

  InstrZ = 0;
  AddOneReg("ABS"   , 0x01e0, False);  AddOneReg("ASRC" , 0x3a00, False);
  AddOneReg("BREV"  , 0x00f0, False);  AddOneReg("CLRF" , 0x01d0, False);
  AddOneReg("CLRT"  , 0x01c0, False);  AddOneReg("DECF" , 0x0090, False);
  AddOneReg("DECGT" , 0x01a0, False);  AddOneReg("DECLT", 0x0180, False);
  AddOneReg("DECNE" , 0x01b0, False);  AddOneReg("DECT" , 0x0080, False);
  AddOneReg("DIVS"  , 0x3210, False);  AddOneReg("DIVU" , 0x2c10, False);
  AddOneReg("FF1"   , 0x00e0, False);  AddOneReg("INCF" , 0x00b0, False);
  AddOneReg("INCT"  , 0x00a0, False);  AddOneReg("JMP"  , 0x00c0, False);
  AddOneReg("JSR"   , 0x00d0, False);  AddOneReg("LSLC" , 0x3c00, False);
  AddOneReg("LSRC"  , 0x3e00, False);  AddOneReg("MVC"  , 0x0020, False);
  AddOneReg("MVCV"  , 0x0030, False);  AddOneReg("NOT"  , 0x01f0, False);
  AddOneReg("SEXTB" , 0x0150, False);  AddOneReg("SEXTH", 0x0170, False);
  AddOneReg("TSTNBZ", 0x0190, False);  AddOneReg("XSR"  , 0x3800, False);
  AddOneReg("XTRB0" , 0x0130, False);  AddOneReg("XTRB1", 0x0120, False);
  AddOneReg("XTRB2" , 0x0110, False);  AddOneReg("XTRB3", 0x0100, False);
  AddOneReg("ZEXTB" , 0x0140, False);  AddOneReg("ZEXTH", 0x0160, False);

  InstrZ = 0;
  AddTwoReg("ADDC" , 0x0600, False);  AddTwoReg("ADDU" , 0x1c00, False);
  AddTwoReg("AND"  , 0x1600, False);  AddTwoReg("ANDN" , 0x1f00, False);
  AddTwoReg("ASR"  , 0x1a00, False);  AddTwoReg("BGENR", 0x1300, False);
  AddTwoReg("CMPHS", 0x0c00, False);  AddTwoReg("CMPLT", 0x0d00, False);
  AddTwoReg("CMPNE", 0x0f00, False);  AddTwoReg("IXH"  , 0x1d00, False);
  AddTwoReg("IXW"  , 0x1500, False);  AddTwoReg("LSL"  , 0x1b00, False);
  AddTwoReg("LSR"  , 0x0b00, False);  AddTwoReg("MOV"  , 0x1200, False);
  AddTwoReg("MOVF" , 0x0a00, False);  AddTwoReg("MOVT" , 0x0200, False);
  AddTwoReg("MULT" , 0x0300, False);  AddTwoReg("OR"   , 0x1e00, False);
  AddTwoReg("RSUB" , 0x1400, False);  AddTwoReg("SUBC" , 0x0700, False);
  AddTwoReg("SUBU" , 0x0500, False);  AddTwoReg("TST"  , 0x0e00, False);
  AddTwoReg("XOR"  , 0x1700, False);

  InstrZ = 0;
  AddUImm5("ADDI"  , 0x2000, 0, 1);  AddUImm5("ANDI"  , 0x2e00, 0, 0);
  AddUImm5("ASRI"  , 0x3a00, 1, 0);  AddUImm5("BCLRI" , 0x3000, 0, 0);
  AddUImm5("BSETI" , 0x3400, 0, 0);  AddUImm5("BTSTI" , 0x3600, 0, 0);
  AddUImm5("CMPLTI", 0x2200, 0, 1);  AddUImm5("CMPNEI", 0x2a00, 0, 0);
  AddUImm5("LSLI"  , 0x3c00, 1, 0);  AddUImm5("LSRI"  , 0x3e00, 1, 0);
  AddUImm5("ROTLI" , 0x3800, 1, 0);  AddUImm5("RSUBI" , 0x2800, 0, 0);
  AddUImm5("SUBI"  , 0x2400, 0, 1);

  InstrZ = 0;
  AddLJmp("BF"   , 0xe800, False);  AddLJmp("BR"   , 0xf000, False);
  AddLJmp("BSR"  , 0xf800, False);  AddLJmp("BT"   , 0xe000, False);

  InstrZ = 0;
  AddCReg("PSR" , 0); AddCReg("VBR" , 1);
  AddCReg("EPSR", 2); AddCReg("FPSR", 3);
  AddCReg("EPC" , 4); AddCReg("FPC",  5);
  AddCReg("SS0",  6); AddCReg("SS1",  7);
  AddCReg("SS2",  8); AddCReg("SS3",  9);
  AddCReg("SS4", 10); AddCReg("GCR", 11);
  AddCReg("GSR", 12); AddCReg(NULL , 0);

  AddInstTable(InstTable, "BGENI" , 0, DecodeBGENI);
  AddInstTable(InstTable, "BMASKI", 0, DecodeBMASKI);
  AddInstTable(InstTable, "JMPI"  , 0, DecodeSJmp);
  AddInstTable(InstTable, "JSRI"  , 0, DecodeSJmp);
  AddInstTable(InstTable, "LD"    , 0x0ff, DecodeLdSt);
  AddInstTable(InstTable, "LDB"   , 0x000, DecodeLdSt);
  AddInstTable(InstTable, "LDH"   , 0x001, DecodeLdSt);
  AddInstTable(InstTable, "LDW"   , 0x002, DecodeLdSt);
  AddInstTable(InstTable, "ST"    , 0x1ff, DecodeLdSt);
  AddInstTable(InstTable, "STB"   , 0x100, DecodeLdSt);
  AddInstTable(InstTable, "STH"   , 0x101, DecodeLdSt);
  AddInstTable(InstTable, "STW"   , 0x102, DecodeLdSt);
  AddInstTable(InstTable, "LDM"   , 0, DecodeLdStm);
  AddInstTable(InstTable, "STM"   , 1, DecodeLdStm);
  AddInstTable(InstTable, "LDQ"   , 0, DecodeLdStq);
  AddInstTable(InstTable, "STQ"   , 1, DecodeLdStq);
  AddInstTable(InstTable, "LOOPT" , 0, DecodeLoopt);
  AddInstTable(InstTable, "LRM"   , 0, DecodeLrm);
  AddInstTable(InstTable, "MFCR"  , 0, DecodeMcr);
  AddInstTable(InstTable, "MTCR"  , 1, DecodeMcr);
  AddInstTable(InstTable, "MOVI"  , 0, DecodeMovi);
  AddInstTable(InstTable, "TRAP"  , 0, DecodeTrap);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(OneRegOrders);
  order_array_free(TwoRegOrders);
  order_array_free(UImm5Orders);
  order_array_free(LJmpOrders);
  order_array_free(CRegs);
}

/*--------------------------------------------------------------------------*/
/* Callbacks */

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_MCORE(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for M-CORE
 * \param  pArg source argument
 * \param  pResult buffer for possible result
 * ------------------------------------------------------------------------ */

static void InternSymbol_MCORE(char *pArg, TempResult *pResult)
{
  Word RegNum;

  if (DecodeRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Dissect = DissectReg_MCORE;
    pResult->Contents.RegDescr.compare = NULL;
    pResult->Contents.RegDescr.Reg = RegNum;
  }
}

static Boolean DecodeAttrPart_MCORE(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }
  /* operand size identifiers slightly differ from '68K Standard': */

  switch (as_toupper(*AttrPart.str.p_str))
  {
    case 'H': AttrPartOpSize[0] = eSymbolSize16Bit; break;
    case 'W': AttrPartOpSize[0] = eSymbolSize32Bit; break;
    case 'L': WrStrErrorPos(ErrNum_UndefAttr, &AttrPart); return False;
    default:
      return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
  }
  return True;
}

static void MakeCode_MCORE(void)
{
  CodeLen = 0;

  OpSize = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSize32Bit;
  DontPrint = False;

  /* Nullanweisung */

  if ((*OpPart.str.p_str == '\0') && !*AttrPart.str.p_str && (ArgCnt == 0)) return;

  /* Pseudoanweisungen */

  if (DecodeMoto16Pseudo(OpSize,True)) return;

  /* Befehlszaehler ungerade ? */

  if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);

  /* alles aus der Tabelle */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_MCORE(void)
{
  return Memo("REG");
}

static void SwitchTo_MCORE(void)
{
  TurnWords = True;
  SetIntConstMode(eIntConstModeMoto);

   PCSymbol = "*"; HeaderID = 0x03; NOPCode = 0x1200; /* ==MOV r0,r0 */
   DivideChars = ","; HasAttrs = True; AttrChars = ".";

   ValidSegs = (1 << SegCode);
   Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
   SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

   DecodeAttrPart = DecodeAttrPart_MCORE;
   MakeCode = MakeCode_MCORE;
   IsDef = IsDef_MCORE;
   InternSymbol = InternSymbol_MCORE;
   DissectReg = DissectReg_MCORE;

   SwitchFrom = DeinitFields; InitFields();
   onoff_supmode_add();
   AddMoto16PseudoONOFF(True);
}

/*--------------------------------------------------------------------------*/
/* Initialisierung */

void codemcore_init(void)
{
  CPUMCORE = AddCPU("MCORE", SwitchTo_MCORE);
}
