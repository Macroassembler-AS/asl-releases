/* code51.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator fuer MCS-51/252 Prozessoren                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmcode.h"
#include "onoff_common.h"
#include "asmrelocs.h"
#include "asmlist.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "fileformat.h"
#include "errmsg.h"
#include "intformat.h"
#include "headids.h"

#include "code51.h"

#define set_b_guessed_11(flags, offs, len) \
        do { \
          if (mFirstPassUnknownOrQuestionable(flags)) { \
            set_basmcode_guessed(offs, 1, 0xe0); \
            set_basmcode_guessed((offs) + 1, len, 0xff); \
          } \
        } while (0)

/*-------------------------------------------------------------------------*/
/* Daten */

typedef struct
{
  CPUVar MinCPU;
  Word Code;
} FixedOrder;

enum
{
  ModNone = -1,
  ModReg = 1,
  ModIReg8 = 2,
  ModIReg = 3,
  ModInd = 5,
  ModImm = 7,
  ModImmEx = 8,
  ModDir8 = 9,
  ModDir16 = 10,
  ModAcc = 11,
  ModBit51 = 12,
  ModBit251 = 13
};

#define MModReg (1 << ModReg)
#define MModIReg8 (1 << ModIReg8)
#define MModIReg (1 << ModIReg)
#define MModInd (1 << ModInd)
#define MModImm (1 << ModImm)
#define MModImmEx (1 << ModImmEx)
#define MModDir8 (1 << ModDir8)
#define MModDir16 (1 << ModDir16)
#define MModAcc (1 << ModAcc)
#define MModBit51 (1 << ModBit51)
#define MModBit251 (1 << ModBit251)

#define MMod51 (MModReg | MModIReg8 | MModImm | MModAcc | MModDir8)
#define MMod251 (MModIReg | MModInd | MModImmEx | MModDir16)

#define AccReg 11
#define DPXValue 14
#define SPXValue 15

static FixedOrder *FixedOrders;
static FixedOrder *AccOrders;
static FixedOrder *CondOrders;
static FixedOrder *BCondOrders;

typedef struct
{
  Byte values[5];
  Byte count;
  Byte part, size;
  tSymbolFlags flags;

  PRelocEntry reloc_info;
  LongWord adr_offset, reloc_type;
} adr_vals_t;

static tSymbolSize OpSize;
static Boolean MinOneIs0;

static Boolean SrcMode;

static CPUVar CPU87C750, CPU8051, CPU8052, CPU80C320,
       CPU80501, CPU80502, CPU80504, CPU80515, CPU80517,
       CPU80C390,
       CPU80251, CPU80251T;

/*-------------------------------------------------------------------------*/
/* Adressparser */

static Boolean SetOpSize(tSymbolSize NewSize)
{
  if (OpSize == eSymbolSizeUnknown)
    OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pAsc, tRegInt *pValue, tSymbolSize *pSize)
 * \brief  check whether argument describes a CPU register
 * \param  pAsc argument
 * \param  pValue resulting register # if yes
 * \param  pSize resulting register size if yes
 * \return true if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pAsc, tRegInt *pValue, tSymbolSize *pSize)
{
  static Byte Masks[3] = { 0, 1, 3 };

  const char *Start;
  int alen = strlen(pAsc);
  char *p_end;

  if (!as_strcasecmp(pAsc, "DPX"))
  {
    *pValue = DPXValue;
    *pSize = eSymbolSize32Bit;
    return True;
  }

  if (!as_strcasecmp(pAsc, "SPX"))
  {
    *pValue = SPXValue;
    *pSize = eSymbolSize32Bit;
    return True;
  }

  if ((alen >= 2) && (as_toupper(*pAsc) == 'R'))
  {
    Start = pAsc + 1;
    *pSize = eSymbolSize8Bit;
  }
  else if ((MomCPU >= CPU80251) && (alen >= 3) && (as_toupper(*pAsc) == 'W') && (as_toupper(pAsc[1]) == 'R'))
  {
    Start = pAsc + 2;
    *pSize = eSymbolSize16Bit;
  }
  else if ((MomCPU >= CPU80251) && (alen >= 3) && (as_toupper(*pAsc) == 'D') && (as_toupper(pAsc[1]) == 'R'))
  {
    Start = pAsc + 2;
    *pSize = eSymbolSize32Bit;
  }
  else
    return False;

  *pValue = strtoul(Start, &p_end, 10);
  if (*p_end) return False;
  else if (*pValue & Masks[*pSize]) return False;
  else
  {
    *pValue >>= *pSize;
    switch (*pSize)
    {
      case eSymbolSize8Bit:
        return ((*pValue < 8) || ((MomCPU >= CPU80251) && (*pValue < 16)));
      case eSymbolSize16Bit:
        return (*pValue < 16);
      case eSymbolSize32Bit:
        return ((*pValue < 8) || (*pValue == DPXValue) || (*pValue == SPXValue));
      default:
        return False;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_51(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - 80(2)51 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_51(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    case eSymbolSize16Bit:
      as_snprintf(pDest, DestSize, "WR%u", (unsigned)Value << 1);
      break;
    case eSymbolSize32Bit:
      if (SPXValue == Value)
        strmaxcpy(pDest, "SPX", DestSize);
      else if (DPXValue == Value)
        strmaxcpy(pDest, "DPX", DestSize);
      else
        as_snprintf(pDest, DestSize, "DR%u", (unsigned)Value << 2);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Byte *pValue, tSymbolSize *pSize, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or user-defined register alias
 * \param  pArg argument
 * \param  pValue resulting register # if yes
 * \param  pSize resulting register size if yes
 * \param  MustBeReg operand must be a register
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Byte *pValue, tSymbolSize *pSize, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, &RegDescr.Reg, pSize))
  {
    *pValue = RegDescr.Reg;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
  *pValue = RegDescr.Reg;
  *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

static void SaveAdrRelocs(LongWord Type, LongWord Offset, adr_vals_t *p_vals)
{
  p_vals->adr_offset = Offset;
  p_vals->reloc_type = Type;
  p_vals->reloc_info = LastRelocs;
  LastRelocs = NULL;
}

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->count = 0;
  p_vals->flags = eSymbolFlag_None;
  p_vals->part = 0;
  p_vals->size = 0;

  p_vals->adr_offset = 0;
  p_vals->reloc_type = 0;
  p_vals->reloc_info = 0;
}

static ShortInt DecodeAdr(tStrComp *pArg, adr_vals_t *p_vals, Word Mask)
{
  Boolean OK, FirstFlag;
  tEvalResult EvalResult;
  tSymbolSize HSize;
  Word H16;
  LongWord H32;
  tStrComp SegComp, AddrComp, *pAddrComp;
  int SegType;
  char Save = '\0', *pSegSepPos;
  Word ExtMask;
  ShortInt adr_mode;

  adr_mode = ModNone;
  reset_adr_vals(p_vals);

  ExtMask = MMod251 & Mask;
  if (MomCPU < CPU80251) Mask &= MMod51;

  if (!*pArg->str.p_str)
     return adr_mode;

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    if (!(Mask & MModAcc))
    {
      adr_mode = ModReg;
      p_vals->part = AccReg;
    }
    else
      adr_mode = ModAcc;
    if (!SetOpSize(eSymbolSize8Bit))
      reset_adr_vals(p_vals);
    goto chk;
  }

  if (*pArg->str.p_str == '#')
  {
    tStrComp Comp;

    StrCompRefRight(&Comp, pArg, 1);
    if ((OpSize == eSymbolSizeUnknown) && (MinOneIs0)) OpSize = eSymbolSize8Bit;
    switch (OpSize)
    {
      case eSymbolSizeUnknown:
        WrError(ErrNum_UndefOpSizes);
        break;
      case eSymbolSize8Bit:
        p_vals->values[0] = EvalStrIntExpressionWithFlags(&Comp, Int8, &OK, &p_vals->flags);
        if (OK)
        {
          adr_mode = ModImm;
          p_vals->count = 1;
          SaveAdrRelocs(RelocTypeB8, 0, p_vals);
        }
        break;
      case eSymbolSize16Bit:
        H16 = EvalStrIntExpressionWithFlags(&Comp, Int16, &OK, &p_vals->flags);
        if (OK)
        {
          p_vals->values[0] = Hi(H16);
          p_vals->values[1] = Lo(H16);
          adr_mode = ModImm;
          p_vals->count = 2;
          SaveAdrRelocs(RelocTypeB16, 0, p_vals);
        }
        break;
      case eSymbolSize32Bit:
        H32 = EvalStrIntExpressionWithResult(&Comp, Int32, &EvalResult);
        if (mFirstPassUnknown(EvalResult.Flags))
          H32 &= 0xffff;
        if (EvalResult.OK)
        {
          p_vals->flags = EvalResult.Flags;
          p_vals->values[1] = H32 & 0xff;
          p_vals->values[0] = (H32 >> 8) & 0xff;
          H32 >>= 16;
          if (H32 == 0)
            adr_mode = ModImm;
          else if ((H32 == 1) || (H32 == 0xffff))
            adr_mode = ModImmEx;
          else
            WrError(ErrNum_UndefOpSizes);
          if (adr_mode != ModNone)
            p_vals->count = 2;
          SaveAdrRelocs(RelocTypeB16, 0, p_vals);
        }
        break;
      case eSymbolSize24Bit:
        H32 = EvalStrIntExpressionWithFlags(&Comp, Int24, &OK, &p_vals->flags);
        if (OK)
        {
          p_vals->values[0] = (H32 >> 16) & 0xff;
          p_vals->values[1] = (H32 >> 8) & 0xff;
          p_vals->values[2] = H32 & 0xff;
          p_vals->count = 3;
          adr_mode = ModImm;
          SaveAdrRelocs(RelocTypeB24, 0, p_vals);
        }
        break;
      default:
        WrStrErrorPos(ErrNum_InvOpSize, &Comp);
    }
    goto chk;
  }

  switch (DecodeReg(pArg, &p_vals->part, &HSize, False))
  {
    case eIsReg:
      if ((MomCPU >= CPU80251) && ((Mask & MModReg) == 0))
        adr_mode = ((HSize == 0) && (p_vals->part == AccReg)) ? ModAcc : ModReg;
      else
        adr_mode = ModReg;
      if (!SetOpSize(HSize))
        reset_adr_vals(p_vals);
      goto chk;
    case eIsNoReg:
      break;
    case eRegAbort:
      return adr_mode;
  }

  if (*pArg->str.p_str == '@')
  {
    tStrComp IndirComp;
    char *PPos, *MPos;

    StrCompRefRight(&IndirComp, pArg, 1);
    PPos = strchr(IndirComp.str.p_str, '+');
    MPos = strchr(IndirComp.str.p_str, '-');
    if ((MPos) && ((MPos < PPos) || (!PPos)))
      PPos = MPos;
    if (PPos)
    {
      Save = *PPos;
      *PPos = '\0';
      IndirComp.Pos.Len = PPos - IndirComp.str.p_str;
    }
    switch (DecodeReg(&IndirComp, &p_vals->part, &HSize, False))
    {
      case eIsReg:
      {
        if (!PPos)
        {
          H32 = 0;
          OK = True;
        }
        else
        {
          tStrComp DispComp;

          *PPos = Save;
          StrCompRefRight(&DispComp, &IndirComp, PPos - IndirComp.str.p_str + !!(Save == '+'));
          H32 = EvalStrIntExpressionWithFlags(&DispComp, SInt16, &OK, &p_vals->flags);
        }
        if (OK)
          switch (HSize)
          {
            case eSymbolSize8Bit:
              if ((p_vals->part > 1) || (H32 != 0)) WrError(ErrNum_InvAddrMode);
              else
                adr_mode = ModIReg8;
              break;
            case eSymbolSize16Bit:
              if (H32 == 0)
              {
                adr_mode = ModIReg;
                p_vals->size = 0;
              }
              else
              {
                adr_mode = ModInd;
                p_vals->size = 0;
                p_vals->values[1] = H32 & 0xff;
                p_vals->values[0] = (H32 >> 8) & 0xff;
                p_vals->count = 2;
              }
              break;
            case eSymbolSize32Bit:
              if (H32 == 0)
              {
                adr_mode = ModIReg;
                p_vals->size = 2;
              }
              else
              {
                adr_mode = ModInd;
                p_vals->size = 2;
                p_vals->values[1] = H32 & 0xff;
                p_vals->values[0] = (H32 >> 8) & 0xff;
                p_vals->count = 2;
              }
              break;
            default:
              break;
          }
        break;
      }
      case eIsNoReg:
        WrStrErrorPos(ErrNum_InvReg, &IndirComp);
        break;
      case eRegAbort:
        /* will go to function end anyway after restoring separator */
        break;
    }
    if (PPos)
      *PPos = Save;
    goto chk;
  }

  FirstFlag = False;
  SegType = -1;
  pSegSepPos = QuotPos(pArg->str.p_str, ':');
  if (pSegSepPos)
  {
    StrCompSplitRef(&SegComp, &AddrComp, pArg, pSegSepPos);
    if (MomCPU < CPU80251)
    {
      WrError(ErrNum_InvAddrMode);
      return adr_mode;
    }
    else
    {
      if (!as_strcasecmp(SegComp.str.p_str, "S"))
        SegType = -2;
      else
      {
        SegType = EvalStrIntExpressionWithResult(&SegComp, UInt8, &EvalResult);
        if (!EvalResult.OK)
          return adr_mode;
        if (mFirstPassUnknown(EvalResult.Flags))
          FirstFlag = True;
      }
    }
    pAddrComp = &AddrComp;
  }
  else
    pAddrComp = pArg;

  switch (SegType)
  {
    case -2:
      H32 = EvalStrIntExpressionWithResult(pAddrComp, UInt9, &EvalResult);
      ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      if (mFirstPassUnknown(EvalResult.Flags))
        H32 = (H32 & 0xff) | 0x80;
      break;
    case -1:
      H32 = EvalStrIntExpressionWithResult(pAddrComp, UInt24, &EvalResult);
      break;
    default:
      H32 = EvalStrIntExpressionWithResult(pAddrComp, UInt16, &EvalResult);
  }
  if (mFirstPassUnknown(EvalResult.Flags))
    FirstFlag = True;
  if (!EvalResult.OK)
    return adr_mode;

  if ((SegType == -2) || ((SegType == -1) && (EvalResult.AddrSpaceMask & (1 << SegIO))))
  {
    if (ChkRange(H32, 0x80, 0xff))
    {
      p_vals->flags = EvalResult.Flags;
      SaveAdrRelocs(RelocTypeB8, 0, p_vals);
      adr_mode = ModDir8;
      p_vals->values[0] = H32 & 0xff;
      p_vals->count = 1;
    }
  }

  else
  {
    if (SegType >= 0)
      H32 += ((LongWord)SegType) << 16;
    if (FirstFlag)
      H32 &= ((MomCPU < CPU80251) || ((Mask & ModDir16) == 0)) ? 0xff : 0xffff;

    if (((H32 < 128) || ((H32 < 256) && (MomCPU < CPU80251))) && ((Mask & MModDir8) != 0))
    {
      if (MomCPU < CPU80251)
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
      SaveAdrRelocs(RelocTypeB8, 0, p_vals);
      adr_mode = ModDir8;
      p_vals->values[0] = H32 & 0xff;
      p_vals->count = 1;
    }
    else if ((MomCPU < CPU80251) || (H32 > 0xffff)) WrError(ErrNum_AdrOverflow);
    else
    {
      adr_mode = ModDir16;
      p_vals->count = 2;
      p_vals->values[1] = H32 & 0xff;
      p_vals->values[0] = (H32 >> 8) & 0xff;
    }
    p_vals->flags = EvalResult.Flags;
  }

chk:
  if ((adr_mode != ModNone) && ((Mask & (1 << adr_mode)) == 0))
  {
    if (ExtMask & (1 << adr_mode))
      (void)ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported);
    else
      WrError(ErrNum_InvAddrMode);
    reset_adr_vals(p_vals);
  }
  return adr_mode;
}

static void append_adr_vals(adr_vals_t *p_vals)
{
  TransferRelocs2(p_vals->reloc_info, ProgCounter() + p_vals->adr_offset + CodeLen, p_vals->reloc_type);
  p_vals->reloc_info = NULL;
  set_b_guessed(p_vals->flags, CodeLen, p_vals->count, 0xff);
  memcpy(&BAsmCode[CodeLen], p_vals->values, p_vals->count);
  CodeLen += p_vals->count;
}

static void DissectBit_251(char *pDest, size_t DestSize, LargeWord Inp)
{
  as_snprintf(pDest, DestSize, "%~02.*u%s.%u",
              ListRadixBase, (unsigned)(Inp & 0xff), GetIntConstIntelSuffix(ListRadixBase),
              (unsigned)(Inp >> 24));
}

static ShortInt DecodeBitAdr(tStrComp *pArg, LongInt *Erg, tSymbolFlags *p_symbol_flags, Boolean MayShorten)
{
  tEvalResult EvalResult;
  char *pPos, Save = '\0';
  tStrComp RegPart, BitPart;

  pPos = RQuotPos(pArg->str.p_str, '.');
  if (pPos)
    Save = StrCompSplitRef(&RegPart, &BitPart, pArg, pPos);
  if (MomCPU < CPU80251)
  {
    if (!pPos)
    {
      *Erg = EvalStrIntExpressionWithResult(pArg, UInt8, &EvalResult);
      if (EvalResult.OK)
      {
        *p_symbol_flags = EvalResult.Flags;
        ChkSpace(SegBData, EvalResult.AddrSpaceMask);
        return ModBit51;
      }
      else
        return ModNone;
    }
    else
    {
      *Erg = EvalStrIntExpressionWithResult(&RegPart, UInt8, &EvalResult);
      if (mFirstPassUnknown(EvalResult.Flags))
        *Erg = 0x20;
      *pPos = Save;
      if (!EvalResult.OK) return ModNone;
      else
      {
        *p_symbol_flags = EvalResult.Flags;
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
        Save = EvalStrIntExpressionWithResult(&BitPart, UInt3, &EvalResult);
        if (!EvalResult.OK) return ModNone;
        else
        {
          *p_symbol_flags |= EvalResult.Flags;
          if (*Erg > 0x7f)
          {
            if ((*Erg) & 7)
              WrError(ErrNum_NotBitAddressable);
          }
          else
          {
            if (((*Erg) & 0xe0) != 0x20)
              WrError(ErrNum_NotBitAddressable);
            *Erg = (*Erg - 0x20) << 3;
          }
          *Erg += Save;
          return ModBit51;
        }
      }
    }
  }
  else
  {
    if (!pPos)
    {
      static const LongWord ValidBits = 0x070000fful;

      *Erg = EvalStrIntExpressionWithResult(pArg, Int32, &EvalResult);
      if (mFirstPassUnknown(EvalResult.Flags))
        *Erg &= ValidBits;
      if (*Erg & ~ValidBits)
      {
        WrError(ErrNum_InvBitPos);
        EvalResult.OK = False;
      }
      *p_symbol_flags = EvalResult.Flags;
    }
    else
    {
      adr_vals_t adr_vals;
      ShortInt adr_mode;

      adr_mode = DecodeAdr(&RegPart, &adr_vals, MModDir8);
      *pPos = Save;
      if (adr_mode == ModNone)
        EvalResult.OK = False;
      else
      {
        *p_symbol_flags = adr_vals.flags;
        *Erg = EvalStrIntExpressionWithResult(&BitPart, UInt3, &EvalResult) << 24;
        if (EvalResult.OK)
        {
          (*Erg) += adr_vals.values[0];
          *p_symbol_flags |= EvalResult.Flags;
        }
      }
    }
    if (!EvalResult.OK)
      return ModNone;
    else if (MayShorten)
    {
      if (((*Erg) & 0x87) == 0x80)
      {
        *Erg = ((*Erg) & 0xf8) + ((*Erg) >> 24);
        return ModBit51;
      }
      else if (((*Erg) & 0xf0) == 0x20)
      {
        *Erg = (((*Erg) & 0x0f) << 3) + ((*Erg) >> 24);
        return ModBit51;
      }
      else
        return ModBit251;
    }
    else
      return ModBit251;
  }
}

static Boolean Chk504(LongInt Adr)
{
  return ((MomCPU == CPU80504) && ((Adr & 0x7ff) == 0x7fe));
}

static Boolean NeedsPrefix(Word Opcode)
{
  return (((Opcode&0x0f) >= 6) && ((SrcMode != 0) != ((Hi(Opcode) != 0) != 0)));
}

static void PutCode(Word Opcode)
{
  if (((Opcode&0x0f) < 6) || ((SrcMode != 0) != ((Hi(Opcode) == 0) != 0)))
  {
    BAsmCode[0] = Lo(Opcode);
    CodeLen = 1;
  }
  else
  {
    BAsmCode[0] = 0xa5;
    BAsmCode[1] = Lo(Opcode);
    CodeLen = 2;
  }
}

static Boolean IsCarry(const char *pArg)
{
  return (!as_strcasecmp(pArg, "C")) || (!as_strcasecmp(pArg, "CY"));
}

/*-------------------------------------------------------------------------*/
/* Einzelfaelle */

static void DecodeMOV(Word Index)
{
  LongInt AdrLong;
  adr_vals_t adr_vals;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (IsCarry(ArgStr[1].str.p_str))
  {
    switch (DecodeBitAdr(&ArgStr[2], &AdrLong, &adr_vals.flags, True))
    {
      case ModBit51:
        PutCode(0xa2);
        set_b_guessed(adr_vals.flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
      case ModBit251:
        PutCode(0x1a9);
        set_b_guessed(adr_vals.flags, CodeLen, 1, 0x07);
        BAsmCode[CodeLen++] = 0xa0 + (AdrLong >> 24);
        set_b_guessed(adr_vals.flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
    }
  }
  else if ((!as_strcasecmp(ArgStr[2].str.p_str, "C")) || (!as_strcasecmp(ArgStr[2].str.p_str, "CY")))
  {
    switch (DecodeBitAdr(&ArgStr[1], &AdrLong, &adr_vals.flags, True))
    {
      case ModBit51:
        PutCode(0x92);
        set_b_guessed(adr_vals.flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
      case ModBit251:
        PutCode(0x1a9);
        set_b_guessed(adr_vals.flags, CodeLen, 1, 0x07);
        BAsmCode[CodeLen++] = 0x90 + (AdrLong >> 24);
        set_b_guessed(adr_vals.flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        CodeLen+=2;
        break;
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "DPTR"))
  {
    (void)SetOpSize((MomCPU == CPU80C390) ? eSymbolSize24Bit : eSymbolSize16Bit);
    switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm))
    {
      case ModImm:
        PutCode(0x90);
        append_adr_vals(&adr_vals);
        break;
    }
  }
  else
  {
    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModReg | MModIReg8 | MModIReg | MModInd | MModDir8 | MModDir16))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg | MModIReg8 | MModIReg | MModInd | MModDir8 | MModDir16 | MModImm))
        {
          case ModReg:
            if ((adr_vals.part < 8) && (!SrcMode))
              PutCode(0xe8 + adr_vals.part);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17c);
              BAsmCode[CodeLen++] = (AccReg << 4) + adr_vals.part;
            }
            break;
          case ModIReg8:
            PutCode(0xe6 + adr_vals.part);
            break;
          case ModIReg:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = (adr_vals.part << 4) + 0x09 + adr_vals.size;
            BAsmCode[CodeLen++] = (AccReg << 4);
            break;
          case ModInd:
            PutCode(0x109 + (adr_vals.size << 4));
            BAsmCode[CodeLen++] = (AccReg << 4) + adr_vals.part;
            append_adr_vals(&adr_vals);
            break;
          case ModDir8:
            if (!mFirstPassUnknownOrQuestionable(adr_vals.flags)
             && (adr_vals.values[0] == 0xe0)
             && (MomCPU < CPU80251))
              WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[2]);
            PutCode(0xe5);
            append_adr_vals(&adr_vals);
            break;
          case ModDir16:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = (AccReg << 4) + 0x03;
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            PutCode(0x74);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      case ModReg:
      {
        Byte dest_reg = adr_vals.part;

        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg | MModIReg8 | MModIReg | MModInd | MModDir8 | MModDir16 | MModImm | MModImmEx))
        {
          case ModReg:
            if ((OpSize == eSymbolSize8Bit) && (adr_vals.part == AccReg) && (dest_reg < 8))
              PutCode(0xf8 + dest_reg);
            else if ((OpSize == eSymbolSize8Bit) && (dest_reg == AccReg) && (adr_vals.part < 8))
              PutCode(0xe8 + adr_vals.part);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17c + OpSize);
              if (OpSize == eSymbolSize32Bit)
                BAsmCode[CodeLen - 1]++;
              BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
            }
            break;
          case ModIReg8:
            if ((OpSize != eSymbolSize8Bit) || (dest_reg != AccReg)) WrError(ErrNum_InvAddrMode);
            else
              PutCode(0xe6 + adr_vals.part);
            break;
          case ModIReg:
            if (OpSize == eSymbolSize8Bit)
            {
              PutCode(0x17e);
              BAsmCode[CodeLen++] = (adr_vals.part << 4) + 0x09 + adr_vals.size;
              BAsmCode[CodeLen++] = dest_reg << 4;
            }
            else if (OpSize == eSymbolSize16Bit)
            {
              PutCode(0x10b);
              BAsmCode[CodeLen++] = (adr_vals.part << 4) + 0x08 + adr_vals.size;
              BAsmCode[CodeLen++] = dest_reg << 4;
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
          case ModInd:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x109 + (adr_vals.size << 4) + (OpSize << 6));
              BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
              append_adr_vals(&adr_vals);
            }
            break;
          case ModDir8:
            if ((OpSize == eSymbolSize8Bit) && (dest_reg == AccReg))
            {
              PutCode(0xe5);
              append_adr_vals(&adr_vals);
            }
            else if ((OpSize == eSymbolSize8Bit) && (dest_reg < 8) && (!SrcMode))
            {
              PutCode(0xa8 + dest_reg);
              append_adr_vals(&adr_vals);
            }
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17e);
              BAsmCode[CodeLen++] = 0x01 + (dest_reg << 4) + (OpSize << 2);
              if (OpSize == eSymbolSize32Bit)
                BAsmCode[CodeLen - 1] += 4;
              append_adr_vals(&adr_vals);
            }
            break;
          case ModDir16:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = 0x03 + (dest_reg << 4) + (OpSize << 2);
            if (OpSize == eSymbolSize32Bit)
              BAsmCode[CodeLen - 1] += 4;
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            if ((OpSize == eSymbolSize8Bit) && (dest_reg == AccReg))
            {
              PutCode(0x74);
              append_adr_vals(&adr_vals);
            }
            else if ((OpSize == eSymbolSize8Bit) && (dest_reg < 8) && (!SrcMode))
            {
              PutCode(0x78 + dest_reg);
              append_adr_vals(&adr_vals);
            }
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17e);
              BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2);
              append_adr_vals(&adr_vals);
            }
            break;
          case ModImmEx:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = 0x0c + (dest_reg << 4);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      }
      case ModIReg8:
      {
        Byte dest_reg = adr_vals.part;

        (void)SetOpSize(eSymbolSize8Bit);
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAcc | MModDir8 | MModImm))
        {
          case ModAcc:
            PutCode(0xf6 + dest_reg);
            break;
          case ModDir8:
            PutCode(0xa6 + dest_reg);
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            PutCode(0x76 + dest_reg);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      }
      case ModIReg:
      {
        adr_vals_t src_adr_vals;

        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg))
        {
          case ModReg:
            if (OpSize == eSymbolSize8Bit)
            {
              PutCode(0x17a);
              BAsmCode[CodeLen++] = (adr_vals.part << 4) + 0x09 + adr_vals.size;
              BAsmCode[CodeLen++] = src_adr_vals.part << 4;
            }
            else if (OpSize == eSymbolSize16Bit)
            {
              PutCode(0x11b);
              BAsmCode[CodeLen++] = (adr_vals.part << 4) + 0x08 + adr_vals.size;
              BAsmCode[CodeLen++] = src_adr_vals.part << 4;
            }
            else
              WrError(ErrNum_InvAddrMode);
        }
        break;
      }
      case ModInd:
      {
        adr_vals_t src_adr_vals;

        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg))
        {
          case ModReg:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x119 + (adr_vals.size << 4) + (OpSize << 6));
              BAsmCode[CodeLen++] = (src_adr_vals.part << 4) + adr_vals.part;
              append_adr_vals(&adr_vals);
            }
        }
        break;
      }
      case ModDir8:
      {
        adr_vals_t src_adr_vals;

        MinOneIs0 = True;
        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg | MModIReg8 | MModDir8 | MModImm))
        {
          case ModReg:
            if ((OpSize == eSymbolSize8Bit) && (src_adr_vals.part == AccReg))
            {
              PutCode(0xf5);
              append_adr_vals(&adr_vals);
            }
            else if ((OpSize == eSymbolSize8Bit) && (src_adr_vals.part < 8) && (!SrcMode))
            {
              PutCode(0x88 + src_adr_vals.part);
              append_adr_vals(&adr_vals);
            }
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17a);
              BAsmCode[CodeLen++] = 0x01 + (src_adr_vals.part << 4) + (OpSize << 2);
              if (OpSize == eSymbolSize32Bit)
                BAsmCode[CodeLen - 1] += 4;
              append_adr_vals(&adr_vals);
            }
            break;
          case ModIReg8:
            PutCode(0x86 + src_adr_vals.part);
            append_adr_vals(&adr_vals);
            break;
          case ModDir8:
            PutCode(0x85);
            append_adr_vals(&src_adr_vals);
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            PutCode(0x75);
            append_adr_vals(&adr_vals);
            append_adr_vals(&src_adr_vals);
            break;
        }
        break;
      }
      case ModDir16:
      {
        adr_vals_t src_adr_vals;

        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg))
        {
          case ModReg:
            PutCode(0x17a);
            BAsmCode[CodeLen++] = 0x03 + (src_adr_vals.part << 4) + (OpSize << 2);
            if (OpSize == eSymbolSize32Bit) BAsmCode[CodeLen - 1] += 4;
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      }
    }
  }
}

static void DecodeLogic(Word Index)
{
  LongInt AdrLong;
  int z;
  adr_vals_t adr_vals;

  /* Index: ORL=0 ANL=1 XRL=2 */

  if (!ChkArgCnt(2, 2));
  else if (IsCarry(ArgStr[1].str.p_str))
  {
    if (Index == 2) WrError(ErrNum_InvAddrMode);
    else
    {
      Boolean InvFlag;
      ShortInt Result;
      Byte dest_reg = Index << 4;

      InvFlag = *ArgStr[2].str.p_str == '/';
      if (InvFlag)
      {
        tStrComp Comp;

        StrCompRefRight(&Comp, &ArgStr[2], 1);
        Result = DecodeBitAdr(&Comp, &AdrLong, &adr_vals.flags, True);
      }
      else
        Result = DecodeBitAdr(&ArgStr[2], &AdrLong, &adr_vals.flags, True);
      switch (Result)
      {
        case ModBit51:
          PutCode(InvFlag ? 0xa0 + dest_reg : 0x72 + dest_reg);
          set_b_guessed(CodeLen, adr_vals.flags, 1, 0xff);
          BAsmCode[CodeLen++] = AdrLong & 0xff;
          break;
        case ModBit251:
          PutCode(0x1a9);
          set_b_guessed(CodeLen, adr_vals.flags, 1, 0x07);
          BAsmCode[CodeLen++] = (InvFlag ? 0xe0 : 0x70) + dest_reg + (AdrLong >> 24);
          set_b_guessed(CodeLen, adr_vals.flags, 1, 0xff);
          BAsmCode[CodeLen++] = AdrLong & 0xff;
          break;
      }
    }
  }
  else
  {
    z = (Index << 4) + 0x40;
    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModReg | MModDir8))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg | MModIReg8 | MModIReg | MModDir8 | MModDir16 | MModImm))
        {
          case ModReg:
            if ((adr_vals.part < 8) && (!SrcMode)) PutCode(z + 8 + adr_vals.part);
            else
            {
              PutCode(z + 0x10c);
              BAsmCode[CodeLen++] = adr_vals.part + (AccReg << 4);
            }
            break;
          case ModIReg8:
            PutCode(z + 6 + adr_vals.part);
            break;
          case ModIReg:
            PutCode(z + 0x10e);
            BAsmCode[CodeLen++] = 0x09 + adr_vals.size + (adr_vals.part << 4);
            BAsmCode[CodeLen++] = AccReg << 4;
            break;
          case ModDir8:
            PutCode(z + 0x05);
            append_adr_vals(&adr_vals);
            break;
          case ModDir16:
            PutCode(0x10e + z);
            BAsmCode[CodeLen++] = 0x03 + (AccReg << 4);
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            PutCode(z + 0x04);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      case ModReg:
        if (MomCPU < CPU80251) WrError(ErrNum_InvAddrMode);
        else
        {
          Byte dest_reg = adr_vals.part;

          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg | MModIReg8 | MModIReg | MModDir8 | MModDir16 | MModImm))
          {
            case ModReg:
              if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(z + 0x10c + OpSize);
                BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
              }
              break;
            case ModIReg8:
              if ((OpSize != eSymbolSize8Bit) || (dest_reg != AccReg)) WrError(ErrNum_InvAddrMode);
              else
                PutCode(z + 0x06 + adr_vals.part);
              break;
            case ModIReg:
              if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = 0x09 + adr_vals.size + (adr_vals.part << 4);
                BAsmCode[CodeLen++] = dest_reg << 4;
              }
              break;
            case ModDir8:
              if ((OpSize == eSymbolSize8Bit) && (dest_reg == AccReg))
              {
                PutCode(0x05 + z);
                append_adr_vals(&adr_vals);
              }
              else if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2) + 1;
                append_adr_vals(&adr_vals);
              }
              break;
            case ModDir16:
              if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2) + 3;
                append_adr_vals(&adr_vals);
              }
              break;
            case ModImm:
              if ((OpSize == eSymbolSize8Bit) && (dest_reg == AccReg))
              {
                PutCode(0x04 + z);
                append_adr_vals(&adr_vals);
              }
              else if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2);
                append_adr_vals(&adr_vals);
              }
              break;
          }
        }
        break;
      case ModDir8:
      {
        adr_vals_t src_adr_vals;

        if (!SetOpSize(eSymbolSize8Bit))
          return;
        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModAcc | MModImm))
        {
          case ModAcc:
            PutCode(z + 0x02);
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            PutCode(z + 0x03);
            append_adr_vals(&adr_vals);
            append_adr_vals(&src_adr_vals);
            break;
        }
        break;
      }
    }
  }
}

static void DecodeMOVC(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc))
    {
      case ModAcc:
        if (!as_strcasecmp(ArgStr[2].str.p_str, "@A+DPTR"))
          PutCode(0x93);
        else if (!as_strcasecmp(ArgStr[2].str.p_str, "@A+PC"))
          PutCode(0x83);
        else
          WrError(ErrNum_InvAddrMode);
        break;
    }
  }
}

static void DecodeMOVH(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80251))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg))
    {
      case ModReg:
        if (OpSize != eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
        else
        {
          Byte dest_reg = adr_vals.part;
          OpSize--;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm))
          {
            case ModImm:
              PutCode(0x17a);
              BAsmCode[CodeLen++] = 0x0c + (dest_reg << 4);
              append_adr_vals(&adr_vals);
              break;
          }
        }
        break;
    }
  }
}

static void DecodeMOVZS(Word code)
{
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80251))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg))
    {
      case ModReg:
        if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvAddrMode);
        else
        {
          Byte dest_reg = adr_vals.part;
          OpSize--;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg))
          {
            case ModReg:
             PutCode(0x10a + code);
             BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
             break;
          }
        }
        break;
    }
  }
}

static void DecodeMOVX(Word Index)
{
  int z;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    z = 0;
    if ((!as_strcasecmp(ArgStr[2].str.p_str, "A")) || ((MomCPU >= CPU80251) && (!as_strcasecmp(ArgStr[2].str.p_str, "R11"))))
    {
      z = 0x10;
      strcpy(ArgStr[2].str.p_str, ArgStr[1].str.p_str);
      strmaxcpy(ArgStr[1].str.p_str, "A", STRINGSIZE);
    }
    if ((as_strcasecmp(ArgStr[1].str.p_str, "A")) && ((MomCPU < CPU80251) || (!as_strcasecmp(ArgStr[2].str.p_str, "R11")))) WrError(ErrNum_InvAddrMode);
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "@DPTR"))
      PutCode(0xe0 + z);
    else
    {
      adr_vals_t adr_vals;

      switch (DecodeAdr(&ArgStr[2], &adr_vals, MModIReg8))
      {
        case ModIReg8:
          PutCode(0xe2 + adr_vals.part + z);
          break;
      }
    }
  }
}

static void DecodeStack(Word Index)
{
  int z;

  /* Index: PUSH=0 POP=1 PUSHW=2 */

  z = (Index & 1) << 4;
  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    if (*ArgStr[1].str.p_str == '#')
      (void)SetOpSize((Index == 2) ? eSymbolSize16Bit : eSymbolSize8Bit);
    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModDir8 | MModReg | ((z == 0x10) ? 0 : MModImm)))
    {
      case ModDir8:
        PutCode(0xc0 + z);
        append_adr_vals(&adr_vals);
        break;
      case ModReg:
        if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
        {
          PutCode(0x1ca + z);
          BAsmCode[CodeLen++] = 0x08 + (adr_vals.part << 4) + OpSize + (Ord(OpSize == eSymbolSize32Bit));
        }
        break;
      case ModImm:
        if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
        {
          PutCode(0x1ca);
          BAsmCode[CodeLen++] = 0x02 + (OpSize << 2);
          append_adr_vals(&adr_vals);
        }
        break;
    }
  }
}

static void DecodeXCH(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModReg | MModIReg8 | MModDir8))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg | MModIReg8 | MModDir8))
        {
          case ModReg:
            if (adr_vals.part > 7) WrError(ErrNum_InvAddrMode);
            else
              PutCode(0xc8 + adr_vals.part);
            break;
          case ModIReg8:
            PutCode(0xc6 + adr_vals.part);
            break;
          case ModDir8:
            PutCode(0xc5);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      case ModReg:
        if ((OpSize != eSymbolSize8Bit) || (adr_vals.part > 7)) WrError(ErrNum_InvAddrMode);
        else
        {
          Byte dest_reg = adr_vals.part;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAcc))
          {
            case ModAcc:
              PutCode(0xc8 + dest_reg);
              break;
          }
        }
        break;
      case ModIReg8:
      {
        Byte dest_reg = adr_vals.part;

        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAcc))
        {
          case ModAcc:
            PutCode(0xc6 + dest_reg);
            break;
        }
        break;
      }
      case ModDir8:
      {
        adr_vals_t src_adr_vals;

        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModAcc))
        {
          case ModAcc:
            PutCode(0xc5);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      }
    }
  }
}

static void DecodeXCHD(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModIReg8))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModIReg8))
        {
          case ModIReg8:
            PutCode(0xd6 + adr_vals.part);
            break;
        }
        break;
      case ModIReg8:
      {
        Byte dest_reg = adr_vals.part;
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAcc))
        {
          case ModAcc:
            PutCode(0xd6 + dest_reg);
            break;
        }
        break;
      }
    }
  }
}

#define RelocTypeABranch11 (11 | RelocFlagBig | RelocFlagPage | (5 << 8) | (3 << 12)) | (0 << 16)
#define RelocTypeABranch19 (19 | RelocFlagBig | RelocFlagPage | (5 << 8) | (3 << 12)) | (0 << 16)

static void DecodeABranch(Word Index)
{
  /* Index: AJMP = 0 ACALL = 1 */

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    LongInt AdrLong = EvalStrIntExpressionWithResult(&ArgStr[1], Int24, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      if (MomCPU == CPU80C390)
      {
        if (ChkSamePage(EProgCounter() + 3, AdrLong, 19, EvalResult.Flags))
        {
          set_b_guessed_11(EvalResult.Flags, CodeLen, 2);
          PutCode(0x01 + (Index << 4) + (((AdrLong >> 16) & 7) << 5));
          BAsmCode[CodeLen++] = Hi(AdrLong);
          BAsmCode[CodeLen++] = Lo(AdrLong);
          TransferRelocs(ProgCounter() - 3, RelocTypeABranch19);
        }
      }
      else
      {
        if (!ChkSamePage(EProgCounter(), AdrLong, 11, EvalResult.Flags));
        else if (Chk504(EProgCounter())) WrError(ErrNum_NotOnThisAddress);
        else
        {
          set_b_guessed_11(EvalResult.Flags, CodeLen, 1);
          PutCode(0x01 + (Index << 4) + ((Hi(AdrLong) & 7) << 5));
          BAsmCode[CodeLen++] = Lo(AdrLong);
          TransferRelocs(ProgCounter() - 2, RelocTypeABranch11);
        }
      }
    }
  }
}

static void DecodeLBranch(Word Index)
{
  /* Index: LJMP=0 LCALL=1 */

  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU8051));
  else if (*ArgStr[1].str.p_str == '@')
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModIReg))
    {
      case ModIReg:
        if (adr_vals.size != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x189 + (Index << 4));
          BAsmCode[CodeLen++] = 0x04 + (adr_vals.part << 4);
        }
        break;
    }
  }
  else
  {
    tEvalResult EvalResult;
    LongInt AdrLong = EvalStrIntExpressionWithResult(&ArgStr[1], (MomCPU < CPU80C390) ? Int16 : Int24, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      if (MomCPU == CPU80C390)
      {
        PutCode(0x02 + (Index << 4));
        set_b_guessed(EvalResult.Flags, CodeLen, 3, 0xff);
        BAsmCode[CodeLen++] = (AdrLong >> 16) & 0xff;
        BAsmCode[CodeLen++] = (AdrLong >> 8) & 0xff;
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        TransferRelocs(ProgCounter() + 1, RelocTypeB24);
      }
      else
      {
        if ((MomCPU >= CPU80251) && !ChkSamePage(EProgCounter() + 3, AdrLong, 16, EvalResult.Flags));
        else
        {
          PutCode(0x02 + (Index << 4));
          set_b_guessed(EvalResult.Flags, CodeLen, 2, 0xff);
          BAsmCode[CodeLen++] = (AdrLong >> 8) & 0xff;
          BAsmCode[CodeLen++] = AdrLong & 0xff;
          TransferRelocs(ProgCounter() + 1, RelocTypeB16);
        }
      }
    }
  }
}

static void DecodeEBranch(Word Index)
{
  /* Index: AJMP=0 ACALL=1 */

  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU80251));
  else if (*ArgStr[1].str.p_str == '@')
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModIReg))
    {
      case ModIReg:
        if (adr_vals.size != 2) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x189 + (Index << 4));
          BAsmCode[CodeLen++] = 0x08 + (adr_vals.part << 4);
        }
        break;
    }
  }
  else
  {
    tEvalResult EvalResult;
    LongInt AdrLong = EvalStrIntExpressionWithResult(&ArgStr[1], UInt24, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      PutCode(0x18a + (Index << 4));
      set_b_guessed(EvalResult.Flags, CodeLen, 3, 0xff);
      BAsmCode[CodeLen++] = (AdrLong >> 16) & 0xff;
      BAsmCode[CodeLen++] = (AdrLong >>  8) & 0xff;
      BAsmCode[CodeLen++] =  AdrLong        & 0xff;
    }
  }
}

static void DecodeJMP(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "@A+DPTR"))
    PutCode(0x73);
  else if (*ArgStr[1].str.p_str == '@')
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModIReg))
    {
      case ModIReg:
        PutCode(0x189);
        BAsmCode[CodeLen++] = 0x04 + (adr_vals.size << 1) + (adr_vals.part << 4);
        break;
    }
  }
  else
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags);

    if (OK)
    {
      LongInt Dist = AdrLong - (EProgCounter() + 2);

      if ((Dist <= 127) && (Dist >= -128))
      {
        PutCode(0x80);
        set_b_guessed(Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = Dist & 0xff;
      }
      else if ((!Chk504(EProgCounter())) && ((AdrLong >> 11) == ((((long)EProgCounter()) + 2) >> 11)))
      {
        set_b_guessed_11(Flags, CodeLen, 1);
        PutCode(0x01 + ((Hi(AdrLong) & 7) << 5));
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
      else if (MomCPU < CPU8051) WrError(ErrNum_JmpTargOnDiffPage);
      else if (((((long)EProgCounter()) + 3) >> 16) == (AdrLong >> 16))
      {
        PutCode(0x02);
        set_b_guessed(Flags, CodeLen, 2, 0xff);
        BAsmCode[CodeLen++] = Hi(AdrLong);
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
      else if (MomCPU < CPU80251) WrError(ErrNum_JmpTargOnDiffPage);
      else
      {
        PutCode(0x18a);
        set_b_guessed(Flags, CodeLen, 3, 0xff);
        BAsmCode[CodeLen++] = (AdrLong >> 16) & 0xff;
        BAsmCode[CodeLen++] = (AdrLong >>  8) & 0xff;
        BAsmCode[CodeLen++] =  AdrLong        & 0xff;
      }
    }
  }
}

static void DecodeCALL(Word Index)
{
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str == '@')
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModIReg))
    {
      case ModIReg:
        PutCode(0x199);
        BAsmCode[CodeLen++] = 0x04 + (adr_vals.size << 1) + (adr_vals.part << 4);
        break;
    }
  }
  else
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags);
    if (OK)
    {
      if ((!Chk504(EProgCounter())) && ((AdrLong >> 11) == ((((long)EProgCounter()) + 2) >> 11)))
      {
        set_b_guessed_11(Flags, CodeLen, 1);
        PutCode(0x11 + ((Hi(AdrLong) & 7) << 5));
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
      else if (MomCPU < CPU8051) WrError(ErrNum_JmpTargOnDiffPage);
      else if (ChkSamePage(AdrLong, EProgCounter() + 3, 16, Flags))
      {
        PutCode(0x12);
        set_b_guessed(Flags, CodeLen, 2, 0xff);
        BAsmCode[CodeLen++] = Hi(AdrLong);
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
    }
  }
}

static void DecodeDJNZ(Word Index)
{
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt24, &OK, &Flags);
    SubPCRefReloc();
    if (OK)
    {
      adr_vals_t adr_vals;

      switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg | MModDir8))
      {
        case ModReg:
          if ((OpSize != eSymbolSize8Bit) || (adr_vals.part > 7)) WrError(ErrNum_InvAddrMode);
          else
          {
            AdrLong -= EProgCounter() + 2 + Ord(NeedsPrefix(0xd8 + adr_vals.part));
            if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
            else
            {
              PutCode(0xd8 + adr_vals.part);
              set_b_guessed(Flags, CodeLen, 1, 0xff);
              BAsmCode[CodeLen++] = AdrLong & 0xff;
            }
          }
          break;
        case ModDir8:
          AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xd5));
          if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            PutCode(0xd5);
            append_adr_vals(&adr_vals);
            set_b_guessed(Flags, CodeLen, 1, 0xff);
            BAsmCode[CodeLen++] = Lo(AdrLong);
          }
          break;
      }
    }
  }
}

static void DecodeCJNE(Word Index)
{
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;
  UNUSED(Index);

  if (ChkArgCnt(3, 3))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt24, &OK, &Flags);
    SubPCRefReloc();
    if (OK)
    {
      adr_vals_t adr_vals;

      switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModIReg8 | MModReg))
      {
        case ModAcc:
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModDir8 | MModImm))
          {
            case ModDir8:
              AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb5));
              if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
              else
              {
                PutCode(0xb5);
                append_adr_vals(&adr_vals);
                set_b_guessed(Flags, CodeLen, 1, 0xff);
                BAsmCode[CodeLen++] = AdrLong & 0xff;
              }
              break;
            case ModImm:
              AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb5));
              if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
              else
              {
                PutCode(0xb4);
                append_adr_vals(&adr_vals);
                set_b_guessed(Flags, CodeLen, 1, 0xff);
                BAsmCode[CodeLen++] = AdrLong & 0xff;
              }
              break;
          }
          break;
        case ModReg:
          if ((OpSize != eSymbolSize8Bit) || (adr_vals.part > 7)) WrError(ErrNum_InvAddrMode);
          else
          {
            Byte dest_reg = adr_vals.part;

            switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm))
            {
              case ModImm:
                AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb8 + dest_reg));
                if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
                else
                {
                  PutCode(0xb8 + dest_reg);
                  append_adr_vals(&adr_vals);
                  set_b_guessed(Flags, CodeLen, 1, 0xff);
                  BAsmCode[CodeLen++] = AdrLong & 0xff;
                }
                break;
            }
          }
          break;
        case ModIReg8:
        {
          Byte dest_reg = adr_vals.part;

          if (!SetOpSize(eSymbolSize8Bit))
            return;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm))
          {
            case ModImm:
              AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb6 + dest_reg));
              if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
              else
              {
                PutCode(0xb6 + dest_reg);
                append_adr_vals(&adr_vals);
                set_b_guessed(Flags, CodeLen, 1, 0xff);
                BAsmCode[CodeLen++] = AdrLong & 0xff;
              }
              break;
          }
          break;
        }
      }
    }
  }
}

static void DecodeADD(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModReg))
    {
      case ModAcc:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm | MModDir8 | MModDir16 | MModIReg8 | MModIReg | MModReg))
        {
          case ModImm:
            PutCode(0x24);
            append_adr_vals(&adr_vals);
            break;
          case ModDir8:
            PutCode(0x25);
            append_adr_vals(&adr_vals);
            break;
          case ModDir16:
            PutCode(0x12e);
            BAsmCode[CodeLen++] = (AccReg << 4) + 3;
            append_adr_vals(&adr_vals);
            break;
          case ModIReg8:
            PutCode(0x26 + adr_vals.part);
            break;
          case ModIReg:
            PutCode(0x12e);
            BAsmCode[CodeLen++] = 0x09 + adr_vals.size + (adr_vals.part << 4);
            BAsmCode[CodeLen++] = AccReg << 4;
            break;
          case ModReg:
            if ((adr_vals.part < 8) && (!SrcMode)) PutCode(0x28 + adr_vals.part);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x12c);
              BAsmCode[CodeLen++] = adr_vals.part + (AccReg << 4);
            }
            break;
        }
        break;
      case ModReg:
        if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
        {
          Byte dest_reg = adr_vals.part;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm | MModReg | MModDir8 | MModDir16 | MModIReg8 | MModIReg))
          {
            case ModImm:
              if ((OpSize == eSymbolSize8Bit) && (dest_reg == AccReg))
              {
                PutCode(0x24);
                append_adr_vals(&adr_vals);
              }
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2);
                append_adr_vals(&adr_vals);
              }
              break;
            case ModReg:
              PutCode(0x12c + OpSize);
              if (OpSize == eSymbolSize32Bit) BAsmCode[CodeLen - 1]++;
              BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
              break;
            case ModDir8:
              if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
              else if ((OpSize == 0) && (dest_reg == AccReg))
              {
                PutCode(0x25);
                append_adr_vals(&adr_vals);
              }
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2) + 1;
                append_adr_vals(&adr_vals);
              }
              break;
            case ModDir16:
              if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2) + 3;
                append_adr_vals(&adr_vals);
              }
              break;
            case ModIReg8:
              if ((OpSize != eSymbolSize8Bit) || (dest_reg != AccReg)) WrError(ErrNum_InvAddrMode);
              else PutCode(0x26 + adr_vals.part);
              break;
            case ModIReg:
              if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = 0x09 + adr_vals.size + (adr_vals.part << 4);
                BAsmCode[CodeLen++] = dest_reg << 4;
              }
              break;
          }
        }
        break;
    }
  }
}

static void DecodeSUBCMP(Word Index)
{
  int z;

  /* Index: SUB=0 CMP=1 */

  z = 0x90 + (Index << 5);
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80251))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg))
    {
      case ModReg:
      {
        Byte dest_reg = adr_vals.part;

        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModImm | MModReg | MModDir8 | MModDir16 | MModIReg | (Index ? MModImmEx : 0)))
        {
          case ModImm:
            PutCode(0x10e + z);
            BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2);
            append_adr_vals(&adr_vals);
            break;
          case ModImmEx:
            PutCode(0x10e + z);
            BAsmCode[CodeLen++] = (dest_reg << 4) + 0x0c;
            append_adr_vals(&adr_vals);
            break;
          case ModReg:
            PutCode(0x10c + z + OpSize);
            if (OpSize == eSymbolSize32Bit)
              BAsmCode[CodeLen - 1]++;
            BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
            break;
          case ModDir8:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x10e + z);
              BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2) + 1;
              append_adr_vals(&adr_vals);
            }
            break;
          case ModDir16:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x10e + z);
              BAsmCode[CodeLen++] = (dest_reg << 4) + (OpSize << 2) + 3;
              append_adr_vals(&adr_vals);
            }
            break;
          case ModIReg:
            if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x10e + z);
              BAsmCode[CodeLen++] = 0x09 + adr_vals.size + (adr_vals.part << 4);
              BAsmCode[CodeLen++] = dest_reg << 4;
            }
            break;
        }
        break;
      }
    }
  }
}

static void DecodeADDCSUBB(Word Index)
{
  /* Index: ADDC=0 SUBB=1 */

  if (ChkArgCnt(2, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc))
    {
      case ModAcc:
      {
        Byte opcode = 0x30 + (Index * 0x60);

        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg | MModIReg8 | MModDir8 | MModImm))
        {
          case ModReg:
            if (adr_vals.part > 7) WrError(ErrNum_InvAddrMode);
            else
              PutCode(opcode + 0x08 + adr_vals.part);
            break;
          case ModIReg8:
            PutCode(opcode + 0x06 + adr_vals.part);
            break;
          case ModDir8:
            PutCode(opcode + 0x05);
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            PutCode(opcode + 0x04);
            append_adr_vals(&adr_vals);
            break;
        }
        break;
      }
    }
  }
}

static void DecodeINCDEC(Word Index)
{
  Byte increment;
  int z;
  Boolean OK;
  tSymbolFlags Flags;

  /* Index: INC=0 DEC=1 */

  z = Index << 4;
  if (!ChkArgCnt(1, 2));
  else if ((ArgCnt == 2) && (*ArgStr[2].str.p_str != '#')) WrError(ErrNum_InvAddrMode);
  else
  {
    if (1 == ArgCnt)
    {
      increment = 1;
      OK = True;
      Flags = eSymbolFlag_None;
    }
    else
      increment = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], 1, UInt3, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      increment = 1;
    if (OK)
    {
      OK = True;
      if (increment == 1)
        increment = 0;
      else if (increment == 2)
        increment = 1;
      else if (increment == 4)
        increment = 2;
      else
        OK = False;
      if (!OK) WrError(ErrNum_OverRange);
      else if (!as_strcasecmp(ArgStr[1].str.p_str, "DPTR"))
      {
        if (Index == 1) WrError(ErrNum_InvAddrMode);
        else if (increment != 0) WrError(ErrNum_OverRange);
        else
          PutCode(0xa3);
      }
      else
      {
        adr_vals_t adr_vals;

        switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc | MModReg | MModDir8 | MModIReg8))
        {
          case ModAcc:
            if (increment == 0)
              PutCode(0x04 + z);
            else if (MomCPU < CPU80251) WrError(ErrNum_OverRange);
            else
            {
              PutCode(0x10b + z);
              set_b_guessed(Flags, CodeLen, 1, 0x03);
              BAsmCode[CodeLen++] = (AccReg << 4) + increment;
            }
            break;
          case ModReg:
            if ((OpSize == eSymbolSize8Bit) && (adr_vals.part == AccReg) && (increment == 0))
              PutCode(0x04 + z);
            else if ((adr_vals.part < 8) && (OpSize == eSymbolSize8Bit) && (increment == 0) && (!SrcMode))
              PutCode(0x08 + z + adr_vals.part);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x10b + z);
              set_b_guessed(Flags, CodeLen, 1, 0x03);
              BAsmCode[CodeLen++] = (adr_vals.part << 4) + (OpSize << 2) + increment;
              if (OpSize == eSymbolSize32Bit)
                BAsmCode[CodeLen - 1] += 4;
            }
            break;
          case ModDir8:
            if (increment != 0) WrError(ErrNum_OverRange);
            else
            {
              PutCode(0x05 + z);
              append_adr_vals(&adr_vals);
            }
            break;
          case ModIReg8:
            if (increment != 0) WrError(ErrNum_OverRange);
            else
              PutCode(0x06 + z + adr_vals.part);
            break;
        }
      }
    }
  }
}

static void DecodeMULDIV(Word Index)
{
  int z;

  /* Index: DIV=0 MUL=1 */

  z = Index << 5;
  if (!ChkArgCnt(1, 2));
  else if (ArgCnt == 1)
  {
    if (as_strcasecmp(ArgStr[1].str.p_str, "AB")) WrError(ErrNum_InvAddrMode);
    else
      PutCode(0x84 + z);
  }
  else
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg))
    {
      case ModReg:
      {
        Byte dest_reg = adr_vals.part;

        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg))
        {
          case ModReg:
            if (!ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported));
            else if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x18c + z + OpSize);
              BAsmCode[CodeLen++] = (dest_reg << 4) + adr_vals.part;
            }
            break;
        }
        break;
      }
    }
  }
}

static void DecodeBits(Word Index)
{
  LongInt AdrLong;
  int z;

  /* Index: CPL=0 CLR=1 SETB=2 */

  z = Index << 4;
  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
  {
    if (z == 2) WrError(ErrNum_InvAddrMode);
    else
      PutCode(0xf4 - z);
  }
  else if (IsCarry(ArgStr[1].str.p_str))
    PutCode(0xb3 + z);
  else
  {
    tSymbolFlags flags;
    switch (DecodeBitAdr(&ArgStr[1], &AdrLong, &flags, True))
    {
      case ModBit51:
        PutCode(0xb2 + z);
        set_b_guessed(flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
      case ModBit251:
        PutCode(0x1a9);
        set_b_guessed(flags, CodeLen, 1, 0x07);
        BAsmCode[CodeLen++] = 0xb0 + z + (AdrLong >> 24);
        set_b_guessed(flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
    }
  }
}

static void DecodeShift(Word Index)
{
  int z;

  /* Index: SRA=0 SRL=1 SLL=3 */

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80251))
  {
    adr_vals_t adr_vals;

    z = Index << 4;
    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg))
    {
      case ModReg:
        if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x10e + z);
          BAsmCode[CodeLen++] = (adr_vals.part << 4) + (OpSize << 2);
        }
        break;
    }
  }
}

static void DecodeCond(Word Index)
{
  FixedOrder *FixedZ = CondOrders + Index;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(FixedZ->MinCPU))
  {
    tEvalResult EvalResult;
    LongInt AdrLong = EvalStrIntExpressionWithResult(&ArgStr[1], UInt24, &EvalResult);

    SubPCRefReloc();
    if (EvalResult.OK)
    {
      AdrLong -= EProgCounter() + 2 + Ord(NeedsPrefix(FixedZ->Code));
      if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        PutCode(FixedZ->Code);
        set_b_guessed(EvalResult.Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
      }
    }
  }
}

static void DecodeBCond(Word Index)
{
  FixedOrder *FixedZ = BCondOrders + Index;

  if (ChkArgCnt(2, 2))
  {
    tEvalResult EvalResult;
    LongInt AdrLong = EvalStrIntExpressionWithResult(&ArgStr[2], UInt24, &EvalResult);

    SubPCRefReloc();
    if (EvalResult.OK)
    {
      LongInt BitLong;
      tSymbolFlags bit_flags;

      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      switch (DecodeBitAdr(&ArgStr[1], &BitLong, &bit_flags, True))
      {
        case ModBit51:
          AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(FixedZ->Code));
          if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            PutCode(FixedZ->Code);
            set_b_guessed(bit_flags, CodeLen, 1, 0xff);
            BAsmCode[CodeLen++] = BitLong & 0xff;
            set_b_guessed(EvalResult.Flags, CodeLen, 1, 0xff);
            BAsmCode[CodeLen++] = AdrLong & 0xff;
          }
          break;
        case ModBit251:
          AdrLong -= EProgCounter() + 4 + Ord(NeedsPrefix(0x1a9));
          if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            PutCode(0x1a9);
            set_b_guessed(bit_flags, CodeLen, 1, 0x07);
            BAsmCode[CodeLen++] = FixedZ->Code + (BitLong >> 24);
            set_b_guessed(bit_flags, CodeLen, 1, 0xff);
            BAsmCode[CodeLen++] = BitLong & 0xff;
            set_b_guessed(EvalResult.Flags, CodeLen, 1, 0xff);
            BAsmCode[CodeLen++] = AdrLong & 0xff;
          }
          break;
      }
    }
  }
}

static void DecodeAcc(Word Index)
{
  FixedOrder *FixedZ = AccOrders + Index;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(FixedZ->MinCPU))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAcc))
    {
      case ModAcc:
        PutCode(FixedZ->Code);
        break;
    }
  }
}

static void DecodeFixed(Word Index)
{
  FixedOrder *FixedZ = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && ChkMinCPU(FixedZ->MinCPU))
    PutCode(FixedZ->Code);
}


static void DecodeSFR(Word is_sfrb)
{
  Word reg_address;
  Boolean OK;
  tSymbolFlags Flags;
  as_addrspace_t DSeg;

  if (ChkArgCnt(1, 1))
  {
    reg_address = EvalStrIntExpressionWithFlags(&ArgStr[1], (MomCPU >= CPU80251) ? UInt9 : UInt8, &OK, &Flags);
    if (OK && !mFirstPassUnknown(Flags))
    {
      PushLocHandle(-1);
      DSeg = (MomCPU >= CPU80251) ? SegIO : SegData;
      EnterIntSymbol(&LabPart, reg_address, DSeg, False);
      if (MakeUseList)
      {
        if (AddChunk(SegChunks + DSeg, reg_address, 1, False))
          WrError(ErrNum_Overlap);
      }
      if (is_sfrb)
      {
        Byte BitStart;

        if (reg_address > 0x7f)
        {
          if ((reg_address & 7) != 0) WrError(ErrNum_NotBitAddressable);
          BitStart = reg_address;
        }
        else
        {
          if ((reg_address & 0xe0) != 0x20) WrError(ErrNum_NotBitAddressable);
          BitStart = (reg_address - 0x20) << 3;
        }
        if (MakeUseList)
          if (AddChunk(SegChunks + SegBData, BitStart, 8, False)) WrError(ErrNum_Overlap);
        as_snprintf(ListLine, STRINGSIZE, "=%~02.*u%s-%~02.*u%s",
                    ListRadixBase, (unsigned)BitStart, GetIntConstIntelSuffix(ListRadixBase),
                    ListRadixBase, (unsigned)BitStart + 7, GetIntConstIntelSuffix(ListRadixBase));
      }
      else
        as_snprintf(ListLine, STRINGSIZE, "=%~02.*u%s",
                    ListRadixBase, (unsigned)reg_address, GetIntConstIntelSuffix(ListRadixBase));
      PopLocHandle();
    }
  }
}

static void DecodeBIT(Word Index)
{
  LongInt AdrLong;
  tSymbolFlags flags;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (MomCPU >= CPU80251)
  {
    if (DecodeBitAdr(&ArgStr[1], &AdrLong, &flags, False) == ModBit251)
    {
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, AdrLong, SegBData, False);
      PopLocHandle();
      *ListLine = '=';
      DissectBit_251(ListLine + 1, STRINGSIZE - 1, AdrLong);
    }
  }
  else
  {
    if (DecodeBitAdr(&ArgStr[1], &AdrLong, &flags, False) == ModBit51)
    {
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, AdrLong, SegBData, False);
      PopLocHandle();
      as_snprintf(ListLine, STRINGSIZE, "=%~02.*u%s",
                  ListRadixBase, (unsigned)AdrLong, GetIntConstIntelSuffix(ListRadixBase));
    }
  }
}

static void DecodePORT(Word Index)
{
  UNUSED(Index);

  if (ChkMinCPU(CPU80251))
    code_equate_type(SegIO, UInt9);
}

/*-------------------------------------------------------------------------*/
/* dynamische Codetabellenverwaltung */

static void AddFixed(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddAcc(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(AccOrders, FixedOrder);
  AccOrders[InstrZ].Code = NCode;
  AccOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeAcc);
}

static void AddCond(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(CondOrders, FixedOrder);
  CondOrders[InstrZ].Code = NCode;
  CondOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCond);
}

static void AddBCond(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(BCondOrders, FixedOrder);
  BCondOrders[InstrZ].Code = NCode;
  BCondOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeBCond);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "MOV"  , 0, DecodeMOV);
  AddInstTable(InstTable, "ANL"  , 1, DecodeLogic);
  AddInstTable(InstTable, "ORL"  , 0, DecodeLogic);
  AddInstTable(InstTable, "XRL"  , 2, DecodeLogic);
  AddInstTable(InstTable, "MOVC" , 0, DecodeMOVC);
  AddInstTable(InstTable, "MOVH" , 0, DecodeMOVH);
  AddInstTable(InstTable, "MOVZ" , 0x00, DecodeMOVZS);
  AddInstTable(InstTable, "MOVS" , 0x10, DecodeMOVZS);
  AddInstTable(InstTable, "MOVX" , 0, DecodeMOVX);
  AddInstTable(InstTable, "POP"  , 1, DecodeStack);
  AddInstTable(InstTable, "PUSH" , 0, DecodeStack);
  AddInstTable(InstTable, "PUSHW", 2, DecodeStack);
  AddInstTable(InstTable, "XCH"  , 0, DecodeXCH);
  AddInstTable(InstTable, "XCHD" , 0, DecodeXCHD);
  AddInstTable(InstTable, "AJMP" , 0, DecodeABranch);
  AddInstTable(InstTable, "ACALL", 1, DecodeABranch);
  AddInstTable(InstTable, "LJMP" , 0, DecodeLBranch);
  AddInstTable(InstTable, "LCALL", 1, DecodeLBranch);
  AddInstTable(InstTable, "EJMP" , 0, DecodeEBranch);
  AddInstTable(InstTable, "ECALL", 1, DecodeEBranch);
  AddInstTable(InstTable, "JMP"  , 0, DecodeJMP);
  AddInstTable(InstTable, "CALL" , 0, DecodeCALL);
  AddInstTable(InstTable, "DJNZ" , 0, DecodeDJNZ);
  AddInstTable(InstTable, "CJNE" , 0, DecodeCJNE);
  AddInstTable(InstTable, "ADD"  , 0, DecodeADD);
  AddInstTable(InstTable, "SUB"  , 0, DecodeSUBCMP);
  AddInstTable(InstTable, "CMP"  , 1, DecodeSUBCMP);
  AddInstTable(InstTable, "ADDC" , 0, DecodeADDCSUBB);
  AddInstTable(InstTable, "SUBB" , 1, DecodeADDCSUBB);
  AddInstTable(InstTable, "INC"  , 0, DecodeINCDEC);
  AddInstTable(InstTable, "DEC"  , 1, DecodeINCDEC);
  AddInstTable(InstTable, "MUL"  , 1, DecodeMULDIV);
  AddInstTable(InstTable, "DIV"  , 0, DecodeMULDIV);
  AddInstTable(InstTable, "CLR"  , 1, DecodeBits);
  AddInstTable(InstTable, "CPL"  , 0, DecodeBits);
  AddInstTable(InstTable, "SETB" , 2, DecodeBits);
  AddInstTable(InstTable, "SRA"  , 0, DecodeShift);
  AddInstTable(InstTable, "SRL"  , 1, DecodeShift);
  AddInstTable(InstTable, "SLL"  , 3, DecodeShift);
  AddInstTable(InstTable, "SFR"  , 0, DecodeSFR);
  if (MomCPU <= CPU80C390)
    AddInstTable(InstTable, "SFRB" , 1, DecodeSFR);
  AddInstTable(InstTable, "BIT"  , 0, DecodeBIT);
  AddInstTable(InstTable, "PORT" , 0, DecodePORT);

  InstrZ = 0;
  AddFixed("NOP" , 0x0000, CPU87C750);
  AddFixed("RET" , 0x0022, CPU87C750);
  AddFixed("RETI", 0x0032, CPU87C750);
  AddFixed("ERET", 0x01aa, CPU80251);
  AddFixed("TRAP", 0x01b9, CPU80251);

  InstrZ = 0;
  AddAcc("DA"  , 0x00d4, CPU87C750);
  AddAcc("RL"  , 0x0023, CPU87C750);
  AddAcc("RLC" , 0x0033, CPU87C750);
  AddAcc("RR"  , 0x0003, CPU87C750);
  AddAcc("RRC" , 0x0013, CPU87C750);
  AddAcc("SWAP", 0x00c4, CPU87C750);

  InstrZ = 0;
  AddCond("JC"  , 0x0040, CPU87C750);
  AddCond("JE"  , 0x0168, CPU80251);
  AddCond("JG"  , 0x0138, CPU80251);
  AddCond("JLE" , 0x0128, CPU80251);
  AddCond("JNC" , 0x0050, CPU87C750);
  AddCond("JNE" , 0x0178, CPU80251);
  AddCond("JNZ" , 0x0070, CPU87C750);
  AddCond("JSG" , 0x0118, CPU80251);
  AddCond("JSGE", 0x0158, CPU80251);
  AddCond("JSL" , 0x0148, CPU80251);
  AddCond("JSLE", 0x0108, CPU80251);
  AddCond("JZ"  , 0x0060, CPU87C750);
  AddCond("SJMP", 0x0080, CPU87C750);

  InstrZ = 0;
  AddBCond("JB" , 0x0020, CPU87C750);
  AddBCond("JBC", 0x0010, CPU87C750);
  AddBCond("JNB", 0x0030, CPU87C750);

  AddInstTable(InstTable, "REG"  , 0, CodeREG);
  AddIntelPseudo(InstTable, eIntPseudoFlag_DynEndian);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(AccOrders);
  order_array_free(CondOrders);
  order_array_free(BCondOrders);
}

/*-------------------------------------------------------------------------*/
/* Instruktionsdecoder */

static void MakeCode_51(void)
{
  OpSize = eSymbolSizeUnknown;
  MinOneIs0 = False;

  /* zu ignorierendes */

  if (*OpPart.str.p_str == '\0') return;

  /* suchen */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_51(void)
{
  switch (*OpPart.str.p_str)
  {
    case 'B':
      return Memo("BIT");
    case 'S':
      if (Memo("SFR")) return True;
      if (MomCPU >= CPU80251) return False;
      return Memo("SFRB");
    case 'P':
      return (MomCPU >= CPU80251) ? Memo("PORT") : False;
    case 'R':
      return Memo("REG");
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_51(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on 80x51
 * \param  pArg source argument
 * \param  pResult destination buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_51(char *pArg, TempResult *pResult)
{
  tRegInt Erg;
  tSymbolSize Size;

  if (DecodeRegCore(pArg, &Erg, &Size))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = (tSymbolSize)Size;
    pResult->Contents.RegDescr.Reg = Erg;
    pResult->Contents.RegDescr.Dissect = DissectReg_51;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void SwitchTo_51(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("MCS-(2)51");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  /* C251 is entirely different... */

  if (MomCPU >= CPU80251)
  {
    ValidSegs = (1 << SegCode) | (1 << SegIO);
    Grans[SegCode ] = 1; ListGrans[SegCode ] = 1; SegInits[SegCode ] = 0;
    SegLimits[SegCode ] = 0xffffffl;
    Grans[SegIO   ] = 1; ListGrans[SegIO   ] = 1; SegInits[SegIO   ] = 0;
    SegLimits[SegIO   ] = 0x1ff;
    DissectBit = DissectBit_251;
  }

  /* rest of the pack... */

  else
  {
    ValidSegs=(1 << SegCode) | (1 << SegData) | (1 << SegIData) | (1 << SegXData) | (1 << SegBData);

    Grans[SegCode ] = 1; ListGrans[SegCode ] = 1; SegInits[SegCode ] = 0;
    if (MomCPU == CPU80C390)
      SegLimits[SegCode ] = 0xffffff;
    else if (MomCPU == CPU87C750)
      SegLimits[SegCode ] = 0x7ff;
    else
      SegLimits[SegCode ] = 0xffff;


    Grans[SegXData] = 1; ListGrans[SegXData] = 1; SegInits[SegXData] = 0;
    if (MomCPU == CPU80C390)
      SegLimits[SegXData] = 0xffffff;
    else
      SegLimits[SegXData] = 0xffff;

    Grans[SegData ] = 1; ListGrans[SegData ] = 1; SegInits[SegData ] = 0x30;
    SegLimits[SegData ] = 0xff;
    Grans[SegIData] = 1; ListGrans[SegIData] = 1; SegInits[SegIData] = 0x80;
    SegLimits[SegIData] = 0xff;
    Grans[SegBData] = 1; ListGrans[SegBData] = 1; SegInits[SegBData] = 0;
    SegLimits[SegBData] = 0xff;
  }

  MakeCode = MakeCode_51;
  IsDef = IsDef_51;
  InternSymbol = InternSymbol_51;
  DissectReg = DissectReg_51;

  InitFields();
  SwitchFrom = DeinitFields;
  if (!onoff_test_and_set(e_onoff_reg_srcmode))
    SetFlag(&SrcMode, SrcModeSymName, False);
  AddONOFF(SrcModeCmdName, &SrcMode, SrcModeSymName, False);
  onoff_bigendian_add();
}

void code51_init(void)
{
  CPU87C750 = AddCPU("87C750", SwitchTo_51);
  CPU8051   = AddCPU("8051"  , SwitchTo_51);
  CPU8052   = AddCPU("8052"  , SwitchTo_51);
  CPU80C320 = AddCPU("80C320", SwitchTo_51);
  CPU80501  = AddCPU("80C501", SwitchTo_51);
  CPU80502  = AddCPU("80C502", SwitchTo_51);
  CPU80504  = AddCPU("80C504", SwitchTo_51);
  CPU80515  = AddCPU("80515" , SwitchTo_51);
  CPU80517  = AddCPU("80517" , SwitchTo_51);
  CPU80C390 = AddCPU("80C390", SwitchTo_51);
  CPU80251  = AddCPU("80C251", SwitchTo_51);
  CPU80251T = AddCPU("80C251T", SwitchTo_51);
}
