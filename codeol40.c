/* codeol40.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator OKI OLMS-40-Familie                                         */
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
#include "errmsg.h"

#include "codeol40.h"

typedef enum
{
  ModA = 0,
  ModDPL = 1,
  ModDPH = 2,
  ModW = 3,
  ModX = 4,
  ModY = 5,
  ModZ = 6,
  ModM = 7,
  ModImm = 10,
  ModPP = 11,
  ModT = 12,
  ModTL = 13,
  ModTH = 14,
  ModPPI = 15,
  ModNone = 0x7f
} tAdrMode;

#define MModA (1 << ModA)
#define MModDPL (1 << ModDPL)
#define MModDPH (1 << ModDPH)
#define MModW (1 << ModW)
#define MModX (1 << ModX)
#define MModY (1 << ModY)
#define MModZ (1 << ModZ)
#define MModM (1 << ModM)
#define MModImm (1 << ModImm)
#define MModPP (1 << ModPP)
#define MModT (1 << ModT)
#define MModTL (1 << ModTL)
#define MModTH (1 << ModTH)
#define MModPPI (1 << ModPPI)

static CPUVar CPU5840, CPU5842, CPU58421, CPU58422, CPU5847;
static IntType CodeIntType, DataIntType, OpSizeType;
static tAdrMode AdrMode;
static Byte AdrVal;

/*-------------------------------------------------------------------------*/

typedef struct
{
  const char *pAsc;
  tAdrMode AdrMode;
} tNotation;

static void DecodeAdr(const tStrComp *pArg, Word Mask)
{
  static const tNotation Notations[] =
  {
    { "A",    ModA    },
    { "M",    ModM    },
    { "(DP)", ModM    },
    { "W",    ModW    },
    { "X",    ModX    },
    { "Y",    ModY    },
    { "Z",    ModZ    },
    { "DPL",  ModDPL  },
    { "DPH",  ModDPH  },
    { "PP",   ModPP   },
    { "T",    ModT    },
    { "TL",   ModTL   },
    { "TH",   ModTH   },
    { "(PP)", ModPPI  },
    { NULL,   ModNone },
  };
  const tNotation *pNot;
  Boolean OK;

  if (MomCPU != CPU5840)
    Mask &= ~(MModX | MModY | MModZ | MModTL | MModTH);
  if (MomCPU  == CPU5847)
    Mask &= ~(MModW);

  for (pNot = Notations; pNot->pAsc; pNot++)
    if (!as_strcasecmp(pArg->str.p_str, pNot->pAsc))
    {
      AdrMode = pNot->AdrMode;
      goto AdrFound;
    }

  AdrVal = EvalStrIntExpressionOffs(pArg, !!(0[pArg->str.p_str] == '#'), OpSizeType, &OK);
  if (OK)
  {
    AdrMode = ModImm;
    AdrVal &= IntTypeDefs[OpSizeType].Mask;
  }

AdrFound:

  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone; AdrCnt = 0;
  }
}

/*-------------------------------------------------------------------------*/

static void DecodeLD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModA | MModDPL | MModDPH | MModW | MModX | MModY | MModZ | MModPP | MModT);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[2], MModImm | MModW | MModX | MModY | MModZ | MModM | MModDPL | MModTL | MModTH);
        switch (AdrMode)
        {
          case ModW:
          case ModX:
          case ModY:
          case ModZ:
            BAsmCode[CodeLen++] = 0x84 | (AdrMode - ModW);
            break;
          case ModM:
            BAsmCode[CodeLen++] = 0x94;
            break;
          case ModDPL:
            BAsmCode[CodeLen++] = 0x55;
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0x10 | AdrVal;
            break;
          case ModTL:
          case ModTH:
            BAsmCode[CodeLen++] = 0x6b - (AdrMode - ModTL);
            break;
          default:
            break;
        }
        break;
      case ModDPL:
        DecodeAdr(&ArgStr[2], MModImm | MModA);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[CodeLen++] = 0x20 | AdrVal;
            break;
          case ModA:
            BAsmCode[CodeLen++] = 0x54;
            break;
          default:
            break;
        }
        break;
      case ModDPH:
        DecodeAdr(&ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[CodeLen++] = 0x60 | AdrVal;
            break;
          default:
            break;
        }
        break;
      case ModW:
      case ModX:
      case ModY:
      case ModZ:
      {
        tAdrMode HMode = AdrMode;

        DecodeAdr(&ArgStr[2], MModA);
        switch (AdrMode)
        {
          case ModA:
            BAsmCode[CodeLen++] = 0x80 | (HMode - ModW);
            break;
          default:
            break;
        }
        break;
      }
      case ModPP:
        DecodeAdr(&ArgStr[2], MModA);
        switch (AdrMode)
        {
          case ModA:
            BAsmCode[CodeLen++] = 0x58;
            break;
          default:
            break;
        }
        break;
      case ModT:
        OpSizeType = Int8;
        DecodeAdr(&ArgStr[2], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            if (MomCPU == CPU5840)
            {
              BAsmCode[CodeLen++] = 0x68;
              BAsmCode[CodeLen++] = AdrVal;
            }
            else if (AdrVal) WrError(ErrNum_OverRange);
            else
              BAsmCode[CodeLen++] = 0x68;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeINCDEC(Word IsDEC)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModA | MModW | MModX | MModY | MModZ | MModDPL | MModM
                       | ((IsDEC && (MomCPU == CPU5840)) ? MModDPH : 0));
    switch (AdrMode)
    {
      case ModA:
        BAsmCode[CodeLen++] = IsDEC ? 0x0f : 0x01;
        break;
      case ModW:
      case ModX:
      case ModY:
      case ModZ:
        BAsmCode[CodeLen++] = (IsDEC ? 0x8c : 0x88) | (AdrMode - ModW);
        break;
      case ModM:
        BAsmCode[CodeLen++] = IsDEC ? 0x5c : 0x5d;
        break;
      case ModDPL:
        BAsmCode[CodeLen++] = IsDEC ? 0x56 : 0x57;
        break;
      case ModDPH:
        BAsmCode[CodeLen++] = 0x5f;
        break;
      default:
        break;
    }
  }
}

static void DecodeBit(Word Code)
{
  if (ArgCnt == 1)
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "C"))
      BAsmCode[CodeLen++] = Hi(Code);
    else
      WrError(ErrNum_InvAddrMode);
  }
  else if (ChkArgCnt(1, 2))
  {
    Boolean OK;
    unsigned BitNo = EvalStrIntExpression(&ArgStr[2], UInt2, &OK);

    Code = Lo(Code);

    if (OK)
    {
      DecodeAdr(&ArgStr[1], ((Code == 2) ? MModA : MModPPI) | MModM);
      switch (AdrMode)
      {
        case ModA:
          BAsmCode[CodeLen++] = 0xa0 | BitNo;
          break;
        case ModM:
          if (Code == 2)
            BAsmCode[CodeLen++] = 0xa4 | BitNo;
          else
            BAsmCode[CodeLen++] = 0xb8 | (Code << 2) | BitNo;
          break;
        case ModPPI:
          BAsmCode[CodeLen++] = 0xb0 | (Code << 2) | BitNo;
          break;
        default:
          break;
      }
    }
  }
}

static void CodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
    BAsmCode[CodeLen++] = Lo(Code);
}

static void CodeTHB(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
  {
    Boolean OK = True;
    Word ImmVal = ArgCnt ? EvalStrIntExpression(&ArgStr[1], UInt1, &OK) : 0;

    if (OK)
    {
      if ((MomCPU == CPU5840) || (MomCPU == CPU5842))
        BAsmCode[CodeLen++] = Lo(Code) | (ImmVal & 1);
      else if (ImmVal) WrError(ErrNum_OverRange);
      else
        BAsmCode[CodeLen++] = Lo(Code);
    }
  }
}

static void CodeImm2(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
  {
    Boolean OK;
    Word ImmVal = EvalStrIntExpression(&ArgStr[1], UInt2, &OK);

    if (OK)
      BAsmCode[CodeLen++] = Lo(Code) | (ImmVal & 3);
  }
}

static void CodeImm4(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
  {
    Boolean OK;
    Word ImmVal = EvalStrIntExpression(&ArgStr[1], Int4, &OK);

    if (OK)
      BAsmCode[CodeLen++] = Lo(Code) | (ImmVal & 15);
  }
}

static void CodeLTI(Word Code)
{
  if (ChkArgCnt(0, 1)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
  {
    Boolean OK = True;
    Word ImmVal = ArgCnt ? EvalStrIntExpression(&ArgStr[1], Int8, &OK) : 0;

    if (OK)
    {
      /* LTI can only load 0 into timer on smaller devices */

      if (MomCPU == CPU5840)
      {
        BAsmCode[CodeLen++] = Lo(Code);
        BAsmCode[CodeLen++] = Lo(ImmVal);
      }
      else if (ImmVal != 0) WrError(ErrNum_OverRange);
      else
        BAsmCode[CodeLen++] = Lo(Code);
    }
  }
}

static void CodeJC(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
  {
    tEvalResult EvalResult;
    Word Address = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);
    if (EvalResult.OK && ChkSamePage(EProgCounter() + 1, Address, 6, EvalResult.Flags))
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      BAsmCode[CodeLen++] = Lo(Code) | (Address & 0x3f);
    }
  }
}

static void CodeJMP(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(Hi(Code), CPU5840) >= 0))
  {
    tEvalResult EvalResult;
    Word Address= EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      BAsmCode[CodeLen++] = Lo(Code) | (Hi(Address) & 0x07);
      BAsmCode[CodeLen++] = Lo(Address);
    }
  }
}

static void DecodeDATA_OLMS40(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int8, Int4);
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, SegLimits[SegData]);
}

/*-------------------------------------------------------------------------*/

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "INC", 0, DecodeINCDEC);
  AddInstTable(InstTable, "DEC", 1, DecodeINCDEC);
  AddInstTable(InstTable, "BSET", 0x4000, DecodeBit);
  AddInstTable(InstTable, "BCLR", 0x4101, DecodeBit);
  AddInstTable(InstTable, "BTST", 0x4202, DecodeBit);

  AddInstTable(InstTable, "CLA",    0x1f10, CodeFixed);
  AddInstTable(InstTable, "CLL",    0x1f20, CodeFixed);
  AddInstTable(InstTable, "CLH",    0x1f60, CodeFixed);
  AddInstTable(InstTable, "L",      0x1f94, CodeFixed);
  AddInstTable(InstTable, "LAL",    0x1f55, CodeFixed);
  AddInstTable(InstTable, "LLA",    0x1f54, CodeFixed);
  AddInstTable(InstTable, "LAW",    0x0f84, CodeFixed);
  AddInstTable(InstTable, "LAX",    0x0185, CodeFixed);
  AddInstTable(InstTable, "LAY",    0x0186, CodeFixed);
  AddInstTable(InstTable, "LAZ",    0x0187, CodeFixed);
  AddInstTable(InstTable, "SI",     0x1f90, CodeFixed);
  AddInstTable(InstTable, "LWA",    0x0f80, CodeFixed);
  AddInstTable(InstTable, "LXA",    0x0181, CodeFixed);
  AddInstTable(InstTable, "LYA",    0x0182, CodeFixed);
  AddInstTable(InstTable, "LZA",    0x0183, CodeFixed);
  AddInstTable(InstTable, "LPA",    0x1f58, CodeFixed);
  AddInstTable(InstTable, "RTH",    0x016a, CodeFixed);
  AddInstTable(InstTable, "RTL",    0x016b, CodeFixed);
  AddInstTable(InstTable, "XA",     0x0149, CodeFixed);
  AddInstTable(InstTable, "XL",     0x014a, CodeFixed);
  AddInstTable(InstTable, "XCH",    0x0148, CodeFixed);
  AddInstTable(InstTable, "X",      0x1f98, CodeFixed);
  AddInstTable(InstTable, "XAX",    0x014b, CodeFixed);
  AddInstTable(InstTable, "INA",    0x0f01, CodeFixed);
  AddInstTable(InstTable, "INL",    0x0f57, CodeFixed);
  AddInstTable(InstTable, "INM",    0x1f5d, CodeFixed);
  AddInstTable(InstTable, "INW",    0x0f88, CodeFixed);
  AddInstTable(InstTable, "INX",    0x0189, CodeFixed);
  AddInstTable(InstTable, "INY",    0x018a, CodeFixed);
  AddInstTable(InstTable, "INZ",    0x018b, CodeFixed);
  AddInstTable(InstTable, "DCA",    0x0f0f, CodeFixed);
  AddInstTable(InstTable, "DCL",    0x0f56, CodeFixed);
  AddInstTable(InstTable, "DCM",    0x1f5c, CodeFixed);
  AddInstTable(InstTable, "DCW",    0x018c, CodeFixed);
  AddInstTable(InstTable, "DCX",    0x018d, CodeFixed);
  AddInstTable(InstTable, "DCY",    0x018e, CodeFixed);
  AddInstTable(InstTable, "DCZ",    0x018f, CodeFixed);
  AddInstTable(InstTable, "DCH",    0x015f, CodeFixed);
  AddInstTable(InstTable, "CAO",    0x1f50, CodeFixed);
  AddInstTable(InstTable, "AND",    0x0144, CodeFixed);
  AddInstTable(InstTable, "OR",     0x0145, CodeFixed);
  AddInstTable(InstTable, "EOR",    0x0146, CodeFixed);
  AddInstTable(InstTable, "RAL",    0x1f47, CodeFixed);
  AddInstTable(InstTable, "AC",     0x1f4c, CodeFixed);
  AddInstTable(InstTable, "ACS",    0x014d, CodeFixed);
  AddInstTable(InstTable, "AS",     0x1f4e, CodeFixed);
  AddInstTable(InstTable, "DAS",    0x1f5a, CodeFixed);
  AddInstTable(InstTable, "CM",     0x1f5e, CodeFixed);
  AddInstTable(InstTable, "AWS",    0x019c, CodeFixed);
  AddInstTable(InstTable, "AXS",    0x019d, CodeFixed);
  AddInstTable(InstTable, "AYS",    0x019e, CodeFixed);
  AddInstTable(InstTable, "AZS",    0x019f, CodeFixed);
  AddInstTable(InstTable, "TI",     0x01af, CodeFixed);
  AddInstTable(InstTable, "TTM",    0x1fae, CodeFixed);
  AddInstTable(InstTable, "TC",     0x1f42, CodeFixed);
  AddInstTable(InstTable, "SC",     0x1f40, CodeFixed);
  AddInstTable(InstTable, "RC",     0x1f41, CodeFixed);
  AddInstTable(InstTable, "JA",     0x1f43, CodeFixed);
  AddInstTable(InstTable, "RT",     0x1f59, CodeFixed);
  AddInstTable(InstTable, "OBS",    0x0170, CodeFixed);
  AddInstTable(InstTable, "OTD",    0x1f71, CodeFixed);
  AddInstTable(InstTable, "OA",     0x1f72, CodeFixed);
  AddInstTable(InstTable, "OB",     0x1f73, CodeFixed);
  AddInstTable(InstTable, "OP",     0x1f74, CodeFixed);
  AddInstTable(InstTable, "OAB",    0x0175, CodeFixed);
  AddInstTable(InstTable, "OPM",    0x1f76, CodeFixed);
  AddInstTable(InstTable, "IA",     0x1f7a, CodeFixed);
  AddInstTable(InstTable, "IB",     0x1f7b, CodeFixed);
  AddInstTable(InstTable, "IK",     0x0f7c, CodeFixed);
  AddInstTable(InstTable, "IAB",    0x017d, CodeFixed);
  AddInstTable(InstTable, "EI",     0x0153, CodeFixed);
  AddInstTable(InstTable, "DI",     0x0152, CodeFixed);
  AddInstTable(InstTable, "ET",     0x016f, CodeFixed);
  AddInstTable(InstTable, "DT",     0x016e, CodeFixed);
  AddInstTable(InstTable, "ECT",    0x017f, CodeFixed);
  AddInstTable(InstTable, "DCT",    0x017e, CodeFixed);
  AddInstTable(InstTable, "HLT",    0x016d, CodeFixed);
  AddInstTable(InstTable, "EXP",    0x0169, CodeFixed);
  AddInstTable(InstTable, "NOP",    0x1f00, CodeFixed);

  AddInstTable(InstTable, "THB",    0x0fac, CodeTHB);

  AddInstTable(InstTable, "LM",     0x0194, CodeImm2);
  AddInstTable(InstTable, "SMI",    0x0190, CodeImm2);
  AddInstTable(InstTable, "XM",     0x0198, CodeImm2);
  AddInstTable(InstTable, "SPB",    0x01b0, CodeImm2);
  AddInstTable(InstTable, "RPB",    0x01b4, CodeImm2);
  AddInstTable(InstTable, "SMB",    0x1fb8, CodeImm2);
  AddInstTable(InstTable, "RMB",    0x1fbc, CodeImm2);
  AddInstTable(InstTable, "TAB",    0x1fa0, CodeImm2);
  AddInstTable(InstTable, "TMB",    0x1fa4, CodeImm2);
  AddInstTable(InstTable, "TKB",    0x01a8, CodeImm2);

  AddInstTable(InstTable, "LAI",    0x1f10, CodeImm4);
  AddInstTable(InstTable, "LLI",    0x1f20, CodeImm4);
  AddInstTable(InstTable, "LHI",    0x1f60, CodeImm4);
  AddInstTable(InstTable, "AIS",    0x1f00, CodeImm4);

  AddInstTable(InstTable, "LTI",    0x1f68, CodeLTI);

  AddInstTable(InstTable, "JC",     0x1fc0, CodeJC);

  AddInstTable(InstTable, "J",      0x1f30, CodeJMP);
  AddInstTable(InstTable, "CAL",    0x1f38, CodeJMP);

  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_OLMS40);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void  MakeCode_OLMS40(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSizeType = Int4;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}


static Boolean IsDef_OLMS40(void)
{
  return False;
}

static void SwitchFrom_OLMS40(void)
{
  DeinitFields();
}

static void SwitchTo_OLMS40(void)
{
  const TFamilyDescr *pDescr;

  pDescr = FindFamilyByName("OLMS-40");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = pDescr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegCode] = 0;
  if (MomCPU == CPU5840)
  {
    CodeIntType = UInt11;
    DataIntType = UInt7;
    SegLimits[SegData] = IntTypeDefs[DataIntType].Max;
    SegLimits[SegCode] = IntTypeDefs[CodeIntType].Max;
  }
  else if (MomCPU == CPU5842)
  {
    CodeIntType = UInt10;
    DataIntType = UInt5;
    SegLimits[SegData] = IntTypeDefs[DataIntType].Max;
    SegLimits[SegCode] = 767;
  }
  else if ((MomCPU == CPU58421) || (MomCPU == CPU58422))
  {
    CodeIntType = UInt11;
    DataIntType = UInt6;
    SegLimits[SegData] = 39;
    SegLimits[SegCode] = 1535;
  }
  else if (MomCPU == CPU5847)
  {
    CodeIntType = UInt11;
    DataIntType = UInt7;
    SegLimits[SegData] = 95;
    SegLimits[SegCode] = 1535;
  }

  MakeCode = MakeCode_OLMS40;
  IsDef = IsDef_OLMS40;
  SwitchFrom = SwitchFrom_OLMS40; InitFields();

}

void codeolms40_init(void)
{
  CPU5840  = AddCPU("MSM5840" , SwitchTo_OLMS40);
  CPU5842  = AddCPU("MSM5842" , SwitchTo_OLMS40);
  CPU58421 = AddCPU("MSM58421", SwitchTo_OLMS40);
  CPU58422 = AddCPU("MSM58422", SwitchTo_OLMS40);
  CPU5847  = AddCPU("MSM5847" , SwitchTo_OLMS40);
}
