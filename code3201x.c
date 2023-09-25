/* code3201x.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TMS3201x-Familie                                            */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code3201x.h"

typedef struct
{
  Word Code;
  Word AllowShifts;
} AdrShiftOrder;

typedef struct
{
  Word Code;
  Integer Min, Max;
  Word Mask;
} ImmOrder;


static Word AdrMode;
static Boolean AdrOK;

static CPUVar CPU32010, CPU32015;

static AdrShiftOrder *AdrShiftOrders;
static ImmOrder *ImmOrders;

/*----------------------------------------------------------------------------*/

static Word EvalARExpression(const tStrComp *pArg, Boolean *OK)
{
  *OK = True;
  if (!as_strcasecmp(pArg->str.p_str, "AR0"))
    return 0;
  if (!as_strcasecmp(pArg->str.p_str, "AR1"))
    return 1;
  return EvalStrIntExpression(pArg, UInt1, OK);
}

static void DecodeAdr(const tStrComp *pArg, int Aux, Boolean Must1)
{
  Byte h;
  char *p;
  char *Arg = pArg->str.p_str;

  AdrOK = False;

  if ((!strcmp(pArg->str.p_str, "*")) || (!strcmp(pArg->str.p_str, "*-")) || (!strcmp(pArg->str.p_str, "*+")))
  {
    AdrMode = 0x88;
    if (strlen(Arg) == 2)
      AdrMode += (pArg->str.p_str[1] == '+') ? 0x20 : 0x10;
    if (Aux <= ArgCnt)
    {
      h = EvalARExpression(&ArgStr[Aux], &AdrOK);
      if (AdrOK)
      {
        AdrMode &= 0xf7;
        AdrMode += h;
      }
    }
    else
      AdrOK = True;
  }
  else if (ChkArgCnt(1, Aux - 1))
  {
    tEvalResult EvalResult;

    h = 0;
    if ((strlen(pArg->str.p_str) > 3) && (!as_strncasecmp(pArg->str.p_str, "DAT", 3)))
    {
      AdrOK = True;
      for (p = pArg->str.p_str + 3; *p != '\0'; p++)
        if ((*p > '9') || (*p < '0'))
          AdrOK = False;
      if (AdrOK)
      {
        h = EvalStrIntExpressionOffsWithResult(pArg, 3, UInt8, &EvalResult);
        AdrOK = EvalResult.OK;
      }
    }
    if (!AdrOK)
    {
      h = EvalStrIntExpressionWithResult(pArg, Int8, &EvalResult);
      AdrOK = EvalResult.OK;
    }
    if (AdrOK)
    {
      if (Must1 && (h < 0x80) && !mFirstPassUnknown(EvalResult.Flags))
      {
        WrError(ErrNum_UnderRange);
        AdrOK = False;
      }
      else
      {
        AdrMode = h & 0x7f;
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

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
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], UInt12, &OK);
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[0] = Code;
    }
  }
}

/* nur Adresse */

static void DecodeAdrInst(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    DecodeAdr(&ArgStr[1], 2, Code & 1);
    if (AdrOK)
    {
      CodeLen = 1;
      WAsmCode[0] = (Code & 0xfffe) + AdrMode;
    }
  }
}

/* Adresse & schieben */

static void DecodeAdrShift(Word Index)
{
  Boolean HasSh;
  int AuxArgIndex;
  const AdrShiftOrder *pOrder = AdrShiftOrders + Index;

  if (ChkArgCnt(1, 3))
  {
    if (*ArgStr[1].str.p_str == '*')
    {
      switch (ArgCnt)
      {
        case 1:
          HasSh = False;
          AuxArgIndex = 3;
          break;
        case 2:
          if (!as_strncasecmp(ArgStr[2].str.p_str, "AR", 2))
          {
            HasSh = False;
            AuxArgIndex = 2;
          }
          else
          {
            HasSh = True;
            AuxArgIndex = 3;
          }
          break;
        default: /* 3 */
          HasSh = True;
          AuxArgIndex = 3;
      }
    }
    else
    {
      AuxArgIndex = 3;
      HasSh = (ArgCnt == 2);
    }
    DecodeAdr(&ArgStr[1], AuxArgIndex, False);
    if (AdrOK)
    {
      Boolean OK;
      Word AdrWord;

      if (!HasSh)
      {
        OK = True;
        AdrWord = 0;
      }
      else
      {
        tSymbolFlags Flags;

        AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[2], Int4, &OK, &Flags);
        if (OK && mFirstPassUnknown(Flags))
          AdrWord = 0;
      }
      if (OK)
      {
        if ((pOrder->AllowShifts & (1 << AdrWord)) == 0) WrError(ErrNum_InvShiftArg);
        else
        {
          CodeLen = 1;
          WAsmCode[0] = pOrder->Code + AdrMode + (AdrWord << 8);
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
    DecodeAdr(&ArgStr[1], 3, False);
    if (AdrOK)
    {
      tEvalResult EvalResult;
      Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[2], UInt3, &EvalResult);
      if (EvalResult.OK)
      {
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
        CodeLen = 1;
        WAsmCode[0] = Code + AdrMode + (AdrWord << 8);
      }
    }
  }
}

/* konstantes Argument */

static void DecodeImm(Word Index)
{
  const ImmOrder *pOrder = ImmOrders + Index;

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);

    if (OK)
    {
      if (mFirstPassUnknown(Flags))
        AdrLong &= pOrder->Mask;
      if (AdrLong < pOrder->Min) WrError(ErrNum_UnderRange);
      else if (AdrLong > pOrder->Max) WrError(ErrNum_OverRange);
      else
      {
        CodeLen = 1;
        WAsmCode[0] = pOrder->Code + (AdrLong & pOrder->Mask);
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
      WAsmCode[0] = 0x6880 + AdrWord;
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
      DecodeAdr(&ArgStr[2], 3, False);
      if (AdrOK)
      {
        CodeLen = 1;
        WAsmCode[0] = Code + AdrMode + (AdrWord << 8);
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
      WAsmCode[0] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
      if (OK)
      {
        CodeLen = 1;
        WAsmCode[0] = Lo(WAsmCode[0]) + 0x7000 + (AdrWord << 8);
      }
    }
  }
}

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, 7);
}

static void DecodeDATA_3201x(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int16, Int16);
}

/*----------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddJmp(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void AddAdr(const char *NName, Word NCode, Word NMust1)
{
  AddInstTable(InstTable, NName, NCode | NMust1, DecodeAdrInst);
}

static void AddAdrShift(const char *NName, Word NCode, Word NAllow)
{
  order_array_rsv_end(AdrShiftOrders, AdrShiftOrder);
  AdrShiftOrders[InstrZ].Code = NCode;
  AdrShiftOrders[InstrZ].AllowShifts = NAllow;
  AddInstTable(InstTable, NName, InstrZ++, DecodeAdrShift);
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

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "IN", 0x4000, DecodeIN_OUT);
  AddInstTable(InstTable, "OUT", 0x4800, DecodeIN_OUT);
  AddInstTable(InstTable, "LARP", 0, DecodeLARP);
  AddInstTable(InstTable, "LAR", 0x3800, DecodeLAR_SAR);
  AddInstTable(InstTable, "SAR", 0x3000, DecodeLAR_SAR);
  AddInstTable(InstTable, "LARK", 0, DecodeLARK);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);
  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_3201x);

  AddFixed("ABS"   , 0x7f88);  AddFixed("APAC"  , 0x7f8f);
  AddFixed("CALA"  , 0x7f8c);  AddFixed("DINT"  , 0x7f81);
  AddFixed("EINT"  , 0x7f82);  AddFixed("NOP"   , 0x7f80);
  AddFixed("PAC"   , 0x7f8e);  AddFixed("POP"   , 0x7f9d);
  AddFixed("PUSH"  , 0x7f9c);  AddFixed("RET"   , 0x7f8d);
  AddFixed("ROVM"  , 0x7f8a);  AddFixed("SOVM"  , 0x7f8b);
  AddFixed("SPAC"  , 0x7f90);  AddFixed("ZAC"   , 0x7f89);

  AddJmp("B"     , 0xf900);  AddJmp("BANZ"  , 0xf400);
  AddJmp("BGEZ"  , 0xfd00);  AddJmp("BGZ"   , 0xfc00);
  AddJmp("BIOZ"  , 0xf600);  AddJmp("BLEZ"  , 0xfb00);
  AddJmp("BLZ"   , 0xfa00);  AddJmp("BNZ"   , 0xfe00);
  AddJmp("BV"    , 0xf500);  AddJmp("BZ"    , 0xff00);
  AddJmp("CALL"  , 0xf800);

  AddAdr("ADDH"  , 0x6000, False);  AddAdr("ADDS"  , 0x6100, False);
  AddAdr("AND"   , 0x7900, False);  AddAdr("DMOV"  , 0x6900, False);
  AddAdr("LDP"   , 0x6f00, False);  AddAdr("LST"   , 0x7b00, False);
  AddAdr("LT"    , 0x6a00, False);  AddAdr("LTA"   , 0x6c00, False);
  AddAdr("LTD"   , 0x6b00, False);  AddAdr("MAR"   , 0x6800, False);
  AddAdr("MPY"   , 0x6d00, False);  AddAdr("OR"    , 0x7a00, False);
  AddAdr("SST"   , 0x7c00, True );  AddAdr("SUBC"  , 0x6400, False);
  AddAdr("SUBH"  , 0x6200, False);  AddAdr("SUBS"  , 0x6300, False);
  AddAdr("TBLR"  , 0x6700, False);  AddAdr("TBLW"  , 0x7d00, False);
  AddAdr("XOR"   , 0x7800, False);  AddAdr("ZALH"  , 0x6500, False);
  AddAdr("ZALS"  , 0x6600, False);

  InstrZ = 0;
  AddAdrShift("ADD"   , 0x0000, 0xffff);
  AddAdrShift("LAC"   , 0x2000, 0xffff);
  AddAdrShift("SACH"  , 0x5800, 0x0013);
  AddAdrShift("SACL"  , 0x5000, 0x0001);
  AddAdrShift("SUB"   , 0x1000, 0xffff);

  InstrZ = 0;
  AddImm("LACK", 0x7e00,     0,  255,   0xff);
  AddImm("LDPK", 0x6e00,     0,    1,    0x1);
  AddImm("MPYK", 0x8000, -4096, 4095, 0x1fff);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(AdrShiftOrders);
  order_array_free(ImmOrders);
}

/*----------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_3201X(char *pArg, TempResult *pResult)
 * \brief  parse for built-in symbols
 * \param  pArg source argument
 * \param  pResult possible result
 * ------------------------------------------------------------------------ */

static void InternSymbol_3201X(char *pArg, TempResult *pResult)
{
  if ((strlen(pArg) == 3)
   && (as_toupper(pArg[0]) == 'P')
   && (as_toupper(pArg[1]) == 'A')
   && ((pArg[2] >= '0') && (pArg[2] <= '7')))
  {
    as_tempres_set_int(pResult, pArg[2] - '0');
    pResult->AddrSpaceMask |= 1 << SegIO;
  }
}

static void MakeCode_3201X(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_3201X(void)
{
  return (Memo("PORT"));
}

static void SwitchFrom_3201X(void)
{
  DeinitFields();
}

static void SwitchTo_3201X(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x74;
  NOPCode = 0x7f80;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode)|(1 << SegData)|(1 << SegIO);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfff;
  Grans[SegData] = 2; ListGrans[SegData] = 2; SegInits[SegData] = 0;
  SegLimits[SegData] = (MomCPU == CPU32010) ? 0x8f : 0xff;
  Grans[SegIO  ] = 2; ListGrans[SegIO  ] = 2; SegInits[SegIO  ] = 0;
  SegLimits[SegIO  ] = 7;

  MakeCode = MakeCode_3201X;
  IsDef = IsDef_3201X;
  InternSymbol = InternSymbol_3201X;
  SwitchFrom = SwitchFrom_3201X;
  InitFields();
}

void code3201x_init(void)
{
  CPU32010 = AddCPU("32010", SwitchTo_3201X);
  CPU32015 = AddCPU("32015", SwitchTo_3201X);
}
