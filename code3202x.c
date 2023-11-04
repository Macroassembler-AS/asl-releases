/*
 * AS-Portierung
 *
 * AS-Codegeneratormodul fuer die Texas Instruments TMS320C2x-Familie
 *
 * (C) 1996 Thomas Sailer <sailer@ife.ee.ethz.ch>
 *
 */

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "errmsg.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "tipseudo.h"
#include "be_le.h"
#include "errmsg.h"

#include "code3202x.h"

/* ---------------------------------------------------------------------- */

typedef struct
{
  Word Code;
  Boolean Must1;
} AdrOrder;

typedef struct
{
  Word Code;
  Word AllowShifts;
} AdrShiftOrder;

typedef struct
{
  Word Code;
  Integer Min;
  Integer Max;
  Word Mask;
} ImmOrder;

typedef struct
{
  const char *Name;
  Word Mode;
} tAdrMode;

static AdrOrder *AdrOrders;
static AdrOrder *Adr2ndAdrOrders;
static AdrShiftOrder *AdrShiftOrders;
static ImmOrder *ImmOrders;
static tAdrMode *AdrModes;

/* ---------------------------------------------------------------------- */

static Word AdrMode;

static CPUVar CPU32025, CPU32026, CPU32028;

/* ---------------------------------------------------------------------- */

static Word EvalARExpression(const tStrComp *pArg, Boolean *OK)
{
  *OK = True;
  if ((as_toupper(pArg->str.p_str[0]) == 'A')
   && (as_toupper(pArg->str.p_str[1]) == 'R')
   && (pArg->str.p_str[2] >= '0') && (pArg->str.p_str[2] <= '7')
   && (pArg->str.p_str[3] == '\0'))
    return pArg->str.p_str[2] - '0';
  return EvalStrIntExpression(pArg, UInt3, OK);
}

/* ---------------------------------------------------------------------- */

static Boolean DecodeAdr(const tStrComp *pArg, int MinArgCnt, int aux, Boolean Must1)
{
  const tAdrMode *pAdrMode = AdrModes;
  Byte h;
  Boolean AdrOK = False;
  tEvalResult EvalResult;

  while (pAdrMode->Name && as_strcasecmp(pAdrMode->Name, pArg->str.p_str))
    pAdrMode++;
  if (!pAdrMode->Name)
  {
    if (aux <= ArgCnt)
    {
      (void)ChkArgCnt(MinArgCnt, aux - 1);
      return False;
    }
    h = EvalStrIntExpressionWithResult(pArg, Int16, &EvalResult);
    if (!EvalResult.OK)
      return False;
    if (Must1 && (h >= 0x80) && !mFirstPassUnknown(EvalResult.Flags))
    {
      WrError(ErrNum_UnderRange);
      return False;
    }
    AdrMode = h & 0x7f;
    ChkSpace(SegData, EvalResult.AddrSpaceMask);
    return True;
  }
  AdrMode = pAdrMode->Mode;
  if (aux <= ArgCnt)
  {
    h = EvalARExpression(&ArgStr[aux], &AdrOK);
    if (AdrOK)
      AdrMode |= 0x8 | h;
    return AdrOK;
  }
  else
    return True;
}

/* ---------------------------------------------------------------------- */

/* prozessorspezifische Befehle */

static void DecodeCNFD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, 0)
   && ChkExcludeCPU(CPU32026))
  {
    CodeLen = 1;
    WAsmCode[0] = 0xce04;
  }
}

static void DecodeCNFP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, 0)
   && ChkExcludeCPU(CPU32026))
  {
    CodeLen = 1;
    WAsmCode[0] = 0xce05;
  }
}

static void DecodeCONF(Word Code)
{
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkExactCPU(CPU32026))
  {
    WAsmCode[0] = 0xce3c | EvalStrIntExpression(&ArgStr[1], UInt2, &OK);
    if (OK)
      CodeLen = 1;
  }
}

/* kein Argument */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    WAsmCode[0] = Code;
  }
}

/* Spruenge */

static void DecodeJmp(Word Code)
{
  Boolean OK;

  if (ChkArgCnt(1, 3))
  {
    if (ArgCnt > 1)
    {
      OK = DecodeAdr(&ArgStr[2], 1, 3, False);
      if (OK  && (AdrMode < 0x80))
      {
        OK = False;
        WrError(ErrNum_InvAddrMode);
      }
    }
    else
    {
      OK = True;
      AdrMode = 0;
    }
    if (OK)
    {
      WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
      if (OK)
      {
        CodeLen = 2;
        WAsmCode[0] = Code | (AdrMode & 0x7f);
      }
    }
  }
}

/* nur Adresse */

static void DecodeAdrInst(Word Index)
{
  const AdrOrder *pOrder = AdrOrders + Index;

  if (ChkArgCnt(1, 2))
  {
    if (DecodeAdr(&ArgStr[1], 1, 2, pOrder->Must1))
    {
      CodeLen = 1;
      WAsmCode[0] = pOrder->Code | AdrMode;
    }
  }
}

/* 2 Addressen */

static void Decode2ndAdr(Word Index)
{
  const AdrOrder *pOrder = Adr2ndAdrOrders + Index;
  Boolean OK;

  if (ChkArgCnt(2, 3))
  {
    WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK && DecodeAdr(&ArgStr[2], 2, 3, pOrder->Must1))
    {
      CodeLen = 2;
      WAsmCode[0] = pOrder->Code | AdrMode;
    }
  }
}

/* Adresse & schieben */

static void DecodeShiftAdr(Word Index)
{
  const AdrShiftOrder *pOrder = AdrShiftOrders + Index;

  if (ChkArgCnt(1, 3))
  {
    Boolean OK;
    Word AdrWord;
    tSymbolFlags Flags;

    if (DecodeAdr(&ArgStr[1], 1, 3, False))
    {
      if (ArgCnt < 2)
      {
        OK = True;
        AdrWord = 0;
        Flags = eSymbolFlag_None;
      }
      else
      {
        AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[2], Int4, &OK, &Flags);
        if (OK && mFirstPassUnknown(Flags))
          AdrWord = 0;
      }
      if (OK)
      {
        if (pOrder->AllowShifts < AdrWord) WrError(ErrNum_InvShiftArg);
        else
        {
          CodeLen = 1;
          WAsmCode[0] = pOrder->Code | AdrMode | (AdrWord << 8);
        }
      }
    }
  }
}

/* Ein/Ausgabe */

static void DecodeIN_OUT(Word Code)
{
  if (ChkArgCnt(2, 3))
  {
    if (DecodeAdr(&ArgStr[1], 2, 3, False))
    {
      tEvalResult EvalResult;
      Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[2], Int4, &EvalResult);

      if (EvalResult.OK)
      {
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
        CodeLen = 1;
        WAsmCode[0] = Code | AdrMode | (AdrWord << 8);
      }
    }
  }
}

/* konstantes Argument */

static void DecodeImm(Word Index)
{
  const ImmOrder *pOrder = ImmOrders + Index;

  if (ChkArgCnt(1, (pOrder->Mask != 0xffff) ? 1 : 2))
  {
    tEvalResult EvalResult;
    LongInt AdrLong = EvalStrIntExpressionWithResult(&ArgStr[1], Int32, &EvalResult);

    if (EvalResult.OK)
    {
      if (mFirstPassUnknown(EvalResult.Flags))
        AdrLong &= pOrder->Mask;
      if (pOrder->Mask == 0xffff)
      {
        if (ChkRange(AdrLong, -32768, 65535))
        {
          Word AdrWord = 0;

          EvalResult.OK = True;
          if (ArgCnt == 2)
          {
            AdrWord = EvalStrIntExpressionWithResult(&ArgStr[2], Int4, &EvalResult);
            if (EvalResult.OK && mFirstPassUnknown(EvalResult.Flags))
             AdrWord = 0;
          }
          if (EvalResult.OK)
          {
            CodeLen = 2;
            WAsmCode[0] = pOrder->Code | (AdrWord << 8);
            WAsmCode[1] = AdrLong;
          }
        }
      }
      else if (ChkRange(AdrLong, pOrder->Min, pOrder->Max))
      {
        CodeLen = 1;
        WAsmCode[0] = pOrder->Code | (AdrLong & pOrder->Mask);
      }
    }
  }
}

/* mit Hilfsregistern */

static void DecodeLARP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord = EvalARExpression(&ArgStr[1], &OK);

    if (OK)
    {
      CodeLen = 1;
      WAsmCode[0] = 0x5588 | AdrWord;
    }
  }
}

static void DecodeLAR_SAR(Word Code)
{
  if (ChkArgCnt(2, 3))
  {
    Boolean OK;
    Word AdrWord = EvalARExpression(&ArgStr[1], &OK);

    if (OK)
    {
      if (DecodeAdr(&ArgStr[2], 2, 3, False))
      {
        CodeLen = 1;
        WAsmCode[0] = Code | AdrMode | (AdrWord << 8);
      }
    }
  }
}

static void DecodeLARK(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    Word AdrWord = EvalARExpression(&ArgStr[1], &OK);

    if (OK)
    {
      WAsmCode[0] = EvalStrIntExpression(&ArgStr[2], Int8, &OK) & 0xff;
      if (OK)
      {
        CodeLen = 1;
        WAsmCode[0] |= 0xc000 | (AdrWord << 8);
      }
    }
  }
}

static void DecodeLRLK(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    Word AdrWord = EvalARExpression(&ArgStr[1], &OK);

    if (OK)
    {
      WAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
      if (OK)
      {
        CodeLen = 2;
        WAsmCode[0] = 0xd000 | (AdrWord << 8);
      }
    }
  }
}

static void DecodeLDPK(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;

    WAsmCode[0] = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult);
    if (EvalResult.OK)
    {
      if (WAsmCode[0] < 0x1ff)
        WAsmCode[0] |= 0xc800;
      else
      {
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
        WAsmCode[0] = ((WAsmCode[0] >> 7) & 0x1ff) | 0xc800;
      }
      CodeLen = 1;
    }
  }
}

static void DecodeNORM(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    if (DecodeAdr(&ArgStr[1], 1, 2, False))
    {
      if (AdrMode < 0x80) WrError(ErrNum_InvAddrMode);
      else
      {
        CodeLen = 1;
        WAsmCode[0] = 0xce82 | (AdrMode & 0x70);
      }
    }
  }
}

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, 15);
}

/* ---------------------------------------------------------------------- */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddJmp(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void AddAdr(const char *NName, Word NCode, Boolean NMust1)
{
  order_array_rsv_end(AdrOrders, AdrOrder);
  AdrOrders[InstrZ].Code = NCode;
  AdrOrders[InstrZ].Must1 = NMust1;
  AddInstTable(InstTable, NName, InstrZ++, DecodeAdrInst);
}

static void Add2ndAdr(const char *NName, Word NCode, Boolean NMust1)
{
  order_array_rsv_end(Adr2ndAdrOrders, AdrOrder);
  Adr2ndAdrOrders[InstrZ].Code = NCode;
  Adr2ndAdrOrders[InstrZ].Must1 = NMust1;
  AddInstTable(InstTable, NName, InstrZ++, Decode2ndAdr);
}

static void AddShiftAdr(const char *NName, Word NCode, Word nallow)
{
  order_array_rsv_end(AdrShiftOrders, AdrShiftOrder);
  AdrShiftOrders[InstrZ].Code = NCode;
  AdrShiftOrders[InstrZ].AllowShifts = nallow;
  AddInstTable(InstTable, NName, InstrZ++, DecodeShiftAdr);
}

static void AddImm(const char *NName, Word NCode, Integer NMin, Integer NMax, Word NMask)
{
  order_array_rsv_end(ImmOrders, ImmOrder);
  ImmOrders[InstrZ].Code = NCode;
  ImmOrders[InstrZ].Min = NMin;
  ImmOrders[InstrZ].Max = NMax;
  ImmOrders[InstrZ].Mask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeImm);
}

static void AddAdrMode(const char *NName, Word NMode)
{
  order_array_rsv_end(AdrModes, tAdrMode);
  AdrModes[InstrZ].Name = NName;
  AdrModes[InstrZ++].Mode = NMode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(307);
  AddInstTable(InstTable, "CNFD", 0, DecodeCNFD);
  AddInstTable(InstTable, "CNFP", 0, DecodeCNFP);
  AddInstTable(InstTable, "CONF", 0, DecodeCONF);
  AddInstTable(InstTable, "OUT", 0xe000, DecodeIN_OUT);
  AddInstTable(InstTable, "IN", 0x8000, DecodeIN_OUT);
  AddInstTable(InstTable, "LARP", 0, DecodeLARP);
  AddInstTable(InstTable, "LAR", 0x3000, DecodeLAR_SAR);
  AddInstTable(InstTable, "SAR", 0x7000, DecodeLAR_SAR);
  AddInstTable(InstTable, "LARK", 0, DecodeLARK);
  AddInstTable(InstTable, "LRLK", 0, DecodeLRLK);
  AddInstTable(InstTable, "LDPK", 0, DecodeLDPK);
  AddInstTable(InstTable, "NORM", 0, DecodeNORM);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);

  AddFixed("ABS",    0xce1b); AddFixed("CMPL",   0xce27);
  AddFixed("NEG",    0xce23); AddFixed("ROL",    0xce34);
  AddFixed("ROR",    0xce35); AddFixed("SFL",    0xce18);
  AddFixed("SFR",    0xce19); AddFixed("ZAC",    0xca00);
  AddFixed("APAC",   0xce15); AddFixed("PAC",    0xce14);
  AddFixed("SPAC",   0xce16); AddFixed("BACC",   0xce25);
  AddFixed("CALA",   0xce24); AddFixed("RET",    0xce26);
  AddFixed("RFSM",   0xce36); AddFixed("RTXM",   0xce20);
  AddFixed("RXF",    0xce0c); AddFixed("SFSM",   0xce37);
  AddFixed("STXM",   0xce21); AddFixed("SXF",    0xce0d);
  AddFixed("DINT",   0xce01); AddFixed("EINT",   0xce00);
  AddFixed("IDLE",   0xce1f); AddFixed("NOP",    0x5500);
  AddFixed("POP",    0xce1d); AddFixed("PUSH",   0xce1c);
  AddFixed("RC",     0xce30); AddFixed("RHM",    0xce38);
  AddFixed("ROVM",   0xce02); AddFixed("RSXM",   0xce06);
  AddFixed("RTC",    0xce32); AddFixed("SC",     0xce31);
  AddFixed("SHM",    0xce39); AddFixed("SOVM",   0xce03);
  AddFixed("SSXM",   0xce07); AddFixed("STC",    0xce33);
  AddFixed("TRAP",   0xce1e);

  AddJmp("B",      0xff80); AddJmp("BANZ",   0xfb80);
  AddJmp("BBNZ",   0xf980); AddJmp("BBZ",    0xf880);
  AddJmp("BC",     0x5e80); AddJmp("BGEZ",   0xf480);
  AddJmp("BGZ",    0xf180); AddJmp("BIOZ",   0xfa80);
  AddJmp("BLEZ",   0xf280); AddJmp("BLZ",    0xf380);
  AddJmp("BNC",    0x5f80); AddJmp("BNV",    0xf780);
  AddJmp("BNZ",    0xf580); AddJmp("BV",     0xf080);
  AddJmp("BZ",     0xf680); AddJmp("CALL",   0xfe80);

  InstrZ = 0;
  AddAdr("ADDC",   0x4300, False); AddAdr("ADDH",   0x4800, False);
  AddAdr("ADDS",   0x4900, False); AddAdr("ADDT",   0x4a00, False);
  AddAdr("AND",    0x4e00, False); AddAdr("LACT",   0x4200, False);
  AddAdr("OR",     0x4d00, False); AddAdr("SUBB",   0x4f00, False);
  AddAdr("SUBC",   0x4700, False); AddAdr("SUBH",   0x4400, False);
  AddAdr("SUBS",   0x4500, False); AddAdr("SUBT",   0x4600, False);
  AddAdr("XOR",    0x4c00, False); AddAdr("ZALH",   0x4000, False);
  AddAdr("ZALR",   0x7b00, False); AddAdr("ZALS",   0x4100, False);
  AddAdr("LDP",    0x5200, False); AddAdr("MAR",    0x5500, False);
  AddAdr("LPH",    0x5300, False); AddAdr("LT",     0x3c00, False);
  AddAdr("LTA",    0x3d00, False); AddAdr("LTD",    0x3f00, False);
  AddAdr("LTP",    0x3e00, False); AddAdr("LTS",    0x5b00, False);
  AddAdr("MPY",    0x3800, False); AddAdr("MPYA",   0x3a00, False);
  AddAdr("MPYS",   0x3b00, False); AddAdr("MPYU",   0xcf00, False);
  AddAdr("SPH",    0x7d00, False); AddAdr("SPL",    0x7c00, False);
  AddAdr("SQRA",   0x3900, False); AddAdr("SQRS",   0x5a00, False);
  AddAdr("DMOV",   0x5600, False); AddAdr("TBLR",   0x5800, False);
  AddAdr("TBLW",   0x5900, False); AddAdr("BITT",   0x5700, False);
  AddAdr("LST",    0x5000, False); AddAdr("LST1",   0x5100, False);
  AddAdr("POPD",   0x7a00, False); AddAdr("PSHD",   0x5400, False);
  AddAdr("RPT",    0x4b00, False); AddAdr("SST",    0x7800, True);
  AddAdr("SST1",   0x7900, True);

  InstrZ = 0;
  Add2ndAdr("BLKD",   0xfd00, False); Add2ndAdr("BLKP",   0xfc00, False);
  Add2ndAdr("MAC",    0x5d00, False); Add2ndAdr("MACD",   0x5c00, False);

  InstrZ = 0;
  AddShiftAdr("ADD",    0x0000, 0xf); AddShiftAdr("LAC",    0x2000, 0xf);
  AddShiftAdr("SACH",   0x6800, 0x7); AddShiftAdr("SACL",   0x6000, 0x7);
  AddShiftAdr("SUB",    0x1000, 0xf); AddShiftAdr("BIT",    0x9000, 0xf);

  InstrZ = 0;
  AddImm("ADDK",   0xcc00,     0,    255,   0xff);
  AddImm("LACK",   0xca00,     0,    255,   0xff);
  AddImm("SUBK",   0xcd00,     0,    255,   0xff);
  AddImm("ADRK",   0x7e00,     0,    255,   0xff);
  AddImm("SBRK",   0x7f00,     0,    255,   0xff);
  AddImm("RPTK",   0xcb00,     0,    255,   0xff);
  AddImm("MPYK",   0xa000, -4096,   4095, 0x1fff);
  AddImm("SPM",    0xce08,     0,      3,    0x3);
  AddImm("CMPR",   0xce50,     0,      3,    0x3);
  AddImm("FORT",   0xce0e,     0,      1,    0x1);
  AddImm("ADLK",   0xd002,     0, 0x7fff, 0xffff);
  AddImm("ANDK",   0xd004,     0, 0x7fff, 0xffff);
  AddImm("LALK",   0xd001,     0, 0x7fff, 0xffff);
  AddImm("ORK",    0xd005,     0, 0x7fff, 0xffff);
  AddImm("SBLK",   0xd003,     0, 0x7fff, 0xffff);
  AddImm("XORK",   0xd006,     0, 0x7fff, 0xffff);

  InstrZ = 0;
  AddAdrMode( "*-",     0x90 ); AddAdrMode( "*+",     0xa0 );
  AddAdrMode( "*BR0-",  0xc0 ); AddAdrMode( "*0-",    0xd0 );
  AddAdrMode( "*AR0-",  0xd0 ); AddAdrMode( "*0+",    0xe0 );
  AddAdrMode( "*AR0+",  0xe0 ); AddAdrMode( "*BR0+",  0xf0 );
  AddAdrMode( "*",      0x80 ); AddAdrMode( NULL,     0);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(AdrOrders);
  order_array_free(Adr2ndAdrOrders);
  order_array_free(AdrShiftOrders);
  order_array_free(ImmOrders);
  order_array_free(AdrModes);
}

/* ---------------------------------------------------------------------- */

static void MakeCode_3202x(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeTIPseudo())
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/* ---------------------------------------------------------------------- */

static Boolean IsDef_3202x(void)
{
  return Memo("PORT") || IsTIDef();
}

/* ---------------------------------------------------------------------- */

static void SwitchFrom_3202x(void)
{
  DeinitFields();
}

/* ---------------------------------------------------------------------- */

static void SwitchTo_3202x(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x75;
  NOPCode = 0x5500;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
        SegLimits[SegCode] = 0xffff;
  Grans[SegData] = 2; ListGrans[SegData] = 2; SegInits[SegData] = 0;
        SegLimits[SegData] = 0xffff;
  Grans[SegIO  ] = 2; ListGrans[SegIO  ] = 2; SegInits[SegIO  ] = 0;
        SegLimits[SegIO  ] = 0xf;

  MakeCode = MakeCode_3202x;
  IsDef = IsDef_3202x; SwitchFrom = SwitchFrom_3202x;
  InitFields();
}

/* ---------------------------------------------------------------------- */

void code3202x_init(void)
{
  CPU32025 = AddCPU("320C25", SwitchTo_3202x);
  CPU32026 = AddCPU("320C26", SwitchTo_3202x);
  CPU32028 = AddCPU("320C28", SwitchTo_3202x);

  AddCopyright("TMS320C2x-Generator (C) 1994/96 Thomas Sailer");
}
