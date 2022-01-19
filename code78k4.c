/* code78k4.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 78K4-Familie                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "code78k4.h"

/*-------------------------------------------------------------------------*/

typedef enum
{
  ModNone = -1,
  ModImm = 0,
  ModReg8,
  ModReg8_U16,
  ModReg16,
  ModReg24,
  ModMem,
  ModShort1,
  ModShort2,
  ModSFR,
  ModAbs16,
  ModAbs20,
  ModAbs24,
  ModShortIndir1_16,
  ModShortIndir2_16,
  ModShortIndir1_24,
  ModShortIndir2_24,
  ModSP,
  ModSTBC,
  ModWDM,
  ModPSW  /* no associated mask, only used for PUSH/POP! */
} tAdrMode;

typedef enum
{
  eBitTypePSW = 2,
  eBitTypeAX = 3,
  eBitTypeSAddr2_SFR = 4,
  eBitTypeSAddr1 = 5,
  eBitTypeAbs = 6,
  eBitTypeMem = 13
} tBitType;

typedef LongWord tAdrModeMask;

static const Word PSWLAddr = 0xfffe,
                  PSWHAddr = 0xffff;
static const ShortInt WHLReg = 3;
static const LongWord Bit27 = ((LongWord)1) << 27;

static const tAdrModeMask MModImm = (1UL << ModImm),
                          MModReg8 = (1UL << ModReg8),
                          MModReg8_U16 = (1UL << ModReg8_U16),
                          MModReg16 = (1UL << ModReg16),
                          MModReg24 = (1UL << ModReg24),
                          MModMem = (1UL << ModMem),
                          MModShort1 = (1UL << ModShort1),
                          MModShort2 = (1UL << ModShort2),
                          MModSFR = (1UL << ModSFR),
                          MModAbs16 = (1UL << ModAbs16),
                          MModAbs20 = (1UL << ModAbs20),
                          MModAbs24 = (1UL << ModAbs24),
                          MModAbsAll = ((1UL << ModAbs16) | (1UL << ModAbs24)),
                          MModShortIndir1_16 = (1UL << ModShortIndir1_16),
                          MModShortIndir2_16 = (1UL << ModShortIndir2_16),
                          MModShortIndir1_24 = (1UL << ModShortIndir1_24),
                          MModShortIndir2_24 = (1UL << ModShortIndir2_24),
                          MModShortIndir_All = ((1UL << ModShortIndir1_16) | (1UL << ModShortIndir2_16) | (1UL << ModShortIndir1_24) | (1UL << ModShortIndir2_24)),
                          MModSP = (1UL << ModSP),
                          MModSTBC = (1UL << ModSTBC),
                          MModWDM = (1UL << ModWDM);

/*-------------------------------------------------------------------------*/

static CPUVar CPU784026;

typedef struct
{
  tAdrMode AdrMode;
  ShortInt AdrVal;
  int AdrCnt;
  Byte AdrVals[4];
  tSymbolFlags AdrValSymFlags;
  Byte ForceRel;
  Boolean ForceAbs;
} tEncodedAddress;

static ShortInt OpSize;
static Boolean AssumeByte;

static LongInt Reg_RSS, Reg_LOCATION;
static ASSUMERec ASSUME78K4s[] =
{
  {"RSS"      , &Reg_RSS      , 0,  0x1,  0x0, NULL},
  {"LOCATION" , &Reg_LOCATION , 0,  0xf,  0x0, NULL},
};

/*-------------------------------------------------------------------------*/
/* Address Decoders */

static Byte AccReg8(void)
{
  return Reg_RSS ? 5 : 1;
}

static Byte BReg8(void)
{
  return Reg_RSS ? 7 : 3;
}

static Byte CReg8(void)
{
  return Reg_RSS ? 6 : 2;
}

static Byte AccReg16(void)
{
  return Reg_RSS ? 2 : 0;
}

static Boolean SetOpSize(ShortInt NewSize)
{
  if (OpSize < 0)
  {
    OpSize = NewSize;
    return True;
  }
  else if (OpSize != NewSize)
  {
    char Sizes[30];

    as_snprintf(Sizes, sizeof(Sizes), "%d<->%d",
                (int)((OpSize + 1) * 8), (int)((NewSize + 1) * 8));
    WrXError(ErrNum_ConfOpSizes, Sizes);
    return False;
  }
  else
    return True;
}

static void ClearEncodedAddress(tEncodedAddress *pAddress)
{
  pAddress->AdrMode = ModNone;
  pAddress->AdrVal = 0;
  pAddress->AdrCnt = 0;
  pAddress->ForceAbs = 0;
  pAddress->ForceRel = False;
}

static ShortInt DecodeReg8(const char *pAsc)
{
  switch (strlen(pAsc))
  {
    case 1:
    {
      static const char Reg8Names[9] = "XACBEDLH";
      const char *pPos = strchr(Reg8Names, as_toupper(*pAsc));

      if (pPos)
      {
        ShortInt Result = pPos - Reg8Names;
        /* E/D/L/H maps to R12..R15 */
        if (Result >= 4)
          return Result + 8;
        /* X/A/C/B maps to R4..7 if RSS=1 */
        else if (Reg_RSS)
          return Result + 4;
        else
          return Result;
      }
      else
        return -1;
    }

    case 2:
      if ((toupper(pAsc[0]) == 'R') && isdigit(pAsc[1]))
        return pAsc[1] - '0';
      else
        return -1;

    case 3:
    {
      static const char Reg8Names[][4] = { "VPL", "VPH", "UPL", "UPH", "R10", "R11", "R12", "R13", "R14", "R15", "" };
      int z;

      for (z = 0; *Reg8Names[z]; z++)
        if (!as_strcasecmp(pAsc, Reg8Names[z]))
        {
          /* map to 8..11 resp. 10..15 */
          return (z > 4) ? z + 6 : z + 8;
        }
      return -1;
    }

    default:
      return -1;
  }
}

static ShortInt DecodeReg8_U16(const char *pAsc)
{
  static const char Reg8Names[5] = "VUTW";
  const char *pPos;

  if (strlen(pAsc) != 1)
    return -1;
  pPos = strchr(Reg8Names, as_toupper(*pAsc));
  return pPos ? pPos - Reg8Names : -1;
}

static ShortInt DecodeReg16(const char *pAsc)
{
  ShortInt Result = -1;

  switch (strlen(pAsc))
  {
    case  2:
    {
      static const char Reg16Names[][3] = { "AX", "BC", "VP", "UP", "DE", "HL", "" };
      int z;

      for (z = 0; *Reg16Names[z]; z++)
        if (!as_strcasecmp(Reg16Names[z], pAsc))
        {
          Result = ((z >= 2) || Reg_RSS) ? z + 2 : z;
          break;
        }
      break;
    }

    case 3:
    {
      if ((toupper(pAsc[0]) == 'R') && (toupper(pAsc[1]) == 'P')
       && (pAsc[2] >= '0') && (pAsc[2] <= '7'))
        Result = pAsc[2] - '0';

      break;
    }
  }

  return Result;
}

static ShortInt DecodeReg24(const char *pAsc)
{
  static const char Reg24Names[][4] = { "VVP", "UUP", "TDE", "WHL", "RG4", "RG5", "RG6", "RG7", "" };
  int z;

  for (z = 0; *Reg24Names[z]; z++)
    if (!as_strcasecmp(Reg24Names[z], pAsc))
      return (z & 3);

  return -1;
}

static Boolean DecodeRB(const char *pAsc, Byte *pErg)
{
  if ((strlen(pAsc) != 3) || (toupper(*pAsc) != 'R') || (toupper(pAsc[1]) != 'B') || (pAsc[2] < '0') || (pAsc[2] > '7'))
    return False;
  else
  {
    *pErg = pAsc[2] - '0';
    return True;
  }
}

static void ExecAssumeByte(void)
{
  if ((OpSize == -1) && AssumeByte)
  {
    SetOpSize(0);
    AssumeByte = False;
  }
}

static Boolean ChkSAddr1(LongWord Addr, Byte *pShortAddr)
{
  LongWord Start = 0xfe00 + (Reg_LOCATION << 16);

  if ((Addr & 0xffff00) == (Start & 0xffff00))
  {
    *pShortAddr = Addr & 0xff;
    return True;
  }
  else
    return False;
}

static Boolean ChkSAddr2(LongWord Addr, Byte *pShortAddr)
{
  LongWord Start1 = 0xff00 + (Reg_LOCATION << 16),
           Start2 = 0xfd00 + (Reg_LOCATION << 16);

  if ((Addr & 0xffffe0) == Start1)
  {
    *pShortAddr = Addr & 0x1f;
    return True;
  }
  else if (((Addr & 0xffff00) == Start2) && (Lo(Addr) >= 0x20))
  {
    *pShortAddr = Addr & 0x1f;
    return True;
  }
  else
    return False;
}

static Boolean ChkSFR(LongWord Addr, Byte *pShortAddr)
{
  LongWord Start = 0xff00 + (Reg_LOCATION << 16);

  if ((Addr & 0xffff00) == (Start & 0xffff00))
  {
    *pShortAddr = Addr & 0xff;
    return True;
  }
  else
    return False;
}

static Boolean StripIndirect(tStrComp *pArg)
{
  int ArgLen = strlen(pArg->str.p_str);

  if ((ArgLen >= 2) && (*pArg->str.p_str == '[') && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    strmov(pArg->str.p_str, pArg->str.p_str + 1);
    pArg->str.p_str[ArgLen - 2] = '\0';
    pArg->Pos.StartCol++;
    pArg->Pos.Len -= 2;
    return True;
  }
  else
    return False;
}

static Boolean DecodeAdr(const tStrComp *pArg, tAdrModeMask AdrModeMask, tEncodedAddress *pAddress)
{
  int z, ArgLen;
  LongWord Addr;
  ShortInt AddrSize;
  Boolean OK, Is16, IsShort1, IsShort2, IsSFR;
  unsigned Offset;

  ClearEncodedAddress(pAddress);

  /* 8 bit Register? */

  if ((pAddress->AdrVal = DecodeReg8(pArg->str.p_str)) >= 0)
  {
    if (!SetOpSize(0))
      return False;
    pAddress->AdrMode = ModReg8;
    goto AdrFound;
  }

  if ((pAddress->AdrVal = DecodeReg8_U16(pArg->str.p_str)) >= 0)
  {
    if (!SetOpSize(0))
      return False;
    pAddress->AdrMode = ModReg8_U16;
    goto AdrFound;
  }

  if ((pAddress->AdrVal = DecodeReg16(pArg->str.p_str)) >= 0)
  {
    if (!SetOpSize(1))
      return False;
    pAddress->AdrMode = ModReg16;
    goto AdrFound;
  }

  if ((pAddress->AdrVal = DecodeReg24(pArg->str.p_str)) >= 0)
  {
    if (!SetOpSize(2))
      return False;
    pAddress->AdrMode = ModReg24;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "SP"))
  {
    if (!SetOpSize(2))
      return False;
    pAddress->AdrMode = ModSP;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "STBC"))
  {
    if (!SetOpSize(0))
      return False;
    pAddress->AdrMode = ModSTBC;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "WDM"))
  {
    if (!SetOpSize(0))
      return False;
    pAddress->AdrMode = ModWDM;
    goto AdrFound;
  }

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    ExecAssumeByte();
    if ((OpSize >= 0) && (OpSize < 3))
    {
      static const IntType IntTypes[3] = { Int8, Int16, Int24 };
      Boolean OK;
      LongWord Value = EvalStrIntExpressionOffs(pArg, 1, IntTypes[OpSize], &OK);
      if (OK)
      {
        ShortInt z;

        for (z = 0; z <= OpSize; z++)
        {
          pAddress->AdrVals[pAddress->AdrCnt++] = Value & 0xff;
          Value >>= 8;
        }
        pAddress->AdrMode = ModImm;
      }
    }
    else
      WrError(ErrNum_UndefOpSizes);
    goto AdrFound;
  }

  /* memory-indirect addressing? */

  ArgLen = strlen(pArg->str.p_str);
  if ((ArgLen >= 2) && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    String Asc;
    tStrComp Arg;
    char *pStart;

    StrCompMkTemp(&Arg, Asc, sizeof(Asc));
    StrCompCopy(&Arg, pArg);

    /* remove ']' */

    StrCompShorten(&Arg, 1);

    pStart = RQuotPos(Arg.str.p_str, '[');
    if (!pStart)
    {
      WrError(ErrNum_BrackErr);
      goto AdrFound;
    }

    /* purely indirect? */

    if (pStart == Arg.str.p_str)
    {
      static const char Modes[][5] = { "TDE+", "WHL+", "TDE-", "WHL-", "TDE", "WHL", "VVP", "UUP",
                                       "RG6+", "RG7+", "RG6-", "RG7-", "RG6", "RG7", "RG4", "RG5" };
      unsigned z;
      char *pSep, Save;
      tStrComp Base, Remainder;

      /* skip '[' */

      StrCompCutLeft(&Arg, 1);

      /* simple expression without displacement? */

      for (z = 0; z < sizeof(Modes) / sizeof(*Modes); z++)
        if (!as_strcasecmp(Arg.str.p_str, Modes[z]))
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x16;
          pAddress->AdrVals[0] = z % (sizeof(Modes) / sizeof(*Modes) / 2);
          pAddress->AdrCnt = 1;
          goto AdrFound;
        }

      /* no -> extract base register. Its name ends with the first non-letter,
         which either means +/- or a blank */

      for (pSep = Arg.str.p_str; *pSep; pSep++)
        if (!as_isalpha(*pSep))
          break;

      /* decode base register.  SP is not otherwise handled. */

      Save = StrCompSplitRef(&Base, &Remainder, &Arg, pSep);
      if (!as_strcasecmp(Base.str.p_str, "SP"))
        pAddress->AdrVals[0] = 1;
      else
      {
        int tmp;

        tmp = DecodeReg24(Base.str.p_str);
        switch (tmp)
        {
          case -1: pAddress->AdrVals[0] = 0xff; break; /* no register */
          case 0: pAddress->AdrVals[0] = 4; break; /* VVP */
          case 1: pAddress->AdrVals[0] = 3; break; /* UUP */
          case 2: pAddress->AdrVals[0] = 0; break; /* TDE */
          case 3: pAddress->AdrVals[0] = 2; break; /* WHL */
          default:
            WrStrErrorPos(ErrNum_InvReg, &Base);
            goto AdrFound;
        }
      }
      *pSep = Save;

      /* no base register detected: purely indirect */

      if (0xff == pAddress->AdrVals[0])
      {
        unsigned Is24 = !!(*Arg.str.p_str == '%');
        tSymbolFlags Flags;

        Addr = EvalStrIntExpressionOffsWithFlags(&Arg, Is24, UInt24, &OK, &Flags);
        if (OK)
        {
          if (mFirstPassUnknown(Flags))
            Addr = 0xfe20 + (Reg_LOCATION << 16);
          if (ChkSAddr1(Addr, &pAddress->AdrVals[0]))
            pAddress->AdrMode = Is24 ? ModShortIndir1_24 : ModShortIndir1_16;
          if (ChkSAddr2(Addr, &pAddress->AdrVals[0]))
            pAddress->AdrMode = Is24 ? ModShortIndir2_24 : ModShortIndir2_16;
        }
        goto AdrFound;
      }

      /* Now that we have the base, prepare displacement.  May
         be an 8/16-bit register in certain combinations, or a number: */

      if (*pSep == '+')
      {
        int tmp;

        tmp = DecodeReg8(pSep + 1);
        if (tmp == -1); /* no reg at all, go on with 16-bit reg below */
        else if ((tmp == AccReg8()) && (pAddress->AdrVals[0] == 0)) /* TDE + A */
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x17;
          pAddress->AdrCnt = 1;
          pAddress->AdrVals[0] = 0;
          goto AdrFound;
        }
        else if ((tmp == BReg8()) && (pAddress->AdrVals[0] == 0)) /* TDE + B */
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x17;
          pAddress->AdrCnt = 1;
          pAddress->AdrVals[0] = 2;
          goto AdrFound;
        }
        else if ((tmp == CReg8()) && (pAddress->AdrVals[0] == 0)) /* TDE + C */
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x17;
          pAddress->AdrCnt = 1;
          pAddress->AdrVals[0] = 6;
          goto AdrFound;
        }
        else if ((tmp == AccReg8()) && (pAddress->AdrVals[0] == 2)) /* WHL + A */
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x17;
          pAddress->AdrCnt = 1;
          pAddress->AdrVals[0] = 1;
          goto AdrFound;
        }
        else if ((tmp == BReg8()) && (pAddress->AdrVals[0] == 2)) /* WHL + B */
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x17;
          pAddress->AdrCnt = 1;
          pAddress->AdrVals[0] = 3;
          goto AdrFound;
        }
        else if ((tmp == CReg8()) && (pAddress->AdrVals[0] == 2)) /* WHL + C */
        {
          pAddress->AdrMode = ModMem;
          pAddress->AdrVal = 0x17;
          pAddress->AdrCnt = 1;
          pAddress->AdrVals[0] = 7;
          goto AdrFound;
        }
        else
        {
          WrError(ErrNum_InvAddrMode);
          goto AdrFound;
        }
        tmp = DecodeReg16(pSep + 1);
        switch (tmp)
        {
          case -1: /* no reg at all */
            break;
          case 6: /* DE */
          case 7: /* HL */
            if (pAddress->AdrVals[0] == 4) /* VVP+DE/HL */
            {
              pAddress->AdrMode = ModMem;
              pAddress->AdrVal = 0x17;
              pAddress->AdrCnt = 1;
              pAddress->AdrVals[0] = tmp - 2;
              goto AdrFound;
            }
            /* fall-through */
          default:
            WrError(ErrNum_InvAddrMode);
            goto AdrFound;
        }
      }

      /* it's a number: put a fake 0 in front so displacement expression evaluates correctly! */

      if (pSep > Arg.str.p_str)
        pSep--;
      *pSep = '0';
      pAddress->AdrVals[1] = EvalStrIntExpressionOffs(&Arg, pSep - Arg.str.p_str, Int8, &OK);
      if (OK)
      {
        pAddress->AdrMode = ModMem;
        pAddress->AdrVal = 0x06;
        pAddress->AdrCnt = 2;
        goto AdrFound;
      }
    }

    /* no -> with outer displacement */

    else
    {
      tStrComp RegArg, DispArg;
      int tmp;

      /* split displacement + register */

      StrCompSplitRef(&DispArg, &RegArg, &Arg, pStart);

       /* handle base register */

      tmp = DecodeReg8(RegArg.str.p_str);
      if ((tmp == AccReg8()) /* A */
       || (tmp == BReg8())) /* B */
      {
        pAddress->AdrVals[0] = tmp & 3;
      }
      else if (tmp == -1)
      {
        tmp = DecodeReg16(RegArg.str.p_str);
        if (tmp >= 6) /* DE/HL */
        {
          pAddress->AdrVals[0] = (tmp - 6) << 1;
        }
        else
        {
          WrStrErrorPos(ErrNum_InvReg, &RegArg);
          goto AdrFound;
        }
      }
      else
      {
        WrStrErrorPos(ErrNum_InvReg, &RegArg);
        goto AdrFound;
      }

      /* compute displacement */

      Addr = EvalStrIntExpression(&DispArg, Int24, &OK);
      if (OK)
      {
        pAddress->AdrMode = ModMem;
        pAddress->AdrVal = 0x0a;
        pAddress->AdrVals[1] = (Addr >>  0) & 0xff;
        pAddress->AdrVals[2] = (Addr >>  8) & 0xff;
        pAddress->AdrVals[3] = (Addr >> 16) & 0xff;
        pAddress->AdrCnt = 4;
        goto AdrFound;
      }
    }
  } /* indirect */

  /* absolute: */

  Offset = 0;
  if (pArg->str.p_str[Offset] == '$')
  {
    pAddress->ForceRel = True;
    Offset++;
  }
  for (AddrSize = 0, z = 0; z < 2; z++)
    if (pArg->str.p_str[Offset] == '!')
    {
      AddrSize++;
      Offset++;
      pAddress->ForceAbs++;
    }
  Addr = EvalStrIntExpressionOffsWithFlags(pArg, Offset, (AdrModeMask & MModAbs20) ? UInt20 : UInt24, &OK, &pAddress->AdrValSymFlags);
  if (!OK)
    return False;

  IsShort1 = ChkSAddr1(Addr, &pAddress->AdrVals[0]);
  IsShort2 = ChkSAddr2(Addr, &pAddress->AdrVals[1]);
  IsSFR = ChkSFR(Addr, &pAddress->AdrVals[2]);
  Is16 = Addr <= 0xffff;

  if (!AddrSize)
  {
    if (!IsShort1 && !IsShort2 && !IsSFR && !Is16 && (AdrModeMask & (MModAbs24 | MModAbs20)))
      AddrSize = 2;
    else if (!IsShort1 && !IsShort2 && !IsSFR && Is16 && (!(AdrModeMask & MModAbs16)))
      AddrSize = 2;
    else if (!IsShort1 && !IsShort2 && !IsSFR && (AdrModeMask & MModAbs16))
      AddrSize = 1;
  }

  switch (AddrSize)
  {
    case 2:
      pAddress->AdrMode = (AdrModeMask & MModAbs20) ? ModAbs20 : ModAbs24;
      pAddress->AdrVals[pAddress->AdrCnt++] = (Addr >>  0) & 0xff;
      pAddress->AdrVals[pAddress->AdrCnt++] = (Addr >>  8) & 0xff;
      pAddress->AdrVals[pAddress->AdrCnt++] = (Addr >> 16) & 0xff;
      break;
    case 1:
      if (!Is16 && mFirstPassUnknown(pAddress->AdrValSymFlags))
      {
        Addr &= 0xffff;
        Is16 = True;
      }
      if (Is16)
      {
        pAddress->AdrMode = ModAbs16;
        pAddress->AdrVals[pAddress->AdrCnt++] = (Addr >>  0) & 0xff;
        pAddress->AdrVals[pAddress->AdrCnt++] = (Addr >>  8) & 0xff;
      }
      else
        WrError(ErrNum_OverRange);
      break;
    case 0:
      if (IsShort1)
      {
        pAddress->AdrMode = ModShort1;
        pAddress->AdrCnt++;
      }
      else if (IsShort2)
      {
        pAddress->AdrMode = ModShort2;
        pAddress->AdrVals[pAddress->AdrCnt++] = pAddress->AdrVals[1];
      }
      else if (IsSFR)
      {
        pAddress->AdrMode = ModSFR;
        pAddress->AdrVals[pAddress->AdrCnt++] = pAddress->AdrVals[2];
      }
      else if (mFirstPassUnknown(pAddress->AdrValSymFlags))
      {
        if (AdrModeMask & MModShort1)
        {
          pAddress->AdrMode = ModShort1;
          pAddress->AdrVals[pAddress->AdrCnt++] = Lo(Addr);
        }
        else if (AdrModeMask & MModShort2)
        {
          pAddress->AdrMode = ModShort2;
          pAddress->AdrVals[pAddress->AdrCnt++] = Lo(Addr);
        }
        else if (AdrModeMask & MModSFR)
        {
          pAddress->AdrMode = ModSFR;
          pAddress->AdrVals[pAddress->AdrCnt++] = Lo(Addr);
        }
        else
          WrError(ErrNum_InvAddrMode);
      }
      else
        WrError(ErrNum_OverRange);
      break;
  }

AdrFound:
  if ((pAddress->AdrMode != ModNone) && (!(AdrModeMask & (1UL << pAddress->AdrMode))))
  {
    WrError(ErrNum_InvAddrMode);
    ClearEncodedAddress(pAddress);
    return False;
  }
  return True;
}

static Boolean DecodeMem3(const char *pAsc, Byte *pResult)
{
  int l = strlen(pAsc);
  char Reg[10];
  ShortInt BinReg;

  if ((l < 3) || (l > 5) || (pAsc[0] != '[') || (pAsc[l - 1] != ']'))
    return False;
  memcpy(Reg, pAsc + 1, l - 2);
  Reg[l - 2] = '\0';
  BinReg = DecodeReg16(Reg);
  if ((BinReg >= 0) && (BinReg <= 3))
  {
    *pResult = (BinReg << 1);
    return True;
  }
  BinReg = DecodeReg24(Reg);
  if ((BinReg >= 0) && (BinReg <= 3))
  {
    *pResult = (BinReg << 1) | 1;
    return True;
  }
  return False;
}

static void AppendAdrVals(const tEncodedAddress *pAddress)
{
  /* weird byte order for !!abs24: */

  if (pAddress->AdrMode == ModAbs24)
  {
    BAsmCode[CodeLen++] = pAddress->AdrVals[2];
    BAsmCode[CodeLen++] = pAddress->AdrVals[0];
    BAsmCode[CodeLen++] = pAddress->AdrVals[1];
  }
  else
  {
    memcpy(BAsmCode + CodeLen, pAddress->AdrVals, pAddress->AdrCnt);
    CodeLen += pAddress->AdrCnt;
  }
}

static void AppendAdrValsMem(const tEncodedAddress *pAddress, Byte LowNibble)
{
  AppendAdrVals(pAddress);
  BAsmCode[CodeLen - pAddress->AdrCnt] = (BAsmCode[CodeLen - pAddress->AdrCnt] << 4) | LowNibble;
}

static Byte ModShortVal(tAdrMode AdrMode)
{
  switch (AdrMode)
  {
    case ModShort2:
      return 0;
    case ModShort1:
      return 1;
    case ModSFR:
      return 2;
    default:
      return 255;
  }
}

static LongWord GetAbsVal(const tEncodedAddress *pAddress)
{
  LongWord Result = 0;

  switch (pAddress->AdrMode)
  {
    case ModAbs24:
    case ModAbs20:
      Result |= (((LongWord)pAddress->AdrVals[2]) << 16);
      /* fall-through */
    case ModAbs16:
      Result |= (((LongWord)pAddress->AdrVals[1]) <<  8);
      Result |= (((LongWord)pAddress->AdrVals[0]) <<  0);
      return Result;
    default:
      return 0xfffffffful;
  }
}

static Byte ModIs24(tAdrMode AdrMode)
{
  switch (AdrMode)
  {
    case ModAbs24:
    case ModShortIndir1_24:
    case ModShortIndir2_24:
      return 1;
    case ModAbs16:
    case ModShortIndir1_16:
    case ModShortIndir2_16:
      return 0;
    default:
      return 0xff;
  }
}

static Byte ModIsShort1(tAdrMode AdrMode)
{
  switch (AdrMode)
  {
    case ModShort1:
    case ModShortIndir1_24:
    case ModShortIndir1_16:
      return 1;
    case ModShort2:
    case ModShortIndir2_24:
    case ModShortIndir2_16:
      return 0;
    default:
      return 0xff;
  }
}

static Boolean DecodeBitAdr(const tStrComp *pArg, LongWord *pResult)
{
  char *pSplit;
  Boolean OK;

  pSplit = RQuotPos(pArg->str.p_str, '.');

  if (pSplit)
  {
    tEncodedAddress Addr;
    tStrComp AddrArg, BitArg;

    StrCompSplitRef(&AddrArg, &BitArg, pArg, pSplit);
    *pResult = EvalStrIntExpression(&BitArg, UInt3, &OK) << 24;
    if (OK)
    {
      DecodeAdr(&AddrArg, MModReg8 | MModSFR | MModShort1 | MModShort2 | MModMem | MModAbsAll, &Addr);
      switch (Addr.AdrMode)
      {
        case ModReg8:
          if ((Addr.AdrVal != AccReg8()) && (Addr.AdrVal != AccReg8() - 1))
          {
            WrStrErrorPos(ErrNum_InvReg, &AddrArg);
            OK = FALSE;
          }
          else
          {
            *pResult |= (((LongWord)eBitTypeAX) << 28);
            if (Addr.AdrVal & 1)
              *pResult |= Bit27;
          }
          break;
        case ModSFR:
          if (Addr.AdrVals[0] == (PSWLAddr & 0xff))
            *pResult |= ((LongWord)eBitTypePSW) << 28;
          else if (Addr.AdrVals[0] == (PSWHAddr & 0xff))
            *pResult |= (((LongWord)eBitTypePSW) << 28) | Bit27;
          else
            *pResult |= Addr.AdrVals[0]
                     | (((LongWord)eBitTypeSAddr2_SFR) << 28)
                     | Bit27;
          break;
        case ModShort1:
          *pResult |= Addr.AdrVals[0]
                   | (((LongWord)eBitTypeSAddr1) << 28);
          break;
        case ModShort2:
          *pResult |= Addr.AdrVals[0]
                   | (((LongWord)eBitTypeSAddr2_SFR) << 28);
          break;
        case ModMem:
          if ((Addr.AdrVal != 0x16) || ((Addr.AdrVals[0] & 0x0e) != 4))
          {
            OK = False;
            WrError(ErrNum_InvAddrMode);
          }
          else
          {
            *pResult |= (((LongWord)eBitTypeMem) << 28);
            if (Addr.AdrVals[0] & 1)
              *pResult |= Bit27;
          }
          break;
        case ModAbs16:
        case ModAbs24:
          *pResult |= ((LongWord)eBitTypeAbs) << 28;
          *pResult |= ((LongWord)Addr.AdrVals[0]) << 0;
          *pResult |= ((LongWord)Addr.AdrVals[1]) << 8;
          if (Addr.AdrMode == ModAbs24)
          {
            *pResult |= Bit27;
            *pResult |= ((LongWord)Addr.AdrVals[2]) << 16;
          }
          break;
        default:
          OK = FALSE;
      }
    }
  }
  else
    *pResult = EvalStrIntExpression(pArg, UInt32, &OK);

  return OK;
}

static void AppendRel8(const tStrComp *pArg)
{
  Boolean OK;
  LongInt Dist;
  tSymbolFlags Flags;

  Dist = EvalStrIntExpressionOffsWithFlags(pArg, !!(*pArg->str.p_str == '$'), UInt20, &OK, &Flags) - (EProgCounter() + CodeLen + 1);
  if (!OK) CodeLen = 0;
  else if (!mSymbolQuestionable(Flags) && ((Dist < -0x80) || (Dist > 0x7f)))
  {
    WrError(ErrNum_JmpDistTooBig);
    CodeLen = 0;
  }
  else
    BAsmCode[CodeLen++] = Dist & 0xff;
}

/*-------------------------------------------------------------------------*/
/* Instruction Decoders */

static void PutCode(Word Code)
{
  if (Hi(Code))
    BAsmCode[CodeLen++] = Hi(Code);
  BAsmCode[CodeLen++] = Lo(Code);
}

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    PutCode(Code);
}

static void DecodeBitOpCore2(LongWord BitAddr, Word Code)
{
  switch ((BitAddr >> 28) & 15)
  {
    case eBitTypePSW:
    case eBitTypeAX:
      BAsmCode[CodeLen++] = (BitAddr >> 28) & 15;
      BAsmCode[CodeLen++] = Code | ((BitAddr >> 24) & 15);
      break;
    case eBitTypeSAddr1:
      BAsmCode[CodeLen++] = 0x3c;
      /* fall-through */
    case eBitTypeSAddr2_SFR:
      BAsmCode[CodeLen++] = 0x08;
      BAsmCode[CodeLen++] = Code | ((BitAddr >> 24) & 15);
      BAsmCode[CodeLen++] = Lo(BitAddr);
      break;
    case eBitTypeMem:
      BAsmCode[CodeLen++] = 0x3d;
      BAsmCode[CodeLen++] = Code | ((BitAddr >> 24) & 15);
      break;
    case eBitTypeAbs:
      BAsmCode[CodeLen++] = 0x09;
      BAsmCode[CodeLen++] = 0xd0;
      BAsmCode[CodeLen++] = Code | ((BitAddr >> 24) & 15);
      if (BAsmCode[CodeLen - 1] & 0x08)
        BAsmCode[CodeLen++] = (BitAddr >> 16) & 255;
      BAsmCode[CodeLen++] = (BitAddr >> 0) & 255;
      BAsmCode[CodeLen++] = (BitAddr >> 8) & 255;
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeBitOpCore(const tStrComp *pBitArg, Word Code)
{
  LongWord BitAddr;

  if (!DecodeBitAdr(pBitArg, &BitAddr))
    return;
  DecodeBitOpCore2(BitAddr, Code);
}

static void DecodeMOV1(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2))
    return;
  if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
    DecodeBitOpCore(&ArgStr[2], 0x00);
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "CY"))
    DecodeBitOpCore(&ArgStr[1], 0x10);
  else
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }
}

static void DecodeAND1_OR1(Word Code)
{
  if (!ChkArgCnt(2, 2))
    return;
  if (as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }
  if (*ArgStr[2].str.p_str == '/')
  {
    tStrComp BitArg;

    StrCompRefRight(&BitArg, &ArgStr[2], 1);
    DecodeBitOpCore(&BitArg, Code | 0x10);
  }
  else
    DecodeBitOpCore(&ArgStr[2], Code);
}

static void DecodeXOR1(Word Code)
{
  if (!ChkArgCnt(2, 2))
    return;
  if (as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }
  DecodeBitOpCore(&ArgStr[2], Code);
}

static void DecodeNOT1(Word Code)
{
  if (!ChkArgCnt(1, 1))
    return;
  if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
    BAsmCode[CodeLen++] = 0x42;
  else
    DecodeBitOpCore(&ArgStr[1], Code);
}

static void DecodeSET1_CLR1(Word Code)
{
  if (!ChkArgCnt(1, 1))
    return;
  if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
    BAsmCode[CodeLen++] = 0x41 - ((Code >> 4) & 1);
  else
  {
    LongWord BitAddr;

    if (!DecodeBitAdr(&ArgStr[1], &BitAddr))
      return;
    switch ((BitAddr >> 28) & 15)
    {
      case eBitTypeSAddr1:
        BAsmCode[CodeLen++] = 0x3c;
        /* fall-through */
      case eBitTypeSAddr2_SFR:
        if (!(BitAddr & Bit27))
        {
          BAsmCode[CodeLen++] = (0x130 - Code) | ((BitAddr >> 24) & 7);
          BAsmCode[CodeLen++] = Lo(BitAddr);
          break;
        }
        /* else fall-through */ /* SFR is regularly coded for SET1/CLR1 */
      default:
        DecodeBitOpCore2(BitAddr, Code);
    }
  }
}

static void DecodeMOV(Word ForceOpSize)
{
  tEncodedAddress Dest, Src;
  tAdrModeMask AdrModeMask;

  if (ForceOpSize)
    SetOpSize(ForceOpSize);

  if (!ChkArgCnt(2, 2))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    DecodeBitOpCore(&ArgStr[2], 0x00);
    return;
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "CY"))
  {
    DecodeBitOpCore(&ArgStr[1], 0x10);
    return;
  }

  AdrModeMask = MModShort1 | MModShort2 | MModAbsAll | MModShortIndir1_24 | MModShortIndir2_24 | MModMem;
  if (OpSize != 2)
    AdrModeMask |= MModSFR | MModShortIndir1_16 | MModShortIndir2_16;
  if ((OpSize == -1) || (OpSize == 0))
    AdrModeMask |= MModReg8 | MModReg8_U16 | MModSTBC | MModWDM;
  if ((OpSize == -1) || (OpSize == 1))
    AdrModeMask |= MModReg16;
  if ((OpSize == -1) || (OpSize == 2))
    AdrModeMask |= MModReg24 | MModSP;
  DecodeAdr(&ArgStr[1], AdrModeMask, &Dest);
  switch (Dest.AdrMode)
  {
    case ModReg8:
      AdrModeMask = MModImm | MModReg8 | MModShort1 | MModShort2 | MModSFR | MModAbsAll;
      if (Dest.AdrVal == AccReg8())
        AdrModeMask |= MModReg8_U16 | MModShortIndir_All | MModMem;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal >= 8)
          {
            BAsmCode[CodeLen++] = 0x3c;
            Src.AdrVal -= 8;
          }
          if (Dest.AdrVal == AccReg8())
            BAsmCode[CodeLen++] = 0xd0 | Src.AdrVal;
          else
          {
            BAsmCode[CodeLen++] = 0x24;
            BAsmCode[CodeLen++] = (Dest.AdrVal << 4) | Src.AdrVal;
          }
          break;
        case ModReg8_U16:
          BAsmCode[CodeLen++] = 0x05;
          BAsmCode[CodeLen++] = 0xc1 | (Src.AdrVal << 1);
          break;
        case ModImm:
          if (Dest.AdrVal >= 8)
          {
            BAsmCode[CodeLen++] = 0x3c;
            Dest.AdrVal -= 8;
          }
          BAsmCode[CodeLen++] = 0xb8 | Dest.AdrVal;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModShort2:
          if (Dest.AdrVal == AccReg8())
          {
            BAsmCode[CodeLen++] = 0x20;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          else
            goto CommonReg8ModShort;
        case ModSFR:
          if (Dest.AdrVal == AccReg8())
          {
            BAsmCode[CodeLen++] = 0x10;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          else
            goto CommonReg8ModShort;
        CommonReg8ModShort:
        case ModShort1:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x00 | ModShortVal(Src.AdrMode) | (Dest.AdrVal << 4);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModAbs16:
        case ModAbs24:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x00 | (ModIs24(Src.AdrMode) << 1) | (Dest.AdrVal << 4);
          AppendAdrVals(&Src);
          break;
        case ModShortIndir1_16:
        case ModShortIndir2_16:
        case ModShortIndir1_24:
        case ModShortIndir2_24:
          if (ModIsShort1(Src.AdrMode))
            BAsmCode[CodeLen++] = 0x3c;
          if (ModIs24(Src.AdrMode))
          {
            BAsmCode[CodeLen++] = 0x07;
            BAsmCode[CodeLen++] = 0x30;
          }
          else
            BAsmCode[CodeLen++] = 0x18;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModMem:
          if ((Src.AdrVal == 0x16) && (Src.AdrVals[0] < 6))
            BAsmCode[CodeLen++] = 0x58 + Src.AdrVals[0];
          else
          {
            BAsmCode[CodeLen++] = Src.AdrVal;
            AppendAdrValsMem(&Src, 0x00);
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModReg8_U16:
      DecodeAdr(&ArgStr[2], MModImm | MModReg8, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal == AccReg8())
          {
            BAsmCode[CodeLen++] = 0x05;
            BAsmCode[CodeLen++] = 0xc9 | (Dest.AdrVal << 1);
          }
          break;
        case ModImm:
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0x61 | (Dest.AdrVal << 1);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModReg16:
      AdrModeMask = MModImm | MModReg16 | MModShort1 | MModShort2 | MModSFR | MModAbsAll;
      if (Dest.AdrVal == AccReg16())
        AdrModeMask |= MModShortIndir_All | MModMem;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x60 | Dest.AdrVal;
          AppendAdrVals(&Src);
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x24;
          BAsmCode[CodeLen++] = 0x08 | (Src.AdrVal << 5) | Dest.AdrVal;
          AppendAdrVals(&Src);
          break;
        case ModShort2:
          if (Dest.AdrVal == AccReg16())
          {
            BAsmCode[CodeLen++] = 0x1c;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          else
            goto CommonReg16ModShort;
        case ModSFR:
          if (Dest.AdrVal == AccReg16())
          {
            BAsmCode[CodeLen++] = 0x11;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
            goto CommonReg16ModShort;
        CommonReg16ModShort:
        case ModShort1:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x08 | ModShortVal(Src.AdrMode) | (Dest.AdrVal << 5);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModAbs16:
        case ModAbs24:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x08 | (ModIs24(Src.AdrMode) << 1) | (Dest.AdrVal << 5);
          AppendAdrVals(&Src);
          break;
        case ModShortIndir1_16:
          BAsmCode[CodeLen++] = 0x3c;
          /* fall-through */
        case ModShortIndir2_16:
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0x21;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModShortIndir1_24:
          BAsmCode[CodeLen++] = 0x3c;
          /* fall-through */
        case ModShortIndir2_24:
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0x31;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModMem:
          BAsmCode[CodeLen++] = Src.AdrVal;
          AppendAdrValsMem(&Src, 0x01);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModReg24:
      AdrModeMask = MModImm | MModReg24 | MModAbs24 | MModShort1 | MModShort2;
      if (Dest.AdrVal == WHLReg)
        AdrModeMask |= MModShortIndir1_24 | MModShortIndir2_24 | MModMem | MModSP;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x9b | (Dest.AdrVal << 5);
          AppendAdrVals(&Src);
          break;
        case ModReg24:
          BAsmCode[CodeLen++] = 0x24;
          BAsmCode[CodeLen++] = 0x99 | (Dest.AdrVal << 5) | (Src.AdrVal << 1);
          AppendAdrVals(&Src);
          break;
        case ModAbs24:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x9a | (Dest.AdrVal << 5);
          AppendAdrVals(&Src);
          break;
        case ModShort1:
        case ModShort2:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x98 | ModShortVal(Src.AdrMode) | (Dest.AdrVal << 5);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModShortIndir1_24:
          BAsmCode[CodeLen++] = 0x3c;
          /* fall-through */
        case ModShortIndir2_24:
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0x32;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModMem:
          if ((Src.AdrVal == 0x16) && ((Src.AdrVals[0] == 1) || (Src.AdrVals[0] == 3))) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = Src.AdrVal;
            AppendAdrValsMem(&Src, 0x02);
          }
          break;
        case ModSP:
          BAsmCode[CodeLen++] = 0x05;
          BAsmCode[CodeLen++] = 0xfa;
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModSP:
      DecodeAdr(&ArgStr[2], MModImm | MModReg24, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x09;
          BAsmCode[CodeLen++] = 0x20;
          AppendAdrVals(&Src);
          break;
        case ModReg24:
          if (Src.AdrVal != WHLReg) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = 0x05;
            BAsmCode[CodeLen++] = 0xfb;
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModSTBC:
    case ModWDM:
      if (DecodeAdr(&ArgStr[2], MModImm, &Src))
      {
        BAsmCode[CodeLen++] = 0x09;
        BAsmCode[CodeLen++] = (Dest.AdrMode == ModSTBC) ? 0xc0: 0xc2;
        BAsmCode[CodeLen++] = ~Src.AdrVals[0];
        BAsmCode[CodeLen++] = Src.AdrVals[0];
      }
      break;

    case ModShort1:
      AssumeByte = True;
      AdrModeMask = 0;
      if (OpSize != 2) AdrModeMask |= MModImm | MModShort2 | MModShort1;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      if ((OpSize == -1) || (OpSize == 2)) AdrModeMask |= MModReg24;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x3c;
          BAsmCode[CodeLen++] = OpSize ? 0x0c : 0x3a;
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x05 | (Src.AdrVal << 4);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x0d | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg24:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x9d | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort1:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          BAsmCode[CodeLen++] = 0x30 | (OpSize << 7);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort2:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          BAsmCode[CodeLen++] = 0x20 | (OpSize << 7);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShort2:
      AssumeByte = True;
      AdrModeMask = 0;
      if (OpSize != 2) AdrModeMask |= MModImm | MModShort2 | MModShort1;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      if ((OpSize == -1) || (OpSize == 2)) AdrModeMask |= MModReg24;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = OpSize ? 0x0c : 0x3a;
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          if (Src.AdrVal == AccReg8())
            BAsmCode[CodeLen++] = 0x22;
          else
          {
            BAsmCode[CodeLen++] = 0x38;
            BAsmCode[CodeLen++] = 0x04 | (Src.AdrVal << 4);
          }
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          if (Src.AdrVal == AccReg16())
            BAsmCode[CodeLen++] = 0x1a;
          else
          {
            BAsmCode[CodeLen++] = 0x38;
            BAsmCode[CodeLen++] = 0x0c | (Src.AdrVal << 5);
          }
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg24:
          BAsmCode[CodeLen++] = 0x38;
          BAsmCode[CodeLen++] = 0x9c | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort2:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          BAsmCode[CodeLen++] = OpSize << 7;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort1:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          BAsmCode[CodeLen++] = 0x10 | (OpSize << 7);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModSFR:
      AssumeByte = True;
      AdrModeMask = 0;
      if (OpSize != 2) AdrModeMask |= MModImm;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = OpSize ? 0x0b : 0x2b;
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          if (Src.AdrVal == AccReg8())
            BAsmCode[CodeLen++] = 0x12;
          else
          {
            BAsmCode[CodeLen++] = 0x38;
            BAsmCode[CodeLen++] = 0x06 | (Src.AdrVal << 4);
          }
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          if (Src.AdrVal == AccReg8())
            BAsmCode[CodeLen++] = 0x13;
          else
          {
            BAsmCode[CodeLen++] = 0x38;
            BAsmCode[CodeLen++] = 0x0e | (Src.AdrVal << 5);
          }
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModAbs16:
    case ModAbs24:
      AssumeByte = True;
      AdrModeMask = 0;
      if (OpSize != 2) AdrModeMask |= MModImm;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      if ((OpSize == -1) || (OpSize == 2)) AdrModeMask |= MModReg24;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x09;
          BAsmCode[CodeLen++] = 0x40 | (ModIs24(Dest.AdrMode) << 4) | OpSize;
          AppendAdrVals(&Dest);
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x01 | (ModIs24(Dest.AdrMode) << 1) | (Src.AdrVal << 4);
          AppendAdrVals(&Dest);
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x09 | (ModIs24(Dest.AdrMode) << 1) | (Src.AdrVal << 5);
          AppendAdrVals(&Dest);
          break;
        case ModReg24:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x9b | (Src.AdrVal << 5);
          if (Dest.AdrMode == ModAbs16)
            BAsmCode[CodeLen++] = 0x00;
          AppendAdrVals(&Dest);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShortIndir2_16:
    case ModShortIndir1_16:
      AssumeByte = True;
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
          {
            if (ModIsShort1(Dest.AdrMode))
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0x19;
            BAsmCode[CodeLen++] = Dest.AdrVals[0];
          }
          break;
        case ModReg16:
          if (Src.AdrVal != AccReg16()) WrError(ErrNum_InvAddrMode);
          else
          {
            if (ModIsShort1(Dest.AdrMode))
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0x07;
            BAsmCode[CodeLen++] = 0xa1;
            BAsmCode[CodeLen++] = Dest.AdrVals[0];
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShortIndir2_24:
    case ModShortIndir1_24:
      AssumeByte = True;
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      if ((OpSize == -1) || (OpSize == 2)) AdrModeMask |= MModReg24;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
            goto CommonRegIndir;
          break;
        case ModReg16:
          if (Src.AdrVal != AccReg16()) WrError(ErrNum_InvAddrMode);
          else
            goto CommonRegIndir;
          break;
        case ModReg24:
          if (Src.AdrVal != WHLReg) WrError(ErrNum_InvAddrMode);
          else
            goto CommonRegIndir;
          break;
        CommonRegIndir:
          if (ModIsShort1(Dest.AdrMode))
            BAsmCode[CodeLen++] = 0x3c;
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0xb0 + OpSize;
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModMem:
      AssumeByte = True;
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      if ((OpSize == -1) || (OpSize == 2)) AdrModeMask |= MModReg24;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else if ((Dest.AdrVal == 0x16) && (Dest.AdrVals[0] < 6))
            BAsmCode[CodeLen++] = 0x50 + Dest.AdrVals[0];
          else
            goto CommonRegMem;
          break;
        case ModReg16:
          if (Src.AdrVal != AccReg16()) WrError(ErrNum_InvAddrMode);
          else
            goto CommonRegMem;
          break;
        case ModReg24:
          if (Src.AdrVal != WHLReg) WrError(ErrNum_InvAddrMode);
          else
            goto CommonRegMem;
          break;
        CommonRegMem:
          BAsmCode[CodeLen++] = Dest.AdrVal;
          AppendAdrValsMem(&Dest, 0x80 + OpSize);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;
    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeXCH(Word ForceOpSize)
{
  tEncodedAddress Dest, Src;
  tAdrModeMask AdrModeMask;

  if (ForceOpSize)
    SetOpSize(ForceOpSize);

  if (!ChkArgCnt(2, 2))
    return;

  AdrModeMask = MModShort1 | MModShort2 | MModSFR | MModAbsAll | MModShortIndir_All | MModMem;
  if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
  if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
  DecodeAdr(&ArgStr[1], AdrModeMask, &Dest);
  switch (Dest.AdrMode)
  {
    case ModReg8:
      AdrModeMask = MModReg8 | MModShort2 | MModShort1 | MModSFR | MModAbsAll;
      if (Dest.AdrVal == AccReg8())
        AdrModeMask |= MModShortIndir_All | MModMem;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Dest.AdrVal == AccReg8())
          {
            if (Src.AdrVal >= 8)
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0xd8 | (Src.AdrVal & 7);
          }
          else if (Src.AdrVal == AccReg8())
          {
            if (Dest.AdrVal >= 8)
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0xd8 | (Dest.AdrVal & 7);
          }
          else
          {
            if (Src.AdrVal >= 8)
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0x25;
            BAsmCode[CodeLen++] = (Dest.AdrVal << 4) | (Src.AdrVal & 7);
          }
          break;
        case ModShort2:
          if (Dest.AdrVal == AccReg8())
          {
            BAsmCode[CodeLen++] = 0x21;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          /* fall-through */
        case ModShort1:
        case ModSFR:
          BAsmCode[CodeLen++] = 0x39;
          BAsmCode[CodeLen++] = 0x00 | ModShortVal(Src.AdrMode) | (Dest.AdrVal << 4);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModAbs16:
        case ModAbs24:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x04 | (ModIs24(Src.AdrMode) << 1) | (Dest.AdrVal << 4);
          AppendAdrVals(&Src);
          break;
        case ModShortIndir1_16:
        case ModShortIndir2_16:
        case ModShortIndir1_24:
        case ModShortIndir2_24:
          if (ModIsShort1(Src.AdrMode))
            BAsmCode[CodeLen++] = 0x3c;
          if (ModIs24(Src.AdrMode))
          {
            BAsmCode[CodeLen++] = 0x07;
            BAsmCode[CodeLen++] = 0x34;
          }
          else
            BAsmCode[CodeLen++] = 0x23;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModMem:
          BAsmCode[CodeLen++] = Src.AdrVal;
          AppendAdrValsMem(&Src, 0x04);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModReg16:
      AdrModeMask = MModReg16 | MModShort1 | MModShort2 | MModSFR;
      if (Dest.AdrVal == AccReg16())
        AdrModeMask |= MModShortIndir_All | MModAbsAll | MModMem;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg16:
          BAsmCode[CodeLen++] = 0x25;
          BAsmCode[CodeLen++] = 0x08 | (Src.AdrVal << 5) | Dest.AdrVal;
          break;
        case ModShort2:
          if (Dest.AdrVal == AccReg16())
          {
            BAsmCode[CodeLen++] = 0x1b;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          /* fall-through */
        case ModShort1:
        case ModSFR:
          BAsmCode[CodeLen++] = 0x39;
          BAsmCode[CodeLen++] = 0x08 | ModShortVal(Src.AdrMode) | (Dest.AdrVal << 5);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModShortIndir1_16:
        case ModShortIndir2_16:
        case ModShortIndir1_24:
        case ModShortIndir2_24:
          if (ModIsShort1(Src.AdrMode))
            BAsmCode[CodeLen++] = 0x3c;
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0x25 | (ModIs24(Src.AdrMode) << 4);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModAbs16:
        case ModAbs24:
          BAsmCode[CodeLen++] = 0x0a;
          BAsmCode[CodeLen++] = 0x45 | (ModIs24(Src.AdrMode) << 4);
          AppendAdrVals(&Src);
          break;
        case ModMem:
          BAsmCode[CodeLen++] = Src.AdrVal;
          AppendAdrValsMem(&Src, 0x05);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShort2:
      AssumeByte = True;
      AdrModeMask = MModShort2 | MModShort1;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal == AccReg8())
            BAsmCode[CodeLen++] = 0x21;
          else
          {
            BAsmCode[CodeLen++] = 0x39;
            BAsmCode[CodeLen++] = 0x00 | (Src.AdrVal << 4);
          }
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          if (Src.AdrVal == AccReg16())
            BAsmCode[CodeLen++] = 0x1b;
          else
          {
            BAsmCode[CodeLen++] = 0x39;
            BAsmCode[CodeLen++] = 0x08 | (Src.AdrVal << 5);
          }
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort2:
        case ModShort1:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          BAsmCode[CodeLen++] = (OpSize << 7) | ((Src.AdrMode == ModShort1) ? 0x14 : 0x04);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShort1:
      AssumeByte = True;
      AdrModeMask = MModShort2 | MModShort1;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          BAsmCode[CodeLen++] = 0x39;
          BAsmCode[CodeLen++] = 0x01 | (Src.AdrVal << 4);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x39;
          BAsmCode[CodeLen++] = 0x09 | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort2:
        case ModShort1:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          BAsmCode[CodeLen++] = (OpSize << 7) | ((Src.AdrMode == ModShort1) ? 0x34 : 0x24);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModSFR:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          BAsmCode[CodeLen++] = 0x39;
          BAsmCode[CodeLen++] = 0x02 | (Src.AdrVal << 4);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x39;
          BAsmCode[CodeLen++] = 0x0a | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModAbs16:
    case ModAbs24:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          BAsmCode[CodeLen++] = 0x3e;
          BAsmCode[CodeLen++] = 0x04 | (ModIs24(Dest.AdrMode) << 1) | (Src.AdrVal << 4);
          AppendAdrVals(&Dest);
          break;
        case ModReg16:
          if (Src.AdrVal != AccReg16()) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = 0x0a;
            BAsmCode[CodeLen++] = 0x45 | (ModIs24(Dest.AdrMode) << 4);
            AppendAdrVals(&Dest);
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShortIndir2_16:
    case ModShortIndir1_16:
    case ModShortIndir2_24:
    case ModShortIndir1_24:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
          {
            if (ModIsShort1(Dest.AdrMode))
              BAsmCode[CodeLen++] = 0x3c;
            if (ModIs24(Dest.AdrMode))
            {
              BAsmCode[CodeLen++] = 0x07;
              BAsmCode[CodeLen++] = 0x34;
            }
            else
              BAsmCode[CodeLen++] = 0x23;
            BAsmCode[CodeLen++] = Dest.AdrVals[0];
          }
          break;
        case ModReg16:
          if (Src.AdrVal != AccReg16()) WrError(ErrNum_InvAddrMode);
          else
          {
            if (ModIsShort1(Dest.AdrMode))
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0x07;
            BAsmCode[CodeLen++] = ModIs24(Dest.AdrMode) ? 0x35 : 0x25;
            BAsmCode[CodeLen++] = Dest.AdrVals[0];
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModMem:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = Dest.AdrVal;
            AppendAdrValsMem(&Dest, 0x04);
          }
          break;
        case ModReg16:
          if (Src.AdrVal != AccReg16()) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = Dest.AdrVal;
            AppendAdrValsMem(&Dest, 0x05);
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeALU(Word Code)
{
  tEncodedAddress Dest, Src;
  tAdrModeMask AdrModeMask;
  Byte Code16, Code16_AX, Code24;

  if (Code & 15)
    SetOpSize(Code & 15);
  Code16 = (Code >> 8) & 15;
  Code24 = (Code >> 12) & 15;
  Code16_AX = (Code16 == 0 ? 0x0d : (Code16 == 2 ? 0x0e : 0x0f));
  Code = (Code >> 4) & 15;

  if (!ChkArgCnt(2, 2))
    return;

  if ((OpSize == -1) && (!as_strcasecmp(ArgStr[1].str.p_str, "CY")))
  {
    switch (Code)
    {
      case 4: /* AND CY,... -> AND1 CY,...*/
        DecodeAND1_OR1(0x20);
        return;
      case 6: /* OR CY,...  -> OR1 CY,...*/
        DecodeAND1_OR1(0x40);
        return;
      case 5: /* XOR CY,... -> XOR1 CY,...*/
        DecodeXOR1(0x60);
        return;
    }
  }

  AdrModeMask = 0;
  if ((OpSize == -1) || (OpSize == 0) || (OpSize == 1))
    AdrModeMask = MModShort1 | MModShort2 | MModSFR;
  if ((OpSize == -1) || (OpSize == 0))
    AdrModeMask |= MModReg8 | MModShortIndir_All | MModAbsAll | MModMem;
  if ((Code16 != 15) && ((OpSize == -1) || (OpSize == 1)))
    AdrModeMask |= MModReg16;
  if ((Code24 != 15) && ((OpSize == -1) || (OpSize == 2)))
    AdrModeMask |= MModReg24 | MModSP;
  DecodeAdr(&ArgStr[1], AdrModeMask, &Dest);
  switch (Dest.AdrMode)
  {
    case ModReg8:
      AdrModeMask = MModImm | MModReg8 | MModShort1 | MModShort2 | MModSFR | MModAbsAll;
      if (Dest.AdrVal == AccReg8())
        AdrModeMask |= MModShortIndir_All | MModMem;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          if (Dest.AdrVal == AccReg8())
            BAsmCode[CodeLen++] = 0xa8 | Code;
          else
          {
            BAsmCode[CodeLen++] = 0x78 | Code;
            BAsmCode[CodeLen++] = 0x03 | (Dest.AdrVal << 4);
          }
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModReg8:
          if (Src.AdrVal >= 8)
            BAsmCode[CodeLen++] = 0x3c;
          BAsmCode[CodeLen++] = 0x88 | Code;
          BAsmCode[CodeLen++] = (Dest.AdrVal << 4) | (Src.AdrVal & 7);
          break;
        case ModShort2:
          if (Dest.AdrVal == AccReg8())
          {
            BAsmCode[CodeLen++] = 0x98 | Code;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          /* else fall-through */
        case ModShort1:
        case ModSFR:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = (Dest.AdrVal << 4) | ModShortVal(Src.AdrMode);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModShortIndir1_16:
        case ModShortIndir2_16:
        case ModShortIndir1_24:
        case ModShortIndir2_24:
          if (ModIsShort1(Src.AdrMode))
            BAsmCode[CodeLen++] = 0x3c;
          BAsmCode[CodeLen++] = 0x07;
          BAsmCode[CodeLen++] = 0x28 | (ModIs24(Src.AdrMode) << 4) | Code;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModAbs16:
        case ModAbs24:
          BAsmCode[CodeLen++] = 0x0a;
          BAsmCode[CodeLen++] = 0x48 | (ModIs24(Src.AdrMode) << 4) | Code;
          AppendAdrVals(&Src);
          break;
        case ModMem:
          BAsmCode[CodeLen++] = Src.AdrVal;
          AppendAdrValsMem(&Src, 0x08 | Code);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModReg16:
      AdrModeMask = MModImm | MModReg16 | MModShort1 | MModShort2 | MModSFR;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          if (Dest.AdrVal == AccReg16())
            BAsmCode[CodeLen++] = 0x20 | Code16_AX;
          else
          {
            BAsmCode[CodeLen++] = 0x78 | Code16;
            BAsmCode[CodeLen++] = 0x0d | (Dest.AdrVal << 5);
          }
          AppendAdrVals(&Src);
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x88 | Code16;
          BAsmCode[CodeLen++] = (Src.AdrVal << 5) | 0x08 | Dest.AdrVal;
          break;
        case ModShort2:
          if (Dest.AdrVal == AccReg16())
          {
            BAsmCode[CodeLen++] = 0x10 | Code16_AX;
            BAsmCode[CodeLen++] = Src.AdrVals[0];
            break;
          }
          /* else fall-through */
        case ModShort1:
        case ModSFR:
          BAsmCode[CodeLen++] = 0x78 | Code16;
          BAsmCode[CodeLen++] = 0x08 | ModShortVal(Src.AdrMode) | (Dest.AdrVal << 5);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModReg24:
      AdrModeMask = MModImm | MModReg24;
      if (Dest.AdrVal == WHLReg)
        AdrModeMask |= MModShort1 | MModShort2;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg24:
          BAsmCode[CodeLen++] = 0x88 | Code24;
          BAsmCode[CodeLen++] = 0x99 | (Dest.AdrVal << 5) | (Src.AdrVal << 1);
          break;
        case ModImm:
          BAsmCode[CodeLen++] = 0x78 | Code24;
          BAsmCode[CodeLen++] = 0x9b | (Dest.AdrVal << 5);
          AppendAdrVals(&Src);
          break;
        case ModShort1:
        case ModShort2:
          BAsmCode[CodeLen++] = 0x78 | Code24;
          BAsmCode[CodeLen++] = 0xf8 | ModShortVal(Src.AdrMode);
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModSP:
      OpSize = 1; /* !!! ADDWG/SUBWG, i.e. 24-bit dest & 16-bit src */
      DecodeAdr(&ArgStr[2], MModImm, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x09;
          BAsmCode[CodeLen++] = 0x28 | Code24;
          AppendAdrVals(&Src);
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShort2:
      AssumeByte = True;
      AdrModeMask = MModImm | MModShort1 | MModShort2;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = OpSize ? Code16_AX : (0x68 | Code);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = 0x04 | (Src.AdrVal << 4);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = 0x0c | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort1:
        case ModShort2:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          if (OpSize)
            BAsmCode[CodeLen++] = 0x80 | (ModShortVal(Src.AdrMode) << 4) | Code16_AX;
          else
            BAsmCode[CodeLen++] = 0x08 | (ModShortVal(Src.AdrMode) << 4) | Code;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShort1:
      AssumeByte = True;
      AdrModeMask = MModImm | MModShort1 | MModShort2;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x3c;
          BAsmCode[CodeLen++] = OpSize ? Code16_AX : (0x68 | Code);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = 0x05 | (Src.AdrVal << 4);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = 0x0d | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModShort1:
        case ModShort2:
          ExecAssumeByte();
          BAsmCode[CodeLen++] = 0x2a;
          if (OpSize)
            BAsmCode[CodeLen++] = 0xa0 | (ModShortVal(Src.AdrMode) << 4) | Code16_AX;
          else
            BAsmCode[CodeLen++] = 0x28 | (ModShortVal(Src.AdrMode) << 4) | Code;
          BAsmCode[CodeLen++] = Src.AdrVals[0];
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModSFR:
      AssumeByte = True;
      AdrModeMask = MModImm;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModImm:
          BAsmCode[CodeLen++] = 0x01;
          BAsmCode[CodeLen++] = OpSize ? Code16_AX : (0x68 | Code);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          AppendAdrVals(&Src);
          break;
        case ModReg8:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = 0x06 | (Src.AdrVal << 4);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModReg16:
          BAsmCode[CodeLen++] = 0x78 | Code;
          BAsmCode[CodeLen++] = 0x0e | (Src.AdrVal << 5);
          BAsmCode[CodeLen++] = Dest.AdrVals[0];
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModShortIndir2_16:
    case ModShortIndir1_16:
    case ModShortIndir2_24:
    case ModShortIndir1_24:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
          {
            if (ModIsShort1(Dest.AdrMode))
              BAsmCode[CodeLen++] = 0x3c;
            BAsmCode[CodeLen++] = 0x07;
            BAsmCode[CodeLen++] = 0xa8 | (ModIs24(Dest.AdrMode) << 4) | Code;
            BAsmCode[CodeLen++] = Dest.AdrVals[0];
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModAbs16:
    case ModAbs24:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = 0x0a;
            BAsmCode[CodeLen++] = 0xc8 | (ModIs24(Dest.AdrMode) << 4) | Code;
            AppendAdrVals(&Dest);
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;

    case ModMem:
      AdrModeMask = 0;
      if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
      DecodeAdr(&ArgStr[2], AdrModeMask, &Src);
      switch (Src.AdrMode)
      {
        case ModReg8:
          if (Src.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = Dest.AdrVal;
            AppendAdrValsMem(&Dest, 0x88 | Code);
          }
          break;
        case ModNone:
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
      break;
    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeADDWG_SUBWG(Word Code)
{
  tEncodedAddress Addr;

  if (ChkArgCnt(2, 2))
  {
    OpSize = 2;
    if (DecodeAdr(&ArgStr[1], MModSP, &Addr))
    {
      OpSize = 1; /* !!! ADDWG/SUBWG, i.e. 24-bit dest & 16-bit src */
      if (DecodeAdr(&ArgStr[2], MModImm, &Addr))
      {
        BAsmCode[CodeLen++] = 0x09;
        BAsmCode[CodeLen++] = 0x28 | Code;
        AppendAdrVals(&Addr);
      }
    }
  }
}

static void DecodeMULU(Word Code)
{
  tEncodedAddress Src;
  tAdrModeMask AdrModeMask;

  if (Code & 15)
    SetOpSize(Code & 15);

  if (!ChkArgCnt(1, 1))
    return;

  AdrModeMask = 0;
  if ((OpSize == -1) || (OpSize == 0))
    AdrModeMask |= MModReg8;
  if ((OpSize == -1) || (OpSize == 1))
    AdrModeMask |= MModReg16;
  DecodeAdr(&ArgStr[1], AdrModeMask, &Src);
  switch (Src.AdrMode)
  {
    case ModReg8:
      if (Src.AdrVal >= 8)
        BAsmCode[CodeLen++] = 0x3c;
      BAsmCode[CodeLen++] = 0x05;
      BAsmCode[CodeLen++] = 0x08 | (Src.AdrVal & 7);
      break;
    case ModReg16:
      BAsmCode[CodeLen++] = 0x05;
      BAsmCode[CodeLen++] = 0x28 | Src.AdrVal;
      break;
    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeDIVUW(Word Code)
{
  tEncodedAddress Src;
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModReg8, &Src))
  {
    if (Src.AdrVal >= 8)
      BAsmCode[CodeLen++] = 0x3c;
    BAsmCode[CodeLen++] = 0x05;
    BAsmCode[CodeLen++] = 0x18 | (Src.AdrVal & 7);
  }
}

static void DecodeMULW_DIVUX(Word Code)
{
  tEncodedAddress Src;

  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModReg16, &Src))
  {
    BAsmCode[CodeLen++] = 0x05;
    BAsmCode[CodeLen++] = Code | Src.AdrVal;
  }
}

static void DecodeMACW_MACSW(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    BAsmCode[2] = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
    if (OK)
    {
      BAsmCode[CodeLen++] = 0x07;
      BAsmCode[CodeLen++] = Code;
      CodeLen++;
    }
  }
}

static void DecodeSACW(Word Code)
{
  tEncodedAddress Addr;
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModMem, &Addr))
  {
    if ((Addr.AdrVal != 0x16) || (Addr.AdrVals[0] != 0x00)) WrError(ErrNum_InvAddrMode);
    else if (DecodeAdr(&ArgStr[2], MModMem, &Addr))
    {
      if ((Addr.AdrVal != 0x16) || (Addr.AdrVals[0] != 0x01)) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[CodeLen++] = 0x09;
        BAsmCode[CodeLen++] = 0x64;
        BAsmCode[CodeLen++] = 0x41;
        BAsmCode[CodeLen++] = 0x46;
      }
    }
  }
}

static void DecodeINCDEC(Word Code)
{
  tEncodedAddress Addr;
  tAdrModeMask AdrModeMask;

  if (Code & 15)
    SetOpSize(Code & 15);
  Code >>= 4;

  if (!ChkArgCnt(1, 1))
    return;

  AdrModeMask = 0;
  if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8 | MModShort1 | MModShort2;
  if ((OpSize == -1) || (OpSize == 1)) AdrModeMask |= MModReg16 | MModShort1 | MModShort2;
  if ((OpSize == -1) || (OpSize == 2)) AdrModeMask |= MModReg24 | MModSP;
  AssumeByte = True;
  DecodeAdr(&ArgStr[1], AdrModeMask, &Addr);
  switch (Addr.AdrMode)
  {
    case ModReg8:
      if (Addr.AdrVal >= 8)
        BAsmCode[CodeLen++] = 0x3c;
      BAsmCode[CodeLen++] = 0xc0 | (Code << 3) | (Addr.AdrVal & 7);
      break;
    case ModReg16:
      if (Addr.AdrVal >= 4)
        BAsmCode[CodeLen++] = 0x40 | (Code << 3) | Addr.AdrVal;
      else
      {
        BAsmCode[CodeLen++] = 0x3e;
        BAsmCode[CodeLen++] = 0x0d | (Code << 1) | (Addr.AdrVal << 5);
      }
      break;
    case ModReg24:
      BAsmCode[CodeLen++] = 0x3e;
      BAsmCode[CodeLen++] = 0x9d | (Code << 1) | (Addr.AdrVal << 5);
      break;
    case ModSP:
      BAsmCode[CodeLen++] = 0x05;
      BAsmCode[CodeLen++] = 0xf8 | Code;
      break;
    case ModShort1:
      BAsmCode[CodeLen++] = 0x3c;
      /* fall-through */
    case ModShort2:
      ExecAssumeByte();
      if (OpSize)
      {
        BAsmCode[CodeLen++] = 0x07;
        BAsmCode[CodeLen++] = 0xe8 | Code;
      }
      else
        BAsmCode[CodeLen++] = 0x26 | Code;
      BAsmCode[CodeLen++] = Addr.AdrVals[0];
      break;
    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeShift(Word Code)
{
  tEncodedAddress Addr;
  Boolean OK;
  Byte Shift;
  tAdrModeMask AdrModeMask;

  if (!ChkArgCnt(2, 2))
    return;
  Shift = EvalStrIntExpression(&ArgStr[2], UInt3, &OK);
  if (!OK)
    return;

  if (Code & 15)
    SetOpSize(Code & 15);
  AdrModeMask = 0;
  if ((OpSize == -1) || (OpSize == 0)) AdrModeMask |= MModReg8;
  if ((Code & 0x8000) && ((OpSize == -1) || (OpSize == 1))) AdrModeMask |= MModReg16;
  DecodeAdr(&ArgStr[1], AdrModeMask, &Addr);
  switch (Addr.AdrMode)
  {
    case ModReg8:
      if (Addr.AdrVal >= 8)
        BAsmCode[CodeLen++] = 0x3c;
      BAsmCode[CodeLen++] = Hi(Code) & 0x7f;
      BAsmCode[CodeLen++] = (Lo(Code) & 0xc0) | (Shift << 3) | (Addr.AdrVal & 7);
      break;
    case ModReg16:
      BAsmCode[CodeLen++] = Hi(Code) & 0x7f;
      BAsmCode[CodeLen++] = 0x40 | (Lo(Code) & 0xc0) | (Shift << 3) | (Addr.AdrVal & 7);
      break;
    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeROR4_ROL4(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (!DecodeMem3(ArgStr[1].str.p_str, BAsmCode + 1)) WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[CodeLen++] = 0x05;
    BAsmCode[CodeLen] |= Code; CodeLen++;
  }
}

static void DecodeBIT(Word Code)
{
  LongWord Result;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (DecodeBitAdr(&ArgStr[1], &Result))
  {
    TempResult t;

    as_tempres_ini(&t);
    as_tempres_set_int(&t, Result);
    SetListLineVal(&t);
    EnterIntSymbol(&LabPart, Result, SegNone, False);
    as_tempres_free(&t);
  }
}

static void DecodePUSH_POP(Word Code)
{
  tEncodedAddress Address, SumAddress;
  int z;
  Boolean IsU, IsPUSH;

  if (Code & 15)
    SetOpSize(Code & 15);

  Code = (Code >> 4) & 0xff;
  IsU = (Code == 0x37) || (Code == 0x36),
  IsPUSH = (Code == 0x35);

  if (!ChkArgCnt(1, ArgCntMax))
    return;

  ClearEncodedAddress(&SumAddress);
  for (z = 1; z <= ArgCnt; z++)
  {
    ClearEncodedAddress(&Address);
    if (!as_strcasecmp(ArgStr[z].str.p_str, "PSW"))
    {
      /* PSW replaces UP in bitmask for PUSHU/POPU */
      if (IsU)
      {
        Address.AdrMode = ModReg16;
        Address.AdrVal = 5;
      }
      /* PSW only allowed a single (first) arg on for PUSH/POP */
      else if (z == 1)
        Address.AdrMode = ModPSW;
      else
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[z]);
        return;
      }
      if (!SetOpSize(1))
        return;
    }
    else
    {
      tAdrModeMask AdrModeMask = MModReg16;

      /* sfr/sfrp/rg only allowed as single (first) argument, and only for PUSH/POP */

      if ((z == 1) && (!IsU))
        AdrModeMask |= MModSFR | MModReg24;
      if (!DecodeAdr(&ArgStr[z], AdrModeMask, &Address))
        return;

      /* UP not allowed for PUSHU(POPU */

      if (IsU && (Address.AdrVal == 5))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[z]);
        return;
      }
    }

    /* merge this argument with rest so far: */

    if (SumAddress.AdrMode == ModNone)
    {
      SumAddress = Address;
      if (Address.AdrMode == ModReg16)
        SumAddress.AdrVals[0] = 1 << Address.AdrVal;
    }
    else if ((Address.AdrMode != SumAddress.AdrMode) || (SumAddress.AdrMode != ModReg16))
    {
      WrError(ErrNum_InvAddrMode);
      return;
    }
    else
      SumAddress.AdrVals[0] |= 1 << Address.AdrVal;
  }

  AssumeByte = True;
  switch (SumAddress.AdrMode)
  {
    case ModReg16:
      BAsmCode[CodeLen++] = Code;
      BAsmCode[CodeLen++] = SumAddress.AdrVals[0];
      break;
    case ModPSW:
      BAsmCode[CodeLen++] = IsPUSH ? 0x49 : 0x48;
      break;
    case ModSFR:
      ExecAssumeByte();
      BAsmCode[CodeLen++] = 0x07;
      BAsmCode[CodeLen++] = (IsPUSH ? 0xdb: 0xda) - (OpSize << 1);
      BAsmCode[CodeLen++] = SumAddress.AdrVals[0];
      break;
    case ModReg24:
      BAsmCode[CodeLen++] = 0x09;
      BAsmCode[CodeLen++] = (IsPUSH ? 0x89 : 0x99) | (SumAddress.AdrVal << 1);
      break;
    case ModNone:
      break;
    default:
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeCALL_BR(Word IsCALL)
{
  tEncodedAddress Addr;

  if (ChkArgCnt(1, 1)
   && StripIndirect(&ArgStr[1]))
  {
    DecodeAdr(&ArgStr[1], MModReg16 | MModReg24, &Addr);
    switch (Addr.AdrMode)
    {
      case ModReg16:
        BAsmCode[CodeLen++] = 0x05;
        BAsmCode[CodeLen++] = 0x68 | (IsCALL << 4) | Addr.AdrVal;
        break;
      case ModReg24:
        BAsmCode[CodeLen++] = 0x05;
        BAsmCode[CodeLen++] = 0x61 | (IsCALL << 4) | (Addr.AdrVal << 1);
        break;
      case ModNone:
        break;
      default:
        WrError(ErrNum_InvAddrMode);
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg16 | MModReg24 | MModAbs16 | MModAbs20, &Addr);
    switch (Addr.AdrMode)
    {
      case ModReg16:
        BAsmCode[CodeLen++] = 0x05;
        BAsmCode[CodeLen++] = 0x48 | (IsCALL << 4) | Addr.AdrVal;
        break;
      case ModReg24:
        BAsmCode[CodeLen++] = 0x05;
        BAsmCode[CodeLen++] = 0x41 | (IsCALL << 4) | (Addr.AdrVal << 1);
        break;
      case ModAbs16:
      case ModAbs20:
      {
        LongWord Dest = GetAbsVal(&Addr);
        LongInt Dist16 = Dest - (EProgCounter() + 3),
                Dist8 = Dest - (EProgCounter() + 2);
        Boolean Dist16OK = (Dist16 >= -0x8000l) && (Dist16 <= 0x7fffl),
                Dist8OK = (Dist8 >= -0x80) && (Dist8 <= 0x7f);

        if (!Addr.ForceAbs && !Addr.ForceRel)
        {
          if (Dist8OK && !IsCALL)
          {
            Addr.ForceRel = True; Addr.ForceAbs = False;
          }
          else if (Dist16OK)
          {
            Addr.ForceRel = Addr.ForceAbs = True;
          }
          else
          {
            Addr.ForceAbs = True; Addr.ForceRel = False;
          }
        }

        if (!Addr.ForceRel)
        {
          if (Addr.AdrMode == ModAbs20)
          {
            BAsmCode[CodeLen++] = 0x09;
            BAsmCode[CodeLen++] = 0xe0 | (IsCALL << 4) | Lo(Addr.AdrVals[2]);
          }
          else
            BAsmCode[CodeLen++] = IsCALL ? 0x28 : 0x2c;
          BAsmCode[CodeLen++] = Addr.AdrVals[0];
          BAsmCode[CodeLen++] = Addr.AdrVals[1];
        }
        else if (IsCALL || Addr.ForceAbs)
        {
          if (!mFirstPassUnknown(Addr.AdrValSymFlags) && !Dist16OK) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[CodeLen++] = IsCALL ? 0x3f : 0x43;
            BAsmCode[CodeLen++] = Dist16 & 0xff;
            BAsmCode[CodeLen++] = (Dist16 >> 8) & 0xff;
          }
        }
        else
        {
          if (!mFirstPassUnknown(Addr.AdrValSymFlags) && !Dist8OK) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[CodeLen++] = 0x14;
            BAsmCode[CodeLen++] = Dist8 & 0xff;
          }
        }

        break;
      }
      case ModNone:
        break;
      default:
        WrError(ErrNum_InvAddrMode);
    }
  }
}

static void DecodeCALLF(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 1) WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;
    Word Addr;
    tSymbolFlags Flags;

    Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt12, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      Addr |= 0x800;
    if (OK && ChkRange(Addr, 0x800, 0xfff))
    {
      BAsmCode[CodeLen++] = 0x90 | ((Addr >> 8) & 7);
      BAsmCode[CodeLen++] = Lo(Addr);
    }
  }
}

static void DecodeCALLT(Word Code)
{
  UNUSED(Code);

  if (ArgCnt != 1) WrError(ErrNum_InvAddrMode);
  else if (!StripIndirect(&ArgStr[1])) WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;
    Word Addr;
    tSymbolFlags Flags;

    Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt7, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      Addr = (Addr | 0x40) & 0xfe;
    if (OK && (Addr & 1)) WrError(ErrNum_NotAligned);
    else if (OK && ChkRange(Addr, 0x40, 0xff))
    {
      BAsmCode[CodeLen++] = 0xe0 | ((Addr >> 1) & 0x1f);
    }
  }
}

static void DecodeBRKCS(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (!DecodeRB(ArgStr[1].str.p_str, BAsmCode + 1)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[CodeLen++] = 0x05;
    BAsmCode[CodeLen++] = BAsmCode[1] | 0xd8;
  }
}

static void DecodeRETCS(Word Code)
{
  if (ArgCnt != 1) WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;
    Word Addr;

    Addr = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt16, &OK);
    if (OK)
    {
      PutCode(Code);
      BAsmCode[CodeLen++] = Lo(Addr);
      BAsmCode[CodeLen++] = Hi(Addr);
    }
  }
}

static void DecodeRel(Word Code)
{
  if (ArgCnt != 1) WrError(ErrNum_InvAddrMode);
  else
  {
    PutCode(Code);
    AppendRel8(&ArgStr[1]);
  }
}

static void DecodeBitRel(Word Code)
{
  LongWord BitAddr;

  if (ArgCnt != 2) WrError(ErrNum_InvAddrMode);
  else if (DecodeBitAdr(&ArgStr[1], &BitAddr))
  {
    switch ((BitAddr >> 28) & 15)
    {
      case eBitTypeSAddr1:
        if (Code != 0xb0)
          goto Common;
        BAsmCode[CodeLen++] = 0x3c;
        BAsmCode[CodeLen++] = 0x70 | ((BitAddr >> 24) & 7);
        BAsmCode[CodeLen++] = BitAddr & 0xff;
        break;
      case eBitTypeSAddr2_SFR:
        if ((Code != 0xb0) || (BitAddr & Bit27))
          goto Common;
        BAsmCode[CodeLen++] = 0x70 | ((BitAddr >> 24) & 7);
        BAsmCode[CodeLen++] = BitAddr & 0xff;
        break;
      Common:
      default:
        DecodeBitOpCore2(BitAddr, Code);
    }

    AppendRel8(&ArgStr[2]);
  }
}

static void DecodeDBNZ(Word Code)
{
  tEncodedAddress Addr;

  UNUSED(Code);

  if (ArgCnt != 2)
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }

  DecodeAdr(&ArgStr[1], MModShort1 | MModShort2 | MModReg8, &Addr);
  switch (Addr.AdrMode)
  {
    case ModShort1:
      BAsmCode[CodeLen++] = 0x3c;
      /* fall-through */
    case ModShort2:
      BAsmCode[CodeLen++] = 0x3b;
      BAsmCode[CodeLen++] = Addr.AdrVals[0];
      break;
    case ModReg8:
      if (Addr.AdrVal == BReg8())
        BAsmCode[CodeLen++] = 0x33;
      else if (Addr.AdrVal == CReg8())
        BAsmCode[CodeLen++] = 0x32;
      else
      {
        WrError(ErrNum_InvAddrMode);
        return;
      }
      break;
    case ModNone:
      return;
    default:
      WrError(ErrNum_InvAddrMode);
      return;
  }

  AppendRel8(&ArgStr[2]);
}

static void DecodeLOCATION(Word Code)
{
  Byte Val;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1))
    return;

  Val = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt4, &OK, &Flags);
  if (!OK)
    return;
  if (mFirstPassUnknown(Flags))
    Val = 0;
  switch (Val)
  {
    case 0:
      BAsmCode[CodeLen++] = 0x09;
      BAsmCode[CodeLen++] = 0xc1;
      BAsmCode[CodeLen++] = 0xfe;
      BAsmCode[CodeLen++] = 0x01;
      break;
    case 15:
      BAsmCode[CodeLen++] = 0x09;
      BAsmCode[CodeLen++] = 0xc1;
      BAsmCode[CodeLen++] = 0xff;
      BAsmCode[CodeLen++] = 0x00;
      break;
    default:
      WrError(ErrNum_OverRange);
  }
}

static void DecodeSEL(Word Code)
{
  Byte Bank;

  UNUSED(Code);

  if ((ArgCnt == 2) && (as_strcasecmp(ArgStr[2].str.p_str, "ALT")))
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }
  if (!ChkArgCnt(1, 2))
    return;

  if (!DecodeRB(ArgStr[1].str.p_str, &Bank)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[CodeLen++] = 0x05;
    BAsmCode[CodeLen++] = 0xa8 | ((ArgCnt - 1) << 4) | Bank;
  }
}

static void DecodeCHK(Word Code)
{
  tEncodedAddress Address;

  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModSFR, &Address))
  {
    PutCode(Code);
    BAsmCode[CodeLen++] = Address.AdrVals[0];
  }
}

static void DecodeMOVTBLW(Word Code)
{
  Byte Addr, Value;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Addr = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt8, &OK);
    if (OK)
    {
      Value = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
      if (OK)
      {
        BAsmCode[CodeLen++] = 0x09;
        BAsmCode[CodeLen++] = 0xa0;
        BAsmCode[CodeLen++] = Addr;
        BAsmCode[CodeLen++] = Value;
      }
    }
  }
}

static void DecodeStringAcc(Word Code)
{
  tEncodedAddress Addr;

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[2], MModReg8, &Addr))
  {
    if (Addr.AdrVal != AccReg8()) WrError(ErrNum_InvAddrMode);
    else if (DecodeAdr(&ArgStr[1], MModMem, &Addr))
    {
      if (Addr.AdrVal != 0x16) WrError(ErrNum_InvAddrMode);
      else if ((Addr.AdrVals[0] & 0xfd) != 0) WrError(ErrNum_InvAddrMode);
      else
        PutCode(Code | ((Addr.AdrVals[0] & 2) << 3));
    }
  }
}

static void DecodeStringString(Word Code)
{
  tEncodedAddress Addr;

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModMem, &Addr))
  {
    Byte Dir = Addr.AdrVals[0];

    if (Addr.AdrVal != 0x16) WrError(ErrNum_InvAddrMode);
    else if ((Addr.AdrVals[0] & 0xfd) != 0) WrError(ErrNum_InvAddrMode);
    else if (DecodeAdr(&ArgStr[2], MModMem, &Addr))
    {
      if (Addr.AdrVal != 0x16) WrError(ErrNum_InvAddrMode);
      else if (Addr.AdrVals[0] != Dir + 1) WrError(ErrNum_InvAddrMode);
      else
        PutCode(Code | ((Dir & 2) << 3));
    }
  }
}

/*-------------------------------------------------------------------------*/
/* dynamic code table handling */

static void AddInstr(const char *pInstr, InstProc Proc, Word Code, unsigned SizeMask)
{
  char Instr[20];

  Code <<= 4;

  if (SizeMask & 1)
    AddInstTable(InstTable, pInstr, Code, Proc);
  if (SizeMask & 2)
  {
    as_snprintf(Instr, sizeof(Instr), "%sW", pInstr);
    AddInstTable(InstTable, Instr, Code + 1, Proc);
  }
  if (SizeMask & 4)
  {
    as_snprintf(Instr, sizeof(Instr), "%sG", pInstr);
    AddInstTable(InstTable, Instr, Code + 2, Proc);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(301);
  SetDynamicInstTable(InstTable);

  AddInstr("MOV"  , DecodeMOV, 0x000, 0x07);
  AddInstr("XCH"  , DecodeXCH, 0x000, 0x03);
  AddInstr("ADD"  , DecodeALU, 0x000, 0x07);
  AddInstr("ADDC" , DecodeALU, 0xff1, 0x01);
  AddInstr("SUB"  , DecodeALU, 0x222, 0x07);
  AddInstr("SUBC" , DecodeALU, 0xff3, 0x01);
  AddInstr("CMP"  , DecodeALU, 0xf77, 0x03);
  AddInstr("AND"  , DecodeALU, 0xff4, 0x01);
  AddInstr("OR"   , DecodeALU, 0xff6, 0x01);
  AddInstr("XOR"  , DecodeALU, 0xff5, 0x01);
  AddInstr("MULU" , DecodeMULU,0x000, 0x03);
  AddInstTable(InstTable, "DIVUW", 0, DecodeDIVUW);
  AddInstTable(InstTable, "MULW",  0x38, DecodeMULW_DIVUX);
  AddInstTable(InstTable, "DIVUX", 0xe8, DecodeMULW_DIVUX);
  AddInstTable(InstTable, "MACW" , 0x85, DecodeMACW_MACSW);
  AddInstTable(InstTable, "MACSW", 0x95, DecodeMACW_MACSW);
  AddInstTable(InstTable, "SACW",  0, DecodeSACW);
  AddInstr("INC"  , DecodeINCDEC, 0, 0x07);
  AddInstr("DEC"  , DecodeINCDEC, 1, 0x07);
  AddInstTable(InstTable, "ADJBA", 0x05fe, DecodeFixed);
  AddInstTable(InstTable, "ADJBS", 0x05ff, DecodeFixed);
  AddInstTable(InstTable, "CVTBW", 0x0004, DecodeFixed);
  AddInstr("ROR"  , DecodeShift, 0x304, 0x01);
  AddInstr("ROL"  , DecodeShift, 0x314, 0x01);
  AddInstr("RORC" , DecodeShift, 0x300, 0x01);
  AddInstr("ROLC" , DecodeShift, 0x310, 0x01);
  AddInstr("SHR"  , DecodeShift, 0xb08, 0x03);
  AddInstr("SHL"  , DecodeShift, 0xb18, 0x03);
  AddInstTable(InstTable, "ROR4", 0x88, DecodeROR4_ROL4);
  AddInstTable(InstTable, "ROL4", 0x98, DecodeROR4_ROL4);
  AddInstTable(InstTable, "MOV1", 0, DecodeMOV1);
  AddInstTable(InstTable, "AND1", 0x20, DecodeAND1_OR1);
  AddInstTable(InstTable, "OR1",  0x40, DecodeAND1_OR1);
  AddInstTable(InstTable, "XOR1", 0x60, DecodeXOR1);
  AddInstTable(InstTable, "NOT1", 0x70, DecodeNOT1);
  AddInstTable(InstTable, "SET1", 0x80, DecodeSET1_CLR1);
  AddInstTable(InstTable, "CLR1", 0x90, DecodeSET1_CLR1);
  AddInstr("PUSH", DecodePUSH_POP, 0x35, 0x07);
  AddInstr("PUSHU",DecodePUSH_POP, 0x37, 0x03);
  AddInstr("POP" , DecodePUSH_POP, 0x34, 0x07);
  AddInstr("POPU", DecodePUSH_POP, 0x36, 0x03);
  AddInstTable(InstTable, "ADDWG", 0, DecodeADDWG_SUBWG);
  AddInstTable(InstTable, "SUBWG", 2, DecodeADDWG_SUBWG);

  AddInstTable(InstTable, "CALL", 1, DecodeCALL_BR);
  AddInstTable(InstTable, "BR"  , 0, DecodeCALL_BR);
  AddInstTable(InstTable, "CALLF", 0, DecodeCALLF);
  AddInstTable(InstTable, "CALLT", 0, DecodeCALLT);
  AddInstTable(InstTable, "BRK", 0x5e, DecodeFixed);
  AddInstTable(InstTable, "BRKCS", 0, DecodeBRKCS);
  AddInstTable(InstTable, "RET", 0x56, DecodeFixed);
  AddInstTable(InstTable, "RETI", 0x57, DecodeFixed);
  AddInstTable(InstTable, "RETB", 0x5f, DecodeFixed);
  AddInstTable(InstTable, "RETCS", 0x29, DecodeRETCS);
  AddInstTable(InstTable, "RETCSB", 0x09b0, DecodeRETCS);

  AddInstTable(InstTable, "BNZ", 0x80, DecodeRel);
  AddInstTable(InstTable, "BNE", 0x80, DecodeRel);
  AddInstTable(InstTable, "BZ", 0x81, DecodeRel);
  AddInstTable(InstTable, "BE", 0x81, DecodeRel);
  AddInstTable(InstTable, "BNC", 0x82, DecodeRel);
  AddInstTable(InstTable, "BNL", 0x82, DecodeRel);
  AddInstTable(InstTable, "BC", 0x83, DecodeRel);
  AddInstTable(InstTable, "BL", 0x83, DecodeRel);
  AddInstTable(InstTable, "BNV", 0x84, DecodeRel);
  AddInstTable(InstTable, "BPO", 0x84, DecodeRel);
  AddInstTable(InstTable, "BV", 0x85, DecodeRel);
  AddInstTable(InstTable, "BPE", 0x85, DecodeRel);
  AddInstTable(InstTable, "BP", 0x86, DecodeRel);
  AddInstTable(InstTable, "BN", 0x87, DecodeRel);
  AddInstTable(InstTable, "BLT", 0x07f8, DecodeRel);
  AddInstTable(InstTable, "BGE", 0x07f9, DecodeRel);
  AddInstTable(InstTable, "BLE", 0x07fa, DecodeRel);
  AddInstTable(InstTable, "BGT", 0x07fb, DecodeRel);
  AddInstTable(InstTable, "BNH", 0x07fc, DecodeRel);
  AddInstTable(InstTable, "BH", 0x07fd, DecodeRel);
  AddInstTable(InstTable, "BF", 0xa0, DecodeBitRel);
  AddInstTable(InstTable, "BT", 0xb0, DecodeBitRel);
  AddInstTable(InstTable, "BTCLR", 0xd0, DecodeBitRel);
  AddInstTable(InstTable, "BFSET", 0xc0, DecodeBitRel);
  AddInstTable(InstTable, "DBNZ", 0, DecodeDBNZ);

  AddInstTable(InstTable, "LOCATION", 0, DecodeLOCATION);
  AddInstTable(InstTable, "SEL", 0, DecodeSEL);
  AddInstTable(InstTable, "SWRS", 0x05fc, DecodeFixed);
  AddInstTable(InstTable, "NOP", 0x00, DecodeFixed);
  AddInstTable(InstTable, "EI", 0x4b, DecodeFixed);
  AddInstTable(InstTable, "DI", 0x4a, DecodeFixed);

  AddInstTable(InstTable, "CHKL", 0x07c8, DecodeCHK);
  AddInstTable(InstTable, "CHKLA", 0x07c9, DecodeCHK);

  AddInstTable(InstTable, "MOVTBLW", 0, DecodeMOVTBLW);
  AddInstTable(InstTable, "MOVM"   , 0x1500, DecodeStringAcc);
  AddInstTable(InstTable, "MOVBK"  , 0x1520, DecodeStringString);
  AddInstTable(InstTable, "XCHM"   , 0x1501, DecodeStringAcc);
  AddInstTable(InstTable, "XCHBK"  , 0x1521, DecodeStringString);
  AddInstTable(InstTable, "CMPME"  , 0x1504, DecodeStringAcc);
  AddInstTable(InstTable, "CMPBKE" , 0x1524, DecodeStringString);
  AddInstTable(InstTable, "CMPMNE" , 0x1505, DecodeStringAcc);
  AddInstTable(InstTable, "CMPBKNE", 0x1525, DecodeStringString);
  AddInstTable(InstTable, "CMPMC"  , 0x1507, DecodeStringAcc);
  AddInstTable(InstTable, "CMPBKC" , 0x1527, DecodeStringString);
  AddInstTable(InstTable, "CMPMNC" , 0x1506, DecodeStringAcc);
  AddInstTable(InstTable, "CMPBKNC", 0x1526, DecodeStringString);

  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/
/* interface to common layer */

static void MakeCode_78K4(void)
{
  CodeLen = 0; DontPrint = False; OpSize = -1;
  AssumeByte = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_78K4(void)
{
  return Memo("BIT");
}

static void InternSymbol_78K4(char *pAsc, TempResult *pErg)
{
  if ((!as_strcasecmp(pAsc, "PSWL"))
   || (!as_strcasecmp(pAsc, "PSW")))
    as_tempres_set_int(pErg, PSWLAddr);
  else if (!as_strcasecmp(pAsc, "PSWH"))
    as_tempres_set_int(pErg, PSWHAddr);
}

static void SwitchFrom_78K4(void)
{
  DeinitFields();
}

static void SwitchTo_78K4(void)
{
  const TFamilyDescr *pDescr;

  pDescr = FindFamilyByName("78K4");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "PC";
  HeaderID = pDescr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffffff;

  pASSUMERecs = ASSUME78K4s;
  ASSUMERecCnt = sizeof(ASSUME78K4s) / sizeof(ASSUME78K4s[0]);

  MakeCode = MakeCode_78K4;
  IsDef = IsDef_78K4;
  InternSymbol = InternSymbol_78K4;
  SwitchFrom = SwitchFrom_78K4; InitFields();
}

void code78k4_init(void)
{
  CPU784026 = AddCPU("784026", SwitchTo_78K4);
}
