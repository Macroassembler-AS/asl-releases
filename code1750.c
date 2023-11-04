/* code1750.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator MIL-STD-1750                                               */
/*                                                                           */
/* This generator is heavily based on the as1750 assembler written by        */
/* Oliver M. Kellogg, Dornier Satellite Systems.  Yes, I know, it's been     */
/* floating around on my hard drive for almost two decades before I got my   */
/* a** up to finally do it.  But maybe someone still reads it...             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "be_le.h"
#include "ieeefloat.h"
#include "errmsg.h"

#include "code1750.h"

/*-------------------------------------------------------------------------*/

typedef struct
{
  const char *pName;
  Word Code;
} tCondition;

static CPUVar CPU1750;
static Word AdrReg, AdrWord;

/*-------------------------------------------------------------------------*/
/* Utility Functions */

static void PutCode(Word Code)
{
  WAsmCode[CodeLen++] = Code;
}

static Boolean DecodeReg(const char *pAsc, Word *pResult)
{
  if (toupper(*pAsc) != 'R')
    return False;
  pAsc++;

  *pResult = 0;
  while (*pAsc)
  {
    if (!isdigit(*pAsc))
      return False;
    *pResult = (*pResult * 10) + (*pAsc - '0');
    if (*pResult > 15)
      return False;
    pAsc++;
  }
  return True;
}

static Boolean DecodeBaseReg(const char *pAsc, Word *pResult)
{
  if ((toupper(*pAsc) != 'R') && (toupper(*pAsc) != 'B'))
    return False;
  pAsc++;

  *pResult = 0;
  while (*pAsc)
  {
    if (!isdigit(*pAsc))
      return False;
    *pResult = (*pResult * 10) + (*pAsc - '0');
    if (*pResult > 15)
      return False;
    pAsc++;
  }
  if (*pResult < 12)
    return False;
  *pResult -= 12;
  return True;
}

static Boolean DecodeArgReg(unsigned Index, Word *pResult)
{
  Boolean Result = DecodeReg(ArgStr[Index].str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[Index]);
  return Result;
}

static Boolean DecodeArgBaseReg(unsigned Index, Word *pResult)
{
  Boolean Result = DecodeBaseReg(ArgStr[Index].str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[Index]);
  return Result;
}

static Boolean DecodeAdr(int StartIdx, int StopIdx)
{
  Boolean OK;

  AdrWord = EvalStrIntExpression(&ArgStr[StartIdx], Int16, &OK);
  if (!OK)
    return False;

  if (StopIdx > StartIdx)
  {
    OK = False;
    if (!DecodeArgReg(StartIdx + 1, &AdrReg));
    else if (AdrReg == 0)
      WrXErrorPos(ErrNum_InvAddrMode, "!R0", &ArgStr[StartIdx + 1].Pos);
    else
      OK = True;
    return OK;
  }
  else
    AdrReg = 0;
  return OK;
}

static Boolean DecodeCondition(const char *pAsc, Word *pResult)
{
  static const tCondition Conditions[] =
  {
    { "LT",  0x1 },		/* 0001 */
    { "LZ",  0x1 },		/* 0001 */
    { "EQ",  0x2 },		/* 0010 */
    { "EZ",  0x2 },		/* 0010 */
    { "LE",  0x3 },		/* 0011 */
    { "LEZ", 0x3 },		/* 0011 */
    { "GT",  0x4 },		/* 0100 */
    { "GTZ", 0x4 },		/* 0100 */
    { "NE",  0x5 },		/* 0101 */
    { "NZ",  0x5 },		/* 0101 */
    { "GE",  0x6 },		/* 0110 */
    { "GEZ", 0x6 },		/* 0110 */
    { "ALL", 0x7 },		/* 0111 */
    { "CY",  0x8 },		/* 1000 */
    { "CLT", 0x9 },		/* 1001 */
    { "CEQ", 0xA },		/* 1010 */
    { "CEZ", 0xA },		/* 1010 */
    { "CLE", 0xB },		/* 1011 */
    { "CGT", 0xC },		/* 1100 */
    { "CNZ", 0xD },		/* 1101 */
    { "CGE", 0xE },		/* 1110 */
    { "UC",  0xF },		/* 1111 */
    { NULL,  0x0 },
  };
  const tCondition *pCond;

  for (pCond = Conditions; pCond->pName; pCond++)
    if (!as_strcasecmp(pAsc, pCond->pName))
    {
      *pResult = pCond->Code;
      return True;
    }
  return False;
}

static Boolean DecodeArgCondition(unsigned Index, Word *pResult)
{
  Boolean Result = DecodeCondition(ArgStr[Index].str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_UndefCond, &ArgStr[Index]);
  return Result;
}

static Boolean DecodeArgXIOCmd(unsigned Index, Word *pResult)
{
  static const tCondition XIO[] =
  {
    { "SMK",  0x2000 },
    { "CLIR", 0x2001 },
    { "ENBL", 0x2002 },
    { "DSBL", 0x2003 },
    { "RPI",  0x2004 },
    { "SPI",  0x2005 },
    { "OD",   0x2008 },
    { "RNS",  0x200A },
    { "WSW",  0x200E },
    { "CO",   0x4000 },
    { "CLC",  0x4001 },
    { "MPEN", 0x4003 },
    { "ESUR", 0x4004 },
    { "DSUR", 0x4005 },
    { "DMAE", 0x4006 },
    { "DMAD", 0x4007 },
    { "TAS",  0x4008 },
    { "TAH",  0x4009 },
    { "OTA",  0x400A },
    { "GO",   0x400B },
    { "TBS",  0x400C },
    { "TBH",  0x400D },
    { "OTB",  0x400E },
    { "LMP",  0x5000 },
    { "WIPR", 0x5100 },
    { "WOPR", 0x5200 },
    { "RMP",  0xD000 },
    { "RIPR", 0xD100 },
    { "ROPR", 0xD200 },
    { "RMK",  0xA000 },
    { "RIC1", 0xA001 },
    { "RIC2", 0xA002 },
    { "RPIR", 0xA004 },
    { "RDOR", 0xA008 },
    { "RDI",  0xA009 },
    { "TPIO", 0xA00B },
    { "RMFS", 0xA00D },
    { "RSW",  0xA00E },
    { "RCFR", 0xA00F },
    { "CI",   0xC000 },
    { "RCS",  0xC001 },
    { "ITA",  0xC00A },
    { "ITB",  0xC00E },
    { NULL,   0xFFFF }
  };
  const tCondition *pRun;
  Boolean OK;

  if (isalpha(ArgStr[Index].str.p_str[0]))
  {
    for (pRun = XIO; pRun->pName; pRun++)
      if (!as_strcasecmp(ArgStr[Index].str.p_str, pRun->pName))
      {
        *pResult = pRun->Code;
        return True;
      }
  }
  *pResult = EvalStrIntExpression(&ArgStr[Index], UInt16, &OK);
  return OK;
}

/*-------------------------------------------------------------------------*/
/* Code Generators */

static void DecodeNone(Word Code)
{
  if (ChkArgCnt(0, 0))
    PutCode(Code);
}

static void DecodeR(Word Code)
{
  Word Ra, Rb;

  if (ChkArgCnt(2, 2)
   && DecodeArgReg(1, &Ra)
   && DecodeArgReg(2, &Rb))
    PutCode(Code | (Ra << 4) | Rb);
}

static void DecodeRImm(Word Code)
{
  Word N, Rb;
  Boolean OK;

  if (ChkArgCnt(2, 2) && DecodeArgReg(1, &Rb))
  {
    N = EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
    if (OK)
      PutCode(Code | (N << 4) | Rb);
  }
}

static void DecodeIS(Word Code)
{
  Word N, Ra;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(2, 2) && DecodeArgReg(1, &Ra))
  {
    N = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt5, &OK, &Flags);
    if (OK && mFirstPassUnknown(Flags))
      N = 1;
    if (OK && ChkRange(N, 1, 16))
      PutCode(Code | (Ra << 4) | (N - 1));
  }
}

static void DecodeMem(Word Code)
{
  Word Ra;

  if (ChkArgCnt(2, 3)
   && DecodeArgReg(1, &Ra)
   && DecodeAdr(2, ArgCnt))
  {
    PutCode(Code | (Ra << 4) | AdrReg);
    PutCode(AdrWord);
  }
}

static void DecodeImOcx(Word Code)
{
  Word Ra;

  if (ChkArgCnt(2, 2) && DecodeArgReg(1, &Ra))
  {
    Boolean OK;
    Word ImmVal = EvalStrIntExpression(&ArgStr[2], Int16, &OK);

    if (OK)
    {
      PutCode(Code | (Ra << 4));
      PutCode(ImmVal);
    }
  }
}

static void DecodeB(Word Code)
{
  Word Br;

  if (ChkArgCnt(2, 2) && DecodeArgBaseReg(1, &Br))
  {
    Boolean OK;
    Word LoByte = EvalStrIntExpression(&ArgStr[2], UInt8, &OK);

    if (OK)
      PutCode(Code | (Br << 8) | LoByte);
  }
}

static void DecodeBX(Word Code)
{
  Word Br, Rx;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeArgBaseReg(1, &Br));
  else if (!DecodeArgReg(2, &Rx));
  else if (0 == Rx) WrXErrorPos(ErrNum_InvAddrMode, "!R0", &ArgStr[2].Pos);
  else
    PutCode(Code | (Br << 8) | Rx);
}

static void DecodeICR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LargeInt Diff = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags) - EProgCounter();

    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((Diff < -128) || (Diff > 127))) WrError(ErrNum_JmpDistTooBig);
      else
        PutCode(Code | (Diff & 0xff));
    }
  }
}

static void DecodeS(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Value = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

    if (OK)
      PutCode(Code | Value);
  }
}

static void DecodeIM1_16(Word Code)
{
  if (ChkArgCnt(2, 3) && DecodeAdr(2, ArgCnt))
  {
    Boolean OK;
    Word N;
    tSymbolFlags Flags;

    N = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt5, &OK, &Flags);
    if (OK && mFirstPassUnknown(Flags))
      N = 1;

    if (OK && ChkRange(N, 1, 16))
    {
      PutCode(Code | ((N - 1) << 4) | AdrReg);
      PutCode(AdrWord);
    }
  }
}

static void DecodeXMem(Word Code)
{
  Word Ra;

  if (ChkArgCnt(2, 3)
   && DecodeArgReg(1, &Ra)
   && DecodeAdr(2, ArgCnt))
  {
    PutCode(Code | (Ra << 4) | AdrReg);
    PutCode(AdrWord);
  }
}

static void DecodeImmR(Word Code)
{
  Word Rb, N;
  Boolean OK;

  if (ChkArgCnt(2, 2) && DecodeArgReg(2, &Rb))
  {
    N = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

    if (OK)
      PutCode(Code | (N << 4) | Rb);
  }
}

static void DecodeJump(Word Code)
{
  Word Cond;

  if (ChkArgCnt(2, 3)
   && DecodeArgCondition(1, &Cond)
   && DecodeAdr(2, ArgCnt))
  {
    PutCode(Code | (Cond << 4) | AdrReg);
    PutCode(AdrWord);
  }
}

static void DecodeAddr(Word Code)
{
  if (ChkArgCnt(1, 2) && DecodeAdr(1, ArgCnt))
  {
    PutCode(Code | AdrReg);
    PutCode(AdrWord);
  }
}

static void DecodeIM_0_15(Word Code)
{
  if (ChkArgCnt(2, 3) && DecodeAdr(2, ArgCnt))
  {
    Boolean OK;
    Word N = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

    if (OK)
    {
      PutCode(Code | (N << 4) | AdrReg);
      PutCode(AdrWord);
    }
  }
}

static void DecodeSR(Word Code)
{
  Word R;

  if (ChkArgCnt(1, 1)  && DecodeArgReg(1, &R))
    PutCode(Code | (R << 4));
}

static void DecodeXIO(Word Code)
{
  Word Ra, Cmd;

  if (ChkArgCnt(2, 3)
   && DecodeArgReg(1, &Ra)
   && DecodeArgXIOCmd(2, &Cmd))
  {
    if (ArgCnt == 3)
    {
      Word Ri;

      if (!DecodeArgReg(3, &Ri));
      else if (Ri == 0) WrXErrorPos(ErrNum_InvAddrMode, "!R0", &ArgStr[3].Pos);
      else
      {
        PutCode(Code | (Ra << 4) | Ri);
        PutCode(Cmd);
      }
    }
    else
    {
      PutCode(Code | (Ra << 4));
      PutCode(Cmd);
    }
  }
}

static void ShiftMantRight(Byte Field[8])
{
  int Index;

  for (Index = 7; Index >= 1; Index--)
    Field[Index] = ((Field[Index] >> 1) & 0x7f) | ((Field[Index - 1] & 1) << 7);
}

static void ShiftMantLeft(Byte Field[8])
{
  int Index;

  for (Index = 1; Index <= 7; Index++)
  {
    Field[Index] = ((Field[Index] & 0x7f) << 1);
    if (Index < 7)
      Field[Index] |= (Field[Index + 1] >> 7) & 0x01;
  }
}

static void DecodeFLOAT(Word Extended)
{
  int z;
  Boolean OK;
  Double Value;
  Byte Field[8];
  Byte Sign;
  Word Exponent, Word0, Word1, Word2;
  Integer SignedExponent;

  if (!ChkArgCnt(1, ArgCntMax))
    return;

  for (z = 1; z <= ArgCnt; z++)
  {
    Value = EvalStrFloatExpression(&ArgStr[z], Float64, &OK);
    if (!OK)
      break;

    /* get binary representation, big endian */

    Double_2_ieee8(Value, Field, True);
#if 0
    printf("Field 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
           Field[0], Field[1], Field[2], Field[3], Field[4], Field[5], Field[6], Field[7]);
#endif

    /* split off sign & exponent */

    Sign = (Field[0] > 0x7f);
    Exponent = (((Word) Field[0] & 0x7f) << 4) + (Field[1] >> 4);
    Field[0] = 0;
    Field[1] &= 0x0f;

#if 0
    printf("Sign %u Exponent %u Mantissa 0x%02x%02x%02x%02x%02x%02x%02x\n", Sign, Exponent,
           Field[1], Field[2], Field[3], Field[4], Field[5], Field[6], Field[7]);
#endif

    /* 1750 format has no implicit leading one in mantissa; i.e. mantissa can only
       represent values of 1 > mant >= -1. If number is not denormalized, we have to
       increase the exponent and shift the mantissa by one to the right: */

    if (Exponent > 0)
    {
      Field[1] |= 0x10; /* make leading one explicit */
      Exponent++;
      ShiftMantRight(Field);
    }

    /* If exponent is too small to represent, shift down mantissa
       until exponent is large enough, or mantissa is all-zeroes: */

    while (Exponent < 1023 - 128)
    {
      Exponent++;
      ShiftMantRight(Field);
      /* todo: regord only relevant bits of mantissa */
      if (!(Field[1] | Field[2] | Field[3] | Field[4] | Field[5] | Field[6] | Field[7]))
      {
        Exponent = 0;
        break;
      }
    }
    SignedExponent = Exponent - 1023;

    /* exponent overflow? */

    if (SignedExponent > 127)
    {
      WrError(ErrNum_OverRange);
      break;
    }

#if 0
    printf("Sign %u SignedExponent %d Mantissa 0x%02x%02x%02x%02x%02x%02x%02x\n", Sign, SignedExponent,
           Field[1], Field[2], Field[3], Field[4], Field[5], Field[6], Field[7]);
#endif

    /* form 2s complement of mantissa when sign is set */

    if (Sign)
    {
      /* 2s complement range is asymmetric: we can represent -1.0 in the
         mantissa, but not +1.0.  So if the mantissa is +0.5, and the
         exponent is not at minimum, convert it to +1.0 and decrement the
         exponent: */

      if ((Field[1] == 0x08) && !Field[2] && !Field[3] && !Field[4]
       && !Field[5] && !Field[6] && !Field[7] && (SignedExponent > -127))
      {
        ShiftMantLeft(Field);
        SignedExponent--;
      }

      Field[7] ^= 0xff;
      Field[6] ^= 0xff;
      Field[5] ^= 0xff;
      Field[4] ^= 0xff;
      Field[3] ^= 0xff;
      Field[2] ^= 0xff;
      Field[1] ^= 0x1f;
      if (!(++Field[7]))
        if (!(++Field[6]))
          if (!(++Field[5]))
            if (!(++Field[4]))
              if (!(++Field[3]))
                if (!(++Field[2]))
                  Field[1] = (Field[1] + 1) & 0x1f;
    }

    /* assemble mantissa */
    /* TODO: mantissa rounding? */

    Word0 = ((Word)(Field[1] & 0x1f) << 11) | ((Word)Field[2] << 3) | ((Field[3] >> 5) & 0x07);
    Word1 = ((Word)(Field[3] & 0x1f) << 11) | ((Word)(Field[4] & 0xe0) << 3);
    if (Extended)
      Word2 = ((Word)(Field[4] & 0x1f) << 11) | ((Word)Field[5] << 3) | ((Field[6] >> 5) & 0x07);
    else
      Word2 = 0;

    /* zero mantissa means zero exponent */

    if (!Word0 && !Word1 && !Word2);
    else
      Word1 |= (SignedExponent & 0xff);

    /* now copy the mantissa to the destination */

    PutCode(Word0);
    PutCode(Word1);
    if (Extended)
      PutCode(Word2);
  }
}

static void DecodeDATA_1750(Word Code)
{
  UNUSED(Code);

  DecodeDATA(UInt16, UInt16);
}

/*-------------------------------------------------------------------------*/
/* dynamic code table handling */

static void InitFields(void)
{
  InstTable = CreateInstTable(403);

  AddInstTable(InstTable, "AISP", 0xA200, DecodeIS);
  AddInstTable(InstTable, "AIM",  0x4A01, DecodeImOcx);
  AddInstTable(InstTable, "AR",   0xA100, DecodeR);
  AddInstTable(InstTable, "A",    0xA000, DecodeMem);
  AddInstTable(InstTable, "ANDM", 0x4A07, DecodeImOcx);
  AddInstTable(InstTable, "ANDR", 0xE300, DecodeR);
  AddInstTable(InstTable, "AND",  0xE200, DecodeMem);
  AddInstTable(InstTable, "ABS",  0xA400, DecodeR);
  AddInstTable(InstTable, "AB",   0x1000, DecodeB);
  AddInstTable(InstTable, "ANDB", 0x3400, DecodeB);
  AddInstTable(InstTable, "ABX",  0x4040, DecodeBX);
  AddInstTable(InstTable, "ANDX", 0x40E0, DecodeBX);
  AddInstTable(InstTable, "BEZ",  0x7500, DecodeICR);
  AddInstTable(InstTable, "BNZ",  0x7A00, DecodeICR);
  AddInstTable(InstTable, "BGT",  0x7900, DecodeICR);
  AddInstTable(InstTable, "BLE",  0x7800, DecodeICR);
  AddInstTable(InstTable, "BGE",  0x7B00, DecodeICR);
  AddInstTable(InstTable, "BLT",  0x7600, DecodeICR);
  AddInstTable(InstTable, "BR",   0x7400, DecodeICR);
  AddInstTable(InstTable, "BEX",  0x7700, DecodeS);
  AddInstTable(InstTable, "BPT",  0xFFFF, DecodeNone);
  AddInstTable(InstTable, "BIF",  0x4F00, DecodeS);
  AddInstTable(InstTable, "CISP", 0xF200, DecodeIS);
  AddInstTable(InstTable, "CIM",  0x4A0A, DecodeImOcx);
  AddInstTable(InstTable, "CR",   0xF100, DecodeR);
  AddInstTable(InstTable, "C",    0xF000, DecodeMem);
  AddInstTable(InstTable, "CISN", 0xF300, DecodeIS);
  AddInstTable(InstTable, "CB",   0x3800, DecodeB);
  AddInstTable(InstTable, "CBL",  0xF400, DecodeMem);
  AddInstTable(InstTable, "CBX",  0x40C0, DecodeBX);
  AddInstTable(InstTable, "DLR",  0x8700, DecodeR);
  AddInstTable(InstTable, "DL",   0x8600, DecodeMem);
  AddInstTable(InstTable, "DST",  0x9600, DecodeMem);
  AddInstTable(InstTable, "DSLL", 0x6500, DecodeRImm);
  AddInstTable(InstTable, "DSRL", 0x6600, DecodeRImm);
  AddInstTable(InstTable, "DSRA", 0x6700, DecodeRImm);
  AddInstTable(InstTable, "DSLC", 0x6800, DecodeRImm);
  AddInstTable(InstTable, "DSLR", 0x6D00, DecodeR);
  AddInstTable(InstTable, "DSAR", 0x6E00, DecodeR);
  AddInstTable(InstTable, "DSCR", 0x6F00, DecodeR);
  AddInstTable(InstTable, "DECM", 0xB300, DecodeIM1_16);
  AddInstTable(InstTable, "DAR",  0xA700, DecodeR);
  AddInstTable(InstTable, "DA",   0xA600, DecodeMem);
  AddInstTable(InstTable, "DSR",  0xB700, DecodeR);
  AddInstTable(InstTable, "DS",   0xB600, DecodeMem);
  AddInstTable(InstTable, "DMR",  0xC700, DecodeR);
  AddInstTable(InstTable, "DM",   0xC600, DecodeMem);
  AddInstTable(InstTable, "DDR",  0xD700, DecodeR);
  AddInstTable(InstTable, "DD",   0xD600, DecodeMem);
  AddInstTable(InstTable, "DCR",  0xF700, DecodeR);
  AddInstTable(InstTable, "DC",   0xF600, DecodeMem);
  AddInstTable(InstTable, "DLB",  0x0400, DecodeB);
  AddInstTable(InstTable, "DSTB", 0x0C00, DecodeB);
  AddInstTable(InstTable, "DNEG", 0xB500, DecodeR);
  AddInstTable(InstTable, "DABS", 0xA500, DecodeR);
  AddInstTable(InstTable, "DR",   0xD500, DecodeR);
  AddInstTable(InstTable, "D",    0xD400, DecodeMem);
  AddInstTable(InstTable, "DISP", 0xD200, DecodeIS);
  AddInstTable(InstTable, "DIM",  0x4A05, DecodeImOcx);
  AddInstTable(InstTable, "DISN", 0xD300, DecodeIS);
  AddInstTable(InstTable, "DVIM", 0x4A06, DecodeImOcx);
  AddInstTable(InstTable, "DVR",  0xD100, DecodeR);
  AddInstTable(InstTable, "DV",   0xD000, DecodeMem);
  AddInstTable(InstTable, "DLI",  0x8800, DecodeMem);
  AddInstTable(InstTable, "DSTI", 0x9800, DecodeMem);
  AddInstTable(InstTable, "DB",   0x1C00, DecodeB);
  AddInstTable(InstTable, "DBX",  0x4070, DecodeBX);
  AddInstTable(InstTable, "DLBX", 0x4010, DecodeBX);
  AddInstTable(InstTable, "DSTX", 0x4030, DecodeBX);
  AddInstTable(InstTable, "DLE",  0xDF00, DecodeXMem);
  AddInstTable(InstTable, "DSTE", 0xDD00, DecodeXMem);
  AddInstTable(InstTable, "EFL",  0x8A00, DecodeMem);
  AddInstTable(InstTable, "EFST", 0x9A00, DecodeMem);
  AddInstTable(InstTable, "EFCR", 0xFB00, DecodeR);
  AddInstTable(InstTable, "EFC",  0xFA00, DecodeMem);
  AddInstTable(InstTable, "EFAR", 0xAB00, DecodeR);
  AddInstTable(InstTable, "EFA",  0xAA00, DecodeMem);
  AddInstTable(InstTable, "EFSR", 0xBB00, DecodeR);
  AddInstTable(InstTable, "EFS",  0xBA00, DecodeMem);
  AddInstTable(InstTable, "EFMR", 0xCB00, DecodeR);
  AddInstTable(InstTable, "EFM",  0xCA00, DecodeMem);
  AddInstTable(InstTable, "EFDR", 0xDB00, DecodeR);
  AddInstTable(InstTable, "EFD",  0xDA00, DecodeMem);
  AddInstTable(InstTable, "EFLT", 0xEB00, DecodeR);
  AddInstTable(InstTable, "EFIX", 0xEA00, DecodeR);
  AddInstTable(InstTable, "FAR",  0xA900, DecodeR);
  AddInstTable(InstTable, "FA",   0xA800, DecodeMem);
  AddInstTable(InstTable, "FSR",  0xB900, DecodeR);
  AddInstTable(InstTable, "FS",   0xB800, DecodeMem);
  AddInstTable(InstTable, "FMR",  0xC900, DecodeR);
  AddInstTable(InstTable, "FM",   0xC800, DecodeMem);
  AddInstTable(InstTable, "FDR",  0xD900, DecodeR);
  AddInstTable(InstTable, "FD",   0xD800, DecodeMem);
  AddInstTable(InstTable, "FCR",  0xF900, DecodeR);
  AddInstTable(InstTable, "FC",   0xF800, DecodeMem);
  AddInstTable(InstTable, "FABS", 0xAC00, DecodeR);
  AddInstTable(InstTable, "FIX",  0xE800, DecodeR);
  AddInstTable(InstTable, "FLT",  0xE900, DecodeR);
  AddInstTable(InstTable, "FNEG", 0xBC00, DecodeR);
  AddInstTable(InstTable, "FAB",  0x2000, DecodeB);
  AddInstTable(InstTable, "FABX", 0x4080, DecodeBX);
  AddInstTable(InstTable, "FSB",  0x2400, DecodeB);
  AddInstTable(InstTable, "FSBX", 0x4090, DecodeBX);
  AddInstTable(InstTable, "FMB",  0x2800, DecodeB);
  AddInstTable(InstTable, "FMBX", 0x40A0, DecodeBX);
  AddInstTable(InstTable, "FDB",  0x2C00, DecodeB);
  AddInstTable(InstTable, "FDBX", 0x40B0, DecodeBX);
  AddInstTable(InstTable, "FCB",  0x3C00, DecodeB);
  AddInstTable(InstTable, "FCBX", 0x40D0, DecodeBX);
  AddInstTable(InstTable, "INCM", 0xA300, DecodeIM1_16);
  AddInstTable(InstTable, "JC",   0x7000, DecodeJump);
  AddInstTable(InstTable, "J",    0x7400, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JEZ",  0x7500, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JLE",  0x7800, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JGT",  0x7900, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JNZ",  0x7A00, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JGE",  0x7B00, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JLT",  0x7600, DecodeICR);	/* TBD (GAS) */
  AddInstTable(InstTable, "JCI",  0x7100, DecodeJump);
  AddInstTable(InstTable, "JS",   0x7200, DecodeMem);
  AddInstTable(InstTable, "LISP", 0x8200, DecodeIS);
  AddInstTable(InstTable, "LIM",  0x8500, DecodeMem);
  AddInstTable(InstTable, "LR",   0x8100, DecodeR);
  AddInstTable(InstTable, "L",    0x8000, DecodeMem);
  AddInstTable(InstTable, "LISN", 0x8300, DecodeIS);
  AddInstTable(InstTable, "LB",   0x0000, DecodeB);
  AddInstTable(InstTable, "LBX",  0x4000, DecodeBX);
  AddInstTable(InstTable, "LSTI", 0x7C00, DecodeAddr);
  AddInstTable(InstTable, "LST",  0x7D00, DecodeAddr);
  AddInstTable(InstTable, "LI",   0x8400, DecodeMem);
  AddInstTable(InstTable, "LM",   0x8900, DecodeIM_0_15);
  AddInstTable(InstTable, "LUB",  0x8B00, DecodeMem);
  AddInstTable(InstTable, "LLB",  0x8C00, DecodeMem);
  AddInstTable(InstTable, "LUBI", 0x8D00, DecodeMem);
  AddInstTable(InstTable, "LLBI", 0x8E00, DecodeMem);
  AddInstTable(InstTable, "LE",   0xDE00, DecodeXMem);
  AddInstTable(InstTable, "MISP", 0xC200, DecodeIS);
  AddInstTable(InstTable, "MSIM", 0x4A04, DecodeImOcx);
  AddInstTable(InstTable, "MSR",  0xC100, DecodeR);
  AddInstTable(InstTable, "MS",   0xC000, DecodeMem);
  AddInstTable(InstTable, "MISN", 0xC300, DecodeIS);
  AddInstTable(InstTable, "MIM",  0x4A03, DecodeImOcx);
  AddInstTable(InstTable, "MR",   0xC500, DecodeR);
  AddInstTable(InstTable, "M",    0xC400, DecodeMem);
  AddInstTable(InstTable, "MOV",  0x9300, DecodeR);
  AddInstTable(InstTable, "MB",   0x1800, DecodeB);
  AddInstTable(InstTable, "MBX",  0x4060, DecodeBX);
  AddInstTable(InstTable, "NEG",  0xB400, DecodeR);
  AddInstTable(InstTable, "NOP",  0xFF00, DecodeNone);
  AddInstTable(InstTable, "NIM",  0x4A0B, DecodeImOcx);
  AddInstTable(InstTable, "NR",   0xE700, DecodeR);
  AddInstTable(InstTable, "N",    0xE600, DecodeMem);
  AddInstTable(InstTable, "ORIM", 0x4A08, DecodeImOcx);
  AddInstTable(InstTable, "ORR",  0xE100, DecodeR);
  AddInstTable(InstTable, "OR",   0xE000, DecodeMem);
  AddInstTable(InstTable, "ORB",  0x3000, DecodeB);
  AddInstTable(InstTable, "ORBX", 0x40F0, DecodeBX);
  AddInstTable(InstTable, "PSHM", 0x9F00, DecodeR);
  AddInstTable(InstTable, "POPM", 0x8F00, DecodeR);
  AddInstTable(InstTable, "RBR",  0x5400, DecodeImmR);
  AddInstTable(InstTable, "RVBR", 0x5C00, DecodeR);
  AddInstTable(InstTable, "RB",   0x5300, DecodeIM_0_15);
  AddInstTable(InstTable, "RBI",  0x5500, DecodeIM_0_15);
  AddInstTable(InstTable, "ST",   0x9000, DecodeMem);
  AddInstTable(InstTable, "STC",  0x9100, DecodeIM_0_15);
  AddInstTable(InstTable, "SISP", 0xB200, DecodeIS);
  AddInstTable(InstTable, "SIM",  0x4A02, DecodeImOcx);
  AddInstTable(InstTable, "SR",   0xB100, DecodeR);
  AddInstTable(InstTable, "S",    0xB000, DecodeMem);
  AddInstTable(InstTable, "SLL",  0x6000, DecodeRImm);
  AddInstTable(InstTable, "SRL",  0x6100, DecodeRImm);
  AddInstTable(InstTable, "SRA",  0x6200, DecodeRImm);
  AddInstTable(InstTable, "SLC",  0x6300, DecodeRImm);
  AddInstTable(InstTable, "SLR",  0x6A00, DecodeR);
  AddInstTable(InstTable, "SAR",  0x6B00, DecodeR);
  AddInstTable(InstTable, "SCR",  0x6C00, DecodeR);
  AddInstTable(InstTable, "SJS",  0x7E00, DecodeMem);
  AddInstTable(InstTable, "STB",  0x0800, DecodeB);
  AddInstTable(InstTable, "SBR",  0x5100, DecodeImmR);
  AddInstTable(InstTable, "SB",   0x5000, DecodeIM_0_15);
  AddInstTable(InstTable, "SVBR", 0x5A00, DecodeR);
  AddInstTable(InstTable, "SOJ",  0x7300, DecodeMem);
  AddInstTable(InstTable, "SBB",  0x1400, DecodeB);
  AddInstTable(InstTable, "STBX", 0x4020, DecodeBX);
  AddInstTable(InstTable, "SBBX", 0x4050, DecodeBX);
  AddInstTable(InstTable, "SBI",  0x5200, DecodeIM_0_15);
  AddInstTable(InstTable, "STZ",  0x9100, DecodeAddr);
  AddInstTable(InstTable, "STCI", 0x9200, DecodeIM_0_15);
  AddInstTable(InstTable, "STI",  0x9400, DecodeMem);
  AddInstTable(InstTable, "SFBS", 0x9500, DecodeR);
  AddInstTable(InstTable, "SRM",  0x9700, DecodeMem);
  AddInstTable(InstTable, "STM",  0x9900, DecodeIM_0_15);
  AddInstTable(InstTable, "STUB", 0x9B00, DecodeMem);
  AddInstTable(InstTable, "STLB", 0x9C00, DecodeMem);
  AddInstTable(InstTable, "SUBI", 0x9D00, DecodeMem);
  AddInstTable(InstTable, "SLBI", 0x9E00, DecodeMem);
  AddInstTable(InstTable, "STE",  0xDC00, DecodeXMem);
  AddInstTable(InstTable, "TBR",  0x5700, DecodeImmR);
  AddInstTable(InstTable, "TB",   0x5600, DecodeIM_0_15);
  AddInstTable(InstTable, "TBI",  0x5800, DecodeIM_0_15);
  AddInstTable(InstTable, "TSB",  0x5900, DecodeIM_0_15);
  AddInstTable(InstTable, "TVBR", 0x5E00, DecodeR);
  AddInstTable(InstTable, "URS",  0x7F00, DecodeSR);
  AddInstTable(InstTable, "UAR",  0xAD00, DecodeR);
  AddInstTable(InstTable, "UA",   0xAE00, DecodeMem);
  AddInstTable(InstTable, "USR",  0xBD00, DecodeR);
  AddInstTable(InstTable, "US",   0xBE00, DecodeMem);
  AddInstTable(InstTable, "UCIM", 0xF500, DecodeImOcx);
  AddInstTable(InstTable, "UCR",  0xFC00, DecodeR);
  AddInstTable(InstTable, "UC",   0xFD00, DecodeMem);
  AddInstTable(InstTable, "VIO",  0x4900, DecodeMem);
  AddInstTable(InstTable, "XORR", 0xE500, DecodeR);
  AddInstTable(InstTable, "XORM", 0x4A09, DecodeImOcx);
  AddInstTable(InstTable, "XOR",  0xE400, DecodeMem);
  AddInstTable(InstTable, "XWR",  0xED00, DecodeR);
  AddInstTable(InstTable, "XBR",  0xEC00, DecodeSR);
  AddInstTable(InstTable, "XIO",  0x4800, DecodeXIO);

  AddInstTable(InstTable, "FLOAT", 0, DecodeFLOAT);
  AddInstTable(InstTable, "EXTENDED", 1, DecodeFLOAT);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_1750);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/
/* interface to common layer */

static void MakeCode_1750(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_1750(void)
{
  return Memo("BIT");
}

static void SwitchFrom_1750(void)
{
  DeinitFields();
}

static void SwitchTo_1750(void)
{
  const TFamilyDescr *pDescr;

  pDescr = FindFamilyByName("1750");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = pDescr->Id;
  NOPCode = 0xff00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  ASSUMERecCnt = 0;

  MakeCode = MakeCode_1750;
  IsDef = IsDef_1750;
  SwitchFrom = SwitchFrom_1750; InitFields();
}

void code1750_init(void)
{
  CPU1750 = AddCPU("1750", SwitchTo_1750);
  AddCopyright("MIL-STD 1750 Generator also (C) 2019 Oliver Kellogg");
}
