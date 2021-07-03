/* motpseudo.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Commonly Used Motorola-Style Pseudo Instructions                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "bpemu.h"
#include "endian.h"
#include "ieeefloat.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmallg.h"
#include "asmcode.h"
#include "errmsg.h"

#include "motpseudo.h"

#define LEAVE goto func_exit

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static Boolean M16Turn = False;

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static Boolean CutRep(tStrComp *pDest, const tStrComp *pSrc, LongInt *pErg, tSymbolFlags *pFlags)
{
  tStrComp Src;

  if (pFlags)
    *pFlags = eSymbolFlag_None;
  StrCompRefRight(&Src, pSrc, 0);
  KillPrefBlanksStrCompRef(&Src);
  if (*Src.str.p_str != '[')
  {
    *pErg = 1;
    *pDest = *pSrc;
    return True;
  }
  else
  {
    tStrComp RepArg;
    char *pEnd;
    Boolean OK;

    pEnd = QuotPos(Src.str.p_str + 1, ']');
    if (!pEnd)
    {
      WrError(ErrNum_BrackErr);
      return False;
    }
    else
    {
      StrCompSplitRef(&RepArg, pDest, &Src, pEnd);
      StrCompIncRefLeft(&RepArg, 1);
      *pErg = EvalStrIntExpressionWithFlags(&RepArg, Int32, &OK, pFlags);
      return OK;
    }
  }
}

static void PutByte(Byte Value)
{
  if ((ListGran() == 1) || (!(CodeLen & 1)))
    BAsmCode[CodeLen] = Value;
  else if (M16Turn)
    WAsmCode[CodeLen >> 1] = (((Word)BAsmCode[CodeLen -1]) << 8) | Value;
  else
    WAsmCode[CodeLen >> 1] = (((Word)Value) << 8) | BAsmCode[CodeLen -1];
  CodeLen++;
}

void DecodeMotoBYT(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, ArgCntMax))
  {
    ShortInt SpaceFlag = -1;
    tStrComp *pArg, Arg;
    LongInt Rep;
    Boolean OK = True;

    forallargs (pArg, OK)
    {
      if (!*pArg->str.p_str)
      {
        OK = FALSE;
        WrError(ErrNum_EmptyArgument);
        break;
      }

      OK = CutRep(&Arg, pArg, &Rep, NULL);
      if (!OK)
        break;

      if (!strcmp(Arg.str.p_str, "?"))
      {
        if (SpaceFlag == 0)
        {
          WrError(ErrNum_MixDBDS);
          OK = FALSE;
        }
        else
        {
          SpaceFlag = 1;
          CodeLen += Rep;
        }
      }
      else if (SpaceFlag == 1)
      {
        WrError(ErrNum_MixDBDS);
        OK = FALSE;
      }
      else
      {
        TempResult t;

        SpaceFlag = 0;

        as_tempres_ini(&t);
        EvalStrExpression(&Arg, &t);
        switch (t.Typ)
        {
          case TempInt:
          ToInt:
            if (!mFirstPassUnknownOrQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, Int8))
            {
              WrStrErrorPos(ErrNum_OverRange, &Arg);
              OK = False;
            }
            else if (SetMaxCodeLen(CodeLen + Rep))
            {
              WrError(ErrNum_CodeOverflow);
              OK = False;
            }
            else
            {
              LongInt z2;

              for (z2 = 0; z2 < Rep; z2++)
                PutByte(t.Contents.Int);
            }
            break;

          case TempFloat:
            WrStrErrorPos(ErrNum_StringOrIntButFloat, &Arg);
            OK = False;
            break;

          case TempString:
          {
            int l;

            if (MultiCharToInt(&t, 1))
              goto ToInt;

            l = t.Contents.str.len;
            TranslateString(t.Contents.str.p_str, l);

            if (SetMaxCodeLen(CodeLen + (Rep * l)))
            {
              WrError(ErrNum_CodeOverflow);
              OK = False;
            }
            else
            {
              LongInt z2;
              int z3;

              for (z2 = 0; z2 < Rep; z2++)
                for (z3 = 0; z3 < l; z3++)
                  PutByte(t.Contents.str.p_str[z3]);
            }
            break;
          }

          default:
            OK = False;
            break;
        }
        as_tempres_free(&t);
      }
    }

    if (!OK)
      CodeLen = 0;
    else
    {
      if (SpaceFlag == 1)
        DontPrint = True;
      if (*LabPart.str.p_str)
        SetSymbolOrStructElemSize(&LabPart, eSymbolSize8Bit);
    }
  }
}

static void PutADR(Word Value)
{
  if (ListGran() > 1)
  {
    WAsmCode[CodeLen >> 1] = Value;
    CodeLen += 2;
  }
  else if (M16Turn)
  {
    BAsmCode[CodeLen++] = Hi(Value);
    BAsmCode[CodeLen++] = Lo(Value);
  }
  else
  {
    BAsmCode[CodeLen++] = Lo(Value);
    BAsmCode[CodeLen++] = Hi(Value);
  }
}

void DecodeMotoADR(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, ArgCntMax))
  {
    tStrComp *pArg, Arg;
    Boolean OK = True;
    LongInt Rep;
    ShortInt SpaceFlag = -1;

    forallargs (pArg, OK)
    {
      if (!*pArg->str.p_str)
      {
        OK = FALSE;
        WrError(ErrNum_EmptyArgument);
        break;
      }

      OK = CutRep(&Arg, pArg, &Rep, NULL);
      if (!OK)
        break;

      if (!strcmp(Arg.str.p_str, "?"))
      {
        if (SpaceFlag == 0)
        {
          WrError(ErrNum_MixDBDS);
          OK = False;
        }
        else
        {
          SpaceFlag = 1;
          CodeLen += 2 * Rep;
        }
      }
      else if (SpaceFlag == 1)
      {
        WrError(ErrNum_MixDBDS);
        OK = False;
      }
      else
      {
        TempResult Res;
        LongInt z2, Cnt;

        SpaceFlag = 0;
        as_tempres_ini(&Res);
        EvalStrExpression(&Arg, &Res);

        switch (Res.Typ)
        {
          case TempInt:
          ToInt:
            if (mFirstPassUnknown(Res.Flags))
              Res.Contents.Int &= 0xffff;
            if (!mSymbolQuestionable(Res.Flags) && !RangeCheck(Res.Contents.Int, Int16))
            {
              WrError(ErrNum_OverRange);
              Res.Typ = TempNone;
            }
            Cnt = 1;
            break;
          case TempString:
            if (MultiCharToInt(&Res, 2))
              goto ToInt;
            Cnt = Res.Contents.str.len;
            TranslateString(Res.Contents.str.p_str, Res.Contents.str.len);
            break;
          case TempFloat:
            WrStrErrorPos(ErrNum_StringOrIntButFloat, &Arg);
            /* fall-through */
          default:
            Res.Typ = TempNone;
            Cnt = 0;
            break;
        }
        if (TempNone == Res.Typ)
        {
          OK = False;
          break;
        }

        if (SetMaxCodeLen(CodeLen + ((Cnt * Rep) << 1)))
        {
          WrError(ErrNum_CodeOverflow);
          OK = False;
          break;
        }

        for (z2 = 0; z2 < Rep; z2++)
          switch (Res.Typ)
          {
            case TempInt:
              PutADR(Res.Contents.Int);
              break;
            case TempString:
            {
              unsigned z3;

              for (z3 = 0; z3 < Res.Contents.str.len; z3++)
                PutADR(Res.Contents.str.p_str[z3]);
              break;
            }
            default:
              break;
          }
        as_tempres_free(&Res);
      }
    }

    if (!OK)
      CodeLen = 0;
    else
    {
      if (SpaceFlag)
        DontPrint = True;
      if (*LabPart.str.p_str)
        SetSymbolOrStructElemSize(&LabPart, eSymbolSize16Bit);
    }
  }
}

static void DecodeFCC(Word Index)
{
  String SVal;
  Boolean OK;
  tStrComp *pArg, Arg;
  int z3, l;
  LongInt Rep,z2;
  UNUSED(Index);

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;

    forallargs (pArg, OK)
    {
      if (!*pArg->str.p_str)
      {
        OK = FALSE;
        WrError(ErrNum_EmptyArgument);
        break;
      }

      OK = CutRep(&Arg, pArg, &Rep, NULL);
      if (!OK)
        break;

      EvalStrStringExpression(&Arg, &OK, SVal);
      if (OK)
      {
        if (SetMaxCodeLen(CodeLen + Rep * strlen(SVal)))
        {
          WrError(ErrNum_CodeOverflow);
          OK = False;
        }
        else
        {
          l = strlen(SVal);
          TranslateString(SVal, l);
          for (z2 = 0; z2 < Rep; z2++)
            for (z3 = 0; z3 < l; z3++)
              PutByte(SVal[z3]);
        }
      }
    }

    if (!OK)
      CodeLen = 0;
    else if (*LabPart.str.p_str)
      SetSymbolOrStructElemSize(&LabPart, eSymbolSize8Bit);
  }
}

void DecodeMotoDFS(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    Word HVal16;
    Boolean OK;
    tSymbolFlags Flags;

    HVal16 = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      WrError(ErrNum_FirstPassCalc);
    else if (OK)
    {
      DontPrint = True;
      CodeLen = HVal16;
      if (!HVal16)
        WrError(ErrNum_NullResMem);
      BookKeeping();
      if (*LabPart.str.p_str)
        SetSymbolOrStructElemSize(&LabPart, eSymbolSize8Bit);
    }
  }
}

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

Boolean DecodeMotoPseudo(Boolean Turn)
{
  static PInstTable InstTable = NULL;

  if (!InstTable)
  {
    InstTable = CreateInstTable(17);
    AddInstTable(InstTable, "BYT", 0, DecodeMotoBYT);
    AddInstTable(InstTable, "FCB", 0, DecodeMotoBYT);
    AddInstTable(InstTable, "ADR", 0, DecodeMotoADR);
    AddInstTable(InstTable, "FDB", 0, DecodeMotoADR);
    AddInstTable(InstTable, "FCC", 0, DecodeFCC);
    AddInstTable(InstTable, "DFS", 0, DecodeMotoDFS);
    AddInstTable(InstTable, "RMB", 0, DecodeMotoDFS);
  }

  M16Turn = Turn;
  return LookupInstTable(InstTable, OpPart.str.p_str);
}

static void DigIns(char Ch, int Pos, Byte *pDest)
{
  int bytepos = Pos >> 1, bitpos = (Pos & 1) << 2;
  Byte dig = Ch - '0';

  pDest[bytepos] |= (dig << bitpos);
}

void ConvertMotoFloatDec(Double F, Byte *pDest, Boolean NeedsBig)
{
  char s[30], Man[30], Exp[30];
  char *pSplit;
  int z, ManLen, ExpLen;
  Byte epos;

  UNUSED(NeedsBig);

  /* convert to ASCII, split mantissa & exponent */

  as_snprintf(s, sizeof(s), "%0.16e", F);
  pSplit = strchr(s, HexStartCharacter + ('e' - 'a'));
  if (!pSplit)
  {
    strcpy(Man, s);
    strcpy(Exp, "+0000");
  }
  else
  {
    *pSplit = '\0';
    strcpy(Man, s);
    strcpy(Exp, pSplit + 1);
  }

  memset(pDest, 0, 12);

  /* handle mantissa sign */

  if (*Man == '-')
  {
    pDest[11] |= 0x80; strmov(Man, Man + 1);
  }
  else if (*Man == '+')
    strmov(Man, Man + 1);

  /* handle exponent sign */

  if (*Exp == '-')
  {
    pDest[11] |= 0x40;
    strmov(Exp, Exp + 1);
  }
  else if (*Exp == '+')
    strmov(Exp, Exp + 1);

  /* integral part of mantissa (one digit) */

  DigIns(*Man, 16, pDest);
  strmov(Man, Man + 2);

  /* truncate mantissa if we have more digits than we can represent */

  if (strlen(Man) > 16)
    Man[16] = '\0';

  /* insert mantissa digits */

  ManLen = strlen(Man);
  for (z = 0; z < ManLen; z++)
    DigIns(Man[z], 15 - z, pDest);

  /* truncate exponent if we have more digits than we can represent - this should
     never occur since an IEEE double is limited to ~1E308 and we have for digits */

  if (strlen(Exp) > 4)
    strmov(Exp, Exp + strlen(Exp) - 4);

  /* insert exponent bits */

  ExpLen = strlen(Exp);
  for (z = ExpLen - 1; z >= 0; z--)
  {
    epos = ExpLen - 1 - z;
    if (epos == 3)
      DigIns(Exp[z], 19, pDest);
    else
      DigIns(Exp[z], epos + 20, pDest);
  }

  if (HostBigEndian)
    WSwap(pDest, 12);
}

static void EnterByte(LargeWord b)
{
  if (((CodeLen & 1) == 1) && (!HostBigEndian) && (ListGran() != 1))
  {
    BAsmCode[CodeLen    ] = BAsmCode[CodeLen - 1];
    BAsmCode[CodeLen - 1] = b;
  }
  else
  {
    BAsmCode[CodeLen] = b;
  }
  CodeLen++;
}

static void EnterWord(LargeWord w)
{
  if (ListGran() == 1)
  {
    BAsmCode[CodeLen    ] = Hi(w);
    BAsmCode[CodeLen + 1] = Lo(w);
  }
  else
    WAsmCode[CodeLen >> 1] = w;
  CodeLen += 2;
}

static void EnterPointer(LargeWord w)
{
  EnterByte((w >> 16) & 0xff);
  EnterByte((w >>  8) & 0xff);
  EnterByte((w      ) & 0xff);
}

static void EnterLWord(LargeWord l)
{
  if (ListGran() == 1)
  {
    BAsmCode[CodeLen    ] = (l >> 24) & 0xff;
    BAsmCode[CodeLen + 1] = (l >> 16) & 0xff;
    BAsmCode[CodeLen + 2] = (l >>  8) & 0xff;
    BAsmCode[CodeLen + 3] = (l      ) & 0xff;
  }
  else
  {
    WAsmCode[(CodeLen >> 1)    ] = (l >> 16) & 0xffff;
    WAsmCode[(CodeLen >> 1) + 1] = (l      ) & 0xffff;
  }
  CodeLen += 4;
}

static void EnterQWord(LargeWord q)
{
  if (ListGran() == 1)
  {
#ifdef HAS64
    BAsmCode[CodeLen    ] = (q >> 56) & 0xff;
    BAsmCode[CodeLen + 1] = (q >> 48) & 0xff;
    BAsmCode[CodeLen + 2] = (q >> 40) & 0xff;
    BAsmCode[CodeLen + 3] = (q >> 32) & 0xff;
#else
    /* TempResult is LargeInt, so sign-extend */
    BAsmCode[CodeLen    ] =
    BAsmCode[CodeLen + 1] =
    BAsmCode[CodeLen + 2] =
    BAsmCode[CodeLen + 3] = (q & 0x80000000ul) ? 0xff : 0x00;
#endif
    BAsmCode[CodeLen + 4] = (q >> 24) & 0xff;
    BAsmCode[CodeLen + 5] = (q >> 16) & 0xff;
    BAsmCode[CodeLen + 6] = (q >>  8) & 0xff;
    BAsmCode[CodeLen + 7] = (q      ) & 0xff;
  }
  else
  {
#ifdef HAS64
    WAsmCode[(CodeLen >> 1)    ] = (q >> 48) & 0xffff;
    WAsmCode[(CodeLen >> 1) + 1] = (q >> 32) & 0xffff;
#else
    /* TempResult is LargeInt, so sign-extend */
    WAsmCode[(CodeLen >> 1)    ] =
    WAsmCode[(CodeLen >> 1) + 1] = (q & 0x80000000ul) ? 0xffff : 0x00;
#endif
    WAsmCode[(CodeLen >> 1) + 2] = (q >> 16) & 0xffff;
    WAsmCode[(CodeLen >> 1) + 3] = (q      ) & 0xffff;
  }
  CodeLen += 8;
}

static void EnterIEEE2(Word *pField)
{
  if (ListGran() == 1)
  {
     BAsmCode[CodeLen    ] = Hi(pField[1]);
     BAsmCode[CodeLen + 1] = Lo(pField[0]);
  }
  else
  {
    WAsmCode[(CodeLen >> 1)    ] = pField[0];
  }
  CodeLen += 2;
}

static void EnterIEEE4(Word *pField)
{
  if (ListGran() == 1)
  {
     BAsmCode[CodeLen    ] = Hi(pField[1]);
     BAsmCode[CodeLen + 1] = Lo(pField[1]);
     BAsmCode[CodeLen + 2] = Hi(pField[0]);
     BAsmCode[CodeLen + 3] = Lo(pField[0]);
  }
  else
  {
    WAsmCode[(CodeLen >> 1)    ] = pField[1];
    WAsmCode[(CodeLen >> 1) + 1] = pField[0];
  }
  CodeLen += 4;
}

static void EnterIEEE8(Word *pField)
{
  if (ListGran() == 1)
  {
    BAsmCode[CodeLen    ] = Hi(pField[3]);
    BAsmCode[CodeLen + 1] = Lo(pField[3]);
    BAsmCode[CodeLen + 2] = Hi(pField[2]);
    BAsmCode[CodeLen + 3] = Lo(pField[2]);
    BAsmCode[CodeLen + 4] = Hi(pField[1]);
    BAsmCode[CodeLen + 5] = Lo(pField[1]);
    BAsmCode[CodeLen + 6] = Hi(pField[0]);
    BAsmCode[CodeLen + 7] = Lo(pField[0]);
  }
  else
  {
    WAsmCode[(CodeLen >> 1)    ] = pField[3];
    WAsmCode[(CodeLen >> 1) + 1] = pField[2];
    WAsmCode[(CodeLen >> 1) + 2] = pField[1];
    WAsmCode[(CodeLen >> 1) + 3] = pField[0];
  }
  CodeLen += 8;
}

static void EnterIEEE10(Word *pField)
{
  if (ListGran() == 1)
  {
    BAsmCode[CodeLen    ] = Hi(pField[4]);
    BAsmCode[CodeLen + 1] = Lo(pField[4]);
    BAsmCode[CodeLen + 2] = 0;
    BAsmCode[CodeLen + 3] = 0;
    BAsmCode[CodeLen + 4] = Hi(pField[3]);
    BAsmCode[CodeLen + 5] = Lo(pField[3]);
    BAsmCode[CodeLen + 6] = Hi(pField[2]);
    BAsmCode[CodeLen + 7] = Lo(pField[2]);
    BAsmCode[CodeLen + 8] = Hi(pField[1]);
    BAsmCode[CodeLen + 9] = Lo(pField[1]);
    BAsmCode[CodeLen +10] = Hi(pField[0]);
    BAsmCode[CodeLen +11] = Lo(pField[0]);
  }
  else
  {
    WAsmCode[(CodeLen >> 1)    ] = pField[4];
    WAsmCode[(CodeLen >> 1) + 1] = 0;
    WAsmCode[(CodeLen >> 1) + 2] = pField[3];
    WAsmCode[(CodeLen >> 1) + 3] = pField[2];
    WAsmCode[(CodeLen >> 1) + 4] = pField[1];
    WAsmCode[(CodeLen >> 1) + 5] = pField[0];
  }
  CodeLen += 12;
}

static void EnterMotoFloatDec(Word *pField)
{
  if (ListGran() == 1)
  {
    BAsmCode[CodeLen    ] = Hi(pField[5]);
    BAsmCode[CodeLen + 1] = Lo(pField[5]);
    BAsmCode[CodeLen + 2] = Hi(pField[4]);
    BAsmCode[CodeLen + 3] = Lo(pField[4]);
    BAsmCode[CodeLen + 4] = Hi(pField[3]);
    BAsmCode[CodeLen + 5] = Lo(pField[3]);
    BAsmCode[CodeLen + 6] = Hi(pField[2]);
    BAsmCode[CodeLen + 7] = Lo(pField[2]);
    BAsmCode[CodeLen + 8] = Hi(pField[1]);
    BAsmCode[CodeLen + 9] = Lo(pField[1]);
    BAsmCode[CodeLen +10] = Hi(pField[0]);
    BAsmCode[CodeLen +11] = Lo(pField[0]);
  }
  else
  {
    WAsmCode[(CodeLen >> 1)    ] = pField[5];
    WAsmCode[(CodeLen >> 1) + 1] = pField[4];
    WAsmCode[(CodeLen >> 1) + 2] = pField[3];
    WAsmCode[(CodeLen >> 1) + 3] = pField[2];
    WAsmCode[(CodeLen >> 1) + 4] = pField[1];
    WAsmCode[(CodeLen >> 1) + 5] = pField[0];
  }
  CodeLen += 12;
}

static void Double_2_ieee2_wrap(Double Inp, Byte *pDest, Boolean BigEndian)
{
  (void)Double_2_ieee2(Inp, pDest, BigEndian);
}

void AddMoto16PseudoONOFF(void)
{
  AddONOFF("PADDING", &DoPadding, DoPaddingName, False);
}

/*!------------------------------------------------------------------------
 * \fn     GetWSize(tSymbolSize OpSize)
 * \brief  return size in bytes of data type
 * \param  OpSize data type
 * \return size in bytes
 * ------------------------------------------------------------------------ */

static Word GetWSize(tSymbolSize OpSize)
{
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      return 1;
    case eSymbolSize16Bit:
      return 2;
    case eSymbolSize24Bit:
      return 3;
    case eSymbolSize32Bit:
      return 4;
    case eSymbolSize64Bit:
      return 8;
    case eSymbolSizeFloat32Bit:
      return 4;
    case eSymbolSizeFloat64Bit:
      return 8;

    /* NOTE: Double_2_ieee10() creates 10 bytes, but WSize is set to 12 (two
       padding bytes in binary representation).  This means that WSwap() will
       swap 12 instead of 10 bytes, which doesn't hurt, since TurnField is
       large enough and the two (garbage) bytes at the end will not be used
       by EnterIEEE10() anyway: */

    case eSymbolSizeFloat96Bit:
      return 12;
    case eSymbolSizeFloatDec96Bit:
      return 12;
    case eSymbolSizeFloat16Bit:
      return 2;
    default:
      return 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMotoDC(void)
 * \brief  decode DC.x instruction
 * ------------------------------------------------------------------------ */

void DecodeMotoDC(tSymbolSize OpSize, Boolean Turn)
{
  ShortInt SpaceFlag;
  tStrComp *pArg, Arg;
  LongInt z2, WSize, Rep = 0;
  char *zp;
  Boolean OK;
  TempResult t;
  tSymbolFlags Flags;
  void (*EnterInt)(LargeWord) = NULL;
  void (*ConvertFloat)(Double, Byte*, Boolean) = NULL;
  void (*EnterFloat)(Word*) = NULL;
  void (*Swap)(void*, int) = NULL;
  IntType IntTypeEnum = UInt1;
  FloatType FloatTypeEnum = Float32;
  Boolean PadBeforeStart = Odd(EProgCounter()) && DoPadding && (OpSize != eSymbolSize8Bit);

  UNUSED(Turn);

  as_tempres_ini(&t);
  if (*LabPart.str.p_str)
    SetSymbolOrStructElemSize(&LabPart, OpSize);

  if (!ChkArgCnt(1, ArgCntMax))
    LEAVE;

  if (OpSize < 0)
    OpSize = eSymbolSize16Bit;

  WSize = GetWSize(OpSize);
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      EnterInt = EnterByte;
      IntTypeEnum = Int8;
      break;
    case eSymbolSize16Bit:
      EnterInt = EnterWord;
      IntTypeEnum = Int16;
      break;
    case eSymbolSize24Bit:
      EnterInt = EnterPointer;
      IntTypeEnum = Int24;
      break;
    case eSymbolSize32Bit:
      EnterInt = EnterLWord;
      IntTypeEnum = Int32;
      break;
    case eSymbolSize64Bit:
      EnterInt = EnterQWord;
#ifdef HAS64
      IntTypeEnum = Int64;
#else
      IntTypeEnum = Int32;
#endif
      break;
    case eSymbolSizeFloat16Bit:
      ConvertFloat = Double_2_ieee2_wrap;
      EnterFloat = EnterIEEE2;
      FloatTypeEnum = Float16;
      Swap = NULL;
      break;
    case eSymbolSizeFloat32Bit:
      ConvertFloat = Double_2_ieee4;
      EnterFloat = EnterIEEE4;
      FloatTypeEnum = Float32;
      Swap = DWSwap;
      break;
    case eSymbolSizeFloat64Bit:
      ConvertFloat = Double_2_ieee8;
      EnterFloat = EnterIEEE8;
      FloatTypeEnum = Float64;
      Swap = QWSwap;
      break;

    /* NOTE: Double_2_ieee10() creates 10 bytes, but WSize is set to 12 (two
       padding bytes in binary representation).  This means that WSwap() will
       swap 12 instead of 10 bytes, which doesn't hurt, since TurnField is
       large enough and the two (garbage) bytes at the end will not be used
       by EnterIEEE10() anyway: */

    case eSymbolSizeFloat96Bit:
      ConvertFloat = Double_2_ieee10;
      EnterFloat = EnterIEEE10;
      FloatTypeEnum = Float80;
      Swap = TWSwap;
      break;
    case eSymbolSizeFloatDec96Bit:
      ConvertFloat = ConvertMotoFloatDec;
      EnterFloat = EnterMotoFloatDec;
      FloatTypeEnum = FloatDec;
      break;
    default:
      break;
  }

  OK = True;
  SpaceFlag = -1;

  forallargs (pArg, OK)
  {
    if (!*pArg->str.p_str)
    {
      OK = FALSE;
      WrError(ErrNum_EmptyArgument);
      break;
    }

    OK = CutRep(&Arg, pArg, &Rep, &Flags);
    if (!OK)
      break;
    if (mFirstPassUnknown(Flags))
    {
      OK = FALSE;
      WrError(ErrNum_FirstPassCalc);
      break;
    }

    if (!strcmp(Arg.str.p_str, "?"))
    {
      if (SpaceFlag == 0)
      {
        WrError(ErrNum_MixDBDS);
        OK = FALSE;
      }
      else
      {
        if (PadBeforeStart)
        {
          InsertPadding(1, True);
          PadBeforeStart = False;
        }

        SpaceFlag = 1;
        CodeLen += (Rep * WSize);
      }
    }
    else if (SpaceFlag == 1)
    {
      WrError(ErrNum_MixDBDS);
      OK = FALSE;
    }
    else
    {
      SpaceFlag = 0;

      if (PadBeforeStart)
      {
        InsertPadding(1, False);
        PadBeforeStart = False;
      }

      EvalStrExpression(&Arg, &t);

      switch (t.Typ)
      {
        case TempInt:
        ToInt:
          if (!EnterInt)
          {
            if (ConvertFloat && EnterFloat)
            {
              TempResultToFloat(&t);
              goto HandleFloat;
            }
            else
            {
              WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
              OK = False;
            }
          }
          else if (!mFirstPassUnknownOrQuestionable(t.Flags) && !RangeCheck(t.Contents.Int, IntTypeEnum))
          {
            WrError(ErrNum_OverRange);
            OK = False;
          }
          else if (SetMaxCodeLen(CodeLen + (Rep * WSize)))
          {
            WrError(ErrNum_CodeOverflow);
            OK = False;
          }
          else
            for (z2 = 0; z2 < Rep; z2++)
              EnterInt(t.Contents.Int);
          break;
        HandleFloat:
        case TempFloat:
          if (!ConvertFloat || !EnterFloat)
          {
            WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
            OK = False;
          }
          else if (!FloatRangeCheck(t.Contents.Float, FloatTypeEnum))
          {
            WrError(ErrNum_OverRange);
            OK = False;
          }
          else if (SetMaxCodeLen(CodeLen + (Rep * WSize)))
          {
            WrError(ErrNum_CodeOverflow);
            OK = False;
          }
          else
          {
            Word TurnField[8];

            ConvertFloat(t.Contents.Float, (Byte *) TurnField, HostBigEndian);
            if (HostBigEndian && Swap)
              Swap((void*) TurnField, WSize);
            for (z2 = 0; z2 < Rep; z2++)
              EnterFloat(TurnField);
          }
          break;
        case TempString:
          if (MultiCharToInt(&t, (WSize < 8) ? WSize : 8))
            goto ToInt;
          if (!EnterInt)
          {
            if (ConvertFloat && EnterFloat)
            {
              if (SetMaxCodeLen(CodeLen + (Rep * WSize * t.Contents.str.len)))
              {
                WrError(ErrNum_CodeOverflow);
                OK = False;
              }
              else
              {
                for (z2 = 0; z2 < Rep; z2++)
                  for (zp = t.Contents.str.p_str; zp < t.Contents.str.p_str + t.Contents.str.len; zp++)
                  {
                    Word TurnField[8];

                    ConvertFloat(CharTransTable[(usint) (*zp & 0xff)], (Byte *) TurnField, HostBigEndian);
                    if (HostBigEndian && Swap)
                      Swap((void*) TurnField, WSize);
                    EnterFloat(TurnField);
                  }
              }
            }
            else
            {
              WrError(ErrNum_FloatButString);
              OK = False;
            }
          }
          else if (SetMaxCodeLen(CodeLen + Rep * t.Contents.str.len))
          {
            WrError(ErrNum_CodeOverflow);
            OK = False;
          }
          else
            for (z2 = 0; z2 < Rep; z2++)
              for (zp = t.Contents.str.p_str; zp < t.Contents.str.p_str + t.Contents.str.len; EnterInt(CharTransTable[((usint) *(zp++)) & 0xff]));
          break;
        case TempNone:
          OK = False;
          break;
        default:
          assert(0);
      }

    }
  }

  /* purge results if an error occured */

  if (!OK) CodeLen = 0;

  /* just space reservation ? */

  else if (SpaceFlag == 1)
    DontPrint = True;

func_exit:
  as_tempres_free(&t);
}

Boolean DecodeMoto16Pseudo(tSymbolSize OpSize, Boolean Turn)
{
  LongInt NewPC, HVal;
  Boolean ValOK;
  tSymbolFlags Flags;
  Boolean PadBeforeStart;

  if (OpSize < 0)
    OpSize = eSymbolSize16Bit;

  PadBeforeStart = Odd(EProgCounter()) && DoPadding && (OpSize != eSymbolSize8Bit);
  if (*OpPart.str.p_str != 'D')
    return False;

  if (Memo("DC"))
  {
    DecodeMotoDC(OpSize, Turn);
    return True;
  }

  if (Memo("DS"))
  {
    Word WSize = GetWSize(OpSize);

    if (ChkArgCnt(1, 1))
    {
      HVal = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ValOK, &Flags);
      if (mFirstPassUnknown(Flags))
        WrError(ErrNum_FirstPassCalc);
      if (ValOK && !mFirstPassUnknown(Flags))
      {
        Boolean OddSize = (eSymbolSize8Bit == OpSize) || (eSymbolSize24Bit == OpSize);

        if (PadBeforeStart)
        {
          InsertPadding(1, True);
          PadBeforeStart = False;
        }

        DontPrint = True;

        /* value of 0 means aligning the PC.  Doesn't make sense for bytes and 24 bit values */

        if ((HVal == 0) && !OddSize)
        {
          NewPC = EProgCounter() + WSize - 1;
          NewPC -= NewPC % WSize;
          CodeLen = NewPC - EProgCounter();
          if (CodeLen == 0)
          {
            DontPrint = False;
            if (WSize == 1)
              WrError(ErrNum_NullResMem);
          }
        }
        else
          CodeLen = HVal * WSize;
        if (DontPrint)
          BookKeeping();
      }
    }
    if (*LabPart.str.p_str)
      SetSymbolOrStructElemSize(&LabPart, OpSize);
    return True;
  }

  return False;
}

static Boolean DecodeMoto16AttrSizeCore(char SizeSpec, tSymbolSize *pResult, Boolean Allow24)
{
  switch (as_toupper(SizeSpec))
  {
    case 'B': *pResult = eSymbolSize8Bit; break;
    case 'W': *pResult = eSymbolSize16Bit; break;
    case 'L': *pResult = eSymbolSize32Bit; break;
    case 'Q': *pResult = eSymbolSize64Bit; break;
    case 'S': *pResult = eSymbolSizeFloat32Bit; break;
    case 'D': *pResult = eSymbolSizeFloat64Bit; break;
    case 'X': *pResult = eSymbolSizeFloat96Bit; break;
    case 'C': *pResult = eSymbolSizeFloat16Bit; break;
    case 'P': *pResult = Allow24 ? eSymbolSize24Bit : eSymbolSizeFloatDec96Bit; break;
    case '\0': break;
    default:
      return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMoto16AttrSize(char SizeSpec, tSymbolSize *pResult, Boolean Allow24)
 * \brief  decode Motorola-style operand size character
 * \param  SizeSpec size specifier character
 * \param  pResult returns result size
 * \param  Allow24 allow 'p' as specifier for 24 Bits (S12Z-specific)
 * \return True if decoded
 * ------------------------------------------------------------------------ */

Boolean DecodeMoto16AttrSize(char SizeSpec, tSymbolSize *pResult, Boolean Allow24)
{
  if (!DecodeMoto16AttrSizeCore(SizeSpec, pResult, Allow24))
  {
    WrError(ErrNum_UndefAttr);
    return False;
  }
  return True;
}

Boolean DecodeMoto16AttrSizeStr(const struct sStrComp *pSizeSpec, tSymbolSize *pResult, Boolean Allow24)
{
  if ((strlen(pSizeSpec->str.p_str) > 1)
   || !DecodeMoto16AttrSizeCore(*pSizeSpec->str.p_str, pResult, Allow24))
  {
    WrStrErrorPos(ErrNum_UndefAttr, pSizeSpec);
    return False;
  }
  return True;
}
