/* code7000.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator SH7x00                                                      */
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
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code7000.h"

#define FixedOrderCount 13
#define OneRegOrderCount 22
#define TwoRegOrderCount 20
#define MulRegOrderCount 3
#define BWOrderCount 3
#define SRegCnt 9

enum
{
  ModNone = -1,
  ModReg = 0,
  ModIReg = 1,
  ModPreDec = 2,
  ModPostInc = 3,
  ModIndReg = 4,
  ModR0Base = 5,
  ModGBRBase = 6,
  ModGBRR0 = 7,
  ModPCRel = 8,
  ModImm = 9
};

#define MModReg (1 << ModReg)
#define MModIReg (1 << ModIReg)
#define MModPreDec (1 << ModPreDec)
#define MModPostInc (1 << ModPostInc)
#define MModIndReg (1 << ModIndReg)
#define MModR0Base (1 << ModR0Base)
#define MModGBRBase (1 << ModGBRBase)
#define MModGBRR0 (1 << ModGBRR0)
#define MModPCRel (1 << ModPCRel)
#define MModImm (1 << ModImm)

#define REG_SP 15
#define RegNone (-1)
#define RegPC (-2)
#define RegGBR (-3)

#define CompLiteralsName "COMPRESSEDLITERALS"

typedef struct
{
  CPUVar MinCPU;
  Boolean Priv;
  Word Code;
} FixedOrder;

typedef struct
{
  CPUVar MinCPU;
  Boolean Priv;
  Word Code;
  Boolean Delayed;
} OneRegOrder;

typedef struct
{
  CPUVar MinCPU;
  Boolean Priv;
  Word Code;
  ShortInt DefSize;
} TwoRegOrder;

typedef struct
{
  CPUVar MinCPU;
  Word Code;
} FixedMinOrder;

typedef struct
{
   const char *Name;
   Word Code;
   CPUVar MinCPU;
   Boolean NeedsDSP;
} TRegDef;

typedef struct _TLiteral
{
  struct _TLiteral *Next;
  LongInt Value, FCount;
  Boolean Is32, IsForward;
  Integer PassNo;
  LongInt DefSection;
} *PLiteral, TLiteral;

static tSymbolSize OpSize;  /* Groesse=8*(2^OpSize) */
static ShortInt AdrMode;    /* Ergebnisadressmodus */
static Word AdrPart;        /* Adressierungsmodusbits im Opcode */

static PLiteral FirstLiteral;
static LongInt ForwardCount;

static CPUVar CPU7000, CPU7600, CPU7700;

static FixedOrder *FixedOrders;
static OneRegOrder *OneRegOrders;
static TwoRegOrder *TwoRegOrders;
static FixedMinOrder *MulRegOrders;
static FixedOrder *BWOrders;
static TRegDef *RegDefs;

static Boolean CurrDelayed, PrevDelayed, CompLiterals, DSPAvail;
static LongInt DelayedAdr;

/*-------------------------------------------------------------------------*/
/* die PC-relative Adresse: direkt nach verzoegerten Spruengen = Sprungziel+2 */

static LongInt PCRelAdr(void)
{
  if (PrevDelayed) return DelayedAdr + 2;
  else return EProgCounter() + 4;
}

static void ChkDelayed(void)
{
  if (PrevDelayed) WrError(ErrNum_Pipeline);
}

/*-------------------------------------------------------------------------*/
/* Adressparsing */

static char *LiteralName(PLiteral Lit, char *Result, int ResultSize)
{
  as_snprintf(Result, ResultSize, "LITERAL_");
  if (Lit->IsForward)
    as_snprcatf(Result, ResultSize, "F_%08lllx", (LargeWord)Lit->FCount);
  else if (Lit->Is32)
    as_snprcatf(Result, ResultSize, "L_%08lllx", (LargeWord)Lit->Value);
  else
    as_snprcatf(Result, ResultSize, "W_%04x", (unsigned)Lit->Value);
  as_snprcatf(Result, ResultSize, "_%x", (unsigned)Lit->PassNo);
  return Result;
}
/*
static void PrintLiterals(void)
{
  PLiteral Lauf;
  String Name;

  WrLstLine("LiteralList");
  Lauf = FirstLiteral;
  while (Lauf)
  {
    LiteralName(Lauf, Name, sizeof(Name));
    WrLstLine(Name); Lauf = Lauf->Next;
  }
}
*/
static void SetOpSize(tSymbolSize Size)
{
  if (OpSize == eSymbolSizeUnknown) OpSize = Size;
  else if (Size != OpSize)
  {
    WrError(ErrNum_ConfOpSizes); AdrMode = ModNone;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Word *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Word *pResult)
{
  size_t l;
  Boolean OK;

  if (!as_strcasecmp(pArg, "SP"))
  {
    *pResult = REG_SP | REGSYM_FLAG_ALIAS;
    return True;
  }

  l = strlen(pArg);
  if ((l < 2) || (l > 3) || (as_toupper(*pArg) != 'R'))
    return False;

  *pResult = ConstLongInt(pArg + 1, &OK, 10);
  return OK && (*pResult <= 15);
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_7000(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - SH7x00 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_7000(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
      if (Value == (REG_SP | REGSYM_FLAG_ALIAS))
        as_snprintf(pDest, DestSize, "SP");
      else
        as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg source argument
 * \param  pResult register # if yes
 * \return eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
{
  tRegEvalResult RegEvalResult;
  tEvalResult EvalResult;
  tRegDescr RegDescr;

  if (DecodeRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
  *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  return RegEvalResult;
}

static Boolean DecodeCtrlReg(char *Asc, Word *Erg)
{
  CPUVar MinCPU = CPU7000;

  *Erg = 0xff;
  if (!as_strcasecmp(Asc, "SR")) *Erg = 0;
  else if (!as_strcasecmp(Asc, "GBR")) *Erg = 1;
  else if (!as_strcasecmp(Asc, "VBR")) *Erg = 2;
  else if (!as_strcasecmp(Asc, "SSR"))
  {
    *Erg = 3; MinCPU = CPU7700;
  }
  else if (!as_strcasecmp(Asc, "SPC"))
  {
    *Erg = 4; MinCPU = CPU7700;
  }
  else if ((strlen(Asc) == 7) && (as_toupper(*Asc) == 'R')
      && (!as_strcasecmp(Asc + 2, "_BANK"))
      && (Asc[1] >= '0') && (Asc[1] <= '7'))
  {
    *Erg = Asc[1] - '0' + 8; MinCPU = CPU7700;
  }
  if ((*Erg == 0xff) || (MomCPU < MinCPU))
  {
    WrXError(ErrNum_InvCtrlReg, Asc); return False;
  }
  else return True;
}

static Boolean DecodeSReg(char *Asc, Word *Erg)
{
  int z;
  Boolean Result = FALSE;

  for (z = 0; z < SRegCnt; z++)
    if (!as_strcasecmp(Asc, RegDefs[z].Name))
      break;
  if (z < SRegCnt)
  {
    if (MomCPU < RegDefs[z].MinCPU);
    else if ((!DSPAvail) && RegDefs[z].NeedsDSP);
    else
    {
      Result = TRUE;
      *Erg = RegDefs[z].Code;
    }
  }
  return Result;
}

static LongInt ExtOp(LongInt Inp, Byte Src, Boolean Signed)
{
  switch (Src)
  {
    case 0:
      Inp &= 0xff;
      break;
    case 1:
      Inp &= 0xffff;
      break;
  }
  if (Signed)
  {
    if (Src < 1)
      if ((Inp & 0x80) == 0x80)
        Inp += 0xff00;
    if (Src < 2)
      if ((Inp & 0x8000) == 0x8000)
        Inp += 0xffff0000;
  }
  return Inp;
}

static LongInt OpMask(ShortInt OpSize)
{
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      return 0xff;
    case eSymbolSize16Bit:
      return 0xffff;
    case eSymbolSize32Bit:
      return 0xffffffff;
    default:
      return 0;
  }
}

static void DecodeAdr(const tStrComp *pArg, Word Mask, Boolean Signed)
{
  Byte p;
  Word HReg;
  char *pos;
  ShortInt BaseReg, IndReg;
  tSymbolSize DOpSize;
  LongInt DispAcc;
  String AdrStr;
  Boolean OK, FirstFlag, NIs32, Critical, Found, LDef;
  tSymbolFlags Flags;
  PLiteral Lauf, Last;
  String Name;

  AdrMode = ModNone;

  switch (DecodeReg(pArg, &HReg, False))
  {
    case eIsReg:
      AdrPart = HReg;
      AdrMode = ModReg;
      goto chk;
    case eIsNoReg:
      break;
    case eRegAbort:
      return;
  }

  if (*pArg->str.p_str == '@')
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    if (IsIndirect(Arg.str.p_str))
    {
      tStrComp Remainder;

      StrCompIncRefLeft(&Arg, 1);
      StrCompShorten(&Arg, 1);
      BaseReg = RegNone;
      IndReg = RegNone;
      DispAcc = 0;
      FirstFlag = False;
      OK = True;
      do
      {
        pos = QuotPos(Arg.str.p_str, ',');
        if (pos)
          StrCompSplitRef(&Arg, &Remainder, &Arg, pos);
        if (!as_strcasecmp(Arg.str.p_str, "PC"))
        {
          if (BaseReg == RegNone)
            BaseReg = RegPC;
          else
          {
            WrError(ErrNum_InvAddrMode);
            OK = False;
          }
        }
        else if (!as_strcasecmp(Arg.str.p_str, "GBR"))
        {
          if (BaseReg == RegNone)
            BaseReg = RegGBR;
          else
          {
            WrError(ErrNum_InvAddrMode);
            OK = False;
          }
        }
        else switch (DecodeReg(&Arg, &HReg, False))
        {
          case eIsReg:
            if (IndReg == RegNone)
              IndReg = HReg;
            else if ((BaseReg == RegNone) && (HReg == 0))
              BaseReg = 0;
            else if ((IndReg == 0) && (BaseReg == RegNone))
            {
              BaseReg = 0;
              IndReg = HReg;
            }
            else
            {
              WrStrErrorPos(ErrNum_InvAddrMode, &Arg); OK = False;
            }
            break;
          case eIsNoReg:
            DispAcc += EvalStrIntExpressionWithFlags(&Arg, Int32, &OK, &Flags);
            if (mFirstPassUnknown(Flags))
              FirstFlag = True;
            break;
          case eRegAbort:
            OK = False;
        }
        if (pos)
          Arg = Remainder;
      }
      while (pos && OK);
      if (FirstFlag) DispAcc = 0;
      if ((OK) && ((DispAcc & ((1 << OpSize) - 1)) != 0))
      {
        WrError(ErrNum_NotAligned);
        OK = False;
      }
      else if ((OK) && (DispAcc < 0))
      {
        WrXError(ErrNum_UnderRange, "Disp<0");
        OK = False;
      }
      else DispAcc = DispAcc >> OpSize;
      if (OK)
      {
        switch (BaseReg)
        {
          case 0:
            if ((IndReg < 0) || (DispAcc != 0)) WrError(ErrNum_InvAddrMode);
            else
            {
              AdrMode = ModR0Base;
              AdrPart = IndReg;
            }
            break;
          case RegGBR:
            if ((IndReg == 0) && (DispAcc == 0)) AdrMode = ModGBRR0;
            else if (IndReg != RegNone) WrError(ErrNum_InvAddrMode);
            else if (DispAcc > 255) WrError(ErrNum_OverRange);
            else
            {
              AdrMode = ModGBRBase;
              AdrPart = DispAcc;
            }
            break;
          case RegNone:
            if (IndReg == RegNone) WrError(ErrNum_InvAddrMode);
            else if (DispAcc > 15) WrError(ErrNum_OverRange);
            else
            {
              AdrMode = ModIndReg;
              AdrPart = (IndReg << 4) + DispAcc;
            }
            break;
          case RegPC:
            if (IndReg != RegNone) WrError(ErrNum_InvAddrMode);
            else if (DispAcc > 255) WrError(ErrNum_OverRange);
            else
            {
              AdrMode = ModPCRel;
              AdrPart = DispAcc;
            }
            break;
        }
      }
      goto chk;
    }
    else /* !IsIndirect */
    {
      int ArgLen = strlen(Arg.str.p_str);

      if ((ArgLen > 1) && (*Arg.str.p_str == '-'))
      {
        StrCompIncRefLeft(&Arg, 1);
        if (DecodeReg(&Arg, &HReg, True) == eIsReg)
        {
          AdrPart = HReg;
          AdrMode = ModPreDec;
        }
      }
      else if ((ArgLen > 1) && (Arg.str.p_str[ArgLen - 1] == '+'))
      {
        StrCompShorten(&Arg, 1);
        if (DecodeReg(&Arg, &HReg, True) == eIsReg)
        {
          AdrPart = HReg;
          AdrMode = ModPostInc;
        }
      }
      else if (DecodeReg(&Arg, &HReg, True))
      {
        AdrPart = HReg;
        AdrMode = ModIReg;
      }
      goto chk;
    }
  }

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case eSymbolSize8Bit:
        DispAcc = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int8, &OK, &Flags);
        break;
      case eSymbolSize16Bit:
        DispAcc = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int16, &OK, &Flags);
        break;
      case eSymbolSize32Bit:
        DispAcc = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int32, &OK, &Flags);
        break;
      default:
        DispAcc = 0;
        OK = True;
        Flags = eSymbolFlag_None;
    }
    Critical = mFirstPassUnknown(Flags) || mUsesForwards(Flags);
    if (OK)
    {
      /* minimale Groesse optimieren */

      DOpSize = (OpSize == eSymbolSize8Bit) ? eSymbolSize8Bit : (Critical ? eSymbolSize16Bit : eSymbolSize8Bit);
      while (((ExtOp(DispAcc, DOpSize, Signed) ^ DispAcc) & OpMask(OpSize)) != 0)
        DOpSize++;
      if (DOpSize == 0)
      {
        AdrPart = DispAcc & 0xff;
        AdrMode = ModImm;
      }
      else if ((Mask & MModPCRel) != 0)
      {
        tStrComp LComp;
        String LStr;

        StrCompMkTemp(&LComp, LStr, sizeof(LStr));

        /* Literalgroesse ermitteln */

        NIs32 = (DOpSize == 2);
        if (!NIs32)
          DispAcc &= 0xffff;

        /* Literale sektionsspezifisch */

        strcpy(AdrStr, "[PARENT0]");

        /* schon vorhanden ? */

        Lauf = FirstLiteral;
        p = 0;
        OK = False;
        Last = NULL;
        Found = False;
        while ((Lauf) && (!Found))
        {
          Last = Lauf;
          if ((!Critical)
           && (!Lauf->IsForward)
           && (Lauf->DefSection == MomSectionHandle))
          {
            if (((Lauf->Is32 == NIs32) && (DispAcc == Lauf->Value))
             || ((Lauf->Is32) && (!NIs32) && (DispAcc == (Lauf->Value >> 16))))
              Found = True;
            else if ((Lauf->Is32) && (!NIs32) && (DispAcc == (Lauf->Value & 0xffff)))
            {
              Found = True;
              p = 2;
            }
          }
          if (!Found)
            Lauf = Lauf->Next;
        }

        /* nein - erzeugen */

        if (!Found)
        {
          Lauf = (PLiteral) malloc(sizeof(TLiteral));
          Lauf->Is32 = NIs32;
          Lauf->Value = DispAcc;
          Lauf->IsForward = Critical;
          if (Critical)
            Lauf->FCount = ForwardCount++;
          Lauf->Next = NULL;
          Lauf->PassNo = 1;
          Lauf->DefSection = MomSectionHandle;
          do
          {
            tStrComp LStrComp;

            as_snprintf(LComp.str.p_str, STRINGSIZE, "%s%s",
                        LiteralName(Lauf, Name, sizeof(Name)),
                        AdrStr);
            StrCompMkTemp(&LStrComp, LStr, sizeof(LStr));
            LDef = IsSymbolDefined(&LStrComp);
            if (LDef)
              Lauf->PassNo++;
          }
          while (LDef);
          if (!Last)
            FirstLiteral = Lauf;
          else
            Last->Next = Lauf;
        }

        /* Distanz abfragen - im naechsten Pass... */

        as_snprintf(LComp.str.p_str, STRINGSIZE, "%s%s",
                    LiteralName(Lauf, Name, sizeof(Name)),
                    AdrStr);
        DispAcc = EvalStrIntExpressionWithFlags(&LComp, Int32, &OK, &Flags) + p;
        if (OK)
        {
          if (mFirstPassUnknown(Flags))
            DispAcc = 0;
          else if (NIs32)
            DispAcc = (DispAcc - (PCRelAdr() & 0xfffffffc)) >> 2;
          else
            DispAcc = (DispAcc - PCRelAdr()) >> 1;
          if (DispAcc < 0)
          {
            WrXError(ErrNum_UnderRange, "Disp<0");
            OK = False;
          }
          else if ((DispAcc > 255) && !mSymbolQuestionable(Flags)) WrError(ErrNum_DistTooBig);
          else
          {
            AdrMode = ModPCRel;
            AdrPart = DispAcc;
            OpSize = NIs32 ? eSymbolSize32Bit : eSymbolSize16Bit;
          }
        }
      }
      else
        WrError(ErrNum_InvAddrMode);
    }
    goto chk;
  }

  /* absolut ueber PC-relativ abwickeln */

  if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    DispAcc = EvalStrIntExpressionWithFlags(pArg, Int32, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      DispAcc = 0;
    else if (OpSize == eSymbolSize32Bit)
      DispAcc -= (PCRelAdr() & 0xfffffffc);
    else
      DispAcc -= PCRelAdr();
    if (DispAcc < 0)
      WrXError(ErrNum_UnderRange, "Disp<0");
    else if ((DispAcc & ((1 << OpSize) - 1)) != 0)
      WrError(ErrNum_NotAligned);
    else
    {
      DispAcc = DispAcc >> OpSize;
      if (DispAcc > 255) WrError(ErrNum_OverRange);
      else
      {
        AdrMode = ModPCRel;
        AdrPart = DispAcc;
      }
    }
  }

chk:
  if ((AdrMode != ModNone) && ((Mask & (1 << AdrMode)) == 0))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
  }
}

static void SetCode(Word Code)
{
  CodeLen = 2;
  WAsmCode[0] = Code;
}

/*-------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Index)
{
  const FixedOrder *pOrder = FixedOrders + Index;

  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkMinCPU(pOrder->MinCPU))
  {
    SetCode(pOrder->Code);
    if ((!SupAllowed) && (pOrder->Priv)) WrError(ErrNum_PrivOrder);
  }
}

static void DecodeMOV(Word Code)
{
  Word HReg;

  UNUSED(Code);

  if (OpSize == eSymbolSizeUnknown)
    SetOpSize(eSymbolSize32Bit);
  if (!ChkArgCnt(2, 2));
  else if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (DecodeReg(&ArgStr[1], &HReg, False) == eIsReg)
  {
    DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModPreDec | MModIndReg | MModR0Base | MModGBRBase, True);
    switch (AdrMode)
    {
      case ModReg:
        if (OpSize != eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
        else
          SetCode(0x6003 + (HReg << 4) + (AdrPart << 8));
        break;
      case ModIReg:
        SetCode(0x2000 + (HReg  << 4) + (AdrPart << 8) + OpSize);
        break;
      case ModPreDec:
        SetCode(0x2004 + (HReg << 4) + (AdrPart << 8) + OpSize);
        break;
      case ModIndReg:
        if (OpSize == eSymbolSize32Bit)
          SetCode(0x1000 + (HReg << 4) + (AdrPart & 15) + ((AdrPart & 0xf0) << 4));
        else if (HReg != 0)
          WrError(ErrNum_InvAddrMode);
        else
          SetCode(0x8000 + AdrPart + (((Word)OpSize) << 8));
        break;
      case ModR0Base:
        SetCode(0x0004 + (AdrPart << 8) + (HReg << 4) + OpSize);
        break;
      case ModGBRBase:
        if (HReg != 0)
          WrError(ErrNum_InvAddrMode);
        else
          SetCode(0xc000 + AdrPart + (((Word)OpSize) << 8));
        break;
    }
  }
  else if (DecodeReg(&ArgStr[2], &HReg, False) == eIsReg)
  {
    DecodeAdr(&ArgStr[1], MModImm | MModPCRel | MModIReg | MModPostInc | MModIndReg | MModR0Base | MModGBRBase, True);
    switch (AdrMode)
    {
      case ModIReg:
        SetCode(0x6000 + (AdrPart << 4) + (((Word)HReg) << 8) + OpSize);
        break;
      case ModPostInc:
        SetCode(0x6004 + (AdrPart << 4) + (((Word)HReg) << 8) + OpSize);
        break;
      case ModIndReg:
        if (OpSize == eSymbolSize32Bit)
          SetCode(0x5000 + (((Word)HReg) << 8) + AdrPart);
        else if (HReg != 0)
          WrError(ErrNum_InvAddrMode);
        else
          SetCode(0x8400 + AdrPart + (((Word)OpSize) << 8));
        break;
      case ModR0Base:
        SetCode(0x000c + (AdrPart << 4) + (((Word)HReg) << 8) + OpSize);
        break;
      case ModGBRBase:
        if (HReg != 0)
          WrError(ErrNum_InvAddrMode);
        else
          SetCode(0xc400 + AdrPart + (((Word)OpSize) << 8));
        break;
      case ModPCRel:
        if (OpSize == eSymbolSize8Bit)
          WrError(ErrNum_InvAddrMode);
        else
          SetCode(0x9000 + (((Word)OpSize - 1) << 14) + (((Word)HReg) << 8) + AdrPart);
        break;
      case ModImm:
        SetCode(0xe000 + (((Word)HReg) << 8) + AdrPart);
        break;
    }
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeMOVA(Word Code)
{
  Word HReg;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(&ArgStr[2], &HReg, True));
  else if (HReg != 0) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else
  {
    SetOpSize(eSymbolSize32Bit);
    DecodeAdr(&ArgStr[1], MModPCRel, False);
    if (AdrMode != ModNone)
      SetCode(0xc700 + AdrPart);
  }
}

static void DecodePREF(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    DecodeAdr(&ArgStr[1], MModIReg, False);
    if (AdrMode != ModNone)
      SetCode(WAsmCode[0] = 0x0083 + (AdrPart << 8));
  }
}

static void DecodeLDC_STC(Word IsLDC)
{
  if (OpSize == eSymbolSizeUnknown)
    SetOpSize(eSymbolSize32Bit);

  if (ChkArgCnt(2, 2))
  {
    tStrComp *pArg1 = IsLDC ? &ArgStr[2] : &ArgStr[1],
             *pArg2 = IsLDC ? &ArgStr[1] : &ArgStr[2];
    Word HReg;

    if (DecodeCtrlReg(pArg1->str.p_str, &HReg))
    {
      DecodeAdr(pArg2, MModReg | (IsLDC ? MModPostInc : MModPreDec), False);
      switch (AdrMode)
      {
        case ModReg:
          SetCode((IsLDC ? 0x400e : 0x0002) + (AdrPart << 8) + (HReg << 4));
          break;
        case ModPostInc:
          SetCode(0x4007 + (AdrPart << 8) + (HReg << 4));
          break;
        case ModPreDec:
          SetCode(0x4003 + (AdrPart << 8) + (HReg << 4));
          break;
      }
      if ((AdrMode != ModNone) && (!SupAllowed))
        WrError(ErrNum_PrivOrder);
    }
  }
}

static void DecodeLDS_STS(Word IsLDS)
{
  if (OpSize == eSymbolSizeUnknown)
    SetOpSize(eSymbolSize32Bit);

  if (ChkArgCnt(2, 2))
  {
    tStrComp *pArg1 = IsLDS ? &ArgStr[2] : &ArgStr[1],
             *pArg2 = IsLDS ? &ArgStr[1] : &ArgStr[2];
    Word HReg;

    if (!DecodeSReg(pArg1->str.p_str, &HReg)) WrError(ErrNum_InvCtrlReg);
    else
    {
      DecodeAdr(pArg2, MModReg | (IsLDS ? MModPostInc : MModPreDec), False);
      switch (AdrMode)
      {
        case ModReg:
          SetCode((IsLDS << 14) + 0x000a + (AdrPart << 8) + (HReg << 4));
          break;
        case ModPostInc:
          SetCode(0x4006 + (AdrPart << 8) + (HReg << 4));
          break;
        case ModPreDec:
          SetCode(0x4002 + (AdrPart << 8) + (HReg << 4));
          break;
      }
    }
  }
}

static void DecodeOneReg(Word Index)
{
  const OneRegOrder *pOrder = OneRegOrders + Index;

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkMinCPU(pOrder->MinCPU))
  {
    DecodeAdr(&ArgStr[1], MModReg, False);
    if (AdrMode != ModNone)
      SetCode(pOrder->Code + (AdrPart << 8));
    if ((!SupAllowed) && (pOrder->Priv)) WrError(ErrNum_PrivOrder);
    if (pOrder->Delayed)
    {
      CurrDelayed = True;
      DelayedAdr = 0x7fffffff;
      ChkDelayed();
    }
  }
}

static void DecodeTAS(Word Code)
{
  UNUSED(Code);

  if (OpSize == eSymbolSizeUnknown)
    SetOpSize(eSymbolSize8Bit);
  if (!ChkArgCnt(1, 1));
  else if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
  else
  {
    DecodeAdr(&ArgStr[1], MModIReg, False);
    if (AdrMode != ModNone)
      SetCode(0x401b + (AdrPart << 8));
  }
}

static void DecodeTwoReg(Word Index)
{
  const TwoRegOrder *pOrder = TwoRegOrders + Index;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str && (OpSize != pOrder->DefSize)) WrError(ErrNum_UseLessAttr);
  else if (ChkMinCPU(pOrder->MinCPU))
  {
    DecodeAdr(&ArgStr[1], MModReg, False);
    if (AdrMode != ModNone)
    {
      WAsmCode[0] = pOrder->Code + (AdrPart << 4);
      DecodeAdr(&ArgStr[2], MModReg, False);
      if (AdrMode != ModNone)
        SetCode(WAsmCode[0] + (((Word)AdrPart) << 8));
      if ((!SupAllowed) && (pOrder->Priv))
        WrError(ErrNum_PrivOrder);
    }
  }
}

static void DecodeMulReg(Word Index)
{
  const FixedMinOrder *pOrder = MulRegOrders + Index;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(pOrder->MinCPU))
  {
    if (!*AttrPart.str.p_str)
      OpSize = eSymbolSize32Bit;
    if (OpSize != eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
    else
    {
      DecodeAdr(&ArgStr[1], MModReg, False);
      if (AdrMode != ModNone)
      {
        WAsmCode[0] = pOrder->Code + (AdrPart << 4);
        DecodeAdr(&ArgStr[2], MModReg, False);
        if (AdrMode != ModNone)
          SetCode(WAsmCode[0] + (((Word)AdrPart) << 8));
      }
    }
  }
}

static void DecodeBW(Word Index)
{
  const FixedOrder *pOrder = BWOrders + Index;

  if (OpSize == eSymbolSizeUnknown)
    SetOpSize(eSymbolSize16Bit);
  if (!ChkArgCnt(2, 2));
  else if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    DecodeAdr(&ArgStr[1], MModReg, False);
    if (AdrMode != ModNone)
    {
      WAsmCode[0] = pOrder->Code + OpSize + (AdrPart << 4);
      DecodeAdr(&ArgStr[2], MModReg, False);
      if (AdrMode != ModNone)
        SetCode(WAsmCode[0] + (((Word)AdrPart) << 8));
    }
  }
}

static void DecodeMAC(Word Code)
{
  UNUSED(Code);

  if (OpSize == eSymbolSizeUnknown)
    SetOpSize(eSymbolSize16Bit);
  if (!ChkArgCnt(2, 2));
  else if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if ((OpSize == eSymbolSize32Bit) && !ChkMinCPU(CPU7600));
  else
  {
    DecodeAdr(&ArgStr[1], MModPostInc, False);
    if (AdrMode != ModNone)
    {
      WAsmCode[0] = 0x000f + (AdrPart << 4) + (((Word)2 - OpSize) << 14);
      DecodeAdr(&ArgStr[2], MModPostInc, False);
      if (AdrMode != ModNone)
        SetCode(WAsmCode[0] + (((Word)AdrPart) << 8));
    }
  }
}

static void DecodeADD(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    DecodeAdr(&ArgStr[2], MModReg, False);
    if (AdrMode != ModNone)
    {
      Word HReg = AdrPart;

      OpSize = eSymbolSize32Bit;
      DecodeAdr(&ArgStr[1], MModReg | MModImm, True);
      switch (AdrMode)
      {
        case ModReg:
          SetCode(0x300c + (((Word)HReg) << 8) + (AdrPart << 4));
          break;
        case ModImm:
          SetCode(0x7000 + AdrPart + (((Word)HReg) << 8));
          break;
      }
    }
  }
}

static void DecodeCMPEQ(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    DecodeAdr(&ArgStr[2], MModReg, False);
    if (AdrMode != ModNone)
    {
      Word HReg = AdrPart;

      OpSize = eSymbolSize32Bit;
      DecodeAdr(&ArgStr[1], MModReg | MModImm, True);
      switch (AdrMode)
      {
        case ModReg:
          SetCode(0x3000 + (((Word)HReg) << 8) + (AdrPart << 4));
          break;
        case ModImm:
          if (HReg != 0) WrError(ErrNum_InvAddrMode);
          else
            SetCode(0x8800 + AdrPart);
          break;
      }
    }
  }
}

static void DecodeLog(Word Code)
{
  Word HReg;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg | MModGBRR0, False);
    switch (AdrMode)
    {
      case ModReg:
        if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
        else
        {
          OpSize = eSymbolSize32Bit;
          HReg = AdrPart;
          DecodeAdr(&ArgStr[1], MModReg | MModImm, False);
          switch (AdrMode)
          {
            case ModReg:
              SetCode(0x2008 + Code + (((Word)HReg) << 8) + (AdrPart << 4));
              break;
            case ModImm:
              if (HReg != 0) WrError(ErrNum_InvAddrMode);
              else
                SetCode(0xc800 + (Code << 8) + AdrPart);
              break;
          }
        }
        break;
      case ModGBRR0:
        DecodeAdr(&ArgStr[1], MModImm, False);
        if (AdrMode != ModNone)
          SetCode(0xcc00 + (Code << 8) + AdrPart);
        break;
    }
  }
}

static void DecodeTRAPA(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    OpSize = eSymbolSize8Bit;
    DecodeAdr(&ArgStr[1], MModImm, False);
    if (AdrMode == ModImm)
      SetCode(0xc300 + AdrPart);
    ChkDelayed();
  }
}

static void DecodeBT_BF(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if ((Code & 0x400) && !ChkMinCPU(CPU7600));
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong;

    DelayedAdr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);
    AdrLong = DelayedAdr - (EProgCounter() + 4);
    if (OK)
    {
      if (Odd(AdrLong)) WrError(ErrNum_DistIsOdd);
      else if (((AdrLong < -256) || (AdrLong > 254)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        SetCode(Code + ((AdrLong >> 1) & 0xff));
        if (Code & 0x400)
          CurrDelayed = True;
        ChkDelayed();
      }
    }
  }
}

static void DecodeBRA_BSR(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong;

    DelayedAdr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);
    AdrLong = DelayedAdr - (EProgCounter() + 4);
    if (OK)
    {
      if (Odd(AdrLong)) WrError(ErrNum_DistIsOdd);
      else if (((AdrLong < -4096) || (AdrLong > 4094)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        SetCode(Code + ((AdrLong >> 1) & 0xfff));
        CurrDelayed = True;
        ChkDelayed();
      }
    }
  }
}

static void DecodeJSR_JMP(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    DecodeAdr(&ArgStr[1], MModIReg, False);
    if (AdrMode != ModNone)
    {
      SetCode(Code + (AdrPart << 8));
      CurrDelayed = True;
      DelayedAdr = 0x7fffffff;
      ChkDelayed();
    }
  }
}

static void DecodeDCT_DCF(Word Cond)
{
  char *pos;
  int z;

  if (!DSPAvail)
  {
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
    return;
  }

  /* strip off DSP condition */

  if (!ChkArgCnt(1, ArgCntMax))
    return;

  pos = FirstBlank(ArgStr[1].str.p_str);
  if (!pos)
  {
    StrCompCopy(&OpPart, &ArgStr[1]);
    for (z = 1; z < ArgCnt; z++)
      StrCompCopy(&ArgStr[z], &ArgStr[z + 1]);
    ArgCnt--;
  }
  else
    StrCompSplitLeft(&ArgStr[1], &OpPart, pos);

  UNUSED(Cond);
}

static void LTORG_16(void)
{
  PLiteral Lauf;
  String Name;
  tStrComp TmpComp;

  Lauf = FirstLiteral;
  while (Lauf)
  {
    if ((!Lauf->Is32) && (Lauf->DefSection == MomSectionHandle))
    {
      WAsmCode[CodeLen >> 1] = Lauf->Value;
      LiteralName(Lauf, Name, sizeof(Name));
      StrCompMkTemp(&TmpComp, Name, sizeof(Name));
      EnterIntSymbol(&TmpComp, EProgCounter() + CodeLen, SegCode, False);
      Lauf->PassNo = (-1);
      CodeLen += 2;
    }
    Lauf = Lauf->Next;
  }
}

static void LTORG_32(void)
{
  PLiteral Lauf, EqLauf;
  String Name;
  tStrComp TmpComp;

  Lauf = FirstLiteral;
  while (Lauf)
  {
    if ((Lauf->Is32) && (Lauf->DefSection == MomSectionHandle) && (Lauf->PassNo >= 0))
    {
      if (((EProgCounter() + CodeLen) & 2) != 0)
      {
        WAsmCode[CodeLen >> 1] = 0; CodeLen += 2;
      }
      WAsmCode[CodeLen >> 1] = (Lauf->Value >> 16);
      WAsmCode[(CodeLen >> 1) + 1] = (Lauf->Value & 0xffff);
      LiteralName(Lauf, Name, sizeof(Name));
      StrCompMkTemp(&TmpComp, Name, sizeof(Name));
      EnterIntSymbol(&TmpComp, EProgCounter() + CodeLen, SegCode, False);
      Lauf->PassNo = -1;
      if (CompLiterals)
      {
        EqLauf = Lauf->Next;
        while (EqLauf)
        {
          if ((EqLauf->Is32) && (EqLauf->PassNo >= 0)
           && (EqLauf->DefSection == MomSectionHandle)
           && (EqLauf->Value == Lauf->Value))
          {
            LiteralName(EqLauf, Name, sizeof(Name));
            StrCompMkTemp(&TmpComp, Name, sizeof(Name));
            EnterIntSymbol(&TmpComp, EProgCounter() + CodeLen, SegCode, False);
            EqLauf->PassNo = -1;
          }
          EqLauf = EqLauf->Next;
        }
      }
      CodeLen += 4;
    }
    Lauf = Lauf->Next;
  }
}

static void DecodeLTORG(Word Code)
{
  PLiteral Lauf, Tmp, Last;

  UNUSED(Code);

  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    if ((EProgCounter() & 3) == 0)
    {
      LTORG_32();
      LTORG_16();
    }
    else
    {
      LTORG_16();
      LTORG_32();
    }
    Lauf = FirstLiteral;
    Last = NULL;
    while (Lauf)
    {
      if ((Lauf->DefSection == MomSectionHandle) && (Lauf->PassNo < 0))
      {
        Tmp = Lauf->Next;
        if (!Last)
          FirstLiteral = Tmp;
        else
          Last->Next = Tmp;
        free(Lauf);
        Lauf = Tmp;
      }
      else
      {
        Last = Lauf;
        Lauf = Lauf->Next;
      }
    }
  }
}

/*-------------------------------------------------------------------------*/
/* dynamische Belegung/Freigabe Codetabellen */

static void AddFixed(const char *NName, Word NCode, Boolean NPriv, CPUVar NMin)
{
  if (InstrZ >= FixedOrderCount) exit(255);
  FixedOrders[InstrZ].Priv = NPriv;
  FixedOrders[InstrZ].MinCPU = NMin;
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddOneReg(const char *NName, Word NCode, CPUVar NMin, Boolean NPriv, Boolean NDel)
{
  if (InstrZ >= OneRegOrderCount) exit(255);
  OneRegOrders[InstrZ].Code = NCode;
  OneRegOrders[InstrZ].MinCPU = NMin;
  OneRegOrders[InstrZ].Priv = NPriv;
  OneRegOrders[InstrZ].Delayed = NDel;
  AddInstTable(InstTable, NName, InstrZ++, DecodeOneReg);
}

static void AddTwoReg(const char *NName, Word NCode, Boolean NPriv, CPUVar NMin, ShortInt NDef)
{
  if (InstrZ >= TwoRegOrderCount) exit(255);
  TwoRegOrders[InstrZ].Priv = NPriv;
  TwoRegOrders[InstrZ].DefSize = NDef;
  TwoRegOrders[InstrZ].MinCPU = NMin;
  TwoRegOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeTwoReg);
}

static void AddMulReg(const char *NName, Word NCode, CPUVar NMin)
{
  if (InstrZ >= MulRegOrderCount) exit(255);
  MulRegOrders[InstrZ].Code = NCode;
  MulRegOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeMulReg);
}

static void AddBW(const char *NName, Word NCode)
{
  if (InstrZ >= BWOrderCount) exit(255);
  BWOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeBW);
}

static void AddSReg(const char *NName, Word NCode, CPUVar NMin, Boolean NDSP)
{
  if (InstrZ >= SRegCnt) exit(255);
  RegDefs[InstrZ].Name = NName;
  RegDefs[InstrZ].Code = NCode;
  RegDefs[InstrZ].MinCPU = NMin;
  RegDefs[InstrZ++].NeedsDSP = NDSP;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MOVA", 0, DecodeMOVA);
  AddInstTable(InstTable, "PREF", 0, DecodePREF);
  AddInstTable(InstTable, "LDC", 1, DecodeLDC_STC);
  AddInstTable(InstTable, "STC", 0, DecodeLDC_STC);
  AddInstTable(InstTable, "LDS", 1, DecodeLDS_STS);
  AddInstTable(InstTable, "STS", 0, DecodeLDS_STS);
  AddInstTable(InstTable, "TAS", 0, DecodeTAS);
  AddInstTable(InstTable, "MAC", 0, DecodeMAC);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "CMP/EQ", 0, DecodeCMPEQ);
  AddInstTable(InstTable, "TRAPA", 0, DecodeTRAPA);
  AddInstTable(InstTable, "BF", 0x8b00, DecodeBT_BF);
  AddInstTable(InstTable, "BT", 0x8900, DecodeBT_BF);
  AddInstTable(InstTable, "BF/S", 0x8f00, DecodeBT_BF);
  AddInstTable(InstTable, "BT/S", 0x8d00, DecodeBT_BF);
  AddInstTable(InstTable, "BRA", 0xa000, DecodeBRA_BSR);
  AddInstTable(InstTable, "BSR", 0xb000, DecodeBRA_BSR);
  AddInstTable(InstTable, "JSR", 0x400b, DecodeJSR_JMP);
  AddInstTable(InstTable, "JMP", 0x402b, DecodeJSR_JMP);
  AddInstTable(InstTable, "DCT", 1, DecodeDCT_DCF);
  AddInstTable(InstTable, "DCF", 2, DecodeDCT_DCF);
  AddInstTable(InstTable, "LTORG", 0, DecodeLTORG);

  FixedOrders = (FixedOrder *) malloc(sizeof(FixedOrder) * FixedOrderCount); InstrZ = 0;
  AddFixed("CLRT"  , 0x0008, False, CPU7000);
  AddFixed("CLRMAC", 0x0028, False, CPU7000);
  AddFixed("NOP"   , 0x0009, False, CPU7000);
  AddFixed("RTE"   , 0x002b, False, CPU7000);
  AddFixed("SETT"  , 0x0018, False, CPU7000);
  AddFixed("SLEEP" , 0x001b, False, CPU7000);
  AddFixed("RTS"   , 0x000b, False, CPU7000);
  AddFixed("DIV0U" , 0x0019, False, CPU7000);
  AddFixed("BRK"   , 0x0000, True , CPU7000);
  AddFixed("RTB"   , 0x0001, True , CPU7000);
  AddFixed("CLRS"  , 0x0048, False, CPU7700);
  AddFixed("SETS"  , 0x0058, False, CPU7700);
  AddFixed("LDTLB" , 0x0038, True , CPU7700);

  OneRegOrders = (OneRegOrder *) malloc(sizeof(OneRegOrder) * OneRegOrderCount); InstrZ = 0;
  AddOneReg("MOVT"  , 0x0029, CPU7000, False, False);
  AddOneReg("CMP/PZ", 0x4011, CPU7000, False, False);
  AddOneReg("CMP/PL", 0x4015, CPU7000, False, False);
  AddOneReg("ROTL"  , 0x4004, CPU7000, False, False);
  AddOneReg("ROTR"  , 0x4005, CPU7000, False, False);
  AddOneReg("ROTCL" , 0x4024, CPU7000, False, False);
  AddOneReg("ROTCR" , 0x4025, CPU7000, False, False);
  AddOneReg("SHAL"  , 0x4020, CPU7000, False, False);
  AddOneReg("SHAR"  , 0x4021, CPU7000, False, False);
  AddOneReg("SHLL"  , 0x4000, CPU7000, False, False);
  AddOneReg("SHLR"  , 0x4001, CPU7000, False, False);
  AddOneReg("SHLL2" , 0x4008, CPU7000, False, False);
  AddOneReg("SHLR2" , 0x4009, CPU7000, False, False);
  AddOneReg("SHLL8" , 0x4018, CPU7000, False, False);
  AddOneReg("SHLR8" , 0x4019, CPU7000, False, False);
  AddOneReg("SHLL16", 0x4028, CPU7000, False, False);
  AddOneReg("SHLR16", 0x4029, CPU7000, False, False);
  AddOneReg("LDBR"  , 0x0021, CPU7000, True , False);
  AddOneReg("STBR"  , 0x0020, CPU7000, True , False);
  AddOneReg("DT"    , 0x4010, CPU7600, False, False);
  AddOneReg("BRAF"  , 0x0023, CPU7600, False, True );
  AddOneReg("BSRF"  , 0x0003, CPU7600, False, True );

  TwoRegOrders = (TwoRegOrder *) malloc(sizeof(TwoRegOrder) * TwoRegOrderCount); InstrZ = 0;
  AddTwoReg("XTRCT" , 0x200d, False, CPU7000, 2);
  AddTwoReg("ADDC"  , 0x300e, False, CPU7000, 2);
  AddTwoReg("ADDV"  , 0x300f, False, CPU7000, 2);
  AddTwoReg("CMP/HS", 0x3002, False, CPU7000, 2);
  AddTwoReg("CMP/GE", 0x3003, False, CPU7000, 2);
  AddTwoReg("CMP/HI", 0x3006, False, CPU7000, 2);
  AddTwoReg("CMP/GT", 0x3007, False, CPU7000, 2);
  AddTwoReg("CMP/STR", 0x200c, False, CPU7000, 2);
  AddTwoReg("DIV1"  , 0x3004, False, CPU7000, 2);
  AddTwoReg("DIV0S" , 0x2007, False, CPU7000, -1);
  AddTwoReg("MULS"  , 0x200f, False, CPU7000, 1);
  AddTwoReg("MULU"  , 0x200e, False, CPU7000, 1);
  AddTwoReg("NEG"   , 0x600b, False, CPU7000, 2);
  AddTwoReg("NEGC"  , 0x600a, False, CPU7000, 2);
  AddTwoReg("SUB"   , 0x3008, False, CPU7000, 2);
  AddTwoReg("SUBC"  , 0x300a, False, CPU7000, 2);
  AddTwoReg("SUBV"  , 0x300b, False, CPU7000, 2);
  AddTwoReg("NOT"   , 0x6007, False, CPU7000, 2);
  AddTwoReg("SHAD"  , 0x400c, False, CPU7700, 2);
  AddTwoReg("SHLD"  , 0x400d, False, CPU7700, 2);

  MulRegOrders = (FixedMinOrder *) malloc(sizeof(FixedMinOrder) * MulRegOrderCount); InstrZ = 0;
  AddMulReg("MUL"   , 0x0007, CPU7600);
  AddMulReg("DMULU" , 0x3005, CPU7600);
  AddMulReg("DMULS" , 0x300d, CPU7600);

  BWOrders = (FixedOrder *) malloc(sizeof(FixedOrder) * BWOrderCount); InstrZ = 0;
  AddBW("SWAP", 0x6008); AddBW("EXTS", 0x600e); AddBW("EXTU", 0x600c);

  InstrZ = 0;
  AddInstTable(InstTable, "TST", InstrZ++, DecodeLog);
  AddInstTable(InstTable, "AND", InstrZ++, DecodeLog);
  AddInstTable(InstTable, "XOR", InstrZ++, DecodeLog);
  AddInstTable(InstTable, "OR" , InstrZ++, DecodeLog);

  AddInstTable(InstTable, "REG", 0, CodeREG);

  RegDefs = (TRegDef*) malloc(sizeof(TRegDef) * SRegCnt); InstrZ = 0;
  AddSReg("MACH",  0, CPU7000, FALSE);
  AddSReg("MACL",  1, CPU7000, FALSE);
  AddSReg("PR"  ,  2, CPU7000, FALSE);
  AddSReg("DSR" ,  6, CPU7000, TRUE );
  AddSReg("A0"  ,  7, CPU7000, TRUE );
  AddSReg("X0"  ,  8, CPU7000, TRUE );
  AddSReg("X1"  ,  9, CPU7000, TRUE );
  AddSReg("Y0"  , 10, CPU7000, TRUE );
  AddSReg("Y1"  , 11, CPU7000, TRUE );
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  free(FixedOrders);
  free(OneRegOrders);
  free(TwoRegOrders);
  free(MulRegOrders);
  free(BWOrders);
  free(RegDefs);
}

/*-------------------------------------------------------------------------*/

static Boolean DecodeAttrPart_7000(void)
{
  if (*AttrPart.str.p_str)
  {
    if (strlen(AttrPart.str.p_str) != 1)
    {
      WrError(ErrNum_TooLongAttr);
      return False;
    }
    if (!DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False))
      return False;
  }
  return True;
}

static void MakeCode_7000(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = eSymbolSizeUnknown;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* ab hier (und weiter in der Hauptroutine) stehen die Befehle,
     die Code erzeugen, deshalb wird der Merker fuer verzoegerte
     Spruenge hier weiter geschaltet. */

  PrevDelayed = CurrDelayed;
  CurrDelayed = False;

  /* Attribut verwursten */

  if (*AttrPart.str.p_str)
    SetOpSize(AttrPartOpSize[0]);

  if (DecodeMoto16Pseudo(OpSize, True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_7000(void)
{
  FirstLiteral = NULL;
  ForwardCount = 0;
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_7000(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols in SH7x00
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_7000(char *pArg, TempResult *pResult)
{
  Word Reg;

  if (DecodeRegCore(pArg, &Reg))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_7000;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static Boolean IsDef_7000(void)
{
  return Memo("REG");
}

static void SwitchFrom_7000(void)
{
  DeinitFields();
  if (FirstLiteral)
    WrError(ErrNum_MsgMissingLTORG);
}

static void SwitchTo_7000(void)
{
  TurnWords = True;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x6c;
  NOPCode = 0x0009;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

  DecodeAttrPart = DecodeAttrPart_7000;
  MakeCode = MakeCode_7000;
  IsDef = IsDef_7000;
  InternSymbol = InternSymbol_7000;
  DissectReg = DissectReg_7000;
  SwitchFrom = SwitchFrom_7000;
  InitFields();
  onoff_supmode_add();
  AddONOFF("COMPLITERALS", &CompLiterals, CompLiteralsName, False);
  AddMoto16PseudoONOFF(False);

  if (!onoff_test_and_set(e_onoff_reg_dsp))
    SetFlag(&DSPAvail, DSPSymName, False);
  AddONOFF(DSPCmdName, &DSPAvail, DSPSymName, False);

  CurrDelayed = False; PrevDelayed = False;
}

void code7000_init(void)
{
  CPU7000 = AddCPU("SH7000", SwitchTo_7000);
  CPU7600 = AddCPU("SH7600", SwitchTo_7000);
  CPU7700 = AddCPU("SH7700", SwitchTo_7000);

  AddInitPassProc(InitCode_7000);
  FirstLiteral = NULL;
}
