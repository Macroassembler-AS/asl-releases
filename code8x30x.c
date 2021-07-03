/* code8x30x.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Signetics 8X30x                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "chunks.h"
#include "bpemu.h"
#include "strutil.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codevars.h"
#include "errmsg.h"

#include "code8x30x.h"

/*****************************************************************************/

static CPUVar CPU8x300, CPU8x305;

/*-------------------------------------------------------------------------*/

static Boolean DecodeReg(const tStrComp *pArg, Word *Erg, ShortInt *ErgLen)
{
  Boolean OK;
  Word Acc;
  LongInt Adr;
  char *z;
  int Len = strlen(pArg->str.p_str);

  *ErgLen = -1;

  if (!as_strcasecmp(pArg->str.p_str, "AUX"))
  {
    *Erg = 0;
    return True;
  }

  if (!as_strcasecmp(pArg->str.p_str, "OVF"))
  {
    *Erg = 8;
    return True;
  }

  if (!as_strcasecmp(pArg->str.p_str, "IVL"))
  {
    *Erg = 7;
    return True;
  }

  if (!as_strcasecmp(pArg->str.p_str, "IVR"))
  {
    *Erg = 15;
    return True;
  }

  if ((as_toupper(*pArg->str.p_str) == 'R') && (Len > 1) && (Len < 4))
  {
    Acc = 0;
    OK = True;
    for (z = pArg->str.p_str + 1; *z != '\0'; z++)
      if (OK)
      {
        if ((*z < '0') || (*z > '7'))
          OK = False;
        else
          Acc = (Acc << 3) + (*z - '0');
      }
    if ((OK) && (Acc < 32))
    {
      if ((MomCPU == CPU8x300) && (Acc > 9) && (Acc < 15))
      {
        WrStrErrorPos(ErrNum_InvReg, pArg);
        return False;
      }
      else *Erg = Acc;
      return True;
    }
  }

  if ((Len == 4) && (as_strncasecmp(pArg->str.p_str + 1, "IV", 2) == 0) && (pArg->str.p_str[3] >= '0') && (pArg->str.p_str[3] <= '7'))
  {
    if (as_toupper(*pArg->str.p_str) == 'L')
    {
      *Erg = pArg->str.p_str[3]-'0' + 0x10;
      return True;
    }
    else if (as_toupper(*pArg->str.p_str) == 'R')
    {
      *Erg = pArg->str.p_str[3] - '0' + 0x18;
      return True;
    }
  }

  /* IV - Objekte */

  Adr = EvalStrIntExpression(pArg, UInt24, &OK);
  if (OK)
  {
    *ErgLen = Adr & 7;
    *Erg = 0x10 | ((Adr & 0x10) >> 1) | ((Adr & 0x700) >> 8);
    return True;
  }
  else
    return False;
}

static char *HasDisp(char *Asc)
{
  int Lev;
  char *z;
  int l = strlen(Asc);

  if (Asc[l - 1] == ')')
  {
    z = Asc + l - 2;
    Lev = 0;
    while ((z >= Asc) && (Lev != -1))
    {
      switch (*z)
      {
        case '(':
          Lev--;
          break;
        case ')':
          Lev++;
          break;
      }
      if (Lev != -1)
        z--;
    }
    if (Lev != -1)
    {
      WrError(ErrNum_BrackErr);
      return NULL;
    }
  }
  else
    z = NULL;

  return z;
}

static Boolean GetLen(const tStrComp *pArg, Word *Erg, tSymbolFlags *pFlags)
{
  Boolean OK;

  *Erg = EvalStrIntExpressionWithFlags(pArg, UInt4, &OK, pFlags);
  if (!OK)
    return False;
  if (mFirstPassUnknown(*pFlags))
    *Erg = 8;
  if (!ChkRange(*Erg, 1, 8))
    return False;
  *Erg &= 7;
  return True;
}

/*-------------------------------------------------------------------------*/

static void DecodeNOP(Word Code)     /* NOP = MOVE AUX,AUX */
{
  UNUSED(Code);

  if (ChkArgCnt(0, 0))
  {
    WAsmCode[0] = 0x0000;
    CodeLen = 1;
  }
}

static void DecodeHALT(Word Code)      /* HALT = JMP * */
{
  UNUSED(Code);

  if (ChkArgCnt(0, 0))
  {
    WAsmCode[0] = 0xe000 | (EProgCounter() & 0x1fff);
    CodeLen = 1;
  }
}

static void DecodeXML_XMR(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU8x305))
  {
    Boolean OK;
    Word Adr = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      WAsmCode[0] = Code | (Adr & 0xff);
      CodeLen = 1;
    }
  }
}

static void DecodeSEL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    LongInt Op = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);
    if (OK)
    {
      WAsmCode[0] = 0xc700 | ((Op & 0x10) << 7) | ((Op >> 16) & 0xff);
      CodeLen = 1;
    }
  }
}

static void DecodeXMIT(Word Code)
{
  Word SrcReg, Rot;
  ShortInt SrcLen;
  Boolean OK;
  tSymbolFlags Flags;
  LongInt Adr;

  UNUSED(Code);

  if (ChkArgCnt(2, 3)
   && DecodeReg(&ArgStr[2], &SrcReg, &SrcLen))
  {
    if (SrcReg < 16)
    {
      if (ChkArgCnt(2, 2))
      {
        Adr = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
        if (OK)
        {
          WAsmCode[0] = 0xc000 | (SrcReg << 8) | (Adr & 0xff);
          CodeLen = 1;
        }
      }
    }
    else
    {
      if (ArgCnt == 2)
      {
        Rot = 0xffff; OK = True; Flags = eSymbolFlag_None;
      }
      else
        OK = GetLen(&ArgStr[3], &Rot, &Flags);
      if (OK)
      {
        if (Rot == 0xffff)
          Rot = (SrcLen == -1) ? 0 : SrcLen;
        if ((SrcLen != -1) && (Rot != SrcLen)) WrError(ErrNum_ConfOpSizes);
        else
        {
          Adr = EvalStrIntExpression(&ArgStr[1], Int5, &OK);
          if (OK)
          {
            WAsmCode[0] = 0xc000 | (SrcReg << 8) | (Rot << 5) | (Adr & 0x1f);
            CodeLen = 1;
          }
        }
      }
    }
  }
}

static void DecodeAri(Word Code)
{
  Word SrcReg, DestReg, Rot;
  ShortInt SrcLen, DestLen;
  char *p;
  Boolean OK;

  if (ChkArgCnt(2, 3)
   && DecodeReg(&ArgStr[ArgCnt], &DestReg, &DestLen))
  {
    if (DestReg < 16)         /* Ziel Register */
    {
      if (ArgCnt == 2)        /* wenn nur zwei Operanden und Ziel Register... */
      {
        p = HasDisp(ArgStr[1].str.p_str); /* kann eine Rotation dabei sein */
        if (p)
        {                 /* jau! */
          tStrComp RegArg, RotArg;

          StrCompSplitRef(&RegArg, &RotArg, &ArgStr[1], p);
          StrCompShorten(&RotArg, 1);
          Rot = EvalStrIntExpression(&RotArg, UInt3, &OK);
          if (OK)
          {
            if (DecodeReg(&RegArg, &SrcReg, &SrcLen))
            {
              if (SrcReg >= 16) WrStrErrorPos(ErrNum_InvReg, &RegArg);
              else
              {
                WAsmCode[0] = (Code << 13) | (SrcReg << 8) | (Rot << 5) | DestReg;
                CodeLen = 1;
              }
            }
          }
        }
        else                   /* noi! */
        {
          if (DecodeReg(&ArgStr[1], &SrcReg, &SrcLen))
          {
            WAsmCode[0] = (Code << 13) | (SrcReg << 8) | DestReg;
            if ((SrcReg >= 16) && (SrcLen != -1)) WAsmCode[0] += SrcLen << 5;
            CodeLen = 1;
          }
        }
      }
      else                     /* 3 Operanden --> Quelle ist I/O */
      {
        tSymbolFlags Flags;

        if (GetLen(&ArgStr[2], &Rot, &Flags))
         if (DecodeReg(&ArgStr[1], &SrcReg, &SrcLen))
         {
           if (SrcReg < 16) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
           else if ((SrcLen != -1) && (SrcLen != Rot)) WrError(ErrNum_ConfOpSizes);
           else
           {
             WAsmCode[0] = (Code << 13) | (SrcReg << 8) | (Rot << 5) | DestReg;
             CodeLen = 1;
           }
         }
      }
    }
    else                       /* Ziel I/O */
    {
      if (ArgCnt == 2)           /* 2 Argumente: Laenge=Laenge Ziel */
      {
        Rot = DestLen; OK = True;
      }
      else                     /* 3 Argumente: Laenge=Laenge Ziel+Angabe */
      {
        tSymbolFlags Flags;

        OK = GetLen(&ArgStr[2], &Rot, &Flags);
        if (OK)
        {
          if (mFirstPassUnknown(Flags)) Rot = DestLen;
          if (DestLen == -1) DestLen = Rot;
          OK = Rot == DestLen;
          if (!OK) WrError(ErrNum_ConfOpSizes);
        }
      }
      if (OK)
       if (DecodeReg(&ArgStr[1], &SrcReg, &SrcLen))
       {
         if (Rot == 0xffff)
           Rot = ((SrcLen == -1)) ? 0 : SrcLen;
         if ((DestReg >= 16) && (SrcLen != -1) && (SrcLen != Rot)) WrError(ErrNum_ConfOpSizes);
         else
         {
           WAsmCode[0] = (Code << 13) | (SrcReg << 8) | (Rot << 5) | DestReg;
           CodeLen = 1;
         }
       }
    }
  }
}

static void DecodeXEC(Word Code)
{
  char *p;
  Word SrcReg, Rot;
  ShortInt SrcLen;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    p = HasDisp(ArgStr[1].str.p_str);
    if (!p) WrError(ErrNum_InvAddrMode);
    else
    {
      tStrComp DispArg, RegArg;

      StrCompSplitRef(&DispArg, &RegArg, &ArgStr[1], p);
      StrCompShorten(&RegArg, 1);
      if (DecodeReg(&RegArg, &SrcReg, &SrcLen))
      {
        if (SrcReg < 16)
        {
          if (ChkArgCnt(1, 1))
          {
            WAsmCode[0] = EvalStrIntExpression(&DispArg, UInt8, &OK);
            if (OK)
            {
              WAsmCode[0] |= 0x8000 | (SrcReg << 8);
              CodeLen = 1;
            }
          }
        }
        else
        {
          tSymbolFlags Flags;

          if (ArgCnt == 1)
          {
            Rot = 0xffff; OK = True; Flags = eSymbolFlag_None;
          }
          else OK = GetLen(&ArgStr[2], &Rot, &Flags);
          if (OK)
          {
            if (Rot == 0xffff)
             Rot = (SrcLen == -1) ? 0 : SrcLen;
            if ((SrcLen != -1) && (Rot != SrcLen)) WrError(ErrNum_ConfOpSizes);
            else
            {
              WAsmCode[0] = EvalStrIntExpression(&DispArg, UInt5, &OK);
              if (OK)
              {
                WAsmCode[0] |= 0x8000 | (SrcReg << 8) | (Rot << 5);
                CodeLen = 1;
              }
            }
          }
        }
      }
    }
  }
}

static void DecodeJMP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt13, &OK);
    if (OK)
    {
      WAsmCode[0] |= 0xe000;
      CodeLen = 1;
    }
  }
  return;
}

static void DecodeNZT(Word Code)
{
  Word SrcReg, Adr, Rot;
  ShortInt SrcLen;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(2, 3)
   && DecodeReg(&ArgStr[1], &SrcReg, &SrcLen))
  {
    if (SrcReg < 16)
    {
      if (ChkArgCnt(2, 2))
      {
        Adr = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt13, &OK, &Flags);
        if (OK && ChkSamePage(Adr, EProgCounter(), 8, Flags))
        {
          WAsmCode[0] = 0xa000 | (SrcReg << 8) | (Adr & 0xff);
          CodeLen = 1;
        }
      }
    }
    else
    {
      if (ArgCnt == 2)
      {
        Rot = 0xffff; OK = True; Flags = eSymbolFlag_None;
      }
      else OK = GetLen(&ArgStr[2], &Rot, &Flags);
      if (OK)
      {
        if (Rot == 0xffff)
         Rot = (SrcLen == -1) ? 0 : SrcLen;
        if ((SrcLen != -1) && (Rot != SrcLen)) WrError(ErrNum_ConfOpSizes);
        else
        {
          Adr = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], UInt13, &OK, &Flags);
          if (OK && ChkSamePage(Adr, EProgCounter(), 5, Flags))
          {
            WAsmCode[0] = 0xa000 | (SrcReg << 8) | (Rot << 5) | (Adr & 0x1f);
            CodeLen = 1;
          }
        }
      }
    }
  }
}

/* Symbol: 00AA0ORL */

static void DecodeLIV_RIV(Word Code)
{
  LongInt Adr, Ofs;
  Word Len;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(3, 3))
  {
    Adr = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
    if (OK)
    {
      Ofs = EvalStrIntExpression(&ArgStr[2], UInt3, &OK);
      if (OK)
       if (GetLen(&ArgStr[3], &Len, &Flags))
       {
         PushLocHandle(-1);
         EnterIntSymbol(&LabPart, Code | (Adr << 16) | (Ofs << 8) | (Len & 7), SegNone, False);
         PopLocHandle();
       }
    }
  }
}

/*-------------------------------------------------------------------------*/

static void AddAri(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAri);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(103);
  AddInstTable(InstTable, "NOP", 0, DecodeNOP);
  AddInstTable(InstTable, "HALT", 0, DecodeHALT);
  AddInstTable(InstTable, "XML", 0xca00, DecodeXML_XMR);
  AddInstTable(InstTable, "XMR", 0xcb00, DecodeXML_XMR);
  AddInstTable(InstTable, "SEL", 0, DecodeSEL);
  AddInstTable(InstTable, "XMIT", 0, DecodeXMIT);
  AddInstTable(InstTable, "XEC", 0, DecodeXEC);
  AddInstTable(InstTable, "JMP", 0, DecodeJMP);
  AddInstTable(InstTable, "NZT", 0, DecodeNZT);
  AddInstTable(InstTable, "LIV", 0, DecodeLIV_RIV);
  AddInstTable(InstTable, "RIV", 0x10, DecodeLIV_RIV);

  AddAri("MOVE", 0); AddAri("ADD", 1); AddAri("AND", 2); AddAri("XOR", 3);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

static void MakeCode_8x30X(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_8x30X(void)
{
  return (Memo("LIV") || Memo("RIV"));
}

static void SwitchFrom_8x30X(void)
{
  DeinitFields();
}

static void SwitchTo_8x30X(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x3a;
  NOPCode = 0x0000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 2;
  ListGrans[SegCode] = 2;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0x1fff;

  MakeCode = MakeCode_8x30X;
  IsDef = IsDef_8x30X;
  SwitchFrom = SwitchFrom_8x30X;
  InitFields();
}

void code8x30x_init(void)
{
  CPU8x300 = AddCPU("8x300", SwitchTo_8x30X);
  CPU8x305 = AddCPU("8x305", SwitchTo_8x30X);
}
