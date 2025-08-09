/* code96.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator MCS/96-Familie                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmcode.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "assume.h"
#include "errmsg.h"
#include "strutil.h"
#include "headids.h"

#include "code96.h"

typedef enum
{
  ModNone = -1,
  ModDir = 0,
  ModInd = 1,
  ModPost = 2,
  ModIdx = 3,
  ModImm = 4
} tAdrType;

typedef struct
{
  Byte mode;
  Byte vals[4];
  int val_cnt;
  tSymbolFlags value_flags;
} adr_vals_t;

#define MModDir (1 << ModDir)
#define MModInd (1 << ModInd)
#define MModPost (1 << ModPost)
#define MModIdx (1 << ModIdx)
#define MModMem (MModInd | MModPost | MModIdx)
#define MModImm (1 << ModImm)

#define SFRStart 2
#define SFRStop 0x17

typedef enum
{
  eForceNone = 0,
  eForceShort = 1,
  eForceLong = 2
} tForceSize;

static CPUVar CPU8096, CPU80196, CPU80196N, CPU80296;

static ShortInt OpSize;

static LongInt WSRVal, WSR1Val;
static Word WinStart, WinStop, WinEnd, WinBegin;
static Word Win1Start, Win1Stop, Win1Begin, Win1End;

static IntType MemInt;

/*---------------------------------------------------------------------------*/

static void ChkSFR(Word Adr, const tStrComp *pArg)
{
  if ((Adr >= SFRStart) && (Adr <= SFRStop))
    WrStrErrorPos(ErrNum_IOAddrNotAllowed, pArg);
}

static void Chk296(Word Adr, const tStrComp *pArg)
{
  if ((MomCPU == CPU80296) && (Adr <= 1))
    WrStrErrorPos(ErrNum_IOAddrNotAllowed, pArg);
}

static Boolean ChkWork(Word *Adr)
{
  /* Registeradresse, die von Fenstern ueberdeckt wird ? */

  if ((*Adr >= WinBegin) && (*Adr <= WinEnd))
    return False;

  else if ((*Adr >= Win1Begin) && (*Adr <= Win1End))
    return False;

  /* Speicheradresse in Fenster ? */

  else if ((*Adr >= WinStart) && (*Adr <= WinStop))
  {
    *Adr = (*Adr) - WinStart + WinBegin;
    return True;
  }

  else if ((*Adr >= Win1Start) && (*Adr <= Win1Stop))
  {
    *Adr = (*Adr) - Win1Start + Win1Begin;
    return True;
  }

  /* Default */

  else
    return (*Adr <= 0xff);
}

/*!------------------------------------------------------------------------
 * \fn     reset_adr_vals(adr_vals_t *p_vals)
 * \brief  clear storage of decoded address
 * \param  p_vals storage reference
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = 0;
  p_vals->val_cnt = 0;
  p_vals->value_flags = eSymbolFlag_None;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals(int pos, const adr_vals_t *p_vals)
 * \brief  append extension bytes of decoded address to machine code
 * \param  pos destination index in machine instruction
 * \param  p_vals storage reference
 * ------------------------------------------------------------------------ */

static int append_adr_vals(int pos, const adr_vals_t *p_vals)
{
  memcpy(BAsmCode + pos, p_vals->vals, p_vals->val_cnt);
  set_b_guessed(p_vals->value_flags, pos, p_vals->val_cnt, 0xff);
  return pos + p_vals->val_cnt;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, adr_vals_t *p_vals, Byte Mask, Boolean AddrWide)
 * \brief  decode/parse address expression
 * \param  pArg source argument
 * \param  p_vals dest buffer for decoded address
 * \param  Mask bit mask of allowed addressing modes
 * \param  AddrWide extended address space?
 * \return deduced addressing mode
 * ------------------------------------------------------------------------ */

static tAdrType ChkAdr(tAdrType adr_type, adr_vals_t *p_vals, Byte Mask, const tStrComp *pArg)
{
  if ((adr_type == ModDir) && (!(Mask & MModDir)))
  {
    adr_type = ModInd; /* not exactly right, but mode counts */
    p_vals->mode = 0;
  }

  if ((adr_type != ModNone) && (!(Mask & (1 << adr_type))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    reset_adr_vals(p_vals);
    adr_type = ModNone;
  }
  return adr_type;
}

static int SplitForceSize(const char *pArg, tForceSize *pForceSize)
{
  switch (*pArg)
  {
    case '>': *pForceSize = eForceLong; return 1;
    case '<': *pForceSize = eForceShort; return 1;
    default: return 0;
  }
}

static tAdrType DecodeAdr(const tStrComp *pArg, adr_vals_t *p_vals, Byte Mask, Boolean AddrWide)
{
  LongInt AdrInt;
  LongWord AdrWord;
  Word BReg;
  Boolean OK;
  tSymbolFlags Flags;
  char *p, *p2;
  int ArgLen;
  Byte Reg;
  LongWord OMask;
  tAdrType adr_type;

  adr_type = ModNone;
  reset_adr_vals(p_vals);
  OMask = (1 << OpSize) - 1;

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case -1:
        WrStrErrorPos(ErrNum_UndefOpSizes, pArg);
        break;
      case 0:
        p_vals->vals[0] = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int8, &OK, &p_vals->value_flags);
        if (OK)
        {
          adr_type = ModImm;
          p_vals->val_cnt = 1;
          p_vals->mode = 1;
        }
        break;
      case 1:
        AdrWord = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int16, &OK, &p_vals->value_flags);
        if (OK)
        {
          adr_type = ModImm;
          p_vals->val_cnt = 2;
          p_vals->mode = 1;
          p_vals->vals[0] = Lo(AdrWord);
          p_vals->vals[1] = Hi(AdrWord);
        }
        break;
    }
    goto chk;
  }

  p = QuotPos(pArg->str.p_str, '[');
  if (p)
  {
    tStrComp Left, Mid, Right;

    StrCompSplitRef(&Left, &Mid, pArg, p);
    p2 = RQuotPos(Mid.str.p_str, ']');
    ArgLen = strlen(Mid.str.p_str);
    if (!p2 || (p2 > Mid.str.p_str + ArgLen - 1) || (p2 < Mid.str.p_str + ArgLen - 2)) WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    else
    {
      StrCompSplitRef(&Mid, &Right, &Mid, p2);
      BReg = EvalStrIntExpressionWithFlags(&Mid, Int16, &OK, &Flags);
      if (mFirstPassUnknown(Flags))
        BReg = 0;
      if (OK)
      {
        p_vals->value_flags |= Flags;
        if (!ChkWork(&BReg)) WrStrErrorPos(ErrNum_OverRange, &Mid);
        else
        {
          Reg = Lo(BReg);
          ChkSFR(Reg, &Mid);
          if (Reg & 1) WrStrErrorPos(ErrNum_AddrMustBeEven, &Mid);
          else if ((strlen(Left.str.p_str) == 0) && !strcmp(Right.str.p_str, "+"))
          {
            adr_type = ModPost;
            p_vals->mode = 2;
            p_vals->val_cnt = 1;
            p_vals->vals[0] = Reg + 1;
          }
          else if (strlen(Right.str.p_str) != 0) WrStrErrorPos(ErrNum_InvAddrMode, pArg);
          else if (strlen(Left.str.p_str) == 0)
          {
            p_vals->vals[0] = Reg;
            p_vals->val_cnt = 1;
            if (Mask & MModInd)
            {
              adr_type = ModInd;
              p_vals->mode = 2;
              p_vals->val_cnt = 1;
            }
            else
            {
              WrStrErrorPos(ErrNum_IndexedForIndirect, pArg);
              adr_type = ModIdx;
              p_vals->mode = 3;
              p_vals->vals[p_vals->val_cnt++] = 0;
            }
          }
          else
          {
            tForceSize ForceSize = eForceNone;
            int Offset = SplitForceSize(Left.str.p_str, &ForceSize);

            AdrInt = EvalStrIntExpressionOffsWithFlags(&Left, Offset, AddrWide ? Int24 : Int16, &OK, &Flags);
            if (OK)
            {
              p_vals->value_flags |= Flags;
              if ((AdrInt == 0) && (Mask & MModInd) && !ForceSize)
              {
                adr_type = ModInd;
                p_vals->mode = 2;
                p_vals->val_cnt = 1;
                p_vals->vals[0] = Reg;
              }
              else if (AddrWide)
              {
                adr_type = ModIdx;
                p_vals->mode= 3;
                p_vals->val_cnt = 4;
                p_vals->vals[0] = Reg;
                p_vals->vals[1] = AdrInt & 0xff;
                p_vals->vals[2] = (AdrInt >> 8) & 0xff;
                p_vals->vals[3] = (AdrInt >> 16) & 0xff;
              }
              else
              {
                Boolean IsShort = (AdrInt >= -128) && (AdrInt <= 127);

                if (!ForceSize)
                  ForceSize = IsShort ? eForceShort : eForceLong;
                if (ForceSize == eForceShort)
                {
                  if ((AdrInt > 127) && !mSymbolQuestionable(Flags)) WrStrErrorPos(ErrNum_OverRange, &Left);
                  else if ((AdrInt < -128) && !mSymbolQuestionable(Flags)) WrStrErrorPos(ErrNum_UnderRange, &Left);
                  else
                  {
                    adr_type = ModIdx;
                    p_vals->mode = 3;
                    p_vals->val_cnt = 2;
                    p_vals->vals[0] = Reg;
                    p_vals->vals[1] = Lo(AdrInt);
                  }
                }
                else
                {
                  adr_type = ModIdx;
                  p_vals->mode = 3;
                  p_vals->val_cnt = 3;
                  p_vals->vals[0] = Reg + 1;
                  p_vals->vals[1] = Lo(AdrInt);
                  p_vals->vals[2] = Hi(AdrInt);
                }
              }
            }
          }
        }
      }
    }
  }
  else
  {
    tForceSize ForceSize = eForceNone;
    int Offset = SplitForceSize(pArg->str.p_str, &ForceSize);

    AdrWord = EvalStrIntExpressionOffsWithFlags(pArg, Offset, MemInt, &OK, &p_vals->value_flags);
    if (mFirstPassUnknown(p_vals->value_flags))
      AdrWord &= (0xffffffff - OMask);
    if (OK)
    {
      if (AdrWord & OMask) WrStrErrorPos(ErrNum_NotAligned, pArg);
      else
      {
        BReg = AdrWord & 0xffff;
        if ((!(BReg & 0xffff0000)) && ChkWork(&BReg) && !ForceSize)
        {
          adr_type = ModDir;
          p_vals->val_cnt = 1;
          p_vals->vals[0] = Lo(BReg);
        }
        else if (AddrWide)
        {
          adr_type = ModIdx;
          p_vals->mode = 3;
          p_vals->val_cnt = 4;
          p_vals->vals[0] = 0;
          p_vals->vals[1] = AdrWord & 0xff;
          p_vals->vals[2] = (AdrWord >> 8) & 0xff;
          p_vals->vals[3] = (AdrWord >> 16) & 0xff;
        }
        else
        {
          Boolean IsShort = AdrWord >= 0xff80;

          if (!ForceSize)
            ForceSize = IsShort ? eForceShort : eForceLong;
          if (ForceSize == eForceShort)
          {
            if (!IsShort) WrStrErrorPos(ErrNum_UnderRange, pArg);
            else
            {
              adr_type = ModIdx;
              p_vals->mode = 3;
              p_vals->val_cnt = 2;
              p_vals->vals[0] = 0;
              p_vals->vals[1] = Lo(AdrWord);
            }
          }
          else
          {
            adr_type = ModIdx;
            p_vals->mode = 3;
            p_vals->val_cnt = 3;
            p_vals->vals[0] = 1;
            p_vals->vals[1] = Lo(AdrWord);
            p_vals->vals[2] = Hi(AdrWord);
          }
        }
      }
    }
  }

chk:
  return ChkAdr(adr_type, p_vals, Mask, pArg);
}

static void CalcWSRWindow(void)
{
  WSRVal &= 0x7f;
  if (WSRVal <= 0x0f)
  {
    WinStart = 0xffff;
    WinStop = 0;
    WinBegin = 0xff;
    WinEnd = 0;
  }
  else if (WSRVal <= 0x1f)
  {
    WinBegin = 0x80;
    WinEnd = 0xff;
    WinStart = (WSRVal < 0x18) ? ((WSRVal - 0x10) << 7) : ((WSRVal + 0x20) << 7);
    WinStop = WinStart + 0x7f;
  }
  else if (WSRVal <= 0x3f)
  {
    WinBegin = 0xc0;
    WinEnd = 0xff;
    WinStart = (WSRVal < 0x30) ? ((WSRVal - 0x20) << 6) : ((WSRVal + 0x40) << 6);
    WinStop = WinStart + 0x3f;
  }
  else if (WSRVal <= 0x7f)
  {
    WinBegin = 0xe0;
    WinEnd = 0xff;
    WinStart = (WSRVal < 0x60) ? ((WSRVal - 0x40) << 5) : ((WSRVal + 0x80) << 5);
    WinStop = WinStart + 0x1f;
  }
  if ((WinStop > 0x1fdf) && (MomCPU < CPU80296))
    WinStop = 0x1fdf;
}

static void CalcWSR1Window(void)
{
  if (WSR1Val <= 0x1f)
  {
    Win1Start = 0xffff;
    Win1Stop = 0;
    Win1Begin = 0xff;
    Win1End = 0;
  }
  else if (WSR1Val <= 0x3f)
  {
    Win1Begin = 0x40;
    Win1End = 0x7f;
    Win1Start = (WSR1Val < 0x30) ? ((WSR1Val - 0x20) << 6) : ((WSR1Val + 0x40) << 6);
    Win1Stop = Win1Start + 0x3f;
  }
  else if (WSR1Val <= 0x7f)
  {
    Win1Begin = 0x60;
    Win1End = 0x7f;
    Win1Start = (WSR1Val < 0x60) ? ((WSR1Val - 0x40) << 5) : ((WSR1Val + 0x80) << 5);
    Win1Stop = Win1Start + 0x1f;
  }
  else
  {
    Win1Begin = 0x40;
    Win1End = 0x7f;
    Win1Start = (WSR1Val + 0x340) << 6;
    Win1Stop = Win1Start + 0x3f;
  }
}

static Boolean IsShortBranch(LongInt Dist)
{
  return (Dist >= -1024) && (Dist <= 1023);
}

static Boolean IsByteBranch(LongInt Dist)
{
  return (Dist >= -128) && (Dist <= 127);
}

static Boolean GetShort(Word Code, LongInt Dist)
{
  switch (Code)
  {
    case 0: return True;
    case 1: return False;
    default: return IsShortBranch(Dist);
  }
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    BAsmCode[(CodeLen = 1) - 1] = Code;
}

static void DecodeALU3(Word Code)
{
  if (ChkArgCnt(2, 3))
  {
    int Start = 0;
    Boolean Special = (Hi(Code) & 0x40) || False,
            DoubleDest = (Hi(Code) & 0x08) || False;
    adr_vals_t adr_vals;

    OpSize = Hi(Code) & 3;
    if (Hi(Code) & 0x80)
      BAsmCode[Start++] = 0xfe;
    BAsmCode[Start++] = 0x40 + (Ord(ArgCnt==2) << 5)
                      + ((1 - OpSize) << 4)
                      + (Lo(Code) << 2);
    if (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModImm | MModMem, False) != ModNone)
    {
      Boolean OK;

      BAsmCode[Start - 1] += adr_vals.mode;
      Start = append_adr_vals(Start, &adr_vals);
      if (Special && (adr_vals.mode == 0))
        ChkSFR(adr_vals.vals[0], &ArgStr[ArgCnt]);
      if (ArgCnt == 3)
      {
        OK = (DecodeAdr(&ArgStr[2], &adr_vals, MModDir, False) != ModNone);
        if (OK)
        {
          Start = append_adr_vals(Start, &adr_vals);
          if (Special)
            ChkSFR(adr_vals.vals[0], &ArgStr[2]);
        }
      }
      else
        OK = True;
      if (OK)
      {
        OpSize += DoubleDest;
        if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
        {
          CodeLen = append_adr_vals(Start, &adr_vals);
          if (Special)
          {
            ChkSFR(adr_vals.vals[0], &ArgStr[1]);
            Chk296(adr_vals.vals[0], &ArgStr[1]);
          }
        }
      }
    }
  }
}

static void DecodeALU2(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    int Start = 0;
    Boolean Special = (Hi(Code) & 0x40) || False,
            DoubleDest = (Hi(Code) & 0x08) || False;
    Byte HReg, Mask;
    adr_vals_t adr_vals;

    OpSize = Hi(Code) & 3;

    if (Hi(Code) & 0x80)
      BAsmCode[Start++] = 0xfe;
    HReg = ((Hi(Code) & 0x20) ? 2 : 1) << 1;
    BAsmCode[Start++] = Lo(Code) + ((1 - OpSize) << HReg);
    Mask = MModMem | ((Hi(Code) & 0x20) ? MModImm : 0);
    if (DecodeAdr(&ArgStr[2], &adr_vals, Mask, False) != ModNone)
    {
      BAsmCode[Start - 1] += adr_vals.mode;
      Start = append_adr_vals(Start, &adr_vals);
      if ((Special) && (adr_vals.mode == 0))
        ChkSFR(adr_vals.vals[0], &ArgStr[2]);
      OpSize += DoubleDest;
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
      {
        CodeLen = append_adr_vals(Start, &adr_vals);
        if (Special)
        {
          ChkSFR(adr_vals.vals[0], &ArgStr[1]);
        }
      }
    }
  }
}

static void DecodeCMPL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80196))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    OpSize = eSymbolSize32Bit;
    BAsmCode[Start++] = 0xc5;
    if (DecodeAdr(&ArgStr[2], &adr_vals, MModDir, False) != ModNone)
    {
      Start = append_adr_vals(Start, &adr_vals);
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
        CodeLen = append_adr_vals(Start, &adr_vals);
    }
  }
}

static void DecodePUSH_POP(Word IsPOP)
{
  OpSize = eSymbolSize16Bit;

  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[1], &adr_vals, MModMem | (IsPOP ? 0 : MModImm), False) != ModNone)
    {
      BAsmCode[0] = 0xc8 + adr_vals.mode + (IsPOP << 2);
      CodeLen = append_adr_vals(1, &adr_vals);
    }
  }
}

static void DecodeBMOV(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    BAsmCode[Start++] = Code;
    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[2], &adr_vals, MModDir, False) != ModNone)
    {
      Start = append_adr_vals(Start, &adr_vals);
      OpSize = eSymbolSize32Bit;
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
        CodeLen = append_adr_vals(Start, &adr_vals);
    }
  }
}

static void DecodeALU1(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean DoubleDest = (Hi(Code) & 0x08) || False;
    adr_vals_t adr_vals;

    OpSize = (Hi(Code) & 3) + DoubleDest;
    if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
    {
      OpSize -= DoubleDest;
      BAsmCode[0] = Code + ((1 - OpSize) << 4);
      CodeLen = append_adr_vals(1, &adr_vals);
    }
  }
}

static void DecodeXCH(Word Code)
{
  OpSize = Hi(Code) & 3;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80196))
  {
    adr_vals_t adr_vals;
    Byte Start = 0;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModIdx | MModDir, False))
    {
      case ModIdx:
        BAsmCode[Start++] = (adr_vals.mode ? 0x0b : 0x04) + ((1 - OpSize) << 4);
        Start = append_adr_vals(Start, &adr_vals);
        if (DecodeAdr(&ArgStr[2], &adr_vals, MModDir, False) != ModNone)
          CodeLen = append_adr_vals(Start, &adr_vals);
        break;
      case ModDir:
      {
        Byte HReg = adr_vals.vals[0];

        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModDir | MModIdx, False))
        {
          case ModIdx:
            BAsmCode[Start] = 0x0b;
            goto common;
          case ModDir:
            BAsmCode[Start] = 0x04;
            goto common;
          common:
            BAsmCode[Start] += ((1 - OpSize) << 4); Start++;
            Start = append_adr_vals(Start, &adr_vals);
            BAsmCode[Start] = HReg;
            CodeLen = Start + 1;
            break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }
}

static void DecodeLDBZE_LDBSE(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[2], &adr_vals, MModMem | MModImm, False) != ModNone)
    {
      int Start = 0;

      BAsmCode[Start++] = Code + adr_vals.mode;
      Start = append_adr_vals(Start, &adr_vals);
      OpSize = eSymbolSize16Bit;
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
        CodeLen = append_adr_vals(Start, &adr_vals);
    }
  }
}

static void DecodeNORML(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    BAsmCode[Start++] = 0x0f;
    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[2], &adr_vals, MModDir, False) != ModNone)
    {
      Start = append_adr_vals(Start, &adr_vals);
      OpSize = eSymbolSize32Bit;
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
      {
        CodeLen = append_adr_vals(Start, &adr_vals);
      }
    }
  }
}

static void DecodeIDLPD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80196))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], &adr_vals, MModImm, False) != ModNone)
    {
      BAsmCode[0] = 0xf6;
      CodeLen = append_adr_vals(1, &adr_vals);
    }
  }
}

static void DecodeShift(Word Code)
{
  tSymbolSize inst_op_size = (tSymbolSize)(Hi(Code) & 3);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    BAsmCode[Start++] = 0x08 + Lo(Code)
                      + (Ord(inst_op_size == eSymbolSize8Bit) << 4)
                      + (Ord(inst_op_size == eSymbolSize32Bit) << 2);
    OpSize = eSymbolSize8Bit;
    switch (DecodeAdr(&ArgStr[2], &adr_vals, MModDir | MModImm, False))
    {
      case ModImm:
        if (adr_vals.vals[0] > 15) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
        else goto common;
        break;
      case ModDir:
        if (adr_vals.vals[0] < 16) WrStrErrorPos(ErrNum_UnderRange, &ArgStr[2]);
        else goto common;
        break;
      common:
        Start = append_adr_vals(Start, &adr_vals);
        OpSize = inst_op_size;
        if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
          CodeLen = append_adr_vals(Start, &adr_vals);
        break;
      default:
        break;
    }
  }
}

static void DecodeSKIP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
    {
      BAsmCode[0] = 0;
      CodeLen = append_adr_vals(1, &adr_vals);
    }
  }
}

static void DecodeELD_EST(Word Code)
{
  OpSize = Hi(Code) & 3;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80196N))
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[2], &adr_vals, MModInd | MModIdx, True) != ModNone)
    {
      Byte Start = 0;

      BAsmCode[Start++] = Code + (adr_vals.mode & 1) + ((1 - OpSize) << 1);
      Start = append_adr_vals(Start, &adr_vals);
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) == ModDir)
        CodeLen = append_adr_vals(Start, &adr_vals);
    }
  }
}

static void DecodeMac(Word Code)
{
  if (ChkArgCnt(1, 2)
   && ChkMinCPU(CPU80296))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModMem | (Hi(Code) ? 0 : MModImm), False) == ModNone)
      return;

    BAsmCode[Start++] = 0x4c + (Ord(ArgCnt == 1) << 5) + adr_vals.mode;
    Start = append_adr_vals(Start, &adr_vals);
    if (ArgCnt == 2)
    {
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModDir)
        return;
      Start = append_adr_vals(Start, &adr_vals);
    }
    BAsmCode[Start] = Lo(Code);
    CodeLen = Start + 1;
  }
}

static void DecodeMVAC_MSAC(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80296))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    BAsmCode[Start++] = 0x0d;
    OpSize = eSymbolSize8Bit;
    switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm | MModDir, False))
    {
      case ModImm:
        if (adr_vals.vals[0] > 31) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
        else goto common;
        break;
      case ModDir:
        if (adr_vals.vals[0] < 32) WrStrErrorPos(ErrNum_UnderRange, &ArgStr[2]);
        else goto common;
        break;
      common:
        Start = append_adr_vals(Start, &adr_vals);
        OpSize = eSymbolSize32Bit;
        if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) == ModDir)
        {
          CodeLen = append_adr_vals(Start, &adr_vals);
          BAsmCode[CodeLen - 1] += Code;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeRpt(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80296))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[1], &adr_vals, MModImm | MModPost | MModInd, False) != ModNone)
    {
      BAsmCode[CodeLen++] = 0x40 + adr_vals.mode;
      CodeLen = append_adr_vals(CodeLen, &adr_vals);
      BAsmCode[CodeLen++] = Code;
      BAsmCode[CodeLen++] = 4;
    }
  }
}

static void DecodeRel(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], MemInt, &OK, &Flags) - (EProgCounter() + 2);

    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && !IsByteBranch(AdrInt)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = Code;
        set_b_guessed(Flags, 1, 1, 0xff);
        BAsmCode[1] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeSCALL_LCALL_CALL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongWord AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[1], MemInt, &OK, &Flags);

    if (OK)
    {
      LongInt AdrInt = AdrWord - (EProgCounter() + 2);
      Boolean IsShort = GetShort(Code, AdrInt);

      if (IsShort)
      {
        if (!mFirstPassUnknownOrQuestionable(Flags) && !IsShortBranch(AdrInt)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
        else
        {
          CodeLen = 2;
          set_b_guessed(Flags, 1, 1, 0xff);
          BAsmCode[1] = AdrInt & 0xff;
          set_b_guessed(Flags, 0, 1, 0x07);
          BAsmCode[0] = 0x28 + ((AdrInt & 0x700) >> 8);
        }
      }
      else
      {
        CodeLen = 3;
        BAsmCode[0] = 0xef;
        AdrInt--;
        set_b_guessed(Flags, 1, 2, 0xff);
        BAsmCode[1] = Lo(AdrInt);
        BAsmCode[2] = Hi(AdrInt);
        if (!mFirstPassUnknownOrQuestionable(Flags) && IsShortBranch(AdrInt))
          WrStrErrorPos(ErrNum_ShortJumpPossible, &ArgStr[1]);
      }
    }
  }
}

static void DecodeBR_LJMP_SJMP(Word Code)
{
  OpSize = eSymbolSize16Bit;
  if (!ChkArgCnt(1, 1));
  else if ((Code == 0xff) && (QuotPos(ArgStr[1].str.p_str, '[')))
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[1], &adr_vals, MModInd, False) != ModNone)
    {
      BAsmCode[0] = 0xe3;
      CodeLen = append_adr_vals(1, &adr_vals);
    }
  }
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongWord AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[1], MemInt, &OK, &Flags);

    if (OK)
    {
      LongInt AdrInt = AdrWord - (EProgCounter() + 2);
      Boolean IsShort = GetShort(Code, AdrInt);

      if (IsShort)
      {
        if (!mFirstPassUnknownOrQuestionable(Flags) && !IsShortBranch(AdrInt)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
        else
        {
          CodeLen = 2;
          set_b_guessed(Flags, 1, 1, 0xff);
          BAsmCode[1] = AdrInt & 0xff;
          set_b_guessed(Flags, 0, 1, 0x07);
          BAsmCode[0] = 0x20 + ((AdrInt & 0x700) >> 8);
        }
      }
      else
      {
        CodeLen = 3;
        BAsmCode[0] = 0xe7;
        AdrInt--;
        set_b_guessed(Flags, 1, 2, 0xff);
        BAsmCode[1] = Lo(AdrInt);
        BAsmCode[2] = Hi(AdrInt);
        if (!mFirstPassUnknownOrQuestionable(Flags) && IsShortBranch(AdrInt))
          WrStrErrorPos(ErrNum_ShortJumpPossible, &ArgStr[1]);
      }
    }
  }
}

static void DecodeTIJMP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && ChkMinCPU(CPU80196))
  {
    adr_vals_t adr_vals;
    int Start = 0;

    BAsmCode[Start++] = 0xe2;
    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[2], &adr_vals, MModDir, False) != ModNone)
    {
      Start = append_adr_vals(Start, &adr_vals);
      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[3], &adr_vals, MModImm, False) != ModNone)
      {
        Start = append_adr_vals(Start, &adr_vals);
        OpSize = eSymbolSize16Bit;
        if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
          CodeLen = append_adr_vals(Start, &adr_vals);
      }
    }
  }
}

static void DecodeDJNZ_DJNZW(Word Size)
{
  if (ChkArgCnt(2, 2)
   && (!Size || ChkMinCPU(CPU80196)))
  {
    adr_vals_t adr_vals;

    OpSize = Size;
    if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
    {
      Boolean OK;
      tSymbolFlags Flags;
      LongInt AdrInt;
      int Start;

      BAsmCode[0] = 0xe0 + OpSize;
      Start = append_adr_vals(1, &adr_vals);
      AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], MemInt, &OK, &Flags) - (EProgCounter() + 3);
      if (OK)
      {
        if (!mSymbolQuestionable(Flags) && !IsByteBranch(AdrInt)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
        else
        {
          set_b_guessed(Flags, Start, 1, 0xff);
          BAsmCode[Start] = AdrInt & 0xff;
          CodeLen = Start + 1;
        }
      }
    }
  }
}

static void DecodeJBC_JBS(Word Code)
{
  if (ChkArgCnt(3, 3))
  {
    Boolean OK;
    tSymbolFlags Flags;

    BAsmCode[0] = Code + EvalStrIntExpressionWithFlags(&ArgStr[2], UInt3, &OK, &Flags);
    if (OK)
    {
      adr_vals_t adr_vals;

      set_b_guessed(Flags, 0, 1, 0x07);
      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModDir, False) != ModNone)
      {
        LongInt AdrInt;
        tSymbolFlags Flags;
        int Start;

        Start = append_adr_vals(1, &adr_vals);
        AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], MemInt, &OK, &Flags) - (EProgCounter() + 3);
        if (OK)
        {
          if (!mSymbolQuestionable(Flags) && !IsByteBranch(AdrInt)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[3]);
          else
          {
            set_b_guessed(Flags, Start, 1, 0xff);
            BAsmCode[Start] = AdrInt & 0xff;
            CodeLen = Start + 1;
          }
        }
      }
    }
  }
}

static void DecodeECALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80196N))
  {
    Boolean OK;
    tSymbolFlags Flags;

    LongInt AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], MemInt, &OK, &Flags) - (EProgCounter() + 4);
    if (OK)
    {
      BAsmCode[0] = 0xf1;
      set_b_guessed(Flags, 1, 3, 0xff);
      BAsmCode[1] = AdrInt & 0xff;
      BAsmCode[2] = (AdrInt >> 8) & 0xff;
      BAsmCode[3] = (AdrInt >> 16) & 0xff;
      CodeLen = 4;
    }
  }
}

static void DecodeEJMP_EBR(Word Code)
{
  UNUSED(Code);

  OpSize = eSymbolSize16Bit;
  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU80196N));
  else if (*ArgStr[1].str.p_str == '[')
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[1], &adr_vals, MModInd, False) != ModNone)
    {
      BAsmCode[0] = 0xe3;
      adr_vals.vals[0] += 1;
      CodeLen = append_adr_vals(1, &adr_vals);
    }
  }
  else
  {
    Boolean OK;
    tSymbolFlags Flags;

    LongInt AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], MemInt, &OK, &Flags) - (EProgCounter() + 4);
    if (OK)
    {
      BAsmCode[0] = 0xe6;
      set_b_guessed(Flags, 1, 3, 0xff);
      BAsmCode[1] = AdrInt & 0xff;
      BAsmCode[2] = (AdrInt >> 8) & 0xff;
      BAsmCode[3] = (AdrInt >> 16) & 0xff;
      CodeLen = 4;
    }
  }
}

/*---------------------------------------------------------------------------*/

static void AddSize(const char *NName, Word NCode, InstProc Proc, Word SizeMask)
{
  int l;
  char SizeName[20];

  if (SizeMask & 2)
    AddInstTable(InstTable, NName, 0x0100 | NCode, Proc);

  l = as_snprintf(SizeName, sizeof(SizeName), "%sB", NName);
  if (SizeMask & 1)
    AddInstTable(InstTable, SizeName, 0x0000 | NCode, Proc);

  if (SizeMask & 4)
  {
    SizeName[l - 1] = 'L';
    AddInstTable(InstTable, SizeName, 0x0200 | NCode, Proc);
  }
}

static void AddFixed(const char *NName, Byte NCode, CPUVar NMin, CPUVar NMax)
{
  if ((MomCPU >= NMin) && (MomCPU <= NMax))
    AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddALU3(const char *NName, Word NCode)
{
  AddSize(NName, NCode, DecodeALU3, 3);
}

static void AddALU2(const char *NName, Word NCode)
{
  AddSize(NName, NCode, DecodeALU2, 3);
}

static void AddALU1(const char *NName, Word NCode)
{
  AddSize(NName, NCode, DecodeALU1, 3);
}

static void AddRel(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel);
}

static void AddMac(const char *NName, Word NCode, Boolean NRel)
{
  AddInstTable(InstTable, NName, NCode | (NRel ? 0x100 : 0), DecodeMac);
}

static void AddRpt(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRpt);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(207);
  SetDynamicInstTable(InstTable);

  add_null_pseudo(InstTable);

  AddInstTable(InstTable, "CMPL", 0, DecodeCMPL);
  AddInstTable(InstTable, "PUSH", 0, DecodePUSH_POP);
  AddInstTable(InstTable, "POP" , 1, DecodePUSH_POP);
  if (MomCPU >= CPU80196)
  {
    AddInstTable(InstTable, "BMOV", 0xc1, DecodeBMOV);
    AddInstTable(InstTable, "BMOVI", 0xcd, DecodeBMOV);
  }
  if (MomCPU >= CPU80196N)
    AddInstTable(InstTable, "EBMOVI", 0xe4, DecodeBMOV);
  AddSize("XCH", 0, DecodeXCH, 3);
  AddInstTable(InstTable, "LDBZE", 0xac, DecodeLDBZE_LDBSE);
  AddInstTable(InstTable, "LDBSE", 0xbc, DecodeLDBZE_LDBSE);
  AddInstTable(InstTable, "NORML", 0, DecodeNORML);
  AddInstTable(InstTable, "IDLPD", 0, DecodeIDLPD);
  AddSize("SHR", 0, DecodeShift, 7);
  AddSize("SHL", 1, DecodeShift, 7);
  AddSize("SHRA", 2, DecodeShift, 7);
  AddInstTable(InstTable, "SKIP", 0, DecodeSKIP);
  AddSize("ELD", 0xe8, DecodeELD_EST, 3);
  AddSize("EST", 0x1c, DecodeELD_EST, 3);
  AddInstTable(InstTable, "MVAC", 1, DecodeMVAC_MSAC);
  AddInstTable(InstTable, "MSAC", 3, DecodeMVAC_MSAC);
  AddInstTable(InstTable, "CALL", 0xff, DecodeSCALL_LCALL_CALL);
  AddInstTable(InstTable, "LCALL", 1, DecodeSCALL_LCALL_CALL);
  AddInstTable(InstTable, "SCALL", 0, DecodeSCALL_LCALL_CALL);
  AddInstTable(InstTable, "BR", 0xff, DecodeBR_LJMP_SJMP);
  AddInstTable(InstTable, "LJMP", 1, DecodeBR_LJMP_SJMP);
  AddInstTable(InstTable, "SJMP", 0, DecodeBR_LJMP_SJMP);
  AddInstTable(InstTable, "TIJMP", 0, DecodeTIJMP);
  AddInstTable(InstTable, "DJNZ", 0, DecodeDJNZ_DJNZW);
  AddInstTable(InstTable, "DJNZW", 1, DecodeDJNZ_DJNZW);
  AddInstTable(InstTable, "JBC", 0x30, DecodeJBC_JBS);
  AddInstTable(InstTable, "JBS", 0x38, DecodeJBC_JBS);
  AddInstTable(InstTable, "ECALL", 0, DecodeECALL);
  AddInstTable(InstTable, "EJMP", 0, DecodeEJMP_EBR);
  AddInstTable(InstTable, "EBR", 0, DecodeEJMP_EBR);

  AddFixed("CLRC" , 0xf8, CPU8096  , CPU80296 );
  AddFixed("CLRVT", 0xfc, CPU8096  , CPU80296 );
  AddFixed("DI"   , 0xfa, CPU8096  , CPU80296 );
  AddFixed("DPTS" , 0xec, CPU80196 , CPU80196N);
  AddFixed("EI"   , 0xfb, CPU8096  , CPU80296 );
  AddFixed("EPTS" , 0xed, CPU80196 , CPU80196N);
  AddFixed("NOP"  , 0xfd, CPU8096  , CPU80296 );
  AddFixed("POPA" , 0xf5, CPU80196 , CPU80296 );
  AddFixed("POPF" , 0xf3, CPU8096  , CPU80296 );
  AddFixed("PUSHA", 0xf4, CPU80196 , CPU80296 );
  AddFixed("PUSHF", 0xf2, CPU8096  , CPU80296 );
  AddFixed("RET"  , 0xf0, CPU8096  , CPU80296 );
  AddFixed("RST"  , 0xff, CPU8096  , CPU80296 );
  AddFixed("SETC" , 0xf9, CPU8096  , CPU80296 );
  AddFixed("TRAP" , 0xf7, CPU8096  , CPU80296 );
  AddFixed("RETI" , 0xe5, CPU80196N, CPU80296 );

  AddALU3("ADD" , 0x0001);
  AddALU3("AND" , 0x0000);
  AddALU3("MUL" , 0xc803);
  AddALU3("MULU", 0x4803);
  AddALU3("SUB" , 0x0002);

  AddALU2("ADDC", 0x20a4);
  AddALU2("CMP" , 0x2088);
  AddALU2("DIV" , 0xe88c);
  AddALU2("DIVU", 0x688c);
  AddALU2("LD"  , 0x20a0);
  AddALU2("OR"  , 0x2080);
  AddALU2("ST"  , 0x00c0);
  AddALU2("SUBC", 0x20a8);
  AddALU2("XOR" , 0x2084);

  AddALU1("CLR", 0x0001);
  AddALU1("DEC", 0x0005);
  AddALU1("EXT", 0x8806);
  AddALU1("INC", 0x0007);
  AddALU1("NEG", 0x0003);
  AddALU1("NOT", 0x0002);

  AddRel("JC"   , 0xdb);
  AddRel("JE"   , 0xdf);
  AddRel("JGE"  , 0xd6);
  AddRel("JGT"  , 0xd2);
  AddRel("JH"   , 0xd9);
  AddRel("JLE"  , 0xda);
  AddRel("JLT"  , 0xde);
  AddRel("JNC"  , 0xd3);
  AddRel("JNE"  , 0xd7);
  AddRel("JNH"  , 0xd1);
  AddRel("JNST" , 0xd0);
  AddRel("JNV"  , 0xd5);
  AddRel("JNVT" , 0xd4);
  AddRel("JST"  , 0xd8);
  AddRel("JV"   , 0xdd);
  AddRel("JVT"  , 0xdc);

  AddMac("MAC"   , 0x00, False); AddMac("SMAC"  , 0x01, False);
  AddMac("MACR"  , 0x04, True ); AddMac("SMACR" , 0x05, True );
  AddMac("MACZ"  , 0x08, False); AddMac("SMACZ" , 0x09, False);
  AddMac("MACRZ" , 0x0c, True ); AddMac("SMACRZ", 0x0d, True );

  AddRpt("RPT"    , 0x00); AddRpt("RPTNST" , 0x10); AddRpt("RPTNH"  , 0x11);
  AddRpt("RPTGT"  , 0x12); AddRpt("RPTNC"  , 0x13); AddRpt("RPTNVT" , 0x14);
  AddRpt("RPTNV"  , 0x15); AddRpt("RPTGE"  , 0x16); AddRpt("RPTNE"  , 0x17);
  AddRpt("RPTST"  , 0x18); AddRpt("RPTH"   , 0x19); AddRpt("RPTLE"  , 0x1a);
  AddRpt("RPTC"   , 0x1b); AddRpt("RPTVT"  , 0x1c); AddRpt("RPTV"   , 0x1d);
  AddRpt("RPTLT"  , 0x1e); AddRpt("RPTE"   , 0x1f); AddRpt("RPTI"   , 0x20);
  AddRpt("RPTINST", 0x30); AddRpt("RPTINH" , 0x31); AddRpt("RPTIGT" , 0x32);
  AddRpt("RPTINC" , 0x33); AddRpt("RPTINVT", 0x34); AddRpt("RPTINV" , 0x35);
  AddRpt("RPTIGE" , 0x36); AddRpt("RPTINE" , 0x37); AddRpt("RPTIST" , 0x38);
  AddRpt("RPTIH"  , 0x39); AddRpt("RPTILE" , 0x3a); AddRpt("RPTIC"  , 0x3b);
  AddRpt("RPTIVT" , 0x3c); AddRpt("RPTIV"  , 0x3d); AddRpt("RPTILT" , 0x3e);
  AddRpt("RPTIE"  , 0x3f);

  AddIntelPseudo(InstTable, eIntPseudoFlag_LittleEndian);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_96(void)
{
  OpSize = eSymbolSizeUnknown;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_96(void)
{
  WSRVal  = 0; CalcWSRWindow();
  WSR1Val = 0; CalcWSR1Window();
}

static Boolean IsDef_96(void)
{
  return False;
}

static void SwitchFrom_96(void)
{
  DeinitFields();
}

static as_assume_rec_t ASSUME96s[] =
{
  {"WSR" , &WSRVal , 0, 0xff, 0x00, CalcWSRWindow  },
  {"WSR1", &WSR1Val, 0, 0xbf, 0x00, CalcWSR1Window },
};

static void SwitchTo_96(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("MCS-96/196");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0xfd;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode ] = 1;
  ListGrans[SegCode ] = 1;
  SegInits[SegCode ] = 0;
  SegLimits[SegCode] = (MomCPU >= CPU80196N) ? 0xffffffl : 0xffff;

  MakeCode = MakeCode_96;
  IsDef = IsDef_96;
  SwitchFrom = SwitchFrom_96;

  MemInt = (MomCPU >= CPU80196N) ? UInt24 : UInt16;

  if (MomCPU >= CPU80196)
    assume_set(ASSUME96s, (MomCPU >= CPU80296) ? as_array_size(ASSUME96s) : 1);

  InitFields();
}

void code96_init(void)
{
  CPU8096   = AddCPU("8096"  , SwitchTo_96);
  CPU80196  = AddCPU("80196" , SwitchTo_96);
  CPU80196N = AddCPU("80196N", SwitchTo_96);
  CPU80296  = AddCPU("80296" , SwitchTo_96);

  AddInitPassProc(InitCode_96);
}
