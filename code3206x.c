/* code3206x.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TMS320C6x                                                   */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include "be_le.h"

#include "strutil.h"
#include "bpemu.h"
#include "nls.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmcode.h"
#include "errmsg.h"
#include "ieeefloat.h"
#include "codepseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "onoff_common.h"
#include "nlmessages.h"
#include "chartrans.h"
#include "as.rsc"

/*---------------------------------------------------------------------------*/

typedef enum
{
  NoUnit, L1, L2, S1, S2, M1, M2, D1, D2, LastUnit, UnitCnt
} TUnit;

#ifdef __cplusplus
# include "code3206x.hpp"
#endif

typedef struct
{
  LongInt OpCode;
  LongInt SrcMask, SrcMask2, DestMask;
  Byte CrossUsed; /* Bit 0 -->X1 benutzt, Bit 1 -->X2 benutzt */
  Byte AddrUsed;  /* Bit 0 -->Addr1 benutzt, Bit 1 -->Addr2 benutzt
                  Bit 2 -->LdSt1 benutzt, Bit 3 -->LdSt2 benutzt */
  Byte LongUsed;  /* Bit 0 -->lange Quelle, Bit 1-->langes Ziel */
  Boolean AbsBranch;
  Boolean StoreUsed, LongSrc, LongDest;
  TUnit U;
} InstrRec;

typedef struct
{
  LongInt Code;
} FixedOrder;

typedef struct
{
  LongInt Code;
  Boolean WithImm;
} CmpOrder;

typedef struct
{
  LongInt Code;
  LongInt Scale;
} MemOrder;

typedef struct
{
  LongInt Code;
  Boolean DSign,SSign1,SSign2;
  Boolean MayImm;
} MulOrder;

typedef struct
{
  const char *Name;
  LongInt Code;
  Boolean Rd,Wr;
} CtrlReg;

static const char UnitNames[UnitCnt][3] =
{
  "  ", "L1", "L2", "S1", "S2", "M1", "M2", "D1", "D2", "  "
};

#define MaxParCnt 8
#define FirstUnit L1

enum
{
  ModNone = -1,
  ModReg = 0,
  ModLReg = 1,
  ModImm = 2
};

#define MModReg (1 << ModReg)
#define MModLReg (1 << ModLReg)
#define MModImm (1 << ModImm)

static ShortInt AdrMode;

static CPUVar CPU32060;

static Boolean ThisPar, ThisCross, ThisStore, ThisAbsBranch;
static Byte ThisAddr, ThisLong;
static LongInt ThisSrc, ThisSrc2, ThisDest;
static LongInt Condition;
static TUnit ThisUnit;
static LongWord UnitFlag, ThisInst;
static Integer ParCnt;
static LongWord PacketAddr;

static InstrRec *ParRecs;

static FixedOrder *LinAddOrders;
static CmpOrder *CmpOrders;
static MemOrder *MemOrders;
static MulOrder *MulOrders;
static CtrlReg *CtrlRegs;

/*-------------------------------------------------------------------------*/

static Boolean CheckOpt(char *Asc)
{
  Boolean Flag, erg = True;
  int l = strlen(Asc);

  if (!strcmp(Asc, "||"))
    ThisPar = True;
  else if ((*Asc == '[') && (Asc[l - 1] == ']'))
  {
    Asc++;
    Asc[l - 2] = '\0';
    l -= 2;
    if (*Asc == '!')
    {
      Asc++;
      l--;
      Condition = 1;
    }
    else
      Condition = 0;
    Flag = True;
    if (l != 2)
      Flag = False;
    else if (as_toupper(*Asc) == 'A')
    {
      if ((Asc[1] >= '1') && (Asc[1] <= '2'))
        Condition += (Asc[1] - '0' + 3) << 1;
      else
        Flag = False;
    }
    else if (as_toupper(*Asc) == 'B')
    {
      if ((Asc[1] >= '0') && (Asc[1] <= '2'))
        Condition += (Asc[1] - '0' + 1) << 1;
      else
        Flag = False;
    }
    if (!Flag)
      WrXError(ErrNum_InvReg, Asc);
    erg = Flag;
  }
  else
    erg = False;

  return erg;
}

static Boolean ReiterateOpPart(void)
{
  char *p;
  int z;

  if (!CheckOpt(OpPart.str.p_str))
    return False;

  if (ArgCnt<1)
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
  p = FirstBlank(ArgStr[1].str.p_str);
  if (!p)
  {
    StrCompCopy(&OpPart, &ArgStr[1]);
    for (z = 2; z <= ArgCnt; z++)
      StrCompCopy(&ArgStr[z - 1], &ArgStr[z]);
    ArgCnt--;
  }
  else
  {
    StrCompSplitLeft(&ArgStr[1], &OpPart, p);
    KillPrefBlanksStrComp(&ArgStr[1]);
  }
  NLS_UpString(OpPart.str.p_str);
  p = strchr(OpPart.str.p_str, '.');
  if (!p)
    *AttrPart.str.p_str = '\0';
  else
  {
    strcpy(AttrPart.str.p_str, p + 1);
    *p = '\0';
  }
  return True;
}

/*-------------------------------------------------------------------------*/

static void AddSrc(LongWord Reg)
{
  LongWord Mask = 1 << Reg;

  if (!(ThisSrc & Mask))
    ThisSrc |= Mask;
  else
    ThisSrc2 |= Mask;
}

static void AddLSrc(LongWord Reg)
{
  AddSrc(Reg);
  AddSrc(Reg + 1);
  ThisLong |= 1;
}

static void AddDest(LongWord Reg)
{
  ThisDest |= (1 << Reg);
}

static void AddLDest(LongWord Reg)
{
  ThisDest |= (3 << Reg);
  ThisLong |= 2;
}

static LongInt FindReg(LongInt Mask)
{
  int z;

  for (z = 0; z < 32; z++)
  {
    if (Mask & 1)
      break;
    Mask = Mask >> 1;
  }
  return z;
}

static char *RegName(LongInt Num)
{
  static char s[5];

  Num &= 31;
  as_snprintf(s, sizeof(s), "%c%ld", 'A' + (Num >> 4), (long) (Num & 15));
  return s;
}

static Boolean DecodeSReg(char *Asc, LongWord *Reg, Boolean Quarrel)
{
  char *end;
  Byte RVal;
  Boolean TFlag;

  TFlag = True;
  switch (as_toupper(*Asc))
  {
    case 'A':
      *Reg = 0; break;
    case 'B':
      *Reg = 16; break;
    default:
      TFlag = False;
  }
  if (TFlag)
  {
    RVal = strtol(Asc + 1, &end, 10);
    if (*end != '\0')
      TFlag = False;
    else if
      (RVal>15) TFlag = False;
    else
      *Reg += RVal;
  }
  if ((!TFlag) && (Quarrel))
    WrXError(ErrNum_InvReg, Asc);
  return TFlag;
}

static Boolean DecodeReg(char *Asc, LongWord *Reg, Boolean *PFlag, Boolean Quarrel)
{
  char *p;
  LongWord NextReg;

  p = strchr(Asc, ':');
  if (p == 0)
  {
    *PFlag = False;
    return DecodeSReg(Asc, Reg, Quarrel);
  }
  else
  {
    *PFlag = True;
    *p = '\0';
    if (!DecodeSReg(Asc, &NextReg, Quarrel))
      return False;
    else if (!DecodeSReg(p + 1, Reg, Quarrel))
     return False;
    else if ((Odd(*Reg)) || (NextReg != (*Reg) + 1) || ((((*Reg) ^ NextReg) & 0x10) != 0))
    {
      if (Quarrel)
        WrXError(ErrNum_InvRegPair, Asc);
      return False;
    }
    else
      return True;
  }
}

static Boolean DecodeCtrlReg(char *Asc, LongWord *Erg, Boolean Write)
{
  int z;

  for (z = 0; CtrlRegs[z].Name; z++)
    if (!as_strcasecmp(Asc, CtrlRegs[z].Name))
    {
      *Erg = CtrlRegs[z].Code;
      return (Write && CtrlRegs[z].Wr) || ((!Write) && CtrlRegs[z].Rd);
    }
  return False;
}

/* Was bedeutet das r-Feld im Adressoperanden mit kurzem Offset ???
   und wie ist das genau mit der Skalierung gemeint ??? */

static Boolean DecodeMem(const tStrComp *pArg, LongWord *Erg, LongWord Scale)
{
  LongInt DispAcc, Mode;
  LongWord BaseReg, IndReg;
  int l;
  char Counter;
  char *p, EmptyStr[] = "";
  Boolean OK;
  tStrComp Arg, DispArg, RegArg;

  StrCompRefRight(&Arg, pArg, 0);

  /* das muss da sein */

  if (*pArg->str.p_str != '*')
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  StrCompIncRefLeft(&Arg, 1);

  /* teilen */

  p = strchr(Arg.str.p_str, '[');
  Counter = ']';
  if (!p)
  {
    p = strchr(Arg.str.p_str, '(');
    Counter = ')';
  }
  if (p)
  {
    if (Arg.str.p_str[strlen(Arg.str.p_str) - 1] != Counter)
    {
      WrError(ErrNum_InvAddrMode);
      return False;
    }
    StrCompSplitRef(&RegArg, &DispArg, &Arg, p);
    StrCompShorten(&DispArg, 1);
  }
  else
  {
    RegArg = Arg;
    StrCompMkTemp(&DispArg, EmptyStr, 0);
  }

  /* Registerfeld entschluesseln */

  l = strlen(RegArg.str.p_str);
  Mode = 1; /* Default ist *+R */
  if (*RegArg.str.p_str == '+')
  {
    StrCompIncRefLeft(&RegArg, 1);
    Mode = 1;
    if (*RegArg.str.p_str == '+')
    {
      StrCompIncRefLeft(&RegArg, 1);
      Mode = 9;
    }
  }
  else if (*RegArg.str.p_str == '-')
  {
    StrCompIncRefLeft(&RegArg, 1);
    Mode = 0;
    if (*RegArg.str.p_str == '-')
    {
      StrCompIncRefLeft(&RegArg, 1);
      Mode = 8;
    }
  }
  else if (RegArg.str.p_str[l - 1] == '+')
  {
    if (RegArg.str.p_str[l - 2] != '+')
    {
      WrError(ErrNum_InvAddrMode);
      return False;
    }
    StrCompShorten(&RegArg, 2);
    Mode = 11;
  }
  else if (RegArg.str.p_str[l - 1] == '-')
  {
    if (RegArg.str.p_str[l - 2] != '-')
    {
      WrError(ErrNum_InvAddrMode);
      return False;
    }
    StrCompShorten(&RegArg, 2);
    Mode = 10;
  }
  if (!DecodeSReg(RegArg.str.p_str, &BaseReg, False))
  {
    WrStrErrorPos(ErrNum_InvReg, &RegArg);
    return False;
  }
  AddSrc(BaseReg);

  /* kein Offsetfeld ? --> Skalierungsgroesse bei Autoinkrement/De-
     krement, sonst 0 */

  if (*DispArg.str.p_str == '\0')
    DispAcc = (Mode < 2) ? 0 : Scale;

  /* Register als Offsetfeld? Dann Bit 2 in Modus setzen */

  else if (DecodeSReg(DispArg.str.p_str, &IndReg, False))
  {
    if ((IndReg ^ BaseReg) > 15)
    {
      WrError(ErrNum_InvAddrMode);
      return False;
    }
    Mode += 4;
    AddSrc(DispAcc = IndReg);
  }

  /* ansonsten normaler Offset */

  else
  {
    tSymbolFlags Flags;

    DispAcc = EvalStrIntExpressionWithFlags(&DispArg, UInt15, &OK, &Flags);
    if (!OK)
      return False;
    if (mFirstPassUnknown(Flags))
      DispAcc &= 7;
    if (Counter  ==  ')')
    {
      if (DispAcc % Scale != 0)
      {
        WrError(ErrNum_NotAligned);
        return False;
      }
      else
       DispAcc /= Scale;
    }
  }

  /* Benutzung des Adressierers markieren */

  ThisAddr |= (BaseReg > 15) ? 2 : 1;

  /* Wenn Offset>31, muessen wir Variante 2 benutzen */

  if (((Mode & 4) == 0) && (DispAcc > 31))
  {
    if ((BaseReg < 0x1e) || (Mode != 1)) WrError(ErrNum_InvAddrMode);
    else
    {
      *Erg = ((DispAcc & 0x7fff) << 8) + ((BaseReg & 1) << 7) + 12;
      return True;
    }
  }

  else
  {
    *Erg = (BaseReg << 18) + ((DispAcc & 0x1f) << 13) + (Mode << 9)
         + ((BaseReg & 0x10) << 3) + 4;
    return True;
  }

  return False;
}

static Boolean DecodeAdr(const tStrComp *pArg, Byte Mask, Boolean Signed, LongWord *AdrVal)
{
  Boolean OK;

  AdrMode = ModNone;

  if (DecodeReg(pArg->str.p_str, AdrVal, &OK, False))
  {
    AdrMode = (OK) ? ModLReg : ModReg;
  }
  else
  {
    *AdrVal = Signed ?
              EvalStrIntExpression(pArg, SInt5, &OK) & 0x1f :
              EvalStrIntExpression(pArg, UInt5, &OK);
    if (OK)
      AdrMode = ModImm;
  }

  if ((AdrMode != ModNone) && (((1 << AdrMode) & Mask) == 0))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
    return False;
  }
  else return True;
}

static Boolean ChkUnit(LongWord Reg, TUnit U1, TUnit U2)
{
  UnitFlag = Ord(Reg>15);
  if (ThisUnit == NoUnit)
  {
    ThisUnit = (Reg>15) ? U2 : U1;
    return True;
  }
  else if (((ThisUnit == U1) && (Reg < 16)) || ((ThisUnit == U2) && (Reg>15)))
    return True;
  else
  {
    WrError(ErrNum_UndefAttr);
    return False;
  }
}

static TUnit UnitCode(char c)
{
  switch (c)
  {
    case 'L': return L1;
    case 'S': return S1;
    case 'D': return D1;
    case 'M': return M1;
    default: return NoUnit;
  }
}

static Boolean UnitUsed(TUnit TestUnit)
{
  Integer z;

  for (z = 0; z < ParCnt; z++)
    if (ParRecs[z].U == TestUnit)
      return True;

  return False;
}

static Boolean IsCross(LongWord Reg)
{
  return (Reg >> 4) != UnitFlag;
}

static void SetCross(LongWord Reg)
{
  ThisCross = ((Reg >> 4) != UnitFlag);
}

static Boolean DecideUnit(LongWord Reg, const char *Units)
{
  Integer z;
  TUnit TestUnit;

  if (ThisUnit == NoUnit)
  {
    z = 0;
    while ((Units[z] != '\0') && (ThisUnit == NoUnit))
    {
      TestUnit = UnitCode(Units[z]);
      if (Reg >= 16) TestUnit++;
      if (!UnitUsed(TestUnit))
        ThisUnit = TestUnit;
      z++;
    }
    if (ThisUnit == NoUnit)
    {
      ThisUnit = UnitCode(*Units);
      if (Reg > 16)
        TestUnit++;
    }
  }
  UnitFlag = (ThisUnit - FirstUnit) & 1;
  if (IsCross(Reg))
  {
    WrError(ErrNum_UndefAttr);
    return False;
  }
  else
    return True;
}

static void SwapReg(LongWord *r1, LongWord *r2)
{
  LongWord tmp;

  tmp = (*r1);
  *r1 = (*r2);
  *r2 = tmp;
}

static Boolean DecodePseudo(void)
{
  Boolean OK;
  unsigned dword_index;
  tStrComp *pArg;
  LongInt Size;

  if (Memo("SINGLE"))
  {
    if (ChkArgCnt(1, ArgCntMax))
    {
      OK = True;
      forallargs(pArg, OK)
      {
        double Float = EvalStrFloatExpression(pArg, Float32, &OK);

        if (OK)
        {
          dword_index = CodeLen >> 2;
          Double_2_ieee4(Float, (Byte *) (DAsmCode + dword_index), HostBigEndian);
          CodeLen += 4;
        }
      }
      if (!OK) CodeLen = 0;
    }
    return True;
  }

  if (Memo("DOUBLE"))
  {
    if (ChkArgCnt(1, ArgCntMax))
    {
      double Float;

      OK = True;
      forallargs(pArg, OK)
      {
        Float = EvalStrFloatExpression(pArg, Float64, &OK);
        if (OK)
        {
          dword_index = CodeLen >> 2;
          Double_2_ieee8(Float, (Byte *) (DAsmCode + dword_index), HostBigEndian);
          if (!HostBigEndian)
          {
            DAsmCode[dword_index + 2] = DAsmCode[dword_index + 0];
            DAsmCode[dword_index + 0] = DAsmCode[dword_index + 1];
            DAsmCode[dword_index + 1] = DAsmCode[dword_index + 2];
          }
          CodeLen += 8;
        }
      }
      if (!OK) CodeLen = 0;
    }
    return True;
  }

  if (Memo("DATA"))
  {
    if (ChkArgCnt(1, ArgCntMax))
    {
      TempResult t;
      int cnt = 0;

      as_tempres_ini(&t);
      OK = True;
      forallargs (pArg, OK)
      {
        EvalStrExpression(pArg, &t);
        switch (t.Typ)
        {
          case TempString:
          {
            unsigned z2;
            LongWord Trans;

            if (MultiCharToInt(&t, 4))
              goto ToInt;

            if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
              OK = False;
            else
              for (z2 = 0; z2 < t.Contents.str.len; z2++)
              {
                Trans = t.Contents.str.p_str[z2] & 0xff;
                if (Packing)
                {
                  if ((z2 & 3) == 0) DAsmCode[cnt++] = 0;
                  DAsmCode[cnt - 1] += Trans << (8 * (3 - (z2 & 3)));
                }
                else
                  DAsmCode[cnt++] = Trans;
              }
            break;
          }
          case TempInt:
          ToInt:
#ifdef HAS64
            if (!RangeCheck(t.Contents.Int, Int32))
            {
              OK = False;
              WrStrErrorPos(ErrNum_OverRange, pArg);
            }
            else
#endif
              DAsmCode[cnt++] = t.Contents.Int;
            break;
          case TempFloat:
            if (!FloatRangeCheck(t.Contents.Float, Float32))
            {
              OK = False;
              WrStrErrorPos(ErrNum_OverRange, pArg);
            }
            else
            {
              Double_2_ieee4(t.Contents.Float, (Byte *) (DAsmCode + cnt), HostBigEndian);
              cnt++;
            }
            break;
          default:
            OK = False;
        }
      }
      if (OK)
        CodeLen = cnt << 2;
      as_tempres_free(&t);
    }
    return True;
  }

  if (Memo("BSS"))
  {
    if (ChkArgCnt(1, 1))
    {
      tSymbolFlags Flags;

      Size = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags);
      if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
      if (OK && !mFirstPassUnknown(Flags))
      {
        DontPrint = True;
        if (!Size) WrError(ErrNum_NullResMem);
        CodeLen = Size;
        BookKeeping();
      }
    }
    return True;
  }

  return False;
}


static Boolean CodeL(LongWord OpCode, LongWord Dest, LongWord Src1, LongWord Src2)
{
  ThisInst = 0x18 + (OpCode << 5) + (UnitFlag << 1) + (Ord(ThisCross) << 12)
                  + (Dest << 23) + (Src2 << 18) + (Src1 << 13);
  return True;
}

static Boolean CodeM(LongWord OpCode, LongWord Dest, LongWord Src1, LongWord Src2)
{
  ThisInst = 0x00 + (OpCode << 7) + (UnitFlag << 1) + (Ord(ThisCross) << 12)
                  + (Dest << 23) + (Src2 << 18) + (Src1 << 13);
  return True;
}

static Boolean CodeS(LongWord OpCode, LongWord Dest, LongWord Src1, LongWord Src2)
{
  ThisInst = 0x20 + (OpCode << 6) + (UnitFlag << 1) + (Ord(ThisCross) << 12)
                  + (Dest << 23) + (Src2 << 18) + (Src1 << 13);
  return True;
}

static Boolean CodeD(LongWord OpCode, LongWord Dest, LongWord Src1, LongWord Src2)
{
  ThisInst = 0x40 + (OpCode << 7) + (UnitFlag << 1)
                  + (Dest << 23) + (Src2 << 18) + (Src1 << 13);
  return True;
}

/*-------------------------------------------------------------------------*/

static Boolean __erg;

static void DecodeIDLE(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(0, 0));
  else if ((ThisCross) || (ThisUnit != NoUnit)) WrError(ErrNum_UndefAttr);
  else
  {
    ThisInst = 0x0001e000;
    __erg = True;
  }
}

static void DecodeNOP(Word Index)
{
  LongInt Count;
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(0, 1));
  else if ((ThisCross) || (ThisUnit != NoUnit)) WrError(ErrNum_UndefAttr);
  else
  {
    if (ArgCnt == 0)
    {
      OK = True;
      Count = 0;
    }
    else
    {
      tSymbolFlags Flags;

      Count = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt4, &OK, &Flags);
      Count = mFirstPassUnknown(Flags) ? 0 : Count - 1;
      OK = ChkRange(Count, 0, 8);
    }
    if (OK)
    {
      ThisInst = Count << 13;
      __erg = True;
    }
  }
}

static void DecodeMul(Word Index)
{
  LongWord DReg, S1Reg, S2Reg;
  MulOrder *POrder = MulOrders + Index;

  if (ChkArgCnt(3, 3))
  {
    if ((DecodeAdr(&ArgStr[3], MModReg, POrder->DSign, &DReg))
     && (ChkUnit(DReg, M1, M2)))
    {
      if (DecodeAdr(&ArgStr[2], MModReg, POrder->SSign2, &S2Reg))
      {
        AddSrc(S2Reg);
        DecodeAdr(&ArgStr[1], (POrder->MayImm ? MModImm : 0) + MModReg, POrder->SSign1, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:
            if ((ThisCross) && (!IsCross(S2Reg)) && (!IsCross(S1Reg))) WrError(ErrNum_InvAddrMode);
            else if ((IsCross(S2Reg)) && (IsCross(S1Reg))) WrError(ErrNum_InvAddrMode);
            else
            {
              if (IsCross(S1Reg))
                SwapReg(&S1Reg, &S2Reg);
              SetCross(S2Reg);
              AddSrc(S1Reg);
              __erg = CodeM(POrder->Code, DReg, S1Reg, S2Reg);
            }
            break;
          case ModImm:
            __erg = Memo("MPY") ?
                    CodeM(POrder->Code - 1, DReg, S1Reg, S2Reg) :
                    CodeM(POrder->Code + 3, DReg, S1Reg, S2Reg);
            break;
        }
      }
    }
  }
}

static void DecodeMemO(Word Index)
{
  LongWord DReg, S1Reg;
  MemOrder *POrder = MemOrders + Index;
  Boolean OK, IsStore;

  if (ChkArgCnt(2, 2))
  {
    const tStrComp *pArg1, *pArg2;

    IsStore = (*OpPart.str.p_str) == 'S';
    pArg1 = IsStore ? &ArgStr[2] : &ArgStr[1];
    pArg2 = IsStore ? &ArgStr[1] : &ArgStr[2];
    if (IsStore)
      ThisStore = True;
    if (DecodeAdr(pArg2, MModReg, False, &DReg))
    {
      if (IsStore)
        AddSrc(DReg);
      ThisAddr |= (DReg > 15) ? 8 : 4;
      /* Zielregister 4 Takte verzoegert, nicht als Dest eintragen */
      OK = DecodeMem(pArg1, &S1Reg, POrder->Scale);
      if (OK)
      {
        OK = (S1Reg & 8) ?
             ChkUnit(0x1e, D1, D2) :
             ChkUnit((S1Reg >> 18) & 31, D1, D2);
      }
      if (OK)
      {
        ThisInst = S1Reg + (DReg << 23) + (POrder->Code << 4)
                 + ((DReg & 16) >> 3);
        __erg = True;
      }
    }
  }
}

static void DecodeSTP(Word Index)
{
  LongWord S2Reg;
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkUnit(0x10, S1, S2))
  {
    if (DecodeAdr(&ArgStr[1], MModReg, False, &S2Reg))
    {
      if ((ThisCross) || (S2Reg < 16)) WrError(ErrNum_InvAddrMode);
      else
      {
        AddSrc(S2Reg);
        __erg = CodeS(0x0c, 0, 0, S2Reg);
      }
    }
  }
}

static void DecodeABS(Word Index)
{
  Boolean DPFlag, S1Flag;
  LongWord DReg, S1Reg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && DecodeReg(ArgStr[2].str.p_str, &DReg, &DPFlag, True)
   && ChkUnit(DReg, L1, L2)
   && DecodeReg(ArgStr[1].str.p_str, &S1Reg, &S1Flag, True))
  {
    if (DPFlag != S1Flag) WrError(ErrNum_InvAddrMode);
    else if ((ThisCross) && ((S1Reg >> 4) == UnitFlag)) WrError(ErrNum_InvAddrMode);
    else
    {
      SetCross(S1Reg);
      if (DPFlag)
      {
        __erg = CodeL(0x38, DReg, 0, S1Reg);
        AddLSrc(S1Reg);
        AddLDest(DReg);
      }
      else
      {
        __erg = CodeL(0x1a, DReg, 0, S1Reg);
        AddSrc(S1Reg);
        AddDest(DReg);
      }
    }
  }
}

static void DecodeADD(Word Index)
{
  LongWord S1Reg, S2Reg, DReg;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[3], MModReg + MModLReg, True, &DReg);
    UnitFlag = DReg >> 4;
    switch (AdrMode)
    {
      case ModLReg:      /* ADD ?,?,long */
        AddLDest(DReg);
        DecodeAdr(&ArgStr[1], MModReg + MModLReg + MModImm, True, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:    /* ADD int,?,long */
            AddSrc(S1Reg);
            DecodeAdr(&ArgStr[2], MModReg + MModLReg, True, &S2Reg);
            switch (AdrMode)
            {
              case ModReg: /* ADD int,int,long */
                if (ChkUnit(DReg, L1, L2))
                {
                  if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                  else if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    AddSrc(S2Reg);
                    if (IsCross(S1Reg))
                      SwapReg(&S1Reg, &S2Reg);
                    SetCross(S2Reg);
                    __erg = CodeL(0x23, DReg, S1Reg, S2Reg);
                  }
                }
                break;
              case ModLReg:/* ADD int,long,long */
                if (ChkUnit(DReg, L1, L2))
                {
                  if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
                  else if ((ThisCross) && (!IsCross(S1Reg))) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    AddLSrc(S2Reg);
                    SetCross(S1Reg);
                    __erg = CodeL(0x21, DReg, S1Reg, S2Reg);
                  }
                }
                break;
            }
            break;
          case ModLReg:   /* ADD long,?,long */
            AddLSrc(S1Reg);
            DecodeAdr(&ArgStr[2], MModReg + MModImm, True, &S2Reg);
            switch (AdrMode)
            {
              case ModReg: /* ADD long,int,long */
                if (ChkUnit(DReg, L1, L2))
                {
                  if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                  else if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    AddSrc(S2Reg);
                    SetCross(S2Reg);
                    __erg = CodeL(0x21, DReg, S2Reg, S1Reg);
                  }
                }
                break;
              case ModImm: /* ADD long,imm,long */
                if (ChkUnit(DReg, L1, L2))
                {
                  if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                  else if (ThisCross) WrError(ErrNum_InvAddrMode);
                  else
                    __erg = CodeL(0x20, DReg, S2Reg, S1Reg);
                }
                break;
             }
             break;
           case ModImm:    /* ADD imm,?,long */
             if (DecodeAdr(&ArgStr[2], MModLReg, True, &S2Reg))
             {         /* ADD imm,long,long */
               if (ChkUnit(DReg, L1, L2))
               {
                 if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
                 else if (ThisCross) WrError(ErrNum_InvAddrMode);
                 else
                 {
                   AddLSrc(S2Reg);
                   __erg = CodeL(0x20, DReg, S1Reg, S2Reg);
                 }
               }
             }
             break;
        }
        break;
      case ModReg:       /* ADD ?,?,int */
        AddDest(DReg);
        DecodeAdr(&ArgStr[1], MModReg + MModImm, True, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:    /* ADD int,?,int */
            AddSrc(S1Reg);
            DecodeAdr(&ArgStr[2], MModReg + MModImm, True, &S2Reg);
            switch (AdrMode)
            {
              case ModReg: /* ADD int,int,int */
                AddSrc(S2Reg);
                if (((DReg ^ S1Reg) > 15) && ((DReg ^ S2Reg)>15)) WrError(ErrNum_InvAddrMode);
                else if ((ThisCross) && ((DReg ^ S1Reg) < 16) && ((DReg ^ S2Reg) < 15)) WrError(ErrNum_InvAddrMode);
                else
                {
                  if ((S1Reg ^ DReg) > 15)
                    SwapReg(&S1Reg, &S2Reg);
                  OK = DecideUnit(DReg, ((S2Reg ^ DReg)>15) ? "LS" : "LSD");
                  if (OK)
                  {
                    switch (ThisUnit)
                    {
                      case L1: case L2: /* ADD.Lx int,int,int */
                        __erg = CodeL(0x03, DReg, S1Reg, S2Reg);
                        break;
                      case S1: case S2: /* ADD.Sx int,int,int */
                        __erg = CodeS(0x07, DReg, S1Reg, S2Reg);
                        break;
                      case D1: case D2: /* ADD.Dx int,int,int */
                        __erg = CodeD(0x10, DReg, S1Reg, S2Reg);
                        break;
                      default:
                        WrError(ErrNum_CannotUseUnit);
                    }
                  }
                }
                break;
              case ModImm: /* ADD int,imm,int */
                if ((ThisCross) && ((S1Reg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
                else
                {
                  SetCross(S1Reg);
                  if (DecideUnit(DReg, "LS"))
                    switch (ThisUnit)
                    {
                      case L1: case L2:
                        __erg = CodeL(0x02, DReg, S2Reg, S1Reg);
                        break;
                      case S1: case S2:
                        __erg = CodeS(0x06, DReg, S2Reg, S1Reg);
                        break;
                      default:
                        WrError(ErrNum_CannotUseUnit);
                    }
                }
                break;
            }
            break;
          case ModImm:   /* ADD imm,?,int */
            if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
            {
              AddSrc(S2Reg);
              if ((ThisCross) && ((S2Reg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
              else
              {
                SetCross(S2Reg);
                if (DecideUnit(DReg, "LS"))
                  switch (ThisUnit)
                  {
                    case L1: case L2:
                      __erg = CodeL(0x02, DReg, S1Reg, S2Reg);
                      break;
                    case S1: case S2:
                      __erg = CodeS(0x06, DReg, S1Reg, S2Reg);
                      break;
                    default:
                      WrError(ErrNum_CannotUseUnit);
                  }
              }
            }
            break;
        }
        break;
    }
  }
}

static void DecodeADDU(Word Index)
{
  LongWord DReg, S1Reg, S2Reg;
  UNUSED(Index);

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[3], MModReg + MModLReg, False, &DReg);
    switch (AdrMode)
    {
      case ModReg:      /* ADDU ?,?,int */
        if (ChkUnit(DReg, D1, D2))
        {
          AddDest(DReg);
          DecodeAdr(&ArgStr[1], MModReg + MModImm, False, &S1Reg);
          switch (AdrMode)
          {
            case ModReg: /* ADDU int,?,int */
              if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
              else
              {
                AddSrc(S1Reg);
                if (DecodeAdr(&ArgStr[2], MModImm, False, &S2Reg))
                 __erg = CodeD(0x12, DReg, S2Reg, S1Reg);
              }
              break;
            case ModImm: /* ADDU imm,?,int */
              if (DecodeAdr(&ArgStr[2], MModReg, False, &S2Reg))
              {
                if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
                else
                {
                  AddSrc(S2Reg);
                  __erg = CodeD(0x12, DReg, S1Reg, S2Reg);
                }
              }
              break;
          }
        }
        break;
      case ModLReg:     /* ADDU ?,?,long */
        if (ChkUnit(DReg, L1, L2))
        {
          AddLDest(DReg);
          DecodeAdr(&ArgStr[1], MModReg + MModLReg, False, &S1Reg);
          switch (AdrMode)
          {
            case ModReg: /* ADDU int,?,long */
              AddSrc(S1Reg);
              DecodeAdr(&ArgStr[2], MModReg + MModLReg, False, &S2Reg);
              switch (AdrMode)
              {
                case ModReg: /* ADDU int,int,long */
                  if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                  else if ((ThisCross) && (((S1Reg ^ DReg) < 16) && ((S2Reg ^ DReg) < 16))) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    if ((S1Reg ^ DReg) > 15)
                      SwapReg(&S1Reg, &S2Reg);
                    SetCross(S2Reg);
                    __erg = CodeL(0x2b, DReg, S1Reg, S2Reg);
                  }
                  break;
                case ModLReg: /* ADDU int,long,long */
                  if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
                  else if ((ThisCross) && ((S1Reg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    AddLSrc(S2Reg);
                    SetCross(S1Reg);
                    __erg = CodeL(0x29, DReg, S1Reg, S2Reg);
                  }
                  break;
              }
              break;
            case ModLReg:
              if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
              else
              {
                AddLSrc(S1Reg);
                if (DecodeAdr(&ArgStr[2], MModReg, False, &S2Reg))
                {
                  if ((ThisCross) && ((S2Reg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    AddSrc(S2Reg); SetCross(S2Reg);
                    __erg = CodeL(0x29, DReg, S2Reg, S1Reg);
                  }
                }
              }
              break;
          }
        }
        break;
    }
  }
}

static void DecodeSUB(Word Index)
{
  LongWord DReg, S1Reg, S2Reg;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[3], MModReg + MModLReg, True, &DReg);
    switch (AdrMode)
    {
      case ModReg:
        AddDest(DReg);
        DecodeAdr(&ArgStr[1], MModReg + MModImm, True, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:
            AddSrc(S1Reg);
            DecodeAdr(&ArgStr[2], MModReg + MModImm, True, &S2Reg);
            switch (AdrMode)
            {
              case ModReg:
               if ((ThisCross) && ((S1Reg ^ DReg) < 16) && ((S2Reg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
               else if (((S1Reg ^ DReg) > 15) && ((S2Reg ^ DReg) > 15)) WrError(ErrNum_InvAddrMode);
               else
               {
                 AddSrc(S2Reg);
                 ThisCross = ((S1Reg ^ DReg) > 15) || ((S2Reg ^ DReg) > 15);
                 if ((S1Reg ^ DReg) > 15) OK = DecideUnit(DReg, "L");
                 else if ((S2Reg ^ DReg) > 15) OK = DecideUnit(DReg, "LS");
                 else OK = DecideUnit(DReg, "LSD");
                 if (OK)
                   switch (ThisUnit)
                   {
                     case L1: case L2:
                       if ((S1Reg ^ DReg) > 15) __erg = CodeL(0x17, DReg, S1Reg, S2Reg);
                       else __erg = CodeL(0x07, DReg, S1Reg, S2Reg);
                       break;
                     case S1: case S2:
                       __erg = CodeS(0x17, DReg, S1Reg, S2Reg);
                       break;
                     case D1: case D2:
                       __erg = CodeD(0x11, DReg, S2Reg, S1Reg);
                       break;
                     default:
                       WrError(ErrNum_CannotUseUnit);
                   }
               }
               break;
              case ModImm:
               if (ChkUnit(DReg, D1, D2))
               {
                 if ((ThisCross) || ((S1Reg ^ DReg) > 15)) WrError(ErrNum_InvAddrMode);
                 else __erg = CodeD(0x13, DReg, S2Reg, S1Reg);
               }
               break;
            }
            break;
          case ModImm:
            if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
            {
              if ((ThisCross) && ((S2Reg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
              else
              {
                AddSrc(S2Reg);
                if (DecideUnit(DReg, "LS"))
                  switch (ThisUnit)
                  {
                    case L1: case L2:
                      __erg = CodeL(0x06, DReg, S1Reg, S2Reg);
                      break;
                    case S1: case S2:
                      __erg = CodeS(0x16, DReg, S1Reg, S2Reg);
                      break;
                    default:
                      WrError(ErrNum_CannotUseUnit);
                  }
              }
            }
            break;
        }
        break;
      case ModLReg:
        AddLDest(DReg);
        if (ChkUnit(DReg, L1, L2))
        {
          DecodeAdr(&ArgStr[1], MModImm + MModReg, True, &S1Reg);
          switch (AdrMode)
          {
            case ModImm:
              if (DecodeAdr(&ArgStr[2], MModLReg, True, &S2Reg))
              {
                if ((ThisCross) || (/*NOT*/ IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                else
                {
                  AddLSrc(S2Reg);
                  __erg = CodeL(0x24, DReg, S1Reg, S2Reg);
                }
              }
              break;
            case ModReg:
              AddSrc(S1Reg);
              if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
              {
                if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                else if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                else
                {
                  AddSrc(S2Reg);
                  ThisCross = (IsCross(S1Reg)) || (IsCross(S2Reg));
                  /* what did I do here? */
                  __erg = IsCross(S1Reg) ?
                          CodeL(0x37, DReg, S1Reg, S2Reg) :
                          CodeL(0x47, DReg, S1Reg, S2Reg);
                }
              }
              break;
          }
        }
        break;
    }
  }
}

static void DecodeSUBU(Word Index)
{
  LongWord S1Reg, S2Reg, DReg;
  UNUSED(Index);

  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[3], MModLReg, False, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    AddLDest(DReg);
    if (DecodeAdr(&ArgStr[1], MModReg, False, &S1Reg))
    {
      AddSrc(S1Reg);
      if (DecodeAdr(&ArgStr[2], MModReg, False, &S2Reg))
      {
        if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else
        {
          AddSrc(S2Reg);
          ThisCross = IsCross(S1Reg) || IsCross(S2Reg);
          __erg = IsCross(S1Reg) ?
                  CodeL(0x3f, DReg, S1Reg, S2Reg) :
                  CodeL(0x2f, DReg, S1Reg, S2Reg);
        }
      }
    }
  }
}

static void DecodeSUBC(Word Index)
{
  LongWord DReg, S1Reg, S2Reg;
  UNUSED(Index);

  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[3], MModReg, False, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    AddLDest(DReg);
    if (DecodeAdr(&ArgStr[1], MModReg, False, &S1Reg))
    {
      if (DecodeAdr(&ArgStr[2], MModReg, False, &S2Reg))
      {
        if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
        else
        {
          AddSrc(S2Reg);
          SetCross(S2Reg);
          __erg = CodeL(0x4b, DReg, S1Reg, S2Reg);
        }
      }
    }
  }
}

static void DecodeLinAdd(Word Index)
{
  LongWord DReg, S1Reg, S2Reg;
  FixedOrder *POrder = LinAddOrders + Index;

  if (!ChkArgCnt(3, 3));
  else if (ThisCross) WrError(ErrNum_InvAddrMode);
  else
  {
    if ((DecodeAdr(&ArgStr[3], MModReg, True, &DReg))
     && (ChkUnit(DReg, D1, D2)))
    {
      AddDest(DReg);
      if (DecodeAdr(&ArgStr[1], MModReg, True, &S2Reg))
      {
        if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
        else
        {
          AddSrc(S2Reg);
          DecodeAdr(&ArgStr[2], MModReg + MModImm, False, &S1Reg);
          switch (AdrMode)
          {
            case ModReg:
              if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
              else
              {
                AddSrc(S1Reg);
                __erg = CodeD(POrder->Code, DReg, S1Reg, S2Reg);
              }
              break;
            case ModImm:
              __erg = CodeD(POrder->Code + 2, DReg, S1Reg, S2Reg);
              break;
          }
        }
      }
    }
  }
}

static void DecodeADDK(Word Index)
{
  LongInt Value;
  LongWord DReg;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[2], MModReg, False, &DReg)
   && ChkUnit(DReg, S1, S2))
  {
    AddDest(DReg);
    Value = EvalStrIntExpression(&ArgStr[1], SInt16, &OK);
    if (OK)
    {
      ThisInst = 0x50 + (UnitFlag << 1) + ((Value & 0xffff) << 7) + (DReg << 23);
      __erg = True;
    }
  }
}

static void DecodeADD2_SUB2(Word Index)
{
  LongWord DReg, S1Reg, S2Reg;
  Boolean OK;

  Index = (Index << 5) + 1;
  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[3], MModReg, True, &DReg)
   && ChkUnit(DReg, S1, S2))
  {
    AddDest(DReg);
    if (DecodeAdr(&ArgStr[1], MModReg, True, &S1Reg))
    {
      AddSrc(S1Reg);
      if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
      {
        if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else
        {
          OK = True; AddSrc(S2Reg);
          if (IsCross(S1Reg))
          {
            if (Index > 1)
            {
              WrError(ErrNum_InvAddrMode);
              OK = False;
            }
            else SwapReg(&S1Reg, &S2Reg);
          }
          if (OK)
          {
            SetCross(S2Reg);
            __erg = CodeS(Index, DReg, S1Reg, S2Reg);
          }
        }
      }
    }
  }
}

static void DecodeMV(Word Index)
{
  LongWord SReg, DReg;
  UNUSED(Index);

  /* MV src,dst == ADD 0,src,dst */

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg + MModLReg, True, &DReg);
    UnitFlag = DReg >> 4;
    switch (AdrMode)
    {
      case ModLReg:      /* MV ?,long */
        AddLDest(DReg);
        if (DecodeAdr(&ArgStr[1], MModLReg, True, &SReg))
        {                 /* MV long,long */
          if (ChkUnit(DReg, L1, L2))
          {
            if (IsCross(SReg)) WrError(ErrNum_InvAddrMode);
            else if (ThisCross) WrError(ErrNum_InvAddrMode);
            else
            {
              AddLSrc(SReg);
              __erg = CodeL(0x20, DReg, 0, SReg);
            }
          }
        }
        break;

      case ModReg:       /* MV ?,int */
        AddDest(DReg);
        if (DecodeAdr(&ArgStr[1], MModReg, True, &SReg))
        {
          AddSrc(SReg);
          if ((ThisCross) && ((SReg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
          else
          {
            SetCross(SReg);
            if (DecideUnit(DReg, "LSD"))
              switch (ThisUnit)
              {
                case L1: case L2:
                  __erg = CodeL(0x02, DReg, 0, SReg);
                  break;
                case S1: case S2:
                  __erg = CodeS(0x06, DReg, 0, SReg);
                  break;
                case D1: case D2:
                  __erg = CodeD(0x12, DReg, 0, SReg);
                  break;
                default:
                  WrError(ErrNum_CannotUseUnit);
              }
          }
        }
        break;
    }
  }
}

static void DecodeNEG(Word Index)
{
  LongWord DReg, SReg;
  UNUSED(Index);

  /* NEG src,dst == SUB 0,src,dst */

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg + MModLReg, True, &DReg);
    switch (AdrMode)
    {
      case ModReg:
        AddDest(DReg);
        if (DecodeAdr(&ArgStr[1], MModReg, True, &SReg))
        {
          if ((ThisCross) && ((SReg ^ DReg) < 16)) WrError(ErrNum_InvAddrMode);
          else
          {
            AddSrc(SReg);
            if (DecideUnit(DReg, "LS"))
              switch (ThisUnit)
              {
                case L1: case L2:
                  __erg = CodeL(0x06, DReg, 0, SReg);
                  break;
                case S1: case S2:
                  __erg = CodeS(0x16, DReg, 0, SReg);
                  break;
                default:
                  WrError(ErrNum_CannotUseUnit);
              }
          }
        }
        break;
      case ModLReg:
        AddLDest(DReg);
        if (ChkUnit(DReg, L1, L2))
        {
          if (DecodeAdr(&ArgStr[1], MModLReg, True, &SReg))
          {
            if ((ThisCross) || (IsCross(SReg))) WrError(ErrNum_InvAddrMode);
            else
            {
              AddLSrc(SReg);
              __erg = CodeL(0x24, DReg, 0, SReg);
            }
          }
        }
        break;
    }
  }
}

static void DecodeLogic(Word Index)
{
  LongWord S1Reg, S2Reg, DReg;
  LongWord Code1, Code2;
  Boolean OK, WithImm;

  Code1 = Lo(Index);
  Code2 = Hi(Index);

  if (ChkArgCnt(3, 3))
  {
    if (DecodeAdr(&ArgStr[3], MModReg, True, &DReg))
    {
      AddDest(DReg);
      DecodeAdr(&ArgStr[1], MModImm + MModReg, True, &S1Reg);
      WithImm = False;
      switch (AdrMode)
      {
        case ModImm:
          OK = DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg);
          if (OK) AddSrc(S2Reg);
          WithImm = True;
          break;
        case ModReg:
          AddSrc(S1Reg);
          OK = DecodeAdr(&ArgStr[2], MModImm + MModReg, True, &S2Reg);
          switch (AdrMode)
          {
            case ModImm:
              SwapReg(&S1Reg, &S2Reg);
              WithImm = True;
              break;
            case ModReg:
              AddSrc(S2Reg);
              WithImm = False;
              break;
            default:
              OK = False;
          }
          break;
        default:
          OK = False;
      }
      if ((OK) && (DecideUnit(DReg, "LS")))
      {
        if ((!WithImm) && (IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else if ((ThisCross) && (!IsCross(S2Reg)) && ((WithImm) || (!IsCross(S1Reg)))) WrError(ErrNum_InvAddrMode);
        else
        {
          if ((!WithImm) && (IsCross(S1Reg)))
            SwapReg(&S1Reg, &S2Reg);
          SetCross(S2Reg);
          switch (ThisUnit)
          {
            case L1: case L2:
              __erg = CodeL(Code1 - Ord(WithImm), DReg, S1Reg, S2Reg);
              break;
            case S1: case S2:
              __erg = CodeS(Code2 - Ord(WithImm), DReg, S1Reg, S2Reg);
              break;
            default:
              WrError(ErrNum_CannotUseUnit);
          }
        }
      }
    }
  }
}

static void DecodeNOT(Word Index)
{
  LongWord SReg, DReg;

  UNUSED(Index);

  /* NOT src,dst == XOR -1,src,dst */

  if (ChkArgCnt(2, 2))
  {
    if (DecodeAdr(&ArgStr[2], MModReg, True, &DReg))
    {
      AddDest(DReg);
      if (DecodeAdr(&ArgStr[1], MModReg, True, &SReg))
      {
        AddSrc(SReg);
        if (DecideUnit(DReg, "LS"))
        {
          if ((ThisCross) && (!IsCross(SReg))) WrError(ErrNum_InvAddrMode);
          else
          {
            SetCross(SReg);
            switch (ThisUnit)
            {
              case L1: case L2:
                __erg = CodeL(0x6e, DReg, 0x1f, SReg);
                break;
              case S1: case S2:
                __erg = CodeS(0x0a, DReg, 0x1f, SReg);
                break;
              default:
                WrError(ErrNum_CannotUseUnit);
            }
          }
        }
      }
    }
  }
}

static void DecodeZERO(Word Index)
{
  LongWord DReg;
  UNUSED(Index);

  /* ZERO dst == SUB dst,dst,dst */

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg + MModLReg, True, &DReg);
    if ((ThisCross) || (IsCross(DReg))) WrError(ErrNum_InvAddrMode);
    else
      switch (AdrMode)
      {
        case ModReg:
          AddDest(DReg);
          AddSrc(DReg);
          if (DecideUnit(DReg, "LSD"))
            switch (ThisUnit)
            {
              case L1: case L2:
                __erg = CodeL(0x17, DReg, DReg, DReg);
                break;
              case S1: case S2:
                __erg = CodeS(0x17, DReg, DReg, DReg);
                break;
              case D1: case D2:
                __erg = CodeD(0x11, DReg, DReg, DReg);
                break;
              default:
                WrError(ErrNum_CannotUseUnit);
            }
          break;
        case ModLReg:
          AddLDest(DReg);
          AddLSrc(DReg);
          if (ChkUnit(DReg, L1, L2))
            __erg = CodeL(0x37, DReg, DReg, DReg);
          break;
      }
  }
}

static void DecodeCLR_EXT_EXTU_SET(Word Code)
{
  LongWord DReg, S1Reg, S2Reg, HReg;
  Boolean OK, IsEXT = (Code == 0x2f48);

  if (ChkArgCnt(3, 4))
  {
    if ((DecodeAdr(&ArgStr[ArgCnt], MModReg, IsEXT, &DReg))
     && (ChkUnit(DReg, S1, S2)))
    {
      AddDest(DReg);
      if (DecodeAdr(&ArgStr[1], MModReg, IsEXT, &S2Reg))
      {
        AddSrc(S2Reg);
        if (ArgCnt == 3)
        {
          if (DecodeAdr(&ArgStr[2], MModReg, False, &S1Reg))
          {
            if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
            else if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
            else
            {
              SetCross(S2Reg);
              __erg = CodeS(Hi(Code), DReg, S1Reg, S2Reg);
            }
          }
        }
        else if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else
        {
          S1Reg = EvalStrIntExpression(&ArgStr[2], UInt5, &OK);
          if (OK)
          {
            HReg = EvalStrIntExpression(&ArgStr[3], UInt5, &OK);
            if (OK)
            {
              ThisInst = (DReg << 23) + (S2Reg << 18) + (S1Reg << 13)
                       + (HReg << 8) + (UnitFlag << 1) + Lo(Code);
              __erg = True;
            }
          }
        }
      }
    }
  }
}

static void DecodeCmp(Word Index)
{
  const CmpOrder *pOrder = CmpOrders + Index;
  LongWord DReg, S1Reg, S2Reg;

  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[3], MModReg, False, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    AddDest(DReg);
    DecodeAdr(&ArgStr[1], MModReg + MModImm, pOrder->WithImm, &S1Reg);
    switch (AdrMode)
    {
      case ModReg:
        AddSrc(S1Reg);
        DecodeAdr(&ArgStr[2], MModReg + MModLReg, pOrder->WithImm, &S2Reg);
        switch (AdrMode)
        {
          case ModReg:
            if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
            else if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
            else
            {
              AddSrc(S2Reg);
              if (IsCross(S1Reg)) SwapReg(&S1Reg, &S2Reg);
              SetCross(S2Reg);
              __erg = CodeL(pOrder->Code + 3, DReg, S1Reg, S2Reg);
            }
            break;
          case ModLReg:
            if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
            else if ((ThisCross) && (!IsCross(S1Reg))) WrError(ErrNum_InvAddrMode);
            else
            {
              AddLSrc(S2Reg); SetCross(S1Reg);
              __erg = CodeL(pOrder->Code + 1, DReg, S1Reg, S2Reg);
            }
            break;
        }
        break;
      case ModImm:
        DecodeAdr(&ArgStr[2], MModReg + MModLReg, pOrder->WithImm, &S2Reg);
        switch (AdrMode)
        {
          case ModReg:
            if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
            else
            {
              AddSrc(S2Reg); SetCross(S2Reg);
              __erg = CodeL(pOrder->Code + 2, DReg, S1Reg, S2Reg);
            }
            break;
          case ModLReg:
            if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
            else
            {
              AddLSrc(S2Reg);
              __erg = CodeL(pOrder->Code, DReg, S1Reg, S2Reg);
            }
            break;
        }
       break;
    }
  }
}

static void DecodeLMBD(Word Code)
{
  LongWord DReg, S1Reg, S2Reg;
  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[3], MModReg, False, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    AddDest(DReg);
    if (DecodeAdr(&ArgStr[2], MModReg, False, &S2Reg))
    {
      if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
      else
      {
        SetCross(S2Reg);
        if (DecodeAdr(&ArgStr[1], MModImm + MModReg, False, &S1Reg))
        {
          if (AdrMode == ModReg)
            AddSrc(S1Reg);
          __erg = CodeL(0x6a + Ord(AdrMode == ModImm), DReg, S1Reg, S2Reg);
        }
      }
    }
  }
}

static void DecodeNORM(Word Code)
{
  LongWord DReg, S2Reg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[2], MModReg, False, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    AddDest(DReg);
    DecodeAdr(&ArgStr[1], MModReg + MModLReg, True, &S2Reg);
    switch (AdrMode)
    {
      case ModReg:
        if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else
        {
          SetCross(S2Reg); AddSrc(S2Reg);
          __erg = CodeL(0x63, DReg, 0, S2Reg);
        }
        break;
      case ModLReg:
        if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
        else
        {
          AddLSrc(S2Reg);
          __erg = CodeL(0x60, DReg, 0, S2Reg);
        }
        break;
    }
  }
}

static void DecodeSADD(Word Code)
{
  LongWord DReg, S1Reg, S2Reg;

  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[3], MModReg + MModLReg, True, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    switch (AdrMode)
    {
      case ModReg:
        AddDest(DReg);
        DecodeAdr(&ArgStr[1], MModReg + MModImm, True, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:
            AddSrc(S1Reg);
            DecodeAdr(&ArgStr[2], MModReg + MModImm, True, &S2Reg);
            switch (AdrMode)
            {
              case ModReg:
               if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
               else if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
               else
               {
                 AddSrc(S2Reg);
                 if (IsCross(S1Reg)) SwapReg(&S1Reg, &S2Reg);
                 SetCross(S2Reg);
                 __erg = CodeL(0x13, DReg, S1Reg, S2Reg);
               }
               break;
              case ModImm:
               if ((ThisCross) && (!IsCross(S1Reg))) WrError(ErrNum_InvAddrMode);
               else
               {
                 SetCross(S1Reg);
                 __erg = CodeL(0x12, DReg, S2Reg, S1Reg);
               }
               break;
            }
            break;
          case ModImm:
            if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
            {
              if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
              else
              {
                SetCross(S2Reg);
                __erg = CodeL(0x12, DReg, S1Reg, S2Reg);
              }
            }
            break;
        }
        break;
      case ModLReg:
        AddLDest(DReg);
        DecodeAdr(&ArgStr[1], MModReg + MModLReg + MModImm, True, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:
            AddSrc(S1Reg);
            if (DecodeAdr(&ArgStr[2], MModLReg, True, &S2Reg))
            {
              if ((ThisCross) && (!IsCross(S1Reg))) WrError(ErrNum_InvAddrMode);
              else
              {
                AddLSrc(S2Reg); SetCross(S1Reg);
                __erg = CodeL(0x31, DReg, S1Reg, S2Reg);
              }
            }
            break;
          case ModLReg:
            if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
            else
            {
              AddLSrc(S1Reg);
              DecodeAdr(&ArgStr[2], MModReg + MModImm, True, &S2Reg);
              switch (AdrMode)
              {
                case ModReg:
                  if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    AddSrc(S2Reg); SetCross(S2Reg);
                    __erg = CodeL(0x31, DReg, S2Reg, S1Reg);
                  }
                  break;
                case ModImm:
                  __erg = CodeL(0x30, DReg, S2Reg, S1Reg);
                  break;
              }
            }
            break;
          case ModImm:
            if (DecodeAdr(&ArgStr[2], MModLReg, True, &S2Reg))
            {
              if (IsCross(S2Reg)) WrError(ErrNum_InvAddrMode);
              else
              {
                AddLSrc(S2Reg);
                __erg = CodeL(0x30, DReg, S1Reg, S2Reg);
              }
            }
            break;
        }
        break;
    }
  }
}

static void DecodeSAT(Word Code)
{
  LongWord DReg, S2Reg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[2], MModReg, True, &DReg)
   && ChkUnit(DReg, L1, L2))
  {
    AddDest(DReg);
    if (DecodeAdr(&ArgStr[1], MModLReg, True, &S2Reg))
    {
      if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
      else
      {
        AddLSrc(S2Reg);
        __erg = CodeL(0x40, DReg, 0, S2Reg);
      }
    }
  }
}

static void DecodeMVC(Word Code)
{
  LongWord S1Reg, CReg;

  UNUSED(Code);

  if ((ThisUnit != NoUnit) && (ThisUnit != S2)) WrError(ErrNum_InvAddrMode);
  else if (ChkArgCnt(2, 2))
  {
    int z;

    z = 0;
    ThisUnit = S2;
    UnitFlag = 1;
    if (DecodeCtrlReg(ArgStr[1].str.p_str, &CReg, False))
      z = 2;
    else if (DecodeCtrlReg(ArgStr[2].str.p_str, &CReg, True))
      z = 1;
    else
      WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
    if (z > 0)
    {
      if (DecodeAdr(&ArgStr[z], MModReg, False, &S1Reg))
      {
        if ((ThisCross) && ((z == 2) || (IsCross(S1Reg)))) WrError(ErrNum_InvAddrMode);
        else
        {
          if (z == 1)
          {
            AddSrc(S1Reg);
            SetCross(S1Reg);
            __erg = CodeS(0x0e, CReg, 0, S1Reg);
          }
          else
          {
            AddDest(S1Reg);
            __erg = CodeS(0x0f, S1Reg, 0, CReg);
          }
        }
      }
    }
  }
}

static void DecodeMVK(Word Code)
{
  LongWord DReg, S1Reg;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    if (DecodeAdr(&ArgStr[2], MModReg, True, &DReg))
     if (ChkUnit(DReg, S1, S2))
     {
       if (Memo("MVKLH"))
         S1Reg = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
       else if (Memo("MVKL"))
         S1Reg = EvalStrIntExpression(&ArgStr[1], SInt16, &OK);
       else
         S1Reg = EvalStrIntExpression(&ArgStr[1], Int32, &OK);
       if (OK)
       {
         AddDest(DReg);
         ThisInst = (DReg << 23) + (((S1Reg >> Hi(Code)) & 0xffff) << 7) + (UnitFlag << 1) + Lo(Code);
         __erg = True;
       }
     }
  }
}

static void DecodeSHL(Word Code)
{
  LongWord DReg, S1Reg, S2Reg;

  UNUSED(Code);

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[3], MModReg + MModLReg, True, &DReg);
    if ((AdrMode != ModNone) && (ChkUnit(DReg, S1, S2)))
     switch (AdrMode)
     {
       case ModReg:
         AddDest(DReg);
         if (DecodeAdr(&ArgStr[1], MModReg, True, &S2Reg))
         {
           if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
           else
           {
             AddSrc(S2Reg);
             SetCross(S2Reg);
             DecodeAdr(&ArgStr[2], MModReg + MModImm, False, &S1Reg);
             switch (AdrMode)
             {
               case ModReg:
                 if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                 else
                 {
                   AddSrc(S1Reg);
                   __erg = CodeS(0x33, DReg, S1Reg, S2Reg);
                 }
                 break;
               case ModImm:
                 __erg = CodeS(0x32, DReg, S1Reg, S2Reg);
                 break;
             }
           }
         }
         break;
       case ModLReg:
         AddLDest(DReg);
         DecodeAdr(&ArgStr[1], MModReg + MModLReg, True, &S2Reg);
         switch (AdrMode)
         {
           case ModReg:
             if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
             else
             {
               AddSrc(S2Reg); SetCross(S2Reg);
               DecodeAdr(&ArgStr[2], MModImm + MModReg, False, &S1Reg);
               switch (AdrMode)
               {
                 case ModReg:
                   if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                   else
                   {
                     AddSrc(S1Reg);
                     __erg = CodeS(0x13, DReg, S1Reg, S2Reg);
                   }
                   break;
                 case ModImm:
                   __erg = CodeS(0x12, DReg, S1Reg, S2Reg);
                   break;
               }
             }
             break;
           case ModLReg:
             if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
             else
             {
               AddLSrc(S2Reg);
               DecodeAdr(&ArgStr[2], MModImm + MModReg, False, &S1Reg);
               switch (AdrMode)
               {
                 case ModReg:
                   if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                   else
                   {
                     AddSrc(S1Reg);
                     __erg = CodeS(0x31, DReg, S1Reg, S2Reg);
                   }
                   break;
                 case ModImm:
                   __erg = CodeS(0x30, DReg, S1Reg, S2Reg);
                   break;
               }
             }
             break;
         }
         break;
      }
  }
}

static void DecodeSHR_SHRU(Word Code)
{
  LongWord DReg, S1Reg, S2Reg;
  Boolean HasSign = Code != 0;

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[3], MModReg + MModLReg, HasSign, &DReg);
    if ((AdrMode != ModNone) && (ChkUnit(DReg, S1, S2)))
     switch (AdrMode)
     {
       case ModReg:
         AddDest(DReg);
         if (DecodeAdr(&ArgStr[1], MModReg, HasSign, &S2Reg))
         {
           if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
           else
           {
             AddSrc(S2Reg); SetCross(S2Reg);
             DecodeAdr(&ArgStr[2], MModReg + MModImm, False, &S1Reg);
             switch (AdrMode)
             {
               case ModReg:
                 if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                 else
                 {
                   AddSrc(S1Reg);
                   __erg = CodeS(0x27 + Code, DReg, S1Reg, S2Reg);
                 }
                 break;
               case ModImm:
                 __erg = CodeS(0x26 + Code, DReg, S1Reg, S2Reg);
                 break;
             }
           }
         }
         break;
       case ModLReg:
         AddLDest(DReg);
         if (DecodeAdr(&ArgStr[1], MModLReg, HasSign, &S2Reg))
         {
           if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
           else
           {
             AddLSrc(S2Reg);
             DecodeAdr(&ArgStr[2], MModReg + MModImm, False, &S1Reg);
             switch (AdrMode)
             {
               case ModReg:
                 if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
                 else
                 {
                   AddSrc(S1Reg);
                   __erg = CodeS(0x25 + Code, DReg, S1Reg, S2Reg);
                 }
                 break;
               case ModImm:
                 __erg = CodeS(0x24 + Code, DReg, S1Reg, S2Reg);
                 break;
             }
           }
         }
         break;
     }
  }
}

static void DecodeSSHL(Word Code)
{
  LongWord DReg, S1Reg, S2Reg;

  UNUSED(Code);

  if (ChkArgCnt(3, 3))
  {
    if (DecodeAdr(&ArgStr[3], MModReg, True, &DReg))
     if (ChkUnit(DReg, S1, S2))
     {
       AddDest(DReg);
       if (DecodeAdr(&ArgStr[1], MModReg, True, &S2Reg))
       {
         if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
         else
         {
           AddSrc(S2Reg); SetCross(S2Reg);
           DecodeAdr(&ArgStr[2], MModReg + MModImm, False, &S1Reg);
           switch (AdrMode)
           {
             case ModReg:
               if (IsCross(S1Reg)) WrError(ErrNum_InvAddrMode);
               else
               {
                 AddSrc(S1Reg);
                 __erg = CodeS(0x23, DReg, S1Reg, S2Reg);
               }
               break;
             case ModImm:
               __erg = CodeS(0x22, DReg, S1Reg, S2Reg);
               break;
           }
         }
       }
     }
  }
}

static void DecodeSSUB(Word Code)
{
  LongWord DReg, S1Reg, S2Reg;

  UNUSED(Code);

  if (ChkArgCnt(3, 3))
  {
    DecodeAdr(&ArgStr[3], MModReg + MModLReg, True, &DReg);
    if ((AdrMode != ModNone) && (ChkUnit(DReg, L1, L2)))
     switch (AdrMode)
     {
       case ModReg:
        AddDest(DReg);
        DecodeAdr(&ArgStr[1], MModReg + MModImm, True, &S1Reg);
        switch (AdrMode)
        {
          case ModReg:
            AddSrc(S1Reg);
            if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
            {
              if ((ThisCross) && (!IsCross(S1Reg)) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
              else if ((IsCross(S1Reg)) && (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
              else if (IsCross(S1Reg))
              {
                ThisCross = True;
                __erg = CodeL(0x1f, DReg, S1Reg, S2Reg);
              }
              else
              {
                SetCross(S2Reg);
                __erg = CodeL(0x0f, DReg, S1Reg, S2Reg);
              }
            }
            break;
          case ModImm:
            if (DecodeAdr(&ArgStr[2], MModReg, True, &S2Reg))
            {
              if ((ThisCross) && (!IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
              else
              {
                AddSrc(S2Reg); SetCross(S2Reg);
                __erg = CodeL(0x0e, DReg, S1Reg, S2Reg);
              }
            }
            break;
        }
        break;
       case ModLReg:
        AddLDest(DReg);
         if (DecodeAdr(&ArgStr[1], MModImm, True, &S1Reg))
         {
           if (DecodeAdr(&ArgStr[2], MModLReg, True, &S2Reg))
           {
             if ((ThisCross) || (IsCross(S2Reg))) WrError(ErrNum_InvAddrMode);
             else
             {
               AddLSrc(S2Reg);
               __erg = CodeL(0x2c, DReg, S1Reg, S2Reg);
             }
           }
         }
         break;
     }
  }
}

/* Spruenge */

static void DecodeB(Word Code)
{
  LongWord S2Reg, Code1;
  LongInt Dist;
  Boolean WithImm, OK;

  UNUSED(Code);

  if (ArgCnt != 1) WrError(ErrNum_InvAddrMode);
  else if (ThisCross) WrError(ErrNum_InvAddrMode);
  else if ((ThisUnit != NoUnit) && (ThisUnit != S1) && (ThisUnit != S2)) WrError(ErrNum_InvAddrMode);
  else
  {
    OK = True;
    S2Reg = 0;
    WithImm = False;
    Code1 = 0;
    if (!as_strcasecmp(ArgStr[1].str.p_str, "IRP"))
    {
      Code1 = 0x03;
      S2Reg = 0x06;
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "NRP"))
    {
      Code1 = 0x03;
      S2Reg = 0x07;
    }
    else if (DecodeReg(ArgStr[1].str.p_str, &S2Reg, &OK, False))
    {
      if (OK) WrError(ErrNum_InvAddrMode);
      OK = !OK;
      Code1 = 0x0d;
    }
    else
      WithImm = OK = True;
    if (OK)
    {
      if (WithImm)
      {
        tSymbolFlags Flags;

        if (ThisUnit == NoUnit)
          ThisUnit = (UnitUsed(S1)) ? S2 : S1;
        UnitFlag = Ord(ThisUnit == S2);

        /* branches relative to fetch packet */

        Dist = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags) - (PacketAddr & (~31));
        if (OK)
        {
          if ((Dist & 3) != 0) WrError(ErrNum_NotAligned);
          else if (!mSymbolQuestionable(Flags) && ((Dist > 0x3fffff) || (Dist < -0x400000))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            ThisInst = 0x10 + ((Dist & 0x007ffffc) << 5) + (UnitFlag << 1);
            ThisAbsBranch = True;
            __erg = True;
          }
        }
      }
      else
      {
        if (ChkUnit(0x10, S1, S2))
        {
          SetCross(S2Reg);
          __erg = CodeS(Code1, 0, 0, S2Reg);
        }
      }
    }
  }
}

static Boolean DecodeInst(void)
{
  __erg = False;

  /* ueber Tabelle: */

  if (LookupInstTable(InstTable, OpPart.str.p_str))
    return __erg;

  WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
  return False;
}

static void ChkPacket(void)
{
  LongWord EndAddr, Mask;
  LongInt z, z1, z2;
  Integer RegReads[32];
  char TestUnit[4];
  int BranchCnt;

  /* nicht ueber 8er-Grenze */

  EndAddr = PacketAddr + ((ParCnt << 2) - 1);
  if ((PacketAddr >> 5) != (EndAddr >> 5))
    WrError(ErrNum_PackCrossBoundary);

  /* doppelte Units,Crosspaths,Adressierer,Zielregister */

  for (z1 = 0; z1 < ParCnt; z1++)
    for (z2 = z1 + 1; z2 < ParCnt; z2++)
      if ((ParRecs[z1].OpCode >> 28) == (ParRecs[z2].OpCode >> 28))
      {
        /* doppelte Units */

        if ((ParRecs[z1].U != NoUnit) && (ParRecs[z1].U == ParRecs[z2].U))
          WrXError(ErrNum_UnitMultipleUsed, UnitNames[ParRecs[z1].U]);

        /* Crosspaths */

        z = ParRecs[z1].CrossUsed & ParRecs[z2].CrossUsed;
        if (z != 0)
        {
          *TestUnit = z + '0';
          TestUnit[1] = 'X';
          TestUnit[2] = '\0';
          WrXError(ErrNum_UnitMultipleUsed, TestUnit);
        }

        z = ParRecs[z1].AddrUsed & ParRecs[z2].AddrUsed;

        /* Adressgeneratoren */

        if ((z & 1) == 1) WrXError(ErrNum_UnitMultipleUsed, "Addr. A");
        if ((z & 2) == 2) WrXError(ErrNum_UnitMultipleUsed, "Addr. B");

        /* Hauptspeicherpfade */

        if ((z & 4) == 4) WrXError(ErrNum_UnitMultipleUsed, "LdSt. A");
        if ((z & 8) == 8) WrXError(ErrNum_UnitMultipleUsed, "LdSt. B");

        /* ueberlappende Zielregister */

        z = ParRecs[z1].DestMask & ParRecs[z2].DestMask;
        if (z != 0)
          WrXError(ErrNum_OverlapDests, RegName(FindReg(z)));

        if ((ParRecs[z1].U & 1) == (ParRecs[z2].U & 1))
        {
          TestUnit[0] = ParRecs[z1].U - NoUnit - 1 + 'A';
          TestUnit[1] = '\0';

          /* mehrere Long-Reads */

          if ((ParRecs[z1].LongSrc) && (ParRecs[z2].LongSrc))
            WrXError(ErrNum_MultipleLongRead, TestUnit);

          /* mehrere Long-Writes */

          if ((ParRecs[z1].LongDest) && (ParRecs[z2].LongDest))
            WrXError(ErrNum_MultipleLongWrite, TestUnit);

          /* Long-Read mit Store */

          if ((ParRecs[z1].StoreUsed) && (ParRecs[z2].LongSrc))
            WrXError(ErrNum_LongReadWithStore, TestUnit);
          if ((ParRecs[z2].StoreUsed) && (ParRecs[z1].LongSrc))
            WrXError(ErrNum_LongReadWithStore, TestUnit);
        }
      }

  for (z2 = 0; z2 < 32; z2++)
    RegReads[z2] = 0;
  for (z1 = 0; z1 < ParCnt; z1++)
  {
    Mask = 1;
    for (z2 = 0; z2 < 32; z2++)
    {
      if ((ParRecs[z1].SrcMask & Mask) != 0)
        RegReads[z2]++;
      if ((ParRecs[z1].SrcMask2 & Mask) != 0)
        RegReads[z2]++;
      Mask = Mask << 1;
    }
  }

  /* Register mehr als 4mal gelesen */

  for (z1 = 0; z1 < 32; z1++)
    if (RegReads[z1] > 4)
      WrXError(ErrNum_TooManyRegisterReads, RegName(z1));

  /* more than one branch to an absolute address */

  BranchCnt = 0;
  for (z1 = 0; z1 < ParCnt; z1++)
    if (ParRecs[z1].AbsBranch)
      BranchCnt++;
  if (BranchCnt > 1)
    WrError(ErrNum_TooManyBranchesInExPacket);
}

static void MakeCode_3206X(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if ((*OpPart.str.p_str == '\0') && (*LabPart.str.p_str == '\0'))
    return;

  /* Pseudoanweisungen */

  if (DecodePseudo())
    return;

  /* Flags zuruecksetzen */

  ThisPar = False;
    Condition = 0;

  /* Optionen aus Label holen */

  if (*LabPart.str.p_str != '\0')
    if ((!strcmp(LabPart.str.p_str, "||")) || (*LabPart.str.p_str == '['))
     if (!CheckOpt(LabPart.str.p_str))
       return;

  /* eventuell falsche Mnemonics verwerten */

  if (!strcmp(OpPart.str.p_str, "||"))
    if (!ReiterateOpPart())
      return;
  if (*OpPart.str.p_str == '[')
    if (!ReiterateOpPart())
      return;

  if (Memo(""))
    return;

  /* Attribut auswerten */

  ThisUnit = NoUnit;
  ThisCross = False;
  if (*AttrPart.str.p_str)
  {
    if (as_toupper(AttrPart.str.p_str[strlen(AttrPart.str.p_str) - 1]) == 'X')
    {
      ThisCross = True;
      AttrPart.str.p_str[strlen(AttrPart.str.p_str) - 1] = '\0';
    }
    if (*AttrPart.str.p_str == '\0') ThisUnit = NoUnit;
    else
      for (; ThisUnit != LastUnit; ThisUnit++)
        if (!as_strcasecmp(AttrPart.str.p_str, UnitNames[ThisUnit]))
          break;
    if (ThisUnit == LastUnit)
    {
      WrError(ErrNum_UndefAttr);
      return;
    }
    if (((ThisUnit == D1) || (ThisUnit == D2)) && (ThisCross))
    {
      WrError(ErrNum_InvAddrMode);
      return;
    }
  }

  /* falls nicht parallel, vorherigen Stack durchpruefen und verwerfen */

  if ((!ThisPar) && (ParCnt > 0))
  {
    ChkPacket();
    ParCnt = 0;
    PacketAddr = EProgCounter();
  }

  /* dekodieren */

  ThisSrc = 0;
  ThisSrc2 = 0;
  ThisDest = 0;
  ThisAddr = 0;
  ThisStore = ThisAbsBranch = False;
  ThisLong = 0;
  if (!DecodeInst())
    return;

  /* einsortieren */

  ParRecs[ParCnt].OpCode = (Condition << 28) + ThisInst;
  ParRecs[ParCnt].U = ThisUnit;
  if (ThisCross)
    switch (ThisUnit)
    {
      case L1: case S1: case M1: case D1:
        ParRecs[ParCnt].CrossUsed = 1;
        break;
      default:
        ParRecs[ParCnt].CrossUsed = 2;
    }
  else
    ParRecs[ParCnt].CrossUsed = 0;
  ParRecs[ParCnt].AddrUsed = ThisAddr;
  ParRecs[ParCnt].SrcMask = ThisSrc;
  ParRecs[ParCnt].SrcMask2 = ThisSrc2;
  ParRecs[ParCnt].DestMask = ThisDest;
  ParRecs[ParCnt].LongSrc = ((ThisLong & 1) == 1);
  ParRecs[ParCnt].LongDest = ((ThisLong & 2) == 2);
  ParRecs[ParCnt].StoreUsed = ThisStore;
  ParRecs[ParCnt].AbsBranch = ThisAbsBranch;
  ParCnt++;

  /* wenn mehr als eine Instruktion, Ressourcenkonflikte abklopfen und
    vorherige Instruktion zuruecknehmen */

  if (ParCnt > 1)
  {
    RetractWords(4);
    DAsmCode[CodeLen >> 2] = ParRecs[ParCnt - 2].OpCode | 1;
    CodeLen += 4;
  }

  /* aktuelle Instruktion auswerfen: fuer letzte kein Parallelflag setzen */

  DAsmCode[CodeLen >> 2] = ParRecs[ParCnt - 1].OpCode;
  CodeLen += 4;
}

/*-------------------------------------------------------------------------*/

static void AddLinAdd(const char *NName, LongInt NCode)
{
  order_array_rsv_end(LinAddOrders, FixedOrder);
  LinAddOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeLinAdd);
}

static void AddCmp(const char *NName, LongInt NCode)
{
  order_array_rsv_end(CmpOrders, CmpOrder);
  CmpOrders[InstrZ].WithImm = NName[strlen(NName) - 1] != 'U';
  CmpOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCmp);
}

static void AddMem(const char *NName, LongInt NCode, LongInt NScale)
{
  order_array_rsv_end(MemOrders, MemOrder);
  MemOrders[InstrZ].Code = NCode;
  MemOrders[InstrZ].Scale = NScale;
  AddInstTable(InstTable,NName, InstrZ++, DecodeMemO);
}

static void AddMul(const char *NName, LongInt NCode,
                   Boolean NDSign, Boolean NSSign1, Boolean NSSign2, Boolean NMay)
{
  order_array_rsv_end(MulOrders, MulOrder);
  MulOrders[InstrZ].Code = NCode;
  MulOrders[InstrZ].DSign = NDSign;
  MulOrders[InstrZ].SSign1 = NSSign1;
  MulOrders[InstrZ].SSign2 = NSSign2;
  MulOrders[InstrZ].MayImm = NMay;
  AddInstTable(InstTable, NName, InstrZ++, DecodeMul);
}

static void AddCtrl(const char *NName, LongInt NCode,
                    Boolean NWr, Boolean NRd)
{
  order_array_rsv_end(CtrlRegs, CtrlReg);
  CtrlRegs[InstrZ].Name = NName;
  CtrlRegs[InstrZ].Code = NCode;
  CtrlRegs[InstrZ].Wr = NWr;
  CtrlRegs[InstrZ++].Rd = NRd;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);

  AddInstTable(InstTable, "IDLE", 0, DecodeIDLE);
  AddInstTable(InstTable, "NOP", 0, DecodeNOP);
  AddInstTable(InstTable, "STP", 0, DecodeSTP);
  AddInstTable(InstTable, "ABS", 0, DecodeABS);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "ADDU", 0, DecodeADDU);
  AddInstTable(InstTable, "SUB", 0, DecodeSUB);
  AddInstTable(InstTable, "SUBU", 0, DecodeSUBU);
  AddInstTable(InstTable, "SUBC", 0, DecodeSUBC);
  AddInstTable(InstTable, "ADDK", 0, DecodeADDK);
  AddInstTable(InstTable, "ADD2", 0, DecodeADD2_SUB2);
  AddInstTable(InstTable, "SUB2", 1, DecodeADD2_SUB2);
  AddInstTable(InstTable, "AND", 0x1f7b, DecodeLogic);
  AddInstTable(InstTable, "OR", 0x1b7f, DecodeLogic);
  AddInstTable(InstTable, "XOR", 0x0b6f, DecodeLogic);
  AddInstTable(InstTable, "MV", 0, DecodeMV);
  AddInstTable(InstTable, "NEG", 0, DecodeNEG);
  AddInstTable(InstTable, "NOT", 0, DecodeNOT);
  AddInstTable(InstTable, "ZERO", 0, DecodeZERO);
  AddInstTable(InstTable, "CLR",  0x3fc8, DecodeCLR_EXT_EXTU_SET);
  AddInstTable(InstTable, "EXT",  0x2f48, DecodeCLR_EXT_EXTU_SET);
  AddInstTable(InstTable, "EXTU", 0x2b08, DecodeCLR_EXT_EXTU_SET);
  AddInstTable(InstTable, "SET",  0x3b88, DecodeCLR_EXT_EXTU_SET);
  AddInstTable(InstTable, "LMBD", 0, DecodeLMBD);
  AddInstTable(InstTable, "NORM", 0, DecodeNORM);
  AddInstTable(InstTable, "SADD", 0, DecodeSADD);
  AddInstTable(InstTable, "SAT", 0, DecodeSAT);
  AddInstTable(InstTable, "MVC", 0, DecodeMVC);
  AddInstTable(InstTable, "MVKL", 0x0028, DecodeMVK);
  AddInstTable(InstTable, "MVK", 0x0028, DecodeMVK);
  AddInstTable(InstTable, "MVKH", 0x1068, DecodeMVK);
  AddInstTable(InstTable, "MVKLH", 0x0068, DecodeMVK);
  AddInstTable(InstTable, "SHL", 0, DecodeSHL);
  AddInstTable(InstTable, "SHR", 16, DecodeSHR_SHRU);
  AddInstTable(InstTable, "SHRU", 0, DecodeSHR_SHRU);
  AddInstTable(InstTable, "SSHL", 0, DecodeSSHL);
  AddInstTable(InstTable, "SSUB", 0, DecodeSSUB);
  AddInstTable(InstTable, "B", 0, DecodeB);

  InstrZ = 0;
  AddLinAdd("ADDAB", 0x30); AddLinAdd("ADDAH", 0x34); AddLinAdd("ADDAW", 0x38);
  AddLinAdd("SUBAB", 0x31); AddLinAdd("SUBAH", 0x35); AddLinAdd("SUBAW", 0x39);

  InstrZ = 0;
  AddCmp("CMPEQ", 0x50); AddCmp("CMPGT", 0x44); AddCmp("CMPGTU", 0x4c);
  AddCmp("CMPLT", 0x54); AddCmp("CMPLTU", 0x5c);

  InstrZ = 0;
  AddMem("LDB", 2, 1);  AddMem("LDH", 4, 2);  AddMem("LDW", 6, 4);
  AddMem("LDBU", 1, 1); AddMem("LDHU", 0, 2); AddMem("STB", 3, 1);
  AddMem("STH", 5, 2);  AddMem("STW", 7, 4);

  InstrZ = 0;
  AddMul("MPY"    , 0x19, True , True , True , True );
  AddMul("MPYU"   , 0x1f, False, False, False, False);
  AddMul("MPYUS"  , 0x1d, True , False, True , False);
  AddMul("MPYSU"  , 0x1b, True , True , False, True );
  AddMul("MPYH"   , 0x01, True , True , True , False);
  AddMul("MPYHU"  , 0x07, False, False, False, False);
  AddMul("MPYHUS" , 0x05, True , False, True , False);
  AddMul("MPYHSU" , 0x03, True , True , False, False);
  AddMul("MPYHL"  , 0x09, True , True , True , False);
  AddMul("MPYHLU" , 0x0f, False, False, False, False);
  AddMul("MPYHULS", 0x0d, True , False, True , False);
  AddMul("MPYHSLU", 0x0b, True , True , False, False);
  AddMul("MPYLH"  , 0x11, True , True , True , False);
  AddMul("MPYLHU" , 0x17, False, False, False, False);
  AddMul("MPYLUHS", 0x15, True , False, True , False);
  AddMul("MPYLSHU", 0x13, True , True , False, False);
  AddMul("SMPY"   , 0x1a, True , True , True , False);
  AddMul("SMPYHL" , 0x0a, True , True , True , False);
  AddMul("SMPYLH" , 0x12, True , True , True , False);
  AddMul("SMPYH"  , 0x02, True , True , True , False);

  InstrZ = 0;
  AddCtrl("AMR"    ,  0, True , True );
  AddCtrl("CSR"    ,  1, True , True );
  AddCtrl("IFR"    ,  2, False, True );
  AddCtrl("ISR"    ,  2, True , False);
  AddCtrl("ICR"    ,  3, True , False);
  AddCtrl("IER"    ,  4, True , True );
  AddCtrl("ISTP"   ,  5, True , True );
  AddCtrl("IRP"    ,  6, True , True );
  AddCtrl("NRP"    ,  7, True , True );
  AddCtrl("IN"     ,  8, False, True );
  AddCtrl("OUT"    ,  9, True , True );
  AddCtrl("PCE1"   , 16, False, True );
  AddCtrl("PDATA_O", 15, True , True );
  AddCtrl(NULL     ,  0, False, False);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(LinAddOrders);
  order_array_free(CmpOrders);
  order_array_free(MemOrders);
  order_array_free(MulOrders);
  order_array_free(CtrlRegs);
}

/*------------------------------------------------------------------------*/

static Boolean IsDef_3206X(void)
{
  return (!strcmp(LabPart.str.p_str, "||")) || (*LabPart.str.p_str == '[');
}

static void SwitchFrom_3206X(void)
{
  if (ParCnt > 1)
    ChkPacket();
  DeinitFields();
  if (ParRecs)
  {
    free(ParRecs);
    ParRecs = NULL;
  }
}

static Boolean Chk34Arg(void)
{
  return (ArgCnt >= 3);
}

static void SwitchTo_3206X(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = Chk34Arg;

  PCSymbol = "$";
  HeaderID = 0x47;
  NOPCode = 0x00000000;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 4; SegInits[SegCode] = 0;
  SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

  MakeCode = MakeCode_3206X;
  IsDef = IsDef_3206X;
  SwitchFrom = SwitchFrom_3206X;
  ParRecs = (InstrRec*)malloc(sizeof(InstrRec) * MaxParCnt);
  InitFields();

  onoff_packing_add(True);

  ParCnt = 0;
  PacketAddr = 0;
}

void code3206x_init(void)
{
  CPU32060 = AddCPU("32060", SwitchTo_3206X);
}
