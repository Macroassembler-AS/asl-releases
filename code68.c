/* code68.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator fuer 68xx Prozessoren                                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmsub.h"
#include "errmsg.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "cpu2phys.h"
#include "function.h"
#include "nlmessages.h"
#include "as.rsc"

#include "code68.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  CPUVar MinCPU, MaxCPU;
  Word Code;
} FixedOrder;

typedef struct
{
  CPUVar MinCPU;
  Word Code;
} RelOrder;

typedef struct
{
  Boolean MayImm;
  CPUVar MinCPU;    /* Shift  andere   ,Y   */
  Byte PageShift;   /* 0 :     nix    Pg 2  */
  Byte Code;        /* 1 :     Pg 3   Pg 4  */
} ALU16Order;       /* 2 :     nix    Pg 4  */
                    /* 3 :     Pg 2   Pg 3  */

enum
{
  ModNone = -1,
  ModAcc  = 0,
  ModDir  = 1,
  ModExt  = 2,
  ModInd  = 3,
  ModImm  = 4
};

#define MModAcc (1 << ModAcc)
#define MModDir (1 << ModDir)
#define MModExt (1 << ModExt)
#define MModInd (1 << ModInd)
#define MModImm (1 << ModImm)

#define Page2Prefix 0x18
#define Page3Prefix 0x1a
#define Page4Prefix 0xcd


static tSymbolSize OpSize;
static Byte PrefCnt;           /* Anzahl Befehlspraefixe */
static ShortInt AdrMode;       /* Ergebnisadressmodus */
static Byte AdrPart;           /* Adressierungsmodusbits im Opcode */
static Byte AdrVals[4];        /* Adressargument */

static FixedOrder *FixedOrders;
static RelOrder   *RelOrders;
static ALU16Order *ALU16Orders;

static LongInt Reg_MMSIZ, Reg_MMWBR, Reg_MM1CR, Reg_MM2CR, Reg_INIT, Reg_INIT2, Reg_CONFIG;

static CPUVar CPU6800, CPU6801, CPU6301, CPU6811, CPU68HC11K4;

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     compute_window(Byte w_size_code, Byte cpu_start_code, LongInt phys_start_code, LongWord phys_offset)
 * \brief  compute single window from MMU registers
 * \param  w_size_code MMSIZ bits (0..3)
 * \param  cpu_start_code Reg_MMWBR bits (0,2,4,6...14)
 * \param  phys_start_code Reg_MMxCR bits
 * \param  phys_offset offset in physical space
 * ------------------------------------------------------------------------ */

static void compute_window(Byte w_size_code, Byte cpu_start_code, LongInt phys_start_code, LongWord phys_offset)
{
  if (w_size_code)
  {
    Word size, cpu_start;
    LongWord phys_start;

    /* window size */

    size = 0x1000 << w_size_code;

    /* CPU space start address: assume 8K window, systematically clip out bits for
       larger windows */

    cpu_start = (Word)cpu_start_code << 12;
    if (w_size_code > 1)
      cpu_start &= ~0x2000;
    if (w_size_code > 2)
      cpu_start = (cpu_start == 0xc000) ? 0x8000 : cpu_start;

    /* physical space start: mask out lower bits according to window size */

    phys_start = ((phys_start_code & 0x7f & (~((1 << w_size_code) - 1))) << 12) + phys_offset;

    /* set addresses */

    cpu_2_phys_area_add(SegCode, cpu_start, phys_start, size);
  }
}

static void SetK4Ranges(void)
{
  Word ee_bank, io_bank, ram_bank;

  cpu_2_phys_area_clear(SegCode);

  /* Add window 1 after window 2, since it has higher priority and may partially overlap window 2 */

  compute_window((Reg_MMSIZ >> 4) & 0x3, (Reg_MMWBR >> 4) & 0x0e, Reg_MM2CR, 0x90000);
  compute_window(Reg_MMSIZ & 0x3, Reg_MMWBR & 0x0e, Reg_MM1CR, 0x10000);

  /* Internal registers, RAM and EEPROM (if enabled) have priority in CPU address space: */

  if (Reg_CONFIG & 1)
  {
    ee_bank = ((Reg_INIT & 15) << 12) + 0x0d80;
    cpu_2_phys_area_add(SegCode, ee_bank, ee_bank, 640);
  }

  io_bank = (Reg_INIT & 15) << 12;
  cpu_2_phys_area_add(SegCode, io_bank, io_bank, 128);

  /* If RAM position overlaps registers, 128 bytes of RAM get relocated to upper end: */

  ram_bank = ((Reg_INIT >> 4) & 15) << 12;
  if (ram_bank == io_bank)
    ram_bank += 128;
  cpu_2_phys_area_add(SegCode, ram_bank, ram_bank, 768);

  /* Fill the remainder of CPU address space with 1:1 mappings: */

  cpu_2_phys_area_fill(SegCode, 0x0000, 0xffff);
}

/*---------------------------------------------------------------------------*/

static Boolean DecodeAcc(const char *pArg, Byte *pReg)
{
  static const char Regs[] = "AB";

  if (strlen(pArg) == 1)
  {
    const char *pPos = strchr(Regs, as_toupper(*pArg));

    if (pPos)
    {
      *pReg = pPos - Regs;
      return True;
    }
  }
  return False;
}

static void DecodeAdr(int StartInd, int StopInd, Byte Erl)
{
  tStrComp *pStartArg = &ArgStr[StartInd];
  Boolean OK, ErrOcc;
  tSymbolFlags Flags;
  Word AdrWord;
  Byte Bit8;

  AdrMode = ModNone;
  AdrPart = 0;
  ErrOcc = False;

  /* eine Komponente ? */

  if (StartInd == StopInd)
  {
    /* Akkumulatoren ? */

    if (DecodeAcc(pStartArg->str.p_str, &AdrPart))
    {
      if (MModAcc & Erl)
        AdrMode = ModAcc;
    }

    /* immediate ? */

    else if ((strlen(pStartArg->str.p_str) > 1) && (*pStartArg->str.p_str == '#'))
    {
      if (MModImm & Erl)
      {
        if (OpSize == eSymbolSize16Bit)
        {
          AdrWord = EvalStrIntExpressionOffs(pStartArg, 1, Int16, &OK);
          if (OK)
          {
            AdrMode = ModImm;
            AdrVals[AdrCnt++] = Hi(AdrWord);
            AdrVals[AdrCnt++] = Lo(AdrWord);
          }
          else
            ErrOcc = True;
        }
        else
        {
          AdrVals[AdrCnt] = EvalStrIntExpressionOffs(pStartArg, 1, Int8, &OK);
          if (OK)
          {
            AdrMode = ModImm;
            AdrCnt++;
          }
          else
            ErrOcc = True;
        }
      }
    }

    /* absolut ? */

    else
    {
      unsigned Offset = 0;

      Bit8 = 0;
      if (pStartArg->str.p_str[Offset] == '<')
      {
        Bit8 = 2;
        Offset++;
      }
      else if (pStartArg->str.p_str[Offset] == '>')
      {
        Bit8 = 1;
        Offset++;
      }
      if (MomCPU == CPU68HC11K4)
      {
        LargeWord AdrLWord = EvalStrIntExpressionOffsWithFlags(pStartArg, Offset, UInt21, &OK, &Flags);
        if (OK)
        {
          if (!def_phys_2_cpu(SegCode, &AdrLWord))
          {
            WrError(ErrNum_InAccPage);
            AdrWord = AdrLWord & 0xffffu;
          }
          else
            AdrWord = AdrLWord;
        }
      }
      else
        AdrWord = EvalStrIntExpressionOffsWithFlags(pStartArg, Offset, UInt16, &OK, &Flags);
      if (OK)
      {
        if ((MModDir & Erl) && (Bit8 != 1) && ((Bit8 == 2) || (!(MModExt & Erl)) || (Hi(AdrWord) == 0)))
        {
          if ((Hi(AdrWord) != 0) && !mFirstPassUnknown(Flags))
          {
            WrError(ErrNum_NoShortAddr);
            ErrOcc = True;
          }
          else
          {
            AdrMode = ModDir;
            AdrPart = 1;
            AdrVals[AdrCnt++] = Lo(AdrWord);
          }
        }
        else if ((MModExt & Erl)!=0)
        {
          AdrMode = ModExt;
          AdrPart = 3;
          AdrVals[AdrCnt++] = Hi(AdrWord);
          AdrVals[AdrCnt++] = Lo(AdrWord);
        }
      }
      else
        ErrOcc = True;
    }
  }

  /* zwei Komponenten ? */

  else if (StartInd + 1 == StopInd)
  {
    Boolean IsX = !as_strcasecmp(ArgStr[StopInd].str.p_str, "X"),
            IsY = !as_strcasecmp(ArgStr[StopInd].str.p_str, "Y");

    /* indiziert ? */

    if (IsX || IsY)
    {
      if (MModInd & Erl)
      {
        AdrWord = EvalStrIntExpression(pStartArg, UInt8, &OK);
        if (OK)
        {
          if (IsY && !ChkMinCPUExt(CPU6811, ErrNum_AddrModeNotSupported))
            ErrOcc = True;
          else
          {
            AdrVals[AdrCnt++] = Lo(AdrWord);
            AdrMode = ModInd;
            AdrPart = 2;
            if (IsY)
            {
              BAsmCode[PrefCnt++] = 0x18;
            }
          }
        }
        else
          ErrOcc = True;
      }
    }
    else
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[StopInd]);
      ErrOcc = True;
    }
  }

  else
  {
    char Str[100];

    as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgAddrArgCnt), 1, 2, StopInd - StartInd + 1);
    WrXError(ErrNum_WrongArgCnt, Str);
    ErrOcc = True;
  }

  if ((!ErrOcc) && (AdrMode == ModNone))
    WrError(ErrNum_InvAddrMode);
}

static void AddPrefix(Byte Prefix)
{
  BAsmCode[PrefCnt++] = Prefix;
}

static void Try2Split(int Src)
{
  char *p;
  size_t SrcLen;

  KillPrefBlanksStrComp(&ArgStr[Src]);
  KillPostBlanksStrComp(&ArgStr[Src]);
  SrcLen = strlen(ArgStr[Src].str.p_str);
  p = ArgStr[Src].str.p_str + SrcLen - 1;
  while ((p > ArgStr[Src].str.p_str) && !as_isspace(*p))
    p--;
  if (p > ArgStr[Src].str.p_str)
  {
    InsertArg(Src + 1, SrcLen);
    StrCompSplitRight(&ArgStr[Src], &ArgStr[Src + 1], p);
    KillPostBlanksStrComp(&ArgStr[Src]);
    KillPrefBlanksStrComp(&ArgStr[Src + 1]);
  }
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  const FixedOrder *forder = FixedOrders + Index;

  if (!ChkArgCnt(0, 0));
  else if (!ChkRangeCPU(forder->MinCPU, forder->MaxCPU));
  else if (Hi(forder->Code) != 0)
  {
    CodeLen = 2;
    BAsmCode[0] = Hi(forder->Code);
    BAsmCode[1] = Lo(forder->Code);
  }
  else
  {
    CodeLen = 1;
    BAsmCode[0] = Lo(forder->Code);
  }
}

static void DecodeRel(Word Index)
{
  const RelOrder *pOrder = &RelOrders[Index];
  Integer AdrInt;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(pOrder->MinCPU))
  {
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (OK)
    {
      AdrInt -= EProgCounter() + 2;
      if (((AdrInt < -128) || (AdrInt > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = pOrder->Code;
        BAsmCode[1] = Lo(AdrInt);
      }
    }
  }
}

static void DecodeALU16(Word Index)
{
  const ALU16Order *forder = ALU16Orders + Index;

  OpSize = eSymbolSize16Bit;
  if (ChkArgCnt(1, 2)
   && ChkMinCPU(forder->MinCPU))
  {
    DecodeAdr(1, ArgCnt, (forder->MayImm ? MModImm : 0) | MModInd | MModExt | MModDir);
    if (AdrMode != ModNone)
    {
      switch (forder->PageShift)
      {
        case 1:
          if (PrefCnt == 1)
            BAsmCode[PrefCnt - 1] = Page4Prefix;
          else
            AddPrefix(Page3Prefix);
          break;
        case 2:
          if (PrefCnt == 1)
            BAsmCode[PrefCnt - 1] = Page4Prefix;
          break;
        case 3:
          if (PrefCnt == 0)
            AddPrefix((AdrMode == ModInd) ? Page3Prefix : Page2Prefix);
          break;
      }
      BAsmCode[PrefCnt] = forder->Code + (AdrPart << 4);
      CodeLen = PrefCnt + 1 + AdrCnt;
      memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
    }
  }
}

static void DecodeBit63(Word Code)
{
  if (ChkArgCnt(2, 3)
   && ChkExactCPU(CPU6301))
  {
    DecodeAdr(1, 1, MModImm);
    if (AdrMode != ModNone)
    {
      DecodeAdr(2, ArgCnt, MModDir | MModInd);
      if (AdrMode != ModNone)
      {
        BAsmCode[PrefCnt] = Code;
        if (AdrMode == ModDir)
          BAsmCode[PrefCnt] |= 0x10;
        CodeLen = PrefCnt + 1 + AdrCnt;
        memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
      }
    }
  }
}

static void DecodeJMP(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 2))
  {
    DecodeAdr(1, ArgCnt, MModExt | MModInd);
    if (AdrMode != ModImm)
    {
      CodeLen = PrefCnt + 1 + AdrCnt;
      BAsmCode[PrefCnt] = 0x4e + (AdrPart << 4);
      memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
    }
  }
}

static void DecodeJSR(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 2))
  {
    DecodeAdr(1, ArgCnt, MModExt | MModInd | ((MomCPU >= CPU6801) ? MModDir : 0));
    if (AdrMode != ModImm)
    {
      CodeLen=PrefCnt + 1 + AdrCnt;
      BAsmCode[PrefCnt] = 0x8d + (AdrPart << 4);
      memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
    }
  }
}

static void DecodeBRxx(Word Index)
{
  Boolean OK;
  Byte Mask;
  Integer AdrInt;

  if (ArgCnt == 1)
  {
    Try2Split(1);
    Try2Split(1);
  }
  else if (ArgCnt == 2)
  {
    Try2Split(ArgCnt);
    Try2Split(2);
  }
  if (ChkArgCnt(3, 4)
   && ChkMinCPU(CPU6811))
  {
    Mask = EvalStrIntExpressionOffs(&ArgStr[ArgCnt - 1], !!(*ArgStr[ArgCnt - 1].str.p_str == '#'), Int8, &OK);
    if (OK)
    {
      DecodeAdr(1, ArgCnt - 2, MModDir | MModInd);
      if (AdrMode != ModNone)
      {
        AdrInt = EvalStrIntExpression(&ArgStr[ArgCnt], Int16, &OK);
        if (OK)
        {
          AdrInt -= EProgCounter() + 3 + PrefCnt + AdrCnt;
          if ((AdrInt < -128) || (AdrInt > 127)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            CodeLen = PrefCnt + 3 + AdrCnt;
            BAsmCode[PrefCnt] = 0x12 + Index;
            if (AdrMode == ModInd)
              BAsmCode[PrefCnt] += 12;
            memcpy(BAsmCode + PrefCnt + 1, AdrVals, AdrCnt);
            BAsmCode[PrefCnt + 1 + AdrCnt] = Mask;
            BAsmCode[PrefCnt + 2 + AdrCnt] = Lo(AdrInt);
          }
        }
      }
    }
  }
}

static void DecodeBxx(Word Index)
{
  Byte Mask;
  Boolean OK;
  int AddrStart, AddrEnd;
  tStrComp *pMaskArg;

  if (MomCPU == CPU6301)
  {
    pMaskArg = &ArgStr[1];
    AddrStart = 2;
    AddrEnd = ArgCnt;
  }
  else
  {
    if ((ArgCnt >= 1) && (ArgCnt <= 2)) Try2Split(ArgCnt);
    pMaskArg = &ArgStr[ArgCnt];
    AddrStart = 1;
    AddrEnd = ArgCnt - 1;
  }
  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPU6301))
  {
    Mask = EvalStrIntExpressionOffs(pMaskArg, !!(*pMaskArg->str.p_str == '#'),
                                    (MomCPU == CPU6301) ? UInt3 : Int8, &OK);
    if (OK && (MomCPU == CPU6301))
    {
      Mask = 1 << Mask;
      if (Index == 1) Mask = 0xff - Mask;
    }
    if (OK)
    {
      DecodeAdr(AddrStart, AddrEnd, MModDir | MModInd);
      if (AdrMode != ModNone)
      {
        CodeLen = PrefCnt + 2 + AdrCnt;
        if (MomCPU == CPU6301)
        {
          BAsmCode[PrefCnt] = 0x62 - Index;
          if (AdrMode == ModDir)
            BAsmCode[PrefCnt] += 0x10;
          BAsmCode[1 + PrefCnt] = Mask;
          memcpy(BAsmCode + 2 + PrefCnt, AdrVals, AdrCnt);
        }
        else
        {
          BAsmCode[PrefCnt] = 0x14 + Index;
          if (AdrMode == ModInd)
            BAsmCode[PrefCnt] += 8;
          memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
          BAsmCode[1 + PrefCnt + AdrCnt] = Mask;
        }
      }
    }
  }
}

static void DecodeBTxx(Word Index)
{
  Boolean OK;
  Byte AdrByte;

  if (ChkArgCnt(2, 3)
   && ChkExactCPU(CPU6301))
  {
    AdrByte = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), UInt3, &OK);
    if (OK)
    {
      DecodeAdr(2, ArgCnt, MModDir | MModInd);
      if (AdrMode != ModNone)
      {
        CodeLen = PrefCnt + 2 + AdrCnt;
        BAsmCode[1 + PrefCnt] = 1 << AdrByte;
        memcpy(BAsmCode + 2 + PrefCnt, AdrVals, AdrCnt);
        BAsmCode[PrefCnt] = 0x65 + Index;
        if (AdrMode == ModDir)
          BAsmCode[PrefCnt] += 0x10;
      }
    }
  }
}

static void DecodeALU8(Word Code)
{
  Byte Reg;
  int MinArgCnt = Hi(Code) & 3;

  /* dirty hack: LDA/STA/ORA, and first arg is not A or B, treat like LDAA/STAA/ORAA: */

  if ((MinArgCnt == 2)
   && (as_toupper(OpPart.str.p_str[2]) == 'A')
   && (ArgCnt >= 1)
   && !DecodeAcc(ArgStr[1].str.p_str, &Reg))
    MinArgCnt = 1;

  if (ChkArgCnt(MinArgCnt, MinArgCnt + 1))
  {
    DecodeAdr(MinArgCnt , ArgCnt, ((Code & 0x8000) ? MModImm : 0) | MModInd | MModExt | MModDir);
    if (AdrMode != ModNone)
    {
      BAsmCode[PrefCnt] = Lo(Code) | (AdrPart << 4);
      if (MinArgCnt == 1)
      {
        AdrMode = ModAcc;
        AdrPart = (Code & 0x4000) >> 14;
      }
      else
        DecodeAdr(1, 1, MModAcc);
      if (AdrMode != ModNone)
      {
        BAsmCode[PrefCnt] |= AdrPart << 6;
        CodeLen = PrefCnt + 1 + AdrCnt;
        memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
      }
    }
  }
}

static void DecodeSing8(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    DecodeAdr(1, ArgCnt, MModAcc | MModExt | MModInd);
    if (AdrMode!=ModNone)
    {
      CodeLen = PrefCnt + 1 + AdrCnt;
      BAsmCode[PrefCnt] = Code | (AdrPart << 4);
      memcpy(BAsmCode + 1 + PrefCnt, AdrVals, AdrCnt);
    }
  }
}

static void DecodeSing8_Acc(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[PrefCnt] = Code;
    CodeLen = PrefCnt + 1;
  }
}

static void DecodePSH_PUL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(1, 1, MModAcc);
    if (AdrMode != ModNone)
    {
      CodeLen = 1;
      BAsmCode[0]=Code | AdrPart;
    }
  }
}

static void DecodePRWINS(Word Code)
{
  UNUSED(Code);

  if (ChkExactCPU(CPU68HC11K4))
  {
    printf("\nMMSIZ $%02x MMWBR $%02x MM1CR $%02x MM2CR $%02x INIT $%02x INIT2 $%02x CONFIG $%02x\n",
           (unsigned)Reg_MMSIZ, (unsigned)Reg_MMWBR, (unsigned)Reg_MM1CR, (unsigned)Reg_MM2CR,
           (unsigned)Reg_INIT, (unsigned)Reg_INIT2, (unsigned)Reg_CONFIG);
    cpu_2_phys_area_dump(SegCode, stdout);
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, CPUVar NMin, CPUVar NMax, Word NCode)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].MinCPU = NMin;
  FixedOrders[InstrZ].MaxCPU = NMax;
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddRel(const char *NName, CPUVar NMin, Word NCode)
{
  order_array_rsv_end(RelOrders, RelOrder);
  RelOrders[InstrZ].MinCPU = NMin;
  RelOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRel);
}

static void AddALU8(const char *NamePlain, const char *NameA, const char *NameB, const char *NameB2, Boolean MayImm, Byte NCode)
{
  Word BaseCode = NCode | (MayImm ? 0x8000 : 0);

  AddInstTable(InstTable, NamePlain, BaseCode | (2 << 8), DecodeALU8);
  AddInstTable(InstTable, NameA, BaseCode | (1 << 8), DecodeALU8);
  AddInstTable(InstTable, NameB, BaseCode | (1 << 8) | 0x4000, DecodeALU8);
  if (NameB2)
    AddInstTable(InstTable, NameB2, BaseCode | (1 << 8) | 0x4000, DecodeALU8);
}

static void AddALU16(const char *NName, Boolean NMay, CPUVar NMin, Byte NShift, Byte NCode)
{
  order_array_rsv_end(ALU16Orders, ALU16Order);
  ALU16Orders[InstrZ].MayImm = NMay;
  ALU16Orders[InstrZ].MinCPU = NMin;
  ALU16Orders[InstrZ].PageShift = NShift;
  ALU16Orders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU16);
}

static void AddSing8(const char *NamePlain, const char *NameA, const char *NameB, Byte NCode)
{
  AddInstTable(InstTable, NamePlain, NCode, DecodeSing8);
  AddInstTable(InstTable, NameA, NCode | 0, DecodeSing8_Acc);
  AddInstTable(InstTable, NameB, NCode | 0x10, DecodeSing8_Acc);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(317);
  AddInstTable(InstTable, "JMP"  , 0, DecodeJMP);
  AddInstTable(InstTable, "JSR"  , 0, DecodeJSR);
  AddInstTable(InstTable, "BRCLR", 1, DecodeBRxx);
  AddInstTable(InstTable, "BRSET", 0, DecodeBRxx);
  AddInstTable(InstTable, "BCLR" , 1, DecodeBxx);
  AddInstTable(InstTable, "BSET" , 0, DecodeBxx);
  AddInstTable(InstTable, "BTST" , 6, DecodeBTxx);
  AddInstTable(InstTable, "BTGL" , 0, DecodeBTxx);

  InstrZ = 0;
  AddFixed("ABA"  ,CPU6800, CPU68HC11K4, 0x001b); AddFixed("ABX"  ,CPU6801, CPU68HC11K4, 0x003a);
  AddFixed("ABY"  ,CPU6811, CPU68HC11K4, 0x183a); AddFixed("ASLD" ,CPU6801, CPU68HC11K4, 0x0005);
  AddFixed("CBA"  ,CPU6800, CPU68HC11K4, 0x0011); AddFixed("CLC"  ,CPU6800, CPU68HC11K4, 0x000c);
  AddFixed("CLI"  ,CPU6800, CPU68HC11K4, 0x000e); AddFixed("CLV"  ,CPU6800, CPU68HC11K4, 0x000a);
  AddFixed("DAA"  ,CPU6800, CPU68HC11K4, 0x0019); AddFixed("DES"  ,CPU6800, CPU68HC11K4, 0x0034);
  AddFixed("DEX"  ,CPU6800, CPU68HC11K4, 0x0009); AddFixed("DEY"  ,CPU6811, CPU68HC11K4, 0x1809);
  AddFixed("FDIV" ,CPU6811, CPU68HC11K4, 0x0003); AddFixed("IDIV" ,CPU6811, CPU68HC11K4, 0x0002);
  AddFixed("INS"  ,CPU6800, CPU68HC11K4, 0x0031); AddFixed("INX"  ,CPU6800, CPU68HC11K4, 0x0008);
  AddFixed("INY"  ,CPU6811, CPU68HC11K4, 0x1808); AddFixed("LSLD" ,CPU6801, CPU68HC11K4, 0x0005);
  AddFixed("LSRD" ,CPU6801, CPU68HC11K4, 0x0004); AddFixed("MUL"  ,CPU6801, CPU68HC11K4, 0x003d);
  AddFixed("NOP"  ,CPU6800, CPU68HC11K4, 0x0001); AddFixed("PSHX" ,CPU6801, CPU68HC11K4, 0x003c);
  AddFixed("PSHY" ,CPU6811, CPU68HC11K4, 0x183c); AddFixed("PULX" ,CPU6801, CPU68HC11K4, 0x0038);
  AddFixed("PULY" ,CPU6811, CPU68HC11K4, 0x1838); AddFixed("RTI"  ,CPU6800, CPU68HC11K4, 0x003b);
  AddFixed("RTS"  ,CPU6800, CPU68HC11K4, 0x0039); AddFixed("SBA"  ,CPU6800, CPU68HC11K4, 0x0010);
  AddFixed("SEC"  ,CPU6800, CPU68HC11K4, 0x000d); AddFixed("SEI"  ,CPU6800, CPU68HC11K4, 0x000f);
  AddFixed("SEV"  ,CPU6800, CPU68HC11K4, 0x000b); AddFixed("SLP"  ,CPU6301, CPU6301    , 0x001a);
  AddFixed("STOP" ,CPU6811, CPU68HC11K4, 0x00cf); AddFixed("SWI"  ,CPU6800, CPU68HC11K4, 0x003f);
  AddFixed("TAB"  ,CPU6800, CPU68HC11K4, 0x0016); AddFixed("TAP"  ,CPU6800, CPU68HC11K4, 0x0006);
  AddFixed("TBA"  ,CPU6800, CPU68HC11K4, 0x0017); AddFixed("TPA"  ,CPU6800, CPU68HC11K4, 0x0007);
  AddFixed("TSX"  ,CPU6800, CPU68HC11K4, 0x0030); AddFixed("TSY"  ,CPU6811, CPU68HC11K4, 0x1830);
  AddFixed("TXS"  ,CPU6800, CPU68HC11K4, 0x0035); AddFixed("TYS"  ,CPU6811, CPU68HC11K4, 0x1835);
  AddFixed("WAI"  ,CPU6800, CPU68HC11K4, 0x003e);
  AddFixed("XGDX" ,CPU6301, CPU68HC11K4, (MomCPU == CPU6301) ? 0x0018 : 0x008f);
  AddFixed("XGDY" ,CPU6811, CPU68HC11K4, 0x188f);

  InstrZ = 0;
  AddRel("BCC", CPU6800, 0x24);
  AddRel("BCS", CPU6800, 0x25);
  AddRel("BEQ", CPU6800, 0x27);
  AddRel("BGE", CPU6800, 0x2c);
  AddRel("BGT", CPU6800, 0x2e);
  AddRel("BHI", CPU6800, 0x22);
  AddRel("BHS", CPU6800, 0x24);
  AddRel("BLE", CPU6800, 0x2f);
  AddRel("BLO", CPU6800, 0x25);
  AddRel("BLS", CPU6800, 0x23);
  AddRel("BLT", CPU6800, 0x2d);
  AddRel("BMI", CPU6800, 0x2b);
  AddRel("BNE", CPU6800, 0x26);
  AddRel("BPL", CPU6800, 0x2a);
  AddRel("BRA", CPU6800, 0x20);
  AddRel("BRN", CPU6801, 0x21);
  AddRel("BSR", CPU6800, 0x8d);
  AddRel("BVC", CPU6800, 0x28);
  AddRel("BVS", CPU6800, 0x29);

  AddALU8("ADC", "ADCA", "ADCB", NULL , True , 0x89);
  AddALU8("ADD", "ADDA", "ADDB", NULL , True , 0x8b);
  AddALU8("AND", "ANDA", "ANDB", NULL , True , 0x84);
  AddALU8("BIT", "BITA", "BITB", NULL , True , 0x85);
  AddALU8("CMP", "CMPA", "CMPB", NULL , True , 0x81);
  AddALU8("EOR", "EORA", "EORB", NULL , True , 0x88);
  AddALU8("LDA", "LDAA", "LDAB", "LDB", True , 0x86);
  AddALU8("ORA", "ORAA", "ORAB", "ORB", True , 0x8a);
  AddALU8("SBC", "SBCA", "SBCB", NULL , True , 0x82);
  AddALU8("STA", "STAA", "STAB", "STB", False, 0x87);
  AddALU8("SUB", "SUBA", "SUBB", NULL , True , 0x80);

  InstrZ = 0;
  AddALU16("ADDD", True , CPU6801, 0, 0xc3);
  AddALU16("CPD" , True , CPU6811, 1, 0x83);
  AddALU16("CMPD", True , CPU6811, 1, 0x83);
  AddALU16("CPX" , True , CPU6800, 2, 0x8c);
  AddALU16("CMPX", True , CPU6800, 2, 0x8c);
  AddALU16("CPY" , True , CPU6811, 3, 0x8c);
  AddALU16("CMPY", True , CPU6811, 3, 0x8c);
  AddALU16("LDD" , True , CPU6801, 0, 0xcc);
  AddALU16("LDS" , True , CPU6800, 0, 0x8e);
  AddALU16("LDX" , True , CPU6800, 2, 0xce);
  AddALU16("LDY" , True , CPU6811, 3, 0xce);
  AddALU16("STD" , False, CPU6801, 0, 0xcd);
  AddALU16("STS" , False, CPU6800, 0, 0x8f);
  AddALU16("STX" , False, CPU6800, 2, 0xcf);
  AddALU16("STY" , False, CPU6811, 3, 0xcf);
  AddALU16("SUBD", True , CPU6801, 0, 0x83);

  AddSing8("ASL", "ASLA", "ASLB", 0x48);
  AddSing8("ASR", "ASRA", "ASRB", 0x47);
  AddSing8("CLR", "CLRA", "CLRB", 0x4f);
  AddSing8("COM", "COMA", "COMB", 0x43);
  AddSing8("DEC", "DECA", "DECB", 0x4a);
  AddSing8("INC", "INCA", "INCB", 0x4c);
  AddSing8("LSL", "LSLA", "LSLB", 0x48);
  AddSing8("LSR", "LSRA", "LSRB", 0x44);
  AddSing8("NEG", "NEGA", "NEGB", 0x40);
  AddSing8("ROL", "ROLA", "ROLB", 0x49);
  AddSing8("ROR", "RORA", "RORB", 0x46);
  AddSing8("TST", "TSTA", "TSTB", 0x4d);

  AddInstTable(InstTable, "PSH" , 0x36, DecodePSH_PUL);
  AddInstTable(InstTable, "PSHA", 0x36, DecodeSing8_Acc);
  AddInstTable(InstTable, "PSHB", 0x37, DecodeSing8_Acc);
  AddInstTable(InstTable, "PUL" , 0x32, DecodePSH_PUL);
  AddInstTable(InstTable, "PULA", 0x32, DecodeSing8_Acc);
  AddInstTable(InstTable, "PULB", 0x33, DecodeSing8_Acc);

  AddInstTable(InstTable, "AIM", 0x61, DecodeBit63);
  AddInstTable(InstTable, "EIM", 0x65, DecodeBit63);
  AddInstTable(InstTable, "OIM", 0x62, DecodeBit63);
  AddInstTable(InstTable, "TIM", 0x6b, DecodeBit63);

  AddInstTable(InstTable, "PRWINS", 0, DecodePRWINS);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(RelOrders);
  order_array_free(ALU16Orders);
}

static Boolean DecodeAttrPart_68(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }
  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_68(void)
{
  CodeLen = 0;
  DontPrint = False;
  PrefCnt = 0;
  AdrCnt = 0;

  /* Operandengroesse festlegen */

  OpSize = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSize8Bit;

  /* zu ignorierendes */

  if (*OpPart.str.p_str == '\0')
    return;

  /* Pseudoanweisungen */

  if (DecodeMoto16Pseudo(OpSize, True))
    return;

  /* gehashtes */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_68(void)
{
  Reg_MMSIZ = Reg_MMWBR = Reg_MM1CR = Reg_MM2CR = 0;
}

static Boolean IsDef_68(void)
{
  return False;
}

static void SwitchTo_68(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x61;
  NOPCode = 0x01;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = (MomCPU == CPU68HC11K4) ? 0x10ffffl : 0xffff;

  DecodeAttrPart = DecodeAttrPart_68;
  MakeCode = MakeCode_68;
  IsDef = IsDef_68;
  SwitchFrom = DeinitFields;
  InitFields();
  AddMoto16PseudoONOFF(False);

  if (MomCPU == CPU68HC11K4)
  {
    static const ASSUMERec ASSUMEHC11s[] =
    {
      {"MMSIZ" , &Reg_MMSIZ , 0, 0xff, 0, SetK4Ranges},
      {"MMWBR" , &Reg_MMWBR , 0, 0xff, 0, SetK4Ranges},
      {"MM1CR" , &Reg_MM1CR , 0, 0xff, 0, SetK4Ranges},
      {"MM2CR" , &Reg_MM2CR , 0, 0xff, 0, SetK4Ranges},
      {"INIT"  , &Reg_INIT  , 0, 0xff, 0, SetK4Ranges},
      {"INIT2" , &Reg_INIT2 , 0, 0xff, 0, SetK4Ranges},
      {"CONFIG", &Reg_CONFIG, 0, 0xff, 0, SetK4Ranges},
    };

    pASSUMERecs = ASSUMEHC11s;
    ASSUMERecCnt = as_array_size(ASSUMEHC11s);

    SetK4Ranges();
  }
  else
    cpu_2_phys_area_clear(SegCode);
}

void code68_init(void)
{
  CPU6800 = AddCPU("6800", SwitchTo_68);
  CPU6801 = AddCPU("6801", SwitchTo_68);
  CPU6301 = AddCPU("6301", SwitchTo_68);
  CPU6811 = AddCPU("6811", SwitchTo_68);
  CPU68HC11K4 = AddCPU("68HC11K4", SwitchTo_68);

  AddInitPassProc(InitCode_68);
}
