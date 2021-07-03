/* codest9.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator SGS-Thomson ST9                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codest9.h"

typedef struct
{
  char *Name;
  Word Code;
} FixedOrder;


#define WorkOfs 0xd0

#define ModNone (-1)
#define ModReg 0
#define MModReg (1l << ModReg)                  /* Rn */
#define ModWReg 1
#define MModWReg (1l << ModWReg)                /* rn */
#define ModRReg 2
#define MModRReg (1l << ModRReg)                /* RRn */
#define ModWRReg 3
#define MModWRReg (1l << ModWRReg)              /* rrn */
#define ModIReg 4
#define MModIReg (1l << ModIReg)                /* (Rn) */
#define ModIWReg 5
#define MModIWReg (1l << ModIWReg)              /* (rn) */
#define ModIRReg 6
#define MModIRReg (1l << ModIRReg)              /* (RRn) */
#define ModIWRReg 7
#define MModIWRReg (1l << ModIWRReg)            /* (rrn) */
#define ModIncWReg 8
#define MModIncWReg (1l << ModIncWReg)          /* (rn)+ */
#define ModIncWRReg 9
#define MModIncWRReg (1l << ModIncWRReg)        /* (rrn)+ */
#define ModDecWRReg 10
#define MModDecWRReg (1l << ModDecWRReg)        /* -(rrn) */
#define ModDisp8WReg 11
#define MModDisp8WReg (1l << ModDisp8WReg)      /* d8(rn) */
#define ModDisp8WRReg 12
#define MModDisp8WRReg (1l << ModDisp8WRReg)    /* d8(rrn) */
#define ModDisp16WRReg 13
#define MModDisp16WRReg (1l << ModDisp16WRReg)  /* d16(rrn) */
#define ModDispRWRReg 14
#define MModDispRWRReg (1l << ModDispRWRReg)    /* rrm(rrn */
#define ModAbs 15
#define MModAbs (1l << ModAbs)                  /* NN */
#define ModImm 16
#define MModImm (1l << ModImm)                  /* #N/#NN */
#define ModDisp8RReg 17
#define MModDisp8RReg (1l << ModDisp8RReg)      /* d8(RRn) */
#define ModDisp16RReg 18
#define MModDisp16RReg (1l << ModDisp16RReg)    /* d16(RRn) */


static CPUVar CPUST9020,CPUST9030,CPUST9040,CPUST9050;

static ShortInt AdrMode,AbsSeg;
static Byte AdrPart,OpSize;
static Byte AdrVals[3];

static LongInt DPAssume;

#define ASSUMEST9Count 1
static ASSUMERec ASSUMEST9s[ASSUMEST9Count] =
{
  {"DP", &DPAssume, 0,  1, 0x0, NULL}
};

/*--------------------------------------------------------------------------*/
/* helper functions */

static Boolean DecodeReg(char *Asc_O, Byte *Erg, Byte *Size)
{
  Boolean Res;
  char *Asc;

  *Size = 0;
  Asc=Asc_O;

  if (strlen(Asc) < 2) return False;
  if (*Asc != 'r')
    return False;
  Asc++;
  if (*Asc == 'r')
  {
    if (strlen(Asc) < 2) return False;
    *Size = 1; Asc++;
  }
  else
    *Size = 0;

  *Erg = ConstLongInt(Asc, &Res, 10);
  if ((!Res) || (*Erg > 15)) return False;
  if ((*Size == 1) && (Odd(*Erg))) return False;

  return True;
}

static void DecodeAdr(tStrComp *pArg, LongWord Mask)
{
  Word AdrWord;
  int level;
  Byte flg,Size;
  Boolean OK, IsIndirect;
  tEvalResult EvalResult;
  char *p;
  int l;

  AdrMode = ModNone; AdrCnt = 0;

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case 0:
        AdrVals[0] = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        break;
      case 1:
        AdrWord = EvalStrIntExpressionOffs(pArg, 1, Int16, &OK);
        AdrVals[0] = Hi(AdrWord);
        AdrVals[1] = Lo(AdrWord);
        break;
    }
    if (OK)
    {
      AdrMode = ModImm;
      AdrCnt = OpSize + 1;
    }
    goto func_exit;
  }

  /* Arbeitsregister */

  if (DecodeReg(pArg->str.p_str, &AdrPart, &Size))
  {
    if (Size == 0)
    {
      if (Mask & MModWReg) AdrMode = ModWReg;
      else
      {
        AdrVals[0] = WorkOfs + AdrPart;
        AdrCnt = 1;
        AdrMode = ModReg;
      }
    }
    else
    {
      if (Mask & MModWRReg) AdrMode = ModWRReg;
      else
      {
        AdrVals[0] = WorkOfs + AdrPart;
        AdrCnt = 1;
        AdrMode = ModRReg;
      }
    }
    goto func_exit;
  }

  l = strlen(pArg->str.p_str);

  /* Postinkrement */

  if ((l > 0) && (pArg->str.p_str[l - 1] == '+'))
  {
    if ((l < 3) || (*pArg->str.p_str != '(') || (pArg->str.p_str[l - 2] != ')')) WrError(ErrNum_InvAddrMode);
    else
    {
      tStrComp RegComp;

      pArg->str.p_str[l - 2] = '\0'; pArg->Pos.Len -= 2;
      StrCompRefRight(&RegComp, pArg, 1);
      if (!DecodeReg(RegComp.str.p_str, &AdrPart, &Size)) WrStrErrorPos(ErrNum_InvReg, &RegComp);
      AdrMode = (Size == 0) ? ModIncWReg : ModIncWRReg;
    }
    goto func_exit;
  }

  /* Predekrement */

  if ((*pArg->str.p_str == '-') && (pArg->str.p_str[1] == '(') && (pArg->str.p_str[l - 1] == ')'))
  {
    tStrComp RegComp;

    pArg->str.p_str[l - 1] = '\0';  pArg->Pos.Len--;
    StrCompRefRight(&RegComp, pArg, 2);
    if (DecodeReg(RegComp.str.p_str, &AdrPart, &Size))
    {
      if (Size == 0) WrError(ErrNum_InvAddrMode); else AdrMode = ModDecWRReg;
      goto func_exit;
    }
  }

  /* indirekt<->direkt */

  if ((l < 3) || (pArg->str.p_str[l - 1] != ')'))
  {
    IsIndirect = False; p = pArg->str.p_str;
  }
  else
  {
    level = 0; p = pArg->str.p_str + l - 2; flg = 0;
    while ((p >= pArg->str.p_str) && (level >= 0))
    {
      switch (*p)
      {
        case '(': if (flg == 0) level--; break;
        case ')': if (flg == 0) level++; break;
        case '\'': if ((!(flg & 2))) flg ^= 1; break;
        case '"': if ((!(flg & 1))) flg ^= 2; break;
      }
      p--;
    }
    IsIndirect = (level == -1) && ((p < pArg->str.p_str) || ((*p == '.') || (*p == '_') || (isdigit(((unsigned int)*p) & 0xff)) || (isalpha(((unsigned int)*p) & 0xff))));
  }

  /* indirekt */

  if (IsIndirect)
  {
    tStrComp RegComp;

    /* discard closing ) at end */

    pArg->str.p_str[--l] = '\0'; pArg->Pos.Len--;

    /* split off part in () */

    StrCompRefRight(&RegComp, pArg, p + 2 - pArg->str.p_str);

    /* truncate arg to everything before ( */

    p[1] = '\0';
    pArg->Pos.Len = p + 1 - pArg->str.p_str;
    if (DecodeReg(RegComp.str.p_str, &AdrPart, &Size))
    {
      if (Size == 0)   /* d(r) */
      {
        AdrVals[0] = EvalStrIntExpression(pArg, Int8, &OK);
        if (OK)
        {
          if ((Mask & MModIWReg) && (AdrVals[0] == 0)) AdrMode = ModIWReg;
          else if (((Mask & MModIReg) != 0) && (AdrVals[0] == 0))
          {
            AdrVals[0] = WorkOfs + AdrPart; AdrMode = ModIReg; AdrCnt = 1;
          }
          else
          {
            AdrMode = ModDisp8WReg; AdrCnt = 1;
          }
        }
      }
      else            /* ...(rr) */
      {
        if (DecodeReg(pArg->str.p_str, AdrVals, &Size))
        {             /* rr(rr) */
          if (Size != 1) WrError(ErrNum_InvAddrMode);
          else
          {
            AdrMode = ModDispRWRReg; AdrCnt = 1;
          }
        }
        else
        {             /* d(rr) */
          AdrWord = EvalStrIntExpression(pArg, Int16, &OK);
          if ((AdrWord == 0) && (Mask & (MModIRReg | MModIWRReg)))
          {
            if (Mask & MModIWRReg) AdrMode = ModIWRReg;
            else
            {
              AdrMode = ModIRReg; AdrVals[0] = AdrPart + WorkOfs; AdrCnt = 1;
            }
          }
          else if ((AdrWord < 0x100) && (Mask & (MModDisp8WRReg | MModDisp8RReg)))
          {
            if (Mask & MModDisp8WRReg)
            {
              AdrVals[0] = Lo(AdrWord); AdrCnt = 1; AdrMode = ModDisp8WRReg;
            }
            else
            {
              AdrVals[0] = AdrPart + WorkOfs;
              AdrVals[1] = Lo(AdrWord);
              AdrCnt = 2;
              AdrMode = ModDisp8RReg;
            }
          }
          else if (Mask & MModDisp16WRReg)
          {
            AdrVals[0] = Hi(AdrWord);
            AdrVals[1] = Lo(AdrWord);
            AdrCnt = 2;
            AdrMode = ModDisp16WRReg;
          }
          else
          {
            AdrVals[0] = AdrPart + WorkOfs;
            AdrVals[2] = Hi(AdrWord);
            AdrVals[1] = Lo(AdrWord);
            AdrCnt = 3;
            AdrMode = ModDisp16RReg;
          }
        }
      }
    }
    else             /* ...(RR) */
    {
      AdrWord = EvalStrIntExpressionWithResult(&RegComp, UInt9, &EvalResult);
      if (!(EvalResult.AddrSpaceMask & (1 << SegReg)))  WrError(ErrNum_InvAddrMode);
      else if (AdrWord < 0xff)
      {
        AdrVals[0] = Lo(AdrWord);
        AdrWord = EvalStrIntExpression(pArg, Int8, &OK);
        if (AdrWord != 0) WrError(ErrNum_OverRange);
        else
        {
          AdrCnt = 1; AdrMode = ModIReg;
        }
      }
      else if ((AdrWord > 0x1ff) || (Odd(AdrWord))) WrError(ErrNum_InvAddrMode);
      else
      {
        AdrVals[0] = Lo(AdrWord);
        AdrWord = EvalStrIntExpression(pArg, Int16, &OK);
        if ((AdrWord == 0) && (Mask & MModIRReg))
        {
          AdrCnt = 1; AdrMode = ModIRReg;
        }
        else if ((AdrWord < 0x100) && (Mask & MModDisp8RReg))
        {
          AdrVals[1] = Lo(AdrWord); AdrCnt = 2; AdrMode = ModDisp8RReg;
        }
        else
        {
          AdrVals[2] = Hi(AdrWord);
          AdrVals[1] = Lo(AdrWord);
          AdrCnt = 3;
          AdrMode = ModDisp16RReg;
        }
      }
    }
    goto func_exit;
  }

  /* direkt */

  AdrWord = EvalStrIntExpressionWithResult(pArg, UInt16, &EvalResult);
  if (EvalResult.OK)
  {
    if (!(EvalResult.AddrSpaceMask & (1 << SegReg)))
    {
      AdrMode = ModAbs;
      AdrVals[0] = Hi(AdrWord);
      AdrVals[1] = Lo(AdrWord);
      AdrCnt = 2;
      ChkSpace(AbsSeg, EvalResult.AddrSpaceMask);
    }
    else if (AdrWord < 0xff)
    {
      AdrMode = ModReg;
      AdrVals[0] = Lo(AdrWord);
      AdrCnt = 1;
    }
    else if ((AdrWord > 0x1ff) || (Odd(AdrWord))) WrError(ErrNum_InvAddrMode);
    else
    {
      AdrMode = ModRReg;
      AdrVals[0] = Lo(AdrWord);
      AdrCnt = 1;
    }
  }

func_exit:
  if ((AdrMode != ModNone) && (((1l << AdrMode) & Mask) == 0))
  {
    WrError(ErrNum_InvAddrMode); AdrMode = ModNone; AdrCnt = 0;
  }
}

static Boolean SplitBit(tStrComp *pDest, const tStrComp *pSrc, Byte *pErg)
{
  char *p;
  Boolean OK;
  Byte Inv = 0;

  StrCompRefRight(pDest, pSrc, 0);
  p = RQuotPos(pDest->str.p_str, '.');
  if ((!p) || (p == pDest->str.p_str + strlen(pDest->str.p_str) + 1))
  {
    Integer val;

    if (*pDest->str.p_str == '!')
    {
      Inv = 1;
      pDest->str.p_str++;
      pDest->Pos.StartCol++;
      pDest->Pos.Len--;
    }
    val = EvalStrIntExpression(pDest, UInt8, &OK);
    if (OK)
    {
      *pErg = (val & 15) ^ Inv;
      as_snprintf(pDest->str.p_str, STRINGSIZE, "r%d", (int)(val >> 4));
      return True;
    }
    return False;
  }

  Inv = !!(p[1] == '!');
  *pErg = Inv + (EvalStrIntExpressionOffs(pSrc, p + 1 + Inv - pSrc->str.p_str, UInt3, &OK) << 1);
  *p = '\0';
  return OK;
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    if (Hi(Code))
      BAsmCode[CodeLen++] = Hi(Code);
    BAsmCode[CodeLen++] = Lo(Code);
  }
}

static void DecodeLD(Word IsLDW)
{
  Byte HReg, HPart;
  Word Mask1, Mask2;

  if (ChkArgCnt(2, 2))
  {
    if (IsLDW)
    {
      OpSize = 1; Mask1 = MModWRReg; Mask2 = MModRReg;
    }
    else
    {
      Mask1 = MModWReg; Mask2 = MModReg;
    }
    DecodeAdr(&ArgStr[1], Mask1 | Mask2 | MModIWReg | MModDisp8WReg | MModIncWReg |
                         MModIWRReg | MModIncWRReg | MModDecWRReg | MModDisp8WRReg |
                         MModDisp16WRReg | MModDispRWRReg | MModAbs | MModIRReg);
    switch (AdrMode)
    {
      case ModWReg:
      case ModWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask1 | Mask2 | MModIWReg | MModDisp8WReg | MModIWRReg |
                             MModIncWRReg | MModDecWRReg | MModDispRWRReg | MModDisp8WRReg |
                             MModDisp16WRReg | MModAbs | MModImm);
        switch (AdrMode)
        {
          case ModWReg:
            BAsmCode[0] = (HReg << 4) + 8;
            BAsmCode[1] = WorkOfs + AdrPart;
            CodeLen = 2;
            break;
          case ModWRReg:
            BAsmCode[0] = 0xe3;
            BAsmCode[1] = (HReg << 4) + AdrPart;
            CodeLen = 2;
            break;
          case ModReg:
            BAsmCode[0] = (HReg << 4) + 8;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModRReg:
            BAsmCode[0] = 0xef;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = HReg + WorkOfs;
            CodeLen = 3;
            break;
          case ModIWReg:
            if (OpSize == 0)
            {
              BAsmCode[0] = 0xe4;
              BAsmCode[1] = (HReg << 4) + AdrPart;
              CodeLen = 2;
            }
            else
            {
              BAsmCode[0] = 0xa6;
              BAsmCode[1] = 0xf0 + AdrPart;
              BAsmCode[2] = WorkOfs + HReg;
              CodeLen = 3;
            }
            break;
          case ModDisp8WReg:
            BAsmCode[0] = 0xb3 + (OpSize * 0x2b);
            BAsmCode[1] = (HReg << 4) + AdrPart;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModIWRReg:
            BAsmCode[0] = 0xb5 + (OpSize * 0x2e);
            BAsmCode[1] = (HReg << 4) + AdrPart + OpSize;
            CodeLen = 2;
            break;
          case ModIncWRReg:
            BAsmCode[0] = 0xb4 + (OpSize * 0x21);
            BAsmCode[1] = 0xf1 + AdrPart;
            BAsmCode[2] = WorkOfs + HReg;
            CodeLen = 3;
            break;
          case ModDecWRReg:
            BAsmCode[0] = 0xc2 + OpSize;
            BAsmCode[1] = 0xf1 + AdrPart;
            BAsmCode[2] = WorkOfs + HReg;
            CodeLen = 3;
            break;
          case ModDispRWRReg:
            BAsmCode[0] = 0x60;
            BAsmCode[1] = (0x10 * (1 - OpSize)) + (AdrVals[0] << 4) + AdrPart;
            BAsmCode[2] = 0xf0 + HReg;
            CodeLen = 3;
            break;
          case ModDisp8WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = 0xf1 + AdrPart;
            BAsmCode[2] = AdrVals[0];
            BAsmCode[3] = HReg + WorkOfs;
            CodeLen = 4;
            break;
          case ModDisp16WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = 0xf0 + AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            BAsmCode[2 + AdrCnt] = HReg + WorkOfs;
            CodeLen = 3 + AdrCnt;
            break;
          case ModAbs:
            BAsmCode[0] = 0xc4 + (OpSize * 0x1e);
            BAsmCode[1] = 0xf0 + AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
          case ModImm:
            if (OpSize == 0)
            {
              BAsmCode[0] = (HReg << 4) + 0x0c;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              CodeLen = 1 + AdrCnt;
            }
            else
            {
              BAsmCode[0] = 0xbf;
              BAsmCode[1] = WorkOfs + HReg;
              memcpy(BAsmCode + 2, AdrVals, AdrCnt);
              CodeLen = 2 + AdrCnt;
            }
            break;
        }
        break;
      case ModReg:
      case ModRReg:
        HReg = AdrVals[0];
        DecodeAdr(&ArgStr[2], Mask1 | Mask2 | MModIWReg | MModIWRReg | MModIncWRReg |
                             MModDecWRReg | MModDispRWRReg | MModDisp8WRReg | MModDisp16WRReg |
                             MModImm);
        switch (AdrMode)
        {
          case ModWReg:
            BAsmCode[0] = (AdrPart << 4) + 0x09;
            BAsmCode[1] = HReg;
            CodeLen = 2;
            break;
          case ModWRReg:
            BAsmCode[0] = 0xef;
            BAsmCode[1] = WorkOfs + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0xf4 - (OpSize * 5);
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModIWReg:
            BAsmCode[0] = 0xe7 - (0x41 * OpSize);
            BAsmCode[1] = 0xf0 + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModIWRReg:
            BAsmCode[0] = 0x72 + (OpSize * 12);
            BAsmCode[1] = 0xf1 + AdrPart - OpSize;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModIncWRReg:
            BAsmCode[0] = 0xb4 + (0x21 * OpSize);
            BAsmCode[1] = 0xf1 + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModDecWRReg:
            BAsmCode[0] = 0xc2 + OpSize;
            BAsmCode[1] = 0xf1 + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModDisp8WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = 0xf1 + AdrPart;
            BAsmCode[2] = AdrVals[0];
            BAsmCode[3] = HReg;
            CodeLen = 4;
            break;
          case ModDisp16WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = 0xf0 + AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            BAsmCode[2 + AdrCnt] = HReg;
            CodeLen = 3 + AdrCnt;
            break;
          case ModImm:
            BAsmCode[0] = 0xf5 - (OpSize * 0x36);
            BAsmCode[1] = HReg;
            memcpy(BAsmCode + 2, AdrVals, 2);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      case ModIWReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2 | Mask1);
        switch (AdrMode)
        {
          case ModWReg:
            BAsmCode[0] = 0xe5;
            BAsmCode[1] = (HReg << 4) + AdrPart;
            CodeLen = 2;
            break;
          case ModWRReg:
            BAsmCode[0] = 0x96;
            BAsmCode[1] = WorkOfs + AdrPart;
            BAsmCode[2] = 0xf0 + HReg;
            CodeLen = 3;
            break;
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0xe6 - (0x50 * OpSize);
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = 0xf0 + HReg;
            CodeLen = 3;
            break;
        }
        break;
      case ModDisp8WReg:
        BAsmCode[2] = AdrVals[0]; HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask1);
        switch (AdrMode)
        {
          case ModWReg:
          case ModWRReg:
            BAsmCode[0] = 0xb2 + (OpSize * 0x2c);
            BAsmCode[1] = (AdrPart << 4) + (OpSize << 4) + HReg;
            CodeLen = 3;
            break;
        }
        break;
      case ModIncWReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModIncWRReg * (1 - OpSize));
        switch (AdrMode)
        {
          case ModIncWRReg:
            BAsmCode[0] = 0xd7;
            BAsmCode[1] = (HReg << 4) + AdrPart + 1;
            CodeLen = 2;
            break;
        }
        break;
      case ModIWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], (MModIWReg * (1 - OpSize)) | Mask1 | Mask2 | MModIWRReg | MModImm);
        switch (AdrMode)
        {
          case ModIWReg:
            BAsmCode[0] = 0xb5;
            BAsmCode[1] = (AdrPart << 4) + HReg + 1;
            CodeLen = 2;
            break;
          case ModWReg:
            BAsmCode[0] = 0x72;
            BAsmCode[1] = 0xf0 + HReg;
            BAsmCode[2] = AdrPart + WorkOfs;
            CodeLen = 3;
            break;
          case ModWRReg:
            BAsmCode[0] = 0xe3;
            BAsmCode[1] = (HReg << 4) + 0x10 + AdrPart;
            CodeLen = 2;
            break;
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0x72 + (OpSize * 0x4c);
            BAsmCode[1] = 0xf0 + HReg + OpSize;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModIWRReg:
            if (OpSize == 0)
            {
              BAsmCode[0] = 0x73;
              BAsmCode[1] = 0xf0 + AdrPart;
              BAsmCode[2] = WorkOfs + HReg;
              CodeLen = 3;
            }
            else
            {
              BAsmCode[0] = 0xe3;
              BAsmCode[1] = 0x11 + (HReg << 4) + AdrPart;
              CodeLen = 2;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0xf3 - (OpSize * 0x35);
            BAsmCode[1] = 0xf0 + HReg;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      case ModIncWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2 | (MModIncWReg * (1 - OpSize)));
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0xb4 + (OpSize * 0x21);
            BAsmCode[1] = 0xf0 + HReg;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModIncWReg:
            BAsmCode[0] = 0xd7;
            BAsmCode[1] = (AdrPart << 4) + HReg;
            CodeLen = 2;
            break;
        }
        break;
      case ModDecWRReg:
         HReg = AdrPart;
         DecodeAdr(&ArgStr[2], Mask2);
         switch (AdrMode)
         {
           case ModReg:
           case ModRReg:
             BAsmCode[0] = 0xc2 + OpSize;
             BAsmCode[1] = 0xf0 + HReg;
             BAsmCode[2] = AdrVals[0];
             CodeLen = 3;
             break;
         }
         break;
      case ModDispRWRReg:
        HReg = AdrPart; HPart = AdrVals[0];
        DecodeAdr(&ArgStr[2], Mask1);
        switch (AdrMode)
        {
          case ModWReg:
          case ModWRReg:
            BAsmCode[0] = 0x60;
            BAsmCode[1] = (0x10 * (1 - OpSize)) + 0x01 + (HPart << 4) + HReg;
            BAsmCode[2] = 0xf0 + AdrPart;
            CodeLen = 3;
            break;
        }
        break;
      case ModDisp8WRReg:
        BAsmCode[2] = AdrVals[0]; HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2 | (OpSize * MModImm));
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0x26 + (OpSize * 0x60);
            BAsmCode[1] = 0xf1 + HReg;
            BAsmCode[3] = AdrVals[0];
            CodeLen = 4;
            break;
          case ModImm:
            BAsmCode[0] = 0x06;
            BAsmCode[1] = 0xf1 + HReg;
            memcpy(BAsmCode + 3, AdrVals, AdrCnt);
            CodeLen = 3 + AdrCnt;
            break;
        }
        break;
      case ModDisp16WRReg:
        memcpy(BAsmCode + 2, AdrVals, 2); HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2 | (OpSize * MModImm));
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0x26 + (OpSize * 0x60);
            BAsmCode[1] = 0xf0 + HReg;
            BAsmCode[4] = AdrVals[0];
            CodeLen = 5;
            break;
          case ModImm:
            BAsmCode[0] = 0x06;
            BAsmCode[1] = 0xf0 + HReg;
            memcpy(BAsmCode + 4, AdrVals, AdrCnt);
            CodeLen =4 + AdrCnt;
            break;
        }
        break;
      case ModAbs:
        memcpy(BAsmCode + 2, AdrVals, 2);
        DecodeAdr(&ArgStr[2], Mask1 | MModImm);
        switch (AdrMode)
        {
          case ModWReg:
          case ModWRReg:
            BAsmCode[0] = 0xc5 + (OpSize * 0x1d);
            BAsmCode[1] = 0xf0 + AdrPart + OpSize;
            CodeLen = 4;
            break;
          case ModImm:
            memmove(BAsmCode + 2 + AdrCnt, BAsmCode + 2, 2);
            BAsmCode[0] = 0x2f + (OpSize * 7);
            BAsmCode[1] = 0xf1;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 4 + AdrCnt;
            break;
        }
        break;
      case ModIRReg:
        HReg = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModIWRReg * (1 - OpSize));
        switch (AdrMode)
        {
          case ModIWRReg:
            BAsmCode[0] = 0x73;
            BAsmCode[1] = 0xf0 + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeLoad(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModIncWRReg);
    if (AdrMode == ModIncWRReg)
    {
      HReg = AdrPart << 4;
      DecodeAdr(&ArgStr[2], MModIncWRReg);
      if (AdrMode == ModIncWRReg)
      {
        BAsmCode[0] = 0xd6;
        BAsmCode[1] = Code + HReg + AdrPart;
        CodeLen = 2;
      }
    }
  }
}

static void DecodePEA_PEAU(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModDisp8RReg | MModDisp16RReg);
    if (AdrMode != ModNone)
    {
      BAsmCode[0] = 0x8f;
      BAsmCode[1] = Code;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      BAsmCode[2] += AdrCnt - 2;
      CodeLen =2 + AdrCnt;
    }
  }
}

static void DecodePUSH_PUSHU(Word IsU)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModImm);
    switch (AdrMode)
    {
      case ModReg:
        BAsmCode[0] = 0x66 - (IsU * 0x36);
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
      case ModIReg:
        BAsmCode[0] = 0xf7 - (IsU * 0xc6);
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
      case ModImm:
        BAsmCode[0] = 0x8f;
        BAsmCode[1] = 0xf1 + (IsU * 2);
        BAsmCode[2] = AdrVals[0];
        CodeLen = 3;
        break;
    }
  }
}

static void DecodePUSHW_PUSHUW(Word IsU)
{
  OpSize = 1;
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModRReg | MModImm);
    switch (AdrMode)
    {
      case ModRReg:
        BAsmCode[0] = 0x74 + (IsU * 0x42);
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
      case ModImm:
        BAsmCode[0] = 0x8f;
        BAsmCode[1] = 0xc1 + (IsU * 2);
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
    }
  }
}

static void DecodeXCH(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      BAsmCode[2] = AdrVals[0];
      DecodeAdr(&ArgStr[2], MModReg);
      if (AdrMode == ModReg)
      {
        BAsmCode[1] = AdrVals[0];
        BAsmCode[0] = 0x16;
        CodeLen = 3;
      }
    }
  }
}

static void DecodeALU(Word Code)
{
  Word Mask1, Mask2;
  Byte HReg, HPart;
  Byte CodeMask = Lo(Code) << 4;

  if (ChkArgCnt(2, 2))
  {
    if (Hi(Code))
    {
      OpSize = 1; Mask1 = MModWRReg; Mask2 = MModRReg;
    }
    else
    {
      Mask1 = MModWReg; Mask2 = MModReg;
    }
    DecodeAdr(&ArgStr[1], Mask1 | Mask2 | MModIWReg | MModIWRReg | MModIncWRReg |
                         MModDecWRReg | MModDispRWRReg | MModDisp8WRReg | MModDisp16WRReg |
                         MModAbs | MModIRReg);
    switch (AdrMode)
    {
      case ModWReg:
      case ModWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask1 | MModIWReg | Mask2 | MModIWRReg | MModIncWRReg |
                             MModDecWRReg | MModDispRWRReg | MModDisp8WRReg | MModDisp16WRReg |
                             MModAbs | MModImm);
        switch (AdrMode)
        {
          case ModWReg:
          case ModWRReg:
            BAsmCode[0] = CodeMask + 2 + (OpSize * 12);
            BAsmCode[1] = (HReg << 4) + AdrPart;
            CodeLen = 2;
            break;
          case ModIWReg:
            if (OpSize == 0)
            {
              BAsmCode[0] = CodeMask + 3;
              BAsmCode[1] = (HReg << 4) + AdrPart;
              CodeLen = 2;
            }
            else
            {
              BAsmCode[0] = 0xa6;
              BAsmCode[1] = CodeMask + AdrPart;
              BAsmCode[2] = WorkOfs + HReg;
              CodeLen = 3;
            }
            break;
          case ModReg:
          case ModRReg:
            BAsmCode[0] = CodeMask + 4 + (OpSize * 3);
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = HReg + WorkOfs;
            CodeLen = 3;
            break;
          case ModIWRReg:
            if (OpSize == 0)
            {
              BAsmCode[0] = 0x72;
              BAsmCode[1] = CodeMask + AdrPart + 1;
              BAsmCode[2] = HReg + WorkOfs;
              CodeLen = 3;
            }
            else
            {
              BAsmCode[0] = CodeMask + 0x0e;
              BAsmCode[1] = (HReg << 4) + AdrPart + 1;
              CodeLen = 2;
            }
            break;
          case ModIncWRReg:
            BAsmCode[0] = 0xb4 + (OpSize * 0x21);
            BAsmCode[1] = CodeMask + AdrPart + 1;
            BAsmCode[2] = HReg + WorkOfs;
            CodeLen = 3;
            break;
          case ModDecWRReg:
            BAsmCode[0] = 0xc2 + OpSize;
            BAsmCode[1] = CodeMask + AdrPart + 1;
            BAsmCode[2] = HReg + WorkOfs;
            CodeLen = 3;
            break;
          case ModDispRWRReg:
            BAsmCode[0] = 0x60;
            BAsmCode[1] = 0x10 * (1 - OpSize) + (AdrVals[0] << 4) + AdrPart;
            BAsmCode[2] = CodeMask + HReg;
            CodeLen = 3;
            break;
          case ModDisp8WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = CodeMask + AdrPart + 1;
            BAsmCode[2] = AdrVals[0];
            BAsmCode[3] = WorkOfs + HReg;
            CodeLen = 4;
            break;
          case ModDisp16WRReg:
            BAsmCode[0]= 0x7f + (OpSize * 7);
            BAsmCode[1] = CodeMask + AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            BAsmCode[2 + AdrCnt] = WorkOfs + HReg;
            CodeLen = 3 + AdrCnt;
            break;
          case ModAbs:
            BAsmCode[0] = 0xc4 + (OpSize * 30);
            BAsmCode[1] = CodeMask + HReg;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
          case ModImm:
            BAsmCode[0] = CodeMask + 5 + (OpSize * 2);
            BAsmCode[1] = WorkOfs + HReg + OpSize;
            memcpy(BAsmCode + 2,AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      case ModReg:
      case ModRReg:
        HReg = AdrVals[0];
        DecodeAdr(&ArgStr[2], Mask2 | MModIWReg | MModIWRReg | MModIncWRReg | MModDecWRReg |
                             MModDisp8WRReg | MModDisp16WRReg | MModImm);
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = CodeMask + 4 + (OpSize * 3);
            CodeLen = 3;
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = HReg;
            break;
          case ModIWReg:
            BAsmCode[0] = 0xa6 + 65 * (1 - OpSize);
            BAsmCode[1] = CodeMask + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen=3;
            break;
          case ModIWRReg:
            BAsmCode[0] = 0x72 + (OpSize * 12);
            BAsmCode[1] = CodeMask + AdrPart + (1 - OpSize);
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModIncWRReg:
            BAsmCode[0] = 0xb4 + (OpSize * 0x21);
            BAsmCode[1] = CodeMask + AdrPart + 1;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModDecWRReg:
            BAsmCode[0] = 0xc2 + OpSize;
            BAsmCode[1] = CodeMask + AdrPart + 1;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
          case ModDisp8WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = CodeMask + AdrPart + 1;
            BAsmCode[2] = AdrVals[0];
            BAsmCode[3] = HReg;
            CodeLen = 4;
            break;
          case ModDisp16WRReg:
            BAsmCode[0] = 0x7f + (OpSize * 7);
            BAsmCode[1] = CodeMask + AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            BAsmCode[2 + AdrCnt] = HReg;
            CodeLen = 3 + AdrCnt;
            break;
          case ModImm:
            BAsmCode[0] = CodeMask + 5 + (OpSize * 2);
            memcpy(BAsmCode + 2, AdrVals, AdrCnt); BAsmCode[1] = HReg + OpSize;
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      case ModIWReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2);
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0xe6 - (OpSize * 0x50);
            BAsmCode[1] = AdrVals[0];
            BAsmCode[2] = CodeMask + HReg;
            CodeLen = 3;
            break;
        }
        break;
      case ModIWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], (OpSize * MModWRReg) | Mask2 | MModIWRReg | MModImm);
        switch (AdrMode)
        {
          case ModWRReg:
            BAsmCode[0] = CodeMask + 0x0e;
            BAsmCode[1] = (HReg << 4) + 0x10 + AdrPart;
            CodeLen = 2;
            break;
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0x72 + (OpSize * 76);
            BAsmCode[1] = CodeMask + HReg + OpSize;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModIWRReg:
            if (OpSize == 0)
            {
              BAsmCode[0] = 0x73;
              BAsmCode[1] = CodeMask + AdrPart;
              BAsmCode[2] = HReg + WorkOfs;
              CodeLen = 3;
            }
            else
            {
              BAsmCode[0] = CodeMask + 0x0e;
              BAsmCode[1] = 0x11 + (HReg << 4) + AdrPart;
              CodeLen = 2;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0xf3 - (OpSize * 0x35);
            BAsmCode[1] = CodeMask + HReg;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      case ModIncWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2);
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0xb4 + (OpSize * 0x21);
            BAsmCode[1] = CodeMask + HReg;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
      case ModDecWRReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], Mask2);
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0xc2 + OpSize;
            BAsmCode[1] = CodeMask + HReg;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
      case ModDispRWRReg:
        HReg = AdrPart; HPart = AdrVals[0];
        DecodeAdr(&ArgStr[2], Mask1);
        switch (AdrMode)
        {
          case ModWReg:
          case ModWRReg:
            BAsmCode[0] = 0x60;
            BAsmCode[1] = 0x11 - (OpSize * 0x10) + (HPart << 4) + HReg;
            BAsmCode[2] = CodeMask + AdrPart;
            CodeLen = 3;
            break;
        }
        break;
      case ModDisp8WRReg:
        HReg = AdrPart;
        BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[2], Mask2 + (OpSize * MModImm));
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0x26 + (OpSize * 0x60);
            BAsmCode[1] = CodeMask + HReg + 1;
            BAsmCode[3] = AdrVals[0] + OpSize;
            CodeLen = 4;
            break;
          case ModImm:
            BAsmCode[0] = 0x06;
            BAsmCode[1] = CodeMask + HReg + 1;
            memcpy(BAsmCode + 3, AdrVals, AdrCnt);
            CodeLen = 3 + AdrCnt;
            break;
        }
        break;
      case ModDisp16WRReg:
        HReg = AdrPart; memcpy(BAsmCode + 2, AdrVals, 2);
        DecodeAdr(&ArgStr[2], Mask2 | (OpSize * MModImm));
        switch (AdrMode)
        {
          case ModReg:
          case ModRReg:
            BAsmCode[0] = 0x26 + (OpSize * 0x60);
            BAsmCode[1] = CodeMask + HReg;
            BAsmCode[4] = AdrVals[0] + OpSize;
            CodeLen = 5;
            break;
          case ModImm:
            BAsmCode[0] = 0x06;
            BAsmCode[1] = CodeMask + HReg;
            memcpy(BAsmCode + 4, AdrVals, AdrCnt);
            CodeLen = 4 + AdrCnt;
            break;
        }
        break;
      case ModAbs:
        memcpy(BAsmCode + 2, AdrVals, 2);
        DecodeAdr(&ArgStr[2], Mask1 | MModImm);
        switch (AdrMode)
        {
          case ModWReg:
          case ModWRReg:
            BAsmCode[0] = 0xc5 + (OpSize * 29);
            BAsmCode[1] = CodeMask + AdrPart + OpSize;
            CodeLen = 4;
            break;
          case ModImm:
            memmove(BAsmCode + 2 + AdrCnt, BAsmCode + 2, 2);
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            BAsmCode[0] = 0x2f + (OpSize * 7);
            BAsmCode[1] = CodeMask + 1;
            CodeLen = 4 + AdrCnt;
            break;
        }
        break;
      case ModIRReg:
        HReg = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModIWRReg *(1 - OpSize));
        switch (AdrMode)
        {
          case ModIWRReg:
            BAsmCode[0] = 0x73;
            BAsmCode[1] = CodeMask + AdrPart;
            BAsmCode[2] = HReg;
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeReg8(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModIReg);
    switch (AdrMode)
    {
      case ModReg:
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
      case ModIReg:
        BAsmCode[0] = Code + 1;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeReg16(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModRReg);
    switch (AdrMode)
    {
      case ModRReg:
        BAsmCode[0] = Lo(Code);
        BAsmCode[1] = AdrVals[0] + Hi(Code);
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeDIV_MUL(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModWRReg);
    if (AdrMode == ModWRReg)
    {
      HReg = AdrPart;
      DecodeAdr(&ArgStr[2], MModWReg);
      if (AdrMode == ModWReg)
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = (HReg << 4) + AdrPart;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeDIVWS(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[1], MModWRReg);
    if (AdrMode == ModWRReg)
    {
      HReg = AdrPart;
      DecodeAdr(&ArgStr[2], MModWRReg);
      if (AdrMode == ModWRReg)
      {
        BAsmCode[2] = (HReg << 4) + AdrPart;
        DecodeAdr(&ArgStr[3], MModRReg);
        if (AdrMode == ModRReg)
        {
          BAsmCode[0] = 0x56;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 3;
        }
      }
    }
  }
}

static void DecodeBit2(Word Code)
{
  Byte HReg, HPart;
  tStrComp Comp;

  if (ChkArgCnt(2, 2) && SplitBit(&Comp, &ArgStr[1], &HReg))
  {
    if (Odd(HReg)) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(&Comp, MModWReg);
      if (AdrMode == ModWReg)
      {
        HReg = (HReg << 4) + AdrPart;
        if (SplitBit(&Comp, &ArgStr[2], &HPart))
        {
          DecodeAdr(&Comp, MModWReg);
          if (AdrMode == ModWReg)
          {
            HPart = (HPart << 4) + AdrPart;
            BAsmCode[0] = Code;
            switch (Code)
            {
              case 0xf2: /* BLD */
                BAsmCode[1] = HPart | 0x10;
                BAsmCode[2] = HReg | (HPart & 0x10);
                break;
              case 0x6f: /* BXOR */
                BAsmCode[1] = 0x10 + HReg;
                BAsmCode[2] = HPart;
                break;
              default:
                BAsmCode[1] = 0x10 + HReg;
                BAsmCode[2] = HPart ^ 0x10;
            }
            CodeLen = 3;
          }
        }
      }
    }
  }
}

static void DecodeBit1(Word Code)
{
  Byte HReg;
  tStrComp Comp;

  if (ChkArgCnt(1, 1) && SplitBit(&Comp, &ArgStr[1], &HReg))
  {
    if (Odd(HReg)) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(&Comp, MModWReg + (Hi(Code) * MModIWRReg));
      switch (AdrMode)
      {
        case ModWReg:
          BAsmCode[0] = Lo(Code);
          BAsmCode[1] = (HReg << 4) + AdrPart;
          CodeLen = 2;
          break;
        case ModIWRReg:
          BAsmCode[0] = 0xf6;
          BAsmCode[1] = (HReg << 4) + AdrPart;
          CodeLen = 2;
          break;
      }
    }
  }
}

static void DecodeBTJF_BTJT(Word Code)
{
  Byte HReg;
  Integer AdrInt;
  tStrComp Comp;

  if (ChkArgCnt(2, 2) && SplitBit(&Comp, &ArgStr[1], &HReg))
  {
    if (Odd(HReg)) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(&Comp, MModWReg);
      if (AdrMode == ModWReg)
      {
        tEvalResult EvalResult;

        BAsmCode[1] = (HReg << 4) + AdrPart + Code;
        AdrInt = EvalStrIntExpressionWithResult(&ArgStr[2], UInt16, &EvalResult) - (EProgCounter() + 3);
        if (EvalResult.OK)
        {
          if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[0] = 0xaf;
            BAsmCode[2] = AdrInt & 0xff;
            CodeLen = 3;
            ChkSpace(SegCode, EvalResult.AddrSpaceMask);
          }
        }
      }
    }
  }
}

static void DecodeJP_CALL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    AbsSeg = SegCode;
    DecodeAdr(&ArgStr[1], MModIRReg | MModAbs);
    switch (AdrMode)
    {
      case ModIRReg:
        BAsmCode[0] = Hi(Code);
        BAsmCode[1] = AdrVals[0] + Ord(Memo("CALL"));
        CodeLen = 2;
        break;
      case ModAbs:
        BAsmCode[0] = Lo(Code);
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen =1 + AdrCnt;
        break;
    }
  }
}

static void DecodeCPJFI_CPJTI(Word Code)
{
  Byte HReg;
  Integer AdrInt;

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[1], MModWReg);
    if (AdrMode == ModWReg)
    {
      HReg = AdrPart;
      DecodeAdr(&ArgStr[2], MModIWRReg);
      if (AdrMode == ModIWRReg)
      {
        tEvalResult EvalResult;

        BAsmCode[1] = (AdrPart << 4) + Code + HReg;
        AdrInt = EvalStrIntExpressionWithResult(&ArgStr[3], UInt16, &EvalResult) - (EProgCounter() + 3);
        if (EvalResult.OK)
        {
          if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt<-128) || (AdrInt>127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            ChkSpace(SegCode, EvalResult.AddrSpaceMask);
            BAsmCode[0] = 0x9f;
            BAsmCode[2] = AdrInt & 0xff;
            CodeLen = 3;
          }
        }
      }
    }
  }
}

static void DecodeDJNZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModWReg);
    if (AdrMode == ModWReg)
    {
      Integer AdrInt;
      tEvalResult EvalResult;

      BAsmCode[0] = (AdrPart << 4) + 0x0a;
      AdrInt = EvalStrIntExpressionWithResult(&ArgStr[2], UInt16, &EvalResult) - (EProgCounter() + 2);
      if (EvalResult.OK)
      {
        if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          ChkSpace(SegCode, EvalResult.AddrSpaceMask);
          BAsmCode[1] = AdrInt & 0xff;
          CodeLen = 2;
        }
      }
    }
  }
}

static void DecodeDWJNZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModRReg);
    if (AdrMode == ModRReg)
    {
      Integer AdrInt;
      tEvalResult EvalResult;

      BAsmCode[1] = AdrVals[0];
      AdrInt = EvalStrIntExpressionWithResult(&ArgStr[2], UInt16, &EvalResult) - (EProgCounter() + 3);
      if (EvalResult.OK)
      {
        if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          ChkSpace(SegCode, EvalResult.AddrSpaceMask);
          BAsmCode[0] = 0xc6;
          BAsmCode[2] = AdrInt & 0xff;
          CodeLen = 3;
        }
      }
    }
  }
}

static void DecodeCondAbs(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      BAsmCode[0] = 0x0d + Code;
      BAsmCode[1] = Hi(AdrWord);
      BAsmCode[2] = Lo(AdrWord);
      CodeLen = 3;
    }
  }
}

static void DecodeCondRel(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult) - (EProgCounter() + 2);

    if (EvalResult.OK)
    {
      if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        BAsmCode[0] = 0x0b + Code;
        BAsmCode[1] = AdrInt & 0xff;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeSPP(Word Code)
{
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[1] = (EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt6, &OK) << 2) + 0x02;
    if (OK)
    {
      BAsmCode[0] = 0xc7;
      CodeLen = 2;
    }
  }
}

static void DecodeSRP(Word Code)
{
  Boolean OK;

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt5, &OK) << 3;
    if (OK)
    {
      BAsmCode[0] = 0xc7;
      BAsmCode[1] += Code;
      CodeLen=2;
    }
  }
}

static void DecodeSLA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModWReg | MModReg | MModIWRReg);
    switch (AdrMode)
    {
      case ModWReg:
        BAsmCode[0] = 0x42;
        BAsmCode[1] = (AdrPart << 4) + AdrPart;
        CodeLen = 2;
        break;
      case ModReg:
        BAsmCode[0] = 0x44;
        BAsmCode[1] = AdrVals[0];
        BAsmCode[2] = AdrVals[0];
        CodeLen = 3;
        break;
      case ModIWRReg:
        BAsmCode[0] = 0x73;
        BAsmCode[1] = 0x40 + AdrPart;
        BAsmCode[2] = WorkOfs + AdrPart;
        CodeLen = 3;
        break;
    }
  }
}

static void DecodeSLAW(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModWRReg | MModRReg | MModIWRReg);
    switch (AdrMode)
    {
      case ModWRReg:
       BAsmCode[0] = 0x4e;
       BAsmCode[1] = (AdrPart << 4) + AdrPart;
       CodeLen = 2;
       break;
      case ModRReg:
       BAsmCode[0] = 0x47;
       BAsmCode[1] = AdrVals[0];
       BAsmCode[2] = AdrVals[0];
       CodeLen = 3;
       break;
      case ModIWRReg:
       BAsmCode[0] = 0x4e;
       BAsmCode[1] = 0x11 + (AdrPart << 4) + AdrPart;
       CodeLen = 2;
       break;
    }
  }
}

static void DecodeREG(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegReg,0,0x1ff);
}

static void DecodeBIT(Word Code)
{
  Byte Bit;
  tStrComp Comp;

  UNUSED(Code);

  if (ChkArgCnt(1, 1) && SplitBit(&Comp, &ArgStr[1], &Bit))
  {
    DecodeAdr(&Comp, MModWReg);
    if (AdrMode == ModWReg)
    {
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, (AdrPart << 4) + Bit, SegNone, False);
      PopLocHandle();
      as_snprintf(ListLine, STRINGSIZE, "=r%d.%s%c",
                  (int)AdrPart,
                  (Odd(Bit)) ? "!" : "", (Bit >> 1) + AscOfs);
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Code Table Handling */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddALU(const char *NName8, const char *NName16, Word NCode)
{
  AddInstTable(InstTable, NName8, NCode, DecodeALU);
  AddInstTable(InstTable, NName16, NCode | 0x100, DecodeALU);
}

static void AddReg(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeReg8);
}

static void AddReg16(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeReg16);
}

static void AddBit2(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit2);
}

static void AddBit1(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit1);
}

static void AddCondition(const char *NameAbs, const char *NameRel, Word NCode)
{
  AddInstTable(InstTable, NameAbs, NCode << 4, DecodeCondAbs);
  AddInstTable(InstTable, NameRel, NCode << 4, DecodeCondRel);
}

static void AddLoad(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLoad);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "LDW", 1, DecodeLD);
  AddInstTable(InstTable, "PEA", 0x01, DecodePEA_PEAU);
  AddInstTable(InstTable, "PEAU", 0x03, DecodePEA_PEAU);
  AddInstTable(InstTable, "PUSH", 0, DecodePUSH_PUSHU);
  AddInstTable(InstTable, "PUSHU", 1, DecodePUSH_PUSHU);
  AddInstTable(InstTable, "PUSHW", 0, DecodePUSHW_PUSHUW);
  AddInstTable(InstTable, "PUSHUW", 1, DecodePUSHW_PUSHUW);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "DIV", 0x5f, DecodeDIV_MUL);
  AddInstTable(InstTable, "MUL", 0x4f, DecodeDIV_MUL);
  AddInstTable(InstTable, "DIVWS", 0, DecodeDIVWS);
  AddInstTable(InstTable, "BTJF", 0x10, DecodeBTJF_BTJT);
  AddInstTable(InstTable, "BTJT", 0, DecodeBTJF_BTJT);
  AddInstTable(InstTable, "JP", 0xd48d, DecodeJP_CALL);
  AddInstTable(InstTable, "CALL", 0x74d2, DecodeJP_CALL);
  AddInstTable(InstTable, "CPJFI", 0, DecodeCPJFI_CPJTI);
  AddInstTable(InstTable, "CPJTI", 0x10, DecodeCPJFI_CPJTI);
  AddInstTable(InstTable, "DJNZ", 0, DecodeDJNZ);
  AddInstTable(InstTable, "DWJNZ", 0, DecodeDWJNZ);
  AddInstTable(InstTable, "SPP", 0, DecodeSPP);
  AddInstTable(InstTable, "SRP", 0x00, DecodeSRP);
  AddInstTable(InstTable, "SRP0", 0x04, DecodeSRP);
  AddInstTable(InstTable, "SRP1", 0x05, DecodeSRP);
  AddInstTable(InstTable, "SLA", 0, DecodeSLA);
  AddInstTable(InstTable, "SLAW", 0, DecodeSLAW);
  AddInstTable(InstTable, "REG", 0, DecodeREG);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);

  AddFixed("CCF" , 0x0061); AddFixed("DI"  , 0x0010);
  AddFixed("EI"  , 0x0000); AddFixed("HALT", 0xbf01);
  AddFixed("IRET", 0x00d3); AddFixed("NOP" , 0x00ff);
  AddFixed("RCF" , 0x0011); AddFixed("RET" , 0x0046);
  AddFixed("SCF" , 0x0001); AddFixed("SDM" , 0x00fe);
  AddFixed("SPM" , 0x00ee); AddFixed("WFI" , 0xef01);

  AddALU("ADC", "ADCW", 3); AddALU("ADD", "ADDW", 4); AddALU("AND", "ANDW",  1);
  AddALU("CP" , "CPW" , 9); AddALU("OR" , "ORW" , 0); AddALU("SBC", "SBCW",  2);
  AddALU("SUB", "SUBW", 5); AddALU("TCM", "TCMW", 8); AddALU("TM" , "TMW" , 10);
  AddALU("XOR", "XORW", 6);

  AddReg("CLR" , 0x90); AddReg("CPL" , 0x80); AddReg("DA"  , 0x70);
  AddReg("DEC" , 0x40); AddReg("INC" , 0x50); AddReg("POP" , 0x76);
  AddReg("POPU", 0x20); AddReg("RLC" , 0xb0); AddReg("ROL" , 0xa0);
  AddReg("ROR" , 0xc0); AddReg("RRC" , 0xd0); AddReg("SRA" , 0xe0);
  AddReg("SWAP", 0xf0);

  AddReg16("DECW" , 0x00cf); AddReg16("EXT"  , 0x01c6);
  AddReg16("INCW" , 0x00df); AddReg16("POPUW", 0x00b7);
  AddReg16("POPW" , 0x0075); AddReg16("RLCW" , 0x008f);
  AddReg16("RRCW" , 0x0036); AddReg16("SRAW" , 0x002f);

  AddBit2("BAND", 0x1f); AddBit2("BLD" , 0xf2);
  AddBit2("BOR" , 0x0f); AddBit2("BXOR", 0x6f);

  AddBit1("BCPL", 0x006f); AddBit1("BRES" , 0x001f);
  AddBit1("BSET", 0x000f); AddBit1("BTSET", 0x01f2);

  AddCondition("JPF"   , "JRF"   , 0x0); AddCondition("JPT"   , "JRT"   , 0x8);
  AddCondition("JPC"   , "JRC"   , 0x7); AddCondition("JPNC"  , "JRNC"  , 0xf);
  AddCondition("JPZ"   , "JRZ"   , 0x6); AddCondition("JPNZ"  , "JRNZ"  , 0xe);
  AddCondition("JPPL"  , "JRPL"  , 0xd); AddCondition("JPMI"  , "JRMI"  , 0x5);
  AddCondition("JPOV"  , "JROV"  , 0x4); AddCondition("JPNOV" , "JRNOV" , 0xc);
  AddCondition("JPEQ"  , "JREQ"  , 0x6); AddCondition("JPNE"  , "JRNE"  , 0xe);
  AddCondition("JPGE"  , "JRGE"  , 0x9); AddCondition("JPLT"  , "JRLT"  , 0x1);
  AddCondition("JPGT"  , "JRGT"  , 0xa); AddCondition("JPLE"  , "JRLE"  , 0x2);
  AddCondition("JPUGE" , "JRUGE" , 0xf); AddCondition("JPUL"  , "JRUL"  , 0x7);
  AddCondition("JPUGT" , "JRUGT" , 0xb); AddCondition("JPULE" , "JRULE" , 0x3);

  AddLoad("LDPP", 0x00); AddLoad("LDDP", 0x10);
  AddLoad("LDPD", 0x01); AddLoad("LDDD", 0x11);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/


static void MakeCode_ST9(void)
{
  CodeLen = 0; DontPrint = False; OpSize = 0;
  AbsSeg = (DPAssume == 1) ? SegData : SegCode;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_ST9(void)
{
  DPAssume = 0;
}

static Boolean IsDef_ST9(void)
{
  return (Memo("REG") || Memo("BIT"));
}

static void SwitchFrom_ST9(void)
{
  DeinitFields();
}

static void InternSymbol_ST9(char *Asc, TempResult *Erg)
{
  Boolean OK;
  Boolean Pair;
  LargeInt Num;

  as_tempres_set_none(Erg);
  if ((strlen(Asc) < 2) || (*Asc != 'R'))
    return;
  Asc++;

  if (*Asc == 'R')
  {
    if (strlen(Asc) < 2) return;
    Pair = True; Asc++;
  }
  else
    Pair = False;

  Num = ConstLongInt(Asc, &OK, 10);
  if (!OK || (Num < 0) || (Num > 255)) return;
  if ((Num & 0xf0) == 0xd0) return;
  if (Pair && Odd(Num)) return;

  if (Pair) Num += 0x100;
  as_tempres_set_int(Erg, Num); Erg->AddrSpaceMask |= (1 << SegReg);
}

static void SwitchTo_ST9(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "PC"; HeaderID = 0x32; NOPCode = 0xff;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | ( 1 << SegReg);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  SegLimits[SegData] = 0xffff;
  Grans[SegReg ] = 1; ListGrans[SegReg ] = 1; SegInits[SegReg ] = 0;
  SegLimits[SegReg ] = 0xff;

  MakeCode=MakeCode_ST9; IsDef=IsDef_ST9;
  SwitchFrom=SwitchFrom_ST9; InternSymbol=InternSymbol_ST9;

  pASSUMERecs = ASSUMEST9s;
  ASSUMERecCnt = ASSUMEST9Count;

  InitFields();
}

void codest9_init(void)
{
  CPUST9020 = AddCPU("ST9020", SwitchTo_ST9);
  CPUST9030 = AddCPU("ST9030", SwitchTo_ST9);
  CPUST9040 = AddCPU("ST9040", SwitchTo_ST9);
  CPUST9050 = AddCPU("ST9050", SwitchTo_ST9);
  AddInitPassProc(InitCode_ST9);
}
