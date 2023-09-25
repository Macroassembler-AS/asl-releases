/* code7720.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Makroassembler AS                                                         */
/*                                                                           */
/* Codegenerator NEC uPD772x                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include "strutil.h"
#include "nls.h"
#include "bpemu.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmcode.h"
#include "asmitree.h"
#include "headids.h"
#include "codevars.h"
#include "codepseudo.h"
#include "onoff_common.h"
#include "errmsg.h"
#include "chartrans.h"

#include "code7720.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  const char *Name;
  LongWord Code;
} TReg;

typedef enum
{
  MoveField,
  ALUField,
  DPLField,
  DPHField,
  RPField,
  RetField
} OpComps;

static CPUVar CPU7720, CPU7725;

static LongWord ActCode;
static Boolean InOp;
static Byte UsedOpFields;

static Byte TypePos, ImmValPos, AddrPos, ALUPos, DPLPos, AccPos, ALUSrcPos;
static IntType MemInt;
static Word ROMEnd, DROMEnd, RAMEnd;

static TReg *DestRegs, *SrcRegs, *ALUSrcRegs;

static PInstTable OpTable;

/*---------------------------------------------------------------------------*/
/* Hilfsroutinen */

static Boolean DecodeReg(char *Asc, LongWord *Code, TReg *Regs)
{
  int z;

  for (z = 0; Regs[z].Name; z++)
    if (!as_strcasecmp(Asc, Regs[z].Name))
    {
      *Code = Regs[z].Code;
      return True;
    }

  return False;
}

static Boolean ChkOpPresent(OpComps Comp)
{
  if ((UsedOpFields&(1l << Comp)) != 0)
  {
    WrError(ErrNum_InvParAddrMode); return False;
  }
  else
  {
    UsedOpFields |= 1l << Comp; return True;
  }
}

/*---------------------------------------------------------------------------*/
/* Dekoder */

static void DecodeJmp(Word Code)
{
  Word Dest;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    Dest = EvalStrIntExpression(&ArgStr[1], MemInt, &OK);
    if (OK)
    {
      DAsmCode[0] = (2l << TypePos) + (((LongWord)Code) << 13) + (Dest << AddrPos);
      CodeLen = 1;
    }
  }
}

static void DecodeDATA_7720(Word Index)
{
  LongInt MinV, MaxV;
  TempResult t;

  as_tempres_ini(&t);
  UNUSED(Index);

  if (ActPC == SegCode)
    MaxV = (MomCPU >= CPU7725) ? 16777215 : 8388607;
  else
    MaxV = 65535;
  MinV = (-((MaxV + 1) >> 1));
  if (ChkArgCnt(1, ArgCntMax))
  {
    Boolean OK = True;
    tStrComp *pArg;

    forallargs(pArg, OK)
    {
      EvalStrExpression(pArg, &t);
      if (mFirstPassUnknown(t.Flags) && (t.Typ == TempInt))
        t.Contents.Int &= MaxV;

      switch (t.Typ)
      {
        case TempString:
        {
          if (MultiCharToInt(&t, 3))
            goto ToInt;

          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            OK = False;
          else if (ActPC == SegCode)
            string_2_dasm_code(&t.Contents.str, Packing ? ((MomCPU >= CPU7725) ? 3 : 2) : 1, True);
          else
            string_2_wasm_code(&t.Contents.str, Packing ? 2 : 1, True);
          break;
        }
        case TempInt:
        ToInt:
          OK = ChkRange(t.Contents.Int, MinV, MaxV);
          if (OK)
          {
            if (ActPC == SegCode)
              DAsmCode[CodeLen++] = t.Contents.Int & MaxV;
            else
              WAsmCode[CodeLen++] = t.Contents.Int;
          }
          break;
        case TempFloat:
          WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
          /* fall-through */
        default:
          OK = False;
      }
    }
  }
  as_tempres_free(&t);
}

static void DecodeRES(Word Index)
{
  Word Size;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;

    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (OK && !mFirstPassUnknown(Flags))
    {
      DontPrint = True;
      if (!Size)
        WrError(ErrNum_NullResMem);
      CodeLen = Size;
      BookKeeping();
    }
  }
}

static void DecodeALU2(Word Code)
{
  LongWord Acc = 0xff, Src;
  char ch;

  if (!ChkOpPresent(ALUField))
    return;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[2].str.p_str, &Src, ALUSrcRegs)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else
  {
    if ((strlen(ArgStr[1].str.p_str) == 4) && (!as_strncasecmp(ArgStr[1].str.p_str, "ACC", 3)))
    {
      ch = as_toupper(ArgStr[1].str.p_str[3]);
      if ((ch>='A') && (ch<='B'))
        Acc = ch - 'A';
    }
    if (Acc == 0xff)
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
      ActCode |= (((LongWord)Code) << ALUPos) + (Acc << AccPos) + (Src << ALUSrcPos);
  }
}

static void DecodeALU1(Word Code)
{
  LongWord Acc = 0xff;
  char ch;

  if (!ChkOpPresent(ALUField))
    return;

  if (ChkArgCnt(1, 1))
  {
    if ((strlen(ArgStr[1].str.p_str) == 4) && (!as_strncasecmp(ArgStr[1].str.p_str, "ACC", 3)))
    {
      ch = as_toupper(ArgStr[1].str.p_str[3]);
      if ((ch >= 'A') && (ch <= 'B'))
        Acc = ch - 'A';
    }
    if (Acc == 0xff)
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
      ActCode |= (((LongWord)Code) << ALUPos) + (Acc << AccPos);
  }
}

static void DecodeNOP(Word Index)
{
  UNUSED(Index);

  if (!ChkOpPresent(ALUField))
    return;
}

static void DecodeDPL(Word Index)
{
  if (!ChkOpPresent(DPLField))
    return;

  if (ChkArgCnt(0, 0))
    ActCode |= (((LongWord)Index) << DPLPos);
}

static void DecodeDPH(Word Index)
{
  if (!ChkOpPresent(DPHField))
    return;

  if (ChkArgCnt(0, 0))
    ActCode |= (((LongWord)Index) << 9);
}

static void DecodeRP(Word Index)
{
  if (!ChkOpPresent(RPField))
    return;

  if (ChkArgCnt(0, 0))
    ActCode |= (((LongWord)Index) << 8);
}

static void DecodeRET(Word Index)
{
  UNUSED(Index);

  if (!ChkOpPresent(RetField))
    return;

  if (ChkArgCnt(0, 0))
    ActCode |= (1l << TypePos);
}

static void DecodeLDI(Word Index)
{
  LongWord Value;
  LongWord Reg;
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg, DestRegs)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    Value = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      DAsmCode[0] = (3l << TypePos) + Reg + (Value << ImmValPos);
      CodeLen = 1;
    }
  }
}

static void DecodeOP(Word Index)
{
  char *p;
  int z;
  UNUSED(Index);

  UsedOpFields = 0;
  ActCode = 0;

  if (ArgCnt >= 1)
  {
    p = FirstBlank(ArgStr[1].str.p_str);
    if (p)
    {
      StrCompSplitLeft(&ArgStr[1], &OpPart, p);
      NLS_UpString(OpPart.str.p_str);
      KillPrefBlanksStrComp(&ArgStr[1]);
    }
    else
    {
      StrCompCopy(&OpPart, &ArgStr[1]);
      for (z = 1; z < ArgCnt; z++)
        StrCompCopy(&ArgStr[z], &ArgStr[z + 1]);
      ArgCnt--;
    }
    if (!LookupInstTable(OpTable, OpPart.str.p_str))
      WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
  }

  DAsmCode[0] = ActCode;
  CodeLen = 1;
}

static void DecodeMOV(Word Index)
{
  LongWord Dest, Src;
  UNUSED(Index);

  if (!ChkOpPresent(MoveField))
    return;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Dest, DestRegs)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (!DecodeReg(ArgStr[2].str.p_str, &Src, SrcRegs)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else
    ActCode |= Dest + (Src << 4);
}

/*---------------------------------------------------------------------------*/
/* Tabellenverwaltung */

static void AddJmp(const char *NName, Word NCode)
{
  if ((MomCPU < CPU7725) && (Odd(NCode)))
    return;
  AddInstTable(InstTable, NName, (MomCPU == CPU7725) ? NCode : NCode >> 1, DecodeJmp);
}

static void AddALU2(const char *NName, Word NCode)
{
  AddInstTable(OpTable, NName, NCode, DecodeALU2);
}

static void AddALU1(const char *NName, Word NCode)
{
  AddInstTable(OpTable, NName, NCode, DecodeALU1);
}

static void AddDestReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(DestRegs, TReg);
  DestRegs[InstrZ].Name = NName;
  DestRegs[InstrZ++].Code = NCode;
}

static void AddSrcReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(SrcRegs, TReg);
  SrcRegs[InstrZ].Name = NName;
  SrcRegs[InstrZ++].Code = NCode;
}

static void AddALUSrcReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(ALUSrcRegs, TReg);
  ALUSrcRegs[InstrZ].Name = NName;
  ALUSrcRegs[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(101);
  OpTable = CreateInstTable(79);

  AddInstTable(InstTable, "LDI", 0, DecodeLDI);
  AddInstTable(InstTable, "LD", 0, DecodeLDI);
  AddInstTable(InstTable, "OP", 0, DecodeOP);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_7720);
  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(OpTable, "MOV", 0, DecodeMOV);
  AddInstTable(OpTable, "NOP", 0, DecodeNOP);
  AddInstTable(OpTable, "DPNOP", 0, DecodeDPL);
  AddInstTable(OpTable, "DPINC", 1, DecodeDPL);
  AddInstTable(OpTable, "DPDEC", 2, DecodeDPL);
  AddInstTable(OpTable, "DPCLR", 3, DecodeDPL);
  AddInstTable(OpTable, "M0", 0, DecodeDPH);
  AddInstTable(OpTable, "M1", 1, DecodeDPH);
  AddInstTable(OpTable, "M2", 2, DecodeDPH);
  AddInstTable(OpTable, "M3", 3, DecodeDPH);
  AddInstTable(OpTable, "M4", 4, DecodeDPH);
  AddInstTable(OpTable, "M5", 5, DecodeDPH);
  AddInstTable(OpTable, "M6", 6, DecodeDPH);
  AddInstTable(OpTable, "M7", 7, DecodeDPH);
  if (MomCPU >= CPU7725)
  {
    AddInstTable(OpTable, "M8", 8, DecodeDPH);
    AddInstTable(OpTable, "M9", 9, DecodeDPH);
    AddInstTable(OpTable, "MA", 10, DecodeDPH);
    AddInstTable(OpTable, "MB", 11, DecodeDPH);
    AddInstTable(OpTable, "MC", 12, DecodeDPH);
    AddInstTable(OpTable, "MD", 13, DecodeDPH);
    AddInstTable(OpTable, "ME", 14, DecodeDPH);
    AddInstTable(OpTable, "MF", 15, DecodeDPH);
  }
  AddInstTable(OpTable, "RPNOP", 0, DecodeRP);
  AddInstTable(OpTable, "RPDEC", 1, DecodeRP);
  AddInstTable(OpTable, "RET", 1, DecodeRET);

  AddJmp("JMP"   , 0x100); AddJmp("CALL"  , 0x140);
  AddJmp("JNCA"  , 0x080); AddJmp("JCA"   , 0x082);
  AddJmp("JNCB"  , 0x084); AddJmp("JCB"   , 0x086);
  AddJmp("JNZA"  , 0x088); AddJmp("JZA"   , 0x08a);
  AddJmp("JNZB"  , 0x08c); AddJmp("JZB"   , 0x08e);
  AddJmp("JNOVA0", 0x090); AddJmp("JOVA0" , 0x092);
  AddJmp("JNOVB0", 0x094); AddJmp("JOVB0" , 0x096);
  AddJmp("JNOVA1", 0x098); AddJmp("JOVA1" , 0x09a);
  AddJmp("JNOVB1", 0x09c); AddJmp("JOVB1" , 0x09e);
  AddJmp("JNSA0" , 0x0a0); AddJmp("JSA0"  , 0x0a2);
  AddJmp("JNSB0" , 0x0a4); AddJmp("JSB0"  , 0x0a6);
  AddJmp("JNSA1" , 0x0a8); AddJmp("JSA1"  , 0x0aa);
  AddJmp("JNSB1" , 0x0ac); AddJmp("JSB1"  , 0x0ae);
  AddJmp("JDPL0" , 0x0b0); AddJmp("JDPLF" , 0x0b2);
  AddJmp("JNSIAK", 0x0b4); AddJmp("JSIAK" , 0x0b6);
  AddJmp("JNSOAK", 0x0b8); AddJmp("JSOAK" , 0x0ba);
  AddJmp("JNRQM" , 0x0bc); AddJmp("JRQM"  , 0x0be);
  AddJmp("JDPLN0", 0x0b1); AddJmp("JDPLNF" , 0x0b3);

  AddALU2("OR"  , 1); AddALU2("AND" , 2); AddALU2("XOR" , 3);
  AddALU2("SUB" , 4); AddALU2("ADD" , 5); AddALU2("SBB" , 6);
  AddALU2("ADC" , 7); AddALU2("CMP" ,10);

  AddALU1("DEC" ,  8); AddALU1("INC" ,  9); AddALU1("SHR1", 11);
  AddALU1("SHL1", 12); AddALU1("SHL2", 13); AddALU1("SHL4", 14);
  AddALU1("XCHG", 15);

  InstrZ = 0;
  AddDestReg("@NON",  0); AddDestReg("@A"  ,  1);
  AddDestReg("@B"  ,  2); AddDestReg("@TR" ,  3);
  AddDestReg("@DP" ,  4); AddDestReg("@RP" ,  5);
  AddDestReg("@DR" ,  6); AddDestReg("@SR" ,  7);
  AddDestReg("@SOL",  8); AddDestReg("@SOM",  9);
  AddDestReg("@K"  , 10); AddDestReg("@KLR", 11);
  AddDestReg("@KLM", 12); AddDestReg("@L"  , 13);
  if (MomCPU == CPU7725) AddDestReg("@TRB", 14);
  AddDestReg("@MEM", 15);
  AddDestReg(NULL, 0);

  InstrZ = 0;
  AddSrcReg("NON" ,  0); AddSrcReg("A"   ,  1);
  AddSrcReg("B"   ,  2); AddSrcReg("TR"  ,  3);
  AddSrcReg("DP"  ,  4); AddSrcReg("RP"  ,  5);
  AddSrcReg("RO"  ,  6); AddSrcReg("SGN" ,  7);
  AddSrcReg("DR"  ,  8); AddSrcReg("DRNF",  9);
  AddSrcReg("SR"  , 10); AddSrcReg("SIM" , 11);
  AddSrcReg("SIL" , 12); AddSrcReg("K"   , 13);
  AddSrcReg("L"   , 14); AddSrcReg("MEM" , 15);
  AddSrcReg(NULL, 0);

  InstrZ = 0;
  AddALUSrcReg("RAM", 0); AddALUSrcReg("IDB", 1);
  AddALUSrcReg("M"  , 2); AddALUSrcReg("N"  , 3);
  AddALUSrcReg(NULL, 0);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  DestroyInstTable(OpTable);
  order_array_free(DestRegs);
  order_array_free(SrcRegs);
  order_array_free(ALUSrcRegs);
}

/*---------------------------------------------------------------------------*/
/* Callbacks */

static void MakeCode_7720(void)
{
  Boolean NextOp;

  /* Nullanweisung */

  if (Memo("") && !*AttrPart.str.p_str && (ArgCnt == 0))
    return;

  /* direkte Anweisungen */

  NextOp = Memo("OP");
  if (LookupInstTable(InstTable, OpPart.str.p_str))
  {
    InOp = NextOp; return;
  }

  /* wenn eine parallele Op-Anweisung offen ist, noch deren Komponenten testen */

  if ((InOp) && (LookupInstTable(OpTable, OpPart.str.p_str)))
  {
    RetractWords(1);
    DAsmCode[0] = ActCode;
    CodeLen = 1;
    return;
  }

  /* Hae??? */

  WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_7720(void)
{
  return False;
}

static void SwitchFrom_7720(void)
{
  DeinitFields();
}

static void SwitchTo_7720(void)
{
  const TFamilyDescr *FoundDescr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  if (MomCPU == CPU7725)
  {
    FoundDescr = FindFamilyByName("7725");
    MemInt = UInt11;
    ROMEnd = 0x7ff; DROMEnd = 0x3ff; RAMEnd = 0xff;
    TypePos = 22;
    ImmValPos = 6;
    AddrPos = 2;
    ALUPos = 16;
    DPLPos = 13;
    AccPos = 15;
    ALUSrcPos = 20;
  }
  else
  {
    FoundDescr = FindFamilyByName("7720");
    MemInt = UInt9;
    ROMEnd = 0x1ff; DROMEnd = 0x1ff; RAMEnd = 0x7f;
    TypePos = 21;
    ImmValPos = 5;
    AddrPos = 4;
    ALUPos = 15;
    DPLPos = 12;
    AccPos = 14;
    ALUSrcPos = 19;
  }

  PCSymbol = "$";
  HeaderID = FoundDescr->Id;
  NOPCode = 0x000000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1l << SegCode) | (1l << SegData) | (1l << SegRData);
  Grans[SegCode ] = 4; ListGrans[SegCode ] = 4; SegInits[SegCode ] = 0;
  SegLimits[SegCode ] = ROMEnd;
  Grans[SegData ] = 2; ListGrans[SegData ] = 2; SegInits[SegData ] = 0;
  SegLimits[SegData ] = RAMEnd;
  Grans[SegRData] = 2; ListGrans[SegRData] = 2; SegInits[SegRData] = 0;
  SegLimits[SegRData] = DROMEnd;

  MakeCode = MakeCode_7720;
  IsDef = IsDef_7720;
  SwitchFrom = SwitchFrom_7720;

  onoff_packing_add(True);

  InOp = False;
  UsedOpFields = 0;
  ActCode = 0;
  InitFields();
}

/*---------------------------------------------------------------------------*/
/* Initialisierung */

void code7720_init(void)
{
  CPU7720 = AddCPU("7720", SwitchTo_7720);
  CPU7725 = AddCPU("7725", SwitchTo_7720);
}
