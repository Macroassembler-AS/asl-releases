/* code77230.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Makroassembler AS                                                         */
/*                                                                           */
/* Codegenerator NEC uPD77230                                                */
/*                                                                           */
/*****************************************************************************/

/*---------------------------------------------------------------------------*/
/* Includes */

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "strutil.h"
#include "nls.h"
#include "endian.h"
#include "ieeefloat.h"
#include "bpemu.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "headids.h"
#include "codevars.h"
#include "codepseudo.h"
#include "onoff_common.h"
#include "errmsg.h"
#include "chartrans.h"

#include "code77230.h"

/*---------------------------------------------------------------------------*/
/* Definitionen */

#define CaseCnt 17

typedef struct
{
  LongWord Code;
} FixedOrder;

typedef struct
{
  const char *Name;
  LongWord Code;
} Register;

enum
{
  InstrLDI, InstrBranch,
  InstrALU, InstrMove,
  InstrM0, InstrM1, InstrDP0, InstrDP1,
  InstrEA, InstrRP, InstrFC, InstrLC,
  InstrBASE0, InstrBASE1, InstrRPC,
  InstrP2, InstrP3, InstrEM, InstrBM,
  InstrL, InstrRW, InstrWT, InstrNF, InstrWI,
  InstrFIS, InstrFD, InstrSHV, InstrRPS, InstrNAL, InstrCnt
};

static LongWord CaseMasks[CaseCnt] =
{
  (1l << InstrLDI),
  (1l << InstrBranch) | (1l << InstrMove),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrM0) | (1l << InstrM1) | (1l << InstrDP0) | (1l << InstrDP1),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrEA) | (1l << InstrDP0) | (1l << InstrDP1),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrRP) | (1l << InstrM0) | (1l << InstrDP0) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrRP) | (1l << InstrM1) | (1l << InstrDP1) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrRP) | (1l << InstrM0) | (1l << InstrM1) | (1l << InstrL) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrBASE0) | (1l << InstrBASE1) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrRPC) | (1l << InstrL) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrP3) | (1l << InstrP2) | (1l << InstrEM) | (1l << InstrBM) | (1l << InstrL) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrRW) | (1l << InstrL) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrWT) | (1l << InstrL) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrNF) | (1l << InstrWI) | (1l << InstrL) | (1l << InstrFC),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrFIS) | (1l << InstrFD) | (1l << InstrL),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrSHV),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrRPS),
  (1l << InstrALU) | (1l << InstrMove) | (1l << InstrNAL)
};

static CPUVar CPU77230;
static LongWord InstrMask;
static Boolean Error;
static LongWord *InstrComps, *InstrDefs;

static Register *SrcRegs, *ALUSrcRegs, *DestRegs;
static FixedOrder *JmpOrders, *ALU1Orders, *ALU2Orders;

/*---------------------------------------------------------------------------*/
/* Hilfsroutinen */

static int DiscCnt, SplittedArg;
static char *DiscPtr;

static Boolean SplitArgs(int Count)
{
  char *p, *p1, *p2;

  SplittedArg = DiscCnt = Count;

  if (Count == 0)
  {
    if (ArgCnt > 0)
    {
      DiscPtr = ArgStr[1].str.p_str - 1;
      SplittedArg = 1;
    }
    else
      DiscPtr = NULL;
    return True;
  }

  if (!ChkArgCnt(Count, ArgCntMax))
  {
    Error = True;
    return False;
  }

  for (p = ArgStr[SplittedArg].str.p_str; isspace(((usint)*p) & 0xff); p++);
  p1 = QuotPos(p, ' ');
  p2 = QuotPos(p, '\t');
  DiscPtr = ((!p1) || ((p2) && (p2 < p1))) ? p2 : p1;
  if (DiscPtr)
    *(DiscPtr) = '\0';

  return True;
}

static void DiscardArgs(void)
{
  char *p, *p2;
  int z;
  Boolean Eaten;

  if (DiscPtr)
  {
    for (p = DiscPtr + 1; as_isspace(*p); p++)
      if (*p == '\0') break;
    for (p2 = p; !as_isspace(*p2); p2++)
      if (*p2 == '\0') break;
    Eaten = (*p2 == '\0');
    *p2 = '\0';
    strmov(OpPart.str.p_str, p);
    NLS_UpString(OpPart.str.p_str);
    if (Eaten)
    {
      for (z = 1; z < ArgCnt; z++)
        strmov(ArgStr[z].str.p_str, ArgStr[z + 1].str.p_str);
      ArgCnt--;
    }
    else
    {
      if (p2)
        for (p2++; as_isspace(*p2); p2++);
      strmov(ArgStr[SplittedArg].str.p_str, p2);
    }
  }
  else
    *OpPart.str.p_str = '\0';
  if (DiscCnt > 0)
  {
    for (z = 0; z <= ArgCnt - DiscCnt; z++)
      strmov(ArgStr[z + 1].str.p_str, ArgStr[z + DiscCnt].str.p_str);
    ArgCnt -= DiscCnt - 1;
  }
}

static void AddComp(int Index, LongWord Value)
{
  if ((InstrMask & (1l << Index)) != 0)
  {
    WrError(ErrNum_InvParAddrMode);
    Error = True;
  }
  else
  {
    InstrMask |= (1l << Index);
    InstrComps[Index] = Value;
  }
}

static Boolean DecodeReg(char *Asc, LongWord *Erg, Register *Regs)
{
  int z;

  for (z = 0; Regs[z].Name; z++)
    if (!as_strcasecmp(Asc, Regs[z].Name))
    {
      *Erg = Regs[z].Code;
      return True;
    }
  *Erg = 0;
  return False;
}

/*---------------------------------------------------------------------------*/
/* Dekoder fuer Routinen */

static void DecodeJmp(Word Index)
{
  FixedOrder *Op = JmpOrders + Index;
  int acnt = Hi(Op->Code);
  LongWord Adr;

  if (!SplitArgs(acnt))
    return;
  if (acnt == 0)
  {
    Adr = 0;
    Error = True;
  }
  else
    Adr = EvalStrIntExpression(&ArgStr[1], UInt13, &Error);
  Error = !Error;
  if (!Error)
    AddComp(InstrBranch, Lo(Op->Code) + (Adr << 5));
  DiscardArgs();
}

static void DecodeMOV(Word Index)
{
  LongWord DReg, SReg;
  UNUSED(Index);

  if (!SplitArgs(2))
    return;
  if (!DecodeReg(ArgStr[1].str.p_str, &DReg, DestRegs))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    Error = True;
  }
  else if (!DecodeReg(ArgStr[2].str.p_str, &SReg, SrcRegs))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
    Error = True;
  }
  else
    AddComp(InstrMove, (SReg << 5) + DReg);
  DiscardArgs();
}

static void DecodeLDI(Word Index)
{
  LongWord DReg, Src = 0;
  UNUSED(Index);

  if (!SplitArgs(2))
    return;
  if (!DecodeReg(ArgStr[1].str.p_str, &DReg, DestRegs))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    Error = True;
  }
  else
    Src = EvalStrIntExpression(&ArgStr[2], Int24, &Error);
  Error = !Error;
  if (!Error)
    AddComp(InstrLDI, (Src << 5) + DReg);
  DiscardArgs();
}

static void DecodeNOP(Word Index)
{
  UNUSED(Index);

  if (!SplitArgs(0))
    return;
  AddComp(InstrALU, 0);
  DiscardArgs();
}

static void DecodeALU1(Word Index)
{
  FixedOrder *Op = ALU1Orders + Index;
  LongWord DReg;

  if (!SplitArgs(1))
    return;
  if ((!DecodeReg(ArgStr[1].str.p_str, &DReg, DestRegs))
   || (DReg < 16) || (DReg > 23))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    Error = True;
  }
  else
    AddComp(InstrALU, (Op->Code << 17) + (DReg & 7));
  DiscardArgs();
}

static void DecodeALU2(Word Index)
{
  FixedOrder *Op = ALU2Orders + Index;
  LongWord DReg, SReg;

  if (!SplitArgs(2)) return;
  if ((!DecodeReg(ArgStr[1].str.p_str, &DReg, DestRegs))
   || (DReg < 16) || (DReg > 23))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    Error = True;
  }
  else if (!DecodeReg(ArgStr[2].str.p_str, &SReg, ALUSrcRegs))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
    Error = True;
  }
  else
    AddComp(InstrALU, (Op->Code << 17) + (SReg << 3) + (DReg & 7));
  DiscardArgs();
}

static void DecodeM0(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrM0, Index);
  DiscardArgs();
}

static void DecodeM1(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrM1, Index);
  DiscardArgs();
}

static void DecodeDP0(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrDP0, Index);
  DiscardArgs();
}

static void DecodeDP1(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrDP1, Index);
  DiscardArgs();
}

static void DecodeEA(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrEA, Index);
  DiscardArgs();
}

static void DecodeFC(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrFC, Index);
  DiscardArgs();
}

static void DecodeRP(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrRP, Index);
  DiscardArgs();
}

static void DecodeL(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrL, Index);
  DiscardArgs();
}

static void DecodeBASE(Word Index)
{
  LongWord Value;

  if (!SplitArgs(1))
    return;
  Value = EvalStrIntExpression(&ArgStr[1], UInt3, &Error);
  Error = !Error;
  if (!Error)
    AddComp(Index, Value);
  DiscardArgs();
}

static void DecodeRPC(Word Index)
{
  LongWord Value;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (!SplitArgs(1))
    return;

  Value = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt4, &Error, &Flags);
  if (mFirstPassUnknown(Flags))
    Value &= 7;
  Error = (Value > 9) ? True : !Error;
  if (!Error)
    AddComp(InstrRPC, Value);
  DiscardArgs();
}

static void DecodeP2(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrP2, Index);
  DiscardArgs();
}

static void DecodeP3(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrP3, Index);
  DiscardArgs();
}

static void DecodeBM(Word Index)
{
  /* Wenn EM-Feld schon da war, muss es EI gewesen sein */

  if (!SplitArgs(0))
    return;

  if ((InstrMask & (1 << InstrEM)) != 0)
  {
    Error = (InstrComps[InstrEM] == 0);
    if (Error) WrError(ErrNum_InvParAddrMode);
    else
      AddComp(InstrBM, Index);
  }
  else
    AddComp(InstrBM, Index);
  DiscardArgs();
}

static void DecodeEM(Word Index)
{
  /* Wenn BM-Feld schon da war, muss es EI sein */

  if (!SplitArgs(0))
    return;

  if ((InstrMask & (1 << InstrBM)) != 0)
  {
    Error = (Index == 0);
    if (Error)
      WrError(ErrNum_InvParAddrMode);
    else
      AddComp(InstrEM, Index);
  }
  else
  {
    AddComp(InstrEM, Index);
    if (Index == 0)
      InstrComps[InstrBM] = 3;
  }
  DiscardArgs();
}

static void DecodeRW(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrRW, Index);
  DiscardArgs();
}

static void DecodeWT(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrWT, Index);
  DiscardArgs();
}

static void DecodeNF(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrNF, Index);
  DiscardArgs();
}

static void DecodeWI(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrWI, Index);
  DiscardArgs();
}

static void DecodeFIS(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrFIS, Index);
  DiscardArgs();
}

static void DecodeFD(Word Index)
{
  if (!SplitArgs(0))
    return;

  AddComp(InstrFD, Index);
  DiscardArgs();
}

static void DecodeSHV(Word Index)
{
  LongWord Value;
  tSymbolFlags Flags;

  if (!SplitArgs(1))
    return;

  Value = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt6, &Error, &Flags);
  if (mFirstPassUnknown(Flags))
    Value &= 31;
  Error = (Value > 46) ? True : !Error;
  if (!Error)
    AddComp(InstrSHV, (Index << 6) + Value);
  DiscardArgs();
}

static void DecodeRPS(Word Index)
{
  LongWord Value;
  UNUSED(Index);

  if (!SplitArgs(1))
    return;
  Value = EvalStrIntExpression(&ArgStr[1], UInt9, &Error);
  Error = !Error;
  if (!Error)
    AddComp(InstrRPS, Value);
  DiscardArgs();
}

static void DecodeNAL(Word Index)
{
  LongWord Value;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (!SplitArgs(1))
    return;

  Value = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt13, &Error, &Flags);
  Error = !Error;
  if (!Error)
  {
    if (ChkSamePage(Value, EProgCounter(), 9, Flags))
      AddComp(InstrNAL, Value & 0x1ff);
  }
  DiscardArgs();
}

static Boolean DecodePseudo(void)
{
  LongWord temp;
  LongInt sign, mant, expo, Size;

  if (Memo("DW"))
  {
    if (ChkArgCnt(1, ArgCntMax))
    {
      TempResult t;
      Boolean OK = True;
      tStrComp *pArg;

      as_tempres_ini(&t);
      forallargs(pArg, OK)
      {
        EvalStrExpression(pArg, &t);
        switch(t.Typ)
        {
          case TempString:
            if (MultiCharToInt(&t, 4))
              goto ToInt;

            if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
              OK = False;
            else
              OK = !string_2_dasm_code(&t.Contents.str, Packing ? 4 : 1, True);
            break;
          case TempInt:
          ToInt:
            if (!RangeCheck(t.Contents.Int, Int32))
            {
              WrStrErrorPos(ErrNum_OverRange, pArg);
              OK = False;
              break;
            }
            DAsmCode[CodeLen++] = t.Contents.Int;
            break;
          case TempFloat:
            if (!FloatRangeCheck(t.Contents.Float, Float32))
            {
              WrStrErrorPos(ErrNum_OverRange, pArg);
              OK = False;
              break;
            }
            Double_2_ieee4(t.Contents.Float, (Byte*) &temp, HostBigEndian);
            sign = (temp >> 31) & 1;
            expo = (temp >> 23) & 255;
            mant = temp & 0x7fffff;
            if ((mant == 0) && (expo == 0))
              DAsmCode[CodeLen++] = 0x80000000;
            else
            {
              if (expo > 0)
              {
                mant |= 0x800000;
                expo -= 127;
              }
              else
                expo -= 126;
              if (mant >= 0x800000)
              {
                mant = mant >> 1;
                expo += 1;
              }
              if (sign == 1)
                mant = ((mant ^ 0xffffff) + 1);
              DAsmCode[CodeLen++] = ((expo & 0xff) << 24) | (mant & 0xffffff);
            }
            break;
          default:
            OK = False;
        }
      }
      if (!OK)
        CodeLen = 0;
      as_tempres_free(&t);
    }
    return True;
  }

  if (Memo("DS"))
  {
    if (ChkArgCnt(1, 1))
    {
      tSymbolFlags Flags;
      Boolean OK;

      Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
      if (mFirstPassUnknown(Flags))
      {
        WrError(ErrNum_FirstPassCalc);
        OK = False;
      }
      if (OK)
      {
        DontPrint = True;
        if (!Size)
          WrError(ErrNum_NullResMem);
        CodeLen = Size;
        BookKeeping();
      }
    }
    return True;
  }

  return FALSE;
}

/*---------------------------------------------------------------------------*/
/* Codetabellenverwaltung */

static void AddJmp(const char *NName, LongWord NCode)
{
  order_array_rsv_end(JmpOrders, FixedOrder);
  JmpOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeJmp);
}

static void AddALU1(const char *NName, LongWord NCode)
{
  order_array_rsv_end(ALU1Orders, FixedOrder);
  ALU1Orders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU1);
}

static void AddALU2(const char *NName, LongWord NCode)
{
  order_array_rsv_end(ALU2Orders, FixedOrder);
  ALU2Orders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU2);
}

static void AddSrcReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(SrcRegs, Register);
  SrcRegs[InstrZ].Name = NName;
  SrcRegs[InstrZ++].Code = NCode;
}

static void AddALUSrcReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(ALUSrcRegs, Register);
  ALUSrcRegs[InstrZ].Name = NName;
  ALUSrcRegs[InstrZ++].Code = NCode;
}

static void AddDestReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(DestRegs, Register);
  DestRegs[InstrZ].Name = NName;
  DestRegs[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "LDI", 0, DecodeLDI);
  AddInstTable(InstTable, "NOP", 0, DecodeNOP);

  AddInstTable(InstTable, "SPCBP0", 1, DecodeM0);
  AddInstTable(InstTable, "SPCIX0", 2, DecodeM0);
  AddInstTable(InstTable, "SPCBI0", 3, DecodeM0);

  AddInstTable(InstTable, "SPCBP1", 1, DecodeM1);
  AddInstTable(InstTable, "SPCIX1", 2, DecodeM1);
  AddInstTable(InstTable, "SPCBI1", 3, DecodeM1);

  AddInstTable(InstTable, "INCBP0", 1, DecodeDP0);
  AddInstTable(InstTable, "DECBP0", 2, DecodeDP0);
  AddInstTable(InstTable, "CLRBP0", 3, DecodeDP0);
  AddInstTable(InstTable, "STIX0" , 4, DecodeDP0);
  AddInstTable(InstTable, "INCIX0", 5, DecodeDP0);
  AddInstTable(InstTable, "DECIX0", 6, DecodeDP0);
  AddInstTable(InstTable, "CLRIX0", 7, DecodeDP0);

  AddInstTable(InstTable, "INCBP1", 1, DecodeDP1);
  AddInstTable(InstTable, "DECBP1", 2, DecodeDP1);
  AddInstTable(InstTable, "CLRBP1", 3, DecodeDP1);
  AddInstTable(InstTable, "STIX1" , 4, DecodeDP1);
  AddInstTable(InstTable, "INCIX1", 5, DecodeDP1);
  AddInstTable(InstTable, "DECIX1", 6, DecodeDP1);
  AddInstTable(InstTable, "CLRIX1", 7, DecodeDP1);

  AddInstTable(InstTable, "INCAR" , 1, DecodeEA);
  AddInstTable(InstTable, "DECAR" , 2, DecodeEA);

  AddInstTable(InstTable, "XCHPSW", 1, DecodeFC);

  AddInstTable(InstTable, "INCRP" , 1, DecodeRP);
  AddInstTable(InstTable, "DECRP" , 2, DecodeRP);
  AddInstTable(InstTable, "INCBRP", 3, DecodeRP);

  AddInstTable(InstTable, "DECLC" , 1, DecodeL);

  AddInstTable(InstTable, "MCNBP0", InstrBASE0, DecodeBASE);
  AddInstTable(InstTable, "MCNBP1", InstrBASE1, DecodeBASE);

  AddInstTable(InstTable, "BITRP" , 0, DecodeRPC);

  AddInstTable(InstTable, "CLRP2" , 0, DecodeP2);
  AddInstTable(InstTable, "SETP2" , 1, DecodeP2);

  AddInstTable(InstTable, "CLRP3" , 0, DecodeP3);
  AddInstTable(InstTable, "SETP3" , 1, DecodeP3);

  AddInstTable(InstTable, "DI"    , 0, DecodeEM);
  AddInstTable(InstTable, "EI"    , 1, DecodeEM);
  AddInstTable(InstTable, "CLRBM" , 1, DecodeBM);
  AddInstTable(InstTable, "SETBM" , 2, DecodeBM);

  AddInstTable(InstTable, "RD"    , 1, DecodeRW);
  AddInstTable(InstTable, "WR"    , 2, DecodeRW);

  AddInstTable(InstTable, "WRBORD", 1, DecodeWT);
  AddInstTable(InstTable, "WRBL24", 2, DecodeWT);
  AddInstTable(InstTable, "WRBL23", 3, DecodeWT);
  AddInstTable(InstTable, "WRBEL8", 4, DecodeWT);
  AddInstTable(InstTable, "WRBL8E", 5, DecodeWT);
  AddInstTable(InstTable, "WRBXCH", 6, DecodeWT);
  AddInstTable(InstTable, "WRBBRV", 7, DecodeWT);

  AddInstTable(InstTable, "TRNORM", 2, DecodeNF);
  AddInstTable(InstTable, "RDNORM", 4, DecodeNF);
  AddInstTable(InstTable, "FLTFIX", 6, DecodeNF);
  AddInstTable(InstTable, "FIXMA" , 7, DecodeNF);

  AddInstTable(InstTable, "BWRL24", 1, DecodeWI);
  AddInstTable(InstTable, "BWRORD", 2, DecodeWI);

  AddInstTable(InstTable, "SPCPSW0", 1, DecodeFIS);
  AddInstTable(InstTable, "SPCPSW1", 2, DecodeFIS);
  AddInstTable(InstTable, "CLRPSW0", 4, DecodeFIS);
  AddInstTable(InstTable, "CLRPSW1", 5, DecodeFIS);
  AddInstTable(InstTable, "CLRPSW" , 6, DecodeFIS);

  AddInstTable(InstTable, "SPIE", 1, DecodeFD);
  AddInstTable(InstTable, "IESP", 2, DecodeFD);

  AddInstTable(InstTable, "SETSVL", 0, DecodeSHV);
  AddInstTable(InstTable, "SETSVR", 1, DecodeSHV);

  AddInstTable(InstTable, "SPCRA", 0, DecodeRPS);
  AddInstTable(InstTable, "JBLK" , 0, DecodeNAL);

  InstrZ = 0;
  AddJmp("JMP"   , 0x0100); AddJmp("CALL"  , 0x0101); AddJmp("RET"   , 0x0002);
  AddJmp("JNZRP" , 0x0103); AddJmp("JZ0"   , 0x0104); AddJmp("JNZ0"  , 0x0105);
  AddJmp("JZ1"   , 0x0106); AddJmp("JNZ1"  , 0x0107); AddJmp("JC0"   , 0x0108);
  AddJmp("JNC0"  , 0x0109); AddJmp("JC1"   , 0x010a); AddJmp("JNC1"  , 0x010b);
  AddJmp("JS0"   , 0x010c); AddJmp("JNS0"  , 0x010d); AddJmp("JS1"   , 0x010e);
  AddJmp("JNS1"  , 0x010f); AddJmp("JV0"   , 0x0110); AddJmp("JNV0"  , 0x0111);
  AddJmp("JV1"   , 0x0112); AddJmp("JNV1"  , 0x0113); AddJmp("JEV0"  , 0x0114);
  AddJmp("JEV1"  , 0x0115); AddJmp("JNFSI" , 0x0116); AddJmp("JNESO" , 0x0117);
  AddJmp("JIP0"  , 0x0118); AddJmp("JIP1"  , 0x0119); AddJmp("JNZIX0", 0x011a);
  AddJmp("JNZIX1", 0x011b); AddJmp("JNZBP0", 0x011c); AddJmp("JNZBP1", 0x011d);
  AddJmp("JRDY"  , 0x011e); AddJmp("JRQM"  , 0x011f);

  InstrZ = 0;
  AddALU1("INC"  , 0x01); AddALU1("DEC"  , 0x02); AddALU1("ABS"  , 0x03);
  AddALU1("NOT"  , 0x04); AddALU1("NEG"  , 0x05); AddALU1("SHLC" , 0x06);
  AddALU1("SHRC" , 0x07); AddALU1("ROL"  , 0x08); AddALU1("ROR"  , 0x09);
  AddALU1("SHLM" , 0x0a); AddALU1("SHRM" , 0x0b); AddALU1("SHRAM", 0x0c);
  AddALU1("CLR"  , 0x0d); AddALU1("NORM" , 0x0e); AddALU1("CVT"  , 0x0f);

  InstrZ = 0;
  AddALU2("ADD"  , 0x10); AddALU2("SUB"  , 0x11); AddALU2("ADDC" , 0x12);
  AddALU2("SUBC" , 0x13); AddALU2("CMP"  , 0x14); AddALU2("AND"  , 0x15);
  AddALU2("OR"   , 0x16); AddALU2("XOR"  , 0x17); AddALU2("ADDF" , 0x18);
  AddALU2("SUBF" , 0x19);

  InstrZ = 0;
  AddSrcReg("NON" , 0x00); AddSrcReg("RP"  , 0x01); AddSrcReg("PSW0" , 0x02);
  AddSrcReg("PSW1", 0x03); AddSrcReg("SVR" , 0x04); AddSrcReg("SR"   , 0x05);
  AddSrcReg("LC"  , 0x06); AddSrcReg("STX" , 0x07); AddSrcReg("M"    , 0x08);
  AddSrcReg("ML"  , 0x09); AddSrcReg("ROM" , 0x0a); AddSrcReg("TR"   , 0x0b);
  AddSrcReg("AR"  , 0x0c); AddSrcReg("SI"  , 0x0d); AddSrcReg("DR"   , 0x0e);
  AddSrcReg("DRS" , 0x0f); AddSrcReg("WR0" , 0x10); AddSrcReg("WR1"  , 0x11);
  AddSrcReg("WR2" , 0x12); AddSrcReg("WR3" , 0x13); AddSrcReg("WR4"  , 0x14);
  AddSrcReg("WR5" , 0x15); AddSrcReg("WR6" , 0x16); AddSrcReg("WR7"  , 0x17);
  AddSrcReg("RAM0", 0x18); AddSrcReg("RAM1", 0x19); AddSrcReg("BP0"  , 0x1a);
  AddSrcReg("BP1" , 0x1b); AddSrcReg("IX0" , 0x1c); AddSrcReg("IX1"  , 0x1d);
  AddSrcReg("K"   , 0x1e); AddSrcReg("L"   , 0x1f); AddSrcReg(NULL   , 0);

  InstrZ = 0;
  AddALUSrcReg("IB"  , 0x00); AddALUSrcReg("M"   , 0x01);
  AddALUSrcReg("RAM0", 0x02); AddALUSrcReg("RAM1", 0x03);
  AddALUSrcReg(NULL  , 0);

  InstrZ = 0;
  AddDestReg("NON" , 0x00); AddDestReg("RP"  , 0x01); AddDestReg("PSW0" , 0x02);
  AddDestReg("PSW1", 0x03); AddDestReg("SVR" , 0x04); AddDestReg("SR"   , 0x05);
  AddDestReg("LC"  , 0x06); AddDestReg("STK" , 0x07); AddDestReg("LKR0" , 0x08);
  AddDestReg("KLR1", 0x09); AddDestReg("TRE" , 0x0a); AddDestReg("TR"   , 0x0b);
  AddDestReg("AR"  , 0x0c); AddDestReg("SO"  , 0x0d); AddDestReg("DR"   , 0x0e);
  AddDestReg("DRS" , 0x0f); AddDestReg("WR0" , 0x10); AddDestReg("WR1"  , 0x11);
  AddDestReg("WR2" , 0x12); AddDestReg("WR3" , 0x13); AddDestReg("WR4"  , 0x14);
  AddDestReg("WR5" , 0x15); AddDestReg("WR6" , 0x16); AddDestReg("WR7"  , 0x17);
  AddDestReg("RAM0", 0x18); AddDestReg("RAM1", 0x19); AddDestReg("BP0"  , 0x1a);
  AddDestReg("BP1" , 0x1b); AddDestReg("IX0" , 0x1c); AddDestReg("IX1"  , 0x1d);
  AddDestReg("K"   , 0x1e); AddDestReg("L"   , 0x1f); AddDestReg(NULL   , 0);

  InstrComps = (LongWord*) malloc(sizeof(LongWord) * InstrCnt);
  InstrDefs = (LongWord*) malloc(sizeof(LongWord) * InstrCnt);
  for (InstrZ = 0; InstrZ < InstrCnt; InstrDefs[InstrZ++] = 0xffffffff);
  InstrDefs[InstrALU] = 0;
  InstrDefs[InstrMove] = 0;
  InstrDefs[InstrBM] = 0;
  InstrDefs[InstrEM] = 0;
  InstrDefs[InstrDP0] = 0;
  InstrDefs[InstrDP1] = 0;
  InstrDefs[InstrEA] = 0;
  InstrDefs[InstrFC] = 0;
  InstrDefs[InstrFD] = 0;
  InstrDefs[InstrFIS] = 0;
  InstrDefs[InstrL] = 0;
  InstrDefs[InstrM0] = 0;
  InstrDefs[InstrM1] = 0;
  InstrDefs[InstrNF] = 0;
  InstrDefs[InstrRP] = 0;
  InstrDefs[InstrRW] = 0;
  InstrDefs[InstrWI] = 0;
  InstrDefs[InstrWT] = 0;
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(SrcRegs);
  order_array_free(ALUSrcRegs);
  order_array_free(DestRegs);

  order_array_free(JmpOrders);
  order_array_free(ALU1Orders);
  order_array_free(ALU2Orders);

  free(InstrComps);
  free(InstrDefs);
}

/*---------------------------------------------------------------------------*/
/* Callbacks */

static void MakeCode_77230(void)
{
  int z, z2;
  LongWord Diff;

  /* Nullanweisung */

  if (Memo("") && !*AttrPart.str.p_str && (ArgCnt == 0))
    return;

  /* Pseudoanweisungen */

  if (DecodePseudo())
    return;

  /* solange dekodieren, bis keine Operanden mehr da oder Fehler */

  Error = False;
  InstrMask = 0;
  memset(InstrComps, 0, sizeof(LongWord) * InstrCnt);
  do
  {
    if (!LookupInstTable(InstTable, OpPart.str.p_str))
    {
      WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
      Error = True;
    }
  }
  while (!Error && (*OpPart.str.p_str != '\0'));

  /* passende Verknuepfung suchen */

  if (!Error)
  {
    for (z = 0; z < CaseCnt; z++)
    {
      /* Bits ermitteln, die nur in einer Maske vorhanden sind */

      Diff = InstrMask^CaseMasks[z];

      /* Fall nur moeglich, wenn Bits im aktuellen Fall gesetzt sind, die
         der Fall nicht hat */

      if ((Diff & InstrMask) == 0)
      {
        /* ist irgendein Feld unbenutzt, fuer das wir keinen Default haben? */

        for (z2 = 0; z2 < InstrCnt; z2++)
          if (((Diff & (1l << z2)) != 0) && (InstrDefs[z2] == 0xffffffff))
            break;
        if (z2 == InstrCnt)
          break;
      }
    }

    switch (z)
    {
      case 0: /* nur LDI */
        DAsmCode[0] = 0xe0000000 + InstrComps[InstrLDI];
        CodeLen = 1;
        break;
      case 1: /* JMP + MOV */
        DAsmCode[0] = 0xd0000000 + (InstrComps[InstrBranch] << 10)
                                 + InstrComps[InstrMove];
        CodeLen = 1;
        break;
      case 2: /* ALU + MOV + M0 + M1 + DP0 + DP1 */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrDP1] << 15) + (InstrComps[InstrDP0] << 18)
                    + (InstrComps[InstrM1] << 21) + (InstrComps[InstrM0] << 23);
        CodeLen = 1;
        break;
      case 3: /* ALU + MOV + EA + DP0 + DP1 */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrDP1] << 15) + (InstrComps[InstrDP0] << 18)
                    + (InstrComps[InstrEA] << 21) + 0x02000000;
        CodeLen = 1;
        break;
      case 4: /* ALU + MOV + RP + M0 + DP0 + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrRP] << 21) + (InstrComps[InstrFC] << 15)
                    + (InstrComps[InstrM0] << 19) + (InstrComps[InstrDP0] << 16)
                    + 0x02800000;
        CodeLen = 1;
        break;
      case 5: /* ALU + MOV + RP + M1 + DP1 + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrRP] << 21) + (InstrComps[InstrFC] << 15)
                    + (InstrComps[InstrM1] << 19) + (InstrComps[InstrDP1] << 16)
                    + 0x03000000;
        CodeLen = 1;
        break;
      case 6: /* ALU + MOV + RP + M0 + M1 + L + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrRP] << 21) + (InstrComps[InstrL] << 16)
                    + (InstrComps[InstrM0] << 19) + (InstrComps[InstrM1] << 17)
                    + (InstrComps[InstrFC] << 15) + 0x03800000;
        CodeLen = 1;
        break;
      case 7: /* ALU + MOV + BASE0 + BASE1 + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrBASE0] << 19) + (InstrComps[InstrBASE1] << 16)
                    + (InstrComps[InstrFC] << 15) + 0x04000000;
        CodeLen = 1;
        break;
      case 8: /* ALU + MOV + RPC + L+ FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrRPC] << 18) + (InstrComps[InstrL] << 16)
                    + (InstrComps[InstrFC] << 15) + 0x04400000;
        CodeLen = 1;
        break;
      case 9: /* ALU + MOV + P2 + P3 + EM + BM + L + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrP2] << 20) + (InstrComps[InstrP3] << 21)
                    + (InstrComps[InstrEM] << 19) + (InstrComps[InstrBM] << 17)
                    + (InstrComps[InstrL] << 16) + (InstrComps[InstrFC] << 15)
                    + 0x04800000;
        CodeLen = 1;
        break;
      case 10: /* ALU + MOV + RW + L + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrRW] << 20) + (InstrComps[InstrL] << 16)
                    + (InstrComps[InstrFC] << 15) + 0x04c00000;
        CodeLen = 1;
        break;
      case 11: /* ALU + MOV + WT + L + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrWT] << 19) + (InstrComps[InstrL] << 16)
                    + (InstrComps[InstrFC] << 15) + 0x05000000;
        CodeLen = 1;
        break;
      case 12: /* ALU + MOV + NF + WI + L + FC */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrNF] << 19) + (InstrComps[InstrWI] << 17)
                    + (InstrComps[InstrL] << 16) + (InstrComps[InstrFC] << 15)
                    + 0x05400000;
        CodeLen = 1;
        break;
      case 13: /* ALU + MOV + FIS + FD + L */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrFIS] << 19) + (InstrComps[InstrFD] << 17)
                    + (InstrComps[InstrL] << 16) + 0x05800000;
        CodeLen = 1;
        break;
      case 14: /* ALU + MOV + SHV */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrSHV] << 15) + 0x05c00000;
        CodeLen = 1;
        break;
      case 15: /* ALU + MOV + RPS */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrRPS] << 15) + 0x06000000;
        CodeLen = 1;
        break;
      case 16: /* ALU + MOV + NAL */
        DAsmCode[0] = (InstrComps[InstrALU] << 10) + InstrComps[InstrMove]
                    + (InstrComps[InstrNAL] << 15) + 0x07000000;
        CodeLen = 1;
        break;
      default:
        WrError(ErrNum_InvParAddrMode);
    }
  }
}

static Boolean IsDef_77230(void)
{
  return False;
}

static void SwitchFrom_77230(void)
{
  DeinitFields();
}

static void SwitchTo_77230(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("77230");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  PCSymbol = "$";
  HeaderID = FoundDescr->Id;
  NOPCode = 0x00000000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegXData) | (1 << SegYData) | (1 << SegRData);
  Grans[SegCode ] = 4; ListGrans[SegCode ] = 4; SegInits[SegCode ] = 0;
  SegLimits[SegCode ] = 0x1fff;
  Grans[SegXData] = 4; ListGrans[SegXData] = 4; SegInits[SegXData] = 0;
  SegLimits[SegXData] = 0x1ff;
  Grans[SegYData] = 4; ListGrans[SegYData] = 4; SegInits[SegYData] = 0;
  SegLimits[SegYData] = 0x1ff;
  Grans[SegRData] = 4; ListGrans[SegRData] = 4; SegInits[SegRData] = 0;
  SegLimits[SegRData] = 0x3ff;

  onoff_packing_add(True);

  MakeCode = MakeCode_77230;
  IsDef = IsDef_77230;
  SwitchFrom = SwitchFrom_77230;

  InitFields();
}

/*---------------------------------------------------------------------------*/
/* Initialisierung */

void code77230_init(void)
{
  CPU77230 = AddCPU("77230", SwitchTo_77230);
}
