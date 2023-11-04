/*
 * AS-Portierung
 *
 * AS-Codegeneratormodul fuer die Texas Instruments TMS320C5x-Familie
 *
 */

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "codevars.h"
#include "tipseudo.h"
#include "be_le.h"
#include "errmsg.h"

#include "code3205x.h"

/* ---------------------------------------------------------------------- */

typedef struct
{
  CPUVar MinCPU;
  Word Code;
} FixedOrder;

typedef struct
{
  CPUVar MinCPU;
  Word Code;
  Boolean Cond;
} JmpOrder;

typedef struct
{
  const char *Name;
  CPUVar MinCPU;
  Word Mode;
} tAdrMode;

typedef struct
{
  const char *Name;
  CPUVar MinCPU;
  Word CodeAND;
  Word CodeOR;
  Byte IsZL;
  Byte IsC;
  Byte IsV;
  Byte IsTP;
} Condition;

typedef struct
{
  const char *name;
  CPUVar MinCPU;
  Word Code;
} tBitTable;

#define NOCONDITION 0xffff

static FixedOrder *FixedOrders;
static FixedOrder *AdrOrders;
static JmpOrder *JmpOrders;
static FixedOrder *PluOrders;
static tAdrMode *AdrModes;
static Condition *Conditions;
static tBitTable *BitTable;

static Word AdrMode;

static CPUVar CPU320203;
static CPUVar CPU32050;
static CPUVar CPU32051;
static CPUVar CPU32053;

/* ---------------------------------------------------------------------- */

static Word EvalARExpression(const tStrComp *pArg, Boolean *OK)
{
  *OK = True;

  if ((as_toupper(pArg->str.p_str[0]) == 'A')
   && (as_toupper(pArg->str.p_str[1]) == 'R')
   && (pArg->str.p_str[2] >= '0')
   && (pArg->str.p_str[2] <= '7')
   && (pArg->str.p_str[3] <= '\0'))
     return pArg->str.p_str[2] - '0';
  return EvalStrIntExpression(pArg, UInt3, OK);
}

/* ---------------------------------------------------------------------- */

static Boolean DecodeAdr(const tStrComp *pArg, int MinArgCnt, int aux, Boolean Must1)
{
  Word h;
  tAdrMode *pAdrMode = AdrModes;
  tEvalResult EvalResult;

  /* Annahme: nicht gefunden */

  Boolean AdrOK = False;

  /* Adressierungsmodus suchen */

  while (pAdrMode->Name && as_strcasecmp(pAdrMode->Name, pArg->str.p_str))
   pAdrMode++;

  /* nicht gefunden: dann absolut */

  if (!pAdrMode->Name)
  {
    /* ARn-Register darf dann nicht vorhanden sein */

    if (aux <= ArgCnt)
    {
      (void)ChkArgCnt(MinArgCnt, aux - 1);
      return False;
    }

    /* Adresse berechnen */

    h = EvalStrIntExpressionWithResult(pArg, Int16, &EvalResult);
    if (!EvalResult.OK)
      return False;
    AdrOK = True;

    /* Adresslage pruefen */

    if (Must1 && (h >= 0x80) && !mFirstPassUnknown(EvalResult.Flags))
    {
      WrError(ErrNum_UnderRange);
      return False;
    }

    /* nur untere 7 Bit gespeichert */

    AdrMode = h & 0x7f;
    ChkSpace(SegData, EvalResult.AddrSpaceMask);
  }

  /* ansonsten evtl. noch Adressregister dazu */

  else
  {
    /* auf dieser CPU nicht erlaubter Modus ? */

    if (!ChkMinCPUExt(pAdrMode->MinCPU, ErrNum_AddrModeNotSupported))
      return False;

    AdrMode = pAdrMode->Mode;
    if (aux <= ArgCnt)
    {
      h = EvalARExpression(&ArgStr[aux], &AdrOK);
      if (AdrOK) AdrMode |= 0x8 | h;
    }
    else
      AdrOK = True;
  }

  return AdrOK;
}

/* ---------------------------------------------------------------------- */

static Word DecodeCond(int argp)
{
  Condition *pCondition;
  Byte cntzl = 0, cntc = 0, cntv = 0, cnttp = 0;
  Word ret = 0x300;

  while (argp <= ArgCnt)
  {
    for (pCondition = Conditions; pCondition->Name && as_strcasecmp(pCondition->Name, ArgStr[argp].str.p_str); pCondition++);

    if (!pCondition->Name)
    {
      WrError(ErrNum_UndefCond);
      return ret;
    }
    ret &= pCondition->CodeAND;
    ret |= pCondition->CodeOR;
    cntzl += pCondition->IsZL;
    cntc += pCondition->IsC;
    cntv += pCondition->IsV;
    cnttp += pCondition->IsTP;
    argp++;
  }

  if ((cnttp > 1) || (cntzl > 1) || (cntv > 1) || (cntc > 1))
    WrStrErrorPos(ErrNum_UndefCond, &ArgStr[argp]);

  return ret;
}

/* ---------------------------------------------------------------------- */

static Word DecodeShift(const tStrComp *pArg, Boolean *OK)
{
  Word Shift;
  tSymbolFlags Flags;

  Shift = EvalStrIntExpressionWithFlags(pArg, UInt5, OK, &Flags);
  if (*OK)
  {
    if (mFirstPassUnknown(Flags)) Shift &= 15;
    *OK = ChkRange(Shift, 0, 16);
  }
  return Shift;
}

/* ---------------------------------------------------------------------- */

static void DecodeFixed(Word Index)
{
  const FixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && ChkMinCPU(pOrder->MinCPU))
  {
    CodeLen = 1;
    WAsmCode[0] = pOrder->Code;
  }
}

static void DecodeCmdAdr(Word Index)
{
  const FixedOrder *pOrder = AdrOrders + Index;

  if (ChkArgCnt(1, 2)
   && ChkMinCPU(pOrder->MinCPU)
   && DecodeAdr(&ArgStr[1], 1, 2, False))
  {
    CodeLen = 1;
    WAsmCode[0] = pOrder->Code | AdrMode;
  }
}

static void DecodeCmdJmp(Word Index)
{
  const JmpOrder *pOrder = JmpOrders + Index;

  if (ChkMinCPU(pOrder->MinCPU)
   && ChkArgCnt(1, pOrder->Cond ? ArgCntMax : 3))
  {
    Boolean OK;

    AdrMode  =  0;
    if (pOrder->Cond)
    {
      AdrMode = DecodeCond(2);
      OK = AdrMode != NOCONDITION;
    }
    else if (ArgCnt > 1)
    {
      OK = DecodeAdr(&ArgStr[2], 1, 3, False);
      if ((AdrMode < 0x80) && (OK))
      {
        WrError(ErrNum_InvAddrMode);
        OK = FALSE;
      }
      AdrMode &= 0x7f;
    }
    else
      OK = TRUE;

    if (OK)
    {
      WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
      if (OK)
      {
        CodeLen = 2;
        WAsmCode[0] = pOrder->Code | AdrMode;
      }
    }
  }
}

static void DecodeCmdPlu(Word Index)
{
  Boolean OK;
  const FixedOrder *pOrder = PluOrders + Index;

  if (!ChkMinCPU(pOrder->MinCPU));
  else if (*ArgStr[1].str.p_str == '#')
  {
    if (ChkArgCnt(2, 3))
    {
      WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);
      if ((OK) && (DecodeAdr(&ArgStr[2], 2, 3, False)))
      {
        CodeLen = 2;
        WAsmCode[0] = pOrder->Code | 0x0400 | AdrMode;
      }
    }
  }
  else if (strlen(OpPart.str.p_str) == 4) WrError(ErrNum_OnlyImmAddr);
  else
  {
    if (ChkArgCnt(1, 2))
    {
      if (DecodeAdr(&ArgStr[1], 1, 2, False))
      {
        CodeLen = 1;
        WAsmCode[0] = pOrder->Code | AdrMode;
      }
    }
  }
}

static void DecodeADDSUB(Word Index)
{
  Word Shift;
  LongInt AdrLong;
  Boolean OK;

  if (ChkArgCnt(1, 3))
  {
    if (*ArgStr[1].str.p_str == '#')
    {
      if (ChkArgCnt(1, 2))
      {
        OK = True;
        Shift = (ArgCnt == 1) ? 0 : EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
        if (OK)
        {
          AdrLong = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt16, &OK);
          if (OK)
          {
            if ((Shift == 0) && (Hi(AdrLong) == 0))
            {
              CodeLen = 1;
              WAsmCode[0] = (Index << 9) | 0xb800 | (AdrLong & 0xff);
            }
            else
            {
              CodeLen = 2;
              WAsmCode[0] = ((Index << 4) + 0xbf90) | (Shift & 0xf);
              WAsmCode[1] = AdrLong;
            }
          }
        }
      }
    }
    else
    {
      if (DecodeAdr(&ArgStr[1], 1, 3, False))
      {
        OK = True;
        Shift = (ArgCnt >= 2) ? DecodeShift(&ArgStr[2], &OK) : 0;
        if (OK)
        {
          CodeLen = 1;
          if (Shift == 16)
            WAsmCode[0] = ((Index << 10) | 0x6100) | AdrMode;
          else
            WAsmCode[0] = ((Index << 12) | 0x2000) | ((Shift & 0xf) << 8) | AdrMode;
        }
      }
    }
  }
}

static void DecodeADRSBRK(Word Index)
{
  Word adr_word;
  Boolean OK;

  if (!ChkArgCnt(1, 1))
    return;

  if (*ArgStr[1].str.p_str != '#')
  {
    WrError(ErrNum_OnlyImmAddr); /*invalid parameter*/
    return;
  }

  adr_word = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt8, &OK);
  if (OK)
  {
    CodeLen = 1;
    WAsmCode[0] = (Index << 10)| 0x7800 | (adr_word & 0xff);
  }
}

static void DecodeLogic(Word Index)
{
  Boolean OK;
  Word Shift;

  if (!ChkArgCnt(1, 2));
  else if (*ArgStr[1].str.p_str == '#')
  {
    WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt16, &OK);
    if (OK)
    {
      OK = True;
      Shift = (ArgCnt >= 2) ? DecodeShift(&ArgStr[2], &OK) : 0;
      if (OK)
      {
        CodeLen = 2;
        if (Shift >= 16)
          WAsmCode[0] = 0xbe80 | Lo(Index);
        else
          WAsmCode[0] = 0xbfa0 + ((Index & 3) << 4) + (Shift & 0xf);
      }
    }
  }
  else
  {
    if (DecodeAdr(&ArgStr[1], 1, 2, False))
    {
      CodeLen = 1;
      WAsmCode[0] = (Index & 0xff00) | AdrMode;
    }
  }
}

static void DecodeBIT(Word Index)
{
  Word bit;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 3))
  {
    bit = EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
    if ((OK) && (DecodeAdr(&ArgStr[1], 2, 3, False)))
    {
      CodeLen = 1;
      WAsmCode[0] = 0x4000 | AdrMode | ((bit & 0xf) << 8);
    }
  }
}

static void DecodeBLDD(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(2, 3));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "BMAR"))
  {
    if (ChkMinCPU(CPU32050))
    {
      if (DecodeAdr(&ArgStr[2], 2, 3, False))
      {
        CodeLen = 1;
        WAsmCode[0] = 0xac00 | AdrMode;
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "BMAR"))
  {
    if (ChkMinCPU(CPU32050))
    {
      if (DecodeAdr(&ArgStr[1], 2, 3, False))
      {
        CodeLen = 1;
        WAsmCode[0] = 0xad00 | AdrMode;
      }
    }
  }
  else if (*ArgStr[1].str.p_str == '#')
  {
    WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);
    if ((OK) && (DecodeAdr(&ArgStr[2], 2, 3, False)))
    {
      CodeLen = 2;
      WAsmCode[0] = 0xa800 | AdrMode;
    }
  }
  else if (*ArgStr[2].str.p_str == '#')
  {
    WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[2], 1, Int16, &OK);
    if ((OK) && (DecodeAdr(&ArgStr[1], 2, 3, False)))
    {
      CodeLen = 2;
      WAsmCode[0] = 0xa900 | AdrMode;
    }
  }
  else
    WrError(ErrNum_InvAddrMode); /* invalid addr mode */
}

static void DecodeBLPD(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(2, 3));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "BMAR"))
  {
    if (ChkMinCPU(CPU32050)
     && DecodeAdr(&ArgStr[2], 2, 3, False))
    {
      CodeLen = 1;
      WAsmCode[0] = 0xa400 | AdrMode;
    }
  }
  else if (*ArgStr[1].str.p_str == '#')
  {
    WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);
    if ((OK) && (DecodeAdr(&ArgStr[2], 2, 3, False)))
    {
      CodeLen = 2;
      WAsmCode[0] = 0xa500 | AdrMode;
    }
  }
  else
    WrError(ErrNum_InvAddrMode); /* invalid addressing mode */
}

static void DecodeCLRSETC(Word Index)
{
  tBitTable *pBitTable;

  if (ChkArgCnt(1, 1))
  {
    WAsmCode[0] = Index;
    NLS_UpString(ArgStr[1].str.p_str);

    for (pBitTable = BitTable; pBitTable->name; pBitTable++)
      if (!strcmp(ArgStr[1].str.p_str, pBitTable->name))
      {
        if (ChkMinCPU(pBitTable->MinCPU))
        {
          WAsmCode[0] |= pBitTable->Code;
          CodeLen = 1;
        }
        return;
      }
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]); /* invalid instruction */
  }
}

static void DecodeCMPRSPM(Word Index)
{
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    WAsmCode[0] = Index | (EvalStrIntExpression(&ArgStr[1], UInt2, &OK) & 3);
    if (OK)
      CodeLen = 1;
  }
}

static void DecodeIO(Word Index)
{
  if (ChkArgCnt(2, 3)
   && DecodeAdr(&ArgStr[1], 2, 3, False))
  {
    tEvalResult EvalResult;
    WAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[2], UInt16, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      CodeLen = 2;
      WAsmCode[0] = Index | AdrMode;
    }
  }
}

static void DecodeINTR(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt5, &OK) | 0xbe60;
    if (OK)
      CodeLen = 1;
  }
}

static void DecodeLACC(Word Index)
{
  Boolean OK;
  LongWord AdrLong;
  Word Shift;
  UNUSED(Index);

  if (!ChkArgCnt(1, 3));
  else if (*ArgStr[1].str.p_str == '#')
  {
    if (ChkArgCnt(1, 2))
    {
      AdrLong = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);
      if (OK)
      {
        OK = True;
        Shift = (ArgCnt > 1) ? EvalStrIntExpression(&ArgStr[2], UInt4, &OK) : 0;
        if (OK)
        {
          CodeLen = 2;
          WAsmCode[0] = 0xbf80 | (Shift & 0xf);
          WAsmCode[1] = AdrLong;
        }
      }
    }
  }
  else
  {
    if (DecodeAdr(&ArgStr[1], 1, 3, False))
    {
      OK = True;
      Shift = (ArgCnt >= 2) ? DecodeShift(&ArgStr[2], &OK) : 0;
      if (OK)
      {
        CodeLen = 1;
        if (Shift >= 16)
          WAsmCode[0] = 0x6a00 | AdrMode;
        else
          WAsmCode[0] = 0x1000 | ((Shift & 0xf) << 8) | AdrMode;
      }
    }
  }
}

static void DecodeLACL(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (*ArgStr[1].str.p_str == '#')
  {
    if (ChkArgCnt(1, 1))
    {
      WAsmCode[0] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt8, &OK);
      if (OK)
      {
        CodeLen = 1;
        WAsmCode[0] |= 0xb900;
      }
    }
  }
  else
  {
    if (ChkArgCnt(1, 2))
    {
      if (DecodeAdr(&ArgStr[1], 1, 2, False))
      {
        WAsmCode[0] = 0x6900 | AdrMode;
        CodeLen = 1;
      }
    }
  }
}

static void DecodeLAR(Word Index)
{
  Word Reg;
  LongWord AdrLong;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 3))
  {
    Reg = EvalARExpression(&ArgStr[1], &OK);
    if (OK)
    {
      if (*ArgStr[2].str.p_str == '#')
      {
        if (!ChkArgCnt(2, 2))
          return;
        AdrLong = EvalStrIntExpressionOffs(&ArgStr[2], 1, Int16, &OK) & 0xffff;
        if (OK)
        {
          if (AdrLong > 255)
          {
            CodeLen = 2;
            WAsmCode[0] = 0xbf08 | (Reg & 7);
            WAsmCode[1] = AdrLong;
          }
          else
          {
            CodeLen = 1;
            WAsmCode[0] = 0xb000 | ((Reg & 7) << 8) | (AdrLong & 0xff);
          }
        }
      }
      else
      {
        if (DecodeAdr(&ArgStr[2], 2, 3, False))
        {
          CodeLen = 1;
          WAsmCode[0] = 0x0000 | ((Reg & 7) << 8) | AdrMode;
        }
      }
    }
  }
}

static void DecodeLDP(Word Index)
{
  Word konst;
  Boolean OK;
  UNUSED(Index);

  if (*ArgStr[1].str.p_str == '#')
  {
    if (ChkArgCnt(1, 1))
    {
      konst = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt9, &OK);
      if (OK)
      {
        CodeLen = 1;
        WAsmCode[0] = (konst & 0x1ff) | 0xbc00;
      }
    }
  }
  else
  {
    if (ChkArgCnt(1, 2))
    {
      if (DecodeAdr(&ArgStr[1], 1, 2, False))
      {
        CodeLen = 1;
        WAsmCode[0] = 0x0d00 | AdrMode;
      }
    }
  }
}

static void DecodeLSST(Word Index)
{
  Word konst;
  Boolean OK;

  if (!ChkArgCnt(2, 3));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr); /* invalid instruction */
  else
  {
    konst = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt1, &OK);
    if ((OK) && (DecodeAdr(&ArgStr[2], 2, 3, Index)))
    {
      CodeLen = 1;
      WAsmCode[0] = 0x0e00 | (Index << 15) | ((konst & 1) << 8) | AdrMode;
    }
  }
}

static void DecodeMAC(Word Index)
{
  if (ChkArgCnt(2, 3))
  {
    tEvalResult EvalResult;
    WAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[1], Int16, &EvalResult);

    if (EvalResult.OK && DecodeAdr(&ArgStr[2], 2, 3, False))
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      CodeLen = 2;
      WAsmCode[0] = 0xa200 | (Index << 8) | AdrMode;
    }
  }
}

static void DecodeMPY(Word Index)
{
  LongInt Imm;
  Boolean OK;
  tSymbolFlags Flags;

  if (*ArgStr[1].str.p_str == '#')
  {
    if (ChkArgCnt(1, 1))
    {
      Imm = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, SInt16, &OK, &Flags);
      if (mFirstPassUnknown(Flags))
        Imm &= 0xfff;
      if (OK)
      {
        if ((Imm < -4096) || (Imm > 4095))
        {
          if (ChkMinCPU(CPU32050))
          {
            CodeLen = 2;              /* What does that mean? */
            WAsmCode[0] = 0xbe80;
            WAsmCode[1] = Imm;
          }
        }
        else
        {
          CodeLen = 1;
          WAsmCode[0] = 0xc000 | (Imm & 0x1fff);
        }
      }
    }
  }
  else
  {
    if (ChkArgCnt(1, 2)
     && DecodeAdr(&ArgStr[1], 1, 2, Index))
    {
      CodeLen = 1;
      WAsmCode[0] = 0x5400 | AdrMode;
    }
  }
}

static void DecodeNORM(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 2)
   && DecodeAdr(&ArgStr[1], 1, 2, False))
  {
    if (AdrMode < 0x80) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 1;
      WAsmCode[0] = 0xa080 | (AdrMode & 0x7f);
    }
  }
}

static void DecodeRETC(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (Index && !ChkMinCPU(CPU32050));
  else
  {
    CodeLen = 1;
    WAsmCode[0] = 0xec00 | (Index << 12) | DecodeCond(1);
  }
}

static void DecodeRPT(Word Index)
{
  Word Imm;
  Boolean OK;
  UNUSED(Index);

  if (*ArgStr[1].str.p_str == '#')
  {
    if (ChkArgCnt(1, 1))
    {
      Imm = EvalStrIntExpressionOffs(&ArgStr[1], 1, (MomCPU >= CPU32050) ? UInt16 : UInt8, &OK);
      if (OK)
      {
        if (Imm > 255)
        {
          CodeLen = 2;
          WAsmCode[0] = 0xbec4;
          WAsmCode[1] = Imm;
        }
        else
        {
          CodeLen = 1;
          WAsmCode[0] = 0xbb00 | (Imm & 0xff);
        }
      }
    }
  }
  else
  {
    if (ChkArgCnt(1, 2))
    {
      if (DecodeAdr(&ArgStr[1], 1, 2, False))
      {
        CodeLen = 1;
        WAsmCode[0] = 0x0b00 | AdrMode;
      }
    }
  }
}

static void DecodeSAC(Word Index)
{
  Boolean OK;
  Word Shift;

  if (ChkArgCnt(1, 3))
  {
    OK = True;
    Shift = (ArgCnt >= 2) ? EvalStrIntExpression(&ArgStr[2], UInt3, &OK) : 0;

    if ((DecodeAdr(&ArgStr[1], 1, 3, False)) && (OK))
    {
      CodeLen = 1;
      WAsmCode[0] = (Index << 11) | 0x9000 | AdrMode | ((Shift & 7) << 8);
    }
  }
}

static void DecodeSAR(Word Index)
{
  Word Reg;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 3))
  {
    Reg = EvalARExpression(&ArgStr[1], &OK);

    if ((OK) && (DecodeAdr(&ArgStr[2], 2, 3, False)))
    {
      CodeLen = 1;
      WAsmCode[0] = 0x8000 | ((Reg & 7) << 8) | AdrMode;
    }
  }
}

static void DecodeBSAR(Word Index)
{
  Word Shift;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU32050))
  {
    Shift = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt5, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      Shift = 1;
    if (OK)
    {
      if (ChkRange(Shift, 1, 16))
      {
        CodeLen = 1;
        WAsmCode[0] = 0xbfe0 | ((Shift - 1) & 0xf);
      }
    }
  }
}

static void DecodeLSAMM(Word Index)
{
  if (ChkArgCnt(1, 2)
   && ChkMinCPU(CPU32050)
   && DecodeAdr(&ArgStr[1], 1, 2, True))
  {
    CodeLen = 1;
    WAsmCode[0] = 0x0800 | (Index << 15) | AdrMode;
  }
}

static void DecodeLSMMR(Word Index)
{
  Boolean OK;

  if (!ChkArgCnt(2, 3));
  else if (!ChkMinCPU(CPU32050));
  else if (ArgStr[2].str.p_str[0] != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[2], 1, Int16, &OK);
    if ((OK) && (DecodeAdr(&ArgStr[1], 2, 3, True)))
    {
      CodeLen = 2;
      WAsmCode[0] = 0x0900 | (Index << 15) | AdrMode;
    }
  }
}

static void DecodeRPTB(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU32050))
  {
    WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[0] = 0xbec6;
    }
  }
}

static void DecodeRPTZ(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU32050));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    WAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[0] = 0xbec5;
    }
  }
}

static void DecodeXC(Word Index)
{
  Word Mode;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU32050))
  {
    Mode = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt2, &OK, &Flags);
    if (OK)
    {
      if ((Mode != 1) && (Mode != 2) && !mFirstPassUnknown(Flags)) WrError(ErrNum_UnderRange);
      else
      {
        CodeLen = 1;
        WAsmCode[0] = (0xd400 + (Mode << 12)) | DecodeCond(2);
      }
    }
  }
}

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, 65535);
}

/* ---------------------------------------------------------------------- */

static void AddFixed(const char *NName, CPUVar MinCPU, Word NCode)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].MinCPU = MinCPU;
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddAdr(const char *NName, CPUVar MinCPU, Word NCode)
{
  order_array_rsv_end(AdrOrders, FixedOrder);
  AdrOrders[InstrZ].MinCPU = MinCPU;
  AdrOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCmdAdr);
}

static void AddJmp(const char *NName, CPUVar MinCPU, Word NCode, Boolean NCond)
{
  order_array_rsv_end(JmpOrders, JmpOrder);
  JmpOrders[InstrZ].MinCPU = MinCPU;
  JmpOrders[InstrZ].Code = NCode;
  JmpOrders[InstrZ].Cond = NCond;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCmdJmp);
}

static void AddPlu(const char *NName, CPUVar MinCPU, Word NCode)
{
  order_array_rsv_end(PluOrders, FixedOrder);
  PluOrders[InstrZ].MinCPU = MinCPU;
  PluOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCmdPlu);
}

static void AddAdrMode(const char *NName, CPUVar MinCPU, Word NMode)
{
  order_array_rsv_end(AdrModes, tAdrMode);
  AdrModes[InstrZ].Name = NName;
  AdrModes[InstrZ].MinCPU = MinCPU;
  AdrModes[InstrZ++].Mode = NMode;
}

static void AddCond(const char *NName, CPUVar MinCPU, Word NCodeAND, Word NCodeOR, Byte NIsZL,
                    Byte NIsC, Byte NIsV, Byte NIsTP)
{
  order_array_rsv_end(Conditions, Condition);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ].MinCPU = MinCPU;
  Conditions[InstrZ].CodeAND = NCodeAND;
  Conditions[InstrZ].CodeOR = NCodeOR;
  Conditions[InstrZ].IsZL = NIsZL;
  Conditions[InstrZ].IsC = NIsC;
  Conditions[InstrZ].IsV = NIsV;
  Conditions[InstrZ++].IsTP = NIsTP;
}

static void AddBit(const char *NName, CPUVar MinCPU, Word NCode)
{
  order_array_rsv_end(BitTable, tBitTable);
  BitTable[InstrZ].name = NName;
  BitTable[InstrZ].MinCPU = MinCPU;
  BitTable[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);

  InstrZ = 0;
  AddFixed("ABS",    CPU320203, 0xbe00); AddFixed("ADCB",   CPU32050 , 0xbe11);
  AddFixed("ADDB",   CPU32050 , 0xbe10); AddFixed("ANDB",   CPU32050 , 0xbe12);
  AddFixed("CMPL",   CPU320203, 0xbe01); AddFixed("CRGT",   CPU32050 , 0xbe1b);
  AddFixed("CRLT",   CPU32050 , 0xbe1c); AddFixed("EXAR",   CPU32050 , 0xbe1d);
  AddFixed("LACB",   CPU32050 , 0xbe1f); AddFixed("NEG",    CPU320203, 0xbe02);
  AddFixed("ORB",    CPU32050 , 0xbe13); AddFixed("ROL",    CPU320203, 0xbe0c);
  AddFixed("ROLB",   CPU32050 , 0xbe14); AddFixed("ROR",    CPU320203, 0xbe0d);
  AddFixed("RORB",   CPU32050 , 0xbe15); AddFixed("SACB",   CPU32050 , 0xbe1e);
  AddFixed("SATH",   CPU32050 , 0xbe5a); AddFixed("SATL",   CPU32050 , 0xbe5b);
  AddFixed("SBB",    CPU32050 , 0xbe18); AddFixed("SBBB",   CPU32050 , 0xbe19);
  AddFixed("SFL",    CPU320203, 0xbe09); AddFixed("SFLB",   CPU32050 , 0xbe16);
  AddFixed("SFR",    CPU320203, 0xbe0a); AddFixed("SFRB",   CPU32050 , 0xbe17);
  AddFixed("XORB",   CPU32050 , 0xbe1a); AddFixed("ZAP",    CPU32050 , 0xbe59);
  AddFixed("APAC",   CPU320203, 0xbe04); AddFixed("PAC",    CPU320203, 0xbe03);
  AddFixed("SPAC",   CPU320203, 0xbe05); AddFixed("ZPR",    CPU32050 , 0xbe58);
  AddFixed("BACC",   CPU320203, 0xbe20); AddFixed("BACCD",  CPU32050 , 0xbe21);
  AddFixed("CALA",   CPU320203, 0xbe30); AddFixed("CALAD",  CPU32050 , 0xbe3d);
  AddFixed("NMI",    CPU320203, 0xbe52); AddFixed("RET",    CPU320203, 0xef00);
  AddFixed("RETD",   CPU32050 , 0xff00); AddFixed("RETE",   CPU32050 , 0xbe3a);
  AddFixed("RETI",   CPU32050 , 0xbe38); AddFixed("TRAP",   CPU320203, 0xbe51);
  AddFixed("IDLE",   CPU320203, 0xbe22); AddFixed("NOP",    CPU320203, 0x8b00);
  AddFixed("POP",    CPU320203, 0xbe32); AddFixed("PUSH",   CPU320203, 0xbe3c);
  AddFixed("IDLE2",  CPU32050 , 0xbe23);

  InstrZ = 0;
  AddAdr("ADDC",   CPU320203, 0x6000); AddAdr("ADDS",   CPU320203, 0x6200);
  AddAdr("ADDT",   CPU320203, 0x6300); AddAdr("LACT",   CPU320203, 0x6b00);
  AddAdr("SUBB",   CPU320203, 0x6400); AddAdr("SUBC",   CPU320203, 0x0a00);
  AddAdr("SUBS",   CPU320203, 0x6600); AddAdr("SUBT",   CPU320203, 0x6700);
  AddAdr("ZALR",   CPU320203, 0x6800); AddAdr("MAR",    CPU320203, 0x8b00);
  AddAdr("LPH",    CPU320203, 0x7500); AddAdr("LT",     CPU320203, 0x7300);
  AddAdr("LTA",    CPU320203, 0x7000); AddAdr("LTD",    CPU320203, 0x7200);
  AddAdr("LTP",    CPU320203, 0x7100); AddAdr("LTS",    CPU320203, 0x7400);
  AddAdr("MADD",   CPU32050 , 0xab00); AddAdr("MADS",   CPU32050 , 0xaa00);
  AddAdr("MPYA",   CPU320203, 0x5000); AddAdr("MPYS",   CPU320203, 0x5100);
  AddAdr("MPYU",   CPU320203, 0x5500); AddAdr("SPH",    CPU320203, 0x8d00);
  AddAdr("SPL",    CPU320203, 0x8c00); AddAdr("SQRA",   CPU320203, 0x5200);
  AddAdr("SQRS",   CPU320203, 0x5300); AddAdr("BLDP",   CPU32050 , 0x5700);
  AddAdr("DMOV",   CPU320203, 0x7700); AddAdr("TBLR",   CPU320203, 0xa600);
  AddAdr("TBLW",   CPU320203, 0xa700); AddAdr("BITT",   CPU320203, 0x6f00);
  AddAdr("POPD",   CPU320203, 0x8a00); AddAdr("PSHD",   CPU320203, 0x7600);

  InstrZ = 0;
  AddJmp("B",      CPU320203, 0x7980,  False);
  AddJmp("BD",     CPU32050 , 0x7d80,  False);
  AddJmp("BANZ",   CPU320203, 0x7b80,  False);
  AddJmp("BANZD",  CPU32050 , 0x7f80,  False);
  AddJmp("BCND",   CPU320203, 0xe000,  True);
  AddJmp("BCNDD",  CPU32050 , 0xf000,  True);
  AddJmp("CALL",   CPU320203, 0x7a80,  False);
  AddJmp("CALLD",  CPU32050 , 0x7e80,  False);
  AddJmp("CC",     CPU320203, 0xe800,  True);
  AddJmp("CCD",    CPU32050 , 0xf800,  True);

  InstrZ = 0;
  AddPlu("APL",   CPU32050 , 0x5a00); AddPlu("CPL",   CPU32050 , 0x5b00);
  AddPlu("OPL",   CPU32050 , 0x5900); AddPlu("SPLK",  CPU320203, 0xaa00);
  AddPlu("XPL",   CPU32050 , 0x5800);

  InstrZ = 0;
  AddAdrMode( "*-",     CPU320203, 0x90 ); AddAdrMode( "*+",     CPU320203, 0xa0 );
  AddAdrMode( "*BR0-",  CPU320203, 0xc0 ); AddAdrMode( "*0-",    CPU320203, 0xd0 );
  AddAdrMode( "*AR0-",  CPU32050 , 0xd0 ); AddAdrMode( "*0+",    CPU320203, 0xe0 );
  AddAdrMode( "*AR0+",  CPU32050 , 0xe0 ); AddAdrMode( "*BR0+",  CPU320203, 0xf0 );
  AddAdrMode( "*",      CPU320203, 0x80 ); AddAdrMode( NULL,     CPU32050 , 0);

  InstrZ = 0;
  AddCond("EQ",  CPU32050 , 0xf33, 0x088, 1, 0, 0, 0);
  AddCond("NEQ", CPU32050 , 0xf33, 0x008, 1, 0, 0, 0);
  AddCond("LT",  CPU32050 , 0xf33, 0x044, 1, 0, 0, 0);
  AddCond("LEQ", CPU32050 , 0xf33, 0x0cc, 1, 0, 0, 0);
  AddCond("GT",  CPU32050 , 0xf33, 0x004, 1, 0, 0, 0);
  AddCond("GEQ", CPU32050 , 0xf33, 0x08c, 1, 0, 0, 0);
  AddCond("NC",  CPU32050 , 0xfee, 0x001, 0, 1, 0, 0);
  AddCond("C",   CPU32050 , 0xfee, 0x011, 0, 1, 0, 0);
  AddCond("NOV", CPU32050 , 0xfdd, 0x002, 0, 0, 1, 0);
  AddCond("OV",  CPU32050 , 0xfdd, 0x022, 0, 0, 1, 0);
  AddCond("BIO", CPU32050 , 0x0ff, 0x000, 0, 0, 0, 1);
  AddCond("NTC", CPU32050 , 0x0ff, 0x200, 0, 0, 0, 1);
  AddCond("TC",  CPU32050 , 0x0ff, 0x100, 0, 0, 0, 1);
  AddCond("UNC", CPU32050 , 0x0ff, 0x300, 0, 0, 0, 1);
  AddCond(NULL,  CPU32050 , 0xfff, 0x000, 0, 0, 0, 0);

  InstrZ = 0;
  AddBit("OVM",  CPU320203, 0xbe42 ); AddBit("SXM",  CPU320203, 0xbe46 );
  AddBit("HM",   CPU32050 , 0xbe48 ); AddBit("TC",   CPU320203, 0xbe4a );
  AddBit("C",    CPU320203, 0xbe4e ); AddBit("XF",   CPU320203, 0xbe4c );
  AddBit("CNF",  CPU320203, 0xbe44 ); AddBit("INTM", CPU320203, 0xbe40 );
  AddBit(NULL,   CPU32050 , 0     );

  AddInstTable(InstTable, "ADD"  , 0, DecodeADDSUB);
  AddInstTable(InstTable, "SUB"  , 1, DecodeADDSUB);
  AddInstTable(InstTable, "ADRK" , 0, DecodeADRSBRK);
  AddInstTable(InstTable, "SBRK" , 1, DecodeADRSBRK);
  AddInstTable(InstTable, "AND"  , 0x6e01, DecodeLogic);
  AddInstTable(InstTable, "OR"   , 0x6d02, DecodeLogic);
  AddInstTable(InstTable, "XOR"  , 0x6c03, DecodeLogic);
  AddInstTable(InstTable, "BIT"  , 0, DecodeBIT);
  AddInstTable(InstTable, "BLDD" , 0, DecodeBLDD);
  AddInstTable(InstTable, "BLPD" , 0, DecodeBLPD);
  AddInstTable(InstTable, "CLRC" , 0, DecodeCLRSETC);
  AddInstTable(InstTable, "SETC" , 1, DecodeCLRSETC);
  AddInstTable(InstTable, "CMPR" , 0xbf44, DecodeCMPRSPM);
  AddInstTable(InstTable, "SPM"  , 0xbf00, DecodeCMPRSPM);
  AddInstTable(InstTable, "IN"   , 0xaf00, DecodeIO);
  AddInstTable(InstTable, "OUT"  , 0x0c00, DecodeIO);
  AddInstTable(InstTable, "INTR" , 0, DecodeINTR);
  AddInstTable(InstTable, "LACC" , 0, DecodeLACC);
  AddInstTable(InstTable, "LACL" , 0, DecodeLACL);
  AddInstTable(InstTable, "LAR"  , 0, DecodeLAR);
  AddInstTable(InstTable, "LDP"  , 0, DecodeLDP);
  AddInstTable(InstTable, "SST"  , 1, DecodeLSST);
  AddInstTable(InstTable, "LST"  , 0, DecodeLSST);
  AddInstTable(InstTable, "MAC"  , 0, DecodeMAC);
  AddInstTable(InstTable, "MACD" , 1, DecodeMAC);
  AddInstTable(InstTable, "MPY"  , 0, DecodeMPY);
  AddInstTable(InstTable, "NORM" , 0, DecodeNORM);
  AddInstTable(InstTable, "RETC" , 0, DecodeRETC);
  AddInstTable(InstTable, "RETCD", 1, DecodeRETC);
  AddInstTable(InstTable, "RPT"  , 0, DecodeRPT);
  AddInstTable(InstTable, "SACL" , 0, DecodeSAC);
  AddInstTable(InstTable, "SACH" , 1, DecodeSAC);
  AddInstTable(InstTable, "SAR"  , 0, DecodeSAR);
  AddInstTable(InstTable, "BSAR" , 0, DecodeBSAR);
  AddInstTable(InstTable, "LAMM" , 0, DecodeLSAMM);
  AddInstTable(InstTable, "SAMM" , 1, DecodeLSAMM);
  AddInstTable(InstTable, "LMMR" , 1, DecodeLSMMR);
  AddInstTable(InstTable, "SMMR" , 0, DecodeLSMMR);
  AddInstTable(InstTable, "RPTB" , 0, DecodeRPTB);
  AddInstTable(InstTable, "RPTZ" , 0, DecodeRPTZ);
  AddInstTable(InstTable, "XC"   , 0, DecodeXC);
  AddInstTable(InstTable, "PORT" , 0, DecodePORT);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(AdrOrders);
  order_array_free(JmpOrders);
  order_array_free(PluOrders);
  order_array_free(AdrModes);
  order_array_free(Conditions);
  order_array_free(BitTable);
}

/* ---------------------------------------------------------------------- */

static void MakeCode_3205x(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeTIPseudo())
    return;

  /* per Hash-Tabelle */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/* ---------------------------------------------------------------------- */

static Boolean IsDef_3205x(void)
{
  return Memo("PORT") || IsTIDef();
}

/* ---------------------------------------------------------------------- */

static void SwitchFrom_3205x(void)
{
  DeinitFields();
}

/* ---------------------------------------------------------------------- */

static void SwitchTo_3205x(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x77;
  NOPCode = 0x8b00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;
  Grans[SegData] = 2; ListGrans[SegData] = 2; SegInits[SegData] = 0;
  SegLimits[SegData] = 0xffff;
  Grans[SegIO  ] = 2; ListGrans[SegIO  ] = 2; SegInits[SegIO  ] = 0;
  SegLimits[SegIO  ] = 0xffff;

  MakeCode = MakeCode_3205x;
  IsDef = IsDef_3205x;
  SwitchFrom = SwitchFrom_3205x;
  InitFields();
}

/* ---------------------------------------------------------------------- */

void code3205x_init(void)
{
  CPU320203 = AddCPU("320C203", SwitchTo_3205x);
  CPU32050  = AddCPU("320C50",  SwitchTo_3205x);
  CPU32051  = AddCPU("320C51",  SwitchTo_3205x);
  CPU32053  = AddCPU("320C53",  SwitchTo_3205x);

  AddCopyright("TMS320C5x-Generator (C) 1995/96 Thomas Sailer");
}
