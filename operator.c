/* operator.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* defintion of operators                                                    */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <math.h>

#include "stdinc.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmerr.h"
#include "asmpars.h"
#include "asmrelocs.h"
#include "operator.h"

#define PromoteLValFlags() \
        do \
        { \
          if (pErg->Typ != TempNone) \
          { \
            pErg->Flags |= (pLVal->Flags & eSymbolFlags_Promotable); \
            pErg->AddrSpaceMask |= pLVal->AddrSpaceMask; \
            if (pErg->DataSize == eSymbolSizeUnknown) pErg->DataSize = pLVal->DataSize; \
          } \
        } \
        while (0)

#define PromoteRValFlags() \
        do \
        { \
          if (pErg->Typ != TempNone) \
          { \
            pErg->Flags |= (pRVal->Flags & eSymbolFlags_Promotable); \
            pErg->AddrSpaceMask |= pRVal->AddrSpaceMask; \
            if (pErg->DataSize == eSymbolSizeUnknown) pErg->DataSize = pRVal->DataSize; \
          } \
        } \
        while (0)

#define PromoteLRValFlags() \
        do \
        { \
          if (pErg->Typ != TempNone) \
          { \
            pErg->Flags |= ((pLVal->Flags | pRVal->Flags) & eSymbolFlags_Promotable); \
            pErg->AddrSpaceMask |= pLVal->AddrSpaceMask | pRVal->AddrSpaceMask; \
            if (pErg->DataSize == eSymbolSizeUnknown) pErg->DataSize = pLVal->DataSize; \
            if (pErg->DataSize == eSymbolSizeUnknown) pErg->DataSize = pRVal->DataSize; \
          } \
        } \
        while (0)

/*!------------------------------------------------------------------------
 * \fn     reg_cmp(const tRegDescr *p_reg1, tSymbolSize data_size1,
                   const tRegDescr *p_reg2, tSymbolSize data_size2)
 * \brief  compare two register symbols
 * \param  p_reg1, p_reg2 registers to compare
 * \return -1 : reg1 < reg2
 *          0 : reg1 = reg2
 *         +1 : reg1 > reg2
 *         -2 : unequal, but no smaller/greater relation can be given
 * ------------------------------------------------------------------------ */

static int reg_cmp(const TempResult *p_val1, const TempResult *p_val2)
{
  tRegInt num1, num2;

  /* If the two symbols are for different target architectures,
     they are for sure unequal, but no ordering criteria can be given: */

  if ((p_val1->Contents.RegDescr.Dissect != p_val2->Contents.RegDescr.Dissect)
   || (p_val1->Contents.RegDescr.compare != p_val2->Contents.RegDescr.compare))
    return -2;

  /* architecture-specific comparison function? */

  if (p_val1->Contents.RegDescr.compare)
    return p_val1->Contents.RegDescr.compare(p_val1->Contents.RegDescr.Reg, p_val1->DataSize,
                                             p_val2->Contents.RegDescr.Reg, p_val2->DataSize);

  /* The generic comparison: If operand sizes differ, they are 'just unequal',
     otherwise compare register numbers: */

  if (p_val1->DataSize != p_val2->DataSize)
    return -2;
  num1 = p_val1->Contents.RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  num2 = p_val2->Contents.RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  if (num1 < num2)
    return -1;
  else if (num1 > num2)
    return 1;
  else
    return 0;
}

static Boolean OneComplOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);
  if (!pRVal)
    return False;

  as_tempres_set_int(pErg, ~(pRVal->Contents.Int));
  PromoteLValFlags();
  return True;
}

static Boolean ShLeftOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_int(pErg, pLVal->Contents.Int << pRVal->Contents.Int);
  PromoteLRValFlags();
  return True;
}

static Boolean ShRightOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_int(pErg, pLVal->Contents.Int >> pRVal->Contents.Int);
  PromoteLRValFlags();
  return True;
}

static Boolean BitMirrorOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  int z;

  if (!pLVal || !pRVal)
    return False;

  if ((pRVal->Contents.Int < 1) || (pRVal->Contents.Int > 32)) WrError(ErrNum_OverRange);
  else
  {
    LargeInt Result = (pLVal->Contents.Int >> pRVal->Contents.Int) << pRVal->Contents.Int;

    for (z = 0; z < pRVal->Contents.Int; z++)
    {
      if ((pLVal->Contents.Int & (1 << (pRVal->Contents.Int - 1 - z))) != 0)
        Result |= (1 << z);
    }
    as_tempres_set_int(pErg, Result);
  }
  PromoteLRValFlags();
  return True;
}

static Boolean BinAndOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_int(pErg, pLVal->Contents.Int & pRVal->Contents.Int);
  PromoteLRValFlags();
  return True;
}

static Boolean BinOrOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_int(pErg, pLVal->Contents.Int | pRVal->Contents.Int);
  PromoteLRValFlags();
  return True;
}

static Boolean BinXorOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_int(pErg, pLVal->Contents.Int ^ pRVal->Contents.Int);
  PromoteLRValFlags();
  return True;
}

static Boolean PotOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  LargeInt HVal;

  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      if (pRVal->Contents.Int < 0) as_tempres_set_int(pErg, 0);
      else
      {
        LargeInt l = pLVal->Contents.Int, r = pRVal->Contents.Int;

        HVal = 1;
        while (r > 0)
        {
          if (r & 1)
            HVal *= l;
          r >>= 1;
          if (r)
            l *= l;
        }
        as_tempres_set_int(pErg, HVal);
      }
      break;
    case TempFloat:
      if (pRVal->Contents.Float == 0.0)
        as_tempres_set_float(pErg, 1.0);
      else if (pLVal->Contents.Float == 0.0)
        as_tempres_set_float(pErg, 0.0);
      else if (pLVal->Contents.Float > 0)
        as_tempres_set_float(pErg, pow(pLVal->Contents.Float, pRVal->Contents.Float));
      else if ((as_fabs(pRVal->Contents.Float) <= ((as_float_t)MaxLongInt)) && (floor(pRVal->Contents.Float) == pRVal->Contents.Float))
      {
        as_float_t Base = pLVal->Contents.Float, Result;

        HVal = (LongInt) floor(pRVal->Contents.Float + 0.5);
        if (HVal < 0)
        {
          Base = 1 / Base;
          HVal = -HVal;
        }
        Result = 1.0;
        while (HVal > 0)
        {
          if (HVal & 1)
            Result *= Base;
          Base *= Base;
          HVal >>= 1;
        }
        as_tempres_set_float(pErg, Result);
      }
      else
      {
        WrError(ErrNum_InvArgPair);
        pErg->Typ = TempNone;
      }
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean MultOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, pLVal->Contents.Int * pRVal->Contents.Int);
      break;
    case TempFloat:
      as_tempres_set_float(pErg, pLVal->Contents.Float * pRVal->Contents.Float);
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean DivOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      if (pRVal->Contents.Int == 0) WrError(ErrNum_DivByZero);
      else
        as_tempres_set_int(pErg, pLVal->Contents.Int / pRVal->Contents.Int);
      break;
    case TempFloat:
      if (pRVal->Contents.Float == 0.0) WrError(ErrNum_DivByZero);
      else
        as_tempres_set_float(pErg, pLVal->Contents.Float / pRVal->Contents.Float);
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean ModOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  if (pRVal->Contents.Int == 0) WrError(ErrNum_DivByZero);
  else
    as_tempres_set_int(pErg, pLVal->Contents.Int % pRVal->Contents.Int);
  PromoteLRValFlags();
  return True;
}

/* TODO: handle return code of NonZString2Int() better */

static Boolean AddOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_none(pErg);
  switch (pLVal->Typ)
  {
    case TempInt:
      switch (pRVal->Typ)
      {
        case TempInt:
          as_tempres_set_int(pErg, pLVal->Contents.Int + pRVal->Contents.Int);
          pErg->Relocs = MergeRelocs(&(pLVal->Relocs), &(pRVal->Relocs), TRUE);
          break;
        case TempString:
        {
          LargeInt RIntVal;
          tErrorNum error_num = NonZString2Int(&pRVal->Contents.str, &RIntVal);

          RIntVal += pLVal->Contents.Int;
          if (ErrNum_None == error_num)
          {
            if (RIntVal >= 0)
            {
              as_tempres_set_c_str(pErg, "");
              Int2NonZString(&pErg->Contents.str, RIntVal);
            }
            else
              as_tempres_set_int(pErg, RIntVal);
          }
          else
            WrError(error_num);
          break;
        }
        default:
          break;
      }
      break;
    case TempFloat:
      if (TempFloat == pRVal->Typ)
        as_tempres_set_float(pErg, pLVal->Contents.Float + pRVal->Contents.Float);
      break;
    case TempString:
    {
      switch (pRVal->Typ)
      {
        case TempString:
          as_tempres_set_str(pErg, &pLVal->Contents.str);
          as_nonz_dynstr_append(&pErg->Contents.str, &pRVal->Contents.str);
          break;
        case TempInt:
        {
          LargeInt LIntVal;
          tErrorNum error_num = NonZString2Int(&pLVal->Contents.str, &LIntVal);

          if (ErrNum_None == error_num)
          {
            LIntVal += pRVal->Contents.Int;
            if (LIntVal >= 0)
            {
              as_tempres_set_c_str(pErg, "");
              Int2NonZString(&pErg->Contents.str, LIntVal);
            }
            else
              as_tempres_set_int(pErg, LIntVal);
          }
          else
            WrError(error_num);
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean SubOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, pLVal->Contents.Int - pRVal->Contents.Int);
      break;
    case TempFloat:
      as_tempres_set_float(pErg, pLVal->Contents.Float - pRVal->Contents.Float);
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean SubSglOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);
  if (!pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, -pRVal->Contents.Int);
      break;
    case TempFloat:
      as_tempres_set_float(pErg, -pRVal->Contents.Float);
      break;
    default:
      break;
  }
  PromoteRValFlags();
  return True;
}

static Boolean LogNotOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);
  if (!pRVal)
    return False;

  as_tempres_set_int(pErg, pRVal->Contents.Int ? 0 : 1);
  PromoteLValFlags();
  return True;
}

static Boolean LogAndOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal)
    return False;

  /* short circuit evaluation: 0 && ... -> 0 */

  if (!pRVal)
  {
    if (!pLVal->Contents.Int)
    {
      as_tempres_set_int(pErg, 0);
      PromoteLValFlags();
      return True;
    }
    return False;
  }

  as_tempres_set_int(pErg, (pLVal->Contents.Int && pRVal->Contents.Int) ? 1 : 0);
  PromoteLRValFlags();
  return True;
}

static Boolean LogOrOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal)
    return False;

  /* short circuit evaluation: 1 ||  ... -> 1 */

  if (!pRVal)
  {
    if (pLVal->Contents.Int)
    {
      as_tempres_set_int(pErg, 1);
      PromoteLValFlags();
      return True;
    }
    return False;
  }

  as_tempres_set_int(pErg, (pLVal->Contents.Int || pRVal->Contents.Int) ? 1 : 0);
  PromoteLRValFlags();
  return True;
}

static Boolean LogXorOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  as_tempres_set_int(pErg, ((pLVal->Contents.Int != 0) != (pRVal->Contents.Int != 0)) ? 1 : 0);
  PromoteLRValFlags();
  return True;
}

static Boolean EqOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, (pLVal->Contents.Int == pRVal->Contents.Int) ? 1 : 0);
      break;
    case TempFloat:
      as_tempres_set_int(pErg, (pLVal->Contents.Float == pRVal->Contents.Float) ? 1 : 0);
      break;
    case TempString:
      as_tempres_set_int(pErg, (as_nonz_dynstr_cmp(&pLVal->Contents.str, &pRVal->Contents.str) == 0) ? 1 : 0);
      break;
    case TempReg:
      as_tempres_set_int(pErg, 0 == reg_cmp(pLVal, pRVal));
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean GtOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, (pLVal->Contents.Int > pRVal->Contents.Int) ? 1 : 0);
      break;
    case TempFloat:
      as_tempres_set_int(pErg, (pLVal->Contents.Float > pRVal->Contents.Float) ? 1 : 0);
      break;
    case TempString:
      as_tempres_set_int(pErg, (as_nonz_dynstr_cmp(&pLVal->Contents.str, &pRVal->Contents.str) > 0) ? 1 : 0);
      break;
    case TempReg:
      as_tempres_set_int(pErg, reg_cmp(pLVal, pRVal) == 1);
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean LtOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, (pLVal->Contents.Int < pRVal->Contents.Int) ? 1 : 0);
      break;
    case TempFloat:
      as_tempres_set_int(pErg, (pLVal->Contents.Float < pRVal->Contents.Float) ? 1 : 0);
      break;
    case TempString:
      as_tempres_set_int(pErg, (as_nonz_dynstr_cmp(&pLVal->Contents.str, &pRVal->Contents.str) < 0) ? 1 : 0);
      break;
    case TempReg:
      as_tempres_set_int(pErg, reg_cmp(pLVal, pRVal) == -1);
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean LeOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, (pLVal->Contents.Int <= pRVal->Contents.Int) ? 1 : 0);
      break;
    case TempFloat:
      as_tempres_set_int(pErg, (pLVal->Contents.Float <= pRVal->Contents.Float) ? 1 : 0);
      break;
    case TempString:
      as_tempres_set_int(pErg, (as_nonz_dynstr_cmp(&pLVal->Contents.str, &pRVal->Contents.str) <= 0) ? 1 : 0);
      break;
    case TempReg:
    {
      int cmp_res = reg_cmp(pLVal, pRVal);
      as_tempres_set_int(pErg, (cmp_res == -1) || (cmp_res == 0));
      break;
    }
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean GeOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, (pLVal->Contents.Int >= pRVal->Contents.Int) ? 1 : 0);
      break;
    case TempFloat:
      as_tempres_set_int(pErg, (pLVal->Contents.Float >= pRVal->Contents.Float) ? 1 : 0);
      break;
    case TempString:
      as_tempres_set_int(pErg, (as_nonz_dynstr_cmp(&pLVal->Contents.str, &pRVal->Contents.str) >= 0) ? 1 : 0);
      break;
    case TempReg:
    {
      int cmp_res = reg_cmp(pLVal, pRVal);
      as_tempres_set_int(pErg, (cmp_res == 1) || (cmp_res == 0));
      break;
    }
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

static Boolean UneqOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (!pLVal || !pRVal)
    return False;

  switch (pLVal->Typ)
  {
    case TempInt:
      as_tempres_set_int(pErg, (pLVal->Contents.Int != pRVal->Contents.Int) ? 1 : 0);
      break;
    case TempFloat:
      as_tempres_set_int(pErg, (pLVal->Contents.Float != pRVal->Contents.Float) ? 1 : 0);
      break;
    case TempString:
      as_tempres_set_int(pErg, (as_nonz_dynstr_cmp(&pLVal->Contents.str, &pRVal->Contents.str) != 0) ? 1 : 0);
      break;
    case TempReg:
      as_tempres_set_int(pErg, reg_cmp(pLVal, pRVal) != 0);
      break;
    default:
      break;
  }
  PromoteLRValFlags();
  return True;
}

#define Int2Int       (TempInt    | (TempInt << 4)   )
#define Float2Float   (TempFloat  | (TempFloat << 4) )
#define String2String (TempString | (TempString << 4))
#define Reg2Reg       (TempReg    | (TempReg << 4)   )
#define Int2String    (TempInt    | (TempString << 4))
#define String2Int    (TempString | (TempInt << 4)   )

const as_operator_t operators[] =
{
  {"~" , 1 , e_op_monadic,  1, { TempInt << 4, 0, 0, 0, 0 }, OneComplOp},
  {"<<", 2 , e_op_dyadic ,  3, { Int2Int, 0, 0, 0, 0 }, ShLeftOp},
  {">>", 2 , e_op_dyadic ,  3, { Int2Int, 0, 0, 0, 0 }, ShRightOp},
  {"><", 2 , e_op_dyadic ,  4, { Int2Int, 0, 0, 0, 0 }, BitMirrorOp},
  {"&" , 1 , e_op_dyadic ,  5, { Int2Int, 0, 0, 0, 0 }, BinAndOp},
  {"|" , 1 , e_op_dyadic ,  6, { Int2Int, 0, 0, 0, 0 }, BinOrOp},
  {"!" , 1 , e_op_dyadic ,  7, { Int2Int, 0, 0, 0, 0 }, BinXorOp},
  {"^" , 1 , e_op_dyadic ,  8, { Int2Int, Float2Float, 0, 0, 0 }, PotOp},
  {"*" , 1 , e_op_dyadic , 11, { Int2Int, Float2Float, 0, 0, 0 }, MultOp},
  {"/" , 1 , e_op_dyadic , 11, { Int2Int, Float2Float, 0, 0, 0 }, DivOp},
  {"#" , 1 , e_op_dyadic , 11, { Int2Int, 0, 0, 0, 0 }, ModOp},
  {"+" , 1 , e_op_dyadic , 13, { Int2Int, Float2Float, String2String, Int2String, String2Int }, AddOp},
  /* minus may have one or two operands */
  {"-" , 1 , e_op_dyadic , 13, { Int2Int, Float2Float, 0, 0, 0 }, SubOp},
  {"-" , 1 , e_op_monadic, 13, { TempInt << 4, TempFloat << 4, 0, 0, 0 }, SubSglOp},
  {"~~", 2 , e_op_monadic,  2, { TempInt << 4, 0, 0, 0, 0 }, LogNotOp},
  {"&&", 2 , e_op_dyadic_short , 15, { Int2Int, 0, 0, 0, 0 }, LogAndOp},
  {"||", 2 , e_op_dyadic_short , 16, { Int2Int, 0, 0, 0, 0 }, LogOrOp},
  {"!!", 2 , e_op_dyadic , 17, { Int2Int, 0, 0, 0, 0 }, LogXorOp},
  {"=" , 1 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, EqOp},
  {"==", 2 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, EqOp},
  {">" , 1 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, GtOp},
  {"<" , 1 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, LtOp},
  {"<=", 2 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, LeOp},
  {">=", 2 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, GeOp},
  {"<>", 2 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, UneqOp},
  {"!=", 2 , e_op_dyadic , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, UneqOp},
  /* termination marker */
  {NULL, 0 , e_op_monadic,  0, { 0, 0, 0, 0, 0 }, NULL}
};

const as_operator_t no_operators[] =
{
  {NULL, 0 , e_op_monadic,  0, { 0, 0, 0, 0, 0 }, NULL}
};

const as_operator_t *target_operators = no_operators;
