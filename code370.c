/* code370.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 370-Familie                                                 */
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
#include "asmcode.h"
#include "asmitree.h"
#include "intformat.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code370.h"

typedef struct
{
  char *Name;
  Word Code;
} FixedOrder;

typedef enum
{
  ModNone = -1,
  ModAccA = 0,       /* A */
  ModAccB = 1,       /* B */
  ModReg = 2,        /* Rn */
  ModPort = 3,       /* Pn */
  ModAbs = 4,        /* nnnn */
  ModBRel = 5,       /* nnnn(B) */
  ModSPRel = 6,      /* nn(SP) */
  ModIReg = 7,       /* @Rn */
  ModRegRel = 8,     /* nn(Rn) */
  ModImm = 9,        /* #nn */
  ModImmBRel = 10,   /* #nnnn(B) */
  ModImmRegRel = 11  /* #nn(Rm) */
} adr_mode_t;

#define MModAccA (1 << ModAccA)
#define MModAccB (1 << ModAccB)
#define MModReg (1 << ModReg)
#define MModPort (1 << ModPort)
#define MModAbs (1 << ModAbs)
#define MModBRel (1 << ModBRel)
#define MModSPRel (1 << ModSPRel)
#define MModIReg (1 << ModIReg)
#define MModRegRel (1 << ModRegRel)
#define MModImm (1 << ModImm)
#define MModImmBRel (1 << ModImmBRel)
#define MModImmRegRel (1 << ModImmRegRel)

typedef struct
{
  int count;
  Byte values[2];
  tSymbolFlags flags;
} adr_vals_t;

static CPUVar CPU37010, CPU37020, CPU37030, CPU37040, CPU37050;

static tSymbolSize OpSize;

/****************************************************************************/

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->count = 0;
  p_vals->flags = eSymbolFlag_None;
}

static void append_adr_vals(const adr_vals_t *p_vals)
{
  set_b_guessed(p_vals->flags, CodeLen, p_vals->count, 0xff);
  memcpy(&BAsmCode[CodeLen], p_vals->values, p_vals->count);
  CodeLen += p_vals->count;
}

static char *HasDisp(char *Asc)
{
  char *p;
  int Lev;

  if ((*Asc) && (Asc[strlen(Asc) - 1] == ')'))
  {
    p = Asc + strlen(Asc) - 2;
    Lev = 0;
    while ((p >= Asc) && (Lev != -1))
    {
      switch (*p)
      {
        case '(': Lev--; break;
        case ')': Lev++; break;
      }
      if (Lev != -1)
        p--;
    }
    if (p < Asc)
    {
      WrXError(ErrNum_BrackErr, Asc);
      return NULL;
    }
  }
  else
    p = NULL;

  return p;
}

static adr_mode_t DecodeAdrRel(const tStrComp *pArg, adr_vals_t *p_vals, Word Mask, Boolean AddrRel)
{
  Integer HVal;
  char *p;
  Boolean OK;
  adr_mode_t adr_mode;

  adr_mode = ModNone;
  reset_adr_vals(p_vals);

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    if (Mask & MModAccA)
      adr_mode = ModAccA;
    else if (Mask & MModReg)
    {
      p_vals->count = 1;
      p_vals->values[0] = 0;
      adr_mode = ModReg;
    }
    else
    {
      p_vals->count = 2;
      p_vals->values[0] = 0;
      p_vals->values[1] = 0;
      adr_mode = ModAbs;
    }
    goto chk;
  }

  if (!as_strcasecmp(pArg->str.p_str, "B"))
  {
    if (Mask & MModAccB)
      adr_mode = ModAccB;
    else if (Mask & MModReg)
    {
      p_vals->count = 1;
      p_vals->values[0] = 1;
      adr_mode = ModReg;
    }
    else
    {
      p_vals->count = 2;
      p_vals->values[0] = 0;
      p_vals->values[1] = 1;
      adr_mode = ModAbs;
    }
    goto chk;
  }

  if (*pArg->str.p_str == '#')
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    p = HasDisp(Arg.str.p_str);
    if (!p)
    {
      switch (OpSize)
      {
        case eSymbolSize8Bit:
          p_vals->values[0] = EvalStrIntExpressionWithFlags(&Arg, Int8, &OK, &p_vals->flags);
          break;
        case eSymbolSize16Bit:
          HVal = EvalStrIntExpressionWithFlags(&Arg, Int16, &OK, &p_vals->flags);
          p_vals->values[0] = Hi(HVal);
          p_vals->values[1] = Lo(HVal);
          break;
        default:
          OK = False;
      }
      if (OK)
      {
        p_vals->count = 1 + OpSize;
        adr_mode = ModImm;
      }
    }
    else
    {
      tStrComp Left, Right;
      tSymbolFlags Flags;

      StrCompSplitRef(&Left, &Right, &Arg, p);
      if (Left.str.p_str[0])
        HVal = EvalStrIntExpressionWithFlags(&Left, Int16, &OK, &Flags);
      else
      {
        HVal = 0;
        OK = True;
        Flags = eSymbolFlag_None;
      }
      p_vals->flags |= Flags;
      if (OK)
      {
        StrCompShorten(&Right, 1);
        if (!as_strcasecmp(Right.str.p_str, "B"))
        {
          p_vals->values[0] = Hi(HVal);
          p_vals->values[1] = Lo(HVal);
          p_vals->count = 2;
          adr_mode = ModImmBRel;
        }
        else
        {
          if (mFirstPassUnknown(Flags))
            HVal &= 127;
          if (ChkRange(HVal, -128, 127))
          {
            p_vals->values[0] = HVal & 0xff;
            p_vals->count = 1;
            p_vals->values[1] = EvalStrIntExpressionWithFlags(&Right, UInt8, &OK, &Flags);
            if (OK)
            {
              p_vals->count = 2;
              p_vals->flags |= Flags;
              adr_mode = ModImmRegRel;
            }
          }
        }
      }
    }
    goto chk;
  }

  if (*pArg->str.p_str == '@')
  {
    p_vals->values[0] = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int8, &OK, &p_vals->flags);
    if (OK)
    {
      p_vals->count = 1;
      adr_mode = ModIReg;
    }
    goto chk;
  }

  p = HasDisp(pArg->str.p_str);

  if (!p)
  {
    HVal = EvalStrIntExpressionWithFlags(pArg, Int16, &OK, &p_vals->flags);
    if (OK)
    {
      if (((Mask & MModReg) != 0) && (Hi(HVal) == 0))
      {
        p_vals->values[0] = Lo(HVal);
        p_vals->count = 1;
        adr_mode = ModReg;
      }
      else if (((Mask & MModPort) != 0) && (Hi(HVal) == 0x10))
      {
        p_vals->values[0] = Lo(HVal);
        p_vals->count = 1;
        adr_mode = ModPort;
      }
      else
      {
        if (AddrRel)
          HVal -= EProgCounter() + 3;
        p_vals->values[0] = Hi(HVal);
        p_vals->values[1] = Lo(HVal);
        p_vals->count = 2;
        adr_mode = ModAbs;
      }
    }
    goto chk;
  }
  else
  {
    tStrComp Left, Right;
    tSymbolFlags Flags;

    StrCompSplitRef(&Left, &Right, pArg, p);
    HVal = EvalStrIntExpressionWithFlags(&Left, Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      HVal &= 0x7f;
    if (OK)
    {
      p_vals->flags |= Flags;
      StrCompShorten(&Right, 1);
      if (!as_strcasecmp(Right.str.p_str, "B"))
      {
        if (AddrRel)
          HVal -= EProgCounter() + 3;
        p_vals->values[0] = Hi(HVal);
        p_vals->values[1] = Lo(HVal);
        p_vals->count = 2;
        adr_mode = ModBRel;
      }
      else if (!as_strcasecmp(Right.str.p_str, "SP"))
      {
        if (AddrRel)
          HVal -= EProgCounter() + 3;
        if (ChkRangePos(HVal, -128, 127, &Left))
        {
          p_vals->values[0] = HVal & 0xff;
          p_vals->count = 1;
          adr_mode = ModSPRel;
        }
      }
      else
      {
        if (ChkRangePos(HVal, -128, 127, &Left))
        {
          p_vals->values[0] = HVal & 0xff;
          p_vals->values[1] = EvalStrIntExpressionWithFlags(&Right, Int8, &OK, &Flags);
          if (OK)
          {
            p_vals->flags |= Flags;
            p_vals->count = 2;
            adr_mode = ModRegRel;
          }
        }
      }
    }
    goto chk;
  }

chk:
  if ((adr_mode != ModNone) && (!(Mask & (1 << adr_mode))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    adr_mode = ModNone;
    reset_adr_vals(p_vals);
  }
  return adr_mode;
}

static adr_mode_t DecodeAdr(const tStrComp *pArg, adr_vals_t *p_vals, Word Mask)
{
  return DecodeAdrRel(pArg, p_vals, Mask, FALSE);
}

static void PutCode(Word Code)
{
  if (Hi(Code) == 0)
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
  else
  {
    CodeLen = 2;
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
  }
}

static void DissectBitValue(LargeWord Symbol, Word *pAddr, Byte *pBit)
{
  *pAddr = Symbol & 0xffff;
  *pBit = (Symbol >> 16) & 7;
}

static void DissectBit_370(char *pDest, size_t DestSize, LargeWord Symbol)
{
  Word Addr;
  Byte Bit;

  DissectBitValue(Symbol, &Addr, &Bit);

  if (Addr < 2)
    as_snprintf(pDest, DestSize, "%c", HexStartCharacter + Addr);
  else
    as_snprintf(pDest, DestSize, "%~0.*u%s",
                ListRadixBase, (unsigned)Addr, GetIntConstIntelSuffix(ListRadixBase));
  as_snprcatf(pDest, DestSize, ".%c", Bit + '0');
}

static Boolean DecodeBitExpr(int Start, int Stop, LongWord *pResult)
{
  Boolean OK;

  if (Start == Stop)
  {
    *pResult = EvalStrIntExpression(&ArgStr[Start], UInt19, &OK);
    return OK;
  }
  else
  {
    Byte Bit;
    Word Addr;

    Bit = EvalStrIntExpression(&ArgStr[Start], UInt3, &OK);
    if (!OK)
      return OK;

    if ((!as_strcasecmp(ArgStr[Stop].str.p_str, "A")) || (!as_strcasecmp(ArgStr[Stop].str.p_str, "B")))
    {
      Addr = toupper(*ArgStr[Stop].str.p_str) - 'A';
      OK = True;
    }
    else
    {
      tSymbolFlags Flags;

      Addr = EvalStrIntExpressionWithFlags(&ArgStr[Stop], UInt16, &OK, &Flags);
      if (!OK)
        return OK;
      if (mFirstPassUnknown(Flags))
        Addr &= 0xff;
      if (Addr & 0xef00) /* 00h...0ffh, 1000h...10ffh allowed */
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[Stop]);
        return False;
      }
    }

    *pResult = (((LongWord)Bit) << 16) + Addr;
    return True;
  }
}

/****************************************************************************/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    PutCode(Code);
}

static void DecodeDBIT(Word Code)
{
  LongWord Value;

  UNUSED(Code);

  if (ChkArgCnt(1, 2) && DecodeBitExpr(1, ArgCnt, &Value))
  {
    PushLocHandle(-1);
    EnterIntSymbol(&LabPart, Value, SegBData, False);
    *ListLine = '=';
    DissectBit_370(ListLine + 1, STRINGSIZE - 1, Value);
    PopLocHandle();
  }
}

static void DecodeMOV(Word Code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModAccB + MModReg + MModPort + MModAbs + MModIReg + MModBRel
                                                + MModSPRel + MModRegRel + MModAccA))
    {
      case ModAccA:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModAbs + MModIReg + MModBRel + MModRegRel
                                                   + MModSPRel + MModAccB + MModPort + MModImm))
        {
          case ModReg:
            PutCode(0x12);
            append_adr_vals(&src_adr_vals);
            break;
          case ModAbs:
            PutCode(0x8a);
            append_adr_vals(&src_adr_vals);
            break;
          case ModIReg:
            PutCode(0x9a);
            append_adr_vals(&src_adr_vals);
            break;
          case ModBRel:
            PutCode(0xaa);
            append_adr_vals(&src_adr_vals);
            break;
          case ModRegRel:
            PutCode(0xf4ea);
            append_adr_vals(&src_adr_vals);
            break;
          case ModSPRel:
            PutCode(0xf1);
            append_adr_vals(&src_adr_vals);
            break;
          case ModAccB:
            PutCode(0x62);
            break;
          case ModPort:
            PutCode(0x80);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x22);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModAccB:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA + MModReg + MModPort + MModImm))
        {
          case ModAccA:
            PutCode(0xc0);
            break;
          case ModReg:
            PutCode(0x32);
            append_adr_vals(&src_adr_vals);
            break;
          case ModPort:
            PutCode(0x91);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x52);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModReg:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA + MModAccB + MModReg + MModPort + MModImm))
        {
          case ModAccA:
            PutCode(0xd0);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModAccB:
            PutCode(0xd1);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModReg:
            PutCode(0x42);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModPort:
            PutCode(0xa2);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModImm:
            PutCode(0x72);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModPort:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA + MModAccB + MModReg + MModImm))
        {
          case ModAccA:
            PutCode(0x21);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModAccB:
            PutCode(0x51);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModReg:
            PutCode(0x71);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModImm:
            PutCode(0xf7);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModAbs:
        if (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA) != ModNone)
        {
          PutCode(0x8b);
          append_adr_vals(&dest_adr_vals);
        }
        break;
      case ModIReg:
        if (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA) != ModNone)
        {
          PutCode(0x9b);
          append_adr_vals(&dest_adr_vals);
        }
        break;
      case ModBRel:
        if (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA) != ModNone)
        {
          PutCode(0xab);
          append_adr_vals(&dest_adr_vals);
        }
        break;
      case ModSPRel:
        if (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA) != ModNone)
        {
          PutCode(0xf2);
          append_adr_vals(&dest_adr_vals);
        }
        break;
      case ModRegRel:
        if (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA) != ModNone)
        {
          PutCode(0xf4eb);
          append_adr_vals(&dest_adr_vals);
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeMOVW(Word Code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(Code);

  OpSize = eSymbolSize16Bit;
  if (ChkArgCnt(2, 2))
  {
    if (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg) != ModNone)
    {
      switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm + MModImmBRel + MModImmRegRel))
      {
        case ModReg:
          PutCode(0x98);
          append_adr_vals(&src_adr_vals);
          append_adr_vals(&dest_adr_vals);
          break;
        case ModImm:
          PutCode(0x88);
          append_adr_vals(&src_adr_vals);
          append_adr_vals(&dest_adr_vals);
          break;
        case ModImmBRel:
          PutCode(0xa8);
          append_adr_vals(&src_adr_vals);
          append_adr_vals(&dest_adr_vals);
          break;
        case ModImmRegRel:
          PutCode(0xf4e8);
          append_adr_vals(&src_adr_vals);
          append_adr_vals(&dest_adr_vals);
          break;
        default:
          break;
      }
    }
  }
}

static void DecodeRel8(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags) - (EProgCounter() + 2);

    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        PutCode(Code);
        set_b_guessed(Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeCMP(Word Code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModAccA + MModAccB + MModReg))
    {
      case ModAccA:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAbs + MModIReg + MModBRel + MModRegRel + MModSPRel + MModAccB + MModReg + MModImm))
        {
          case ModAbs:
            PutCode(0x8d);
            append_adr_vals(&src_adr_vals);
            break;
          case ModIReg:
            PutCode(0x9d);
            append_adr_vals(&src_adr_vals);
            break;
          case ModBRel:
            PutCode(0xad);
            append_adr_vals(&src_adr_vals);
            break;
          case ModRegRel:
            PutCode(0xf4ed);
            append_adr_vals(&src_adr_vals);
            break;
          case ModSPRel:
            PutCode(0xf3);
            append_adr_vals(&src_adr_vals);
            break;
          case ModAccB:
            PutCode(0x6d);
            break;
          case ModReg:
            PutCode(0x1d);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x2d);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModAccB:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm))
        {
          case ModReg:
            PutCode(0x3d);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x5d);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModReg:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm))
        {
          case ModReg:
            PutCode(0x4d);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModImm:
            PutCode(0x7d);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeALU1(Word Code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModAccA + MModAccB + MModReg))
    {
      case ModAccA:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccB + MModReg + MModImm))
        {
          case ModAccB:
            PutCode(0x60 + Code);
            break;
          case ModReg:
            PutCode(0x10 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x20 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModAccB:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm))
        {
          case ModReg:
            PutCode(0x30 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x50 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModReg:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm))
        {
          case ModReg:
            PutCode(0x40 + Code);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModImm:
            PutCode(0x70 + Code);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeALU2(Word Code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;
  Boolean Rela = Hi(Code) != 0;
  Code &= 0xff;

  if (ChkArgCnt(Rela ? 3 : 2, Rela ? 3 : 2))
  {
    switch (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModAccA + MModAccB + MModReg + MModPort))
    {
      case ModAccA:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccB + MModReg + MModImm))
        {
          case ModAccB:
            PutCode(0x60 + Code);
            break;
          case ModReg:
            PutCode(0x10 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x20 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModAccB:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm))
        {
          case ModReg:
            PutCode(0x30 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          case ModImm:
            PutCode(0x50 + Code);
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModReg:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg + MModImm))
        {
          case ModReg:
            PutCode(0x40 + Code);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModImm:
            PutCode(0x70 + Code);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          default:
            break;
        }
        break;
      case ModPort:
        switch (DecodeAdr(&ArgStr[1], &src_adr_vals, MModAccA + MModAccB + MModImm))
        {
          case ModAccA:
            PutCode(0x80 + Code);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModAccB:
            PutCode(0x90 + Code);
            append_adr_vals(&dest_adr_vals);
            break;
          case ModImm:
            PutCode(0xa0 + Code);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&dest_adr_vals);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
    if ((CodeLen != 0) && (Rela))
    {
      Boolean OK;
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt16, &OK, &Flags) - (EProgCounter() + CodeLen + 1);

      if (!OK)
        CodeLen = 0;
      else if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128)))
      {
        WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[3]);
        CodeLen = 0;
      }
      else
      {
        set_b_guessed(Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeJmp(Word Code)
{
  adr_vals_t adr_vals;
  Boolean AddrRel = Hi(Code) != 0;
  Code &= 0xff;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdrRel(&ArgStr[1], &adr_vals, MModAbs + MModIReg + MModBRel + MModRegRel, AddrRel))
    {
      case ModAbs:
        PutCode(0x80 + Code);
        append_adr_vals(&adr_vals);
        break;
      case ModIReg:
        PutCode(0x90 + Code);
        append_adr_vals(&adr_vals);
        break;
      case ModBRel:
        PutCode(0xa0 + Code);
        append_adr_vals(&adr_vals);
        break;
      case ModRegRel:
        PutCode(0xf4e0 + Code);
        append_adr_vals(&adr_vals);
        break;
      default:
        break;
    }
  }
}

static void DecodeABReg(Word Code)
{
  int IsDJNZ = Hi(Code) & 1;
  Boolean IsStack = (Code & 0x200) || False;

  Code &= 0xff;

  if (!ChkArgCnt(1 + IsDJNZ, 1 + IsDJNZ));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "ST"))
  {
    if (IsStack)
    {
      BAsmCode[0] = 0xf3 + Code;
      CodeLen = 1;
    }
    else
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  }
  else
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAccA + MModAccB + MModReg))
    {
      case ModAccA:
        PutCode(0xb0 + Code);
        break;
      case ModAccB:
        PutCode(0xc0 + Code);
        break;
      case ModReg:
        PutCode(0xd0 + Code);
        append_adr_vals(&adr_vals);
        break;
      default:
        break;
    }
    if (IsDJNZ && (CodeLen != 0))
    {
      Boolean OK;
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], Int16, &OK, &Flags) - (EProgCounter() + CodeLen + 1);

      if (!OK)
        CodeLen = 0;
      else if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128)))
      {
        WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
        CodeLen = 0;
      }
      else
      {
        set_b_guessed(Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeBit(Word Code)
{
  int Rela = Hi(Code);
  LongWord BitExpr;

  Code &= 0xff;

  if (ChkArgCnt(1 + Rela, 2 + Rela)
   && DecodeBitExpr(1, ArgCnt - Rela, &BitExpr))
  {
    Boolean OK;
    Word Addr;
    Byte Bit;

    DissectBitValue(BitExpr, &Addr, &Bit);

    BAsmCode[1] = 1 << Bit;
    BAsmCode[2] = Lo(Addr);
    switch (Hi(Addr))
    {
      case 0x00:
        BAsmCode[0] = 0x70 + Code;
        CodeLen = 3;
        break;
      case 0x10:
        BAsmCode[0] = 0xa0 + Code;
        CodeLen = 3;
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt - 1]);
    }
    if ((CodeLen != 0) && Rela)
    {
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int16, &OK, &Flags) - (EProgCounter() + CodeLen + 1);

      if (!OK)
        CodeLen = 0;
      else if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128)))
      {
        WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[ArgCnt]);
        CodeLen = 0;
      }
      else
      {
        set_b_guessed(Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeDIV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    if ((DecodeAdr(&ArgStr[2], &adr_vals, MModAccA) != ModNone)
     && (DecodeAdr(&ArgStr[1], &adr_vals, MModReg) != ModNone))
    {
      PutCode(0xf4f8);
      append_adr_vals(&adr_vals);
    }
  }
}

static void DecodeINCW(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t dest_adr_vals, src_adr_vals;

    if ((DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg) != ModNone)
     && (DecodeAdr(&ArgStr[1], &src_adr_vals, MModImm) != ModNone))
    {
      PutCode(0x70);
      append_adr_vals(&src_adr_vals);
      append_adr_vals(&dest_adr_vals);
    }
  }
}

static void DecodeLDST(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[1], &adr_vals, MModImm) != ModNone)
    {
      PutCode(0xf0);
      append_adr_vals(&adr_vals);
    }
  }
}

static void DecodeTRAP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags flags;

    BAsmCode[0] = EvalStrIntExpressionWithFlags(&ArgStr[1], Int4, &OK, &flags);
    if (OK)
    {
      set_b_guessed(flags, 0, 1, 0x0f);
      BAsmCode[0] = 0xef - BAsmCode[0];
      CodeLen = 1;
    }
  }
}

static void DecodeTST(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAccA + MModAccB))
    {
      case ModAccA:
        PutCode(0xb0);
        break;
      case ModAccB:
        PutCode(0xc6);
        break;
      default:
        break;
    }
  }
}

/****************************************************************************/

static void InitFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void InitRel8(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel8);
}

static void InitALU1(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU1);
}

static void InitALU2(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU2);
}

static void InitJmp(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void InitABReg(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeABReg);
}

static void InitBit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);

  add_null_pseudo(InstTable);

  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MOVW", 0, DecodeMOVW);
  AddInstTable(InstTable, "CMP", 0, DecodeCMP);
  AddInstTable(InstTable, "DIV", 0, DecodeDIV);
  AddInstTable(InstTable, "INCW", 0, DecodeINCW);
  AddInstTable(InstTable, "LDST", 0, DecodeLDST);
  AddInstTable(InstTable, "TRAP", 0, DecodeTRAP);
  AddInstTable(InstTable, "TST", 0, DecodeTST);
  AddInstTable(InstTable, "DBIT", 0, DecodeDBIT);

  InitFixed("CLRC" , 0x00b0); InitFixed("DINT" , 0xf000);
  InitFixed("EINT" , 0xf00c); InitFixed("EINTH", 0xf004);
  InitFixed("EINTL", 0xf008); InitFixed("IDLE" , 0x00f6);
  InitFixed("LDSP" , 0x00fd); InitFixed("NOP"  , 0x00ff);
  InitFixed("RTI"  , 0x00fa); InitFixed("RTS"  , 0x00f9);
  InitFixed("SETC" , 0x00f8); InitFixed("STSP" , 0x00fe);

  InitRel8("JMP", 0x00); InitRel8("JC" , 0x03); InitRel8("JEQ", 0x02);
  InitRel8("JG" , 0x0e); InitRel8("JGE", 0x0d); InitRel8("JHS", 0x0b);
  InitRel8("JL" , 0x09); InitRel8("JLE", 0x0a); InitRel8("JLO", 0x0f);
  InitRel8("JN" , 0x01); InitRel8("JNC", 0x07); InitRel8("JNE", 0x06);
  InitRel8("JNV", 0x0c); InitRel8("JNZ", 0x06); InitRel8("JP" , 0x04);
  InitRel8("JPZ", 0x05); InitRel8("JV" , 0x08); InitRel8("JZ" , 0x02);

  InitALU1("ADC",  9); InitALU1("ADD",  8);
  InitALU1("DAC", 14); InitALU1("DSB", 15);
  InitALU1("SBB", 11); InitALU1("SUB", 10); InitALU1("MPY", 12);

  InitALU2("AND" ,  3); InitALU2("BTJO",  0x0106);
  InitALU2("BTJZ",  0x0107); InitALU2("OR"  ,  4); InitALU2("XOR",  5);

  InitJmp("BR"  , 12); InitJmp("CALL" , 14);
  InitJmp("JMPL", 0x0109); InitJmp("CALLR", 0x010f);

  InitABReg("CLR"  ,  5); InitABReg("COMPL", 11); InitABReg("DEC"  ,  2);
  InitABReg("INC"  ,  3); InitABReg("INV"  ,  4); InitABReg("POP"  , 0x0209);
  InitABReg("PUSH" , 0x0208); InitABReg("RL"   , 14); InitABReg("RLC"  , 15);
  InitABReg("RR"   , 12); InitABReg("RRC"  , 13); InitABReg("SWAP" ,  7);
  InitABReg("XCHB" ,  6); InitABReg("DJNZ" , 0x010a);

  InitBit("CMPBIT",  5); InitBit("JBIT0" ,  0x0107); InitBit("JBIT1" ,  0x0106);
  InitBit("SBIT0" ,  3); InitBit("SBIT1" ,  4);

  AddIntelPseudo(InstTable, eIntPseudoFlag_BigEndian);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/****************************************************************************/

static void MakeCode_370(void)
{
  OpSize = eSymbolSize8Bit;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_370(void)
{
  return (Memo("DBIT"));
}

static void InternSymbol_370(char *Asc,  TempResult *Erg)
{
  char *p_end, prefix;
  LargeInt Num;
  unsigned base;

  as_tempres_set_none(Erg);
  if ((strlen(Asc) < 2) || ((as_toupper(*Asc) != 'R') && (as_toupper(*Asc) != 'P')))
    return;
  prefix = *Asc++;

  if ((*Asc == '0') && (strlen(Asc) > 1))
  {
    base = 16;
    Asc++;
  }
  else
    base = 10;
  Num = strtoul(Asc, &p_end, base);
  if (*p_end || (Num < 0) || (Num > 255))
    return;

  if (as_toupper(prefix) == 'P')
    Num += 0x1000;
  as_tempres_set_int(Erg, Num);
}

static void SwitchFrom_370(void)
{
  DeinitFields();
}

static void SwitchTo_370(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x49;
  NOPCode = 0xff;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode ] = 1;
  ListGrans[SegCode ] = 1;
  SegInits[SegCode ] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_370;
  IsDef = IsDef_370;
  SwitchFrom = SwitchFrom_370;
  InternSymbol = InternSymbol_370;
  DissectBit = DissectBit_370;

  InitFields();
}

void code370_init(void)
{
  CPU37010 = AddCPU("370C010" , SwitchTo_370);
  CPU37020 = AddCPU("370C020" , SwitchTo_370);
  CPU37030 = AddCPU("370C030" , SwitchTo_370);
  CPU37040 = AddCPU("370C040" , SwitchTo_370);
  CPU37050 = AddCPU("370C050" , SwitchTo_370);
}
