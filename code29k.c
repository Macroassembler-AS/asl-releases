/* code29k.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator AM29xxx-Familie                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "stringlists.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code29k.h"

typedef struct
{
  Boolean MustSup;
  CPUVar MinCPU;
  LongWord Code;
} StdOrder;

typedef struct
{
  Boolean HasReg, HasInd;
  CPUVar MinCPU;
  LongWord Code;
} JmpOrder;

typedef struct
{
  const char *Name;
  LongWord Code;
} SPReg;

#define REG_LRMARK 256

static StdOrder *StdOrders;
static StdOrder *NoImmOrders;
static StdOrder *VecOrders;
static JmpOrder *JmpOrders;
static StdOrder *FixedOrders;
static StdOrder *MemOrders;
static SPReg *SPRegs;


static CPUVar CPU29000, CPU29240, CPU29243, CPU29245;
static LongInt Reg_RBP;
static StringList Emulations;

/*-------------------------------------------------------------------------*/

static Boolean ChkSup(void)
{
  if (!SupAllowed)
    WrError(ErrNum_PrivOrder);
  return SupAllowed;
}

static Boolean IsSup(LongWord RegNo)
{
  return ((RegNo < 0x80) || (RegNo >= 0xa0));
}

static Boolean ChkCPU(CPUVar Min)
{
  return StringListPresent(Emulations, OpPart.str.p_str) ? True : ChkMinCPU(Min);
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, LongWord *pResult)
 * \brief  check whether argument describes a CPU register
 * \param  pArg source argument
 * \param  pResult resulting register # if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, LongWord *pResult)
{
  int l = strlen(pArg);
  Boolean OK;

  if ((l >= 2) && (as_toupper(*pArg) == 'R'))
  {
    *pResult = ConstLongInt(pArg + 1, &OK, 10);
    return OK && (*pResult <= 255);
  }
  else if ((l >= 3) && (as_toupper(*pArg) == 'G') && (as_toupper(pArg[1]) == 'R'))
  {
    *pResult = ConstLongInt(pArg + 2, &OK, 10);
    if (!OK || (*pResult >= 128))
      return False;
    *pResult |= REG_LRMARK;
    return True;
  }
  else if ((l >= 3) && (as_toupper(*pArg) == 'L') && (as_toupper(pArg[1]) == 'R'))
  {
    *pResult = ConstLongInt(pArg + 2, &OK, 10);
    if (!OK || (*pResult >= 128))
      return False;
    *pResult |= 128 | REG_LRMARK;
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_29K(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - 29K variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_29K(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
      if (Value & REG_LRMARK)
        as_snprintf(pDest, DestSize, "%cR%u", "GL"[(Value >> 7) & 1], (unsigned)(Value & 127));
      else
        as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     compare_reg_29k(tRegInt reg1_num, tSymbolSize reg1_size, tRegInt reg2_num, tRegInt reg2_size)
 * \brief  compare two register symbols
 * \param  reg1_num 1st register's number
 * \param  reg1_size 1st register's data size
 * \param  reg2_num 2nd register's number
 * \param  reg2_size 2nd register's data size
 * \return 0, -1, 1, -2
 * ------------------------------------------------------------------------ */

static int compare_reg_29k(tRegInt reg1_num, tSymbolSize size1, tRegInt reg2_num, tSymbolSize size2)
{
  if ((size1 != eSymbolSize32Bit) || (size2 != eSymbolSize32Bit))
    return -2;

  /* We have up to 512 registers, which conflicts with the generic ALIAS flag in bit 7.
     So we have to compare on our own.  R (0..255), GR (256...383), and LR (384...511)
     spaces are disjoint: */

  if ((reg1_num ^ reg2_num) & REG_LRMARK)
    return -2;

  if (reg1_num & REG_LRMARK)
  {
    if ((reg1_num ^ reg2_num) & 128)
      return -2;
    reg1_num &= 127;
    reg2_num &= 127;
  }
  else
  {
    reg1_num &= 255;
    reg2_num &= 255;
  }

  if (reg1_num < reg2_num)
    return -1;
  else if (reg1_num > reg2_num)
    return 1;
  else
    return 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, LongWord *pResult, Boolean MustBeReg)
 * \brief  check whether argument describes a CPU register
 * \param  pArg source argument
 * \param  pResult resulting register # if yes
 * \param  MustBeReg True if register is expected
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, LongWord *pResult, Boolean MustBeReg)
{
  tRegEvalResult RegEvalResult;
  tEvalResult EvalResult;
  tRegDescr RegDescr;

  if (DecodeRegCore(pArg->str.p_str, pResult))
    RegEvalResult = eIsReg;
  else
  {
    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
    if (eIsReg == RegEvalResult)
      *pResult = RegDescr.Reg;
  }

  if (eIsReg == RegEvalResult)
  {
    *pResult &= ~REG_LRMARK;
    if ((*pResult < 127) && (Odd(Reg_RBP >> (*pResult >> 4))))
    {
      if (!ChkSup())
        RegEvalResult = MustBeReg ? eIsNoReg : eRegAbort;
    }
  }
  return RegEvalResult;
}

static Boolean DecodeArgReg(int ArgIndex, LongWord *pRes)
{
  return DecodeReg(&ArgStr[ArgIndex], pRes, True);
}

static Boolean DecodeSpReg(char *Asc_O, LongWord *Erg)
{
  int z;
  String Asc;

  strmaxcpy(Asc, Asc_O, STRINGSIZE);
  NLS_UpString(Asc);
  for (z = 0; SPRegs[z].Name; z++)
    if (!strcmp(Asc, SPRegs[z].Name))
    {
      *Erg = SPRegs[z].Code;
      return True;;
    }
  return False;;
}

static Boolean DecodeArgSpReg(int ArgIndex, LongWord *pRes)
{
  Boolean Result = DecodeSpReg(ArgStr[ArgIndex].str.p_str, pRes);

  if (!Result)
    WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[ArgIndex]);
  return Result;
}

/*-------------------------------------------------------------------------*/

/* Variante 1: Register <-- Register op Register/uimm8 */

static void DecodeStd(Word Index)
{
  const StdOrder *pOrder = StdOrders + Index;
  LongWord Dest, Src1, Src2, Src3;
  Boolean OK;

  if (ChkArgCnt(2, 3) && DecodeArgReg(1, &Dest))
  {
    OK = True;
    if (ArgCnt == 2) Src1 = Dest;
    else OK = DecodeArgReg(2, &Src1);
    if (OK)
    {
      switch (DecodeReg(&ArgStr[ArgCnt], &Src2, False))
      {
        case eIsReg:
          OK = True; Src3 = 0;
          break;
        case eIsNoReg:
          Src2 = EvalStrIntExpression(&ArgStr[ArgCnt], UInt8, &OK);
          Src3 = 0x1000000;
          break;
        case eRegAbort:
          return;
      }
      if (OK)
      {
        CodeLen = 4;
        DAsmCode[0] = (pOrder->Code << 24) + Src3 + (Dest << 16) + (Src1 << 8) + Src2;
        if (pOrder->MustSup)
          ChkSup();
      }
    }
  }
}

/* Variante 2: Register <-- Register op Register */

static void DecodeNoImm(Word Index)
{
  const StdOrder *pOrder = NoImmOrders + Index;
  Boolean OK;
  LongWord Dest, Src1, Src2;

  if (ChkArgCnt(2, 3) && DecodeArgReg(1, &Dest))
  {
    OK = True;
    if (ArgCnt == 2) Src1 = Dest;
    else OK = DecodeArgReg(2, &Src1);
    if (OK && DecodeArgReg(ArgCnt, &Src2))
    {
      CodeLen = 4;
      DAsmCode[0] = (pOrder->Code << 24) + (Dest << 16) + (Src1 << 8) + Src2;
      if (pOrder->MustSup)
        ChkSup();
    }
  }
}

/* Variante 3: Vektor <-- Register op Register/uimm8 */

static void DecodeVec(Word Index)
{
  const StdOrder *pOrder = VecOrders + Index;
  Boolean OK;
  tSymbolFlags Flags;
  LongWord Dest, Src1, Src2, Src3;

  if (ChkArgCnt(3, 3))
  {
    Dest = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt8, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) Dest = 64;
    if (OK)
    {
      if (DecodeArgReg(2, &Src1))
      {
        switch (DecodeReg(&ArgStr[ArgCnt], &Src2, False))
        {
          case eIsReg:
            OK = True; Src3 = 0;
            break;
          case eIsNoReg:
            Src2 = EvalStrIntExpression(&ArgStr[ArgCnt], UInt8, &OK);
            Src3 = 0x1000000;
            break;
          default:
            return;
        }
        if (OK)
        {
          CodeLen = 4;
          DAsmCode[0] = (pOrder->Code << 24) + Src3 + (Dest << 16) + (Src1 << 8) + Src2;
          if ((pOrder->MustSup) || (Dest <= 63))
            ChkSup();
        }
      }
    }
  }
}

/* Variante 4: ohne Operanden */

static void DecodeFixed(Word Code)
{
  const StdOrder *pOrder = FixedOrders + Code;

  if (ChkArgCnt(0, 0))
  {
    CodeLen = 4;
    DAsmCode[0] = pOrder->Code << 24;
    if (pOrder->MustSup)
      ChkSup();
  }
}

/* Variante 5 : [0], Speichersteuerwort, Register, Register/uimm8 */

static void DecodeMem(Word Index)
{
  const StdOrder *pOrder = MemOrders + Index;
  Boolean OK;
  LongWord AdrLong, Dest, Src1, Src2, Src3;

  if (ChkArgCnt(3, 4))
  {
    if (ArgCnt == 3)
    {
      OK = True; AdrLong = 0;
    }
    else
    {
      AdrLong = EvalStrIntExpression(&ArgStr[1], Int32, &OK);
      if (OK) OK = ChkRange(AdrLong, 0, 0);
    }
    if (OK)
    {
      Dest = EvalStrIntExpression(&ArgStr[ArgCnt - 2], UInt7, &OK);
      if (OK && DecodeArgReg(ArgCnt - 1, &Src1))
      {
        switch (DecodeReg(&ArgStr[ArgCnt], &Src2, False))
        {
          case eIsReg:
            OK = True; Src3 = 0;
            break;
          case eIsNoReg:
            Src2 = EvalStrIntExpression(&ArgStr[ArgCnt], UInt8, &OK);
            Src3 = 0x1000000;
            break;
          default:
            return;
        }
        if (OK)
        {
          CodeLen = 4;
          DAsmCode[0] = (pOrder->Code << 24) + Src3 + (Dest << 16) + (Src1 << 8) + Src2;
          if (pOrder->MustSup)
            ChkSup();
        }
      }
    }
  }
}

/* Sprungbefehle */

static void DecodeJmp(Word Index)
{
  const JmpOrder *pOrder = JmpOrders + (Index & 0xff);
  Word Immediate = Index & 0x100;
  LongWord Dest, Src1, AdrLong;
  LongInt AdrInt;
  Boolean OK;
  tSymbolFlags Flags;
  unsigned NumArgs = 1 + Ord(pOrder->HasReg);

  if (!ChkArgCnt(NumArgs, NumArgs))
    return;

  switch (DecodeReg(&ArgStr[ArgCnt], &Src1, False))
  {
    case eIsReg:
      if (!pOrder->HasReg)
      {
        Dest = 0;
        OK = True;
      }
      else
        OK = DecodeReg(&ArgStr[1], &Dest, True);
      if (OK)
      {
        CodeLen = 4;
        DAsmCode[0] = ((pOrder->Code + 0x20) << 24) + (Dest << 8) + Src1;
      }
      break;
    case eRegAbort:
      return;
    case eIsNoReg:
      if (Immediate) WrError(ErrNum_InvAddrMode);
      else
      {
        if (!pOrder->HasReg)
        {
          Dest = 0;
          OK = True;
        }
        else
          OK = DecodeReg(&ArgStr[1], &Dest, True);
        if (OK)
        {
          AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int32, &OK, &Flags);
          AdrInt = AdrLong - EProgCounter();
          if (OK)
          {
            if ((AdrLong & 3) != 0) WrError(ErrNum_NotAligned);
             else if ((AdrInt <= 0x1ffff) && (AdrInt >= -0x20000))
            {
              CodeLen = 4;
              AdrLong -= EProgCounter();
              DAsmCode[0] = (pOrder->Code << 24)
                          + ((AdrLong & 0x3fc00) << 6)
                          + (Dest << 8) + ((AdrLong & 0x3fc) >> 2);
            }
            else if (!mSymbolQuestionable(Flags) && (AdrLong > 0x3fffff)) WrError(ErrNum_JmpDistTooBig);
            else
            {
              CodeLen = 4;
              DAsmCode[0] = ((pOrder->Code + 1) << 24)
                          + ((AdrLong & 0x3fc00) << 6)
                          + (Dest << 8) + ((AdrLong & 0x3fc) >> 2);
            }
          }
        }
      }
  }
}

static void DecodeCLASS(Word Code)
{
  LongWord Dest, Src1, Src2;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && ChkCPU(CPU29000)
   && DecodeArgReg(1, &Dest)
   && DecodeArgReg(2, &Src1))
  {
    Src2 = EvalStrIntExpression(&ArgStr[3], UInt2, &OK);
    if (OK)
    {
      CodeLen = 4;
      DAsmCode[0] = 0xe6000000 + (Dest << 16) + (Src1 << 8) + Src2;
    }
  }
}

static void DecodeEMULATE(Word Code)
{
  LongWord Dest, Src1, Src2;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(3, 3))
  {
    Dest = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt8, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) Dest = 64;
    if (OK)
    {
      if (DecodeArgReg(2, &Src1)
       && DecodeArgReg(ArgCnt, &Src2))
      {
        CodeLen = 4;
        DAsmCode[0] = 0xd7000000 + (Dest << 16) + (Src1 << 8) + Src2;
        if (Dest <= 63)
          ChkSup();
      }
    }
  }
}

static void DecodeSQRT(Word Code)
{
  Boolean OK;
  LongWord Src1, Src2, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 3)
   && ChkCPU(CPU29000)
   && DecodeArgReg(1, &Dest))
  {
    if (ArgCnt == 2)
    {
      OK = True;
      Src1 = Dest;
    }
    else
      OK = DecodeArgReg(2, &Src1);
    if (OK)
    {
      Src2 = EvalStrIntExpression(&ArgStr[ArgCnt], UInt2, &OK);
      if (OK)
      {
        CodeLen = 4;
        DAsmCode[0] = 0xe5000000 + (Dest << 16) + (Src1 << 8) + Src2;
      }
    }
  }
}

static void DecodeCLZ(Word Code)
{
  Boolean OK;
  LongWord Src1, Src3, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 2) && DecodeArgReg(1, &Dest))
  {
    switch (DecodeReg(&ArgStr[2], &Src1, False))
    {
      case eIsReg:
        OK = True; Src3 = 0;
        break;
      case eIsNoReg:
        Src1 = EvalStrIntExpression(&ArgStr[2], UInt8, &OK);
        Src3 = 0x1000000;
        break;
      default:
        return;
    }
    if (OK)
    {
      CodeLen = 4;
      DAsmCode[0] = 0x08000000 + Src3 + (Dest << 16) + Src1;
    }
  }
}

static void DecodeCONST(Word Code)
{
  Boolean OK;
  LongWord AdrLong, Dest;
  UNUSED(Code);

  if (ChkArgCnt(2, 2) && DecodeArgReg(1, &Dest))
  {
    AdrLong = EvalStrIntExpression(&ArgStr[2], Int32, &OK);
    if (OK)
    {
      CodeLen = 4;
      DAsmCode[0] = ((AdrLong & 0xff00) << 8) + (Dest << 8) + (AdrLong & 0xff);
      AdrLong = AdrLong >> 16;
      if (AdrLong == 0xffff) DAsmCode[0] += 0x01000000;
      else
      {
        DAsmCode[0] += 0x03000000;
        if (AdrLong != 0)
        {
          CodeLen = 8;
          DAsmCode[1] = 0x02000000 + ((AdrLong & 0xff00) << 16) + (Dest << 8) + (AdrLong & 0xff);
        }
      }
    }
  }
  return;
}

static void DecodeCONSTH_CONSTN(Word IsHi)
{
  Boolean OK;
  LongWord AdrLong, Dest;
  tSymbolFlags Flags;

  if (ChkArgCnt(2, 2) && DecodeArgReg(1, &Dest))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[2], Int32, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      AdrLong &= 0xffff;
    if ((!IsHi) && ((AdrLong >> 16) == 0xffff))
      AdrLong &= 0xffff;
    if (ChkRange(AdrLong, 0, 0xffff))
    {
      CodeLen = 4;
      DAsmCode[0] = 0x1000000 + ((AdrLong & 0xff00) << 8) + (Dest << 8) + (AdrLong & 0xff);
      if (IsHi)
        DAsmCode[0] += 0x1000000;
    }
  }
}

static void DecodeCONVERT(Word Code)
{
  Boolean OK;
  LongWord Src1, Src2, Dest;

  UNUSED(Code);

  if (ChkArgCnt(6, 6)
   && ChkCPU(CPU29000)
   && DecodeArgReg(1, &Dest)
   && DecodeArgReg(2, &Src1))
  {
    Src2 = 0;
    Src2 += EvalStrIntExpression(&ArgStr[3], UInt1, &OK) << 7;
    if (OK)
    {
      Src2 += EvalStrIntExpression(&ArgStr[4], UInt3, &OK) << 4;
      if (OK)
      {
        Src2 += EvalStrIntExpression(&ArgStr[5], UInt2, &OK) << 2;
        if (OK)
        {
          Src2 += EvalStrIntExpression(&ArgStr[6], UInt2, &OK);
          if (OK)
          {
            CodeLen = 4;
            DAsmCode[0] = 0xe4000000 + (Dest << 16) + (Src1 << 8) + Src2;
          }
        }
      }
    }
  }
}

static void DecodeEXHWS(Word Code)
{
  LongWord Src1, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeArgReg(1, &Dest)
   && DecodeArgReg(2, &Src1))
  {
    CodeLen = 4;
    DAsmCode[0] = 0x7e000000 + (Dest << 16) + (Src1 << 8);
  }
}

static void DecodeINV_IRETINV(Word Code)
{
  Boolean OK;
  LongWord Src1;

  if (ChkArgCnt(0, 1))
  {
    if (ArgCnt == 0)
    {
      Src1 = 0; OK = True;
    }
    else Src1 = EvalStrIntExpression(&ArgStr[1], UInt2, &OK);
    if (OK)
    {
      CodeLen = 4;
      DAsmCode[0] = (((LongWord)Code) << 16) | Src1 << 16;
      ChkSup();
    }
  }
}

static void DecodeMFSR(Word Code)
{
  LongWord Src1, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeArgReg(1, &Dest)
   && DecodeArgSpReg(2, &Src1))
  {
    DAsmCode[0] = 0xc6000000 + (Dest << 16) + (Src1 << 8);
    CodeLen = 4;
    if (IsSup(Src1))
      ChkSup();
  }
}

static void DecodeMTSR(Word Code)
{
  LongWord Src1, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeArgSpReg(1, &Dest)
   && DecodeArgReg(2, &Src1))
  {
    DAsmCode[0] = 0xce000000 + (Dest << 8) + Src1;
    CodeLen = 4;
    if (IsSup(Dest))
      ChkSup();
  }
}

static void DecodeMTSRIM(Word Code)
{
  LongWord Src1, Dest;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(2, 2) && DecodeArgSpReg(1, &Dest))
  {
    Src1 = EvalStrIntExpression(&ArgStr[2], UInt16, &OK);
    if (OK)
    {
      DAsmCode[0] = 0x04000000 + ((Src1 & 0xff00) << 8) + (Dest << 8) + Lo(Src1);
      CodeLen = 4;
      if (IsSup(Dest))
        ChkSup();
    }
  }
}

static void DecodeMFTLB(Word Code)
{
  LongWord Src1, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeArgReg(1, &Dest)
   && DecodeArgReg(2, &Src1))
  {
    DAsmCode[0] = 0xb6000000 + (Dest << 16) + (Src1 << 8);
    CodeLen = 4;
    ChkSup();
  }
}

static void DecodeMTTLB(Word Code)
{
  LongWord Src1, Dest;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeArgReg(1, &Dest)
   && DecodeArgReg(2, &Src1))
  {
    DAsmCode[0] = 0xbe000000 + (Dest << 8) + Src1;
    CodeLen = 4;
    ChkSup();
  }
}

static void DecodeEMULATED(Word Code)
{
  int z;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
    for (z = 1; z <= ArgCnt; z++)
    {
      NLS_UpString(ArgStr[z].str.p_str);
      if (!StringListPresent(Emulations, ArgStr[z].str.p_str))
        AddStringListLast(&Emulations, ArgStr[z].str.p_str);
    }
}

/*-------------------------------------------------------------------------*/

static void AddStd(const char *NName, CPUVar NMin, Boolean NSup, LongWord NCode)
{
  order_array_rsv_end(StdOrders, StdOrder);
  StdOrders[InstrZ].Code = NCode;
  StdOrders[InstrZ].MustSup = NSup;
  StdOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeStd);
}

static void AddNoImm(const char *NName, CPUVar NMin, Boolean NSup, LongWord NCode)
{
  order_array_rsv_end(NoImmOrders, StdOrder);
  NoImmOrders[InstrZ].Code = NCode;
  NoImmOrders[InstrZ].MustSup = NSup;
  NoImmOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeNoImm);
}

static void AddVec(const char *NName, CPUVar NMin, Boolean NSup, LongWord NCode)
{
  order_array_rsv_end(VecOrders, StdOrder);
  VecOrders[InstrZ].Code = NCode;
  VecOrders[InstrZ].MustSup = NSup;
  VecOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeVec);
}

static void AddJmp(const char *NName, CPUVar NMin, Boolean NHas, Boolean NInd, LongWord NCode)
{
  char IName[30];

  order_array_rsv_end(JmpOrders, JmpOrder);
  JmpOrders[InstrZ].HasReg = NHas;
  JmpOrders[InstrZ].HasInd = NInd;
  JmpOrders[InstrZ].Code = NCode;
  JmpOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ, DecodeJmp);
  as_snprintf(IName, sizeof(IName), "%sI", NName);
  AddInstTable(InstTable, IName, 0x100 | InstrZ, DecodeJmp);
  InstrZ++;
}

static void AddFixed(const char *NName, CPUVar NMin, Boolean NSup, LongWord NCode)
{
  order_array_rsv_end(FixedOrders, StdOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].MustSup = NSup;
  FixedOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddMem(const char *NName, CPUVar NMin, Boolean NSup, LongWord NCode)
{
  order_array_rsv_end(MemOrders, StdOrder);
  MemOrders[InstrZ].Code = NCode;
  MemOrders[InstrZ].MustSup = NSup;
  MemOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeMem);
}

static void AddSP(const char *NName, LongWord NCode)
{
  order_array_rsv_end(SPRegs, SPReg);
  SPRegs[InstrZ].Name = NName;
  SPRegs[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(307);
  SetDynamicInstTable(InstTable);
  AddInstTable(InstTable, "CLASS", 0, DecodeCLASS);
  AddInstTable(InstTable, "EMULATE", 0, DecodeEMULATE);
  AddInstTable(InstTable, "SQRT", 0, DecodeSQRT);
  AddInstTable(InstTable, "CLZ", 0, DecodeCLZ);
  AddInstTable(InstTable, "CONST", 0, DecodeCONST);
  AddInstTable(InstTable, "CONSTH", True, DecodeCONSTH_CONSTN);
  AddInstTable(InstTable, "CONSTN", False, DecodeCONSTH_CONSTN);
  AddInstTable(InstTable, "CONVERT", 0, DecodeCONVERT);
  AddInstTable(InstTable, "EXHWS", 0, DecodeEXHWS);
  AddInstTable(InstTable, "INV", 0x9f00, DecodeINV_IRETINV);
  AddInstTable(InstTable, "IRETINV", 0x8c00, DecodeINV_IRETINV);
  AddInstTable(InstTable, "MFSR", 0, DecodeMFSR);
  AddInstTable(InstTable, "MTSR", 0, DecodeMTSR);
  AddInstTable(InstTable, "MTSRIM", 0, DecodeMTSRIM);
  AddInstTable(InstTable, "MFTLB", 0, DecodeMFTLB);
  AddInstTable(InstTable, "MTTLB", 0, DecodeMTTLB);
  AddInstTable(InstTable, "EMULATED", 0, DecodeEMULATED);
  AddInstTable(InstTable, "REG", 0, CodeREG);

  InstrZ = 0;
  AddStd("ADD"    , CPU29245, False, 0x14); AddStd("ADDC"   , CPU29245, False, 0x1c);
  AddStd("ADDCS"  , CPU29245, False, 0x18); AddStd("ADDCU"  , CPU29245, False, 0x1a);
  AddStd("ADDS"   , CPU29245, False, 0x10); AddStd("ADDU"   , CPU29245, False, 0x12);
  AddStd("AND"    , CPU29245, False, 0x90); AddStd("ANDN"   , CPU29245, False, 0x9c);
  AddStd("CPBYTE" , CPU29245, False, 0x2e); AddStd("CPEQ"   , CPU29245, False, 0x60);
  AddStd("CPGE"   , CPU29245, False, 0x4c); AddStd("CPGEU"  , CPU29245, False, 0x4e);
  AddStd("CPGT"   , CPU29245, False, 0x48); AddStd("CPGTU"  , CPU29245, False, 0x4a);
  AddStd("CPLE"   , CPU29245, False, 0x44); AddStd("CPLEU"  , CPU29245, False, 0x46);
  AddStd("CPLT"   , CPU29245, False, 0x40); AddStd("CPLTU"  , CPU29245, False, 0x42);
  AddStd("CPNEQ"  , CPU29245, False, 0x62); AddStd("DIV"    , CPU29245, False, 0x6a);
  AddStd("DIV0"   , CPU29245, False, 0x68); AddStd("DIVL"   , CPU29245, False, 0x6c);
  AddStd("DIVREM" , CPU29245, False, 0x6e); AddStd("EXBYTE" , CPU29245, False, 0x0a);
  AddStd("EXHW"   , CPU29245, False, 0x7c); AddStd("EXTRACT", CPU29245, False, 0x7a);
  AddStd("INBYTE" , CPU29245, False, 0x0c); AddStd("INHW"   , CPU29245, False, 0x78);
  AddStd("MUL"    , CPU29245, False, 0x64); AddStd("MULL"   , CPU29245, False, 0x66);
  AddStd("MULU"   , CPU29245, False, 0x74); AddStd("NAND"   , CPU29245, False, 0x9a);
  AddStd("NOR"    , CPU29245, False, 0x98); AddStd("OR"     , CPU29245, False, 0x92);
  AddStd("SLL"    , CPU29245, False, 0x80); AddStd("SRA"    , CPU29245, False, 0x86);
  AddStd("SRL"    , CPU29245, False, 0x82); AddStd("SUB"    , CPU29245, False, 0x24);
  AddStd("SUBC"   , CPU29245, False, 0x2c); AddStd("SUBCS"  , CPU29245, False, 0x28);
  AddStd("SUBCU"  , CPU29245, False, 0x2a); AddStd("SUBR"   , CPU29245, False, 0x34);
  AddStd("SUBRC"  , CPU29245, False, 0x3c); AddStd("SUBRCS" , CPU29245, False, 0x38);
  AddStd("SUBRCU" , CPU29245, False, 0x3a); AddStd("SUBRS"  , CPU29245, False, 0x30);
  AddStd("SUBRU"  , CPU29245, False, 0x32); AddStd("SUBS"   , CPU29245, False, 0x20);
  AddStd("SUBU"   , CPU29245, False, 0x22); AddStd("XNOR"   , CPU29245, False, 0x96);
  AddStd("XOR"    , CPU29245, False, 0x94);

  InstrZ = 0;
  AddNoImm("DADD"    , CPU29000, False, 0xf1); AddNoImm("DDIV"    , CPU29000, False, 0xf7);
  AddNoImm("DEQ"     , CPU29000, False, 0xeb); AddNoImm("DGE"     , CPU29000, False, 0xef);
  AddNoImm("DGT"     , CPU29000, False, 0xed); AddNoImm("DIVIDE"  , CPU29000, False, 0xe1);
  AddNoImm("DIVIDU"  , CPU29000, False, 0xe3); AddNoImm("DMUL"    , CPU29000, False, 0xf5);
  AddNoImm("DSUB"    , CPU29000, False, 0xf3); AddNoImm("FADD"    , CPU29000, False, 0xf0);
  AddNoImm("FDIV"    , CPU29000, False, 0xf6); AddNoImm("FDMUL"   , CPU29000, False, 0xf9);
  AddNoImm("FEQ"     , CPU29000, False, 0xea); AddNoImm("FGE"     , CPU29000, False, 0xee);
  AddNoImm("FGT"     , CPU29000, False, 0xec); AddNoImm("FMUL"    , CPU29000, False, 0xf4);
  AddNoImm("FSUB"    , CPU29000, False, 0xf2); AddNoImm("MULTIPLU", CPU29243, False, 0xe2);
  AddNoImm("MULTIPLY", CPU29243, False, 0xe0); AddNoImm("MULTM"   , CPU29243, False, 0xde);
  AddNoImm("MULTMU"  , CPU29243, False, 0xdf); AddNoImm("SETIP"   , CPU29245, False, 0x9e);

  InstrZ = 0;
  AddVec("ASEQ"   , CPU29245, False, 0x70); AddVec("ASGE"   , CPU29245, False, 0x5c);
  AddVec("ASGEU"  , CPU29245, False, 0x5e); AddVec("ASGT"   , CPU29245, False, 0x58);
  AddVec("ASGTU"  , CPU29245, False, 0x5a); AddVec("ASLE"   , CPU29245, False, 0x54);
  AddVec("ASLEU"  , CPU29245, False, 0x56); AddVec("ASLT"   , CPU29245, False, 0x50);
  AddVec("ASLTU"  , CPU29245, False, 0x52); AddVec("ASNEQ"  , CPU29245, False, 0x72);

  InstrZ = 0;
  AddJmp("CALL"   , CPU29245, True , True , 0xa8); AddJmp("JMP"    , CPU29245, False, True , 0xa0);
  AddJmp("JMPF"   , CPU29245, True , True , 0xa4); AddJmp("JMPFDEC", CPU29245, True , False, 0xb4);
  AddJmp("JMPT"   , CPU29245, True , True , 0xac);

  InstrZ = 0;
  AddFixed("HALT"   , CPU29245, True, 0x89); AddFixed("IRET"   , CPU29245, True, 0x88);

  InstrZ = 0;
  AddMem("LOAD"   , CPU29245, False, 0x16); AddMem("LOADL"  , CPU29245, False, 0x06);
  AddMem("LOADM"  , CPU29245, False, 0x36); AddMem("LOADSET", CPU29245, False, 0x26);
  AddMem("STORE"  , CPU29245, False, 0x1e); AddMem("STOREL" , CPU29245, False, 0x0e);
  AddMem("STOREM" , CPU29245, False, 0x3e);

  InstrZ = 0;
  AddSP("VAB",   0);
  AddSP("OPS",   1);
  AddSP("CPS",   2);
  AddSP("CFG",   3);
  AddSP("CHA",   4);
  AddSP("CHD",   5);
  AddSP("CHC",   6);
  AddSP("RBP",   7);
  AddSP("TMC",   8);
  AddSP("TMR",   9);
  AddSP("PC0",  10);
  AddSP("PC1",  11);
  AddSP("PC2",  12);
  AddSP("MMU",  13);
  AddSP("LRU",  14);
  AddSP("CIR",  29);
  AddSP("CDR",  30);
  AddSP("IPC", 128);
  AddSP("IPA", 129);
  AddSP("IPB", 130);
  AddSP("Q",   131);
  AddSP("ALU", 132);
  AddSP("BP",  133);
  AddSP("FC",  134);
  AddSP("CR",  135);
  AddSP("FPE", 160);
  AddSP("INTE",161);
  AddSP("FPS", 162);
  AddSP(NULL ,   0);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(StdOrders);
  order_array_free(NoImmOrders);
  order_array_free(VecOrders);
  order_array_free(JmpOrders);
  order_array_free(FixedOrders);
  order_array_free(MemOrders);
  order_array_free(SPRegs);
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_29K(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on 29K
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_29K(char *pArg, TempResult *pResult)
{
  LongWord Reg;

  if (DecodeRegCore(pArg, &Reg))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_29K;
    pResult->Contents.RegDescr.compare = compare_reg_29k;
  }
}

static void MakeCode_29K(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* Nullanweisung */

  if (Memo("") && !*AttrPart.str.p_str && (ArgCnt == 0))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_29K(void)
{
  Reg_RBP = 0;
  ClearStringList(&Emulations);
}

static Boolean IsDef_29K(void)
{
  return Memo("REG");
}

static void SwitchTo_29K(void)
{
  static ASSUMERec ASSUME29Ks[] =
  {
    {"RBP", &Reg_RBP, 0, 0xff, 0x00000000, NULL}
  };
  static const int ASSUME29KCount = sizeof(ASSUME29Ks) / sizeof(*ASSUME29Ks);
  TurnWords = True;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "$";
  HeaderID = 0x29;
  NOPCode = 0x000000000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 4; SegInits[SegCode] = 0;
  SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

  MakeCode = MakeCode_29K;
  IsDef = IsDef_29K;
  InternSymbol = InternSymbol_29K;
  DissectReg = DissectReg_29K;
  onoff_supmode_add();

  pASSUMERecs = ASSUME29Ks;
  ASSUMERecCnt = ASSUME29KCount;

  SwitchFrom = DeinitFields; InitFields();
}

void code29k_init(void)
{
  CPU29245 = AddCPU("AM29245", SwitchTo_29K);
  CPU29243 = AddCPU("AM29243", SwitchTo_29K);
  CPU29240 = AddCPU("AM29240", SwitchTo_29K);
  CPU29000 = AddCPU("AM29000", SwitchTo_29K);

  Emulations = NULL;

  AddInitPassProc(InitCode_29K);
}
