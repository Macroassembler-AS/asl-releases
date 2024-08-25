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
#include <string.h>
#include <ctype.h>
#include <float.h>
#include "nonzstring.h"
#include "strutil.h"
#include "asmdef.h"
#include "errmsg.h"
#include "asmerr.h"
#include "chartrans.h"
#include "asmpars.h"
#include "cpu2phys.h"
#include "function.h"

static as_float_t log_max, log2_max, log10_max;

static Boolean FuncSUBSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  int cnt = pArgs[0].Contents.str.len - pArgs[1].Contents.Int;

  UNUSED(ArgCnt);
  if ((pArgs[2].Contents.Int != 0) && (pArgs[2].Contents.Int < cnt))
    cnt = pArgs[2].Contents.Int;
  if (cnt < 0)
    cnt = 0;
  as_tempres_set_c_str(pResult, "");
  as_nonz_dynstr_append_raw(&pResult->Contents.str, pArgs[0].Contents.str.p_str + pArgs[1].Contents.Int, cnt);

  return True;
}

static Boolean FuncSTRSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_int(pResult, as_nonz_dynstr_find(&pArgs[0].Contents.str, &pArgs[1].Contents.str));

  return True;
}

static Boolean FuncCHARFROMSTR(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_int(pResult, ((pArgs[1].Contents.Int >= 0) && ((unsigned)pArgs[1].Contents.Int < pArgs[0].Contents.str.len)) ? pArgs[0].Contents.str.p_str[pArgs[1].Contents.Int] : -1);

  return True;
}

static Boolean FuncCODEPAGE_VAL(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  PTransTable p_table;

  UNUSED(ArgCnt);

  if (ArgCnt >= 2)
  {
    String name;

    as_nonz_dynstr_to_c_str(name, &pArgs[1].Contents.str, sizeof(name));
    p_table = FindCodepage(name, NULL);
    if (!p_table)
    {
      WrXError(ErrNum_UnknownCodepage, name);
      as_tempres_set_int(pResult, 0);
      return True;
    }
  }
  else
    p_table = CurrTransTable;

  if (ChkRange(pArgs[0].Contents.Int, 0, 255))
    as_tempres_set_int(pResult, as_chartrans_xlate(p_table->p_table, pArgs[0].Contents.Int));
  else
    as_tempres_set_int(pResult, 0);
  return True;
}

static Boolean FuncEXPRTYPE(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
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

  return True;
}

/* in Grossbuchstaben wandeln */

static Boolean FuncUPSTRING(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  char *pRun;

  UNUSED(ArgCnt);

  as_tempres_set_str(pResult, &pArgs[0].Contents.str);
  for (pRun = pResult->Contents.str.p_str;
       pRun < pResult->Contents.str.p_str + pResult->Contents.str.len;
       pRun++)
    *pRun = as_toupper(*pRun);

  return True;
}

/* in Kleinbuchstaben wandeln */

static Boolean FuncLOWSTRING(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  char *pRun;

  UNUSED(ArgCnt);

  as_tempres_set_str(pResult, &pArgs[0].Contents.str);
  for (pRun = pResult->Contents.str.p_str;
       pRun < pResult->Contents.str.p_str + pResult->Contents.str.len;
       pRun++)
    *pRun = as_tolower(*pRun);

  return True;
}

/* Laenge ermitteln */

static Boolean FuncSTRLEN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_int(pResult, pArgs[0].Contents.str.len);

  return True;
}

/* Parser aufrufen */

static Boolean FuncVAL(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  String Tmp;

  UNUSED(ArgCnt);

  as_nonz_dynstr_to_c_str(Tmp, &pArgs[0].Contents.str, sizeof(Tmp));
  EvalExpression(Tmp, pResult);

  return True;
}

static Boolean FuncTOUPPER(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if ((pArgs[0].Contents.Int < 0) || (pArgs[0].Contents.Int > 255)) WrError(ErrNum_OverRange);
  else
    as_tempres_set_int(pResult, as_toupper(pArgs[0].Contents.Int));

  return True;
}

static Boolean FuncTOLOWER(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if ((pArgs[0].Contents.Int < 0) || (pArgs[0].Contents.Int > 255)) WrError(ErrNum_OverRange);
  else
    as_tempres_set_int(pResult, as_tolower(pArgs[0].Contents.Int));

  return True;
}

static Boolean FuncBITCNT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
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

  return True;
}

static Boolean FuncFIRSTBIT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
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

  return True;
}

static Boolean FuncLASTBIT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
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

  return True;
}

static Boolean FuncBITPOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
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

  return True;
}

static Boolean FuncABS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  switch (pArgs[0].Typ)
  {
    case TempInt:
      as_tempres_set_int(pResult, (pArgs[0].Contents.Int  < 0) ? -pArgs[0].Contents.Int : pArgs[0].Contents.Int);
      break;
    case TempFloat:
      as_tempres_set_float(pResult, as_fabs(pArgs[0].Contents.Float));
      break;
    default:
      pResult->Typ = TempNone;
  }

  return True;
}

static Boolean FuncSGN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
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

  return True;
}

static Boolean FuncINT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (as_fabs(pArgs[0].Contents.Float) > IntTypeDefs[LargeSIntType].Max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_OverRange);
  }
  else
    as_tempres_set_int(pResult, (LargeInt) floor(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncSQRT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float < 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_sqrt(pArgs[0].Contents.Float));

  return True;
}

/* trigonometrische Funktionen */

static Boolean FuncSIN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, as_sin(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncCOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, as_cos(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncTAN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (as_cos(pArgs[0].Contents.Float) == 0.0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_tan(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncCOT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  as_float_t FVal = as_sin(pArgs[0].Contents.Float);
  UNUSED(ArgCnt);

  if (FVal == 0.0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_cos(pArgs[0].Contents.Float) / FVal);

  return True;
}

/* inverse trigonometrische Funktionen */

static Boolean FuncASIN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (as_fabs(pArgs[0].Contents.Float) > 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_asin(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncACOS(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (as_fabs(pArgs[0].Contents.Float) > 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_acos(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncATAN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, as_atan(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncACOT(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, M_PI / 2 - as_atan(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncEXP(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_exp(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncALOG(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log10_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_exp(pArgs[0].Contents.Float * log(10.0)));

  return True;
}

static Boolean FuncALD(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log2_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_exp(pArgs[0].Contents.Float * log(2.0)));

  return True;
}

static Boolean FuncSINH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_sinh(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncCOSH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_cosh(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncTANH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_tanh(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncCOTH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  as_float_t FVal;

  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float > log_max)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else if ((FVal = as_tanh(pArgs[0].Contents.Float)) == 0.0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, pResult->Contents.Float = 1.0 / FVal);

  return True;
}

/* logarithmische & inverse hyperbolische Funktionen */

static Boolean FuncLN(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_log(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncLOG(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_log10(pArgs[0].Contents.Float));

  return True;
}

static Boolean FuncLD(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float <= 0)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_InvFuncArg);
  }
  else
    as_tempres_set_float(pResult, as_log(pArgs[0].Contents.Float) / as_log(2.0));

  return True;
}

static Boolean FuncASINH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  as_tempres_set_float(pResult, as_log(pArgs[0].Contents.Float + as_sqrt(pArgs[0].Contents.Float * pArgs[0].Contents.Float + 1)));

  return True;
}

static Boolean FuncACOSH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (pArgs[0].Contents.Float < 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, as_log(pArgs[0].Contents.Float + as_sqrt(pArgs[0].Contents.Float * pArgs[0].Contents.Float - 1)));

  return True;
}

static Boolean FuncATANH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (as_fabs(pArgs[0].Contents.Float) >= 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, 0.5 * as_log((1 + pArgs[0].Contents.Float) / (1 - pArgs[0].Contents.Float)));

  return True;
}

static Boolean FuncACOTH(TempResult *pResult, const TempResult *pArgs, unsigned ArgCnt)
{
  UNUSED(ArgCnt);

  if (as_fabs(pArgs[0].Contents.Float) <= 1)
  {
    as_tempres_set_none(pResult);
    WrError(ErrNum_FloatOverflow);
  }
  else
    as_tempres_set_float(pResult, 0.5 * as_log((pArgs[0].Contents.Float + 1) / (pArgs[0].Contents.Float - 1)));

  return True;
}

#define TempAll (TempInt | TempFloat | TempString)

static const tFunction Functions[] =
{
  { "SUBSTR"     , 3, 3, { TempString              , TempInt        , TempInt        }, FuncSUBSTR      },
  { "STRSTR"     , 2, 2, { TempString              , TempString     , 0              }, FuncSTRSTR      },
  { "CHARFROMSTR", 2, 2, { TempString              , TempInt        , 0              }, FuncCHARFROMSTR },
  { "CODEPAGE_VAL",1, 2, { TempInt                 , TempString     , 0              }, FuncCODEPAGE_VAL},
  { "EXPRTYPE"   , 1, 1, { TempAll                 , 0              , 0              }, FuncEXPRTYPE    },
  { "UPSTRING"   , 1, 1, { TempString              , 0              , 0              }, FuncUPSTRING    },
  { "LOWSTRING"  , 1, 1, { TempString              , 0              , 0              }, FuncLOWSTRING   },
  { "STRLEN"     , 1, 1, { TempString              , 0              , 0              }, FuncSTRLEN      },
  { "VAL"        , 1, 1, { TempString              , 0              , 0              }, FuncVAL         },
  { "TOUPPER"    , 1, 1, { TempInt                 , 0              , 0              }, FuncTOUPPER     },
  { "TOLOWER"    , 1, 1, { TempInt                 , 0              , 0              }, FuncTOLOWER     },
  { "BITCNT"     , 1, 1, { TempInt                 , 0              , 0              }, FuncBITCNT      },
  { "FIRSTBIT"   , 1, 1, { TempInt                 , 0              , 0              }, FuncFIRSTBIT    },
  { "LASTBIT"    , 1, 1, { TempInt                 , 0              , 0              }, FuncLASTBIT     },
  { "BITPOS"     , 1, 1, { TempInt                 , 0              , 0              }, FuncBITPOS      },
  { "ABS"        , 1, 1, { TempInt | TempFloat     , 0              , 0              }, FuncABS         },
  { "SGN"        , 1, 1, { TempInt | TempFloat     , 0              , 0              }, FuncSGN         },
  { "INT"        , 1, 1, { TempFloat               , 0              , 0              }, FuncINT         },
  { "SQRT"       , 1, 1, { TempFloat               , 0              , 0              }, FuncSQRT        },
  { "SIN"        , 1, 1, { TempFloat               , 0              , 0              }, FuncSIN         },
  { "COS"        , 1, 1, { TempFloat               , 0              , 0              }, FuncCOS         },
  { "TAN"        , 1, 1, { TempFloat               , 0              , 0              }, FuncTAN         },
  { "COT"        , 1, 1, { TempFloat               , 0              , 0              }, FuncCOT         },
  { "ASIN"       , 1, 1, { TempFloat               , 0              , 0              }, FuncASIN        },
  { "ACOS"       , 1, 1, { TempFloat               , 0              , 0              }, FuncACOS        },
  { "ATAN"       , 1, 1, { TempFloat               , 0              , 0              }, FuncATAN        },
  { "ACOT"       , 1, 1, { TempFloat               , 0              , 0              }, FuncACOT        },
  { "EXP"        , 1, 1, { TempFloat               , 0              , 0              }, FuncEXP         },
  { "ALOG"       , 1, 1, { TempFloat               , 0              , 0              }, FuncALOG        },
  { "ALD"        , 1, 1, { TempFloat               , 0              , 0              }, FuncALD         },
  { "SINH"       , 1, 1, { TempFloat               , 0              , 0              }, FuncSINH        },
  { "COSH"       , 1, 1, { TempFloat               , 0              , 0              }, FuncCOSH        },
  { "TANH"       , 1, 1, { TempFloat               , 0              , 0              }, FuncTANH        },
  { "COTH"       , 1, 1, { TempFloat               , 0              , 0              }, FuncCOTH        },
  { "LN"         , 1, 1, { TempFloat               , 0              , 0              }, FuncLN          },
  { "LOG"        , 1, 1, { TempFloat               , 0              , 0              }, FuncLOG         },
  { "LD"         , 1, 1, { TempFloat               , 0              , 0              }, FuncLD          },
  { "ASINH"      , 1, 1, { TempFloat               , 0              , 0              }, FuncASINH       },
  { "ACOSH"      , 1, 1, { TempFloat               , 0              , 0              }, FuncACOSH       },
  { "ATANH"      , 1, 1, { TempFloat               , 0              , 0              }, FuncATANH       },
  { "ACOTH"      , 1, 1, { TempFloat               , 0              , 0              }, FuncACOTH       },
  { "PHYS2CPU"   , 1, 1, { TempInt                 , 0              , 0              }, fnc_phys_2_cpu  },
  { "CPU2PHYS"   , 1, 1, { TempInt                 , 0              , 0              }, fnc_cpu_2_phys  },
  { NULL         , 0, 0, { 0                            , 0              , 0              }, NULL            }
};

/*!------------------------------------------------------------------------
 * \fn     tFunction *function_find(const char *p_name)
 * \brief  look up built-in function
 * \param  p_name function name
 * \return retrieved function or NULL if not found
 * ------------------------------------------------------------------------ */

const tFunction *function_find(const char *p_name)
{
  const tFunction *p_run;

  for (p_run = Functions; p_run->pName; p_run++)
    if (!strcmp(p_name, p_run->pName))
      return p_run;
  return NULL;
}

/*!------------------------------------------------------------------------
 * \fn     function_init(void)
 * \brief  module setup
 * ------------------------------------------------------------------------ */

void function_init(void)
{
  log_max = as_log(AS_FLOAT_MAX);
  log2_max = log_max / log(2.0);
  log10_max = as_log10(AS_FLOAT_MAX);
}
