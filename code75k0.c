/* code75k0.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator NEC 75K0                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"
#include "intformat.h"

#include "code75k0.h"

enum
{
  ModNone  = -1,
  ModReg4  = 0,
  ModReg8  = 1,
  ModImm  = 2 ,
  ModInd  = 3,
  ModAbs  = 4
};

#define MModReg4 (1 << ModReg4)
#define MModReg8 (1 << ModReg8)
#define MModImm (1 << ModImm)
#define MModInd (1 << ModInd)
#define MModAbs (1 << ModAbs)
#define MModMinOneIs0 (1 << 7)

enum
{
  eCore402 = 0,
  eCore004 = 1,
  eCore104 = 2
};

typedef struct
{
  char Name[6];
  Byte CoreType;
} tCPUProps;

typedef struct
{
  Byte Part;
  ShortInt Mode;
  tSymbolFlags PartSymFlags;
} tAdrResult;

static LongInt MBSValue, MBEValue;
static const tCPUProps *pCurrCPUProps;

static ShortInt OpSize;

/*-------------------------------------------------------------------------*/
/* Untermengen von Befehlssatz abpruefen */

static void CheckCore(Byte MinCore)
{
  if (pCurrCPUProps->CoreType < MinCore)
  {
    WrError(ErrNum_InstructionNotSupported);
    CodeLen = 0;
  }
}

static Boolean CheckACore(Byte MinCore)
{
  if (pCurrCPUProps->CoreType < MinCore)
  {
    WrError(ErrNum_AddrModeNotSupported);
    return False;
  }
  return True;
}

/*-------------------------------------------------------------------------*/
/* Adressausdruck parsen */

static Boolean SetOpSize(ShortInt NewSize)
{
  if (OpSize == -1)
    OpSize = NewSize;
  else if (NewSize != OpSize)
  {
    WrError(ErrNum_ConfOpSizes);
    return False;
  }
  return True;
}

static void ChkDataPage(Word Adr)
{
  switch (MBEValue)
  {
    case 0:
      if ((Adr > 0x7f) && (Adr < 0xf80))
        WrError(ErrNum_InAccPage);
      break;
    case 1:
      if (Hi(Adr) != MBSValue)
        WrError(ErrNum_InAccPage);
      break;
  }
}

static void CheckMBE(void)
{
  if ((pCurrCPUProps->CoreType == eCore402) && (MBEValue != 0))
  {
    MBEValue = 0;
    WrError(ErrNum_InvCtrlReg);
  }
}

static Boolean DecodeAdr(const tStrComp *pArg, Byte Mask, tAdrResult *pResult)
{
  static const char RegNames[] = "XAHLDEBC";

  const char *p;
  int pos, ArgLen = strlen(pArg->str.p_str);
  Boolean OK;
  tEvalResult EvalResult;
  String s;

  pResult->Mode = ModNone;
  pResult->PartSymFlags = eSymbolFlag_None;

  /* Register ? */

  memcpy(s, pArg->str.p_str, 2);
  s[2] = '\0';
  NLS_UpString(s);
  p = strstr(RegNames, s);

  if (p)
  {
    pos = p - RegNames;

    /* 8-Bit-Register ? */

    if (ArgLen == 1)
    {
      pResult->Part = pos ^ 1;
      if (SetOpSize(0))
      {
        if ((pResult->Part > 4) && !CheckACore(eCore004));
        else
          pResult->Mode = ModReg4;
      }
      goto chk;
    }

    /* 16-Bit-Register ? */

    if ((ArgLen == 2) && (!Odd(pos)))
    {
      pResult->Part = pos;
      if (SetOpSize(1))
      {
        if ((pResult->Part > 2) && !CheckACore(eCore004));
        else
          pResult->Mode = ModReg8;
      }
      goto chk;
    }

    /* 16-Bit-Schattenregister ? */

    if ((ArgLen == 3) && ((pArg->str.p_str[2] == '\'') || (pArg->str.p_str[2] == '`')) && (!Odd(pos)))
    {
      pResult->Part = pos + 1;
      if (SetOpSize(1))
      {
        if (CheckACore(eCore104))
          pResult->Mode = ModReg8;
      }
      goto chk;
    }
  }

  /* immediate? */

  if (*pArg->str.p_str == '#')
  {
    if ((OpSize == -1) && (Mask & MModMinOneIs0))
      OpSize = 0;
    switch (OpSize)
    {
      case -1:
        WrError(ErrNum_UndefOpSizes);
        OK = False;
        break;
      case 0:
        pResult->Part = EvalStrIntExpressionOffs(pArg, 1, Int4, &OK) & 15;
        break;
      case 1:
        pResult->Part = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        break;
    }
    if (OK)
      pResult->Mode = ModImm;
    goto chk;
  }

  /* indirekt ? */

  if (*pArg->str.p_str == '@')
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    if (!as_strcasecmp(Arg.str.p_str, "HL")) pResult->Part = 1;
    else if (!as_strcasecmp(Arg.str.p_str, "HL+")) pResult->Part = 2;
    else if (!as_strcasecmp(Arg.str.p_str, "HL-")) pResult->Part = 3;
    else if (!as_strcasecmp(Arg.str.p_str, "DE")) pResult->Part = 4;
    else if (!as_strcasecmp(Arg.str.p_str, "DL")) pResult->Part = 5;
    else
      pResult->Part = 0;
    if (pResult->Part != 0)
    {
      if ((pResult->Part != 1) && !CheckACore(eCore004));
      else if (((pResult->Part == 2) || (pResult->Part == 3)) && !CheckACore(eCore104));
      else
        pResult->Mode = ModInd;
      goto chk;
    }
  }

  /* absolut */

  pos = EvalStrIntExpressionWithResult(pArg, UInt12, &EvalResult);
  if (EvalResult.OK)
  {
    pResult->Part = Lo(pos);
    pResult->Mode = ModAbs;
    ChkSpace(SegData, EvalResult.AddrSpaceMask);
    if (!mFirstPassUnknown(EvalResult.Flags))
      ChkDataPage(pos);
    pResult->PartSymFlags = EvalResult.Flags;
  }

chk:
  if ((pResult->Mode != ModNone) && (!(Mask & (1 << pResult->Mode))))
  {
    WrError(ErrNum_InvAddrMode);
    pResult->Mode = ModNone;
  }
  return pResult->Mode != ModNone;
}

/* Bit argument coding:

   aaaa01bbaaaaaaaa -> aaaaaaaaaaaa.bb
           11bbxxxx -> 0ffxh.bb
           10bbxxxx -> 0fbxh.bb
           00bbxxxx -> @h+x.bb
           0100xxxx -> 0fc0h+(x*4).@l
 */

/*!------------------------------------------------------------------------
 * \fn     DissectBit_75K0(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_75K0(char *pDest, size_t DestSize, LargeWord Inp)
{
  if (Hi(Inp))
    as_snprintf(pDest, DestSize, "%~03.*u%s.%c",
                ListRadixBase, (unsigned)(((Inp >> 4) & 0xf00) + Lo(Inp)), GetIntConstIntelSuffix(ListRadixBase),
                '0' + (Hi(Inp) & 3));
  else switch ((Inp >> 6) & 3)
  {
    case 0:
      as_snprintf(pDest, DestSize, "@%c+%0.*u%s.%c",
                  HexStartCharacter + ('H' - 'A'),
                  ListRadixBase, (unsigned)(Inp & 0x0f), GetIntConstIntelSuffix(ListRadixBase),
                  '0' + ((Inp >> 4) & 3));
      break;
    case 1:
      as_snprintf(pDest, DestSize, "%~03.*u%s.@%c",
                  ListRadixBase, (unsigned)(0xfc0 + ((Inp & 0x0f) << 2)), GetIntConstIntelSuffix(ListRadixBase),
                  HexStartCharacter + ('L' - 'A'));
      break;
    case 2:
      as_snprintf(pDest, DestSize, "%~03.*u%s.%c",
                  ListRadixBase, (unsigned)(0xfb0 + (Inp & 15)), GetIntConstIntelSuffix(ListRadixBase),
                  '0' + ((Inp >> 4) & 3));
      break;
    case 3:
      as_snprintf(pDest, DestSize, "%~03.*u%s.%c",
                  ListRadixBase, (unsigned)(0xff0 + (Inp & 15)), GetIntConstIntelSuffix(ListRadixBase),
                  '0' + ((Inp >> 4) & 3));
      break;
  }
}

static Boolean DecodeBitAddr(const tStrComp *pArg, Word *Erg, tEvalResult *pEvalResult)
{
  char *p;
  int Num;
  Boolean OK;
  Word Adr;
  tStrComp AddrPart, BitPart;

  p = QuotPos(pArg->str.p_str, '.');
  if (!p)
  {
    *Erg = EvalStrIntExpressionWithResult(pArg, Int16, pEvalResult);
    if (Hi(*Erg) != 0)
      ChkDataPage(((*Erg >> 4) & 0xf00) + Lo(*Erg));
    return pEvalResult->OK;
  }

  StrCompSplitRef(&AddrPart, &BitPart, pArg, p);

  if (!as_strcasecmp(BitPart.str.p_str, "@L"))
  {
    Adr = EvalStrIntExpressionWithResult(&AddrPart, UInt12, pEvalResult);
    if (mFirstPassUnknown(pEvalResult->Flags))
      Adr = (Adr & 0xffc) | 0xfc0;
    if (pEvalResult->OK)
    {
      ChkSpace(SegData, pEvalResult->AddrSpaceMask);
      if ((Adr & 3) != 0) WrError(ErrNum_NotAligned);
      else if (Adr < 0xfc0) WrError(ErrNum_UnderRange);
      else if (CheckACore(eCore004))
      {
        *Erg = 0x40 + ((Adr & 0x3c) >> 2);
        return True;
      }
    }
  }
  else
  {
    Num = EvalStrIntExpression(&BitPart, UInt2, &OK);
    if (OK)
    {
      if (!as_strncasecmp(AddrPart.str.p_str, "@H", 2))
      {
        Adr = EvalStrIntExpressionOffsWithResult(&AddrPart, 2, UInt4, pEvalResult);
        if (pEvalResult->OK)
        {
          if (CheckACore(eCore004))
          {
            *Erg = (Num << 4) + Adr;
            return True;
          }
        }
      }
      else
      {
        Adr = EvalStrIntExpressionWithResult(&AddrPart, UInt12, pEvalResult);
        if (mFirstPassUnknown(pEvalResult->Flags))
          Adr = (Adr | 0xff0);
        if (pEvalResult->OK)
        {
          ChkSpace(SegData, pEvalResult->AddrSpaceMask);
          if ((Adr >= 0xfb0) && (Adr < 0xfc0))
            *Erg = 0x80 + (Num << 4) + (Adr & 15);
          else if (Adr >= 0xff0)
            *Erg = 0xc0 + (Num << 4) + (Adr & 15);
          else
            *Erg = 0x400 + (((Word)Num) << 8) + Lo(Adr) + (Hi(Adr) << 12);
          return True;
        }
      }
    }
  }
  return False;
}

static Boolean DecodeIntName(char *Asc, Byte *Erg)
{
  Word HErg;
  Byte LPart;
  String Asc_N;

  strmaxcpy(Asc_N, Asc, STRINGSIZE);
  NLS_UpString(Asc_N);
  Asc = Asc_N;

  if (pCurrCPUProps->CoreType <= eCore402)
    LPart = 0;
  else if (pCurrCPUProps->CoreType < eCore004)
    LPart = 1;
  else if (pCurrCPUProps->CoreType < eCore104)
    LPart = 2;
  else
    LPart = 3;
       if (!strcmp(Asc, "IEBT"))   HErg = 0x000;
  else if (!strcmp(Asc, "IEW"))    HErg = 0x102;
  else if (!strcmp(Asc, "IETPG"))  HErg = 0x203;
  else if (!strcmp(Asc, "IET0"))   HErg = 0x104;
  else if (!strcmp(Asc, "IECSI"))  HErg = 0x005;
  else if (!strcmp(Asc, "IECSIO")) HErg = 0x205;
  else if (!strcmp(Asc, "IE0"))    HErg = 0x006;
  else if (!strcmp(Asc, "IE2"))    HErg = 0x007;
  else if (!strcmp(Asc, "IE4"))    HErg = 0x120;
  else if (!strcmp(Asc, "IEKS"))   HErg = 0x123;
  else if (!strcmp(Asc, "IET1"))   HErg = 0x224;
  else if (!strcmp(Asc, "IE1"))    HErg = 0x126;
  else if (!strcmp(Asc, "IE3"))    HErg = 0x227;
  else
    HErg = 0xfff;
  if (HErg == 0xfff)
    return False;
  else if (Hi(HErg) > LPart)
    return False;
  else
  {
    *Erg = Lo(HErg);
    return True;
  }
}

static void PutCode(Word Code)
{
  BAsmCode[0] = Lo(Code);
  if (Hi(Code) == 0) CodeLen = 1;
  else
  {
    BAsmCode[1] = Hi(Code); CodeLen = 2;
  }
}

static Word PageNum(Word Inp)
{
  return ((Inp >> 12) & 15);
}

/*-------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    PutCode(Code);
}

static void DecodeMOV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8 | MModInd | MModAbs, &DestResult);
    switch (DestResult.Mode)
    {
      case ModReg4:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg4 | MModInd | MModAbs | MModImm, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg4:
            if (DestResult.Part == 0)
            {
              PutCode(0x7899 + (((Word)SrcResult.Part) << 8)); CheckCore(eCore004);
            }
            else if (SrcResult.Part == 0)
            {
              PutCode(0x7099 + (((Word)DestResult.Part) << 8)); CheckCore(eCore004);
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModInd:
            if (DestResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else PutCode(0xe0 + SrcResult.Part);
            break;
          case ModAbs:
            if (DestResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xa3; BAsmCode[1] = SrcResult.Part; CodeLen = 2;
            }
            break;
          case ModImm:
            if (DestResult.Part == 0) PutCode(0x70 + SrcResult.Part);
            else
            {
              PutCode(0x089a + (((Word)SrcResult.Part) << 12) + (((Word)DestResult.Part) << 8));
              CheckCore(eCore004);
            }
            break;
        }
        break;
      }
      case ModReg8:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg8 | MModAbs | MModInd | MModImm, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg8:
            if (DestResult.Part == 0)
            {
              PutCode(0x58aa + (((Word)SrcResult.Part) << 8)); CheckCore(eCore004);
            }
            else if (SrcResult.Part == 0)
            {
              PutCode(0x50aa + (((Word)DestResult.Part) << 8)); CheckCore(eCore004);
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModAbs:
            if (DestResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xa2; BAsmCode[1] = SrcResult.Part; CodeLen = 2;
              if (!mFirstPassUnknown(SrcResult.PartSymFlags) && Odd(SrcResult.Part)) WrError(ErrNum_AddrNotAligned);
            }
            break;
          case ModInd:
            if ((DestResult.Part != 0) || (SrcResult.Part != 1)) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x18aa); CheckCore(eCore004);
            }
            break;
          case ModImm:
            if (Odd(DestResult.Part)) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x89 + DestResult.Part; BAsmCode[1] = SrcResult.Part; CodeLen = 2;
            }
            break;
        }
        break;
      }
      case ModInd:
        if (DestResult.Part != 1) WrError(ErrNum_InvAddrMode);
        else
        {
          tAdrResult SrcResult;

          DecodeAdr(&ArgStr[2], MModReg4 | MModReg8, &SrcResult);
          switch (SrcResult.Mode)
          {
            case ModReg4:
              if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0xe8); CheckCore(eCore004);
              }
              break;
            case ModReg8:
              if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10aa); CheckCore(eCore004);
              }
              break;
          }
        }
        break;
      case ModAbs:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg4 | MModReg8, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg4:
            if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x93; BAsmCode[1] = DestResult.Part; CodeLen = 2;
            }
            break;
          case ModReg8:
            if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x92; BAsmCode[1] = DestResult.Part; CodeLen = 2;
              if (!mFirstPassUnknown(SrcResult.PartSymFlags) && Odd(DestResult.Part)) WrError(ErrNum_AddrNotAligned);
            }
            break;
        }
        break;
      }
    }
  }
}

static void DecodeXCH(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8 | MModAbs | MModInd, &DestResult);
    switch (DestResult.Mode)
    {
      case ModReg4:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg4 | MModAbs | MModInd, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg4:
            if (DestResult.Part == 0) PutCode(0xd8 + SrcResult.Part);
            else if (SrcResult.Part == 0) PutCode(0xd8 + DestResult.Part);
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModAbs:
            if (DestResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xb3; BAsmCode[1] = SrcResult.Part; CodeLen = 2;
            }
            break;
          case ModInd:
            if (DestResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else PutCode(0xe8 + SrcResult.Part);
            break;
        }
        break;
      }
      case ModReg8:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg8 | MModAbs | MModInd, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg8:
            if (DestResult.Part == 0)
            {
              PutCode(0x40aa + (((Word)SrcResult.Part) << 8)); CheckCore(eCore004);
            }
            else if (SrcResult.Part == 0)
            {
              PutCode(0x40aa + (((Word)DestResult.Part) << 8)); CheckCore(eCore004);
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModAbs:
            if (DestResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xb2; BAsmCode[1] = SrcResult.Part; CodeLen = 2;
              if (mFirstPassUnknown(SrcResult.PartSymFlags) && Odd(SrcResult.Part)) WrError(ErrNum_AddrNotAligned);
            }
            break;
          case ModInd:
            if ((SrcResult.Part != 1) || (DestResult.Part != 0)) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x11aa); CheckCore(eCore004);
            }
            break;
        }
        break;
      }
      case ModAbs:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg4 | MModReg8, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg4:
            if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xb3; BAsmCode[1] = DestResult.Part; CodeLen = 2;
            }
            break;
          case ModReg8:
            if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xb2; BAsmCode[1] = DestResult.Part; CodeLen = 2;
              if (mFirstPassUnknown(SrcResult.PartSymFlags) && Odd(DestResult.Part)) WrError(ErrNum_AddrNotAligned);
            }
            break;
        }
        break;
      }
      case ModInd:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModReg4 | MModReg8, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg4:
            if (SrcResult.Part != 0) WrError(ErrNum_InvAddrMode);
            else PutCode(0xe8 + DestResult.Part);
            break;
          case ModReg8:
            if ((SrcResult.Part != 0) || (DestResult.Part != 1)) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x11aa);
              CheckCore(eCore004);
            }
            break;
        }
        break;
      }
    }
  }
}

static void DecodeMOVT(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "XA")) WrError(ErrNum_InvAddrMode);
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "@PCDE"))
  {
    PutCode(0xd4);
    CheckCore(eCore004);
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "@PCXA"))
    PutCode(0xd0);
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodePUSH_POP(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "BS"))
  {
    PutCode(0x0699 + (Code << 8)); CheckCore(eCore004);
  }
  else
  {
    tAdrResult Result;

    DecodeAdr(&ArgStr[1], MModReg8, &Result);
    switch (Result.Mode)
    {
      case ModReg8:
        if (Odd(Result.Part)) WrError(ErrNum_InvAddrMode);
        else PutCode(0x48 + Code + Result.Part);
        break;
    }
  }
}

static void DecodeIN_OUT(Word IsIN)
{
  if (ChkArgCnt(2, 2))
  {
    const tStrComp *pPortArg = IsIN ? &ArgStr[2] : &ArgStr[1],
                   *pRegArg = IsIN ? &ArgStr[1] : &ArgStr[2];

    if (as_strncasecmp(pPortArg->str.p_str, "PORT", 4)) WrError(ErrNum_InvAddrMode);
    else
    {
      Boolean OK;

      BAsmCode[1] = 0xf0 + EvalStrIntExpressionOffs(pPortArg, 4, UInt4, &OK);
      if (OK)
      {
        tAdrResult Result;

        DecodeAdr(pRegArg, MModReg8 | MModReg4, &Result);
        switch (Result.Mode)
        {
          case ModReg4:
            if (Result.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x93 + (IsIN << 4); CodeLen = 2;
            }
            break;
          case ModReg8:
            if (Result.Part != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x92 + (IsIN << 4); CodeLen = 2;
              CheckCore(eCore004);
            }
            break;
        }
      }
    }
  }
}

static void DecodeADDS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8, &Result);
    switch (Result.Mode)
    {
      case ModReg4:
        if (Result.Part != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          DecodeAdr(&ArgStr[2], MModImm | MModInd, &Result);
          switch (Result.Mode)
          {
            case ModImm:
              PutCode(0x60 + Result.Part); break;
            case ModInd:
              if (Result.Part == 1) PutCode(0xd2); else WrError(ErrNum_InvAddrMode);
              break;
          }
        }
        break;
      case ModReg8:
        if (Result.Part == 0)
        {
          DecodeAdr(&ArgStr[2], MModReg8 | MModImm, &Result);
          switch (Result.Mode)
          {
            case ModReg8:
              PutCode(0xc8aa + (((Word)Result.Part) << 8));
              CheckCore(eCore104);
              break;
            case ModImm:
              BAsmCode[0] = 0xb9; BAsmCode[1] = Result.Part;
              CodeLen = 2;
              CheckCore(eCore104);
              break;
          }
        }
        else if (as_strcasecmp(ArgStr[2].str.p_str, "XA")) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0xc0aa + (((Word)Result.Part) << 8));
          CheckCore(eCore104);
        }
        break;
    }
  }
}

static void DecodeAri(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8, &Result);
    switch (Result.Mode)
    {
      case ModReg4:
        if (Result.Part != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          DecodeAdr(&ArgStr[2], MModInd, &Result);
          switch (Result.Mode)
          {
            case ModInd:
              if (Result.Part == 1)
              {
                BAsmCode[0] = 0xa8;
                if (Code == 0) BAsmCode[0]++;
                if (Code == 2) BAsmCode[0] += 0x10;
                CodeLen = 1;
                if (Code != 0)
                  CheckCore(eCore004);
              }
              else
                WrError(ErrNum_InvAddrMode);
             break;
          }
        }
        break;
      case ModReg8:
        if (Result.Part == 0)
        {
          DecodeAdr(&ArgStr[2], MModReg8, &Result);
          switch (Result.Mode)
          {
            case ModReg8:
              PutCode(0xc8aa + ((Code + 1) << 12) + (((Word)Result.Part) << 8));
              CheckCore(eCore104);
              break;
          }
        }
        else if (as_strcasecmp(ArgStr[2].str.p_str, "XA")) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0xc0aa + ((Code + 1) << 12) + (((Word)Result.Part) << 8));
          CheckCore(eCore104);
        }
        break;
    }
  }
}

static void DecodeLog(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8, &Result);
    switch (Result.Mode)
    {
      case ModReg4:
        if (Result.Part != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          DecodeAdr(&ArgStr[2], MModImm | MModInd, &Result);
          switch (Result.Mode)
          {
            case ModImm:
              PutCode(0x2099 + (((Word)Result.Part & 15) << 8) + ((Code + 1) << 12));
              CheckCore(eCore004);
              break;
            case ModInd:
              if (Result.Part == 1)
                PutCode(0x80 + ((Code + 1) << 4));
              else
                WrError(ErrNum_InvAddrMode);
              break;
          }
        }
        break;
      case ModReg8:
        if (Result.Part == 0)
        {
          DecodeAdr(&ArgStr[2], MModReg8, &Result);
          switch (Result.Mode)
          {
            case ModReg8:
              PutCode(0x88aa + (((Word)Result.Part) << 8) + ((Code + 1) << 12));
              CheckCore(eCore104);
              break;
          }
        }
        else if (as_strcasecmp(ArgStr[2].str.p_str, "XA")) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x80aa + (((Word)Result.Part) << 8) + ((Code + 1) << 12));
          CheckCore(eCore104);
        }
        break;
    }
  }
}

static void DecodeLog1(Word Code)
{
  Word BVal;
  tEvalResult EvalResult;

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
  else if (DecodeBitAddr(&ArgStr[2], &BVal, &EvalResult))
  {
    if (Hi(BVal) != 0) WrError(ErrNum_InvAddrMode);
    else
    {
      BAsmCode[0] = 0xac + ((Code & 1) << 1) + ((Code & 2) << 3);
      BAsmCode[1] = BVal;
      CodeLen = 2;
    }
  }
}

static void DecodeINCS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8 | MModInd | MModAbs, &Result);
    switch (Result.Mode)
    {
      case ModReg4:
        PutCode(0xc0 + Result.Part);
        break;
      case ModReg8:
        if ((Result.Part < 1) || (Odd(Result.Part))) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x88 + Result.Part);
          CheckCore(eCore104);
        }
        break;
      case ModInd:
        if (Result.Part == 1)
        {
          PutCode(0x0299);
          CheckCore(eCore004);
        }
        else
          WrError(ErrNum_InvAddrMode);
        break;
      case ModAbs:
        BAsmCode[0] = 0x82;
        BAsmCode[1] = Result.Part;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeDECS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8, &Result);
    switch (Result.Mode)
    {
      case ModReg4:
        PutCode(0xc8 + Result.Part);
        break;
      case ModReg8:
        PutCode(0x68aa + (((Word)Result.Part) << 8));
        CheckCore(eCore104);
        break;
    }
  }
}

static void DecodeSKE(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    DecodeAdr(&ArgStr[1], MModReg4 | MModReg8 | MModInd, &DestResult);
    switch (DestResult.Mode)
    {
      case ModReg4:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModImm | MModInd | MModReg4, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg4:
            if (DestResult.Part == 0)
            {
              PutCode(0x0899 + (((Word)SrcResult.Part) << 8));
              CheckCore(eCore004);
            }
            else if (SrcResult.Part == 0)
            {
              PutCode(0x0899 + (((Word)DestResult.Part) << 8));
              CheckCore(eCore004);
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModImm:
            BAsmCode[0] = 0x9a;
            BAsmCode[1] = (SrcResult.Part << 4) + DestResult.Part;
            CodeLen = 2;
            break;
          case ModInd:
            if ((SrcResult.Part == 1) && (DestResult.Part == 0))
              PutCode(0x80);
            else
              WrError(ErrNum_InvAddrMode);
            break;
        }
        break;
      }
      case ModReg8:
      {
        tAdrResult SrcResult;

        DecodeAdr(&ArgStr[2], MModInd | MModReg8, &SrcResult);
        switch (SrcResult.Mode)
        {
          case ModReg8:
            if (DestResult.Part == 0)
            {
              PutCode(0x48aa + (((Word)SrcResult.Part) << 8));
              CheckCore(eCore104);
            }
            else if (SrcResult.Part == 0)
            {
              PutCode(0x48aa + (((Word)DestResult.Part) << 8));
              CheckCore(eCore104);
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModInd:
            if (SrcResult.Part == 1)
            {
              PutCode(0x19aa);
              CheckCore(eCore104);
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
        }
        break;
      }
      case ModInd:
        if (DestResult.Part != 1) WrError(ErrNum_InvAddrMode);
        else
        {
          tAdrResult SrcResult;

          DecodeAdr(&ArgStr[2], MModImm | MModReg4 | MModReg8 | MModMinOneIs0, &SrcResult);
          switch (SrcResult.Mode)
          {
            case ModImm:
              PutCode(0x6099 + (((Word)SrcResult.Part) << 8));
              CheckCore(eCore004);
              break;
            case ModReg4:
              if (SrcResult.Part == 0)
                PutCode(0x80);
              else
                WrError(ErrNum_InvAddrMode);
              break;
            case ModReg8:
              if (SrcResult.Part == 0)
              {
                PutCode(0x19aa);
                CheckCore(eCore004);
              }
              else
                WrError(ErrNum_InvAddrMode);
              break;
          }
        }
        break;
    }
  }
}

static void DecodeAcc(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrError(ErrNum_InvAddrMode);
  else
    PutCode(Code);
}

static void DecodeMOV1(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean OK = True;
    Word BVal;
    tEvalResult EvalResult;

    if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
      Code = 0xbd;
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "CY"))
      Code = 0x9b;
    else OK = False;
    if (!OK) WrError(ErrNum_InvAddrMode);
    else if (DecodeBitAddr(&ArgStr[((Code >> 2) & 3) - 1], &BVal, &EvalResult))
    {
      if (Hi(BVal) != 0) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = BVal;
        CodeLen = 2;
        CheckCore(eCore104);
      }
    }
  }
}

static void DecodeSET1_CLR1(Word Code)
{
  Word BVal;
  tEvalResult EvalResult;

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
    PutCode(0xe6 + Code);
  else if (DecodeBitAddr(&ArgStr[1], &BVal, &EvalResult))
  {
    if (Hi(BVal) != 0)
    {
      BAsmCode[0] = 0x84 + Code + (Hi(BVal & 0x300) << 4);
      BAsmCode[1] = Lo(BVal);
      CodeLen = 2;
    }
    else
    {
      BAsmCode[0] = 0x9c + Code;
      BAsmCode[1] = BVal;
      CodeLen = 2;
    }
  }
}

static void DecodeSKT_SKF(Word Code)
{
  Word BVal;
  tEvalResult EvalResult;

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    if (Code)
      PutCode(0xd7);
    else
      WrError(ErrNum_InvAddrMode);
  }
  else if (DecodeBitAddr(&ArgStr[1], &BVal, &EvalResult))
  {
    if (Hi(BVal) != 0)
    {
      BAsmCode[0] = 0x86 + Code + (Hi(BVal & 0x300) << 4);
      BAsmCode[1] = Lo(BVal);
      CodeLen = 2;
    }
    else
    {
      BAsmCode[0] = 0xbe + Code;
      BAsmCode[1] = BVal;
      CodeLen = 2;
    }
  }
}

static void DecodeNOT1(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
  else
    PutCode(0xd6);
}

static void DecodeSKTCLR(Word Code)
{
  Word BVal;
  tEvalResult EvalResult;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (DecodeBitAddr(&ArgStr[1], &BVal, &EvalResult))
  {
    if (Hi(BVal) != 0) WrError(ErrNum_InvAddrMode);
    else
    {
      BAsmCode[0] = 0x9f;
      BAsmCode[1] = BVal;
      CodeLen = 2;
    }
  }
}

static void DecodeBR(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "PCDE"))
  {
    PutCode(0x0499);
    CheckCore(eCore004);
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "PCXA"))
  {
    BAsmCode[0] = 0x99;
    BAsmCode[1] = 0x00;
    CodeLen = 2;
    CheckCore(eCore104);
  }
  else
  {
    Integer AdrInt, Dist;
    Boolean BrRel, BrLong;
    unsigned Offset = 0;
    tEvalResult EvalResult;

    BrRel = False;
    BrLong = False;
    if (ArgStr[1].str.p_str[Offset] == '$')
    {
      BrRel = True;
      Offset++;
    }
    else if (ArgStr[1].str.p_str[Offset] == '!')
    {
      BrLong = True;
      Offset++;
    }
    AdrInt = EvalStrIntExpressionOffsWithResult(&ArgStr[1], Offset, UInt16, &EvalResult);
    if (EvalResult.OK)
    {
      Dist = AdrInt - EProgCounter();
      if ((BrRel) || ((Dist <= 16) && (Dist >= -15) && (Dist != 0)))
      {
        if (Dist > 0)
        {
          Dist--;
          if ((Dist > 15) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
          else
            PutCode(0x00 + Dist);
        }
        else
        {
          if ((Dist < -15) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
          else
            PutCode(0xf0 + 15 + Dist);
        }
      }
      else if ((!BrLong) && (PageNum(AdrInt) == PageNum(EProgCounter())) && ((EProgCounter() & 0xfff) < 0xffe))
      {
        BAsmCode[0] = 0x50 + ((AdrInt >> 8) & 15);
        BAsmCode[1] = Lo(AdrInt);
        CodeLen = 2;
      }
      else
      {
        BAsmCode[0] = 0xab;
        BAsmCode[1] = Hi(AdrInt & 0x3fff);
        BAsmCode[2] = Lo(AdrInt);
        CodeLen = 3;
        CheckCore(eCore004);
      }
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeBRCB(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult);

    if (EvalResult.OK)
    {
      if (!ChkSamePage(AdrInt, EProgCounter(), 12, EvalResult.Flags));
      else if ((EProgCounter() & 0xfff) >= 0xffe) WrError(ErrNum_NotFromThisAddress);
      else
      {
        BAsmCode[0] = 0x50 + ((AdrInt >> 8) & 15);
        BAsmCode[1] = Lo(AdrInt);
        CodeLen = 2;
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      }
    }
  }
}

static void DecodeCALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    unsigned BrLong = !!(*ArgStr[1].str.p_str == '!');
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionOffsWithResult(&ArgStr[1], BrLong, UInt16, &EvalResult);
    if (mFirstPassUnknown(EvalResult.Flags)) AdrInt &= 0x7ff;
    if (EvalResult.OK)
    {
      if ((BrLong) || (AdrInt > 0x7ff))
      {
        BAsmCode[0] = 0xab;
        BAsmCode[1] = 0x40 + Hi(AdrInt & 0x3fff);
        BAsmCode[2] = Lo(AdrInt);
        CodeLen = 3;
        CheckCore(eCore004);
      }
      else
      {
        BAsmCode[0] = 0x40 + Hi(AdrInt & 0x7ff);
        BAsmCode[1] = Lo(AdrInt);
        CodeLen = 2;
      }
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeCALLF(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionOffsWithResult(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt11, &EvalResult);
    if (EvalResult.OK)
    {
      BAsmCode[0] = 0x40 + Hi(AdrInt);
      BAsmCode[1] = Lo(AdrInt);
      CodeLen = 2;
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeGETI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt6, &OK);
    CodeLen = Ord(OK);
    CheckCore(eCore004);
  }
}

static void DecodeEI_DI(Word Code)
{
  Byte HReg;

  if (ArgCnt == 0)
    PutCode(0xb29c + Code);
  else if (!ChkArgCnt(1, 1));
  else if (DecodeIntName(ArgStr[1].str.p_str, &HReg))
    PutCode(0x989c + Code + (((Word)HReg) << 8));
  else
    WrError(ErrNum_InvCtrlReg);
}

static void DecodeSEL(Word Code)
{
  Boolean OK;

  UNUSED(Code);

  BAsmCode[0] = 0x99;
  if (!ChkArgCnt(1, 1));
  else if (!as_strncasecmp(ArgStr[1].str.p_str, "RB", 2))
  {
    BAsmCode[1] = 0x20 + EvalStrIntExpressionOffs(&ArgStr[1], 2, UInt2, &OK);
    if (OK)
    {
      CodeLen = 2;
      CheckCore(eCore104);
    }
  }
  else if (!as_strncasecmp(ArgStr[1].str.p_str, "MB", 2))
  {
    BAsmCode[1] = 0x10 + EvalStrIntExpressionOffs(&ArgStr[1], 2, UInt4, &OK);
    if (OK)
    {
      CodeLen = 2;
      CheckCore(eCore004);
    }
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, 0xfff);
}

static void DecodeBIT(Word Code)
{
  Word BErg;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;

    if (DecodeBitAddr(&ArgStr[1], &BErg, &EvalResult))
      if (!mFirstPassUnknown(EvalResult.Flags))
      {
        PushLocHandle(-1);
        EnterIntSymbol(&LabPart, BErg, SegBData, False);
        *ListLine = '=';
        DissectBit_75K0(ListLine + 1,  STRINGSIZE- 1, BErg);
        PopLocHandle();
      }
  }
}

/*-------------------------------------------------------------------------*/
/* dynamische Codetabellenverwaltung */

static void AddFixed(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeFixed);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(103);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "MOVT", 0, DecodeMOVT);
  AddInstTable(InstTable, "PUSH", 1, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 0, DecodePUSH_POP);
  AddInstTable(InstTable, "IN", 1, DecodeIN_OUT);
  AddInstTable(InstTable, "OUT", 0, DecodeIN_OUT);
  AddInstTable(InstTable, "ADDS", 0, DecodeADDS);
  AddInstTable(InstTable, "INCS", 0, DecodeINCS);
  AddInstTable(InstTable, "DECS", 0, DecodeDECS);
  AddInstTable(InstTable, "SKE", 0, DecodeSKE);
  AddInstTable(InstTable, "RORC", 0x98, DecodeAcc);
  AddInstTable(InstTable, "NOT", 0x5f99, DecodeAcc);
  AddInstTable(InstTable, "MOV1", 0, DecodeMOV1);
  AddInstTable(InstTable, "SET1", 1, DecodeSET1_CLR1);
  AddInstTable(InstTable, "CLR1", 0, DecodeSET1_CLR1);
  AddInstTable(InstTable, "SKT", 1, DecodeSKT_SKF);
  AddInstTable(InstTable, "SKF", 0, DecodeSKT_SKF);
  AddInstTable(InstTable, "NOT1", 0, DecodeNOT1);
  AddInstTable(InstTable, "SKTCLR", 0, DecodeSKTCLR);
  AddInstTable(InstTable, "BR", 0, DecodeBR);
  AddInstTable(InstTable, "BRCB", 0, DecodeBRCB);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "CALLF", 0, DecodeCALLF);
  AddInstTable(InstTable, "GETI", 0, DecodeGETI);
  AddInstTable(InstTable, "EI", 1, DecodeEI_DI);
  AddInstTable(InstTable, "DI", 0, DecodeEI_DI);
  AddInstTable(InstTable, "SEL", 0, DecodeSEL);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);

  AddFixed("RET" , 0x00ee);
  AddFixed("RETS", 0x00e0);
  AddFixed("RETI", 0x00ef);
  AddFixed("HALT", 0xa39d);
  AddFixed("STOP", 0xb39d);
  AddFixed("NOP" , 0x0060);

  InstrZ = 0;
  AddInstTable(InstTable, "ADDC", InstrZ++, DecodeAri);
  AddInstTable(InstTable, "SUBS", InstrZ++, DecodeAri);
  AddInstTable(InstTable, "SUBC", InstrZ++, DecodeAri);

  InstrZ = 0;
  AddInstTable(InstTable, "AND", InstrZ++, DecodeLog);
  AddInstTable(InstTable, "OR" , InstrZ++, DecodeLog);
  AddInstTable(InstTable, "XOR", InstrZ++, DecodeLog);

  InstrZ = 0;
  AddInstTable(InstTable, "AND1", InstrZ++, DecodeLog1);
  AddInstTable(InstTable, "OR1" , InstrZ++, DecodeLog1);
  AddInstTable(InstTable, "XOR1", InstrZ++, DecodeLog1);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_75K0(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = -1;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_75K0(void)
{
  MBSValue = 0; MBEValue = 0;
}

static Boolean IsDef_75K0(void)
{
  return ((Memo("SFR")) || (Memo("BIT")));
}

static void SwitchFrom_75K0(void)
{
  DeinitFields();
}

static ASSUMERec ASSUME75s[] =
{
  {"MBS", &MBSValue, 0, 0x0f, 0x10, NULL},
  {"MBE", &MBEValue, 0, 0x01, 0x01, CheckMBE}
};
#define ASSUME75Count (sizeof(ASSUME75s) / sizeof(*ASSUME75s))

static void SwitchTo_75K0(void *pUser)
{
  Boolean Err;
  Word ROMEnd;

  pCurrCPUProps = (const tCPUProps*)pUser;
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "PC"; HeaderID = 0x7b; NOPCode = 0x60;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode)|(1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  SegLimits[SegData] = 0xfff;
  ROMEnd = ConstLongInt(&MomCPUName[3], &Err, 10);
  if (ROMEnd > 2)
    ROMEnd %= 10;
  SegLimits[SegCode] = (ROMEnd << 10) - 1;

  pASSUMERecs = ASSUME75s;
  ASSUMERecCnt = ASSUME75Count;

  MakeCode = MakeCode_75K0; IsDef = IsDef_75K0;
  SwitchFrom = SwitchFrom_75K0; InitFields();
  DissectBit = DissectBit_75K0;
}

static const tCPUProps CPUProps[] =
{
  { "75402", eCore402 },
  { "75004", eCore004 },
  { "75006", eCore004 },
  { "75008", eCore004 },
  { "75268", eCore004 },
  { "75304", eCore004 },
  { "75306", eCore004 },
  { "75308", eCore004 },
  { "75312", eCore004 },
  { "75316", eCore004 },
  { "75328", eCore004 },
  { "75104", eCore104 },
  { "75106", eCore104 },
  { "75108", eCore104 },
  { "75112", eCore104 },
  { "75116", eCore104 },
  { "75206", eCore104 },
  { "75208", eCore104 },
  { "75212", eCore104 },
  { "75216", eCore104 },
  { "75512", eCore104 },
  { "75516", eCore104 },
  { ""     , 0        }
};

void code75k0_init(void)
{
  const tCPUProps *pProps;

  for (pProps = CPUProps; pProps->Name[0]; pProps++)
    (void)AddCPUUser(pProps->Name, SwitchTo_75K0, (void*)pProps, NULL);

  AddInitPassProc(InitCode_75K0);
}
