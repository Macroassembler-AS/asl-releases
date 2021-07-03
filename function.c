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
#include "nonzstring.h"
#include "strutil.h"
#include "asmdef.h"
#include "errmsg.h"
#include "asmerr.h"
#include "asmpars.h"
#include "function.h"

static void FuncSUBSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int cnt = pArgs[0].Contents.str.len - pArgs[1].Contents.Int;

  UNUSED(ArgCnt);
  if ((pArgs[2].Contents.Int != 0) && (pArgs[2].Contents.Int < cnt))
    cnt = pArgs[2].Contents.Int;
  if (cnt < 0)
    cnt = 0;
  as_tempres_set_c_str(pResult, "");
  as_nonz_dynstr_append_raw(&pResult->Contents.str, pArgs[0].Contents.str.p_str + pArgs[1].Contents.Int, cnt);
}

static void FuncSTRSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_int(pResult, as_nonz_dynstr_find(&pArgs[0].Contents.str, &pArgs[1].Contents.str));
}

static void FuncCHARFROMSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_int(pResult, ((pArgs[1].Contents.Int >= 0) && ((unsigned)pArgs[1].Contents.Int < pArgs[0].Contents.str.len)) ? pArgs[0].Contents.str.p_str[pArgs[1].Contents.Int] : -1);
}

static void FuncEXPRTYPE(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  switch (pArgs[0].Typ)
  {
    case TempInt:
      as_tempres_set_int(pResult, 0);
      break;
    case TempFloat:
      as_tempres_set_int(pResult, 1);
      break;
    case TempString:
      as_tempres_set_int(pResult, 2);
      break;
    default:
      as_tempres_set_int(pResult, -1);
  }
}

/* in Grossbuchstaben wandeln */

static void FuncUPSTRING(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  char *pRun;

  UNUSED(ArgCnt);

  as_tempres_set_str(pResult, &pArgs[0].Contents.str);
  for (pRun = pResult->Contents.str.p_str;
       pRun < pResult->Contents.str.p_str + pResult->Contents.str.len;
       pRun++)
    *pRun = as_toupper(*pRun);
}

/* in Kleinbuchstaben wandeln */

static void FuncLOWSTRING(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  char *pRun;

  UNUSED(ArgCnt);

  as_tempres_set_str(pResult, &pArgs[0].Contents.str);
  for (pRun = pResult->Contents.str.p_str;
       pRun < pResult->Contents.str.p_str + pResult->Contents.str.len;
       pRun++)
    *pRun = as_tolower(*pRun);
}

/* Laenge ermitteln */

static void FuncSTRLEN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_int(pResult, pArgs[0].Contents.str.len);
}

/* Parser aufrufen */

static void FuncVAL(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  String Tmp;

  UNUSED(ArgCnt);

  as_nonz_dynstr_to_c_str(Tmp, &pArgs[0].Contents.str, sizeof(Tmp));
  EvalExpression(Tmp, pResult);
}

static void FuncTOUPPER(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if ((pArgs[0].Contents.Int < 0) || (pArgs[0].Contents.Int > 255)) WrError(ErrNum_OverRange);
  else
    as_tempres_set_int(pResult, toupper(pArgs[0].Contents.Int));
}

static void FuncTOLOWER(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if ((pArgs[0].Contents.Int < 0) || (pArgs[0].Contents.Int > 255)) WrError(ErrNum_OverRange);
  else
    as_tempres_set_int(pResult, tolower(pArgs[0].Contents.Int));
}

static void FuncBITCNT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int z;
  LargeInt in = pArgs[0].Contents.Int, out = 0;

  UNUSED(ArgCnt);

  out = 0;
  for (z = 0; z < LARGEBITS; z++)
  {
    out += (in & 1);
    in >>= 1;
  }
  as_tempres_set_int(pResult, out);
}

static void FuncFIRSTBIT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  LargeInt in = pArgs[0].Contents.Int;
  int out;

  UNUSED(ArgCnt);

  out = 0;
  do
  {
    if (!Odd(in))
      out++;
    in >>= 1;
  }
  while ((out < LARGEBITS) && !Odd(in));
  as_tempres_set_int(pResult, (out >= LARGEBITS) ? -1 : out);
}

static void FuncLASTBIT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int z, out;
  LargeInt in = pArgs[0].Contents.Int;

  UNUSED(ArgCnt);

  out = -1;
  for (z = 0; z < LARGEBITS; z++)
  {
    if (Odd(in))
      out = z;
    in >>= 1;
  }
  as_tempres_set_int(pResult, out);
}

static void FuncBITPOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  LargeInt out;

  UNUSED(ArgCnt);

  pResult->Typ = TempInt;
  if (!SingleBit(pArgs[0].Contents.Int, &out))
  {
    out = -1;
    WrError(ErrNum_NotOneBit);
  }
  as_tempres_set_int(pResult, out);
}

static void FuncABS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  switch (pArgs[0].Typ)
  {
    case TempInt:
      as_tempres_set_int(pResult, (pArgs[0].Contents.Int  < 0) ? -pArgs[0].Contents.Int : pArgs[0].Contents.Int);
      break;
    case TempFloat:
      as_tempres_set_float(pResult, fabs(pArgs[0].Contents.Float));
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
      as_tempres_set_int(pResult, (pArgs[0].Contents.Int < 0) ? -1 : ((pArgs[0].Contents.Int > 0) ? 1 : 0));
      break;
    case TempFloat:
      as_tempres_set_int(pResult, (pArgs[0].Contents.Float < 0) ? -1 : ((pArgs[0].Contents.Float > 0) ? 1 : 0));
      break;
    default:
      as_tempres_set_none(pResult);
  }
}

static void FuncINT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) > IntTypeDefs[LargeSIntType].Max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_OverRange);
  }
  else
    as_tempres_set_int(pResult, (LargeInt) floor(pArgs[0].Contents.Float));
}

static void FuncSQRT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float < 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, sqrt(pArgs[0].Contents.Float));
}

/* trigonometrische Funktionen */

static void FuncSIN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, sin(pArgs[0].Contents.Float));
}

static void FuncCOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, cos(pArgs[0].Contents.Float));
}

static void FuncTAN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (cos(pArgs[0].Contents.Float) == 0.0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, tan(pArgs[0].Contents.Float));
}

static void FuncCOT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  Double FVal = sin(pArgs[0].Contents.Float);
  UNUSED(ArgCnt);

  if (FVal == 0.0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, cos(pArgs[0].Contents.Float) / FVal);
}

/* inverse trigonometrische Funktionen */

static void FuncASIN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) > 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, asin(pArgs[0].Contents.Float));
}

static void FuncACOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) > 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, acos(pArgs[0].Contents.Float));
}

static void FuncATAN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, atan(pArgs[0].Contents.Float));
}

static void FuncACOT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, M_PI / 2 - atan(pArgs[0].Contents.Float));
}

static void FuncEXP(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, exp(pArgs[0].Contents.Float));
}

static void FuncALOG(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 308)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, exp(pArgs[0].Contents.Float * log(10.0)));
}

static void FuncALD(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 1022)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, exp(pArgs[0].Contents.Float * log(2.0)));
}

static void FuncSINH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, sinh(pArgs[0].Contents.Float));
}

static void FuncCOSH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, cosh(pArgs[0].Contents.Float));
}

static void FuncTANH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, tanh(pArgs[0].Contents.Float));
}

static void FuncCOTH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  Double FVal;

  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > 709)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else if ((FVal = tanh(pArgs[0].Contents.Float)) == 0.0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, pResult->Contents.Float = 1.0 / FVal);
}

/* logarithmische & inverse hyperbolische Funktionen */

static void FuncLN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, log(pArgs[0].Contents.Float));
}

static void FuncLOG(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, log10(pArgs[0].Contents.Float));
}

static void FuncLD(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, log(pArgs[0].Contents.Float) / log(2.0));
}

static void FuncASINH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, log(pArgs[0].Contents.Float+sqrt(pArgs[0].Contents.Float * pArgs[0].Contents.Float + 1)));
}

static void FuncACOSH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float < 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, log(pArgs[0].Contents.Float+sqrt(pArgs[0].Contents.Float * pArgs[0].Contents.Float - 1)));
}

static void FuncATANH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) >= 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, 0.5 * log((1 + pArgs[0].Contents.Float) / (1 - pArgs[0].Contents.Float)));
}

static void FuncACOTH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (fabs(pArgs[0].Contents.Float) <= 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, 0.5 * log((pArgs[0].Contents.Float + 1) / (pArgs[0].Contents.Float - 1)));
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
