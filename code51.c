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

#include "code51.h"

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

static Byte AdrVals[5];
static Byte AdrPart, AdrSize;
static ShortInt AdrMode,OpSize;
static Boolean MinOneIs0;

static Boolean SrcMode;

static CPUVar CPU87C750, CPU8051, CPU8052, CPU80C320,
       CPU80501, CPU80502, CPU80504, CPU80515, CPU80517,
       CPU80C390,
       CPU80251, CPU80251T;

static PRelocEntry AdrRelocInfo, BackupAdrRelocInfo;
static LongWord AdrOffset, AdrRelocType,
                BackupAdrOffset, BackupAdrRelocType;

/*-------------------------------------------------------------------------*/
/* Adressparser */

static void SetOpSize(ShortInt NewSize)
{
  if (OpSize == -1)
    OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes);
    AdrMode = ModNone;
    AdrCnt = 0;
  }
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
  Boolean IO;

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

  *pValue = ConstLongInt(Start, &IO, 10);
  if (!IO) return False;
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

static void SaveAdrRelocs(LongWord Type, LongWord Offset)
{
  AdrOffset = Offset;
  AdrRelocType = Type;
  AdrRelocInfo = LastRelocs;
  LastRelocs = NULL;
}

static void SaveBackupAdrRelocs(void)
{
  BackupAdrOffset = AdrOffset;
  BackupAdrRelocType = AdrRelocType;
  BackupAdrRelocInfo = AdrRelocInfo;
  AdrRelocInfo = NULL;
}

static void TransferAdrRelocs(LongWord Offset)
{
  TransferRelocs2(AdrRelocInfo, ProgCounter() + AdrOffset + Offset, AdrRelocType);
  AdrRelocInfo = NULL;
}

static void TransferBackupAdrRelocs(LargeWord Offset)
{
  TransferRelocs2(BackupAdrRelocInfo, ProgCounter() + BackupAdrOffset + Offset, BackupAdrRelocType);
  AdrRelocInfo = NULL;
}

static void DecodeAdr(tStrComp *pArg, Word Mask)
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

  AdrMode = ModNone; AdrCnt = 0;

  ExtMask = MMod251 & Mask;
  if (MomCPU < CPU80251) Mask &= MMod51;

  if (!*pArg->str.p_str)
     return;

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    if (!(Mask & MModAcc))
    {
      AdrMode = ModReg;
      AdrPart = AccReg;
    }
    else
      AdrMode = ModAcc;
    SetOpSize(0);
    goto chk;
  }

  if (*pArg->str.p_str == '#')
  {
    tStrComp Comp;

    StrCompRefRight(&Comp, pArg, 1);
    if ((OpSize == -1) && (MinOneIs0)) OpSize = 0;
    switch (OpSize)
    {
      case -1:
        WrError(ErrNum_UndefOpSizes);
        break;
      case 0:
        AdrVals[0] = EvalStrIntExpression(&Comp, Int8, &OK);
        if (OK)
        {
          AdrMode = ModImm;
          AdrCnt = 1;
          SaveAdrRelocs(RelocTypeB8, 0);
        }
        break;
      case 1:
        H16 = EvalStrIntExpression(&Comp, Int16, &OK);
        if (OK)
        {
          AdrVals[0] = Hi(H16);
          AdrVals[1] = Lo(H16);
          AdrMode = ModImm;
          AdrCnt = 2;
          SaveAdrRelocs(RelocTypeB16, 0);
        }
        break;
      case 2:
        H32 = EvalStrIntExpressionWithResult(&Comp, Int32, &EvalResult);
        if (mFirstPassUnknown(EvalResult.Flags))
          H32 &= 0xffff;
        if (EvalResult.OK)
        {
          AdrVals[1] = H32 & 0xff;
          AdrVals[0] = (H32 >> 8) & 0xff;
          H32 >>= 16;
          if (H32 == 0)
            AdrMode = ModImm;
          else if ((H32 == 1) || (H32 == 0xffff))
            AdrMode = ModImmEx;
          else
            WrError(ErrNum_UndefOpSizes);
          if (AdrMode != ModNone)
            AdrCnt = 2;
          SaveAdrRelocs(RelocTypeB16, 0);
        }
        break;
      case 3:
        H32 = EvalStrIntExpression(&Comp, Int24, &OK);
        if (OK)
        {
          AdrVals[0] = (H32 >> 16) & 0xff;
          AdrVals[1] = (H32 >> 8) & 0xff;
          AdrVals[2] = H32 & 0xff;
          AdrCnt = 3;
          AdrMode = ModImm;
          SaveAdrRelocs(RelocTypeB24, 0);
        }
        break;
    }
    goto chk;
  }

  switch (DecodeReg(pArg, &AdrPart, &HSize, False))
  {
    case eIsReg:
      if ((MomCPU >= CPU80251) && ((Mask & MModReg) == 0))
        AdrMode = ((HSize == 0) && (AdrPart == AccReg)) ? ModAcc : ModReg;
      else
        AdrMode = ModReg;
      SetOpSize(HSize);
      goto chk;
    case eIsNoReg:
      break;
    case eRegAbort:
      return;
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
    switch (DecodeReg(&IndirComp, &AdrPart, &HSize, False))
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
          H32 = EvalStrIntExpression(&DispComp, SInt16, &OK);
        }
        if (OK)
          switch (HSize)
          {
            case eSymbolSize8Bit:
              if ((AdrPart>1) || (H32 != 0)) WrError(ErrNum_InvAddrMode);
              else
                AdrMode = ModIReg8;
              break;
            case eSymbolSize16Bit:
              if (H32 == 0)
              {
                AdrMode = ModIReg;
                AdrSize = 0;
              }
              else
              {
                AdrMode = ModInd;
                AdrSize = 0;
                AdrVals[1] = H32 & 0xff;
                AdrVals[0] = (H32 >> 8) & 0xff;
                AdrCnt = 2;
              }
              break;
            case eSymbolSize32Bit:
              if (H32 == 0)
              {
                AdrMode = ModIReg;
                AdrSize = 2;
              }
              else
              {
                AdrMode = ModInd;
                AdrSize = 2;
                AdrVals[1] = H32 & 0xff;
                AdrVals[0] = (H32 >> 8) & 0xff;
                AdrCnt = 2;
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
      return;
    }
    else
    {
      if (!as_strcasecmp(SegComp.str.p_str, "S"))
        SegType = -2;
      else
      {
        SegType = EvalStrIntExpressionWithResult(&SegComp, UInt8, &EvalResult);
        if (!EvalResult.OK)
          return;
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
    return;

  if ((SegType == -2) || ((SegType == -1) && (EvalResult.AddrSpaceMask & (1 << SegIO))))
  {
    if (ChkRange(H32, 0x80, 0xff))
    {
      SaveAdrRelocs(RelocTypeB8, 0);
      AdrMode = ModDir8;
      AdrVals[0] = H32 & 0xff;
      AdrCnt = 1;
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
      SaveAdrRelocs(RelocTypeB8, 0);
      AdrMode = ModDir8;
      AdrVals[0] = H32 &0xff;
      AdrCnt = 1;
    }
    else if ((MomCPU < CPU80251) || (H32 > 0xffff)) WrError(ErrNum_AdrOverflow);
    else
    {
      AdrMode = ModDir16;
      AdrCnt = 2;
      AdrVals[1] = H32 & 0xff;
      AdrVals[0] = (H32 >> 8) & 0xff;
    }
  }

chk:
  if ((AdrMode != ModNone) && ((Mask & (1 << AdrMode)) == 0))
  {
    if (ExtMask & (1 << AdrMode))
      (void)ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported);
    else
      WrError(ErrNum_InvAddrMode);
    AdrCnt = 0;
    AdrMode = ModNone;
  }
}

static void DissectBit_251(char *pDest, size_t DestSize, LargeWord Inp)
{
  as_snprintf(pDest, DestSize, "%~02.*u%s.%u",
              ListRadixBase, (unsigned)(Inp & 0xff), GetIntConstIntelSuffix(ListRadixBase),
              (unsigned)(Inp >> 24));
}

static ShortInt DecodeBitAdr(tStrComp *pArg, LongInt *Erg, Boolean MayShorten)
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
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
        Save = EvalStrIntExpressionWithResult(&BitPart, UInt3, &EvalResult);
        if (!EvalResult.OK) return ModNone;
        else
        {
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
    }
    else
    {
      DecodeAdr(&RegPart, MModDir8);
      *pPos = Save;
      if (AdrMode == ModNone)
        EvalResult.OK = False;
      else
      {
        *Erg = EvalStrIntExpressionWithResult(&BitPart, UInt3, &EvalResult) << 24;
        if (EvalResult.OK)
          (*Erg) += AdrVals[0];
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
  Byte HSize, HReg;
  Integer AdrInt;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (IsCarry(ArgStr[1].str.p_str))
  {
    switch (DecodeBitAdr(&ArgStr[2], &AdrLong, True))
    {
      case ModBit51:
        PutCode(0xa2);
        BAsmCode[CodeLen] = AdrLong & 0xff;
        CodeLen++;
        break;
      case ModBit251:
        PutCode(0x1a9);
        BAsmCode[CodeLen  ] = 0xa0 + (AdrLong >> 24);
        BAsmCode[CodeLen + 1] = AdrLong & 0xff;
        CodeLen+=2;
        break;
    }
  }
  else if ((!as_strcasecmp(ArgStr[2].str.p_str, "C")) || (!as_strcasecmp(ArgStr[2].str.p_str, "CY")))
  {
    switch (DecodeBitAdr(&ArgStr[1], &AdrLong, True))
    {
      case ModBit51:
        PutCode(0x92);
        BAsmCode[CodeLen] = AdrLong & 0xff;
        CodeLen++;
        break;
      case ModBit251:
        PutCode(0x1a9);
        BAsmCode[CodeLen] = 0x90 + (AdrLong >> 24);
        BAsmCode[CodeLen + 1] = AdrLong & 0xff;
        CodeLen+=2;
        break;
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "DPTR"))
  {
    SetOpSize((MomCPU == CPU80C390) ? 3 : 1);
    DecodeAdr(&ArgStr[2], MModImm);
    switch (AdrMode)
    {
      case ModImm:
        PutCode(0x90);
        memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
        TransferAdrRelocs(CodeLen);
        CodeLen += AdrCnt;
        break;
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModIReg8 | MModIReg | MModInd | MModDir8 | MModDir16);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModIReg | MModInd | MModDir8 | MModDir16 | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            if ((AdrPart < 8) && (!SrcMode))
              PutCode(0xe8 + AdrPart);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17c);
              BAsmCode[CodeLen++] = (AccReg << 4) + AdrPart;
            }
            break;
          case ModIReg8:
            PutCode(0xe6 + AdrPart);
            break;
          case ModIReg:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = (AdrPart << 4) + 0x09 + AdrSize;
            BAsmCode[CodeLen++] = (AccReg << 4);
            break;
          case ModInd:
            PutCode(0x109 + (AdrSize << 4));
            BAsmCode[CodeLen++] = (AccReg << 4) + AdrPart;
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModDir8:
            PutCode(0xe5);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
          case ModDir16:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = (AccReg << 4) + 0x03;
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModImm:
            PutCode(0x74);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
      case ModReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModIReg | MModInd | MModDir8 | MModDir16 | MModImm | MModImmEx);
        switch (AdrMode)
        {
          case ModReg:
            if ((OpSize == 0) && (AdrPart == AccReg) && (HReg < 8))
              PutCode(0xf8 + HReg);
            else if ((OpSize == 0) && (HReg == AccReg) && (AdrPart < 8))
              PutCode(0xe8 + AdrPart);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17c + OpSize);
              if (OpSize == 2)
                BAsmCode[CodeLen - 1]++;
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            }
            break;
          case ModIReg8:
            if ((OpSize != 0) || (HReg != AccReg)) WrError(ErrNum_InvAddrMode);
            else
              PutCode(0xe6 + AdrPart);
            break;
          case ModIReg:
            if (OpSize == 0)
            {
              PutCode(0x17e);
              BAsmCode[CodeLen++] = (AdrPart << 4) + 0x09 + AdrSize;
              BAsmCode[CodeLen++] = HReg << 4;
            }
            else if (OpSize == 1)
            {
              PutCode(0x10b);
              BAsmCode[CodeLen++] = (AdrPart << 4) + 0x08 + AdrSize;
              BAsmCode[CodeLen++] = HReg << 4;
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
          case ModInd:
            if (OpSize == 2) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x109 + (AdrSize << 4) + (OpSize << 6));
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
            }
            break;
          case ModDir8:
            if ((OpSize == 0) && (HReg == AccReg))
            {
              PutCode(0xe5);
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            else if ((OpSize == 0) && (HReg < 8) && (!SrcMode))
            {
              PutCode(0xa8 + HReg);
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17e);
              BAsmCode[CodeLen++] = 0x01 + (HReg << 4) + (OpSize << 2);
              if (OpSize == 2)
                BAsmCode[CodeLen - 1] += 4;
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            break;
          case ModDir16:
            PutCode(0x17e);
            BAsmCode[CodeLen++] = 0x03 + (HReg << 4) + (OpSize << 2);
            if (OpSize == 2)
              BAsmCode[CodeLen - 1] += 4;
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModImm:
            if ((OpSize == 0) && (HReg == AccReg))
            {
              PutCode(0x74);
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            else if ((OpSize == 0) && (HReg < 8) && (!SrcMode))
            {
              PutCode(0x78 + HReg);
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17e);
              BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2);
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
            }
            break;
          case ModImmEx:
            PutCode(0x17e);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = 0x0c + (HReg << 4);
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
        }
        break;
      case ModIReg8:
        SetOpSize(0); HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModAcc | MModDir8 | MModImm);
        switch (AdrMode)
        {
          case ModAcc:
            PutCode(0xf6 + HReg);
            break;
          case ModDir8:
            PutCode(0xa6 + HReg);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
          case ModImm:
            PutCode(0x76 + HReg);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
      case ModIReg:
        HReg = AdrPart; HSize = AdrSize;
        DecodeAdr(&ArgStr[2], MModReg);
        switch (AdrMode)
        {
          case ModReg:
            if (OpSize == 0)
            {
              PutCode(0x17a);
              BAsmCode[CodeLen++] = (HReg << 4) + 0x09 + HSize;
              BAsmCode[CodeLen++] = AdrPart << 4;
            }
            else if (OpSize == 1)
            {
              PutCode(0x11b);
              BAsmCode[CodeLen++] = (HReg << 4) + 0x08 + HSize;
              BAsmCode[CodeLen++] = AdrPart << 4;
            }
            else
              WrError(ErrNum_InvAddrMode);
        }
        break;
      case ModInd:
        HReg = AdrPart; HSize = AdrSize;
        AdrInt = (((Word)AdrVals[0]) << 8) + AdrVals[1];
        DecodeAdr(&ArgStr[2], MModReg);
        switch (AdrMode)
        {
          case ModReg:
            if (OpSize == 2) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x119 + (HSize << 4) + (OpSize << 6));
              BAsmCode[CodeLen++] = (AdrPart << 4) + HReg;
              BAsmCode[CodeLen++] = Hi(AdrInt);
              BAsmCode[CodeLen++] = Lo(AdrInt);
            }
        }
        break;
      case ModDir8:
        MinOneIs0 = True;
        HReg = AdrVals[0];
        SaveBackupAdrRelocs();
        DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModDir8 | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            if ((OpSize == 0) && (AdrPart == AccReg))
            {
              PutCode(0xf5);
              TransferBackupAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = HReg;
            }
            else if ((OpSize == 0) && (AdrPart < 8) && (!SrcMode))
            {
              PutCode(0x88 + AdrPart);
              TransferBackupAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = HReg;
            }
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x17a);
              BAsmCode[CodeLen++] = 0x01 + (AdrPart << 4) + (OpSize << 2);
              if (OpSize == 2)
                BAsmCode[CodeLen - 1] += 4;
              BAsmCode[CodeLen++] = HReg;
            }
            break;
          case ModIReg8:
            PutCode(0x86 + AdrPart);
            TransferBackupAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = HReg;
            break;
          case ModDir8:
            PutCode(0x85);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            TransferBackupAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = HReg;
            break;
          case ModImm:
            PutCode(0x75);
            TransferBackupAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = HReg;
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
      case ModDir16:
        AdrInt = (((Word)AdrVals[0]) << 8) + AdrVals[1];
        DecodeAdr(&ArgStr[2], MModReg);
        switch (AdrMode)
        {
          case ModReg:
            PutCode(0x17a);
            BAsmCode[CodeLen++] = 0x03 + (AdrPart << 4) + (OpSize << 2);
            if (OpSize == 2) BAsmCode[CodeLen - 1] += 4;
            BAsmCode[CodeLen++] = Hi(AdrInt);
            BAsmCode[CodeLen++] = Lo(AdrInt);
            break;
        }
        break;
    }
  }
}

static void DecodeLogic(Word Index)
{
  Byte HReg;
  LongInt AdrLong;
  int z;

  /* Index: ORL=0 ANL=1 XRL=2 */

  if (!ChkArgCnt(2, 2));
  else if (IsCarry(ArgStr[1].str.p_str))
  {
    if (Index == 2) WrError(ErrNum_InvAddrMode);
    else
    {
      Boolean InvFlag;
      ShortInt Result;

      HReg = Index << 4;
      InvFlag = *ArgStr[2].str.p_str == '/';
      if (InvFlag)
      {
        tStrComp Comp;

        StrCompRefRight(&Comp, &ArgStr[2], 1);
        Result = DecodeBitAdr(&Comp, &AdrLong, True);
      }
      else
        Result = DecodeBitAdr(&ArgStr[2], &AdrLong, True);
      switch (Result)
      {
        case ModBit51:
          PutCode((InvFlag) ? 0xa0 + HReg : 0x72 + HReg);
          BAsmCode[CodeLen++] = AdrLong & 0xff;
          break;
        case ModBit251:
          PutCode(0x1a9);
          BAsmCode[CodeLen++] = ((InvFlag) ? 0xe0 : 0x70) + HReg + (AdrLong >> 24);
          BAsmCode[CodeLen++] = AdrLong & 0xff;
          break;
      }
    }
  }
  else
  {
    z = (Index << 4) + 0x40;
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModDir8);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModIReg | MModDir8 | MModDir16 | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            if ((AdrPart < 8) && (!SrcMode)) PutCode(z + 8 + AdrPart);
            else
            {
              PutCode(z + 0x10c);
              BAsmCode[CodeLen++] = AdrPart + (AccReg << 4);
            }
            break;
          case ModIReg8:
            PutCode(z + 6 + AdrPart);
            break;
          case ModIReg:
            PutCode(z + 0x10e);
            BAsmCode[CodeLen++] = 0x09 + AdrSize + (AdrPart << 4);
            BAsmCode[CodeLen++] = AccReg << 4;
            break;
          case ModDir8:
            PutCode(z + 0x05);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
          case ModDir16:
            PutCode(0x10e + z);
            BAsmCode[CodeLen++] = 0x03 + (AccReg << 4);
            memcpy(BAsmCode+CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModImm:
            PutCode(z + 0x04);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
      case ModReg:
        if (MomCPU < CPU80251) WrError(ErrNum_InvAddrMode);
        else
        {
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModIReg | MModDir8 | MModDir16 | MModImm);
          switch (AdrMode)
          {
            case ModReg:
              if (OpSize == 2) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(z + 0x10c + OpSize);
                BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              }
              break;
            case ModIReg8:
              if ((OpSize != 0) || (HReg != AccReg)) WrError(ErrNum_InvAddrMode);
              else
                PutCode(z + 0x06 + AdrPart);
              break;
            case ModIReg:
              if (OpSize != 0) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = 0x09 + AdrSize + (AdrPart << 4);
                BAsmCode[CodeLen++] = HReg << 4;
              }
              break;
            case ModDir8:
              if ((OpSize == 0) && (HReg == AccReg))
              {
                PutCode(0x05 + z);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
              }
              else if (OpSize == 2) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2) + 1;
                BAsmCode[CodeLen++] = AdrVals[0];
              }
              break;
            case ModDir16:
              if (OpSize == 2) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2) + 3;
                memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
                CodeLen += AdrCnt;
              }
              break;
            case ModImm:
              if ((OpSize == 0) && (HReg == AccReg))
              {
                PutCode(0x04 + z);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
              }
              else if (OpSize == 2) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x10e + z);
                BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2);
                TransferAdrRelocs(CodeLen);
                memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
                CodeLen += AdrCnt;
              }
              break;
          }
        }
        break;
      case ModDir8:
        HReg = AdrVals[0];
        SaveBackupAdrRelocs();
        SetOpSize(0);
        DecodeAdr(&ArgStr[2], MModAcc | MModImm);
        switch (AdrMode)
        {
          case ModAcc:
            PutCode(z + 0x02);
            TransferBackupAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = HReg;
            break;
          case ModImm:
            PutCode(z + 0x03);
            TransferBackupAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = HReg;
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
    }
  }
}

static void DecodeMOVC(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc);
    switch (AdrMode)
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
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80251))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        if (OpSize != 2) WrError(ErrNum_InvAddrMode);
        else
        {
          HReg = AdrPart;
          OpSize--;
          DecodeAdr(&ArgStr[2], MModImm);
          switch (AdrMode)
          {
            case ModImm:
              PutCode(0x17a);
              BAsmCode[CodeLen++] = 0x0c + (HReg << 4);
              TransferAdrRelocs(CodeLen);
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
              break;
          }
        }
        break;
    }
  }
}

static void DecodeMOVZS(Word Index)
{
  Byte HReg;
  int z;
  UNUSED(Index);

  z = Ord(Memo("MOVS")) << 4;
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80251))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        if (OpSize != 1) WrError(ErrNum_InvAddrMode);
        else
        {
          HReg = AdrPart;
          OpSize--;
          DecodeAdr(&ArgStr[2], MModReg);
          switch (AdrMode)
          {
            case ModReg:
             PutCode(0x10a + z);
             BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
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
      DecodeAdr(&ArgStr[2], MModIReg8);
      switch (AdrMode)
      {
        case ModIReg8:
          PutCode(0xe2 + AdrPart + z);
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
    if (*ArgStr[1].str.p_str == '#')
      SetOpSize(Ord(Index == 2));
    DecodeAdr(&ArgStr[1], MModDir8 | MModReg | ((z == 0x10) ? 0 : MModImm));
    switch (AdrMode)
    {
      case ModDir8:
        PutCode(0xc0 + z);
        TransferAdrRelocs(CodeLen);
        BAsmCode[CodeLen++] = AdrVals[0];
        break;
      case ModReg:
        if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
        {
          PutCode(0x1ca + z);
          BAsmCode[CodeLen++] = 0x08 + (AdrPart << 4) + OpSize + (Ord(OpSize == 2));
        }
        break;
      case ModImm:
        if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
        {
          PutCode(0x1ca);
          BAsmCode[CodeLen++] = 0x02 + (OpSize << 2);
          TransferAdrRelocs(CodeLen);
          memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
          CodeLen += AdrCnt;
        }
        break;
    }
  }
}

static void DecodeXCH(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModIReg8 | MModDir8);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModDir8);
        switch (AdrMode)
        {
          case ModReg:
            if (AdrPart > 7) WrError(ErrNum_InvAddrMode);
            else
              PutCode(0xc8 + AdrPart);
            break;
          case ModIReg8:
            PutCode(0xc6 + AdrPart);
            break;
          case ModDir8:
            PutCode(0xc5);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
      case ModReg:
        if ((OpSize != 0) || (AdrPart > 7)) WrError(ErrNum_InvAddrMode);
        else
        {
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModAcc);
          switch (AdrMode)
          {
            case ModAcc:
              PutCode(0xc8 + HReg);
              break;
          }
        }
        break;
      case ModIReg8:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModAcc);
        switch (AdrMode)
        {
          case ModAcc:
            PutCode(0xc6 + HReg);
            break;
        }
        break;
      case ModDir8:
        HReg = AdrVals[0]; SaveBackupAdrRelocs();
        DecodeAdr(&ArgStr[2], MModAcc);
        switch (AdrMode)
        {
          case ModAcc:
            PutCode(0xc5);
            TransferBackupAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = HReg;
            break;
        }
        break;
    }
  }
}

static void DecodeXCHD(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModIReg8);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModIReg8);
        switch (AdrMode)
        {
          case ModIReg8:
            PutCode(0xd6 + AdrPart);
            break;
        }
        break;
      case ModIReg8:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModAcc);
        switch (AdrMode)
        {
          case ModAcc:
            PutCode(0xd6 + HReg);
            break;
        }
        break;
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
    DecodeAdr(&ArgStr[1], MModIReg);
    switch (AdrMode)
    {
      case ModIReg:
        if (AdrSize != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x189 + (Index << 4));
          BAsmCode[CodeLen++] = 0x04 + (AdrPart << 4);
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
    DecodeAdr(&ArgStr[1], MModIReg);
    switch (AdrMode)
    {
      case ModIReg:
        if (AdrSize != 2) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x189 + (Index << 4));
          BAsmCode[CodeLen++] = 0x08 + (AdrPart << 4);
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
      BAsmCode[CodeLen++] = (AdrLong >> 16) & 0xff;
      BAsmCode[CodeLen++] = (AdrLong >>  8) & 0xff;
      BAsmCode[CodeLen++] =  AdrLong        & 0xff;
    }
  }
}

static void DecodeJMP(Word Index)
{
  LongInt AdrLong, Dist;
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "@A+DPTR"))
    PutCode(0x73);
  else if (*ArgStr[1].str.p_str == '@')
  {
    DecodeAdr(&ArgStr[1], MModIReg);
    switch (AdrMode)
    {
      case ModIReg:
        PutCode(0x189);
        BAsmCode[CodeLen++] = 0x04 + (AdrSize << 1) + (AdrPart << 4);
        break;
    }
  }
  else
  {
    AdrLong = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);
    if (OK)
    {
      Dist = AdrLong - (EProgCounter() + 2);
      if ((Dist<=127) && (Dist >= -128))
      {
        PutCode(0x80);
        BAsmCode[CodeLen++] = Dist & 0xff;
      }
      else if ((!Chk504(EProgCounter())) && ((AdrLong >> 11) == ((((long)EProgCounter()) + 2) >> 11)))
      {
        PutCode(0x01 + ((Hi(AdrLong) & 7) << 5));
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
      else if (MomCPU < CPU8051) WrError(ErrNum_JmpTargOnDiffPage);
      else if (((((long)EProgCounter()) + 3) >> 16) == (AdrLong >> 16))
      {
        PutCode(0x02);
        BAsmCode[CodeLen++] = Hi(AdrLong);
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
      else if (MomCPU < CPU80251) WrError(ErrNum_JmpTargOnDiffPage);
      else
      {
        PutCode(0x18a);
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
    DecodeAdr(&ArgStr[1], MModIReg);
    switch (AdrMode)
    {
      case ModIReg:
        PutCode(0x199);
        BAsmCode[CodeLen++] = 0x04 + (AdrSize << 1) + (AdrPart << 4);
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
        PutCode(0x11 + ((Hi(AdrLong) & 7) << 5));
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
      else if (MomCPU < CPU8051) WrError(ErrNum_JmpTargOnDiffPage);
      else if (ChkSamePage(AdrLong, EProgCounter() + 3, 16, Flags))
      {
        PutCode(0x12);
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
      DecodeAdr(&ArgStr[1], MModReg | MModDir8);
      switch (AdrMode)
      {
        case ModReg:
          if ((OpSize != 0) || (AdrPart > 7)) WrError(ErrNum_InvAddrMode);
          else
          {
            AdrLong -= EProgCounter() + 2 + Ord(NeedsPrefix(0xd8 + AdrPart));
            if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
            else
            {
              PutCode(0xd8 + AdrPart);
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
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
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
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(3, 3))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt24, &OK, &Flags);
    SubPCRefReloc();
    if (OK)
    {
      DecodeAdr(&ArgStr[1], MModAcc | MModIReg8 | MModReg);
      switch (AdrMode)
      {
        case ModAcc:
          DecodeAdr(&ArgStr[2], MModDir8 | MModImm);
          switch (AdrMode)
          {
            case ModDir8:
              AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb5));
              if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
              else
              {
                PutCode(0xb5);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
                BAsmCode[CodeLen++] = AdrLong & 0xff;
              }
              break;
            case ModImm:
              AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb5));
              if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
              else
              {
                PutCode(0xb4);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
                BAsmCode[CodeLen++] = AdrLong & 0xff;
              }
              break;
          }
          break;
        case ModReg:
          if ((OpSize != 0) || (AdrPart > 7)) WrError(ErrNum_InvAddrMode);
          else
          {
            HReg = AdrPart;
            DecodeAdr(&ArgStr[2], MModImm);
            switch (AdrMode)
            {
              case ModImm:
                AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb8 + HReg));
                if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
                else
                {
                  PutCode(0xb8 + HReg);
                  TransferAdrRelocs(CodeLen);
                  BAsmCode[CodeLen++] = AdrVals[0];
                  BAsmCode[CodeLen++] = AdrLong & 0xff;
                }
                break;
            }
          }
          break;
        case ModIReg8:
          HReg = AdrPart; SetOpSize(0);
          DecodeAdr(&ArgStr[2], MModImm);
          switch (AdrMode)
          {
            case ModImm:
              AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(0xb6 + HReg));
              if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
              else
              {
                PutCode(0xb6 + HReg);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
                BAsmCode[CodeLen++] = AdrLong & 0xff;
              }
              break;
          }
          break;
      }
    }
  }
}

static void DecodeADD(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModReg);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModImm | MModDir8 | MModDir16 | MModIReg8 | MModIReg | MModReg);
        switch (AdrMode)
        {
          case ModImm:
            PutCode(0x24);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
          case ModDir8:
            PutCode(0x25);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
          case ModDir16:
            PutCode(0x12e);
            BAsmCode[CodeLen++] = (AccReg << 4) + 3;
            memcpy(BAsmCode + CodeLen, AdrVals, 2);
            CodeLen += 2;
            break;
          case ModIReg8:
            PutCode(0x26 + AdrPart);
            break;
          case ModIReg:
            PutCode(0x12e);
            BAsmCode[CodeLen++] = 0x09 + AdrSize + (AdrPart << 4);
            BAsmCode[CodeLen++] = AccReg << 4;
            break;
          case ModReg:
            if ((AdrPart < 8) && (!SrcMode)) PutCode(0x28 + AdrPart);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x12c);
              BAsmCode[CodeLen++] = AdrPart + (AccReg << 4);
            }
            break;
        }
        break;
      case ModReg:
        if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
        {
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModImm | MModReg | MModDir8 | MModDir16 | MModIReg8 | MModIReg);
          switch (AdrMode)
          {
            case ModImm:
              if ((OpSize == 0) && (HReg == AccReg))
              {
                PutCode(0x24);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
              }
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2);
                memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
                CodeLen += AdrCnt;
              }
              break;
            case ModReg:
              PutCode(0x12c + OpSize);
              if (OpSize == 2) BAsmCode[CodeLen - 1]++;
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              break;
            case ModDir8:
              if (OpSize == 2) WrError(ErrNum_InvAddrMode);
              else if ((OpSize == 0) && (HReg == AccReg))
              {
                PutCode(0x25);
                TransferAdrRelocs(CodeLen);
                BAsmCode[CodeLen++] = AdrVals[0];
              }
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2) + 1;
                BAsmCode[CodeLen++] = AdrVals[0];
              }
              break;
            case ModDir16:
              if (OpSize == 2) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2) + 3;
                memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
                CodeLen += AdrCnt;
              }
              break;
            case ModIReg8:
              if ((OpSize != 0) || (HReg != AccReg)) WrError(ErrNum_InvAddrMode);
              else PutCode(0x26 + AdrPart);
              break;
            case ModIReg:
              if (OpSize != 0) WrError(ErrNum_InvAddrMode);
              else
              {
                PutCode(0x12e);
                BAsmCode[CodeLen++] = 0x09 + AdrSize + (AdrPart << 4);
                BAsmCode[CodeLen++] = HReg << 4;
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
  Byte HReg;

  /* Index: SUB=0 CMP=1 */

  z = 0x90 + (Index << 5);
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80251))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModImm | MModReg | MModDir8 | MModDir16 | MModIReg | (Index ? MModImmEx : 0));
        switch (AdrMode)
        {
          case ModImm:
            PutCode(0x10e + z);
            BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2);
            TransferAdrRelocs(CodeLen);
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModImmEx:
            PutCode(0x10e + z);
            BAsmCode[CodeLen++] = (HReg << 4) + 0x0c;
            TransferAdrRelocs(CodeLen);
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModReg:
            PutCode(0x10c + z + OpSize);
            if (OpSize == 2)
              BAsmCode[CodeLen - 1]++;
            BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            break;
          case ModDir8:
            if (OpSize == 2) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x10e + z);
              BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2) + 1;
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            break;
          case ModDir16:
            if (OpSize == 2) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x10e + z);
              BAsmCode[CodeLen++] = (HReg << 4) + (OpSize << 2) + 3;
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
            }
            break;
          case ModIReg:
            if (OpSize != 0) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x10e + z);
              BAsmCode[CodeLen++] = 0x09 + AdrSize + (AdrPart << 4);
              BAsmCode[CodeLen++] = HReg << 4;
            }
            break;
        }
        break;
    }
  }
}

static void DecodeADDCSUBB(Word Index)
{
  Byte HReg;

  /* Index: ADDC=0 SUBB=1 */

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc);
    switch (AdrMode)
    {
      case ModAcc:
        HReg = 0x30 + (Index*0x60);
        DecodeAdr(&ArgStr[2], MModReg | MModIReg8 | MModDir8 | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            if (AdrPart > 7) WrError(ErrNum_InvAddrMode);
            else
              PutCode(HReg + 0x08 + AdrPart);
            break;
          case ModIReg8:
            PutCode(HReg + 0x06 + AdrPart);
            break;
          case ModDir8:
            PutCode(HReg + 0x05);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
          case ModImm:
            PutCode(HReg + 0x04);
            TransferAdrRelocs(CodeLen);
            BAsmCode[CodeLen++] = AdrVals[0];
            break;
        }
        break;
    }
  }
}

static void DecodeINCDEC(Word Index)
{
  Byte HReg;
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
      HReg = 1;
      OK = True;
      Flags = eSymbolFlag_None;
    }
    else
      HReg = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], 1, UInt3, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      HReg = 1;
    if (OK)
    {
      OK = True;
      if (HReg == 1)
        HReg = 0;
      else if (HReg == 2)
        HReg = 1;
      else if (HReg == 4)
        HReg = 2;
      else
        OK = False;
      if (!OK) WrError(ErrNum_OverRange);
      else if (!as_strcasecmp(ArgStr[1].str.p_str, "DPTR"))
      {
        if (Index == 1) WrError(ErrNum_InvAddrMode);
        else if (HReg != 0) WrError(ErrNum_OverRange);
        else
          PutCode(0xa3);
      }
      else
      {
        DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModDir8 | MModIReg8);
        switch (AdrMode)
        {
          case ModAcc:
            if (HReg == 0)
              PutCode(0x04 + z);
            else if (MomCPU < CPU80251) WrError(ErrNum_OverRange);
            else
            {
              PutCode(0x10b + z);
              BAsmCode[CodeLen++] = (AccReg << 4) + HReg;
            }
            break;
          case ModReg:
            if ((OpSize == 0) && (AdrPart == AccReg) && (HReg == 0))
              PutCode(0x04 + z);
            else if ((AdrPart < 8) && (OpSize == 0) && (HReg == 0) && (!SrcMode))
              PutCode(0x08 + z + AdrPart);
            else if (ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported))
            {
              PutCode(0x10b + z);
              BAsmCode[CodeLen++] = (AdrPart << 4) + (OpSize << 2) + HReg;
              if (OpSize == 2)
                BAsmCode[CodeLen - 1] += 4;
            }
            break;
          case ModDir8:
            if (HReg != 0) WrError(ErrNum_OverRange);
            else
            {
              PutCode(0x05 + z);
              TransferAdrRelocs(CodeLen);
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            break;
          case ModIReg8:
            if (HReg != 0) WrError(ErrNum_OverRange);
            else
              PutCode(0x06 + z + AdrPart);
            break;
        }
      }
    }
  }
}

static void DecodeMULDIV(Word Index)
{
  int z;
  Byte HReg;

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
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg);
        switch (AdrMode)
        {
          case ModReg:
            if (!ChkMinCPUExt(CPU80251, ErrNum_AddrModeNotSupported));
            else if (OpSize == 2) WrError(ErrNum_InvAddrMode);
            else
            {
              PutCode(0x18c + z + OpSize);
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            }
            break;
        }
        break;
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
    if (Memo("SETB")) WrError(ErrNum_InvAddrMode);
    else
      PutCode(0xf4 - z);
  }
  else if (IsCarry(ArgStr[1].str.p_str))
    PutCode(0xb3 + z);
  else
    switch (DecodeBitAdr(&ArgStr[1], &AdrLong, True))
    {
      case ModBit51:
        PutCode(0xb2 + z);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
      case ModBit251:
        PutCode(0x1a9);
        BAsmCode[CodeLen++] = 0xb0 + z + (AdrLong >> 24);
        BAsmCode[CodeLen++] = AdrLong & 0xff;
        break;
    }
}

static void DecodeShift(Word Index)
{
  int z;

  /* Index: SRA=0 SRL=1 SLL=3 */

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80251))
  {
    z = Index << 4;
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        if (OpSize == 2) WrError(ErrNum_InvAddrMode);
        else
        {
          PutCode(0x10e + z);
          BAsmCode[CodeLen++] = (AdrPart << 4) + (OpSize << 2);
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
        BAsmCode[CodeLen++] = AdrLong & 0xff;
      }
    }
  }
}

static void DecodeBCond(Word Index)
{
  FixedOrder *FixedZ = BCondOrders + Index;
  LongInt AdrLong, BitLong;
  tEvalResult EvalResult;

  if (ChkArgCnt(2, 2))
  {
    AdrLong = EvalStrIntExpressionWithResult(&ArgStr[2], UInt24, &EvalResult);
    SubPCRefReloc();
    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      switch (DecodeBitAdr(&ArgStr[1], &BitLong, True))
      {
        case ModBit51:
          AdrLong -= EProgCounter() + 3 + Ord(NeedsPrefix(FixedZ->Code));
          if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            PutCode(FixedZ->Code);
            BAsmCode[CodeLen++] = BitLong & 0xff;
            BAsmCode[CodeLen++] = AdrLong & 0xff;
          }
          break;
        case ModBit251:
          AdrLong -= EProgCounter() + 4 + Ord(NeedsPrefix(0x1a9));
          if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(EvalResult.Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            PutCode(0x1a9);
            BAsmCode[CodeLen++] = FixedZ->Code + (BitLong >> 24);
            BAsmCode[CodeLen++] = BitLong & 0xff;
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
    DecodeAdr(&ArgStr[1], MModAcc);
    switch (AdrMode)
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


static void DecodeSFR(Word Index)
{
  Word AdrByte;
  Boolean OK;
  tSymbolFlags Flags;
  as_addrspace_t DSeg;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (Memo("SFRB") && !ChkMaxCPU(CPU80C390));
  else
  {
    AdrByte = EvalStrIntExpressionWithFlags(&ArgStr[1], (MomCPU >= CPU80251) ? UInt9 : UInt8, &OK, &Flags);
    if (OK && !mFirstPassUnknown(Flags))
    {
      PushLocHandle(-1);
      DSeg = (MomCPU >= CPU80251) ? SegIO : SegData;
      EnterIntSymbol(&LabPart, AdrByte, DSeg, False);
      if (MakeUseList)
      {
        if (AddChunk(SegChunks + DSeg, AdrByte, 1, False))
          WrError(ErrNum_Overlap);
      }
      if (Memo("SFRB"))
      {
        Byte BitStart;

        if (AdrByte > 0x7f)
        {
          if ((AdrByte & 7) != 0) WrError(ErrNum_NotBitAddressable);
          BitStart = AdrByte;
        }
        else
        {
          if ((AdrByte & 0xe0) != 0x20) WrError(ErrNum_NotBitAddressable);
          BitStart = (AdrByte - 0x20) << 3;
        }
        if (MakeUseList)
          if (AddChunk(SegChunks + SegBData, BitStart, 8, False)) WrError(ErrNum_Overlap);
        as_snprintf(ListLine, STRINGSIZE, "=%~02.*u%s-%~02.*u%s",
                    ListRadixBase, (unsigned)BitStart, GetIntConstIntelSuffix(ListRadixBase),
                    ListRadixBase, (unsigned)BitStart + 7, GetIntConstIntelSuffix(ListRadixBase));
      }
      else
        as_snprintf(ListLine, STRINGSIZE, "=%~02.*u%s",
                    ListRadixBase, (unsigned)AdrByte, GetIntConstIntelSuffix(ListRadixBase));
      PopLocHandle();
    }
  }
}

static void DecodeBIT(Word Index)
{
  LongInt AdrLong;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (MomCPU >= CPU80251)
  {
    if (DecodeBitAdr(&ArgStr[1], &AdrLong, False) == ModBit251)
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
    if (DecodeBitAdr(&ArgStr[1], &AdrLong, False) == ModBit51)
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
    CodeEquate(SegIO, 0, 0x1ff);
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
  AddInstTable(InstTable, "MOVZ" , 0, DecodeMOVZS);
  AddInstTable(InstTable, "MOVS" , 0, DecodeMOVZS);
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
  CodeLen = 0;
  DontPrint = False;
  OpSize = -1;
  MinOneIs0 = False;

  /* zu ignorierendes */

  if (*OpPart.str.p_str == '\0') return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(TargetBigEndian))
    return;

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
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x31;
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
