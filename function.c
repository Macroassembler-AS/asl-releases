/* function.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* internal holder for int/float/string                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "bpemu.h"
#include <ctype.h>
#include "dynstring.h"
#include "strutil.h"
#include "asmdef.h"
#include "errmsg.h"
#include "asmerr.h"
#include "asmpars.h"
#include "function.h"

static void FuncSUBSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int cnt = pArgs[0].Contents.Ascii.Length - pArgs[1].Contents.Int;

  UNUSED(ArgCnt);
  if ((pArgs[2].Contents.Int != 0) && (pArgs[2].Contents.Int < cnt))
    cnt = pArgs[2].Contents.Int;
  if (cnt < 0)
    cnt = 0;
  pResult->Contents.Ascii.Length = 0;
  DynStringAppend(&pResult->Contents.Ascii, pArgs[0].Contents.Ascii.Contents + pArgs[1].Contents.Int, cnt);
  pResult->Typ = TempString;
}

static void FuncSTRSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Contents.Int = DynStringFind(&pArgs[0].Contents.Ascii, &pArgs[1].Contents.Ascii);
  pResult->Typ = TempInt;
}

static void FuncCHARFROMSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  pResult->Contents.Int = ((pArgs[1].Contents.Int >= 0) && ((unsigned)pArgs[1].Contents.Int < pArgs[0].Contents.Ascii.Length)) ? pArgs[0].Contents.Ascii.Contents[pArgs[1].Contents.Int] : -1;
}

static void FuncEXPRTYPE(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  switch (pArgs[0].Typ)
  {
    case TempInt:
      pResult->Contents.Int = 0;
      break;
    case TempFloat:
      pResult->Contents.Int = 1;
      break;
    case TempString:
      pResult->Contents.Int = 2;
      break;
    default:
      pResult->Contents.Int = -1;
  }
}

/* in Grossbuchstaben wandeln */

static void FuncUPSTRING(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  char *pRun;

  UNUSED(ArgCnt);

  pResult->Typ = TempString;
  DynString2DynString(&pResult->Contents.Ascii, &pArgs[0].Contents.Ascii);
  for (pRun = pResult->Contents.Ascii.Contents;
       pRun < pResult->Contents.Ascii.Contents + pResult->Contents.Ascii.Length;
       pRun++)
    *pRun = as_toupper(*pRun);
}

/* in Kleinbuchstaben wandeln */

static void FuncLOWSTRING(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  char *pRun;

  UNUSED(ArgCnt);

  pResult->Typ = TempString;
  DynString2DynString(&pResult->Contents.Ascii, &pArgs[0].Contents.Ascii);
  for (pRun = pResult->Contents.Ascii.Contents;
       pRun < pResult->Contents.Ascii.Contents + pResult->Contents.Ascii.Length;
       pRun++)
    *pRun = as_tolower(*pRun);
}

/* Laenge ermitteln */

static void FuncSTRLEN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  pResult->Contents.Int = pArgs[0].Contents.Ascii.Length;
}

/* Parser aufrufen */

static void FuncVAL(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  String Tmp;

  UNUSED(ArgCnt);

  DynString2CString(Tmp, &pArgs[0].Contents.Ascii, sizeof(Tmp));
  EvalExpression(Tmp, pResult);
}

static void FuncTOUPPER(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if ((pArgs[0].Contents.Int < 0) || (pArgs[0].Contents.Int > 255)) WrError(ErrNum_OverRange);
  else
  {
    pResult->Typ = TempInt;
    pResult->Contents.Int = toupper(pArgs[0].Contents.Int);
  }
}

static void FuncTOLOWER(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if ((pArgs[0].Contents.Int < 0) || (pArgs[0].Contents.Int > 255)) WrError(ErrNum_OverRange);
  else
  {
    pResult->Typ = TempInt;
    pResult->Contents.Int = tolower(pArgs[0].Contents.Int);
  }
}

static void FuncBITCNT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int z;
  LargeInt in = pArgs[0].Contents.Int;

  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  pResult->Contents.Int = 0;
  for (z = 0; z < LARGEBITS; z++)
  {
    pResult->Contents.Int += (in & 1);
    in >>= 1;
  }
}

static void FuncFIRSTBIT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  LargeInt in = pArgs[0].Contents.Int;

  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  pResult->Contents.Int = 0;
  do
  {
    if (!Odd(in))
      pResult->Contents.Int++;
    in >>= 1;
  }
  while ((pResult->Contents.Int < LARGEBITS) && (!Odd(in)));
  if (pResult->Contents.Int >= LARGEBITS)
    pResult->Contents.Int = -1;
}

static void FuncLASTBIT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int z;
  LargeInt in = pArgs[0].Contents.Int;

  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  pResult->Contents.Int = -1;
  for (z = 0; z < LARGEBITS; z++)
  {
    if (Odd(in))
      pResult->Contents.Int = z;
    in >>= 1;
  }
}

static void FuncBITPOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  if (!SingleBit(pArgs[0].Contents.Int, &pResult->Contents.Int))
  {
    pResult->Contents.Int = -1;
    WrError(ErrNum_NotOneBit);
  }
}

static void FuncABS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  switch (pArgs[0].Typ)
  {
    case TempInt:
      pResult->Typ = TempInt;
      pResult->Contents.Int = (pArgs[0].Contents.Int  < 0) ? -pArgs[0].Contents.Int : pArgs[0].Contents.Int;
      break;
    case TempFloat:
      pResult->Typ = TempFloat;
      pResult->Contents.Float = fabs(pArgs[0].Contents.Float);
      break;
    default:
      pResult->Typ = TempNone;
  }
}

static void FuncSGN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  switch (pArgs[0].Typ)
  {
    case TempInt:
      pResult->Typ = TempInt;
      if (pArgs[0].Contents.Int < 0)
        pResult->Contents.Int = -1;
      else if (pArgs[0].Contents.Int > 0)
        pResult->Contents.Int = 1;
      else
        pResult->Contents.Int = 0;
      break;
    case TempFloat:
      pResult->Typ = TempInt;
      if (pArgs[0].Contents.Float < 0)
        pResult->Contents.Int = -1;
      else if (pArgs[0].Contents.Float > 0)
        pResult->Contents.Int = 1;
      else
        pResult->Contents.Int = 0;
      break;
    default:
      pResult->Typ = TempNone;;
  }
}

static void FuncINT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) > IntTypeDefs[LargeSIntType].Max)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_OverRange);
  }
  else
  {
    pResult->Typ = TempInt;
    pResult->Contents.Int = (LargeInt) floor(pArgs[0].Contents.Float);
  }
}

static void FuncSQRT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float < 0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = sqrt(pArgs[0].Contents.Float);
  }
}

/* trigonometrische Funktionen */

static void FuncSIN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempFloat;
  pResult->Contents.Float = sin(pArgs[0].Contents.Float);
}

static void FuncCOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempFloat;
  pResult->Contents.Float = cos(pArgs[0].Contents.Float);
}

static void FuncTAN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (cos(pArgs[0].Contents.Float) == 0.0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = tan(pArgs[0].Contents.Float);
  }
}

static void FuncCOT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  Double FVal = sin(pArgs[0].Contents.Float);
  UNUSED(ArgCnt);

  if (FVal == 0.0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = cos(pArgs[0].Contents.Float) / FVal;
  }
}

/* inverse trigonometrische Funktionen */

static void FuncASIN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) > 1)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = asin(pArgs[0].Contents.Float);
  }
}

static void FuncACOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) > 1)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = acos(pArgs[0].Contents.Float);
  }
}

static void FuncATAN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempFloat;
  pResult->Contents.Float = atan(pArgs[0].Contents.Float);
}

static void FuncACOT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempFloat;
  pResult->Contents.Float = M_PI / 2 - atan(pArgs[0].Contents.Float);
}

static void FuncEXP(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = exp(pArgs[0].Contents.Float);
  }
}

static void FuncALOG(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 308)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = exp(pArgs[0].Contents.Float * log(10.0));
  }
}

static void FuncALD(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 1022)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = exp(pArgs[0].Contents.Float * log(2.0));
  }
}

static void FuncSINH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = sinh(pArgs[0].Contents.Float);
  }
}

static void FuncCOSH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = cosh(pArgs[0].Contents.Float);
  }
}

static void FuncTANH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = tanh(pArgs[0].Contents.Float);
  }
}

static void FuncCOTH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  Double FVal;

  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else if ((FVal = tanh(pArgs[0].Contents.Float)) == 0.0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = 1.0 / FVal;
  }
}

/* logarithmische & inverse hyperbolische Funktionen */

static void FuncLN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = log(pArgs[0].Contents.Float);
  }
}

static void FuncLOG(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = log10(pArgs[0].Contents.Float);
  }
}

static void FuncLD(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_InvFuncArg);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = log(pArgs[0].Contents.Float) / log(2.0);
  }
}

static void FuncASINH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  pResult->Typ = TempFloat;
  pResult->Contents.Float = log(pArgs[0].Contents.Float+sqrt(pArgs[0].Contents.Float * pArgs[0].Contents.Float + 1));
}

static void FuncACOSH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float < 1)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = log(pArgs[0].Contents.Float+sqrt(pArgs[0].Contents.Float * pArgs[0].Contents.Float - 1));
  }
}

static void FuncATANH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) >= 1)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = 0.5 * log((1 + pArgs[0].Contents.Float) / (1 - pArgs[0].Contents.Float));
  }
}

static void FuncACOTH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) <= 1)
  {
    pResult->Typ = TempNone;
    WrError(ErrNum_FloatOverflow);
  }
  else
  {
    pResult->Typ = TempFloat;
    pResult->Contents.Float = 0.5 * log((pArgs[0].Contents.Float + 1) / (pArgs[0].Contents.Float - 1));
  }
}

#define MInt (1 << TempInt)
#define MFloat (1 << TempFloat)
#define MString (1 << TempString)
#define MAll (MInt | MFloat | MString)

const tFunction Functions[] =
{
  { "SUBSTR"     , 3, 3, { MString, MInt, MInt   }, FuncSUBSTR      },
  { "STRSTR"     , 2, 2, { MString, MString, 0   }, FuncSTRSTR      },
  { "CHARFROMSTR", 2, 2, { MString, MInt, 0      }, FuncCHARFROMSTR },
  { "EXPRTYPE"   , 1, 1, { MAll, 0, 0            }, FuncEXPRTYPE    },
  { "UPSTRING"   , 1, 1, { MString, 0, 0         }, FuncUPSTRING    },
  { "LOWSTRING"  , 1, 1, { MString, 0, 0         }, FuncLOWSTRING   },
  { "STRLEN"     , 1, 1, { MString, 0, 0         }, FuncSTRLEN      },
  { "VAL"        , 1, 1, { MString, 0, 0         }, FuncVAL         },
  { "TOUPPER"    , 1, 1, { MInt, 0, 0            }, FuncTOUPPER     },
  { "TOLOWER"    , 1, 1, { MInt, 0, 0            }, FuncTOLOWER     },
  { "BITCNT"     , 1, 1, { MInt, 0, 0            }, FuncBITCNT      },
  { "FIRSTBIT"   , 1, 1, { MInt, 0, 0            }, FuncFIRSTBIT    },
  { "LASTBIT"    , 1, 1, { MInt, 0, 0            }, FuncLASTBIT     },
  { "BITPOS"     , 1, 1, { MInt, 0, 0            }, FuncBITPOS      },
  { "ABS"        , 1, 1, { MInt | MFloat, 0, 0   }, FuncABS         },
  { "SGN"        , 1, 1, { MInt | MFloat, 0, 0   }, FuncSGN         },
  { "INT"        , 1, 1, { MFloat, 0, 0          }, FuncINT         },
  { "SQRT"       , 1, 1, { MFloat, 0, 0          }, FuncSQRT        },
  { "SIN"        , 1, 1, { MFloat, 0, 0          }, FuncSIN         },
  { "COS"        , 1, 1, { MFloat, 0, 0          }, FuncCOS         },
  { "TAN"        , 1, 1, { MFloat, 0, 0          }, FuncTAN         },
  { "COT"        , 1, 1, { MFloat, 0, 0          }, FuncCOT         },
  { "ASIN"       , 1, 1, { MFloat, 0, 0          }, FuncASIN        },
  { "ACOS"       , 1, 1, { MFloat, 0, 0          }, FuncACOS        },
  { "ATAN"       , 1, 1, { MFloat, 0, 0          }, FuncATAN        },
  { "ACOT"       , 1, 1, { MFloat, 0, 0          }, FuncACOT        },
  { "EXP"        , 1, 1, { MFloat, 0, 0          }, FuncEXP         },
  { "ALOG"       , 1, 1, { MFloat, 0, 0          }, FuncALOG        },
  { "ALD"        , 1, 1, { MFloat, 0, 0          }, FuncALD         },
  { "SINH"       , 1, 1, { MFloat, 0, 0          }, FuncSINH        },
  { "COSH"       , 1, 1, { MFloat, 0, 0          }, FuncCOSH        },
  { "TANH"       , 1, 1, { MFloat, 0, 0          }, FuncTANH        },
  { "COTH"       , 1, 1, { MFloat, 0, 0          }, FuncCOTH        },
  { "LN"         , 1, 1, { MFloat, 0, 0          }, FuncLN          },
  { "LOG"        , 1, 1, { MFloat, 0, 0          }, FuncLOG         },
  { "LD"         , 1, 1, { MFloat, 0, 0          }, FuncLD          },
  { "ASINH"      , 1, 1, { MFloat, 0, 0          }, FuncASINH       },
  { "ACOSH"      , 1, 1, { MFloat, 0, 0          }, FuncACOSH       },
  { "ATANH"      , 1, 1, { MFloat, 0, 0          }, FuncATANH       },
  { "ACOTH"      , 1, 1, { MFloat, 0, 0          }, FuncACOTH       },
  { NULL         , 0, 0, { 0, 0, 0               }, NULL            },
};
