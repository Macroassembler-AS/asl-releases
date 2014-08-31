/* codeace.c */
/*****************************************************************************/
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegeneratormodul ACE-Familie                                            */
/*                                                                           */
/* Historie: 14. 8.1999 Grundsteinlegung                                     */
/*           15. 8.1999 Datensegment immer 256 Byte                          */
/*                      angekuendigte Typen                                  */
/*                      nur noch Intel-Pseudos                               */
/*           16. 8.1999 Fehler beseitigt                                     */
/*            9. 3.2000 'ambiguous else'-Warnungen beseitigt                 */
/*           14. 1.2001 silenced warnings about unused parameters            */
/*                      removed undef'd processors                           */
/*                      X-displacements are unsigned                         */
/*                      unified segments                                     */
/*                      changed segment limits/inits                         */
/*                                                                           */
/*****************************************************************************/
/* $Id: codeace.c,v 1.5 2014/06/23 17:27:49 alfred Exp $                     */
/*****************************************************************************
 * $Log: codeace.c,v $
 * Revision 1.5  2014/06/23 17:27:49  alfred
 * - reworked to current style
 *
 * Revision 1.4  2010/04/17 13:14:23  alfred
 * - address overlapping strcpy()
 *
 * Revision 1.3  2005/09/08 16:53:42  alfred
 * - use common PInstTable
 *
 * Revision 1.2  2004/05/29 11:33:02  alfred
 * - relocated DecodeIntelPseudo() into own module
 *
 *****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"

enum
{
  ModNone = -1,
  ModAcc = 0,
  ModX = 1,
  ModXInd = 2,
  ModXDisp = 3,
  ModDir = 4,
  ModImm = 5
};

#define MModAcc (1 << ModAcc)
#define MModX (1 << ModX)
#define MModXInd (1 << ModXInd)
#define MModXDisp (1 << ModXDisp)
#define MModDir (1 << ModDir)
#define MModImm (1 << ModImm)

#define FixedOrderCnt 9
#define AriOrderCnt 7
#define SingOrderCnt 3
#define BitOrderCnt 6

typedef struct
{
  Byte ImmCode, DirCode, IndCode, DispCode;
} AriOrder;

typedef struct
{
  Byte AccCode, XCode, DirCode;
} SingOrder;

typedef struct
{
  Byte AccCode, XIndCode, DirCode;
} BitOrder;

enum { D_CPUACE1101, D_CPUACE1202 };

static CPUVar CPUACE1101, CPUACE1202;

static AriOrder *AriOrders;
static SingOrder *SingOrders;
static BitOrder *BitOrders;

static ShortInt AdrMode;
static Byte AdrVal;
static Word WAdrVal;
static Boolean BigFlag, OpSize;

/*---------------------------------------------------------------------------*/

static void ChkAdr(Word Mask)
{
  if ((AdrMode == ModXDisp) && (MomCPU < CPUACE1202))
  {
    AdrMode = ModNone; WrError(1505);
  }
  else if ((AdrMode != ModNone) && ((Mask & (1 << AdrMode)) == 0))
  {
    AdrMode = ModNone; WrError(1350);
  }
}

static void DecodeAdr(char *Asc, Word Mask)
{
  Boolean OK, DispOcc, XOcc;
  int l;
  String Part;
  char *p;

  AdrMode = ModNone;

  /* Register ? */

  if (!strcasecmp(Asc, "A"))
   AdrMode = ModAcc;

  else if (!strcasecmp(Asc, "X"))
   AdrMode = ModX;

  /* immediate ? */

  else if (*Asc == '#')
  {
    if (OpSize)
     WAdrVal = EvalIntExpression(Asc + 1, Int12, &OK);
    else
     AdrVal = EvalIntExpression(Asc + 1, Int8, &OK);
    if (OK) AdrMode = ModImm;
  }

  /* indirekt ? */

  else if (*Asc == '[')
  {
    l = strlen(Asc) - 1;
    if (Asc[l] != ']') WrError(1350);
    else
    {
      Asc[l] = '\0'; Asc++;
      DispOcc = XOcc = False;
      while (*Asc != '\0')
      {
        p = QuotPos(Asc, ',');
        if (p != Nil) *p = '\0';
        strmaxcpy(Part, Asc, 255);
        KillPrefBlanks(Part); KillPostBlanks(Part);
        if (!strcasecmp(Part, "X"))
         if (XOcc)
         {
           WrError(1350); break;
         }
         else XOcc = True;
        else if (DispOcc)
        {
          WrError(1350); break;
        }
        else
        {
          char *pPart = Part;

          if (*pPart == '#') pPart++;
          AdrVal = EvalIntExpression(pPart, UInt8, &OK);
          if (!OK) break;
          DispOcc = True;
        }
        if (p == Nil) Asc = "";
        else Asc = p + 1;
      }
      if (*Asc == '\0')
       AdrMode = (DispOcc && (AdrVal != 0)) ? ModXDisp : ModXInd;
    }
  }
   
  /* direkt */

  else
  {
    if (OpSize)
     WAdrVal = EvalIntExpression(Asc, UInt12, &OK);
    else
     AdrVal = EvalIntExpression(Asc, UInt8, &OK);
    if (OK) AdrMode = ModDir;
  }

  ChkAdr(Mask);
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ArgCnt != 0) WrError(1110);
  else
  {
    BAsmCode[0] = Code;
    CodeLen = 1;
  }
}

static void DecodeAri(Word Index)
{
  AriOrder *porder = AriOrders + Index;

  if (ArgCnt != 2) WrError(1110);
  else  
  {
    DecodeAdr(ArgStr[1], MModAcc);
    if (AdrMode != ModNone)
    {
      DecodeAdr(ArgStr[2], MModImm | MModDir | MModXInd | MModXDisp);
      switch (AdrMode)
      {
        case ModImm:
          BAsmCode[0] = porder->ImmCode;
          BAsmCode[1] = AdrVal;
          CodeLen = 2;
          break;
        case ModDir:
          BAsmCode[0] = porder->DirCode;
          BAsmCode[1] = AdrVal;
          CodeLen = 2;
          break;
        case ModXInd:
          BAsmCode[0] = porder->IndCode;
          CodeLen = 1;
          break;
        case ModXDisp:
          BAsmCode[0] = porder->DispCode;
          BAsmCode[1] = AdrVal;
          CodeLen = 2;
          break;
      }
    }
  }
}

static void DecodeSing(Word Index)
{
  SingOrder *porder = SingOrders + Index;

  if (ArgCnt != 1) WrError(1110);
  else  
  {
    DecodeAdr(ArgStr[1], MModDir | MModX | MModAcc);
    switch (AdrMode)
    {
      case ModDir:
        BAsmCode[0] = porder->DirCode;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
      case ModAcc:
        BAsmCode[0] = porder->AccCode;
        CodeLen = 1;
        break;
      case ModX:
        BAsmCode[0] = porder->XCode;
        CodeLen = 1;
        break;
    }
  }
}

static void DecodeBit(Word Index)
{
  Byte Bit, Mask;
  BitOrder *porder = BitOrders + Index;
  Boolean OK;

  if (ArgCnt != 2) WrError(1110);
  else
  {
    Bit = EvalIntExpression(ArgStr[1], UInt8, &OK);
    if (OK)
    {
      Mask = 0;
      if (porder->AccCode != 0xff) Mask |= MModAcc;
      if (porder->XIndCode != 0xff) Mask |= MModXInd;
      if (porder->DirCode != 0xff) Mask |= MModDir;
      DecodeAdr(ArgStr[2], Mask);
      switch (AdrMode)
      {
        case ModAcc:
          BAsmCode[0] = porder->AccCode;
          if (porder->AccCode & 7)
          {
            BAsmCode[1] = 1 << Bit;
            if (porder->AccCode & 1)
              BAsmCode[1] = 255 - BAsmCode[1];
            CodeLen = 2;
          }
          else
          {
            BAsmCode[0] |= Bit;
            CodeLen = 1;
          }
          break;
        case ModXInd:
          BAsmCode[0] = porder->XIndCode + Bit;
          CodeLen = 1;
          break;
        case ModDir:
          BAsmCode[0] = porder->DirCode + Bit;
          BAsmCode[1] = AdrVal;
          CodeLen = 2;
          break;
      }
    }
  }
}

static void DecodeIFEQ(Word Index)
{
  UNUSED(Index);

  if (ArgCnt != 2) WrError(1110);
  else
  {
    DecodeAdr(ArgStr[1], MModAcc | MModX | MModDir);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(ArgStr[2], MModImm | MModDir | MModXInd | MModXDisp);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x65;
            BAsmCode[1] = AdrVal;
            CodeLen = 2;
            break;
          case ModDir:
            BAsmCode[0] = 0x56;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
          case ModXInd:
            BAsmCode[0] = 0x09;
            CodeLen = 1;
            break;
          case ModXDisp:
            BAsmCode[0] = 0x76;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
        }
        break;
      case ModX:
        OpSize = True;
        DecodeAdr(ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x26;
            BAsmCode[1] = Lo(WAdrVal);
            BAsmCode[2] = Hi(WAdrVal);
            CodeLen = 3;
            break;
        }
        break;
      case ModDir:
        BAsmCode[1] = AdrVal;
        DecodeAdr(ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x20;
            BAsmCode[2] = AdrVal;
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeIFGT(Word Index)
{
  UNUSED(Index);

  if (ArgCnt != 2) WrError(1110);
  else
  {
    DecodeAdr(ArgStr[1], MModAcc | MModX);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(ArgStr[2], MModImm | MModDir | MModXInd | MModXDisp);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x67;
            BAsmCode[1] = AdrVal;
            CodeLen = 2;
            break;
          case ModDir:
            BAsmCode[0] = 0x55;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
          case ModXInd:
            BAsmCode[0] = 0x0a;
            CodeLen = 1;
            break;
          case ModXDisp:
            BAsmCode[0] = 0x77;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
        }
        break;
      case ModX:
        OpSize = True;
        DecodeAdr(ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x27;
            BAsmCode[1] = Lo(WAdrVal);
            BAsmCode[2] = Hi(WAdrVal);
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeIFLT(Word Index)
{
  UNUSED(Index);

  if (ArgCnt != 2) WrError(1110);
  else
  {
    DecodeAdr(ArgStr[1], MModX);
    switch (AdrMode)
    {
      case ModX:
        OpSize = True;
        DecodeAdr(ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x28;
            BAsmCode[1] = Lo(WAdrVal);
            BAsmCode[2] = Hi(WAdrVal);
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeJMPJSR(Word Index)
{
  if (ArgCnt != 1) WrError(1110);
  else
  {
    OpSize = True;
    DecodeAdr(ArgStr[1], MModDir | MModXDisp);
    switch (AdrMode)
    {
      case ModDir:
        BAsmCode[0] = 0x24 - Index;
        BAsmCode[1] = Lo(WAdrVal);
        BAsmCode[2] = Hi(WAdrVal);
        CodeLen = 3;
        break;
      case ModXDisp:
        BAsmCode[0] = 0x7e + Index;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeJP(Word Index)
{
  LongInt Dist;
  Boolean OK;
  UNUSED(Index);

  if (ArgCnt != 1) WrError(1110);
  else
  {
    Dist = EvalIntExpression(ArgStr[1], UInt12, &OK) - (EProgCounter() + 1);
    if (OK)
    {
      if ((!SymbolQuestionable) && ((Dist > 31) || (Dist < -31))) WrError(1370);
      else
      {
        BAsmCode[0] = (Dist >= 0) ? 0xe0 + Dist : 0xc0 - Dist;
        CodeLen = 1;
      } 
    }
  }
}

static void DecodeLD(Word Index)
{
  UNUSED(Index);

  if (ArgCnt != 2) WrError(1110);
  else
  {
    DecodeAdr(ArgStr[1], MModAcc | MModX | MModDir);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(ArgStr[2], MModImm | MModDir | MModXInd | MModXDisp);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x51;
            BAsmCode[1] = AdrVal;
            CodeLen = 2;
            break;
          case ModDir:
            BAsmCode[0] = 0x46;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
          case ModXInd:
            BAsmCode[0] = 0x0e;
            CodeLen = 1;
            break;
          case ModXDisp:
            BAsmCode[0] = 0x52;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
        }
        break;
      case ModX:
        OpSize = True;
        DecodeAdr(ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x25;
            BAsmCode[1] = Lo(WAdrVal);
            BAsmCode[2] = Hi(WAdrVal);
            CodeLen = 3;
            break;
        }
        break;
      case ModDir:
        BAsmCode[1] = AdrVal;
        DecodeAdr(ArgStr[2], MModImm | MModDir);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x21;
            BAsmCode[2] = AdrVal;
            CodeLen = 3;
            break;
          case ModDir:
            BAsmCode[0] = 0x22;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = AdrVal;
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeRotate(Word Index)
{
  if (ArgCnt != 1) WrError(1110);
  else
  {
    DecodeAdr(ArgStr[1], MModAcc | MModDir);
    switch (AdrMode)
    {
      case ModAcc:
        BAsmCode[0] = 0x15 - Index - Index;
        CodeLen = 1;
        break;
      case ModDir:
        BAsmCode[0] = 0x79 + Index;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeST(Word Index)
{
  UNUSED(Index);

  if (ArgCnt != 2) WrError(1110);
  else
  {
    DecodeAdr(ArgStr[1], MModAcc);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(ArgStr[2], MModDir | MModXInd | MModXDisp);
        switch (AdrMode)
        {
          case ModDir:
            BAsmCode[0] = 0x47;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
          case ModXInd:
            BAsmCode[0] = 0x11;
            CodeLen = 1;
            break;
          case ModXDisp:
            BAsmCode[0] = 0x40;
            BAsmCode[1] = AdrVal;
            CodeLen = 2; 
            break;
        }
        break;
    }
  }
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegCode, 0, 0xff);
}

/*---------------------------------------------------------------------------*/

static void AddFixed(char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddAri(char *NName, Byte NImm, Byte NDir, Byte NInd, Byte NDisp)
{
  if (InstrZ >= AriOrderCnt) exit(255);
  AriOrders[InstrZ].ImmCode = NImm;
  AriOrders[InstrZ].DirCode = NDir;
  AriOrders[InstrZ].IndCode = NInd;
  AriOrders[InstrZ].DispCode = NDisp;
  AddInstTable(InstTable, NName, InstrZ++, DecodeAri);
}

static void AddSing(char *NName, Byte NAcc, Byte NX, Byte NDir)
{
  if (InstrZ >= SingOrderCnt) exit(255);
  SingOrders[InstrZ].AccCode = NAcc;
  SingOrders[InstrZ].XCode = NX;
  SingOrders[InstrZ].DirCode = NDir;
  AddInstTable(InstTable, NName, InstrZ++, DecodeSing);
}

static void AddBit(char *NName, Byte NAcc, Byte NIndX, Byte NDir)
{
  if (InstrZ >= BitOrderCnt) exit(255);
  BitOrders[InstrZ].AccCode = NAcc;
  BitOrders[InstrZ].XIndCode = NIndX;
  BitOrders[InstrZ].DirCode = NDir;
  AddInstTable(InstTable, NName, InstrZ++, DecodeBit);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(101);

  InstrZ = 0;
  AddFixed("IFC"  ,0x19);  AddFixed("IFNC" ,0x1f);  AddFixed("INTR" ,0x00);
  AddFixed("INVC" ,0x12);  AddFixed("NOP"  ,0x1c);  AddFixed("RC"   ,0x1e);
  AddFixed("RET"  ,0x17);  AddFixed("RETI" ,0x18);  AddFixed("SC"   ,0x1d);

  AriOrders = (AriOrder *) malloc(AriOrderCnt * sizeof(AriOrder));
  InstrZ = 0;
  AddAri("ADC" , 0x60, 0x42, 0x02, 0x70);
  AddAri("ADD" , 0x66, 0x43, 0x03, 0x71);
  AddAri("AND" , 0x61, 0x50, 0x04, 0x72);
  AddAri("IFNE", 0x57, 0x54, 0x0b, 0x78);
  AddAri("OR"  , 0x62, 0x44, 0x05, 0x73);
  AddAri("SUBC", 0x63, 0x53, 0x06, 0x74);
  AddAri("XOR" , 0x64, 0x45, 0x07, 0x75);

  SingOrders = (SingOrder *) malloc(SingOrderCnt * sizeof(SingOrder));
  InstrZ = 0;
  AddSing("CLR" , 0x16, 0x0f, 0x7d);
  AddSing("DEC" , 0x1a, 0x0c, 0x7b);
  AddSing("INC" , 0x1b, 0x0d, 0x7c);

  BitOrders = (BitOrder *) malloc(BitOrderCnt * sizeof(BitOrder));
  InstrZ = 0;
  AddBit("IFBIT", 0xa0, 0xa8, 0x58);
  AddBit("LDC"  , 0xff, 0xff, 0x80);
  AddBit("RBIT" , 0x61, 0xb8, 0x68);
  AddBit("SBIT" , 0x62, 0xb0, 0x48);
  AddBit("STC"  , 0xff, 0xff, 0x88);

  AddInstTable(InstTable, "IFEQ", 0, DecodeIFEQ);
  AddInstTable(InstTable, "IFGT", 0, DecodeIFGT);
  AddInstTable(InstTable, "IFLT", 0, DecodeIFLT);
  AddInstTable(InstTable, "JMP" , 0, DecodeJMPJSR);
  AddInstTable(InstTable, "JSR" , 1, DecodeJMPJSR);
  AddInstTable(InstTable, "JP"  , 0, DecodeJP);
  AddInstTable(InstTable, "LD"  , 0, DecodeLD);
  AddInstTable(InstTable, "RLC" , 0, DecodeRotate);
  AddInstTable(InstTable, "RRC" , 1, DecodeRotate);
  AddInstTable(InstTable, "ST"  , 0, DecodeST);
  AddInstTable(InstTable, "SFR" , 0, DecodeSFR);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  free(AriOrders);
  free(SingOrders);
  free(BitOrders);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_ACE(void)
{
  CodeLen = 0; DontPrint = False; BigFlag = False; OpSize = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(BigFlag)) return;

  if (!LookupInstTable(InstTable, OpPart))
   WrXError(1200, OpPart);
}

static Boolean IsDef_ACE(void)
{
  return (Memo("SFR"));
}

static void SwitchFrom_ACE(void)
{
  DeinitFields();
}

static void SwitchTo_ACE(void)
{
  PFamilyDescr Descr;

  TurnWords = False;
  ConstMode = ConstModeIntel;
  SetIsOccupied = False;

  Descr = FindFamilyByName("ACE");
  PCSymbol = "$";
  HeaderID = Descr->Id;
  NOPCode = 0x1c;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegLimits[SegCode] = 0xfff;

  switch (MomCPU - CPUACE1101)
  {
    case D_CPUACE1101:
      SegInits[SegCode] = 0xc00;
      break;
    case D_CPUACE1202:
      SegInits[SegCode] = 0x800;
      break;
  }

  MakeCode = MakeCode_ACE;
  IsDef = IsDef_ACE;
  SwitchFrom = SwitchFrom_ACE;
  InitFields();
}

void codeace_init(void)
{
  CPUACE1101 = AddCPU("ACE1101", SwitchTo_ACE);
  CPUACE1202 = AddCPU("ACE1202", SwitchTo_ACE);
}
