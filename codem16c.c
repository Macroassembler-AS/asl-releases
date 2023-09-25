/* codem16c.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator M16C                                                        */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmallg.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codem16c.h"

#define REG_SB 6
#define REG_FB 7

#define ModNone (-1)
#define ModGen 0
#define MModGen (1 << ModGen)
#define ModAbs20 1
#define MModAbs20 (1 << ModAbs20)
#define ModAReg32 2
#define MModAReg32 (1 << ModAReg32)
#define ModDisp20 3
#define MModDisp20 (1 << ModDisp20)
#define ModReg32 4
#define MModReg32 (1 << ModReg32)
#define ModIReg32 5
#define MModIReg32 (1 << ModIReg32)
#define ModImm 6
#define MModImm (1 << ModImm)
#define ModSPRel 7
#define MModSPRel (1 << ModSPRel)

typedef struct
{
  Byte Code1,Code2,Code3;
} Gen2Order;

typedef struct
{
  Byte Mode;
  ShortInt Type;
  Byte Cnt;
  Byte Vals[3];
  tSymbolFlags ImmSymFlags;
} tAdrResult;

static const char Flags[] = "CDZSBOIU";

static CPUVar CPUM16C, CPUM30600M8, CPUM30610, CPUM30620;

static char *Format;
static Byte FormatCode;
static ShortInt OpSize;

static Gen2Order *Gen2Orders;
static Gen2Order *DivOrders;

/*------------------------------------------------------------------------*/
/* Adressparser */

static void SetOpSize(ShortInt NSize, tAdrResult *pResult)
{
  if (OpSize == eSymbolSizeUnknown)
    OpSize = NSize;
  else if (NSize != OpSize)
  {
    WrError(ErrNum_ConfOpSizes);
    if (pResult)
    {
      pResult->Cnt = 0;
      pResult->Type = ModNone;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Byte *pResult, tSymbolSize *pSize)
 * \brief  is argument a COU register?
 * \param  pArg source argument
 * \param  pResult result buffer
 * \param  pSize resulting register size
 * \return True if it's a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Byte *pResult, tSymbolSize *pSize)
{
  if (!as_strcasecmp(pArg, "FB"))
  {
    *pResult = REG_FB;
    *pSize = eSymbolSize16Bit;
    return True;
  }
  if (!as_strcasecmp(pArg, "SB"))
  {
    *pResult = REG_SB;
    *pSize = eSymbolSize16Bit;
    return True;
  }
  if (!as_strcasecmp(pArg, "R2R0"))
  {
    *pResult = 0;
    *pSize = eSymbolSize32Bit;
    return True;
  }
  if (!as_strcasecmp(pArg, "R3R1"))
  {
    *pResult = 1;
    *pSize = eSymbolSize32Bit;
    return True;
  }
  if (!as_strcasecmp(pArg, "A1A0"))
  {
    *pResult = 2;
    *pSize = eSymbolSize32Bit;
    return True;
  }

  switch (strlen(pArg))
  {
    case 3:
      if ((as_toupper(*pArg) == 'R')
       && (pArg[1] >= '0') && (pArg[1] <= '1')
       && ((as_toupper(pArg[2]) == 'L') || (as_toupper(pArg[2]) == 'H')))
      {
        *pResult = ((pArg[1] - '0') << 1) + Ord(as_toupper(pArg[2]) == 'H');
        *pSize = eSymbolSize8Bit;
        return True;
      }
      break;
    case 2:
      if ((as_toupper(*pArg) == 'R')
       && (pArg[1] >= '0') && (pArg[1] <= '3'))
      {
        *pResult = pArg[1] - '0';
        *pSize = eSymbolSize16Bit;
        return True;
      }
      if ((as_toupper(*pArg) == 'A')
       && (pArg[1] >= '0') && (pArg[1] <= '1'))
      {
        *pResult = pArg[1] - '0' + 4;
        *pSize = eSymbolSize16Bit;
        return True;
      }
      break;
  }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_M16C(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - M16C variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_M16C(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
      if (Value >= 2)
        as_snprintf(pDest, DestSize, "A%uA%u", Value - 1, Value - 2);
      else
        as_snprintf(pDest, DestSize, "R%uR%u", Value + 2, Value);
      break;
    case eSymbolSize16Bit:
      switch (Value)
      {
        case REG_FB:
          as_snprintf(pDest, DestSize, "FB");
          break;
        case REG_SB:
          as_snprintf(pDest, DestSize, "SB");
          break;
        default:
          as_snprintf(pDest, DestSize, "%c%u", (Value & 4) ? 'A' : 'R', (unsigned)(Value & 3));
      }
      break;
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%u%c", (unsigned)(Value >> 1), "HL"[Value & 1]);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg, tSymbolSize *pSize)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \param  MustBeReg argument is expected to be a register
 * \param  pSize register size (in/out)
 * \return RegEvalResult
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg, tSymbolSize *pSize)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult, &EvalResult.DataSize))
  {
    if ((*pSize != eSymbolSizeUnknown) && (EvalResult.DataSize != *pSize))
    {
      WrStrErrorPos(ErrNum_InvOpSize, pArg);
      return MustBeReg ? eIsNoReg : eRegAbort;
    }
    *pSize = EvalResult.DataSize;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, *pSize, MustBeReg);
  *pResult = RegDescr.Reg;
  *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

static ShortInt DecodeAdr(const tStrComp *pArg, Word Mask, tAdrResult *pResult)
{
  LongInt DispAcc;
  String RegPartStr, DispPartStr;
  tStrComp RegPart, DispPart;
  char *p;
  Boolean OK;
  tSymbolSize RegSize = eSymbolSizeUnknown;
  int ArgLen = strlen(pArg->str.p_str);

  pResult->Cnt = 0;
  pResult->Type = ModNone;
  StrCompMkTemp(&RegPart, RegPartStr, sizeof(RegPartStr));
  StrCompMkTemp(&DispPart, DispPartStr, sizeof(DispPartStr));

  switch (DecodeReg(pArg, &pResult->Mode, False, &RegSize))
  {
    case eIsReg:
      switch (RegSize)
      {
        /* Data Register R(0|1)(L|H) */
        case eSymbolSize8Bit:
          pResult->Type = ModGen;
          SetOpSize(RegSize, pResult);
          goto chk;
        case eSymbolSize16Bit:
          /* Data Register R0..R3, Address Register A0...A1 */
          if (pResult->Type < 6)
          {
            pResult->Type = ModGen;
            /* opsize may be overridden by attribute */
            goto chk;
          }
          break;
        case eSymbolSize32Bit:
          /* Data Register R2R0/R3R1, Address Register A1A0 */
          pResult->Type = (pResult->Mode < 2) ? ModReg32 : ModAReg32;
          pResult->Mode &= 1;
          SetOpSize(RegSize, pResult);
          goto chk;
        default:
          break;
      }
      break;
    case eIsNoReg:
      break;
    case eRegAbort:
      return pResult->Type;
  }

  /* indirekt */

  p = strchr(pArg->str.p_str, '[');
  if ((p) && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    RegSize = eSymbolSizeUnknown;
    StrCompSplitCopy(&DispPart, &RegPart, pArg, p);
    StrCompShorten(&RegPart, 1);
    if (!as_strcasecmp(RegPart.str.p_str, "SP"))
    {
      DispAcc = EvalStrIntExpression(&DispPart, SInt8, &OK);
      if (OK)
      {
        pResult->Type = ModSPRel;
        pResult->Vals[0] = DispAcc & 0xff;
        pResult->Cnt = 1;
      }
    }
    else if (eIsReg == DecodeReg(&RegPart, &pResult->Mode, True, &RegSize))
    {
      switch (RegSize)
      {
        case eSymbolSize8Bit:
          WrStrErrorPos(ErrNum_InvReg, &RegPart);
          break;
        case eSymbolSize16Bit:
          switch (pResult->Mode)
          {
            case REG_SB:
              DispAcc = EvalStrIntExpression(&DispPart, Int16, &OK);
              if (OK)
              {
                if ((DispAcc >= 0) && (DispAcc <= 255))
                {
                  pResult->Type = ModGen;
                  pResult->Vals[0] = DispAcc & 0xff;
                  pResult->Cnt = 1;
                  pResult->Mode = 10;
                }
                else
                {
                  pResult->Type = ModGen;
                  pResult->Vals[0] = DispAcc & 0xff;
                  pResult->Vals[1] = (DispAcc >> 8) & 0xff;
                  pResult->Cnt = 2;
                  pResult->Mode = 14;
                }
              }
              break;
            case REG_FB:
              DispAcc = EvalStrIntExpression(&DispPart, SInt8, &OK);
              if (OK)
              {
                pResult->Type = ModGen;
                pResult->Vals[0] = DispAcc & 0xff;
                pResult->Cnt = 1;
                pResult->Mode = 11;
              }
              break;
            case 4: case 5:
            {
              DispAcc = EvalStrIntExpression(&DispPart, (Mask & MModDisp20)  ? Int20 : Int16, &OK);
              if (OK)
              {
                if ((DispAcc == 0) && (Mask & MModGen))
                {
                  pResult->Type = ModGen;
                  pResult->Mode = (pResult->Mode & 1) + 6;
                }
                else if ((DispAcc >= 0) && (DispAcc <= 255) && (Mask & MModGen))
                {
                  pResult->Type = ModGen;
                  pResult->Vals[0] = DispAcc & 0xff;
                  pResult->Cnt = 1;
                  pResult->Mode = (pResult->Mode & 1) + 8;
                }
                else if ((DispAcc >= -32768) && (DispAcc <= 65535) && (Mask & MModGen))
                {
                  pResult->Type = ModGen;
                  pResult->Vals[0] = DispAcc & 0xff;
                  pResult->Vals[1] = (DispAcc >> 8) & 0xff;
                  pResult->Cnt = 2;
                  pResult->Mode = (pResult->Mode & 1) + 12;
                }
                else if (pResult->Mode != 4) WrError(ErrNum_InvAddrMode);
                else
                {
                  pResult->Type = ModDisp20;
                  pResult->Vals[0] = DispAcc & 0xff;
                  pResult->Vals[1] = (DispAcc >> 8) & 0xff;
                  pResult->Vals[2] = (DispAcc >> 16) & 0x0f;
                  pResult->Cnt = 3;
                  pResult->Mode = 0;
                }
              }
              break;
            }
            default:
              WrStrErrorPos(ErrNum_InvReg, &RegPart);
          }
          break;
        case eSymbolSize32Bit:
          if (pResult->Mode != 2) WrStrErrorPos(ErrNum_InvReg, &RegPart);
          else
          {
            DispAcc = EvalStrIntExpression(&DispPart, SInt8, &OK);
            if (OK)
            {
              if (DispAcc != 0) WrError(ErrNum_OverRange);
              else pResult->Type = ModIReg32;
            }
          }
          break;
        default:
          break;
      }
    }
    goto chk;
  }

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case eSymbolSizeUnknown:
        WrError(ErrNum_UndefOpSizes);
        break;
      case eSymbolSize8Bit:
        pResult->Vals[0] = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int8, &OK, &pResult->ImmSymFlags);
        if (OK)
        {
          pResult->Type = ModImm;
          pResult->Cnt = 1;
        }
        break;
      case eSymbolSize16Bit:
        DispAcc = EvalStrIntExpressionOffsWithFlags(pArg, 1, Int16, &OK, &pResult->ImmSymFlags);
        if (OK)
        {
          pResult->Type = ModImm;
          pResult->Vals[0] = DispAcc & 0xff;
          pResult->Vals[1] = (DispAcc >> 8) & 0xff;
          pResult->Cnt = 2;
        }
        break;
    }
    goto chk;
  }

  /* then it's absolute: */

  DispAcc = EvalStrIntExpression(pArg, (Mask & MModAbs20) ? UInt20 : UInt16, &OK);
  if ((DispAcc <= 0xffff) && ((Mask & MModGen) != 0))
  {
    pResult->Type = ModGen;
    pResult->Mode = 15;
    pResult->Vals[0] = DispAcc & 0xff;
    pResult->Vals[1] = (DispAcc >> 8) & 0xff;
    pResult->Cnt = 2;
  }
  else
  {
    pResult->Type = ModAbs20;
    pResult->Vals[0] = DispAcc & 0xff;
    pResult->Vals[1] = (DispAcc >> 8) & 0xff;
    pResult->Vals[2] = (DispAcc >> 16) & 0x0f;
    pResult->Cnt = 3;
  }

chk:
  if ((pResult->Type != ModNone) && ((Mask & (1 << pResult->Type)) == 0))
  {
     pResult->Cnt = 0;
     pResult->Type = ModNone;
     WrError(ErrNum_InvAddrMode);
  }
  return pResult->Type;
}

static Boolean DecodeCReg(char *Asc, Byte *Erg)
{
  if (!as_strcasecmp(Asc, "INTBL")) *Erg = 1;
  else if (!as_strcasecmp(Asc, "INTBH")) *Erg = 2;
  else if (!as_strcasecmp(Asc, "FLG")) *Erg = 3;
  else if (!as_strcasecmp(Asc, "ISP")) *Erg = 4;
  else if (!as_strcasecmp(Asc, "SP")) *Erg = 5;
  else if (!as_strcasecmp(Asc, "SB")) *Erg = 6;
  else if (!as_strcasecmp(Asc, "FB")) *Erg = 7;
  else
  {
    WrXError(ErrNum_InvCtrlReg, Asc);
    return False;
  }
  return True;
}

static void DecodeDisp(tStrComp *pArg, IntType Type1, IntType Type2, LongInt *DispAcc, Boolean *OK)
{
  if (ArgCnt == 2)
    *DispAcc += EvalStrIntExpression(pArg, Type2, OK) * 8;
  else
    *DispAcc = EvalStrIntExpression(pArg, Type1, OK);
}

static Boolean DecodeBitAdr(Boolean MayShort, tAdrResult *pResult)
{
  LongInt DispAcc;
  Boolean OK;
  char *Pos1;
  String RegPartStr, DispPartStr;
  tStrComp RegPart, DispPart;
  int ArgLen;
  tSymbolSize RegSize = eSymbolSize16Bit;

  pResult->Cnt = 0;
  StrCompMkTemp(&RegPart, RegPartStr, sizeof(RegPartStr));
  StrCompMkTemp(&DispPart, DispPartStr, sizeof(DispPartStr));

  /* Nur 1 oder 2 Argumente zugelassen */

  if (!ChkArgCnt(1, 2))
    return False;

  /* Ist Teil 1 ein Register ? */

  switch (DecodeReg(&ArgStr[ArgCnt], &pResult->Mode, False, &RegSize))
  {
    case eIsReg:
      if (pResult->Mode < 6)
      {
        if (ChkArgCnt(2, 2))
        {
          pResult->Vals[0] = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
          if (OK)
          {
            pResult->Cnt = 1;
            return True;
          }
        }
        return False;
      }
      break;
    case eRegAbort:
      return False;
    case eIsNoReg:
      break;
  }

  /* Bitnummer ? */

  if (ArgCnt == 2)
  {
    DispAcc = EvalStrIntExpression(&ArgStr[1], UInt16, &OK); /* RMS 02: The displacement can be 16 bits */
    if (!OK)
      return False;
  }
  else
    DispAcc = 0;

  /* Registerangabe ? */

  Pos1 = QuotPos(ArgStr[ArgCnt].str.p_str, '[');

  /* nein->absolut */

  if (!Pos1)
  {
    DecodeDisp(&ArgStr[ArgCnt], UInt16, UInt13, &DispAcc, &OK);
    if (OK && (DispAcc < 0x10000))     /* RMS 09: This is optional, it detects rollover of the bit address. */
    {
      pResult->Mode = 15;
      pResult->Vals[0] = DispAcc & 0xff;
      pResult->Vals[1] = (DispAcc >> 8) & 0xff;
      pResult->Cnt = 2;
      return True;
    }
    WrStrErrorPos(ErrNum_InvBitPos, &ArgStr[ArgCnt]);   /* RMS 08: Notify user there's a problem with address */
    return False;
  }

  /* Register abspalten */

  ArgLen = strlen(ArgStr[ArgCnt].str.p_str);
  if ((*ArgStr[ArgCnt].str.p_str) && (ArgStr[ArgCnt].str.p_str[ArgLen - 1] != ']'))
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  StrCompSplitCopy(&DispPart, &RegPart, &ArgStr[ArgCnt], Pos1);
  StrCompShorten(&RegPart, 1);

  if (DecodeReg(&RegPart, &pResult->Mode, True, &RegSize) == eIsReg)
  {
    switch (pResult->Mode)
    {
      case REG_SB:
        DecodeDisp(&DispPart, UInt13, UInt16, &DispAcc, &OK);
        if (OK)
        {
          if ((MayShort) && (DispAcc <= 0x7ff))
          {
            pResult->Mode = 16 + (DispAcc & 7);
            pResult->Vals[0] = DispAcc >> 3;
            pResult->Cnt = 1;
          }
          else if ((DispAcc > 0) && (DispAcc < 256))
          {
            pResult->Mode = 10;
            pResult->Vals[0] = DispAcc & 0xff;
            pResult->Cnt = 1;
          }
          else
          {
            pResult->Mode = 14;
            pResult->Vals[0] = DispAcc & 0xff;
            pResult->Vals[1] = (DispAcc >> 8) & 0xff;
            pResult->Cnt = 2;
          }
          return True;
        }
        WrError(ErrNum_InvBitPos);             /* RMS 08: Notify user there's a problem with the offset */
        return False;
      case REG_FB:
        DecodeDisp(&DispPart, SInt5, SInt8, &DispAcc, &OK);
        if (OK)
        {
          pResult->Mode = 11;
          pResult->Vals[0] = DispAcc & 0xff;
          pResult->Cnt = 1;
          return True;
        }
        WrError(ErrNum_InvBitPos);             /* RMS 08: Notify user there's a problem with the offset */
        return False;
      case 4: case 5:
        pResult->Mode &= 1;
        DecodeDisp(&DispPart, UInt16, UInt16, &DispAcc, &OK); /* RMS 03: The offset is a full 16 bits */
        if (OK)
        {
          if (DispAcc == 0) pResult->Mode += 6;
          else if ((DispAcc > 0) && (DispAcc < 256))
          {
            pResult->Mode += 8;
            pResult->Vals[0] = DispAcc & 0xff;
            pResult->Cnt = 1;
          }
          else
          {
            pResult->Mode += 12;
            pResult->Vals[0] = DispAcc & 0xff;
            pResult->Vals[1] = (DispAcc >> 8) & 0xff;
            pResult->Cnt = 2;
          }
          return True;
        }
        WrError(ErrNum_InvBitPos);             /* RMS 08: Notify user there's a problem with the offset */
        return False;
      default:
        WrStrErrorPos(ErrNum_InvReg, &RegPart);
        return False;
    }
  }
  else
    return False;
}

static Boolean CheckFormat(const char *FSet)
{
  const char *p;

  if (!strcmp(Format, " "))
  {
    FormatCode = 0;
    return True;
  }
  else
  {
    p = strchr(FSet, *Format);
    if (!p) WrError(ErrNum_InvFormat);
    else FormatCode = p - FSet + 1;
    return (p != 0);
  }
}

static Integer ImmVal(const tAdrResult *pResult)
{
  if (OpSize == eSymbolSize8Bit)
    return (ShortInt)pResult->Vals[0];
  else
    return (((Integer)pResult->Vals[1]) << 8) + pResult->Vals[0];
}

static Boolean IsShort(Byte GenMode, Byte *SMode)
{
  switch (GenMode)
  {
    case 0:  *SMode = 4; break;
    case 1:  *SMode = 3; break;
    case 10: *SMode = 5; break;
    case 11: *SMode = 6; break;
    case 15: *SMode = 7; break;
    default: return False;
  }
  return True;
}

static void CodeGen(Byte GenCode, Byte Imm1Code, Byte Imm2Code, const tAdrResult *pSrcAdrResult, const tAdrResult *pDestAdrResult)
{
  if (pSrcAdrResult->Type == ModImm)
  {
    BAsmCode[0] = Imm1Code + OpSize;
    BAsmCode[1] = Imm2Code + pDestAdrResult->Mode;
    memcpy(BAsmCode + 2, pDestAdrResult->Vals, pDestAdrResult->Cnt);
    memcpy(BAsmCode + 2 + pDestAdrResult->Cnt, pSrcAdrResult->Vals, pSrcAdrResult->Cnt);
  }
  else
  {
    BAsmCode[0] = GenCode + OpSize;
    BAsmCode[1] = (pSrcAdrResult->Mode << 4) + pDestAdrResult->Mode;
    memcpy(BAsmCode + 2, pSrcAdrResult->Vals, pSrcAdrResult->Cnt);
    memcpy(BAsmCode + 2 + pSrcAdrResult->Cnt, pDestAdrResult->Vals, pDestAdrResult->Cnt);
  }
  CodeLen = 2 + pSrcAdrResult->Cnt + pDestAdrResult->Cnt;
}

/*------------------------------------------------------------------------*/
/* instruction decoders */

static void DecodeFixed(Word Code)
{
  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (strcmp(Format, " ")) WrError(ErrNum_InvFormat);
  else if (!Hi(Code))
  {
    BAsmCode[0] = Lo(Code);
    CodeLen = 1;
  }
  else
  {
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
    CodeLen = 2;
  }
}

static void DecodeString(Word Code)
{
  if (OpSize == eSymbolSizeUnknown) OpSize = 1;
  if (!ChkArgCnt(0, 0));
  else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
  else if (strcmp(Format," ")) WrError(ErrNum_InvFormat);
  else if (!Hi(Code))
  {
    BAsmCode[0] = Lo(Code) + OpSize;
    CodeLen = 1;
  }
  else
  {
    BAsmCode[0] = Hi(Code) + OpSize;
    BAsmCode[1] = Lo(Code);
    CodeLen = 2;
  }
}

static void DecodeMOV(Word Code)
{
  Integer Num1;
  Byte SMode;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckFormat("GSQZ"))
  {
    tAdrResult DestAdrResult, SrcAdrResult;

    if ((DecodeAdr(&ArgStr[2], MModGen | MModSPRel, &DestAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[1], MModGen | MModSPRel | MModImm, &SrcAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if ((DestAdrResult.Type == ModSPRel) || (SrcAdrResult.Type == ModSPRel))
            FormatCode = 1;
          else if ((OpSize == 0) && (SrcAdrResult.Type == ModImm) && (IsShort(DestAdrResult.Mode, &SMode)))
            FormatCode = (ImmVal(&SrcAdrResult) == 0) ? 4 : 2;
          else if ((SrcAdrResult.Type == ModImm) && (ImmVal(&SrcAdrResult) >= -8) && (ImmVal(&SrcAdrResult) <= 7))
            FormatCode = 3;
          else if ((SrcAdrResult.Type == ModImm) && ((DestAdrResult.Mode & 14) == 4))
            FormatCode = 2;
          else if ((OpSize == 0) && (SrcAdrResult.Type == ModGen) && (IsShort(SrcAdrResult.Mode, &SMode)) && ((DestAdrResult.Mode & 14) == 4)
                && ((SrcAdrResult.Mode >= 2) || (Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))))
            FormatCode = 2;
          else if ((OpSize == 0) && (SrcAdrResult.Type == ModGen) && (SrcAdrResult.Mode <= 1) && (IsShort(DestAdrResult.Mode, &SMode))
                && ((DestAdrResult.Mode >= 2) || (Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))))
            FormatCode = 2;
          else if ((OpSize == 0) && (DestAdrResult.Mode <= 1) && (SrcAdrResult.Type == ModGen) && (IsShort(SrcAdrResult.Mode, &SMode))
                && ((SrcAdrResult.Mode >= 2) || (Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))))
            FormatCode = 2;
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            if (SrcAdrResult.Type == ModSPRel)
            {
              BAsmCode[0] = 0x74 + OpSize;
              BAsmCode[1] = 0xb0 + DestAdrResult.Mode;
              memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
              memcpy(BAsmCode + 2 + DestAdrResult.Cnt, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              CodeLen = 2 + SrcAdrResult.Cnt + DestAdrResult.Cnt;
            }
            else if (DestAdrResult.Type == ModSPRel)
            {
              BAsmCode[0] = 0x74 + OpSize;
              BAsmCode[1] = 0x30 + SrcAdrResult.Mode;
              memcpy(BAsmCode + 2, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              memcpy(BAsmCode + 2 + SrcAdrResult.Cnt, DestAdrResult.Vals, DestAdrResult.Cnt);
              CodeLen = 2 + DestAdrResult.Cnt + SrcAdrResult.Cnt;
            }
            else
              CodeGen(0x72, 0x74, 0xc0, &SrcAdrResult, &DestAdrResult);
            break;
          case 2:
            if (SrcAdrResult.Type == ModImm)
            {
              if (DestAdrResult.Type != ModGen) WrError(ErrNum_InvAddrMode);
              else if ((DestAdrResult.Mode & 14) == 4)
              {
                BAsmCode[0] = 0xe2 - (OpSize << 6) + ((DestAdrResult.Mode & 1) << 3);
                memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
                CodeLen = 1 + SrcAdrResult.Cnt;
              }
              else if (IsShort(DestAdrResult.Mode, &SMode))
              {
                if (OpSize != 0) WrError(ErrNum_InvOpSize);
                else
                {
                  BAsmCode[0] = 0xc0 + SMode;
                  memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
                  memcpy(BAsmCode + 1 + SrcAdrResult.Cnt, DestAdrResult.Vals, DestAdrResult.Cnt);
                  CodeLen = 1 + SrcAdrResult.Cnt + DestAdrResult.Cnt;
                }
              }
              else
                WrError(ErrNum_InvAddrMode);
            }
            else if ((SrcAdrResult.Type == ModGen) && (IsShort(SrcAdrResult.Mode, &SMode)))
            {
              if (DestAdrResult.Type != ModGen) WrError(ErrNum_InvAddrMode);
              else if ((DestAdrResult.Mode & 14) == 4)
              {
                if ((SrcAdrResult.Mode <= 1) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
                else
                {
                  if (SMode == 3) SMode++;
                  BAsmCode[0] = 0x30 + ((DestAdrResult.Mode & 1) << 2) + (SMode & 3);
                  memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
                  CodeLen = 1 + SrcAdrResult.Cnt;
                }
              }
              else if ((DestAdrResult.Mode & 14) == 0)
              {
                if ((SrcAdrResult.Mode <= 1) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
                else
                {
                  if (SMode == 3) SMode++;
                  BAsmCode[0] = 0x08 + ((DestAdrResult.Mode & 1) << 2) + (SMode & 3);
                  memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
                  CodeLen = 1 + SrcAdrResult.Cnt;
                }
              }
              else if (((SrcAdrResult.Mode & 14) != 0) || (!IsShort(DestAdrResult.Mode, &SMode))) WrError(ErrNum_InvAddrMode);
              else if ((DestAdrResult.Mode <= 1) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
              else
              {
                if (SMode == 3) SMode++;
                BAsmCode[0] = 0x00 + ((SrcAdrResult.Mode & 1) << 2) + (SMode & 3);
                memcpy(BAsmCode + 1, DestAdrResult.Vals, DestAdrResult.Cnt);
                CodeLen = 1 + DestAdrResult.Cnt;
              }
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
          case 3:
            if (SrcAdrResult.Type != ModImm) WrError(ErrNum_InvAddrMode);
            else
            {
              Num1 = ImmVal(&SrcAdrResult);
              if (ChkRange(Num1, -8, 7))
              {
                BAsmCode[0] = 0xd8 + OpSize;
                BAsmCode[1] = (Num1 << 4) + DestAdrResult.Mode;
                memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
                CodeLen = 2 + DestAdrResult.Cnt;
              }
            }
            break;
          case 4:
            if (OpSize != 0) WrError(ErrNum_InvOpSize);
            else if (SrcAdrResult.Type != ModImm) WrError(ErrNum_InvAddrMode);
            else if (!IsShort(DestAdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
            else
            {
              Num1 = ImmVal(&SrcAdrResult);
              if (ChkRange(Num1, 0, 0))
              {
                BAsmCode[0] = 0xb0 + SMode;
                memcpy(BAsmCode + 1, DestAdrResult.Vals, DestAdrResult.Cnt);
                CodeLen = 1 + DestAdrResult.Cnt;
              }
            }
            break;
        }
      }
    }
  }
}

static void DecodeLDC_STC(Word IsSTC)
{
  Byte CReg;

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    const tStrComp *pRegArg = IsSTC ? &ArgStr[1] : &ArgStr[2],
                   *pMemArg = IsSTC ? &ArgStr[2] : &ArgStr[1];

    if (!as_strcasecmp(pRegArg->str.p_str, "PC"))
    {
      if (!IsSTC) WrError(ErrNum_InvAddrMode);
      else
      {
        tAdrResult AdrResult;

        DecodeAdr(pMemArg, MModGen | MModReg32 | MModAReg32, &AdrResult);
        if (AdrResult.Type == ModAReg32)
          AdrResult.Mode = 4;
        if ((AdrResult.Type == ModGen) && (AdrResult.Mode < 6)) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] = 0x7c;
          BAsmCode[1] = 0xc0 + AdrResult.Mode;
          memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
          CodeLen = 2 + AdrResult.Cnt;
        }
      }
    }
    else if (DecodeCReg(pRegArg->str.p_str, &CReg))
    {
      tAdrResult AdrResult;

      SetOpSize(1, NULL);
      if (DecodeAdr(pMemArg, MModGen | (IsSTC ? 0 : MModImm), &AdrResult) == ModImm)
      {
        BAsmCode[0] = 0xeb;
        BAsmCode[1] = CReg << 4;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
      else if (AdrResult.Type == ModGen)
      {
        BAsmCode[0] = 0x7a + IsSTC;
        BAsmCode[1] = 0x80 + (CReg << 4) + AdrResult.Mode;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeLDCTX_STCTX(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    tAdrResult SrcAdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &SrcAdrResult) == ModGen)
    {
      if (SrcAdrResult.Mode != 15) WrError(ErrNum_InvAddrMode);
      else
      {
        tAdrResult DestAdrResult;

        memcpy(BAsmCode + 2, SrcAdrResult.Vals, SrcAdrResult.Cnt);
        if (DecodeAdr(&ArgStr[2], MModAbs20, &DestAdrResult) == ModAbs20)
        {
          memcpy(BAsmCode + 4, DestAdrResult.Vals, DestAdrResult.Cnt);
          BAsmCode[0] = Code;
          BAsmCode[1] = 0xf0;
          CodeLen = 7;
        }
      }
    }
  }
}

static void DecodeLDE_STE(Word IsLDE)
{
  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tStrComp *pArg1 = IsLDE ? &ArgStr[2] : &ArgStr[1],
             *pArg2 = IsLDE ? &ArgStr[1] : &ArgStr[2];
    tAdrResult AdrResult1;

    if (DecodeAdr(pArg1, MModGen, &AdrResult1) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else
      {
        tAdrResult AdrResult2;

        if (DecodeAdr(pArg2, MModAbs20 | MModDisp20 | MModIReg32, &AdrResult2) != ModNone)
        {
          BAsmCode[0] = 0x74 + OpSize;
          BAsmCode[1] = (IsLDE << 7) + AdrResult1.Mode;
          switch (AdrResult2.Type)
          {
            case ModDisp20: BAsmCode[1] += 0x10; break;
            case ModIReg32: BAsmCode[1] += 0x20; break;
          }
          memcpy(BAsmCode + 2, AdrResult1.Vals, AdrResult1.Cnt);
          memcpy(BAsmCode + 2 + AdrResult1.Cnt, AdrResult2.Vals, AdrResult2.Cnt);
          CodeLen = 2 + AdrResult1.Cnt + AdrResult2.Cnt;
        }
      }
    }
  }
}

static void DecodeMOVA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tAdrResult SrcAdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &SrcAdrResult) != ModNone)
    {
      if (SrcAdrResult.Mode < 8) WrError(ErrNum_InvAddrMode);
      else
      {
        tAdrResult DestAdrResult;

        if (DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
        {
          if (DestAdrResult.Mode > 5) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0xeb;
            BAsmCode[1] = (DestAdrResult.Mode << 4) + SrcAdrResult.Mode;
            memcpy(BAsmCode + 2, SrcAdrResult.Vals, SrcAdrResult.Cnt);
            CodeLen = 2 + SrcAdrResult.Cnt;
          }
        }
      }
    }
  }
}

static void DecodeDir(Word Code)
{
  Boolean OK;
  Integer Num1;

  if (OpSize > 0) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckFormat("G"))
  {
    OK = True; Num1 = 0;
    if (!as_strcasecmp(ArgStr[2].str.p_str, "R0L"));
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "R0L")) Num1 = 1;
    else OK = False;
    if (!OK) WrError(ErrNum_InvAddrMode);
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[Num1 + 1], MModGen, &AdrResult) != ModNone)
      {
        if (((AdrResult.Mode & 14) == 4) || ((AdrResult.Mode == 0) && (Num1 == 1))) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] = 0x7c;
          BAsmCode[1] = (Num1 << 7) + (Code << 4) + AdrResult.Mode;
          memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
          CodeLen = 2 + AdrResult.Cnt;
        }
      }
    }
  }
}

static void DecodePUSH_POP(Word IsPOP)
{
  if (ChkArgCnt(1, 1)
   && CheckFormat("GS"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen | (IsPOP ? 0 : MModImm), &AdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if ((AdrResult.Type != ModGen))
            FormatCode = 1;
          else if ((OpSize == 0) && (AdrResult.Mode < 2))
            FormatCode = 2;
          else if ((OpSize == 1) && ((AdrResult.Mode & 14) == 4))
            FormatCode = 2;
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            if (AdrResult.Type == ModImm)
            {
              BAsmCode[0] = 0x7c + OpSize;
              BAsmCode[1] = 0xe2;
            }
            else
            {
              BAsmCode[0] = 0x74 + OpSize;
              BAsmCode[1] = 0x40 + (IsPOP * 0x90) + AdrResult.Mode;
            }
            memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
            CodeLen = 2 + AdrResult.Cnt;
            break;
          case 2:
            if (AdrResult.Type != ModGen) WrError(ErrNum_InvAddrMode);
            else if ((OpSize == 0) && (AdrResult.Mode < 2))
            {
              BAsmCode[0] = 0x82 + (AdrResult.Mode << 3) + (IsPOP << 4);
              CodeLen = 1;
            }
            else if ((OpSize == 1) && ((AdrResult.Mode & 14) == 4))
            {
              BAsmCode[0] = 0xc2 + ((AdrResult.Mode & 1) << 3) + (IsPOP << 4);
              CodeLen = 1;
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
        }
      }
    }
  }
}

static void DecodePUSHC_POPC(Word Code)
{
  Byte CReg;

  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
   if (DecodeCReg(ArgStr[1].str.p_str, &CReg))
   {
     BAsmCode[0] = 0xeb;
     BAsmCode[1] = Code + (CReg << 4);
     CodeLen = 2;
   }
}

static void DecodePUSHM_POPM(Word IsPOPM)
{
  if (ChkArgCnt(1, ArgCntMax))
  {
    int z;
    Byte Reg;
    tSymbolSize DataSize = eSymbolSize16Bit;

    BAsmCode[1] = 0; z = 1;
    for (z = 1; z <= ArgCnt; z++)
    {
      if (!DecodeReg(&ArgStr[z], &Reg, True, &DataSize))
        return;
      BAsmCode[1] |= 1 << (IsPOPM ? Reg : 7 - Reg);
    }
    BAsmCode[0] = 0xec + IsPOPM;
    CodeLen = 2;
  }
}

static void DecodePUSHA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &AdrResult) != ModNone)
    {
      if (AdrResult.Mode < 8) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x7d;
        BAsmCode[1] = 0x90 + AdrResult.Mode;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeXCHG(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tAdrResult SrcAdrResult, DestAdrResult;

    if ((DecodeAdr(&ArgStr[1], MModGen, &SrcAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else if (SrcAdrResult.Mode < 4)
      {
        BAsmCode[0] = 0x7a + OpSize;
        BAsmCode[1] = (SrcAdrResult.Mode << 4) + DestAdrResult.Mode;
        memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
        CodeLen = 2 + DestAdrResult.Cnt;
      }
      else if (DestAdrResult.Mode < 4)
      {
        BAsmCode[0] = 0x7a + OpSize;
        BAsmCode[1] = (DestAdrResult.Mode << 4) + SrcAdrResult.Mode;
        memcpy(BAsmCode + 2, SrcAdrResult.Vals, SrcAdrResult.Cnt);
        CodeLen = 2 + SrcAdrResult.Cnt;
      }
      else
        WrError(ErrNum_InvAddrMode);
    }
  }
}

static void DecodeSTZ_STNZ(Word Code)
{
  Byte SMode;

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tAdrResult DestAdrResult;

    if (OpSize == eSymbolSizeUnknown) OpSize++;
    if (DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
    {
      if (!IsShort(DestAdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
      else
      {
        tAdrResult SrcAdrResult;

        if (DecodeAdr(&ArgStr[1], MModImm, &SrcAdrResult) != ModNone)
        {
          BAsmCode[0] = Code + SMode;
          BAsmCode[1] = SrcAdrResult.Vals[0];
          memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
          CodeLen =2 + DestAdrResult.Cnt;
        }
      }
    }
  }
}

static void DecodeSTZX(Word Code)
{
  Byte SMode;

  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult3;

    if (OpSize == eSymbolSizeUnknown) OpSize++;
    if (DecodeAdr(&ArgStr[3], MModGen, &AdrResult3) != ModNone)
    {
      if (!IsShort(AdrResult3.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
      else
      {
        tAdrResult AdrResult1;

        if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult1) != ModNone)
        {
          tAdrResult AdrResult2;

          if (DecodeAdr(&ArgStr[2], MModImm, &AdrResult2) != ModNone)
          {
            BAsmCode[0] = 0xd8 + SMode;
            BAsmCode[1] = AdrResult1.Vals[0];
            memcpy(BAsmCode + 2, AdrResult3.Vals, AdrResult3.Cnt);
            BAsmCode[2 + AdrResult3.Cnt] = AdrResult2.Vals[0];
            CodeLen = 3 + AdrResult3.Cnt;
          }
        }
      }
    }
  }
}

static void DecodeADD(Word Code)
{
  Integer Num1;
  Byte SMode;
  LongInt AdrLong;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "SP"))
  {
    if (OpSize == eSymbolSizeUnknown) OpSize = 1;
    if (CheckFormat("GQ"))
    {
      tAdrResult SrcAdrResult;

      if (DecodeAdr(&ArgStr[1], MModImm, &SrcAdrResult) != ModNone)
      {
        if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
        else if (OpSize > 1) WrError(ErrNum_InvOpSize);
        else
        {
          AdrLong = ImmVal(&SrcAdrResult);
          if (FormatCode == 0)
          {
            if ((AdrLong >= -8) && (AdrLong <= 7)) FormatCode = 2;
            else FormatCode = 1;
          }
          switch (FormatCode)
          {
            case 1:
              BAsmCode[0] = 0x7c + OpSize;
              BAsmCode[1] = 0xeb;
              memcpy(BAsmCode + 2, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              CodeLen = 2 + SrcAdrResult.Cnt;
              break;
            case 2:
              if (ChkRange(AdrLong, -8, 7))
              {
                BAsmCode[0] = 0x7d;
                BAsmCode[1] = 0xb0 + (AdrLong & 15);
                CodeLen = 2;
              }
              break;
          }
        }
      }
    }
  }
  else if (CheckFormat("GQS"))
  {
    tAdrResult DestAdrResult, SrcAdrResult;

    if ((DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[1], MModImm | MModGen, &SrcAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if (SrcAdrResult.Type == ModImm)
          {
            if ((ImmVal(&SrcAdrResult) >= -8) && (ImmVal(&SrcAdrResult) <= 7))
              FormatCode = 2;
            else if ((IsShort(DestAdrResult.Mode, &SMode)) && (OpSize == 0))
              FormatCode = 3;
            else
              FormatCode = 1;
          }
          else
          {
            if ((OpSize == 0) && (IsShort(SrcAdrResult.Mode, &SMode)) && (DestAdrResult.Mode <= 1) &&
                ((SrcAdrResult.Mode > 1) || (Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))))
              FormatCode = 3;
            else
              FormatCode = 1;
          }
        }
        switch (FormatCode)
        {
          case 1:
            CodeGen(0xa0, 0x76, 0x40, &SrcAdrResult, &DestAdrResult);
            break;
          case 2:
            if (SrcAdrResult.Type != ModImm) WrError(ErrNum_InvAddrMode);
            else
            {
              Num1 = ImmVal(&SrcAdrResult);
              if (ChkRange(Num1, -8, 7))
              {
                BAsmCode[0] = 0xc8 + OpSize;
                BAsmCode[1] = (Num1 << 4) + DestAdrResult.Mode;
                memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
                CodeLen = 2 + DestAdrResult.Cnt;
              }
            }
            break;
          case 3:
            if (OpSize != 0) WrError(ErrNum_InvOpSize);
            else if (!IsShort(DestAdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
            else if (SrcAdrResult.Type == ModImm)
            {
              BAsmCode[0] = 0x80 + SMode;
              BAsmCode[1] = SrcAdrResult.Vals[0];
              memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
              CodeLen = 2 + DestAdrResult.Cnt;
            }
            else if ((DestAdrResult.Mode >= 2) || (!IsShort(SrcAdrResult.Mode, &SMode))) WrError(ErrNum_InvAddrMode);
            else if ((SrcAdrResult.Mode < 2) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
            else
            {
              if (SMode == 3) SMode++;
              BAsmCode[0] = 0x20 + ((DestAdrResult.Mode & 1) << 2) + (SMode & 3);    /* RMS 05: Just like #04 */
              memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              CodeLen = 1 + SrcAdrResult.Cnt;
            }
            break;
        }
      }
    }
  }
}

static void DecodeCMP(Word Code)
{
  Byte SMode;
  Integer Num1;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckFormat("GQS"))
  {
    tAdrResult DestAdrResult, SrcAdrResult;

    if ((DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[1], MModImm | MModGen, &SrcAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if (SrcAdrResult.Type == ModImm)
          {
            if ((ImmVal(&SrcAdrResult) >= -8) && (ImmVal(&SrcAdrResult) <= 7))
              FormatCode = 2;
            else if ((IsShort(DestAdrResult.Mode, &SMode)) && (OpSize == 0))
              FormatCode = 3;
            else
              FormatCode = 1;
          }
          else
          {
            if ((OpSize == 0) && (IsShort(SrcAdrResult.Mode, &SMode)) && (DestAdrResult.Mode <= 1) &&
                ((SrcAdrResult.Mode > 1) || (Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))))
              FormatCode = 3;
            else
              FormatCode = 1;
          }
        }
        switch (FormatCode)
        {
          case 1:
            CodeGen(0xc0, 0x76, 0x80, &SrcAdrResult, &DestAdrResult);
            break;
          case 2:
            if (SrcAdrResult.Type != ModImm) WrError(ErrNum_InvAddrMode);
            else
            {
              Num1 = ImmVal(&SrcAdrResult);
              if (ChkRange(Num1, -8, 7))
              {
                BAsmCode[0] = 0xd0 + OpSize;
                BAsmCode[1] = (Num1 << 4) + DestAdrResult.Mode;
                memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
                CodeLen = 2 + DestAdrResult.Cnt;
              }
              /* else? */
            }
            break;
          case 3:
            if (OpSize != 0) WrError(ErrNum_InvOpSize);
            else if (!IsShort(DestAdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
            else if (SrcAdrResult.Type == ModImm)
            {
              BAsmCode[0] = 0xe0 + SMode;
              BAsmCode[1] = SrcAdrResult.Vals[0];
              memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
              CodeLen = 2 + DestAdrResult.Cnt;
            }
            else if ((DestAdrResult.Mode >= 2) || (!IsShort(SrcAdrResult.Mode, &SMode))) WrError(ErrNum_InvAddrMode);
            else if ((SrcAdrResult.Mode < 2) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
            else
            {
              if (SMode == 3) SMode++;
              BAsmCode[0] = 0x38 + ((DestAdrResult.Mode & 1) << 2) + (SMode & 3); /* RMS 04: destination reg is bit 2! */
              memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              CodeLen = 1 + SrcAdrResult.Cnt;
            }
            break;
        }
      }
    }
  }
}

static void DecodeSUB(Word Code)
{
  Byte SMode;
  Integer Num1;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckFormat("GQS"))
  {
    tAdrResult DestAdrResult, SrcAdrResult;

    if ((DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[1], MModImm | MModGen, &SrcAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if (SrcAdrResult.Type == ModImm)
          {
            if ((ImmVal(&SrcAdrResult) >= -7) && (ImmVal(&SrcAdrResult) <= 8))
              FormatCode = 2;
            else if ((IsShort(DestAdrResult.Mode, &SMode)) && (OpSize == 0))
              FormatCode = 3;
            else
              FormatCode = 1;
          }
          else
          {
            if ((OpSize == 0) && (IsShort(SrcAdrResult.Mode, &SMode)) & (DestAdrResult.Mode <= 1) &&
                ((SrcAdrResult.Mode > 1) || (Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))))
              FormatCode = 3;
            else
              FormatCode = 1;
          }
        }
        switch (FormatCode)
        {
          case 1:
            CodeGen(0xa8, 0x76, 0x50, &SrcAdrResult, &DestAdrResult);
            break;
          case 2:
            if (SrcAdrResult.Type != ModImm) WrError(ErrNum_InvAddrMode);
            else
            {
              Num1 = ImmVal(&SrcAdrResult);
              if (ChkRange(Num1, -7, 8))
              {
                BAsmCode[0] = 0xc8 + OpSize;
                BAsmCode[1] = ((-Num1) << 4) + DestAdrResult.Mode;
                memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
                CodeLen = 2 + DestAdrResult.Cnt;
              }
            }
            break;
          case 3:
            if (OpSize != 0) WrError(ErrNum_InvOpSize);
            else if (!IsShort(DestAdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
            else if (SrcAdrResult.Type == ModImm)
            {
              BAsmCode[0] = 0x88 + SMode;
              BAsmCode[1] = SrcAdrResult.Vals[0];
              memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
              CodeLen = 2 + DestAdrResult.Cnt;
            }
            else if ((DestAdrResult.Mode >= 2) || (!IsShort(SrcAdrResult.Mode, &SMode))) WrError(ErrNum_InvAddrMode);
            else if ((SrcAdrResult.Mode < 2) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
            else
            {
              if (SMode == 3) SMode++;
              BAsmCode[0] = 0x28 + ((DestAdrResult.Mode & 1) << 2) + (SMode & 3);    /* RMS 06: just like RMS 04 */
              memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              CodeLen = 1 + SrcAdrResult.Cnt;
            }
            break;
        }
      }
    }
  }
}

static void DecodeGen1(Word Code)
{
  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &AdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else
      {
        BAsmCode[0] = Hi(Code) + OpSize;
        BAsmCode[1] = Lo(Code) + AdrResult.Mode;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeGen2(Word Index)
{
  Gen2Order *pOrder = Gen2Orders + Index;

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tAdrResult DestAdrResult, SrcAdrResult;

    if ((DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[1], MModGen | MModImm, &SrcAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else if ((*OpPart.str.p_str == 'M') && ((DestAdrResult.Mode == 3) || (DestAdrResult.Mode == 5) || (DestAdrResult.Mode - OpSize == 1))) WrError(ErrNum_InvAddrMode);
      else
        CodeGen(pOrder->Code1, pOrder->Code2, pOrder->Code3, &SrcAdrResult, &DestAdrResult);
    }
  }
}

static void DecodeINC_DEC(Word IsDEC)
{
  Byte SMode;

  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &AdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize == 1) && ((AdrResult.Mode & 14) == 4))
      {
        BAsmCode[0] = 0xb2 + (IsDEC << 6) + ((AdrResult.Mode & 1) << 3);
        CodeLen = 1;
      }
      else if (!IsShort(AdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
      else if (OpSize != 0) WrError(ErrNum_InvOpSize);
      else
      {
        BAsmCode[0] = 0xa0 + (IsDEC << 3) + SMode;
        memcpy(BAsmCode + 1, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 1 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeDiv(Word Index)
{
  Gen2Order *pOrder = DivOrders + Index;

  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModImm | MModGen, &AdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 0) && (OpSize != 1)) WrError(ErrNum_InvOpSize);
      else if (AdrResult.Type == ModImm)
      {
        BAsmCode[0] = 0x7c + OpSize;
        BAsmCode[1] = pOrder->Code1;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
      else
      {
        BAsmCode[0] = pOrder->Code2 + OpSize;
        BAsmCode[1] = pOrder->Code3 + AdrResult.Mode;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeBCD(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestAdrResult;

    if (DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
    {
      if (DestAdrResult.Mode != 0) WrError(ErrNum_InvAddrMode);
      else
      {
        tAdrResult SrcAdrResult;

        if (DecodeAdr(&ArgStr[1], MModGen | MModImm, &SrcAdrResult) != ModNone)
        {
          if (SrcAdrResult.Type == ModImm)
          {
            BAsmCode[0] = 0x7c + OpSize;
            BAsmCode[1] = 0xec + Code;
            memcpy(BAsmCode + 2, SrcAdrResult.Vals, SrcAdrResult.Cnt);
            CodeLen = 2 + SrcAdrResult.Cnt;
          }
          else if (SrcAdrResult.Mode != 1) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0x7c + OpSize;
            BAsmCode[1] = 0xe4 + Code;
            CodeLen = 2;
          }
        }
      }
    }
  }
}

static void DecodeEXTS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &AdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) OpSize = 0;
      if (OpSize == 0)
      {
        if ((AdrResult.Mode == 1) || ((AdrResult.Mode >= 3) && (AdrResult.Mode <= 5))) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] = 0x7c;
          BAsmCode[1] = 0x60 + AdrResult.Mode;
          memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
          CodeLen = 2 + AdrResult.Cnt;
        }
      }
      else if (OpSize == 1)
      {
        if (AdrResult.Mode != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] = 0x7c;
          BAsmCode[1] = 0xf3;
          CodeLen = 2;
        }
      }
      else WrError(ErrNum_InvOpSize);
    }
  }
}

static void DecodeNOT(Word Code)
{
  Byte SMode;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && CheckFormat("GS"))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModGen, &AdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if ((OpSize == 0) && (IsShort(AdrResult.Mode, &SMode)))
            FormatCode = 2;
          else
            FormatCode = 1;
        }
        switch (FormatCode)
        {
          case 1:
            BAsmCode[0] = 0x74 + OpSize;
            BAsmCode[1] = 0x70 + AdrResult.Mode;
            memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
            CodeLen = 2 + AdrResult.Cnt;
            break;
          case 2:
            if (OpSize != 0) WrError(ErrNum_InvOpSize);
            else if (!IsShort(AdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xb8 + SMode;
              memcpy(BAsmCode + 1, AdrResult.Vals, AdrResult.Cnt);
              CodeLen = 1 + AdrResult.Cnt;
            }
            break;
        }
      }
    }
  }
}

static void DecodeAND_OR(Word IsOR)
{
  Byte SMode;

  if (ChkArgCnt(2, 2)
   && CheckFormat("GS"))        /* RMS 01: The format codes are G and S, not G and Q */
  {
    tAdrResult DestAdrResult, SrcAdrResult;

    if ((DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
     && (DecodeAdr(&ArgStr[1], MModGen | MModImm, &SrcAdrResult) != ModNone))
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else
      {
        if (FormatCode == 0)
        {
          if (SrcAdrResult.Type == ModImm)
          {
            if ((OpSize == 0) && (IsShort(DestAdrResult.Mode, &SMode)))
              FormatCode = 2;
            else
              FormatCode = 1;
          }
          else
          {
            if ((DestAdrResult.Mode <= 1) && (IsShort(SrcAdrResult.Mode, &SMode)) && ((SrcAdrResult.Mode > 1) || Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode)))
              FormatCode = 2;
            else
              FormatCode = 1;
          }
        }
        switch (FormatCode)
        {
          case 1:
            CodeGen(0x90 + (IsOR << 3), 0x76, 0x20 + (IsOR << 4), &SrcAdrResult, &DestAdrResult);
            break;
          case 2:
            if (OpSize != 0) WrError(ErrNum_InvOpSize);
            else if (SrcAdrResult.Type == ModImm)
            if (!IsShort(DestAdrResult.Mode, &SMode)) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x90 + (IsOR << 3) + SMode;
              BAsmCode[1] = ImmVal(&SrcAdrResult);
              memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
              CodeLen = 2 + DestAdrResult.Cnt;
            }
            else if ((!IsShort(SrcAdrResult.Mode, &SMode)) || (DestAdrResult.Mode > 1)) WrError(ErrNum_InvAddrMode);
            else if ((SrcAdrResult.Mode <= 1) && (!Odd(SrcAdrResult.Mode ^ DestAdrResult.Mode))) WrError(ErrNum_InvAddrMode);
            else
            {
              if (SMode == 3) SMode++;
              BAsmCode[0] = 0x10 + (IsOR << 3) + ((DestAdrResult.Mode & 1) << 2) + (SMode & 3);
              memcpy(BAsmCode + 1, SrcAdrResult.Vals, SrcAdrResult.Cnt);
              CodeLen = 1 + SrcAdrResult.Cnt;
            }
            break;
        }
      }
    }
  }
}

static void DecodeROT(Word Code)
{
  ShortInt OpSize2;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tAdrResult DestAdrResult;

    if (DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else
      {
        tAdrResult SrcAdrResult;

        OpSize2 = OpSize;
        OpSize = 0;
        if (DecodeAdr(&ArgStr[1], MModGen | MModImm, &SrcAdrResult) == ModGen)
        {
          if (SrcAdrResult.Mode != 3) WrError(ErrNum_InvAddrMode);
          else if (DestAdrResult.Mode + 2 * OpSize2 == 3) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0x74 + OpSize2;
            BAsmCode[1] = 0x60 + DestAdrResult.Mode;
            memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
            CodeLen = 2 + DestAdrResult.Cnt;
          }
        }
        else if (SrcAdrResult.Type == ModImm)
        {
          Integer Num1 = ImmVal(&SrcAdrResult);
          if (Num1 == 0) WrError(ErrNum_UnderRange);
          else if (ChkRange(Num1, -8, 8))
          {
            Num1 = (Num1 > 0) ? (Num1 - 1) : -9 -Num1;
            BAsmCode[0] = 0xe0 + OpSize2;
            BAsmCode[1] = (Num1 << 4) + DestAdrResult.Mode;
            memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
            CodeLen = 2 + DestAdrResult.Cnt;
          }
        }
      }
    }
  }
}

static void DecodeSHA_SHL(Word IsSHA)
{
  ShortInt OpSize2;

  if (ChkArgCnt(2, 2)
   && CheckFormat("G"))
  {
    tAdrResult DestAdrResult;

    if (DecodeAdr(&ArgStr[2], MModGen | MModReg32, &DestAdrResult) != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize > 2) || ((OpSize == 2) && (DestAdrResult.Type == ModGen))) WrError(ErrNum_InvOpSize);
      else
      {
        tAdrResult SrcAdrResult;

        OpSize2 = OpSize; OpSize = 0;
        if (DecodeAdr(&ArgStr[1], MModImm | MModGen, &SrcAdrResult) == ModGen)
        {
          if (SrcAdrResult.Mode != 3) WrError(ErrNum_InvAddrMode);
          else if (DestAdrResult.Mode + 2 * OpSize2 == 3) WrError(ErrNum_InvAddrMode);
          else
          {
            if (OpSize2 == 2)
            {
              BAsmCode[0] = 0xeb;
              BAsmCode[1] = 0x01 | (DestAdrResult.Mode << 4) | (IsSHA << 5);
            }
            else
            {
              BAsmCode[0] = 0x74 | OpSize2;
              BAsmCode[1] = 0xe0 | (IsSHA << 4) | DestAdrResult.Mode;
            }
            memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
            CodeLen = 2 + DestAdrResult.Cnt;
          }
        }
        else if (SrcAdrResult.Type == ModImm)
        {
          Integer Num1 = ImmVal(&SrcAdrResult);
          if (Num1 == 0) WrError(ErrNum_UnderRange);
          else if (ChkRange(Num1, -8, 8))
          {
            if (Num1 > 0) Num1--; else Num1 = (-9) - Num1;
            if (OpSize2 == 2)
            {
              BAsmCode[0] = 0xeb;
              BAsmCode[1] = 0x80 | (DestAdrResult.Mode << 4) | (IsSHA << 5) | (Num1 & 15);
            }
            else
            {
              BAsmCode[0] = (0xe8 + (IsSHA << 3)) | OpSize2;
              BAsmCode[1] = (Num1 << 4) | DestAdrResult.Mode;
            }
            memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
            CodeLen = 2 + DestAdrResult.Cnt;
          }
        }
      }
    }
  }
}

static void DecodeBit(Word Code)
{
  Boolean MayShort = (Code & 12) == 8;

  if (CheckFormat(MayShort ? "GS" : "G"))
  {
    tAdrResult AdrResult;

    if (DecodeBitAdr((FormatCode != 1) && MayShort, &AdrResult))
    {
      if (AdrResult.Mode >= 16)
      {
        BAsmCode[0] = 0x40 + ((Code - 8) << 3) + (AdrResult.Mode & 7);
        BAsmCode[1] = AdrResult.Vals[0];
        CodeLen = 2;
      }
      else
      {
        BAsmCode[0] = 0x7e;
        BAsmCode[1] = (Code << 4) + AdrResult.Mode;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeFCLR_FSET(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (strlen(ArgStr[1].str.p_str) != 1) WrError(ErrNum_InvAddrMode);
  else
  {
    const char *p = strchr(Flags, as_toupper(*ArgStr[1].str.p_str));
    if (!p) WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
    else
    {
      BAsmCode[0] = 0xeb;
      BAsmCode[1] = Code + ((p - Flags) << 4);
      CodeLen = 2;
    }
  }
}

static void DecodeJMP(Word Code)
{
  LongInt AdrLong, Diff;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt20, &OK, &Flags);
    Diff = AdrLong - EProgCounter();

    /* RMS 12: Repaired JMP.S forward-label as follows:

               If it's an unknown symbol, make PC+1 the "safe" value, otherwise
               the user will get OUT OF RANGE errors on every attempt to use JMP.S
               Since the instruction can only branch forward, and AS stuffs the PC
               back for a "temp" forward reference value, the range-checking will
               always fail.

               One side-effect also is that for auto-determination purposes, one
               fewer pass is needed.  Before, the first pass would signal JMP.B,
               then once the forward reference is known, it'd signal JMP.S, which
               would cause a "phase error" forcing another pass.
    */

    if (mFirstPassUnknown(Flags) && (Diff == 0))
      Diff = 1;

    if (OpSize == eSymbolSizeUnknown)
    {                                                        /* RMS 13: The other part of the */
      if ((Diff >= 1) && (Diff <= 9))                        /* "looping phase error" fix  */
        OpSize = 4;
      else if ((Diff >= -127) && (Diff <= 128))
        OpSize = 0;
      else if ((Diff >= -32767) && (Diff <= 32768))
        OpSize = 1;
      else
        OpSize = 7;
    }
    /*
       The following code is to deal with a silicon bug in the first generation of
       M16C CPUs (the so-called M16C/60 group).  It has been observed that this
       silicon bug has been fixed as of the M16C/61, so we disable JMP.S promotion
       to JMP.B when the target crosses a 64k boundary for those CPUs.

       Since M16C is a "generic" specification, we do JMP.S promotion for that
       CPU specification, as follows:

         RMS 11: According to Mitsubishi App Note M16C-06-9612
         JMP.S cannot cross a 64k boundary.. so trim up to JMP.B

       It is admittedly a very low likelihood of occurrence [JMP.S has only 8
       possible targets, being a 3 bit "jump addressing mode"], but since the
       occurrence of this bug could cause such evil debugging issues, I have
       taken the liberty of addressing it in the assembler.  Heck, it's JUST one
       extra byte.  One byte's worth the peace of mind, isn't it? :)
    */
    if ((MomCPU == CPUM16C) || (MomCPU == CPUM30600M8))
    {
      if (OpSize == 4)
      {
        if ( (AdrLong & 0x0f0000) != (((int)EProgCounter()) & 0x0f0000) )
          OpSize = 0;
      }            /* NOTE! This not an ASX bug, but rather in the CPU!! */
    }
    switch (OpSize)
    {
      case 4:
        if (((Diff < 1) || (Diff > 9)) && !mSymbolQuestionable(Flags) ) WrError(ErrNum_JmpDistTooBig);
        else
        {
          if (Diff == 1)
          {                                   /* RMS 13: To avoid infinite loop phase errors... Turn a */
            BAsmCode[0] = 0x04;               /* JMP (or JMP.S) to following instruction into a NOP */
          }
          else
          {
            BAsmCode[0] = 0x60 + ((Diff - 2) & 7);
          }
          CodeLen = 1;
        }
        break;
      case 0:
        if (((Diff < -127) || (Diff > 128)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0xfe;
          BAsmCode[1] = (Diff - 1) & 0xff;
          CodeLen = 2;
        }
        break;
      case 1:
        if (((Diff< -32767) || (Diff > 32768)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0xf4;
          Diff--;
          BAsmCode[1] = Diff & 0xff;
          BAsmCode[2] = (Diff >> 8) & 0xff;
          CodeLen = 3;
        }
        break;
      case 7:
        BAsmCode[0] = 0xfc;
        BAsmCode[1] = AdrLong & 0xff;
        BAsmCode[2] = (AdrLong >> 8) & 0xff;
        BAsmCode[3] = (AdrLong >> 16) & 0xff;
        CodeLen = 4;
        break;
      default:
        WrError(ErrNum_InvOpSize);
    }
  }
}

static void DecodeJSR(Word Code)
{
  LongInt AdrLong, Diff;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt20, &OK, &Flags);
    Diff = AdrLong-EProgCounter();
    if (OpSize == eSymbolSizeUnknown)
      OpSize = ((Diff >= -32767) && (Diff <= 32768)) ? 1 : 7;
    switch (OpSize)
    {
      case 1:
        if (((Diff <- 32767) || (Diff > 32768)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0xf5;
          Diff--;
          BAsmCode[1] = Diff & 0xff;
          BAsmCode[2] = (Diff >> 8) & 0xff;
          CodeLen = 3;
        }
        break;
      case 7:
        BAsmCode[0] = 0xfd;
        BAsmCode[1] = AdrLong & 0xff;
        BAsmCode[2] = (AdrLong >> 8) & 0xff;
        BAsmCode[3] = (AdrLong >> 16) & 0xff;
        CodeLen = 4;
        break;
      default:
        WrError(ErrNum_InvOpSize);
    }
  }
}

static void DecodeJMPI_JSRI(Word Code)
{
  if (ChkArgCnt(1, 1)
   && CheckFormat("G"))
  {
    tAdrResult AdrResult;

    if (OpSize == 7)
      OpSize = 2;
    DecodeAdr(&ArgStr[1], MModGen | MModDisp20 | MModReg32| MModAReg32, &AdrResult);
    if ((AdrResult.Type == ModGen) && ((AdrResult.Mode & 14) == 12))
      AdrResult.Vals[AdrResult.Cnt++] = 0;
    if ((AdrResult.Type == ModGen) && ((AdrResult.Mode & 14) == 4))
    {
      if (OpSize == eSymbolSizeUnknown) OpSize = 1;
      else if (OpSize != 1)
      {
        AdrResult.Type = ModNone; WrError(ErrNum_ConfOpSizes);
      }
    }
    if (AdrResult.Type == ModAReg32) AdrResult.Mode = 4;
    if (AdrResult.Type != ModNone)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != 1) && (OpSize != 2)) WrError(ErrNum_InvOpSize);
      else
      {
        BAsmCode[0] = 0x7d;
        BAsmCode[1] = Code + (Ord(OpSize == 1) << 5) + AdrResult.Mode;
        memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeJMPS_JSRS(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    OpSize = 0;
    if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult) != ModNone)
    {
      if (mFirstPassUnknown(AdrResult.ImmSymFlags) && (AdrResult.Vals[0] < 18))
        AdrResult.Vals[0] = 18;
      if (AdrResult.Vals[0] < 18) WrError(ErrNum_UnderRange);
      else
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrResult.Vals[0];
        CodeLen = 2;
      }
    }
  }
}

static void DecodeADJNZ_SBJNZ(Word IsSBJNZ)
{
  if (ChkArgCnt(3, 3)
   && CheckFormat("G"))
  {
    tAdrResult DestAdrResult;

    if (DecodeAdr(&ArgStr[2], MModGen, &DestAdrResult) != ModNone)
    {
      ShortInt OpSize2;

      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if (OpSize > 1) WrError(ErrNum_InvOpSize);
      else
      {
        Integer Num1;
        tAdrResult SrcAdrResult;

        OpSize2 = OpSize;
        OpSize = 0;
        DecodeAdr(&ArgStr[1], MModImm, &SrcAdrResult);
        /* TODO: ChkRes */
        Num1 = ImmVal(&DestAdrResult);
        if (mFirstPassUnknown(SrcAdrResult.ImmSymFlags))
          Num1 = 0;
        if (IsSBJNZ)
          Num1 = -Num1;
        if (ChkRange(Num1, -8, 7))
        {
          LongInt AdrLong;
          Boolean OK;
          tSymbolFlags Flags;

          AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt20, &OK, &Flags) - (EProgCounter() + 2);
          if (OK)
          {
            if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
            else
            {
              BAsmCode[0] = 0xf8 + OpSize2;
              BAsmCode[1] = (Num1 << 4) + DestAdrResult.Mode;
              memcpy(BAsmCode + 2, DestAdrResult.Vals, DestAdrResult.Cnt);
              BAsmCode[2 + DestAdrResult.Cnt] = AdrLong & 0xff;
              CodeLen = 3 + DestAdrResult.Cnt;
            }
          }
        }
      }
    }
  }
}

static void DecodeINT(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[1] = 0xc0 + EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt6, &OK);
    if (OK)
    {
      BAsmCode[0] = 0xeb;
      CodeLen = 2;
    }
  }
}

static void DecodeBM(Word Code)
{
  tAdrResult AdrResult;

  if ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "C")))
  {
    BAsmCode[0] = 0x7d;
    BAsmCode[1] = 0xd0 + Code;
    CodeLen = 2;
  }
  else if (DecodeBitAdr(False, &AdrResult))
  {
    BAsmCode[0] = 0x7e;
    BAsmCode[1] = 0x20 + AdrResult.Mode;
    memcpy(BAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
    if ((Code >= 4) && (Code < 12))
      Code ^= 12;
    if (Code >= 8)
       Code += 0xf0;
    BAsmCode[2 + AdrResult.Cnt] = Code;
    CodeLen = 3 + AdrResult.Cnt;
  }
}

static void DecodeJ(Word Code)
{
  Integer Num1 = 1 + Ord(Code >= 8);
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (strcmp(Format, " ")) WrError(ErrNum_InvFormat);
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt20, &OK, &Flags) - (EProgCounter() + Num1);

    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrLong >127) || (AdrLong < -128))) WrError(ErrNum_JmpDistTooBig);
      else if (Code >= 8)
      {
        BAsmCode[0] = 0x7d;
        BAsmCode[1] = 0xc0 + Code;
        BAsmCode[2] = AdrLong & 0xff;
        CodeLen = 3;
      }
      else
      {
        BAsmCode[0] = 0x68 + Code;
        BAsmCode[1] = AdrLong & 0xff;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeENTER(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[2] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x7c;
      BAsmCode[1] = 0xf2;
      CodeLen = 3;
    }
  }
}

static void DecodeLDINTB(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;
    LongInt AdrLong = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt20, &OK);
    if (OK)
    {
      BAsmCode[0] = 0xeb;
      BAsmCode[1] = 0x20;
      BAsmCode[2] = (AdrLong >> 16) & 0xff;
      BAsmCode[3] = 0;
      BAsmCode[4] = 0xeb;
      BAsmCode[5] = 0x10;
      BAsmCode[7] = (AdrLong >> 8) & 0xff;
      BAsmCode[6] = AdrLong & 0xff; /* RMS 07: needs to be LSB, MSB order */
      CodeLen = 8;
    }
  }
}

static void DecodeLDIPL(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[1] = 0xa0 + EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt3, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x7d;
      CodeLen = 2;
    }
  }
}

/*------------------------------------------------------------------------*/
/* code table handling */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddString(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeString);
}

static void AddGen1(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeGen1);
}

static void AddGen2(const char *NName, Byte NCode1, Byte NCode2, Byte NCode3)
{
  order_array_rsv_end(Gen2Orders, Gen2Order);
  Gen2Orders[InstrZ].Code1 = NCode1;
  Gen2Orders[InstrZ].Code2 = NCode2;
  Gen2Orders[InstrZ].Code3 = NCode3;
  AddInstTable(InstTable, NName, InstrZ++, DecodeGen2);
}

static void AddDiv(const char *NName, Byte NCode1, Byte NCode2, Byte NCode3)
{
  order_array_rsv_end(DivOrders, Gen2Order);
  DivOrders[InstrZ].Code1 = NCode1;
  DivOrders[InstrZ].Code2 = NCode2;
  DivOrders[InstrZ].Code3 = NCode3;
  AddInstTable(InstTable, NName, InstrZ++, DecodeDiv);
}

static void AddCondition(const char *BMName, const char *JName, Word NCode)
{
  AddInstTable(InstTable, BMName, NCode, DecodeBM);
  AddInstTable(InstTable, JName, NCode, DecodeJ);
}

static void AddBCD(const char *NName)
{
  AddInstTable(InstTable, NName, InstrZ++, DecodeBCD);
}

static void AddDir(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeDir);
}

static void AddBit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(403);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "LDC", 0, DecodeLDC_STC);
  AddInstTable(InstTable, "STC", 1, DecodeLDC_STC);
  AddInstTable(InstTable, "LDCTX", 0x7c, DecodeLDCTX_STCTX);
  AddInstTable(InstTable, "STCTX", 0x7d, DecodeLDCTX_STCTX);
  AddInstTable(InstTable, "LDE", 1, DecodeLDE_STE);
  AddInstTable(InstTable, "STE", 0, DecodeLDE_STE);
  AddInstTable(InstTable, "MOVA", 0, DecodeMOVA);
  AddInstTable(InstTable, "POP", 1, DecodePUSH_POP);
  AddInstTable(InstTable, "PUSH", 0, DecodePUSH_POP);
  AddInstTable(InstTable, "POPC", 0x03, DecodePUSHC_POPC);
  AddInstTable(InstTable, "PUSHC", 0x02, DecodePUSHC_POPC);
  AddInstTable(InstTable, "POPM", 1, DecodePUSHM_POPM);
  AddInstTable(InstTable, "PUSHM", 0, DecodePUSHM_POPM);
  AddInstTable(InstTable, "PUSHA", 0, DecodePUSHA);
  AddInstTable(InstTable, "XCHG", 0, DecodeXCHG);
  AddInstTable(InstTable, "STZ", 0xc8, DecodeSTZ_STNZ);
  AddInstTable(InstTable, "STNZ", 0xd0, DecodeSTZ_STNZ);
  AddInstTable(InstTable, "STZX", 0, DecodeSTZX);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "CMP", 0, DecodeCMP);
  AddInstTable(InstTable, "SUB", 0, DecodeSUB);
  AddInstTable(InstTable, "INC", 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 1, DecodeINC_DEC);
  AddInstTable(InstTable, "EXTS", 0, DecodeEXTS);
  AddInstTable(InstTable, "NOT", 0, DecodeNOT);
  AddInstTable(InstTable, "AND", 0, DecodeAND_OR);
  AddInstTable(InstTable, "OR", 1, DecodeAND_OR);
  AddInstTable(InstTable, "ROT", 0, DecodeROT);
  AddInstTable(InstTable, "SHA", 1, DecodeSHA_SHL);
  AddInstTable(InstTable, "SHL", 0, DecodeSHA_SHL);
  AddInstTable(InstTable, "FCLR", 0x05, DecodeFCLR_FSET);
  AddInstTable(InstTable, "FSET", 0x04, DecodeFCLR_FSET);
  AddInstTable(InstTable, "JMP", 0, DecodeJMP);
  AddInstTable(InstTable, "JSR", 0, DecodeJSR);
  AddInstTable(InstTable, "JMPI", 0x00, DecodeJMPI_JSRI);
  AddInstTable(InstTable, "JSRI", 0x10, DecodeJMPI_JSRI);
  AddInstTable(InstTable, "JMPS", 0xee, DecodeJMPS_JSRS);
  AddInstTable(InstTable, "JSRS", 0xef, DecodeJMPS_JSRS);
  AddInstTable(InstTable, "ADJNZ", 0, DecodeADJNZ_SBJNZ);
  AddInstTable(InstTable, "SBJNZ", 1, DecodeADJNZ_SBJNZ);
  AddInstTable(InstTable, "INT", 0, DecodeINT);
  AddInstTable(InstTable, "ENTER", 0, DecodeENTER);
  AddInstTable(InstTable, "LDINTB", 0, DecodeLDINTB);
  AddInstTable(InstTable, "LDIPL", 0, DecodeLDIPL);

  Format = (char*)malloc(sizeof(Char) * STRINGSIZE);

  AddFixed("BRK"   , 0x0000);
  AddFixed("EXITD" , 0x7df2);
  AddFixed("INTO"  , 0x00f6);
  AddFixed("NOP"   , 0x0004);
  AddFixed("REIT"  , 0x00fb);
  AddFixed("RTS"   , 0x00f3);
  AddFixed("UND"   , 0x00ff);
  AddFixed("WAIT"  , 0x7df3);

  AddString("RMPA" , 0x7cf1);
  AddString("SMOVB", 0x7ce9);
  AddString("SMOVF", 0x7ce8);
  AddString("SSTR" , 0x7cea);

  AddGen1("ABS" , 0x76f0);
  AddGen1("ADCF", 0x76e0);
  AddGen1("NEG" , 0x7450);
  AddGen1("ROLC", 0x76a0);
  AddGen1("RORC", 0x76b0);

  InstrZ = 0;
  AddGen2("ADC" , 0xb0, 0x76, 0x60);
  AddGen2("SBB" , 0xb8, 0x76, 0x70);
  AddGen2("TST" , 0x80, 0x76, 0x00);
  AddGen2("XOR" , 0x88, 0x76, 0x10);
  AddGen2("MUL" , 0x78, 0x7c, 0x50);
  AddGen2("MULU", 0x70, 0x7c, 0x40);

  InstrZ = 0;
  AddDiv("DIV" ,0xe1,0x76,0xd0);
  AddDiv("DIVU",0xe0,0x76,0xc0);
  AddDiv("DIVX",0xe3,0x76,0x90);

  AddCondition("BMGEU", "JGEU",  0); AddCondition("BMC"  , "JC"  ,  0);
  AddCondition("BMGTU", "JGTU",  1); AddCondition("BMEQ" , "JEQ" ,  2);
  AddCondition("BMZ"  , "JZ"  ,  2); AddCondition("BMN"  , "JN"  ,  3);
  AddCondition("BMLTU", "JLTU",  4); AddCondition("BMNC" , "JNC" ,  4);
  AddCondition("BMLEU", "JLEU",  5); AddCondition("BMNE" , "JNE" ,  6);
  AddCondition("BMNZ" , "JNZ" ,  6); AddCondition("BMPZ" , "JPZ" ,  7);
  AddCondition("BMLE" , "JLE" ,  8); AddCondition("BMO"  , "JO"  ,  9);
  AddCondition("BMGE" , "JGE" , 10); AddCondition("BMGT" , "JGT" , 12);
  AddCondition("BMNO" , "JNO" , 13); AddCondition("BMLT" , "JLT" , 14);

  InstrZ = 0;
  AddBCD("DADD"); AddBCD("DSUB"); AddBCD("DADC"); AddBCD("DSBB");

  InstrZ = 0;
  AddDir("MOVLL", InstrZ++);
  AddDir("MOVHL", InstrZ++);
  AddDir("MOVLH", InstrZ++);
  AddDir("MOVHH", InstrZ++);

  AddBit("BAND"  , 4); AddBit("BNAND" , 5);
  AddBit("BNOR"  , 7); AddBit("BNTST" , 3);
  AddBit("BNXOR" ,13); AddBit("BOR"   , 6);
  AddBit("BTSTC" , 0); AddBit("BTSTS" , 1);
  AddBit("BXOR"  ,12); AddBit("BCLR"  , 8);
  AddBit("BNOT"  ,10); AddBit("BSET"  , 9);
  AddBit("BTST"  ,11);

  AddInstTable(InstTable, "REG", 0, CodeREG);
}

static void DeinitFields(void)
{
  free(Format);
  order_array_free(Gen2Orders);
  order_array_free(DivOrders);

  DestroyInstTable(InstTable);
}

/*------------------------------------------------------------------------*/

static Boolean DecodeAttrPart_M16C(void)
{
  char *p;

  switch (AttrSplit)
  {
    case '.':
      p = strchr(AttrPart.str.p_str, ':');
      if (p)
      {
        if (p < AttrPart.str.p_str + strlen(AttrPart.str.p_str) - 1)
          strmaxcpy(Format, p + 1, STRINGSIZE - 1);
        *p = '\0';
      }
      else
        strcpy(Format,  " ");
      break;
    case ':':
      p = strchr(AttrPart.str.p_str, '.');
      if (!p)
      {
        strmaxcpy(Format, AttrPart.str.p_str, STRINGSIZE - 1);
        *AttrPart.str.p_str = '\0';
      }
      else
      {
        *p = '\0';
        strmaxcpy(Format, (p == AttrPart.str.p_str) ? " " : AttrPart.str.p_str, STRINGSIZE - 1);
      }
      break;
    default:
      strcpy(Format, " ");
  }
  NLS_UpString(Format);

  /* Attribut abarbeiten */

  switch (as_toupper(*AttrPart.str.p_str))
  {
    case '\0': AttrPartOpSize[0] = eSymbolSizeUnknown; break;
    case 'B': AttrPartOpSize[0] = eSymbolSize8Bit; break;
    case 'W': AttrPartOpSize[0] = eSymbolSize16Bit; break;
    case 'L': AttrPartOpSize[0] = eSymbolSize32Bit; break;
    case 'Q': AttrPartOpSize[0] = eSymbolSize64Bit; break;
    case 'S': AttrPartOpSize[0] = eSymbolSize80Bit; break;
    case 'D': AttrPartOpSize[0] = eSymbolSizeFloat32Bit; break;
    case 'X': AttrPartOpSize[0] = eSymbolSizeFloat64Bit; break;
    case 'A': AttrPartOpSize[0] = eSymbolSizeFloat96Bit; break;
    default:
      WrStrErrorPos(ErrNum_UndefAttr, &AttrPart); return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_M16C(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on M16C
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_M16C(char *pArg, TempResult *pResult)
{
  Byte Erg;
  tSymbolSize Size;

  if (DecodeRegCore(pArg, &Erg, &Size))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = Size;
    pResult->Contents.RegDescr.Reg = Erg;
    pResult->Contents.RegDescr.Dissect = DissectReg_M16C;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void MakeCode_M16C(void)
{
  OpSize = AttrPartOpSize[0];

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_M16C(void)
{
  return Memo("REG");
}

static void SwitchFrom_M16C(void)
{
  DeinitFields();
}

static void SwitchTo_M16C(void)
{
  TurnWords = True;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x14;
  NOPCode = 0x04;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".:";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfffff;        /* RMS 10: Probably a typo (was 0xffff) */

  DecodeAttrPart = DecodeAttrPart_M16C;
  MakeCode = MakeCode_M16C;
  IsDef = IsDef_M16C;
  InternSymbol = InternSymbol_M16C;
  DissectReg = DissectReg_M16C;
  SwitchFrom = SwitchFrom_M16C;
  InitFields();
}

void codem16c_init(void)
{
  CPUM16C = AddCPU("M16C", SwitchTo_M16C);
  CPUM30600M8 = AddCPU("M30600M8", SwitchTo_M16C);
  CPUM30610 = AddCPU("M30610", SwitchTo_M16C);
  CPUM30620 = AddCPU("M30620", SwitchTo_M16C);

  AddCopyright("Mitsubishi M16C-Generator also (C) 1999 RMS");
}
