/* tipseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Commonly Used TI-Style Pseudo Instructions-Befehle                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "strutil.h"
#include "be_le.h"
#include "ieeefloat.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "onoff_common.h"
#include "errmsg.h"
#include "chartrans.h"

#include "codepseudo.h"
#include "fourpseudo.h"
#include "tipseudo.h"

#define LEAVE goto func_exit

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static void define_untyped_label(void)
{
  if (LabPart.str.p_str[0])
  {
    PushLocHandle(-1);
    EnterIntSymbol(&LabPart, EProgCounter(), SegNone, False);
    PopLocHandle();
  }
}

static void pseudo_qxx(Integer num)
{
  tStrComp *pArg;
  Boolean ok = True;
  double res;

  if (!ChkArgCnt(1, ArgCntMax))
    return;

  forallargs (pArg, True)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }

    res = ldexp(EvalStrFloatExpression(pArg, Float64, &ok), num);
    if (!ok)
      break;

    if ((res > 32767.49) || (res < -32768.49))
    {
      ok = False;
      WrError(ErrNum_OverRange);
      break;
    }
    WAsmCode[CodeLen++] = res;
  }

  if (!ok)
    CodeLen = 0;
}

static void pseudo_lqxx(Integer num)
{
  tStrComp *pArg;
  Boolean ok = True;
  double res;
  LongInt resli;

  if (!ChkArgCnt(1, ArgCntMax))
    return;

  forallargs (pArg, True)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }

    res = ldexp(EvalStrFloatExpression(pArg, Float64, &ok), num);
    if (!ok)
      break;

    if ((res > 2147483647.49) || (res < -2147483647.49))
    {
      ok = False;
      WrError(ErrNum_OverRange);
      break;
    }
    resli = res;
    WAsmCode[CodeLen++] = resli & 0xffff;
    WAsmCode[CodeLen++] = resli >> 16;
  }

  if (!ok)
    CodeLen = 0;
}

typedef void (*tcallback)(
#ifdef __PROTOS__
Boolean *, int *, LongInt, tSymbolFlags
#endif
);

static void pseudo_store(tcallback callback, Word MaxMultCharLen)
{
  Boolean ok = True;
  int adr = 0;
  tStrComp *pArg;
  TempResult t;

  as_tempres_ini(&t);

  if (!ChkArgCnt(1, ArgCntMax))
    LEAVE;

  define_untyped_label();

  forallargs (pArg, ok)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }

    EvalStrExpression(pArg, &t);
    switch (t.Typ)
    {
      case TempFloat:
        WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
        LEAVE;
      case TempString:
      {
        unsigned char *cp, *cend;

        if (MultiCharToInt(&t, MaxMultCharLen))
          goto ToInt;

        if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
          ok = False;
        else
        {
          cp = (unsigned char *)t.Contents.str.p_str;
          cend = cp + t.Contents.str.len;
          while (cp < cend)
            callback(&ok, &adr, *cp++ & 0xff, t.Flags);
        }
        break;
      }
      case TempInt:
      ToInt:
        callback(&ok, &adr, t.Contents.Int, t.Flags);
        break;
      default:
        ok = False;
        break;
    }
    if (!ok)
      break;
  }

  if (!ok)
    CodeLen = 0;

func_exit:
  as_tempres_free(&t);
}

static void wr_code_byte(Boolean *ok, int *adr, LongInt val, tSymbolFlags Flags)
{
  if (!mFirstPassUnknownOrQuestionable(Flags) && !RangeCheck(val, Int8))
  {
    WrError(ErrNum_OverRange);
    *ok = False;
    return;
  }
  WAsmCode[(*adr)++] = val & 0xff;
  CodeLen = *adr;
}

static void wr_code_word(Boolean *ok, int *adr, LongInt val, tSymbolFlags Flags)
{
  if (!mFirstPassUnknownOrQuestionable(Flags) && !RangeCheck(val, Int16))
  {
    WrError(ErrNum_OverRange);
    *ok = False;
    return;
  }
  WAsmCode[(*adr)++] = val;
  CodeLen = *adr;
}

static void wr_code_long(Boolean *ok, int *adr, LongInt val, tSymbolFlags Flags)
{
  UNUSED(ok);
  UNUSED(Flags);

  WAsmCode[(*adr)++] = val & 0xffff;
  WAsmCode[(*adr)++] = val >> 16;
  CodeLen = *adr;
}

static void wr_code_byte_hilo(Boolean *ok, int *adr, LongInt val, tSymbolFlags Flags)
{
  if (!mFirstPassUnknownOrQuestionable(Flags) && !RangeCheck(val, Int8))
  {
    WrError(ErrNum_OverRange);
    *ok = False;
    return;
  }
  if ((*adr) & 1)
    WAsmCode[((*adr)++) / 2] |= val & 0xff;
  else
    WAsmCode[((*adr)++) / 2] = val << 8;
  CodeLen = ((*adr) + 1) / 2;
}

static void wr_code_byte_lohi(Boolean *ok, int *adr, LongInt val, tSymbolFlags Flags)
{
  if (!mFirstPassUnknownOrQuestionable(Flags) && !RangeCheck(val, Int8))
  {
    WrError(ErrNum_OverRange);
    *ok = False;
    return;
  }
  if ((*adr) & 1)
    WAsmCode[((*adr)++) / 2] |= val << 8;
  else
    WAsmCode[((*adr)++) / 2] = val & 0xff;
  CodeLen = ((*adr) + 1) / 2;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFLOAT(Word Code)
 * \brief  decode FLOAT instruction
 * ------------------------------------------------------------------------ */

static void DecodeFLOAT(Word Code)
{
  Boolean ok;
  tStrComp *pArg;
  Byte Dest[4];

  UNUSED(Code);

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  define_untyped_label();
  ok = True;
  if (SetMaxCodeLen(ArgCnt * 4))
    return;
  forallargs (pArg, ok)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }
    Double_2_ieee4(EvalStrFloatExpression(pArg, Float32, &ok), Dest, False);
    WAsmCode[CodeLen++] = (Word)Dest[0] | ((Word)Dest[1]) << 8;
    WAsmCode[CodeLen++] = (Word)Dest[2] | ((Word)Dest[3]) << 8;
  }
  if (!ok)
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDOUBLE(Word Code)
 * \brief  decode DOUBLE instruction
 * ------------------------------------------------------------------------ */

static void DecodeDOUBLE(Word Code)
{
  Boolean ok;
  tStrComp *pArg;
  Byte Dest[8];

  UNUSED(Code);

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  define_untyped_label();
  ok = True;
  if (SetMaxCodeLen(ArgCnt * 8))
    return;
  forallargs (pArg, ok)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }
    Double_2_ieee8(EvalStrFloatExpression(pArg, Float64, &ok), Dest, False);
    WAsmCode[CodeLen++] = (Word)Dest[0] | ((Word)Dest[1]) << 8;
    WAsmCode[CodeLen++] = (Word)Dest[2] | ((Word)Dest[3]) << 8;
    WAsmCode[CodeLen++] = (Word)Dest[4] | ((Word)Dest[5]) << 8;
    WAsmCode[CodeLen++] = (Word)Dest[6] | ((Word)Dest[7]) << 8;
  }
  if (!ok)
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEFLOAT(Word Code)
 * \brief  decode EFLOAT instruction
 * ------------------------------------------------------------------------ */

static void DecodeEFLOAT(Word Code)
{
  Boolean ok;
  tStrComp *pArg;
  double dbl, mant;
  int exp;

  UNUSED(Code);

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  define_untyped_label();
  ok = True;
  if (SetMaxCodeLen(ArgCnt * 4))
    return;
  forallargs (pArg, ok)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }
    dbl = EvalStrFloatExpression(pArg, Float64, &ok);
    mant = frexp(dbl, &exp);
    WAsmCode[CodeLen++] = ldexp(mant, 15);
    WAsmCode[CodeLen++] = exp - 1;
  }
  if (!ok)
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBFLOAT(Word Code)
 * \brief  decode BFLOAT instruction
 * ------------------------------------------------------------------------ */

static void DecodeBFLOAT(Word Code)
{
  Boolean ok;
  tStrComp *pArg;
  double dbl, mant;
  long lmant;
  int exp;

  UNUSED(Code);

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  define_untyped_label();
  ok = True;
  if (SetMaxCodeLen(ArgCnt * 6))
    return;
  forallargs (pArg, ok)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }
    dbl = EvalStrFloatExpression(pArg, Float64, &ok);
    mant = frexp(dbl, &exp);
    lmant = ldexp(mant, 31);
    WAsmCode[CodeLen++] = (lmant & 0xffff);
    WAsmCode[CodeLen++] = (lmant >> 16);
    WAsmCode[CodeLen++] = exp - 1;
  }
  if (!ok)
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTFLOAT(Word Code)
 * \brief  decode TFLOAT instruction
 * ------------------------------------------------------------------------ */

static void DecodeTFLOAT(Word Code)
{
  Boolean ok;
  tStrComp *pArg;
  double dbl, mant;
  int exp;

  UNUSED(Code);

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  define_untyped_label();
  ok = True;
  if (SetMaxCodeLen(ArgCnt * 12))
    return;
  forallargs (pArg, ok)
  {
    if (!*pArg->str.p_str)
    {
      ok = False;
      break;
    }
    dbl = EvalStrFloatExpression(pArg, Float64, &ok);
    mant = frexp(dbl, &exp);
    mant = modf(ldexp(mant, 15), &dbl);
    WAsmCode[CodeLen + 3] = dbl;
    mant = modf(ldexp(mant, 16), &dbl);
    WAsmCode[CodeLen + 2] = dbl;
    mant = modf(ldexp(mant, 16), &dbl);
    WAsmCode[CodeLen + 1] = dbl;
    mant = modf(ldexp(mant, 16), &dbl);
    WAsmCode[CodeLen] = dbl;
    CodeLen += 4;
    WAsmCode[CodeLen++] = ((exp - 1) & 0xffff);
    WAsmCode[CodeLen++] = ((exp - 1) >> 16);
  }
  if (!ok)
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSTRING(Word Code)
 * \brief  decode STRING instruction
 * ------------------------------------------------------------------------ */

static void DecodeSTRING(Word Code)
{
  UNUSED(Code);

  pseudo_store(wr_code_byte_hilo, 1);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRSTRING(Word Code)
 * \brief  decode RSTRING instruction
 * ------------------------------------------------------------------------ */

static void DecodeRSTRING(Word Code)
{
  UNUSED(Code);

  pseudo_store(wr_code_byte_lohi, 1);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBYTE(Word Code)
 * \brief  decode BYTE instruction
 * ------------------------------------------------------------------------ */

static void DecodeBYTE(Word Code)
{
  UNUSED(Code);

  pseudo_store(wr_code_byte, 1);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeWORD(Word Code)
 * \brief  decode WORD instruction
 * ------------------------------------------------------------------------ */

static void DecodeWORD(Word Code)
{
  UNUSED(Code);

  pseudo_store(wr_code_word, 2);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLONG(Word Code)
 * \brief  decode LONG instruction
 * ------------------------------------------------------------------------ */

static void DecodeLONG(Word Code)
{
  UNUSED(Code);

  pseudo_store(wr_code_long, 4);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBSS(Word Code)
 * \brief  decode BSS instruction
 * ------------------------------------------------------------------------ */

static void DecodeBSS_TI(Word Code)
{
  UNUSED(Code);

  define_untyped_label();
  DecodeRES(Code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDATA_TI(Word Code)
 * \brief  decode TI-specific DATA instruction
 * ------------------------------------------------------------------------ */

static void DecodeDATA_TI(Word Code)
{
  UNUSED(Code);
  DecodeDATA(Int16, Int16);
}

/*!------------------------------------------------------------------------
 * \fn     Boolean Is99(const char *pStr, Integer *pNum)
 * \brief  does string end with number 00...99?
 * \param  pStr string to check
 * \param  pNum appended number if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean Is99(const char *pStr, Integer *pNum)
{
  int l = strlen(pStr);

  if ((l >= 3)
   && as_isdigit(pStr[l - 2])
   && as_isdigit(pStr[l - 1]))
  {
    *pNum = 10 * (pStr[l - 2] - '0') + (pStr[l - 1] - '0');
    return True;
  }
  return False;
}

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

Boolean DecodeTIPseudo(void)
{
  static PInstTable InstTable;
  Integer Num;

  /* Qxx */

  if (!as_strncasecmp(OpPart.str.p_str, "Q", 1)
   && Is99(OpPart.str.p_str, &Num))
  {
    pseudo_qxx(Num);
    return True;
  }

  /* LQxx */

  if (!as_strncasecmp(OpPart.str.p_str, "LQ", 2)
   && Is99(OpPart.str.p_str, &Num))
  {
    pseudo_lqxx(Num);
    return True;
  }

  if (!InstTable)
  {
    InstTable = CreateInstTable(23);
    AddInstTable(InstTable, "RES"    , 0, DecodeRES);
    AddInstTable(InstTable, "BSS"    , 0, DecodeBSS_TI);
    AddInstTable(InstTable, "DATA"   , 0, DecodeDATA_TI);
    AddInstTable(InstTable, "STRING" , 0, DecodeSTRING);
    AddInstTable(InstTable, "RSTRING", 0, DecodeRSTRING);
    AddInstTable(InstTable, "BYTE"   , 0, DecodeBYTE);
    AddInstTable(InstTable, "WORD"   , 0, DecodeWORD);
    AddInstTable(InstTable, "LONG"   , 0, DecodeLONG);
    AddInstTable(InstTable, "FLOAT"  , 0, DecodeFLOAT);
    AddInstTable(InstTable, "DOUBLE" , 0, DecodeDOUBLE);
    AddInstTable(InstTable, "EFLOAT" , 0, DecodeEFLOAT);
    AddInstTable(InstTable, "BFLOAT" , 0, DecodeBFLOAT);
    AddInstTable(InstTable, "TFLOAT" , 0, DecodeTFLOAT);
  }

  return LookupInstTable(InstTable, OpPart.str.p_str);
}

Boolean IsTIDef(void)
{
  static const char *defs[] =
  {
    "BSS", "STRING", "RSTRING",
    "BYTE", "WORD", "LONG", "FLOAT",
    "DOUBLE", "EFLOAT", "BFLOAT",
    "TFLOAT", NULL
  };
  const char **cp = defs;

  while (*cp)
  {
    if (Memo(*cp))
      return True;
    cp++;
  }
  return False;
}

/*-------------------------------------------------------------------------*/
/* Convert IEEE to C3x/C4x floating point format */

static void SplitExt(Double Inp, LongInt *Expo, LongWord *Mant)
{
  Byte Field[8];
  Boolean Sign;
  int z;

  Double_2_ieee8(Inp, Field, False);
  Sign = (Field[7] > 0x7f);
  *Expo = (((LongWord) Field[7] & 0x7f) << 4) + (Field[6] >> 4);
  *Mant = Field[6] & 0x0f;
  if (*Expo != 0)
    *Mant |= 0x10;
  for (z = 5; z > 2; z--)
    *Mant = ((*Mant) << 8) | Field[z];
  *Mant = ((*Mant) << 3) + (Field[2] >> 5);
  *Expo -= 0x3ff;
  if (Sign)
    *Mant = (0xffffffff - *Mant) + 1;
  *Mant = (*Mant) ^ 0x80000000;
}

Boolean ExtToTIC34xShort(Double Inp, Word *Erg)
{
  LongInt Expo;
  LongWord Mant;

  if (Inp == 0)
    *Erg = 0x8000;
  else
  {
    SplitExt(Inp, &Expo, &Mant);
    if (!ChkRange(Expo, -7, 7))
      return False;
    *Erg = ((Expo << 12) & 0xf000) | ((Mant >> 20) & 0xfff);
  }
  return True;
}

Boolean ExtToTIC34xSingle(Double Inp, LongWord *Erg)
{
  LongInt Expo;
  LongWord Mant;

  if (Inp == 0)
    *Erg = 0x80000000;
  else
  {
    SplitExt(Inp, &Expo, &Mant);
    if (!ChkRange(Expo, -127, 127))
      return False;
    *Erg = ((Expo << 24) & 0xff000000) + (Mant >> 8);
  }
  return True;
}

Boolean ExtToTIC34xExt(Double Inp, LongWord *ErgL, LongWord *ErgH)
{
  LongInt Exp;

  if (Inp == 0)
  {
    *ErgH = 0x80;
    *ErgL = 0x00000000;
  }
  else
  {
    SplitExt(Inp, &Exp, ErgL);
    if (!ChkRange(Exp, -127, 127))
      return False;
    *ErgH = Exp & 0xff;
  }
  return True;
}

/*-------------------------------------------------------------------------*/
/* Pseudo Instructions common to C3x/C4x */

static void DecodeSINGLE(Word Code)
{
  Double f;
  tStrComp *pArg;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    forallargs (pArg, True)
      if (OK)
      {
        f = EvalStrFloatExpression(pArg, Float64, &OK);
        if (OK)
          OK = OK && ExtToTIC34xSingle(f, DAsmCode + (CodeLen++));
      }
    if (!OK)
      CodeLen = 0;
  }
}

static void DecodeEXTENDED(Word Code)
{
  Double f;
  tStrComp *pArg;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    forallargs (pArg, True)
      if (OK)
      {
        f = EvalStrFloatExpression(pArg, Float64, &OK);
        if (OK)
          OK = OK && ExtToTIC34xExt(f, DAsmCode + CodeLen + 1, DAsmCode + CodeLen);
        CodeLen += 2;
      }
    if (!OK)
      CodeLen = 0;
  }
}

static void DecodeWORD_TI34x(Word Code)
{
  Boolean OK;
  tStrComp *pArg;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    forallargs (pArg, True)
      if (OK) DAsmCode[CodeLen++] = EvalStrIntExpression(pArg, Int32, &OK);
    if (!OK)
      CodeLen = 0;
  }
}

static void DecodeDATA_TI34x(Word Code)
{
  Boolean OK;
  TempResult t;
  tStrComp *pArg;

  UNUSED(Code);
  as_tempres_ini(&t);

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    forallargs (pArg, OK)
      if (OK)
      {
        EvalStrExpression(pArg, &t);
        switch (t.Typ)
        {
          case TempInt:
          ToInt:
#ifdef HAS64
            if (!RangeCheck(t.Contents.Int, Int32))
            {
              OK = False;
              WrError(ErrNum_OverRange);
            }
            else
#endif
              DAsmCode[CodeLen++] = t.Contents.Int;
            break;
          case TempFloat:
            if (!ExtToTIC34xSingle(t.Contents.Float, DAsmCode + (CodeLen++)))
              OK = False;
            break;
          case TempString:
          {
            if (MultiCharToInt(&t, 4))
              goto ToInt;

            if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
              OK = False;
            else
              string_2_dasm_code(&t.Contents.str, Packing ? 4 : 1, True);
            break;
          }
          default:
            OK = False;
        }
      }
    if (!OK)
      CodeLen = 0;
  }
  as_tempres_free(&t);
}

static void DecodeBSS_TI34x(Word Code)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt Size;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags);
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

void AddTI34xPseudo(TInstTable *pInstTable)
{
  AddInstTable(pInstTable, "SINGLE", 0, DecodeSINGLE);
  AddInstTable(pInstTable, "EXTENDED", 0, DecodeEXTENDED);
  AddInstTable(pInstTable, "WORD", 0, DecodeWORD_TI34x);
  AddInstTable(pInstTable, "DATA", 0, DecodeDATA_TI34x);
  AddInstTable(pInstTable, "BSS", 0, DecodeBSS_TI34x);
}
