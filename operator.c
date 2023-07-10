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
     they are for sure unequal, but no ordering critera can be given: */

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

static void DummyOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);
  UNUSED(pRVal);
  UNUSED(pErg);
}

static void OneComplOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);
  as_tempres_set_int(pErg, ~(pRVal->Contents.Int));
  PromoteLValFlags();
}

static void ShLeftOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, pLVal->Contents.Int << pRVal->Contents.Int);
  PromoteLRValFlags();
}

static void ShRightOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, pLVal->Contents.Int >> pRVal->Contents.Int);
  PromoteLRValFlags();
}

static void BitMirrorOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  int z;

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
}

static void BinAndOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, pLVal->Contents.Int & pRVal->Contents.Int);
  PromoteLRValFlags();
}

static void BinOrOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, pLVal->Contents.Int | pRVal->Contents.Int);
  PromoteLRValFlags();
}

static void BinXorOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, pLVal->Contents.Int ^ pRVal->Contents.Int);
  PromoteLRValFlags();
}

static void PotOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  LargeInt HVal;

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
      else if ((fabs(pRVal->Contents.Float) <= ((double)MaxLongInt)) && (floor(pRVal->Contents.Float) == pRVal->Contents.Float))
      {
        Double Base = pLVal->Contents.Float, Result;

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
}

static void MultOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void DivOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void ModOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  if (pRVal->Contents.Int == 0) WrError(ErrNum_DivByZero);
  else
    as_tempres_set_int(pErg, pLVal->Contents.Int % pRVal->Contents.Int);
  PromoteLRValFlags();
}

/* TODO: handle return code of NonZString2Int() better */

static void AddOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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

          if (!NonZString2Int(&pRVal->Contents.str, &RIntVal))
          {
            as_tempres_set_c_str(pErg, "");
            Int2NonZString(&pErg->Contents.str, RIntVal + pLVal->Contents.Int);
          }
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

          if (!NonZString2Int(&pLVal->Contents.str, &LIntVal))
          {
            as_tempres_set_c_str(pErg, "");
            Int2NonZString(&pErg->Contents.str, LIntVal + pRVal->Contents.Int);
          }
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
}

static void SubOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void LogNotOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);
  as_tempres_set_int(pErg, pRVal->Contents.Int ? 0 : 1);
  PromoteLValFlags();
}

static void LogAndOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, (pLVal->Contents.Int && pRVal->Contents.Int) ? 1 : 0);
  PromoteLRValFlags();
}

static void LogOrOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, (pLVal->Contents.Int || pRVal->Contents.Int) ? 1 : 0);
  PromoteLRValFlags();
}

static void LogXorOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  as_tempres_set_int(pErg, ((pLVal->Contents.Int != 0) != (pRVal->Contents.Int != 0)) ? 1 : 0);
  PromoteLRValFlags();
}

static void EqOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void GtOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void LtOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void LeOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void GeOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

static void UneqOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
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
}

#define Int2Int       (TempInt    | (TempInt << 4)   )
#define Float2Float   (TempFloat  | (TempFloat << 4) )
#define String2String (TempString | (TempString << 4))
#define Reg2Reg       (TempReg    | (TempReg << 4)   )
#define Int2String    (TempInt    | (TempString << 4))
#define String2Int    (TempString | (TempInt << 4)   )

const Operator Operators[] =
{
  {" " , 1 , False,  0, { 0, 0, 0, 0, 0 }, DummyOp},
  {"~" , 1 , False,  1, { TempInt << 4, 0, 0, 0, 0 }, OneComplOp},
  {"<<", 2 , True ,  3, { Int2Int, 0, 0, 0, 0 }, ShLeftOp},
  {">>", 2 , True ,  3, { Int2Int, 0, 0, 0, 0 }, ShRightOp},
  {"><", 2 , True ,  4, { Int2Int, 0, 0, 0, 0 }, BitMirrorOp},
  {"&" , 1 , True ,  5, { Int2Int, 0, 0, 0, 0 }, BinAndOp},
  {"|" , 1 , True ,  6, { Int2Int, 0, 0, 0, 0 }, BinOrOp},
  {"!" , 1 , True ,  7, { Int2Int, 0, 0, 0, 0 }, BinXorOp},
  {"^" , 1 , True ,  8, { Int2Int, Float2Float, 0, 0, 0 }, PotOp},
  {"*" , 1 , True , 11, { Int2Int, Float2Float, 0, 0, 0 }, MultOp},
  {"/" , 1 , True , 11, { Int2Int, Float2Float, 0, 0, 0 }, DivOp},
  {"#" , 1 , True , 11, { Int2Int, 0, 0, 0, 0 }, ModOp},
  {"+" , 1 , True , 13, { Int2Int, Float2Float, String2String, Int2String, String2Int }, AddOp},
  {"-" , 1 , True , 13, { Int2Int, Float2Float, 0, 0, 0 }, SubOp},
  {"~~", 2 , False,  2, { TempInt << 4, 0, 0, 0, 0 }, LogNotOp},
  {"&&", 2 , True , 15, { Int2Int, 0, 0, 0, 0 }, LogAndOp},
  {"||", 2 , True , 16, { Int2Int, 0, 0, 0, 0 }, LogOrOp},
  {"!!", 2 , True , 17, { Int2Int, 0, 0, 0, 0 }, LogXorOp},
  {"=" , 1 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, EqOp},
  {"==", 2 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, EqOp},
  {">" , 1 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, GtOp},
  {"<" , 1 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, LtOp},
  {"<=", 2 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, LeOp},
  {">=", 2 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, GeOp},
  {"<>", 2 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, UneqOp},
  {"!=", 2 , True , 23, { Int2Int, Float2Float, String2String, Reg2Reg, 0 }, UneqOp},
  /* termination marker */
  {NULL, 0 , False,  0, { 0, 0, 0, 0, 0 }, NULL}
},
/* minus may have one or two operands */
MinusMonadicOperator =
{
  "-" ,1 , False, 13, { TempInt << 4, TempFloat << 4, 0, 0, 0 }, SubOp
};

const Operator *pPotMonadicOperator = NULL;
