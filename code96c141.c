/* code96c141.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TLCS-900(L)                                                 */
/*                                                                           */
/* Historie: 27. 6.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "bpemu.h"
#include "strutil.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "errmsg.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "asmitree.h"
#include "asmcode.h"
#include "codevars.h"
#include "errmsg.h"
#include "operator.h"

#include "code96c141.h"

/*-------------------------------------------------------------------------*/
/* Daten */

typedef struct
{
  Word Code;
  Byte CPUFlag;
  Boolean InSup;
} FixedOrder;

typedef struct
{
  Word Code;
  Boolean InSup;
  Byte MinMax,MaxMax;
  ShortInt Default;
} ImmOrder;

typedef struct
{
  Word Code;
  Byte OpMask;
} RegOrder;

typedef struct
{
  const char *Name;
  Byte Code;
} Condition;

#define ModNone (-1)
#define ModReg 0
#define MModReg (1  << ModReg)
#define ModXReg 1
#define MModXReg (1 << ModXReg)
#define ModMem 2
#define MModMem (1  << ModMem)
#define ModImm 3
#define MModImm (1  << ModImm)
#define ModCReg 4
#define MModCReg (1 << ModCReg)

#define COND_CODE_TRUE 8

static FixedOrder *FixedOrders;
static RegOrder *RegOrders;
static ImmOrder *ImmOrders;
static Condition *Conditions;

static ShortInt AdrType;
static ShortInt OpSize;        /* -1/0/1/2 = nix/Byte/Word/Long */
static Byte AdrMode;
static Byte AdrVals[10];
static Boolean MinOneIs0, AutoIncSizeNeeded;

static CPUVar CPU96C141,CPU93C141;

/*---------------------------------------------------------------------------*/
/* Adressparser */

static Boolean IsRegBase(Byte No, tSymbolSize Size)
{
  return ((Size == eSymbolSize32Bit)
       || ((Size == eSymbolSize16Bit) && (No < 0xf0) && (!MaxMode) && ((No & 3) == 0)));
}

static void ChkMaxMode(Boolean MustMax, tRegEvalResult *Result)
{
  if (MaxMode != MustMax)
  {
    *Result = eRegAbort;
    WrError((MustMax) ? ErrNum_OnlyInMaxmode : ErrNum_NotInMaxmode);
  }
}

static Boolean IsQuot(char Ch)
{
  return ((Ch == '\'') || (Ch == '`'));
}

static tRegEvalResult CodeEReg(char *Asc, Byte *ErgNo, tSymbolSize *ErgSize)
{
#define RegCnt 8
  static const char Reg8Names[RegCnt + 1] = "AWCBEDLH";
  static const char Reg16Names[RegCnt][3] =
  {
    "WA" ,"BC" ,"DE" ,"HL" ,"IX" ,"IY" ,"IZ" ,"SP"
  };
  static const char Reg32Names[RegCnt][4] =
  {
    "XWA","XBC","XDE","XHL","XIX","XIY","XIZ","XSP"
  };

  int z, l = strlen(Asc);
  const char *pos;
  String HAsc, Asc_N;
  tRegEvalResult Result;

  strmaxcpy(Asc_N, Asc, STRINGSIZE);
  NLS_UpString(Asc_N);
  Asc = Asc_N;

  Result = eIsReg;

  /* mom. Bank ? */

  if (l == 1)
  {
    pos = strchr(Reg8Names, *Asc);
    if (pos)
    {
     z = pos - Reg8Names;
     *ErgNo = 0xe0 + ((z & 6) << 1) + (z & 1);
     *ErgSize = eSymbolSize8Bit;
     return Result;
    }
  }
  for (z = 0; z < RegCnt; z++)
  {
    if (!strcmp(Asc, Reg16Names[z]))
    {
      *ErgNo = 0xe0 + (z << 2);
      *ErgSize = eSymbolSize16Bit;
      return Result;
    }
    if (!strcmp(Asc, Reg32Names[z]))
    {
      *ErgNo = 0xe0 + (z << 2);
      *ErgSize = eSymbolSize32Bit;
      if (z < 4)
        ChkMaxMode(True, &Result);
      return Result;
    }
  }

  /* Bankregister, 8 Bit ? */

  if ((l == 3) && ((*Asc == 'Q') || (*Asc == 'R')) && ((Asc[2] >= '0') && (Asc[2] <= '7')))
  {
    for (z = 0; z < RegCnt; z++)
      if (Asc[1] == Reg8Names[z])
      {
        *ErgNo = ((Asc[2] - '0') << 4) + ((z & 6) << 1) + (z & 1);
        if (*Asc == 'Q')
        {
          *ErgNo |= 2;
          ChkMaxMode(True, &Result);
        }
        if (((*Asc == 'Q') || (MaxMode)) && (Asc[2] > '3'))
        {
          WrError(ErrNum_OverRange);
          Result = eRegAbort;
        }
        *ErgSize = eSymbolSize8Bit;
        return Result;
      }
  }

  /* Bankregister, 16 Bit ? */

  if ((l == 4) && ((*Asc == 'Q') || (*Asc == 'R')) && ((Asc[3] >= '0') && (Asc[3] <= '7')))
  {
    strcpy(HAsc, Asc + 1);
    HAsc[2] = '\0';
    for (z = 0; z < RegCnt >> 1; z++)
      if (!strcmp(HAsc, Reg16Names[z]))
      {
        *ErgNo = ((Asc[3] - '0') << 4) + (z << 2);
        if (*Asc == 'Q')
        {
          *ErgNo |= 2;
          ChkMaxMode(True, &Result);
        }
        if (((*Asc == 'Q') || (MaxMode)) && (Asc[3] > '3'))
        {
          WrError(ErrNum_OverRange);
          Result = eRegAbort;
        }
        *ErgSize = eSymbolSize16Bit;
        return Result;
      }
  }

  /* Bankregister, 32 Bit ? */

  if ((l == 4) && ((Asc[3] >= '0') && (Asc[3] <= '7')))
  {
    for (z = 0; z < RegCnt >> 1; z++)
     if (strncmp(Asc, Reg32Names[z], 3) == 0)
     {
       *ErgNo = ((Asc[3] - '0') << 4) + (z << 2);
       ChkMaxMode(True, &Result);
       if (Asc[3] > '3')
       {
         WrError(ErrNum_OverRange);
         Result = eRegAbort;
       }
       *ErgSize = eSymbolSize32Bit;
       return Result;
     }
  }

  /* obere 8-Bit-Haelften momentaner Bank ? */

  if ((l == 2) && (*Asc == 'Q'))
   for (z = 0; z < RegCnt; z++)
    if (Asc[1] == Reg8Names[z])
    {
      *ErgNo = 0xe2 + ((z & 6) << 1) + (z & 1);
      ChkMaxMode(True, &Result);
      *ErgSize = eSymbolSize8Bit;
      return Result;
    }

  /* obere 16-Bit-Haelften momentaner Bank und von XIX..XSP ? */

  if ((l == 3) && (*Asc == 'Q'))
  {
    for (z = 0; z < RegCnt; z++)
      if (!strcmp(Asc + 1, Reg16Names[z]))
      {
        *ErgNo = 0xe2 + (z << 2);
        if (z < 4) ChkMaxMode(True, &Result);
        *ErgSize = eSymbolSize16Bit;
        return Result;
      }
  }

  /* 8-Bit-Teile von XIX..XSP ? */

  if (((l == 3) || ((l == 4) && (*Asc == 'Q')))
  && ((Asc[l - 1] == 'L') || (Asc[l - 1] == 'H')))
  {
    strcpy(HAsc, Asc + (l - 3)); HAsc[2] = '\0';
    for (z = 0; z < RegCnt >> 1; z++)
      if (!strcmp(HAsc, Reg16Names[z + 4]))
      {
        *ErgNo = 0xf0 + (z << 2) + ((l - 3) << 1) + (Ord(Asc[l - 1] == 'H'));
        *ErgSize = eSymbolSize8Bit;
        return Result;
      }
  }

  /* 8-Bit-Teile vorheriger Bank ? */

  if (((l == 2) || ((l == 3) && (*Asc == 'Q'))) && (IsQuot(Asc[l - 1])))
   for (z = 0; z < RegCnt; z++)
     if (Asc[l - 2] == Reg8Names[z])
     {
       *ErgNo = 0xd0 + ((z & 6) << 1) + ((strlen(Asc) - 2) << 1) + (z & 1);
       if (l == 3) ChkMaxMode(True, &Result);
       *ErgSize = eSymbolSize8Bit;
       return Result;
     }

  /* 16-Bit-Teile vorheriger Bank ? */

  if (((l == 3) || ((l == 4) && (*Asc == 'Q'))) && (IsQuot(Asc[l - 1])))
  {
    strcpy(HAsc, Asc + 1);
    HAsc[l - 2] = '\0';
    for (z = 0; z < RegCnt >> 1; z++)
      if (!strcmp(HAsc, Reg16Names[z]))
      {
        *ErgNo = 0xd0 + (z << 2) + ((strlen(Asc) - 3) << 1);
        if (l == 4) ChkMaxMode(True, &Result);
        *ErgSize = eSymbolSize16Bit;
        return Result;
      }
  }

  /* 32-Bit-Register vorheriger Bank ? */

  if ((l == 4) && (IsQuot(Asc[3])))
  {
    strcpy(HAsc, Asc); HAsc[3] = '\0';
    for (z = 0; z < RegCnt >> 1; z++)
      if (!strcmp(HAsc, Reg32Names[z]))
      {
        *ErgNo = 0xd0 + (z << 2);
        ChkMaxMode(True, &Result);
        *ErgSize = eSymbolSize32Bit;
        return Result;
      }
  }

  return (Result = eIsNoReg);
}

static void ChkL(CPUVar Must, tRegEvalResult *Result)
{
  if (MomCPU != Must)
  {
    WrError(ErrNum_InvCtrlReg);
    *Result = eRegAbort;
  }
}

static tRegEvalResult CodeCReg(char *Asc, Byte *ErgNo, tSymbolSize *ErgSize)
{
  tRegEvalResult Result = eIsReg;

  if (!as_strcasecmp(Asc, "NSP"))
  {
    *ErgNo = 0x3c;
    *ErgSize = eSymbolSize16Bit;
    ChkL(CPU96C141, &Result);
    return Result;
  }
  if (!as_strcasecmp(Asc, "XNSP"))
  {
    *ErgNo = 0x3c;
    *ErgSize = eSymbolSize32Bit;
    ChkL(CPU96C141, &Result);
    return Result;
  }
  if (!as_strcasecmp(Asc,"INTNEST"))
  {
    *ErgNo = 0x3c;
    *ErgSize = eSymbolSize16Bit;
    ChkL(CPU93C141, &Result);
    return Result;
  }
  if ((strlen(Asc) == 5)
   && (!as_strncasecmp(Asc, "DMA", 3))
   && (Asc[4] >= '0') && (Asc[4] <= '3'))
  {
    switch (as_toupper(Asc[3]))
    {
      case 'S':
        *ErgNo = (Asc[4] - '0') * 4;
        *ErgSize = eSymbolSize32Bit;
        return Result;
      case 'D':
        *ErgNo = (Asc[4] - '0') * 4 + 0x10;
        *ErgSize = eSymbolSize32Bit;
        return Result;
      case 'M':
        *ErgNo = (Asc[4] - '0') * 4 + 0x22;
        *ErgSize = eSymbolSize8Bit;
        return Result;
      case 'C':
        *ErgNo = (Asc[4] - '0') * 4 + 0x20;
        *ErgSize = eSymbolSize16Bit;
        return Result;
    }
  }

  return (Result = eIsNoReg);
}


typedef struct
{
  char *Name;
  Byte Num;
  Boolean InMax, InMin;
} RegDesc;


static void SetOpSize(ShortInt NewSize)
{
  if (OpSize == -1)
    OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes);
    AdrType = ModNone;
  }
}

static Boolean IsRegCurrent(Byte No, tSymbolSize Size, Byte *Erg)
{
  switch (Size)
  {
    case eSymbolSize8Bit:
      if ((No & 0xf2) == 0xe0)
      {
        *Erg = ((No & 0x0c) >> 1) + ((No & 1) ^ 1);
        return True;
      }
      else
        return False;
    case eSymbolSize16Bit:
    case eSymbolSize32Bit:
      if ((No & 0xe3) == 0xe0)
      {
        *Erg = ((No & 0x1c) >> 2);
        return True;
      }
      else
        return False;
    default:
      return False;
  }
}

static const char Sizes[] = "124";

static Boolean GetPostInc(const char *pArg, int ArgLen, tSymbolSize *pOpSize, int *pCutoffRight)
{
  const char *pPos;

  /* <reg>+ */

  if ((ArgLen > 2) && (pArg[ArgLen - 1] == '+'))
  {
    *pOpSize = eSymbolSizeUnknown;
    *pCutoffRight = 1;
    return True;
  }

  /* <reg>++n, <reg>+:n */

  if ((ArgLen > 4) && (pArg[ArgLen - 3] == '+') && strchr(":+", pArg[ArgLen - 2]))
  {
    pPos = strchr(Sizes, pArg[ArgLen - 1]);
    if (pPos)
    {
      *pOpSize = (tSymbolSize)(pPos - Sizes);
      *pCutoffRight = 3;
      return True;
    }
  }
  return False;
}

static Boolean GetPreDec(const char *pArg, int ArgLen, tSymbolSize *pOpSize, int *pCutoffLeft, int *pCutoffRight)
{
  const char *pPos;

  /* n--<reg> */

  if ((ArgLen > 4) && (pArg[1] == '-') && (pArg[2] == '-'))
  {
    pPos = strchr(Sizes, pArg[0]);
    if (pPos)
    {
      *pOpSize = (tSymbolSize)(pPos - Sizes);
      *pCutoffLeft = 3;
      *pCutoffRight = 0;
      return True;
    }
  }

  if ((ArgLen > 2) && (pArg[0] == '-'))
  {
    *pCutoffLeft = 1;

    /* -<reg>:n */

    if ((ArgLen > 4) && (pArg[ArgLen - 2] == ':'))
    {
      pPos = strchr(Sizes, pArg[ArgLen - 1]);
      if (pPos)
      {
        *pOpSize = (tSymbolSize)(pPos - Sizes);
        *pCutoffRight = 2;
        return True;
      }
    }

    /* -<reg> */

    else
    {
      *pOpSize = eSymbolSizeUnknown;
      *pCutoffRight = 0;
      return True;
    }
  }
  return False;
}

static void ChkAdr(Byte Erl)
{
  if (AdrType != ModNone)
   if (!(Erl & (1 << AdrType)))
   {
     WrError(ErrNum_InvAddrMode);
     AdrType = ModNone;
   }
}

typedef struct
{
  as_eval_cb_data_t cb_data;
  Byte base_reg, index_reg, part_mask;
  tSymbolSize base_size, index_size;
} tlcs900_eval_cb_data_t;

DECLARE_AS_EVAL_CB(tlcs900_eval_cb)
{
  tlcs900_eval_cb_data_t *p_eval_cb_data = (tlcs900_eval_cb_data_t*)p_data;
  Byte reg_num;
  tSymbolSize reg_size;

  switch (CodeEReg(p_arg->str.p_str, &reg_num, &reg_size))
  {
    case eIsNoReg:
    {
      if ((as_eval_cb_data_stack_depth(p_data->p_stack) > 0)
       && as_eval_cb_data_stack_plain_add(p_data->p_stack))
        p_eval_cb_data->part_mask |= 1;
      return e_eval_none;
    }
    case eRegAbort:
      return e_eval_fail;
    case eIsReg:
      if (!as_eval_cb_data_stack_plain_add(p_data->p_stack))
      {
        WrError(ErrNum_InvAddrMode);
        return e_eval_fail;
      }
      else
      {
        Boolean MustInd;

        if (reg_size == eSymbolSize8Bit)
          MustInd = True;
        else if (reg_size == eSymbolSize32Bit)
          MustInd = False;
        else if (!IsRegBase(reg_num, reg_size))
          MustInd = True;
        else if (p_eval_cb_data->part_mask & 4)
          MustInd = True;
        else
          MustInd = False;
        if (MustInd)
        {
          if (p_eval_cb_data->part_mask & 2)
          {
            WrError(ErrNum_InvAddrMode);
            return e_eval_fail;
          }
          else
          {
            p_eval_cb_data->index_reg = reg_num;
            p_eval_cb_data->part_mask |= 2;
            p_eval_cb_data->index_size = reg_size;
          }
        }
        else
        {
          if (p_eval_cb_data->part_mask & 4)
          {
            WrError(ErrNum_InvAddrMode);
            return e_eval_fail;
          }
          else
          {
            p_eval_cb_data->base_reg = reg_num;
            p_eval_cb_data->part_mask |= 4;
            p_eval_cb_data->base_size = reg_size;
          }
        }
      }
      as_tempres_set_int(p_res, 0);
      return e_eval_ok;
    default:
      return e_eval_none;
  }
}

static Boolean force_op_size(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  tSymbolFlags add_flags = eSymbolFlag_None;

  if (!pLVal || !pRVal)
    return False;

  switch (pRVal->Contents.Int)
  {
    case 8:
      add_flags = eSymbolFlag_UserShort;
      goto copy;
    case 16:
      add_flags = eSymbolFlag_UserMedium;
      goto copy;
    case 24:
      add_flags = eSymbolFlag_UserLong;
      goto copy;
    default:
      WrError(ErrNum_InvDispLen);
      as_tempres_set_none(pErg);
      break;
    copy:
      as_tempres_copy_value(pErg, pLVal);
      pErg->Flags |= add_flags;
      pErg->DataSize = pLVal->DataSize;
  }
  return True;
}

static const as_operator_t tlcs900_operators[] =
{
  { ":" ,1 , e_op_dyadic , 1, { TempInt | (TempInt << 4), 0, 0, 0 }, force_op_size},
  {NULL, 0 , e_op_monadic, 0, { 0, 0, 0, 0, 0 }, NULL}
};

static void DecodeAdrMem(const tStrComp *pArg)
{
  LongInt DispAcc;
  tStrComp Arg;
  Byte HNum;
  tSymbolSize HSize;
  tEvalResult eval_result;
  int CutoffLeft, CutoffRight, ArgLen = strlen(pArg->str.p_str);
  tSymbolSize IncOpSize, disp_size;
  tlcs900_eval_cb_data_t eval_cb_data;
  Boolean FirstFlag;

  AdrType = ModNone;
  AutoIncSizeNeeded = False;

  /* post-increment */

  if ((ArgLen > 2)
   && GetPostInc(pArg->str.p_str, ArgLen, &IncOpSize, &CutoffRight))
  {
    String Reg;
    tStrComp RegComp;

    StrCompMkTemp(&RegComp, Reg, sizeof(Reg));
    StrCompCopy(&RegComp, pArg);
    StrCompShorten(&RegComp, CutoffRight);
    if (CodeEReg(RegComp.str.p_str, &HNum, &HSize) == eIsReg)
    {
      if (!IsRegBase(HNum, HSize)) WrStrErrorPos(ErrNum_InvAddrMode, &RegComp);
      else
      {
        if (IncOpSize == eSymbolSizeUnknown)
          IncOpSize = (tSymbolSize)OpSize;
        AdrType = ModMem;
        AdrMode = 0x45;
        AdrCnt = 1;
        AdrVals[0] = HNum;
        if (IncOpSize == eSymbolSizeUnknown)
          AutoIncSizeNeeded = True;
        else
          AdrVals[0] += IncOpSize;
      }
      return;
    }
  }

  /* pre-decrement ? */

  if ((ArgLen > 2)
   && GetPreDec(pArg->str.p_str, ArgLen, &IncOpSize, &CutoffLeft, &CutoffRight))
  {
    String Reg;
    tStrComp RegComp;

    StrCompMkTemp(&RegComp, Reg, sizeof(Reg));
    StrCompCopySub(&RegComp, pArg, CutoffLeft, ArgLen - CutoffLeft - CutoffRight);
    if (CodeEReg(RegComp.str.p_str, &HNum, &HSize) == eIsReg)
    {
      if (!IsRegBase(HNum, HSize)) WrError(ErrNum_InvAddrMode);
      else
      {
        if (IncOpSize == eSymbolSizeUnknown)
          IncOpSize = (tSymbolSize)OpSize;
        AdrType = ModMem;
        AdrMode = 0x44;
        AdrCnt = 1;
        AdrVals[0] = HNum;
        if (IncOpSize == eSymbolSizeUnknown)
          AutoIncSizeNeeded = True;
        else
          AdrVals[0] += IncOpSize;
      }
      return;
    }
  }

  Arg = *pArg;
  as_eval_cb_data_ini(&eval_cb_data.cb_data, tlcs900_eval_cb);
  eval_cb_data.cb_data.p_operators = tlcs900_operators;
  eval_cb_data.base_reg = eval_cb_data.index_reg = 0xff;
  eval_cb_data.base_size = eval_cb_data.index_size = eSymbolSizeUnknown;
  eval_cb_data.part_mask = 0;
  DispAcc = EvalStrIntExprWithResultAndCallback(&Arg, Int32, &eval_result, &eval_cb_data.cb_data);
  if (!eval_result.OK)
    return;
  if (!eval_cb_data.part_mask || DispAcc)
    eval_cb_data.part_mask |= 1;
  FirstFlag = mFirstPassUnknownOrQuestionable(eval_result.Flags);

  /* auto-deduce address/displacement size? */

  if (eval_result.Flags & eSymbolFlag_UserLong)
    disp_size = eSymbolSize24Bit;
  else if (eval_result.Flags & eSymbolFlag_UserMedium)
    disp_size = eSymbolSize16Bit;
  else if (eval_result.Flags & eSymbolFlag_UserShort)
    disp_size = eSymbolSize8Bit;
  else
    switch (eval_cb_data.part_mask)
    {
      case 1:
        if (DispAcc <= 0xff)
          disp_size = eSymbolSize8Bit;
        else if (DispAcc < 0xffff)
          disp_size = eSymbolSize16Bit;
        else
          disp_size = eSymbolSize24Bit;
        break;
      case 5:
        if (!DispAcc)
        {
          eval_cb_data.part_mask &= ~1;
          goto other;
        }
        if (RangeCheck(DispAcc, SInt8) && IsRegCurrent(eval_cb_data.base_reg, eval_cb_data.base_size, &AdrMode))
          disp_size = eSymbolSize8Bit;
        else
          disp_size = eSymbolSize16Bit;
        break;
      other:
      default:
        disp_size = eSymbolSizeUnknown;
    }

  switch (eval_cb_data.part_mask)
  {
    case 0:
    case 2:
    case 3:
    case 7:
      WrError(ErrNum_InvAddrMode);
      break;
    case 1:
      switch (disp_size)
      {
        case eSymbolSize8Bit:
          if (FirstFlag)
            DispAcc &= 0xff;
          if (DispAcc > 0xff) WrError(ErrNum_AdrOverflow);
          else
          {
            AdrType = ModMem;
            AdrMode = 0x40;
            AdrCnt = 1;
            AdrVals[0] = DispAcc;
          }
          break;
        case eSymbolSize16Bit:
          if (FirstFlag)
            DispAcc &= 0xffff;
          if (DispAcc > 0xffff) WrError(ErrNum_AdrOverflow);
          else
          {
            AdrType = ModMem;
            AdrMode = 0x41;
            AdrCnt = 2;
            AdrVals[0] = Lo(DispAcc);
            AdrVals[1] = Hi(DispAcc);
          }
          break;
        case eSymbolSize24Bit:
          if (FirstFlag)
            DispAcc &= 0xffffff;
          if (DispAcc > 0xffffff) WrError(ErrNum_AdrOverflow);
          else
          {
            AdrType = ModMem;
            AdrMode = 0x42;
            AdrCnt = 3;
            AdrVals[0] = DispAcc         & 0xff;
            AdrVals[1] = (DispAcc >>  8) & 0xff;
            AdrVals[2] = (DispAcc >> 16) & 0xff;
          }
          break;
        default:
          break; /* assert(0)? */
      }
      break;
    case 4:
      if (IsRegCurrent(eval_cb_data.base_reg, eval_cb_data.base_size, &AdrMode))
      {
        AdrType = ModMem;
        AdrCnt = 0;
      }
      else
      {
        AdrType = ModMem;
        AdrMode = 0x43;
        AdrCnt = 1;
        AdrVals[0] = eval_cb_data.base_reg;
      }
      break;
    case 5:
      switch (disp_size)
      {
        case eSymbolSize8Bit:
          if (FirstFlag)
            DispAcc &= 0x7f;
          if (!IsRegCurrent(eval_cb_data.base_reg, eval_cb_data.base_size, &AdrMode)) WrError(ErrNum_InvAddrMode);
          else if (ChkRange(DispAcc, -128, 127))
          {
            AdrType = ModMem;
            AdrMode += 8;
            AdrCnt = 1;
            AdrVals[0] = DispAcc & 0xff;
          }
          break;
        case eSymbolSize16Bit:
          if (FirstFlag)
            DispAcc &= 0x7fff;
          if (ChkRange(DispAcc, -32768, 32767))
          {
            AdrType = ModMem;
            AdrMode = 0x43;
            AdrCnt = 3;
            AdrVals[0] = eval_cb_data.base_reg + 1;
            AdrVals[1] = DispAcc & 0xff;
            AdrVals[2] = (DispAcc >> 8) & 0xff;
          }
          break;
        case eSymbolSize24Bit:
          WrError(ErrNum_InvDispLen);
          break;
        default:
          break; /* assert(0)? */
      }
      break;
    case 6:
      AdrType = ModMem;
      AdrMode = 0x43;
      AdrCnt = 3;
      AdrVals[0] = 3 + (eval_cb_data.index_size << 2);
      AdrVals[1] = eval_cb_data.base_reg;
      AdrVals[2] = eval_cb_data.index_reg;
      break;
  }
}

static void DecodeAdr(const tStrComp *pArg, Byte Erl)
{
  Byte HNum;
  tSymbolSize HSize;
  LongInt DispAcc;
  Boolean OK;

  AdrType = ModNone;
  AutoIncSizeNeeded = False;

  /* Register ? */

  switch (CodeEReg(pArg->str.p_str, &HNum, &HSize))
  {
    case eRegAbort:
      ChkAdr(Erl);
      return;
    case eIsReg:
      if (IsRegCurrent(HNum, HSize, &AdrMode))
        AdrType = ModReg;
      else
      {
       AdrType = ModXReg;
       AdrMode = HNum;
      }
      SetOpSize(HSize);
      ChkAdr(Erl);
      return;
    default:
      break;
  }

  /* Steuerregister ? */

  switch (CodeCReg(pArg->str.p_str, &HNum, &HSize))
  {
    case eRegAbort:
      ChkAdr(Erl);
      return;
    case eIsReg:
      AdrType = ModCReg;
      AdrMode = HNum;
      SetOpSize(HSize);
      ChkAdr(Erl);
      return;
    default:
      break;
  }

  /* Speicheroperand ? */

  if (IsIndirect(pArg->str.p_str))
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    StrCompShorten(&Arg, 1);
    KillPrefBlanksStrCompRef(&Arg);
    KillPostBlanksStrComp(&Arg);
    DecodeAdrMem(&Arg);
    ChkAdr(Erl); return;
  }

  /* bleibt nur noch immediate... */

  if ((MinOneIs0) && (OpSize == -1))
    OpSize = 0;
  switch (OpSize)
  {
    case -1:
      WrError(ErrNum_UndefOpSizes);
      break;
    case 0:
      AdrVals[0] = EvalStrIntExpression(pArg, Int8, &OK);
      if (OK)
      {
        AdrType = ModImm;
        AdrCnt = 1;
      }
      break;
    case 1:
      DispAcc = EvalStrIntExpression(pArg, Int16, &OK);
      if (OK)
      {
        AdrType = ModImm;
        AdrCnt = 2;
        AdrVals[0] = Lo(DispAcc);
        AdrVals[1] = Hi(DispAcc);
      }
      break;
    case 2:
      DispAcc = EvalStrIntExpression(pArg, Int32, &OK);
      if (OK)
      {
        AdrType = ModImm;
        AdrCnt = 4;
        AdrVals[0] = Lo(DispAcc);
        AdrVals[1] = Hi(DispAcc);
        AdrVals[2] = Lo(DispAcc >> 16);
        AdrVals[3] = Hi(DispAcc >> 16);
      }
      break;
  }
}

/*---------------------------------------------------------------------------*/

static void SetAutoIncSize(Byte AdrModePos, Byte FixupPos)
{
  if ((BAsmCode[AdrModePos] & 0x4e) == 0x44)
    BAsmCode[FixupPos] = (BAsmCode[FixupPos] & 0xfc) | OpSize;
}

static Boolean ArgPair(const char *Val1, const char *Val2)
{
  return  (((!as_strcasecmp(ArgStr[1].str.p_str, Val1)) && (!as_strcasecmp(ArgStr[2].str.p_str, Val2)))
        || ((!as_strcasecmp(ArgStr[1].str.p_str, Val2)) && (!as_strcasecmp(ArgStr[2].str.p_str, Val1))));
}

static LongInt ImmVal(void)
{
  LongInt tmp;

  tmp = AdrVals[0];
  if (OpSize >= 1)
    tmp += ((LongInt)AdrVals[1]) << 8;
  if (OpSize == 2)
  {
    tmp += ((LongInt)AdrVals[2]) << 16;
    tmp += ((LongInt)AdrVals[3]) << 24;
  }
  return tmp;
}

static Boolean IsPwr2(LongInt Inp, Byte *Erg)
{
  LongInt Shift;

  Shift = 1;
  *Erg = 0;
  do
  {
    if (Inp == Shift)
      return True;
    Shift += Shift;
    (*Erg)++;
  }
  while (Shift != 0);
  return False;
}

static Boolean IsShort(Byte Code)
{
  return ((Code & 0x4e) == 0x40);
}

static void CheckSup(void)
{
  if ((MomCPU == CPU96C141)
   && (!SupAllowed))
    WrError(ErrNum_PrivOrder);
}

/*!------------------------------------------------------------------------
 * \fn     decode_condition(const char *p_asc, Byte *p_condition)
 * \brief  decode condition code identifier
 * \param  p_asc source argument
 * \param  p_condition resulting code if found
 * \return True if found
 * ------------------------------------------------------------------------ */

static Boolean decode_condition(const char *p_asc, Byte *p_condition)
{
  int z;

  for (z = 0; Conditions[z].Name; z++)
    if (!as_strcasecmp(p_asc, Conditions[z].Name))
    {
      *p_condition = Conditions[z].Code;
      return True;
    }
  return False;
}

static void SetInstrOpSize(Byte Size)
{
  if (Size != 255)
    OpSize = Size;
}

/*---------------------------------------------------------------------------*/

static void DecodeMULA(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg);
    if ((AdrType != ModNone) && (OpSize != 2)) WrError(ErrNum_InvOpSize);
    else switch (AdrType)
    {
      case ModReg:
        CodeLen = 2;
        BAsmCode[0] = 0xd8 + AdrMode;
        BAsmCode[1] = 0x19;
        break;
      case ModXReg:
        CodeLen = 3;
        BAsmCode[0] = 0xd7;
        BAsmCode[1] = AdrMode;
        BAsmCode[2] = 0x19;
        break;
    }
  }
}

static void DecodeJPCALL(Word Index)
{
  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    if (IsIndirect(ArgStr[ArgCnt].str.p_str))
      DecodeAdr(&ArgStr[ArgCnt], MModMem);
    else
      DecodeAdrMem(&ArgStr[ArgCnt]);
    if (AdrType == ModMem)
    {
      if ((cond_code == COND_CODE_TRUE) && ((AdrMode == 0x41) || (AdrMode == 0x42)))
      {
        CodeLen = 1 + AdrCnt;
        BAsmCode[0] = 0x1a + 2 * Index + (AdrCnt - 2);
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      }
      else
      {
        CodeLen = 2 + AdrCnt;
        BAsmCode[0] = 0xb0 + AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[1 + AdrCnt] = 0xd0 + (Index << 4) + cond_code;
      }
    }
  }
}

static void DecodeJR(Word Index)
{
  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    Boolean OK;
    LongInt AdrLong;
    tSymbolFlags Flags;

    if (1 == ArgCnt)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int32, &OK, &Flags);
    if (OK)
    {
      if (Index==1)
      {
        AdrLong -= EProgCounter() + 3;
        if (((AdrLong > 0x7fffl) || (AdrLong < -0x8000l)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_DistTooBig);
        else
        {
          CodeLen = 3;
          BAsmCode[0] = 0x70 + cond_code;
          BAsmCode[1] = Lo(AdrLong);
          BAsmCode[2] = Hi(AdrLong);
          if (!mFirstPassUnknown(Flags))
          {
            AdrLong++;
            if ((AdrLong >= -128) && (AdrLong <= 127)) WrError(ErrNum_ShortJumpPossible);
          }
        }
      }
      else
      {
        AdrLong -= EProgCounter() + 2;
        if (((AdrLong > 127) || (AdrLong < -128)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_DistTooBig);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0x60 + cond_code;
          BAsmCode[1] = Lo(AdrLong);
        }
      }
    }
  }
}

static void DecodeCALR(Word Index)
{
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags) - (EProgCounter() + 3);
    if (OK)
    {
      if (((AdrLong < -32768) || (AdrLong > 32767)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_DistTooBig);
      else
      {
        CodeLen = 3;
        BAsmCode[0] = 0x1e;
        BAsmCode[1] = Lo(AdrLong);
        BAsmCode[2] = Hi(AdrLong);
      }
    }
  }
}

static void DecodeRET(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(0, 1))
  {
    Byte cond_code;

    if (ArgCnt == 0)
      cond_code = COND_CODE_TRUE;
    else if (!decode_condition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    if (cond_code == COND_CODE_TRUE)
    {
      CodeLen = 1;
      BAsmCode[0] = 0x0e;
    }
    else
    {
      CodeLen = 2;
      BAsmCode[0] = 0xb0;
      BAsmCode[1] = 0xf0 + cond_code;
    }
  }
}

static void DecodeRETD(Word Index)
{
  Word AdrWord;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x0f;
      BAsmCode[1] = Lo(AdrWord);
      BAsmCode[2] = Hi(AdrWord);
    }
  }
}

static void DecodeDJNZ(Word Index)
{
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(1, 2))
  {
    if (ArgCnt == 1)
    {
      AdrType = ModReg;
      AdrMode = 2;
      OpSize = 0;
    }
    else
      DecodeAdr(&ArgStr[1], MModReg | MModXReg);
    if (AdrType != ModNone)
    {
      if (OpSize == 2) WrError(ErrNum_InvOpSize);
      else
      {
        AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int32, &OK, &Flags) - (EProgCounter() + 3 + Ord(AdrType == ModXReg));
        if (OK)
        {
          if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
           switch (AdrType)
           {
             case ModReg:
               CodeLen = 3;
               BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
               BAsmCode[1] = 0x1c;
               BAsmCode[2] = AdrLong & 0xff;
               break;
             case ModXReg:
               CodeLen = 4;
               BAsmCode[0] = 0xc7 + (OpSize << 4);
               BAsmCode[1] = AdrMode;
               BAsmCode[2] = 0x1c;
               BAsmCode[3] = AdrLong & 0xff;
               break;
           }
        }
      }
    }
  }
}

static void DecodeEX(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  /* work around the parser problem related to the ' character */

  if (!as_strncasecmp(ArgStr[2].str.p_str, "F\'", 2))
    ArgStr[2].str.p_str[2] = '\0';

  if (!ChkArgCnt(2, 2));
  else if ((ArgPair("F", "F\'")) || (ArgPair("F`", "F")))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x16;
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg | MModMem);
    if (OpSize == 2) WrError(ErrNum_InvOpSize);
    else
    {
      switch (AdrType)
      {
        case ModReg:
          HReg = AdrMode;
          DecodeAdr(&ArgStr[2], MModReg | MModXReg | MModMem);
          switch (AdrType)
          {
            case ModReg:
              CodeLen = 2;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
              BAsmCode[1] = 0xb8 + HReg;
              break;
            case ModXReg:
              CodeLen = 3;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = AdrMode;
              BAsmCode[2] = 0xb8 + HReg;
              break;
            case ModMem:
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0x30 + HReg;
              break;
          }
          break;
        case ModXReg:
          HReg = AdrMode;
          DecodeAdr(&ArgStr[2], MModReg);
          if (AdrType == ModReg)
          {
            CodeLen = 3;
            BAsmCode[0] = 0xc7 + (OpSize << 4);
            BAsmCode[1] = HReg;
            BAsmCode[2] = 0xb8 + AdrMode;
          }
          break;
        case ModMem:
        {
          Boolean FixupAutoIncSize = AutoIncSizeNeeded;

          MinOneIs0 = True;
          HReg = AdrCnt;
          BAsmCode[0] = AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          DecodeAdr(&ArgStr[2], MModReg);
          if (AdrType == ModReg)
          {
            CodeLen = 2 + HReg;
            if (FixupAutoIncSize)
              SetAutoIncSize(0, 1);
            BAsmCode[0] += 0x80 + (OpSize << 4);
            BAsmCode[1 + HReg] = 0x30 + AdrMode;
          }
          break;
        }
      }
    }
  }
}

static void DecodeBS1x(Word Index)
{
  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
  {
    DecodeAdr(&ArgStr[2], MModReg | MModXReg);
    if (OpSize != 1) WrError(ErrNum_InvOpSize);
    else switch (AdrType)
    {
      case ModReg:
        CodeLen = 2;
        BAsmCode[0] = 0xd8 + AdrMode;
        BAsmCode[1] = 0x0e + Index; /* ANSI */
        break;
      case ModXReg:
        CodeLen = 3;
        BAsmCode[0] = 0xd7;
        BAsmCode[1] = AdrMode;
        BAsmCode[2] = 0x0e +Index; /* ANSI */
        break;
    }
  }
}

static void DecodeLDA(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrType != ModNone)
    {
      if (OpSize < 1) WrError(ErrNum_InvOpSize);
      else
      {
        HReg = AdrMode;
        if (IsIndirect(ArgStr[2].str.p_str))
          DecodeAdr(&ArgStr[2], MModMem);
        else
          DecodeAdrMem(&ArgStr[2]);
        if (AdrType != ModNone)
        {
          CodeLen = 2 + AdrCnt;
          BAsmCode[0] = 0xb0 + AdrMode;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = 0x20 + ((OpSize - 1) << 4) + HReg;
        }
      }
    }
  }
}

static void DecodeLDAR(Word Index)
{
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[2], Int32, &OK, &Flags) - (EProgCounter() + 4);
    if (OK)
    {
      if (((AdrLong < -32768) || (AdrLong > 32767)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_DistTooBig);
      else
      {
        DecodeAdr(&ArgStr[1], MModReg);
        if (AdrType != ModNone)
        {
          if (OpSize < 1) WrError(ErrNum_InvOpSize);
          else
          {
            CodeLen = 5;
            BAsmCode[0] = 0xf3;
            BAsmCode[1] = 0x13;
            BAsmCode[2] = Lo(AdrLong);
            BAsmCode[3] = Hi(AdrLong);
            BAsmCode[4] = 0x20 + ((OpSize - 1) << 4) + AdrMode;
          }
        }
      }
    }
  }
}

static void DecodeLDC(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg| MModCReg);
    HReg = AdrMode;
    switch (AdrType)
    {
      case ModReg:
        DecodeAdr(&ArgStr[2], MModCReg);
        if (AdrType != ModNone)
        {
          CodeLen = 3;
          BAsmCode[0] = 0xc8 + (OpSize << 4) + HReg;
          BAsmCode[1] = 0x2f;
          BAsmCode[2] = AdrMode;
        }
        break;
      case ModXReg:
        DecodeAdr(&ArgStr[2], MModCReg);
        if (AdrType != ModNone)
        {
          CodeLen = 4;
          BAsmCode[0] = 0xc7 + (OpSize << 4);
          BAsmCode[1] = HReg;
          BAsmCode[2] = 0x2f;
          BAsmCode[3] = AdrMode;
        };
        break;
      case ModCReg:
        DecodeAdr(&ArgStr[2], MModReg | MModXReg);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 3;
            BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
            BAsmCode[1] = 0x2e;
            BAsmCode[2] = HReg;
            break;
          case ModXReg:
            CodeLen = 4;
            BAsmCode[0] = 0xc7 + (OpSize << 4);
            BAsmCode[1] = AdrMode;
            BAsmCode[2] = 0x2e;
            BAsmCode[3] = HReg;
            break;
        }
        break;
    }
  }
}

static void DecodeLDX(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModMem);
    if (AdrType != ModNone)
    {
      if (AdrMode != 0x40) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[4] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
        if (OK)
        {
          CodeLen = 6;
          BAsmCode[0] = 0xf7;
          BAsmCode[1] = 0;
          BAsmCode[2] = AdrVals[0];
          BAsmCode[3] = 0;
          BAsmCode[5] = 0;
        }
      }
    }
  }
}

static void DecodeLINK(Word Index)
{
  Word AdrWord;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    AdrWord = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      DecodeAdr(&ArgStr[1], MModReg | MModXReg);
      if ((AdrType != ModNone) && (OpSize != 2)) WrError(ErrNum_InvOpSize);
      else
       switch (AdrType)
       {
         case ModReg:
           CodeLen = 4;
           BAsmCode[0] = 0xe8 + AdrMode;
           BAsmCode[1] = 0x0c;
           BAsmCode[2] = Lo(AdrWord);
           BAsmCode[3] = Hi(AdrWord);
           break;
         case ModXReg:
           CodeLen = 5;
           BAsmCode[0] = 0xe7;
           BAsmCode[1] = AdrMode;
           BAsmCode[2] = 0x0c;
           BAsmCode[3] = Lo(AdrWord);
           BAsmCode[4] = Hi(AdrWord);
           break;
       }
    }
  }
}

static void DecodeLD(Word Code)
{
  Byte HReg;
  Boolean ShDest, ShSrc, OK;

  SetInstrOpSize(Hi(Code));

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg | MModMem);
    switch (AdrType)
    {
      case ModReg:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg | MModXReg| MModMem| MModImm);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
            BAsmCode[1] = 0x88 + HReg;
            break;
          case ModXReg:
            CodeLen = 3;
            BAsmCode[0] = 0xc7 + (OpSize << 4);
            BAsmCode[1] = AdrMode;
            BAsmCode[2] = 0x88 + HReg;
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x20 + HReg;
            break;
          case ModImm:
          {
            LongInt ImmValue = ImmVal();

            if ((ImmValue <= 7) && (ImmValue >= 0))
            {
              CodeLen = 2;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + HReg;
              BAsmCode[1] = 0xa8 + AdrVals[0];
            }
            else
            {
              CodeLen = 1 + AdrCnt;
              BAsmCode[0] = ((OpSize + 2) << 4) + HReg;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            }
            break;
          }
        }
        break;
      case ModXReg:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 3;
            BAsmCode[0] = 0xc7 + (OpSize << 4);
            BAsmCode[1] = HReg;
            BAsmCode[2] = 0x98 + AdrMode;
            break;
          case ModImm:
          {
            LongInt ImmValue = ImmVal();

            if ((ImmValue <= 7) && (ImmValue >= 0))
            {
              CodeLen = 3;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = HReg;
              BAsmCode[2] = 0xa8 + AdrVals[0];
            }
            else
            {
              CodeLen = 3 + AdrCnt;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = HReg;
              BAsmCode[2] = 3;
              memcpy(BAsmCode + 3, AdrVals, AdrCnt);
            }
            break;
          }
        }
        break;
      case ModMem:
      {
        Boolean FixupAutoIncSize = AutoIncSizeNeeded;

        BAsmCode[0] = AdrMode;
        HReg = AdrCnt;
        MinOneIs0 = True;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModReg | MModMem | MModImm);
        switch (AdrType)
        {
         case ModReg:
           CodeLen = 2 + HReg;
           BAsmCode[0] += 0xb0;
           if (FixupAutoIncSize)
             SetAutoIncSize(0, 1);
           BAsmCode[1 + HReg] = 0x40 + (OpSize << 4) + AdrMode;
           break;
         case ModMem:
           if (OpSize == -1) OpSize = 0;
           ShDest = IsShort(BAsmCode[0]);
           ShSrc = IsShort(AdrMode);
           if (!(ShDest || ShSrc)) WrError(ErrNum_InvAddrMode);
           else
           {
             if ((ShDest && (!ShSrc))) OK = True;
             else if (ShSrc && (!ShDest)) OK = False;
             else if (AdrMode == 0x40) OK = True;
             else OK = False;
             if (OK)  /* dest=(dir8/16) */
             {
               CodeLen = 4 + AdrCnt;
               HReg = BAsmCode[0];
               BAsmCode[3 + AdrCnt] = (BAsmCode[0] == 0x40) ? 0 : BAsmCode[2];
               BAsmCode[2 + AdrCnt] = BAsmCode[1];
               BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
               AdrMode = HReg;
               if (FixupAutoIncSize)
                 SetAutoIncSize(0, 1);
               memcpy(BAsmCode + 1, AdrVals, AdrCnt);
               BAsmCode[1 + AdrCnt] = 0x19;
             }
             else
             {
               CodeLen = 4 + HReg;
               BAsmCode[2 + HReg] = AdrVals[0];
               BAsmCode[3 + HReg] = (AdrMode == 0x40) ? 0 : AdrVals[1];
               BAsmCode[0] += 0xb0;
               if (FixupAutoIncSize)
                 SetAutoIncSize(0, 1);
               BAsmCode[1 + HReg] = 0x14 + (OpSize << 1);
             }
           }
           break;
         case ModImm:
          if (BAsmCode[0] == 0x40)
          {
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0x08 + (OpSize << 1);
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          }
          else
          {
            CodeLen = 2 + HReg + AdrCnt;
            BAsmCode[0] += 0xb0;
            BAsmCode[1 + HReg] = OpSize << 1;
            memcpy(BAsmCode + 2 + HReg, AdrVals, AdrCnt);
          }
          break;
        }
        break;
      }
    }
  }
}

static void DecodeFixed(Word Index)
{
  FixedOrder *FixedZ = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && (ChkExactCPUMask(FixedZ->CPUFlag, CPU96C141) >= 0))
  {
    if (Hi(FixedZ->Code) == 0)
    {
      CodeLen = 1;
      BAsmCode[0] = Lo(FixedZ->Code);
    }
    else
    {
      CodeLen = 2;
      BAsmCode[0] = Hi(FixedZ->Code);
      BAsmCode[1] = Lo(FixedZ->Code);
    }
    if (FixedZ->InSup)
      CheckSup();
  }
}

static void DecodeImm(Word Index)
{
  ImmOrder *ImmZ = ImmOrders + Index;
  Word AdrWord;
  Boolean OK;

  if (ChkArgCnt((ImmZ->Default == -1) ? 1 : 0, 1))
  {
    if (ArgCnt == 0)
    {
      AdrWord = ImmZ->Default;
      OK = True;
    }
    else
      AdrWord = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      if (((MaxMode) && (AdrWord > ImmZ->MaxMax)) || ((!MaxMode) && (AdrWord > ImmZ->MinMax))) WrError(ErrNum_OverRange);
      else if (Hi(ImmZ->Code) == 0)
      {
        CodeLen = 1;
        BAsmCode[0] = Lo(ImmZ->Code) + AdrWord;
      }
      else
      {
        CodeLen = 2;
        BAsmCode[0] = Hi(ImmZ->Code);
        BAsmCode[1] = Lo(ImmZ->Code) + AdrWord;
      }
    }
    if (ImmZ->InSup)
      CheckSup();
  }
}

static void DecodeALU2(Word Code);

static void DecodeReg(Word Index)
{
  RegOrder *RegZ = RegOrders + Index;

  /* dispatch to CPL as compare-long with two args: */

  if ((Memo("CPL")) && (ArgCnt >= 2))
  {
    DecodeALU2(0x0207);
    return;
  }

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg);
    if (AdrType != ModNone)
    {
      if (((1 << OpSize) & RegZ->OpMask) == 0) WrError(ErrNum_InvOpSize);
      else if (AdrType == ModReg)
      {
        BAsmCode[0] = Hi(RegZ->Code) + 8 + (OpSize << 4) + AdrMode;
        BAsmCode[1] = Lo(RegZ->Code);
        CodeLen = 2;
      }
      else
      {
        BAsmCode[0] = Hi(RegZ->Code) + 7 + (OpSize << 4);
        BAsmCode[1] = AdrMode;
        BAsmCode[2] = Lo(RegZ->Code);
        CodeLen = 3;
      }
    }
  }
}

static void DecodeALU2(Word Code)
{
  Byte HReg;

  SetInstrOpSize(Hi(Code));
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg | MModMem);
    switch (AdrType)
    {
      case ModReg:
        HReg=AdrMode;
        DecodeAdr(&ArgStr[2], MModReg | MModXReg | MModMem | MModImm);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
            BAsmCode[1] = 0x80 + (Lo(Code) << 4) + HReg;
            break;
          case ModXReg:
            CodeLen = 3;
            BAsmCode[0] = 0xc7 + (OpSize << 4);
            BAsmCode[1] = AdrMode;
            BAsmCode[2] = 0x80 + (Lo(Code) << 4) + HReg;
            break;
          case ModMem:
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0x80 + AdrMode + (OpSize << 4);
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x80 + HReg + (Lo(Code) << 4);
            break;
          case ModImm:
            if ((Lo(Code) == 7) && (OpSize != 2) && (ImmVal() <= 7) && (ImmVal() >= 0))
            {
              CodeLen = 2;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + HReg;
              BAsmCode[1] = 0xd8 + AdrVals[0];
            }
            else
            {
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + HReg;
              BAsmCode[1] = 0xc8 + Lo(Code);
              memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            }
            break;
        }
        break;
      case ModXReg:
        HReg = AdrMode;
        DecodeAdr(&ArgStr[2], MModImm);
        switch (AdrType)
        {
          case ModImm:
            if ((Lo(Code) == 7) && (OpSize != 2) && (ImmVal() <= 7) && (ImmVal() >= 0))
            {
              CodeLen = 3;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = HReg;
              BAsmCode[2] = 0xd8 + AdrVals[0];
            }
            else
            {
              CodeLen = 3 + AdrCnt;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = HReg;
              BAsmCode[2] = 0xc8 + Lo(Code);
              memcpy(BAsmCode + 3, AdrVals, AdrCnt);
            }
            break;
        }
        break;
      case ModMem:
      {
        Boolean FixupAutoIncSize = AutoIncSizeNeeded;

        MinOneIs0 = True;
        HReg = AdrCnt;
        BAsmCode[0] = AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 2 + HReg;
            if (FixupAutoIncSize)
              SetAutoIncSize(0, 1);
            BAsmCode[0] += 0x80 + (OpSize << 4);
            BAsmCode[1 + HReg] = 0x88 + (Lo(Code) << 4) + AdrMode;
            break;
          case ModImm:
            CodeLen = 2 + HReg + AdrCnt;
            BAsmCode[0] += 0x80 + (OpSize << 4);
            BAsmCode[1 + HReg] = 0x38 + Lo(Code);
            memcpy(BAsmCode + 2 + HReg, AdrVals, AdrCnt);
            break;
        };
        break;
      }
    }
  }
}

static void DecodePUSH_POP(Word Code)
{
  SetInstrOpSize(Hi(Code));

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "F"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x18 + Lo(Code);
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x14 + Lo(Code);
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "SR"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x02 + Lo(Code);
    CheckSup();
  }
  else
  {
    MinOneIs0 = True;
    DecodeAdr(&ArgStr[1], MModReg | MModXReg | MModMem | (Lo(Code) ? 0 : MModImm));
    switch (AdrType)
    {
      case ModReg:
        if (OpSize == 0)
        {
          CodeLen = 2;
          BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
          BAsmCode[1] = 0x04 + Lo(Code);
        }
        else
        {
          CodeLen = 1;
          BAsmCode[0] = 0x28 + (Lo(Code) << 5) + ((OpSize - 1) << 4) + AdrMode;
        }
        break;
      case ModXReg:
        CodeLen = 3;
        BAsmCode[0] = 0xc7 + (OpSize << 4);
        BAsmCode[1] = AdrMode;
        BAsmCode[2] = 0x04 + Lo(Code);
        break;
      case ModMem:
        if (OpSize == -1)
          OpSize=0;
        CodeLen = 2 + AdrCnt;
        if (Lo(Code))
          BAsmCode[0] = 0xb0 + AdrMode;
        else
          BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        if (Lo(Code))
          BAsmCode[1 + AdrCnt] = 0x04 + (OpSize << 1);
        else
          BAsmCode[1 + AdrCnt] = 0x04;
        break;
      case ModImm:
        if (OpSize == -1)
          OpSize = 0;
        BAsmCode[0] = 9 + (OpSize << 1);
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen = 1 + AdrCnt;
        break;
    }
  }
}

static void DecodeShift(Word Code)
{
  Boolean OK;
  tSymbolFlags Flags;
  Byte HReg;

  SetInstrOpSize(Hi(Code));

  if (ChkArgCnt(1, 2))
  {
    OK = True;
    if (ArgCnt == 1)
      HReg = 1;
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      HReg = 0xff;
    else
    {
      HReg = EvalStrIntExpressionWithFlags(&ArgStr[1], Int8, &OK, &Flags);
      if (OK)
      {
        if (mFirstPassUnknown(Flags))
          HReg &= 0x0f;
        else
        {
          if ((HReg == 0) || (HReg > 16))
          {
            WrError(ErrNum_OverRange);
            OK = False;
          }
          else
            HReg &= 0x0f;
        }
      }
    }
    if (OK)
    {
      DecodeAdr(&ArgStr[ArgCnt], MModReg | MModXReg | ((HReg == 0xff) ? 0 : MModMem));
      switch (AdrType)
      {
        case ModReg:
          CodeLen = 2 + Ord(HReg != 0xff);
          BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
          BAsmCode[1] = 0xe8 + Lo(Code);
          CodeLen = 2;
          if (HReg == 0xff)
            BAsmCode[1] += 0x10;
          else
            BAsmCode[CodeLen++] = HReg;
          break;
        case ModXReg:
          BAsmCode[0] = 0xc7+(OpSize << 4);
          BAsmCode[1] = AdrMode;
          BAsmCode[2] = 0xe8 + Lo(Code);
          CodeLen = 3;
          if (HReg == 0xff)
            BAsmCode[2] += 0x10;
          else
            BAsmCode[CodeLen++] = HReg;
          break;
        case ModMem:
          if (HReg != 1) WrError(ErrNum_InvAddrMode);
          else
          {
            if (OpSize == -1)
              OpSize = 0;
            CodeLen = 2 + AdrCnt;
            BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
            memcpy(BAsmCode + 1 , AdrVals, AdrCnt);
            BAsmCode[1 + AdrCnt] = 0x78 + Lo(Code);
          }
          break;
      }
    }
  }
}

static void DecodeMulDiv(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModXReg);
    if (OpSize == 0) WrError(ErrNum_InvOpSize);
    else
    {
      if ((AdrType == ModReg) && (OpSize == 1))
      {
        if (AdrMode > 3)
        {
          AdrType = ModXReg;
          AdrMode = 0xe0 + (AdrMode << 2);
        }
        else
          AdrMode += 1 + AdrMode;
      }
      OpSize--;
      HReg = AdrMode;
      switch (AdrType)
      {
        case ModReg:
          DecodeAdr(&ArgStr[2], MModReg | MModXReg | MModMem | MModImm);
          switch (AdrType)
          {
            case ModReg:
              CodeLen = 2;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
              BAsmCode[1] = 0x40 + (Code << 3) + HReg;
              break;
            case ModXReg:
              CodeLen = 3;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = AdrMode;
              BAsmCode[2] = 0x40 + (Code << 3) + HReg;
              break;
            case ModMem:
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0x40 + (Code << 3) + HReg;
              break;
            case ModImm:
              CodeLen = 2 + AdrCnt;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + HReg;
              BAsmCode[1] = 0x08 + Code;
              memcpy(BAsmCode + 2, AdrVals, AdrCnt);
              break;
          }
          break;
        case ModXReg:
          DecodeAdr(&ArgStr[2], MModImm);
          if (AdrType == ModImm)
          {
            CodeLen = 3 + AdrCnt;
            BAsmCode[0] = 0xc7 + (OpSize << 4);
            BAsmCode[1] = HReg;
            BAsmCode[2] = 0x08 + Code;
            memcpy(BAsmCode + 3, AdrVals, AdrCnt);
          }
          break;
      }
    }
  }
}

static void DecodeBitCF(Word Code)
{
  Boolean OK;
  Byte BitPos;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg | MModXReg | MModMem);
    if (AdrType!=ModNone)
    {
      if (OpSize == 2) WrError(ErrNum_InvOpSize);
      else
      {
        if (AdrType == ModMem)
          OpSize = 0;
        if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
        {
          BitPos = 0xff;
          OK = True;
        }
        else
          BitPos = EvalStrIntExpression(&ArgStr[1], (OpSize == 0) ? UInt3 : Int4, &OK);
        if (OK)
         switch (AdrType)
         {
           case ModReg:
             BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
             BAsmCode[1] = 0x20 + Code;
             CodeLen = 2;
             if (BitPos == 0xff)
               BAsmCode[1] |= 0x08;
             else
               BAsmCode[CodeLen++] = BitPos;
             break;
           case ModXReg:
             BAsmCode[0] = 0xc7 + (OpSize << 4);
             BAsmCode[1] = AdrMode;
             BAsmCode[2] = 0x20 + Code;
             CodeLen = 3;
             if (BitPos == 0xff)
               BAsmCode[2] |= 0x08;
             else
               BAsmCode[CodeLen++] = BitPos;
             break;
           case ModMem:
             CodeLen = 2 + AdrCnt;
             BAsmCode[0] = 0xb0 + AdrMode;
             memcpy(BAsmCode + 1, AdrVals, AdrCnt);
             BAsmCode[1 + AdrCnt] = (BitPos == 0xff)
                                  ? 0x28 + Code
                                  : 0x80 + (Code << 3) + BitPos;
             break;
         }
      }
    }
  }
}

static void DecodeBit(Word Code)
{
  Boolean OK;
  Byte BitPos;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg | MModXReg | MModMem);
    if (AdrType == ModMem)
      OpSize = 0;
    if (AdrType != ModNone)
    {
      if (OpSize == 2) WrError(ErrNum_InvOpSize);
      else
      {
        BitPos = EvalStrIntExpression(&ArgStr[1], (OpSize == 0) ? UInt3 : Int4, &OK);
        if (OK)
        {
          switch (AdrType)
          {
            case ModReg:
              CodeLen = 3;
              BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
              BAsmCode[1] = 0x30 + Code;
              BAsmCode[2] = BitPos;
              break;
            case ModXReg:
              CodeLen = 4;
              BAsmCode[0] = 0xc7 + (OpSize << 4);
              BAsmCode[1] = AdrMode;
              BAsmCode[2] = 0x30 + Code;
              BAsmCode[3] = BitPos;
              break;
            case ModMem:
              CodeLen = 2 + AdrCnt;
              Code = (Code == 4) ? 0 : Code + 1;
              BAsmCode[0] = 0xb0 + AdrMode;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              BAsmCode[1 + AdrCnt] = 0xa8 + (Code << 3) + BitPos;
              break;
          }
        }
      }
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  Boolean OK;
  Byte Incr;
  tSymbolFlags Flags;

  SetInstrOpSize(Hi(Code));

  if (ChkArgCnt(1, 2))
  {
    if (ArgCnt == 1)
    {
      Incr = 1;
      OK = True;
      Flags = eSymbolFlag_None;
    }
    else
      Incr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int4, &OK, &Flags);
    if (OK)
    {
      if (mFirstPassUnknown(Flags))
        Incr &= 7;
      else if ((Incr < 1) || (Incr > 8))
      {
        WrError(ErrNum_OverRange);
        OK = False;
      }
    }
    if (OK)
    {
      Incr &= 7;    /* 8-->0 */
      DecodeAdr(&ArgStr[ArgCnt], MModReg | MModXReg | MModMem);
      switch (AdrType)
      {
        case ModReg:
          CodeLen = 2;
          BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
          BAsmCode[1] = 0x60 + (Lo(Code) << 3) + Incr;
          break;
        case ModXReg:
          CodeLen = 3;
          BAsmCode[0] = 0xc7 + (OpSize << 4);
          BAsmCode[1] = AdrMode;
          BAsmCode[2] = 0x60 + (Lo(Code) << 3) + Incr;
          break;
        case ModMem:
          if (OpSize == -1)
            OpSize = 0;
          CodeLen = 2 + AdrCnt;
          BAsmCode[0] = 0x80 + AdrMode + (OpSize << 4);
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = 0x60 + (Lo(Code) << 3) + Incr;
          break;
      }
    }
  }
}

static void DecodeCPxx(Word Code)
{
  Boolean OK;

  if (ChkArgCntExtEitherOr(ArgCnt, 0, 2))
  {
    OK = True;
    if (ArgCnt == 0)
    {
      OpSize = 0;
      AdrMode = 3;
    }
    else
    {
      tSymbolSize HSize;
      int l = strlen(ArgStr[2].str.p_str);
      const char *CmpStr;

      if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
        OpSize = 0;
      else if (!as_strcasecmp(ArgStr[1].str.p_str, "WA"))
        OpSize = 1;
      CmpStr = (Code & 0x02) ? "-)" : "+)";
      if (OpSize == -1) OK = False;
      else if ((l < 2) || (*ArgStr[2].str.p_str != '(') || (as_strcasecmp(ArgStr[2].str.p_str + l - 2, CmpStr))) OK = False;
      else
      {
        ArgStr[2].str.p_str[l - 2] = '\0';
        if (CodeEReg(ArgStr[2].str.p_str + 1, &AdrMode, &HSize) != eIsReg) OK = False;
        else if (!IsRegBase(AdrMode, HSize)) OK = False;
        else if (!IsRegCurrent(AdrMode, HSize, &AdrMode)) OK = False;
      }
      if (!OK)
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
    }
    if (OK)
    {
      CodeLen = 2;
      BAsmCode[0] = 0x80 + (OpSize << 4) + AdrMode;
      BAsmCode[1] = Code;
    }
  }
}

static void DecodeLDxx(Word Code)
{
  SetInstrOpSize(Hi(Code));

  if (OpSize == -1)
    OpSize = 0;
  if (OpSize == 2) WrError(ErrNum_InvOpSize);
  else if (ChkArgCntExtEitherOr(ArgCnt, 0, 2))
  {
    Boolean OK;
    Byte HReg = 0;

    if (ArgCnt == 0)
    {
      OK = True;
    }
    else
    {
      const char *CmpStr;
      int l1 = strlen(ArgStr[1].str.p_str),
          l2 = strlen(ArgStr[2].str.p_str);

      OK = True;
      CmpStr = (Code & 0x02) ? "-)" : "+)";
      if ((*ArgStr[1].str.p_str != '(') || (*ArgStr[2].str.p_str != '(')
       || (l1 < 3) || (l2 < 3)
       || (as_strcasecmp(ArgStr[1].str.p_str + l1 - 2, CmpStr))
       || (as_strcasecmp(ArgStr[2].str.p_str + l2 - 2, CmpStr)))
        OK = False;
      else
      {
        ArgStr[1].str.p_str[l1 - 2] = '\0';
        ArgStr[2].str.p_str[l2 - 2] = '\0';
        if ((!as_strcasecmp(ArgStr[1].str.p_str + 1,"XIX")) && (!as_strcasecmp(ArgStr[2].str.p_str + 1, "XIY")))
          HReg = 2;
        else if ((MaxMode) && (!as_strcasecmp(ArgStr[1].str.p_str + 1, "XDE")) && (!as_strcasecmp(ArgStr[2].str.p_str + 1 , "XHL")));
        else if ((!MaxMode) && (!as_strcasecmp(ArgStr[1].str.p_str + 1, "DE")) && (!as_strcasecmp(ArgStr[2].str.p_str + 1 , "HL")));
        else
          OK = False;
      }
    }
    if (!OK) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 2;
      BAsmCode[0] = 0x83 + (OpSize << 4) + HReg;
      BAsmCode[1] = Lo(Code);
    }
  }
}

static void DecodeMINC_MDEC(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    Word AdrWord;
    Boolean OK;

    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      Byte Pwr;
      Byte ByteSizeLg2 = Code & 3, ByteSize = 1 << ByteSizeLg2;

      if (!IsPwr2(AdrWord, &Pwr)) WrStrErrorPos(ErrNum_NotPwr2, &ArgStr[1]);
      else if (Pwr <= ByteSizeLg2) WrStrErrorPos(ErrNum_UnderRange, &ArgStr[1]);
      else
      {
        AdrWord -= ByteSize;
        DecodeAdr(&ArgStr[2], MModReg | MModXReg);
        if ((AdrType != ModNone) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
        else
         switch (AdrType)
         {
           case ModReg:
             CodeLen = 4;
             BAsmCode[0] = 0xd8 + AdrMode;
             BAsmCode[1] = Code;
             BAsmCode[2] = Lo(AdrWord);
             BAsmCode[3] = Hi(AdrWord);
             break;
           case ModXReg:
             CodeLen = 5;
             BAsmCode[0] = 0xd7;
             BAsmCode[1] = AdrMode;
             BAsmCode[2] = Code;
             BAsmCode[3] = Lo(AdrWord);
             BAsmCode[4] = Hi(AdrWord);
             break;
         }
      }
    }
  }
}

static void DecodeRLD_RRD(Word Code)
{
  if (!ChkArgCnt(1, 2));
  else if ((ArgCnt == 2) && (as_strcasecmp(ArgStr[1].str.p_str, "A"))) WrError(ErrNum_InvAddrMode);
  else
  {
    DecodeAdr(&ArgStr[ArgCnt], MModMem);
    if (AdrType != ModNone)
    {
      CodeLen = 2 + AdrCnt;
      BAsmCode[0] = 0x80 + AdrMode;
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      BAsmCode[1 + AdrCnt] = Code;
    }
  }
}

static void DecodeSCC(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Byte cond_code;

    if (!decode_condition(ArgStr[1].str.p_str, &cond_code)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
    else
    {
      DecodeAdr(&ArgStr[2], MModReg | MModXReg);
      if (OpSize > 1) WrError(ErrNum_UndefOpSizes);
      else
      {
         switch (AdrType)
         {
           case ModReg:
             CodeLen = 2;
             BAsmCode[0] = 0xc8 + (OpSize << 4) + AdrMode;
             BAsmCode[1] = 0x70 + cond_code;
             break;
           case ModXReg:
             CodeLen = 3;
             BAsmCode[0] = 0xc7 + (OpSize << 4);
             BAsmCode[1] = AdrMode;
             BAsmCode[2] = 0x70 + cond_code;
             break;
         }
      }
    }
  }
  return;
}

/*---------------------------------------------------------------------------*/

static void AddSize(const char *NName, Byte NCode, InstProc Proc, Word SizeMask)
{
  int l;
  char SizeName[20];

  AddInstTable(InstTable, NName, 0xff00 | NCode, Proc);
  l = as_snprintf(SizeName, sizeof(SizeName), "%sB", NName);
  if (SizeMask & 1)
    AddInstTable(InstTable, SizeName, 0x0000 | NCode, Proc);
  if (SizeMask & 2)
  {
    SizeName[l - 1] = 'W';
    AddInstTable(InstTable, SizeName, 0x0100 | NCode, Proc);
  }

  /* CP(L) would generate conflict with CPL instruction - dispatch
     it from CPL single-op instruction if ArgCnt >= 2! */

  if ((SizeMask & 4) && (strcmp(NName, "CP")))
  {
    SizeName[l - 1] = 'L';
    AddInstTable(InstTable, SizeName, 0x0200 | NCode, Proc);
  }
}

static void AddMod(const char *NName, Byte NCode)
{
  int l;
  char SizeName[20];

  l = as_snprintf(SizeName, sizeof(SizeName), "%s1", NName);
  AddInstTable(InstTable, SizeName, NCode, DecodeMINC_MDEC);
  SizeName[l - 1] = '2';
  AddInstTable(InstTable, SizeName, NCode | 1, DecodeMINC_MDEC);
  SizeName[l - 1] = '4';
  AddInstTable(InstTable, SizeName, NCode | 2, DecodeMINC_MDEC);
}

static void AddFixed(const char *NName, Word NCode, Byte NFlag, Boolean NSup)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].CPUFlag = NFlag;
  FixedOrders[InstrZ].InSup = NSup;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddReg(const char *NName, Word NCode, Byte NMask)
{
  order_array_rsv_end(RegOrders, RegOrder);
  RegOrders[InstrZ].Code = NCode;
  RegOrders[InstrZ].OpMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg);
}

static void AddImm(const char *NName, Word NCode, Boolean NInSup,
                   Byte NMinMax, Byte NMaxMax, ShortInt NDefault)
{
  order_array_rsv_end(ImmOrders, ImmOrder);
  ImmOrders[InstrZ].Code = NCode;
  ImmOrders[InstrZ].InSup = NInSup;
  ImmOrders[InstrZ].MinMax = NMinMax;
  ImmOrders[InstrZ].MaxMax = NMaxMax;
  ImmOrders[InstrZ].Default = NDefault;
  AddInstTable(InstTable, NName, InstrZ++, DecodeImm);
}

static void AddALU2(const char *NName, Byte NCode)
{
  AddSize(NName, NCode, DecodeALU2, 7);
}

static void AddShift(const char *NName)
{
  AddSize(NName, InstrZ++, DecodeShift, 7);
}

static void AddMulDiv(const char *NName)
{
  AddInstTable(InstTable, NName, InstrZ++, DecodeMulDiv);
}

static void AddBitCF(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBitCF);
}

static void AddBit(const char *NName)
{
  AddInstTable(InstTable, NName, InstrZ++, DecodeBit);
}

static void AddCondition(const char *NName, Byte NCode)
{
  order_array_rsv_end(Conditions, Condition);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(301);
  SetDynamicInstTable(InstTable);

  add_null_pseudo(InstTable);

  AddInstTable(InstTable, "MULA"  , 0, DecodeMULA);
  AddInstTable(InstTable, "JP"    , 0, DecodeJPCALL);
  AddInstTable(InstTable, "CALL"  , 1, DecodeJPCALL);
  AddInstTable(InstTable, "JR"    , 0, DecodeJR);
  AddInstTable(InstTable, "JRL"   , 1, DecodeJR);
  AddInstTable(InstTable, "CALR"  , 0, DecodeCALR);
  AddInstTable(InstTable, "RET"   , 0, DecodeRET);
  AddInstTable(InstTable, "RETD"  , 0, DecodeRETD);
  AddInstTable(InstTable, "DJNZ"  , 0, DecodeDJNZ);
  AddInstTable(InstTable, "EX"    , 0, DecodeEX);
  AddInstTable(InstTable, "BS1F"  , 0, DecodeBS1x);
  AddInstTable(InstTable, "BS1B"  , 1, DecodeBS1x);
  AddInstTable(InstTable, "LDA"   , 0, DecodeLDA);
  AddInstTable(InstTable, "LDAR"  , 0, DecodeLDAR);
  AddInstTable(InstTable, "LDC"   , 0, DecodeLDC);
  AddInstTable(InstTable, "LDX"   , 0, DecodeLDX);
  AddInstTable(InstTable, "LINK"  , 0, DecodeLINK);
  AddSize("LD", 0, DecodeLD, 7);
  AddSize("PUSH", 0, DecodePUSH_POP, 7);
  AddSize("POP" , 1, DecodePUSH_POP, 7);
  AddSize("INC" , 0, DecodeINC_DEC, 7);
  AddSize("DEC" , 1, DecodeINC_DEC, 7);
  AddInstTable(InstTable, "CPI"  , 0x14, DecodeCPxx);
  AddInstTable(InstTable, "CPIR" , 0x15, DecodeCPxx);
  AddInstTable(InstTable, "CPD"  , 0x16, DecodeCPxx);
  AddInstTable(InstTable, "CPDR" , 0x17, DecodeCPxx);
  AddSize("LDI", 0x10 , DecodeLDxx, 3);
  AddSize("LDIR", 0x11, DecodeLDxx, 3);
  AddSize("LDD", 0x12 , DecodeLDxx, 3);
  AddSize("LDDR", 0x13, DecodeLDxx, 3);
  AddMod("MINC", 0x38);
  AddMod("MDEC", 0x3c);
  AddInstTable(InstTable, "RLD", 0x06, DecodeRLD_RRD);
  AddInstTable(InstTable, "RRD", 0x07, DecodeRLD_RRD);
  AddInstTable(InstTable, "SCC", 0, DecodeSCC);

  InstrZ = 0;
  AddFixed("CCF"   , 0x0012, 3, False);
  AddFixed("DECF"  , 0x000d, 3, False);
  AddFixed("DI"    , 0x0607, 3, True );
  AddFixed("HALT"  , 0x0005, 3, True );
  AddFixed("INCF"  , 0x000c, 3, False);
  AddFixed("MAX"   , 0x0004, 1, True );
  AddFixed("MIN"   , 0x0004, 2, True );
  AddFixed("NOP"   , 0x0000, 3, False);
  AddFixed("NORMAL", 0x0001, 1, True );
  AddFixed("RCF"   , 0x0010, 3, False);
  AddFixed("RETI"  , 0x0007, 3, True );
  AddFixed("SCF"   , 0x0011, 3, False);
  AddFixed("ZCF"   , 0x0013, 3, False);

  InstrZ = 0;
  AddReg("CPL" , 0xc006, 3);
  AddReg("DAA" , 0xc010, 1);
  AddReg("EXTS", 0xc013, 6);
  AddReg("EXTZ", 0xc012, 6);
  AddReg("MIRR", 0xc016, 2);
  AddReg("NEG" , 0xc007, 3);
  AddReg("PAA" , 0xc014, 6);
  AddReg("UNLK", 0xc00d, 4);

  InstrZ = 0;
  AddImm("EI"  , 0x0600, True,  7, 7,  0);
  AddImm("LDF" , 0x1700, False, 7, 3, -1);
  AddImm("SWI" , 0x00f8, False, 7, 7,  7);

  AddALU2("ADC", 1);
  AddALU2("ADD", 0);
  AddALU2("AND", 4);
  AddALU2("OR" , 6);
  AddALU2("SBC", 3);
  AddALU2("SUB", 2);
  AddALU2("XOR", 5);
  AddALU2("CP" , 7);

  InstrZ = 0;
  AddShift("RLC");
  AddShift("RRC");
  AddShift("RL");
  AddShift("RR");
  AddShift("SLA");
  AddShift("SRA");
  AddShift("SLL");
  AddShift("SRL");

  InstrZ = 0;
  AddMulDiv("MUL");
  AddMulDiv("MULS");
  AddMulDiv("DIV");
  AddMulDiv("DIVS");

  AddBitCF("ANDCF" , 0);
  AddBitCF("LDCF"  , 3);
  AddBitCF("ORCF"  , 1);
  AddBitCF("STCF"  , 4);
  AddBitCF("XORCF" , 2);

  InstrZ = 0;
  AddBit("RES");
  AddBit("SET");
  AddBit("CHG");
  AddBit("BIT");
  AddBit("TSET");

  InstrZ = 0;
  AddCondition("F"   ,  0); AddCondition("T"   , COND_CODE_TRUE);
  AddCondition("Z"   ,  6); AddCondition("NZ"  , 14);
  AddCondition("C"   ,  7); AddCondition("NC"  , 15);
  AddCondition("PL"  , 13); AddCondition("MI"  ,  5);
  AddCondition("P"   , 13); AddCondition("M"   ,  5);
  AddCondition("NE"  , 14); AddCondition("EQ"  ,  6);
  AddCondition("OV"  ,  4); AddCondition("NOV" , 12);
  AddCondition("PE"  ,  4); AddCondition("PO"  , 12);
  AddCondition("GE"  ,  9); AddCondition("LT"  ,  1);
  AddCondition("GT"  , 10); AddCondition("LE"  ,  2);
  AddCondition("UGE" , 15); AddCondition("ULT" ,  7);
  AddCondition("UGT" , 11); AddCondition("ULE" ,  3);
  AddCondition(NULL  ,  0);

  AddIntelPseudo(InstTable, eIntPseudoFlag_LittleEndian);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(RegOrders);
  order_array_free(ImmOrders);
  order_array_free(Conditions);
}

static void MakeCode_96C141(void)
{
  OpSize = eSymbolSizeUnknown;
  MinOneIs0 = False;

  /* vermischt */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean ChkPC_96C141(LargeWord Addr)
{
  Boolean ok;

  switch (ActPC)
  {
    case SegCode:
      if (MaxMode) ok = (Addr <= 0xffffff);
              else ok = (Addr <= 0xffff);
      break;
    default:
      ok = False;
  }
  return (ok);
}


static Boolean IsDef_96C141(void)
{
  return False;
}

static Boolean ChkMoreOneArg(void)
{
  return (ArgCnt > 1);
}

static void SwitchTo_96C141(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = ChkMoreOneArg;

  PCSymbol = "$";
  HeaderID = 0x52;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfffffful;

  MakeCode = MakeCode_96C141;
  ChkPC = ChkPC_96C141;
  IsDef = IsDef_96C141;
  SwitchFrom = DeinitFields;
  onoff_maxmode_add();
  onoff_supmode_add();

  InitFields();
}

void code96c141_init(void)
{
  CPU96C141 = AddCPU("96C141", SwitchTo_96C141);
  CPU93C141 = AddCPU("93C141", SwitchTo_96C141);
}
