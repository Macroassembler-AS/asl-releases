/* code86.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 8086/V-Serie                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "asmitree.h"
#include "symbolsize.h"
#include "codevars.h"
#include "nlmessages.h"
#include "as.rsc"
#include "code86.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  const char *Name;
  Byte core_mask;
  Word Code;
} FixedOrder;

typedef struct
{
  const char *Name;
  Byte core_mask;
  Word Code;
  Boolean no_seg_check;
} ModRegOrder;

typedef struct
{
  CPUVar MinCPU;
  Word Code;
  Byte Add;
} AddOrder;

#define NO_FWAIT_FLAG 0x2000

#define SegRegCnt 6
static const char SegRegNames[SegRegCnt][4] =
{
  "ES", "CS", "SS", "DS",
  "DS3", "DS2" /* V55 specific */
};
static const Byte SegRegPrefixes[SegRegCnt] =
{
  0x26, 0x2e, 0x36, 0x3e,
  0xd6, 0x63
};

static char ArgSTStr[] = "ST";
static const tStrComp ArgST = { { 0, 0 }, { 0, ArgSTStr, 0 } };

typedef enum
{
  TypeNone = -1,
  TypeReg8 = 0,
  TypeReg16 = 1,
  TypeRegSeg = 2,
  TypeMem = 3,
  TypeImm = 4,
  TypeFReg = 5
} tAdrType;

#define MTypeReg8 (1 << TypeReg8)
#define MTypeReg16 (1 << TypeReg16)
#define MTypeRegSeg (1 << TypeRegSeg)
#define MTypeMem (1 << TypeMem)
#define MTypeImm (1 << TypeImm)
#define MTypeFReg (1 << TypeFReg)

static tAdrType AdrType;
static Byte AdrMode;
static Byte AdrVals[6];
static tSymbolSize OpSize;
static Boolean UnknownFlag;
static unsigned ImmAddrSpaceMask;

static Boolean NoSegCheck;

static Byte Prefixes[6];
static Byte PrefixLen;

static Byte SegAssumes[SegRegCnt];

enum
{
  e_core_86    = 1 << 0, /* 8086/8088 */
  e_core_186   = 1 << 1, /* 80186/80188 */
  e_core_v30   = 1 << 2, /* V20/30/40/50 */
  e_core_v33   = 1 << 3, /* V33/V53 */
  e_core_v35   = 1 << 4, /* V25/V35 */
  e_core_v55   = 1 << 5, /* V55 */
  e_core_v55pi = 1 << 6, /* V55PI */
  e_core_v55sc = 1 << 7, /* V55SC */

  e_core_all_v55 = e_core_v55 | e_core_v55pi | e_core_v55sc,
  e_core_all_v35 = e_core_v35 | e_core_all_v55,
  e_core_all_v = e_core_v30 | e_core_v33 | e_core_all_v35,
  e_core_all_186 = e_core_all_v | e_core_186,
  e_core_all = e_core_all_186 | e_core_86
};

typedef struct
{
  const char name[6];
  Byte core;
} cpu_props_t;

static const cpu_props_t *p_curr_cpu_props;

static FixedOrder *FixedOrders, *StringOrders, *ReptOrders, *ShiftOrders,
                  *RelOrders, *BrkOrders, *Imm16Orders;
static AddOrder *Reg16Orders;
static ModRegOrder *ModRegOrders;
static unsigned StringOrderCnt;

/*------------------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     PutCode(Word Code)
 * \brief  append 1- or 2-byte machine code to instruction stream
 * \param  Code machine code to append
 * ------------------------------------------------------------------------ */

static void PutCode(Word Code)
{
  if (Hi(Code) != 0)
    BAsmCode[CodeLen++] = Hi(Code);
  BAsmCode[CodeLen++] = Lo(Code);
}

/*!------------------------------------------------------------------------
 * \fn     copy_adr_vals(int Dest)
 * \brief  copy addressing mode extension bytes
 * \param  Dest where to copy relative to current instruction end
 * ------------------------------------------------------------------------ */

static void copy_adr_vals(int Dest)
{
  memcpy(BAsmCode + CodeLen + Dest, AdrVals, AdrCnt);
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals(void)
 * \brief  append addressing mode extension bytes
 * ------------------------------------------------------------------------ */

static void append_adr_vals(void)
{
  copy_adr_vals(0);
  CodeLen += AdrCnt;
}

/*!------------------------------------------------------------------------
 * \fn     Sgn(Byte inp)
 * \brief  get value of upper byte after sign extension
 * \param  inp value that will be extended
 * \return 0 or 0xff
 * ------------------------------------------------------------------------ */

static Byte Sgn(Byte inp)
{
  return (inp > 127) ? 0xff : 0;
}

/*!------------------------------------------------------------------------
 * \fn     AddPrefix(Byte Prefix)
 * \brief  store new prefix
 * \param  Prefix prefix byte to store
 * ------------------------------------------------------------------------ */

static void AddPrefix(Byte Prefix)
{
  Prefixes[PrefixLen++] = Prefix;
}

/*!------------------------------------------------------------------------
 * \fn     AddPrefixes(void)
 * \brief  prepend stored prefixes
 * ------------------------------------------------------------------------ */

static void AddPrefixes(void)
{
  if ((CodeLen != 0) && (PrefixLen != 0))
  {
    memmove(BAsmCode + PrefixLen, BAsmCode, CodeLen);
    memcpy(BAsmCode, Prefixes, PrefixLen);
    CodeLen += PrefixLen;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AbleToSign(Word Arg)
 * \brief  can argument be written as 8-bit vlaue that will be sign extended?
 * \param  Arg value to check
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean AbleToSign(Word Arg)
{
  return ((Arg <= 0x7f) || (Arg >= 0xff80));
}

/*!------------------------------------------------------------------------
 * \fn     MinOneIs0(void)
 * \brief  optionally set operand size to 8 bits if not yet set
 * \return True if operand size was set
 * ------------------------------------------------------------------------ */

static Boolean MinOneIs0(void)
{
  if (UnknownFlag && (OpSize == eSymbolSizeUnknown))
  {
    OpSize = eSymbolSize8Bit;
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     ChkOpSize(tSymbolSize NewSize)
 * \brief  check, match and optionally set operand size
 * \param  NewSize operand size of operand
 * ------------------------------------------------------------------------ */

static void ChkOpSize(tSymbolSize NewSize)
{
  if (OpSize == eSymbolSizeUnknown)
    OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    AdrType = TypeNone;
    WrError(ErrNum_ConfOpSizes);
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkSpaces(ShortInt SegBuffer, Byte MomSegment, const tStrComp *p_arg)
 * \brief  check for matching address space of memory argument
 * \param  SegBuffer index into segment assume table
 * \param  MomSegment current segment being used
 * \param  p_arg source argument for error reporting
 * ------------------------------------------------------------------------ */

static void ChkSingleSpace(Byte Seg, Byte EffSeg, Byte MomSegment, const tStrComp *p_arg)
{
  Byte z;

  /* liegt Operand im zu pruefenden Segment? nein-->vergessen */

  if (!(MomSegment & (1 << Seg)))
    return;

  /* zeigt bish. benutztes Segmentregister auf dieses Segment? ja-->ok */

  if (EffSeg == Seg)
    return;

  /* falls schon ein Override gesetzt wurde, nur warnen */

  if (PrefixLen > 0)
    WrStrErrorPos(ErrNum_WrongSegment, p_arg);

  /* ansonsten ein passendes Segment suchen und warnen, falls keines da */

  else
  {
    z = 0;
    while ((z < SegRegCnt) && (SegAssumes[z] != Seg))
      z++;
    if (z > SegRegCnt)
      WrXErrorPos(ErrNum_InAccSegment, SegRegNames[Seg], &p_arg->Pos);
    else
      AddPrefix(SegRegPrefixes[z]);
  }
}

static void ChkSpaces(ShortInt SegBuffer, Byte MomSegment, const tStrComp *p_arg)
{
  Byte EffSeg;

  if (NoSegCheck)
    return;

  /* in welches Segment geht das benutzte Segmentregister ? */

  EffSeg = SegAssumes[SegBuffer];

  /* Zieloperand in Code-/Datensegment ? */

  ChkSingleSpace(SegCode, EffSeg, MomSegment, p_arg);
  ChkSingleSpace(SegXData, EffSeg, MomSegment, p_arg);
  ChkSingleSpace(SegData, EffSeg, MomSegment, p_arg);
}

/*!------------------------------------------------------------------------
 * \fn     decode_seg_reg(const char *p_arg, Byte *p_ret)
 * \brief  dcheck whether argument is a segment register's name
 * \param  p_arg source argument
 * \param  p_ret returns register # if so
 * \return True if argument is segment register
 * ------------------------------------------------------------------------ */

static Boolean decode_seg_reg(const char *p_arg, Byte *p_ret)
{
  int reg_z, reg_cnt = SegRegCnt;

  /* DS2/DS3 only allowed on V55.  These names should be allowed as
     ordinary symbol names on other targets: */

  if (!(p_curr_cpu_props->core & e_core_all_v55))
    reg_cnt -= 2;
  for (reg_z = 0; reg_z < reg_cnt; reg_z++)
    if (!as_strcasecmp(p_arg, SegRegNames[reg_z]))
    {
      *p_ret = reg_z;
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, unsigned type_mask)
 * \brief  parse addressing mode argument
 * \param  pArg source argument
 * \param  type_mask bit mask of allowed addressing modes
 * \return resulting addressing mode
 * ------------------------------------------------------------------------ */

static tAdrType DecodeAdr(const tStrComp *pArg, unsigned type_mask)
{
  static const int RegCnt = 8;
  static const char Reg16Names[8][3] =
  {
    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"
  };
  static const char Reg8Names[8][3] =
  {
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"
  };
  static const Byte RMCodes[8] =
  {
    11, 12, 21, 22, 1, 2 , 20, 10
  };

  int RegZ, z;
  Boolean IsImm;
  ShortInt IndexBuf, BaseBuf;
  Byte SumBuf;
  LongInt DispAcc, DispSum;
  char *pIndirStart, *pIndirEnd, *pSep;
  ShortInt SegBuffer;
  Byte MomSegment;
  tSymbolSize FoundSize;
  tStrComp Arg;
  int ArgLen = strlen(pArg->str.p_str);

  AdrType = TypeNone; AdrCnt = 0;
  SegBuffer = -1; MomSegment = 0;

  for (RegZ = 0; RegZ < RegCnt; RegZ++)
  {
    if (!as_strcasecmp(pArg->str.p_str, Reg16Names[RegZ]))
    {
      AdrType = TypeReg16; AdrMode = RegZ;
      ChkOpSize(eSymbolSize16Bit);
      goto chk_type;
    }
    if (!as_strcasecmp(pArg->str.p_str, Reg8Names[RegZ]))
    {
      AdrType = TypeReg8; AdrMode = RegZ;
      ChkOpSize(eSymbolSize8Bit);
      goto chk_type;
    }
  }

  if (decode_seg_reg(pArg->str.p_str, &AdrMode))
  {
    AdrType = TypeRegSeg;
    ChkOpSize(eSymbolSize16Bit);
    goto chk_type;
  }

  if (FPUAvail)
  {
    if (!as_strcasecmp(pArg->str.p_str, "ST"))
    {
      AdrType = TypeFReg; AdrMode = 0;
      ChkOpSize(eSymbolSize80Bit);
      goto chk_type;
    }

    if ((ArgLen > 4) && (!as_strncasecmp(pArg->str.p_str, "ST(", 3)) && (pArg->str.p_str[ArgLen - 1] == ')'))
    {
      tStrComp Num;
      Boolean OK;

      StrCompRefRight(&Num, pArg, 3);
      StrCompShorten(&Num, 1);
      AdrMode = EvalStrIntExpression(&Num, UInt3, &OK);
      if (OK)
      {
        AdrType = TypeFReg;
        ChkOpSize(eSymbolSize80Bit);
      }
      goto chk_type;
    }
  }

  IsImm = True;
  IndexBuf = 0; BaseBuf = 0;
  DispAcc = 0; FoundSize = eSymbolSizeUnknown;
  StrCompRefRight(&Arg, pArg, 0);
  if (!as_strncasecmp(Arg.str.p_str, "WORD PTR", 8))
  {
    StrCompIncRefLeft(&Arg, 8);
    FoundSize = eSymbolSize16Bit;
    IsImm = False;
    KillPrefBlanksStrCompRef(&Arg);
  }
  else if (!as_strncasecmp(Arg.str.p_str, "BYTE PTR", 8))
  {
    StrCompIncRefLeft(&Arg, 8);
    FoundSize = eSymbolSize8Bit;
    IsImm = False;
    KillPrefBlanksStrCompRef(&Arg);
  }
  else if (!as_strncasecmp(Arg.str.p_str, "DWORD PTR", 9))
  {
    StrCompIncRefLeft(&Arg, 9);
    FoundSize = eSymbolSize32Bit;
    IsImm = False;
    KillPrefBlanksStrCompRef(&Arg);
  }
  else if (!as_strncasecmp(Arg.str.p_str, "QWORD PTR", 9))
  {
    StrCompIncRefLeft(&Arg, 9);
    FoundSize = eSymbolSize64Bit;
    IsImm = False;
    KillPrefBlanksStrCompRef(&Arg);
  }
  else if (!as_strncasecmp(Arg.str.p_str, "TBYTE PTR", 9))
  {
    StrCompIncRefLeft(&Arg, 9);
    FoundSize = eSymbolSize80Bit;
    IsImm = False;
    KillPrefBlanksStrCompRef(&Arg);
  }

  if ((strlen(Arg.str.p_str) > 2) && (Arg.str.p_str[2] == ':'))
  {
    tStrComp Remainder;
    Byte seg_reg;

    StrCompSplitRef(&Arg, &Remainder, &Arg, Arg.str.p_str + 2);
    if (decode_seg_reg(Arg.str.p_str, &seg_reg))
    {
      SegBuffer = seg_reg;
      AddPrefix(SegRegPrefixes[SegBuffer]);
    }
    if (SegBuffer < 0)
    {
      WrStrErrorPos(ErrNum_InvReg, &Arg);
      goto chk_type;
    }
    Arg = Remainder;
  }

  do
  {
    pIndirStart = QuotPos(Arg.str.p_str, '[');

    /* no address expr or outer displacement: */

    if (!pIndirStart || (pIndirStart != Arg.str.p_str))
    {
      tStrComp Remainder;
      tEvalResult EvalResult;

      if (pIndirStart)
        StrCompSplitRef(&Arg, &Remainder, &Arg, pIndirStart);
      DispAcc += EvalStrIntExpressionWithResult(&Arg, Int16, &EvalResult);
      if (!EvalResult.OK)
         goto chk_type;
      UnknownFlag = UnknownFlag || mFirstPassUnknown(EvalResult.Flags);
      MomSegment |= EvalResult.AddrSpaceMask;
      if (FoundSize == eSymbolSizeUnknown)
        FoundSize = EvalResult.DataSize;
      if (pIndirStart)
        Arg = Remainder;
      else
        break;
    }
    else
      StrCompIncRefLeft(&Arg, 1);

    /* Arg now points right behind [ */

    if (pIndirStart)
    {
      tStrComp IndirArg, OutRemainder, IndirArgRemainder;
      Boolean NegFlag, OldNegFlag;

      IsImm = False;

      pIndirEnd = RQuotPos(Arg.str.p_str, ']');
      if (!pIndirEnd)
      {
        WrStrErrorPos(ErrNum_BrackErr, &Arg);
        goto chk_type;
      }

      StrCompSplitRef(&IndirArg, &OutRemainder, &Arg, pIndirEnd);
      OldNegFlag = False;

      do
      {
        NegFlag = False;
        pSep = QuotMultPos(IndirArg.str.p_str, "+-");
        NegFlag = pSep && (*pSep == '-');

        if (pSep)
          StrCompSplitRef(&IndirArg, &IndirArgRemainder, &IndirArg, pSep);

        if (!as_strcasecmp(IndirArg.str.p_str, "BX"))
        {
          if ((OldNegFlag) || (BaseBuf != 0))
            goto chk_type;
          else
            BaseBuf = 1;
        }
        else if (!as_strcasecmp(IndirArg.str.p_str, "BP"))
        {
          if ((OldNegFlag) || (BaseBuf != 0))
            goto chk_type;
          else
            BaseBuf = 2;
        }
        else if (!as_strcasecmp(IndirArg.str.p_str, "SI"))
        {
          if ((OldNegFlag) || (IndexBuf != 0))
            goto chk_type;
          else
            IndexBuf = 1;
        }
        else if (!as_strcasecmp(IndirArg.str.p_str, "DI"))
        {
          if ((OldNegFlag) || (IndexBuf !=0 ))
            goto chk_type;
          else
            IndexBuf = 2;
        }
        else
        {
          tEvalResult EvalResult;

          DispSum = EvalStrIntExpressionWithResult(&IndirArg, Int16, &EvalResult);
          if (!EvalResult.OK)
            goto chk_type;
          UnknownFlag = UnknownFlag || mFirstPassUnknown(EvalResult.Flags);
          DispAcc = OldNegFlag ? DispAcc - DispSum : DispAcc + DispSum;
          MomSegment |= EvalResult.AddrSpaceMask;
          if (FoundSize == eSymbolSizeUnknown)
            FoundSize = EvalResult.DataSize;
        }
        OldNegFlag = NegFlag;
        if (pSep)
          IndirArg = IndirArgRemainder;
      }
      while (pSep);
      Arg = OutRemainder;
    }
  }
  while (*Arg.str.p_str);

  SumBuf = BaseBuf * 10 + IndexBuf;

  /* welches Segment effektiv benutzt ? */

  if (SegBuffer == -1)
    SegBuffer = (BaseBuf == 2) ? 2 : 3;

  /* nur Displacement */

  if (0 == SumBuf)
  {
    /* immediate */

    if (IsImm)
    {
      ImmAddrSpaceMask = MomSegment;
      if ((UnknownFlag && (OpSize == eSymbolSize8Bit)) || (MinOneIs0()))
        DispAcc &= 0xff;
      switch (OpSize)
      {
        case eSymbolSizeUnknown:
          WrStrErrorPos(ErrNum_UndefOpSizes, &Arg);
          break;
        case eSymbolSize8Bit:
          if ((DispAcc <- 128) || (DispAcc > 255)) WrStrErrorPos(ErrNum_OverRange, &Arg);
          else
          {
            AdrType = TypeImm;
            AdrVals[0] = DispAcc & 0xff;
            AdrCnt = 1;
          }
          break;
        case eSymbolSize16Bit:
          AdrType = TypeImm;
          AdrVals[0] = Lo(DispAcc);
          AdrVals[1] = Hi(DispAcc);
          AdrCnt = 2;
          break;
        default:
          WrStrErrorPos(ErrNum_InvOpSize, &Arg);
          break;
      }
    }

    /* absolut */

    else
    {
      AdrType = TypeMem;
      AdrMode = 0x06;
      AdrVals[0] = Lo(DispAcc);
      AdrVals[1] = Hi(DispAcc);
      AdrCnt = 2;
      if (FoundSize != -1)
        ChkOpSize(FoundSize);
      ChkSpaces(SegBuffer, MomSegment, &Arg);
    }
  }

  /* kombiniert */

  else
  {
    AdrType = TypeMem;
    for (z = 0; z < 8; z++)
      if (SumBuf == RMCodes[z])
        AdrMode = z;
    if (DispAcc == 0)
    {
      if (SumBuf == 20)
      {
        AdrMode += 0x40;
        AdrVals[0] = 0;
        AdrCnt = 1;
      }
    }
    else if (AbleToSign(DispAcc))
    {
      AdrMode += 0x40;
      AdrVals[0] = DispAcc & 0xff;
      AdrCnt = 1;
    }
    else
    {
      AdrMode += 0x80;
      AdrVals[0] = Lo(DispAcc);
      AdrVals[1] = Hi(DispAcc);
      AdrCnt = 2;
    }
    ChkSpaces(SegBuffer, MomSegment, &Arg);
    if (FoundSize != -1)
      ChkOpSize(FoundSize);
  }

chk_type:
  if ((AdrType != TypeNone) && !((type_mask >> AdrType) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    AdrType = TypeNone;
    AdrCnt = 0;
  }
  return AdrType;
}

/*!------------------------------------------------------------------------
 * \fn     AddFWait(Word *pCode)
 * \brief  add FPU instruction entry code
 * \param  pCode machine code of instruction
 * ------------------------------------------------------------------------ */

static void AddFWait(Word *pCode)
{
  if (*pCode & NO_FWAIT_FLAG)
    *pCode &= ~NO_FWAIT_FLAG;
  else
    AddPrefix(0x9b);
}

/*!------------------------------------------------------------------------
 * \fn     FPUEntry(Word *pCode)
 * \brief  check for FPU availibility and add FPU instruction entry code
 * \param  pCode machine code of instruction
 * ------------------------------------------------------------------------ */

static Boolean FPUEntry(Word *pCode)
{
  if (!FPUAvail)
  {
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
    return FALSE;
  }

  AddFWait(pCode);
  return TRUE;
}

/*!------------------------------------------------------------------------
 * \fn     check_core_mask(Byte instr_core_mask)
 * \brief  check whether current CPU supports core requirments
 * \param  instr_core_mask mask of allowed core types
 * \return True if OK
 * ------------------------------------------------------------------------ */

static Boolean check_core_mask(Byte instr_core_mask)
{
  const char *p_cpu_name;

  if (p_curr_cpu_props->core & instr_core_mask)
    return True;

  switch (instr_core_mask)
  {
    case e_core_all_v35:
      p_cpu_name = "V25/V35";
      goto write_min;
    case e_core_all_v:
      p_cpu_name = "V20/V30";
      goto write_min;
    case e_core_all_186:
      p_cpu_name = "80186/80188";
      goto write_min;
    default:
      WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
      break;
    write_min:
    {
      char str[100];

      as_snprintf(str, sizeof(str), getmessage(Num_ErrMsgMinCPUSupported), p_cpu_name);
      WrXErrorPos(ErrNum_InstructionNotSupported, str, &OpPart.Pos);
      break;
    }
  }
  return False;
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     decode_mod_reg_core(Word code, Boolean no_seg_check, int start_index)
 * \brief  decode/append mod/reg instruction
 * \param  code instruction's machine code
 * \param  no_seg_check
 * \param  start_index position of first argument
 * ------------------------------------------------------------------------ */

static void decode_mod_reg_core(Word code, Boolean no_seg_check, int start_index)
{
  switch (DecodeAdr(&ArgStr[start_index], MTypeReg16))
  {
    case TypeReg16:
    {
      Byte reg = (AdrMode << 3);
      OpSize = no_seg_check ? eSymbolSizeUnknown : eSymbolSize32Bit;
      switch (DecodeAdr(&ArgStr[start_index + 1], MTypeMem))
      {
        case TypeMem:
          PutCode(code);
          BAsmCode[CodeLen] = reg + AdrMode;
          copy_adr_vals(1);
          CodeLen += 1 + AdrCnt;
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     append_rel(const tStrComp *p_arg)
 * \brief  append relative displacement to instruction
 * \param  p_arg source argument of branch target
 * ------------------------------------------------------------------------ */

static void append_rel(const tStrComp *p_arg)
{
  tEvalResult eval_result;
  Word adr_word = EvalStrIntExpressionWithResult(p_arg, Int16, &eval_result);

  if (eval_result.OK)
  {
    ChkSpace(SegCode, eval_result.AddrSpaceMask);
    adr_word -= EProgCounter() + CodeLen + 1;
    if ((adr_word >= 0x80) && (adr_word < 0xff80) && !mSymbolQuestionable(eval_result.Flags))
    {
      WrStrErrorPos(ErrNum_JmpDistTooBig, p_arg);
      CodeLen = 0;
    }
    else
      BAsmCode[CodeLen++] = Lo(adr_word);
  }
  else
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOV(Word Index)
 * \brief  handle MOV instruction
 * ------------------------------------------------------------------------ */

static void DecodeMOV(Word Index)
{
  Byte AdrByte;
  UNUSED(Index);

  if (ChkArgCnt(2, 3))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem | MTypeRegSeg))
    {
      case TypeReg8:
      case TypeReg16:
        if (!ChkArgCnt(2, 2))
          return;
        AdrByte = AdrMode;
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeMem | MTypeRegSeg | MTypeImm))
        {
          case TypeReg8:
          case TypeReg16:
            BAsmCode[CodeLen++] = 0x8a | OpSize;
            BAsmCode[CodeLen++] = 0xc0 | (AdrByte << 3) | AdrMode;
            break;
          case TypeMem:
            if ((AdrByte == 0) && (AdrMode == 6))
            {
              BAsmCode[CodeLen] = 0xa0 | OpSize;
              copy_adr_vals(1);
              CodeLen += 1 + AdrCnt;
            }
            else
            {
              BAsmCode[CodeLen++] = 0x8a | OpSize;
              BAsmCode[CodeLen++] = AdrMode | (AdrByte << 3);
              copy_adr_vals(0);
              CodeLen += AdrCnt;
            }
            break;
          case TypeRegSeg:
            if (OpSize == eSymbolSize8Bit) WrError(ErrNum_ConfOpSizes);
            else
            {
              if (AdrMode >= 4) /* V55 DS2/DS3 */
                AdrMode += 2;
              BAsmCode[CodeLen++] = 0x8c;
              BAsmCode[CodeLen++] = 0xc0 | (AdrMode << 3) | AdrByte;
            }
            break;
          case TypeImm:
            BAsmCode[CodeLen++] = 0xb0 | (OpSize << 3) | AdrByte;
            copy_adr_vals(0);
            CodeLen += AdrCnt;
            break;
          default:
            break;
        }
        break;
      case TypeMem:
        if (!ChkArgCnt(2, 2))
          return;
         BAsmCode[CodeLen + 1] = AdrMode;
         copy_adr_vals(2);
         AdrByte = AdrCnt;
         switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeRegSeg | MTypeImm))
         {
           case TypeReg8:
           case TypeReg16:
            if ((AdrMode == 0) && (BAsmCode[CodeLen + 1] == 6))
            {
              BAsmCode[CodeLen] = 0xa2 | OpSize;
              memmove(BAsmCode + CodeLen + 1, BAsmCode + CodeLen + 2, AdrByte);
              CodeLen += 1 + AdrByte;
            }
            else
            {
              BAsmCode[CodeLen] = 0x88 | OpSize;
              BAsmCode[CodeLen + 1] |= AdrMode << 3;
              CodeLen += 2 + AdrByte;
            }
            break;
          case TypeRegSeg:
            if (AdrMode >= 4) /* V55 DS2/DS3 */
              AdrMode += 2;
            BAsmCode[CodeLen] = 0x8c;
            BAsmCode[CodeLen + 1] |= AdrMode << 3;
            CodeLen += 2 + AdrByte;
            break;
          case TypeImm:
            BAsmCode[CodeLen] = 0xc6 | OpSize;
            copy_adr_vals(2 + AdrByte);
            CodeLen += 2 + AdrByte + AdrCnt;
            break;
          default:
            break;
        }
        break;
      case TypeRegSeg:
        if (3 == ArgCnt) /* Alias for LDS, LES... */
        {
          switch (AdrMode)
          {
            case 0: /* LES reg,ea <-> MOV ES,reg,ea */
              decode_mod_reg_core(0x00c4, True, 2);
              break;
            case 3: /* LDS reg,ea <-> MOV DS,reg,ea */
              decode_mod_reg_core(0x00c5, False, 2);
              break;
            case 4: /* LDS3 reg,ea <-> MOV DS3,reg,ea */
              decode_mod_reg_core(0x0f36, False, 2);
              break;
            case 5: /* LDS2 reg,ea <-> MOV DS2,reg,ea */
              decode_mod_reg_core(0x0f3e, False, 2);
              break;
            default:
              WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
          }
        }
        else /* ordinary MOV <segreg>,r/m */
        {
          if (AdrMode >= 4) /* V55 DS2/DS3 */
            AdrMode += 2;
          BAsmCode[CodeLen + 1] = AdrMode << 3;
          switch (DecodeAdr(&ArgStr[2], MTypeReg16 | MTypeMem))
          {
            case TypeReg16:
              BAsmCode[CodeLen++] = 0x8e;
              BAsmCode[CodeLen++] |= 0xc0 + AdrMode;
              break;
            case TypeMem:
              BAsmCode[CodeLen] = 0x8e;
              BAsmCode[CodeLen + 1] |= AdrMode;
              copy_adr_vals(2);
              CodeLen += 2 + AdrCnt;
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINCDEC(Word Index)
 * \brief  handle INC/DEC instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeINCDEC(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeReg16 | MTypeReg8 | MTypeMem))
    {
      case TypeReg16:
        BAsmCode[CodeLen] = 0x40 | AdrMode | Index;
        CodeLen++;
        break;
      case TypeReg8:
        BAsmCode[CodeLen] = 0xfe;
        BAsmCode[CodeLen + 1] = 0xc0 | AdrMode | Index;
        CodeLen += 2;
        break;
      case TypeMem:
        MinOneIs0();
        if (OpSize == eSymbolSizeUnknown) WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
        else
        {
          BAsmCode[CodeLen] = 0xfe | OpSize; /* ANSI :-0 */
          BAsmCode[CodeLen + 1] = AdrMode | Index;
          copy_adr_vals(2);
          CodeLen += 2 + AdrCnt;
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINOUT(Word Index)
 * \brief  handle IN/OUT instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeINT(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    BAsmCode[CodeLen + 1] = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      if (BAsmCode[1] == 3)
        BAsmCode[CodeLen++] = 0xcc;
      else
      {
        BAsmCode[CodeLen] = 0xcd;
        CodeLen += 2;
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBrk(Word Index)
 * \brief  Decode BRKxx instructions
 * \param  Index instruction table index
 * ------------------------------------------------------------------------ */

static void DecodeBrk(Word Index)
{
  Boolean OK;
  const FixedOrder *p_order = &BrkOrders[Index];

  if (ChkArgCnt(1, 1) && check_core_mask(p_order->core_mask))
  {
    PutCode(p_order->Code);
    BAsmCode[CodeLen] = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
    if (OK)
    {
      CodeLen++;
      AddPrefixes();
    }
    else
      CodeLen = 0;
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINOUT(Word Index)
 * \brief  handle IN/OUT (intra-segment) instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeINOUT(Word Index)
{
  if (ChkArgCnt(2, 2))
  {
    tStrComp *pPortArg = Index ? &ArgStr[1] : &ArgStr[2],
             *pRegArg = Index ? &ArgStr[2] : &ArgStr[1];

    switch (DecodeAdr(pRegArg, MTypeReg8 | MTypeReg16))
    {
      case TypeReg8:
      case TypeReg16:
        if (AdrMode != 0) WrStrErrorPos(ErrNum_InvAddrMode, pRegArg);
        else if (!as_strcasecmp(pPortArg->str.p_str, "DX"))
          BAsmCode[CodeLen++] = 0xec | OpSize | Index;
        else
        {
          tEvalResult EvalResult;

          BAsmCode[CodeLen + 1] = EvalStrIntExpressionWithResult(pPortArg, UInt8, &EvalResult);
          if (EvalResult.OK)
          {
            ChkSpace(SegIO, EvalResult.AddrSpaceMask);
            BAsmCode[CodeLen] = 0xe4 | OpSize | Index;
            CodeLen += 2;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALLJMP(Word Index)
 * \brief  handle CALL/JMP (intra-segment) instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeCALLJMP(Word Index)
{
  Byte AdrByte;
  Word AdrWord;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    char *pAdr = ArgStr[1].str.p_str;

    if (!strncmp(pAdr, "SHORT ", 6))
    {
      AdrByte = 2;
      pAdr += 6;
      KillPrefBlanks(pAdr);
    }
    else if ((!strncmp(pAdr, "LONG ", 5)) || (!strncmp(pAdr, "NEAR ", 5)))
    {
      AdrByte = 1;
      pAdr +=  5;
      KillPrefBlanks(pAdr);
    }
    else
      AdrByte = 0;
    OK = True;
    if (Index == 0)
    {
      if (AdrByte == 2)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        OK = False;
      }
      else
        AdrByte = 1;
    }

    if (OK)
    {
      OpSize = eSymbolSize16Bit;
      switch (DecodeAdr(&ArgStr[1], MTypeReg16 | MTypeMem | MTypeImm))
      {
        case TypeReg16:
          BAsmCode[0] = 0xff;
          BAsmCode[1] = AdrMode | (0xd0 + (Index << 4));
          CodeLen = 2;
          break;
        case TypeMem:
          BAsmCode[0] = 0xff;
          BAsmCode[1] = AdrMode | (0x10 + (Index << 4));
          copy_adr_vals(2);
          CodeLen = 2 + AdrCnt;
          break;
        case TypeImm:
          ChkSpace(SegCode, ImmAddrSpaceMask);
          AdrWord = (((Word) AdrVals[1]) << 8) | AdrVals[0];
          if ((AdrByte == 2) || ((AdrByte == 0) && (AbleToSign(AdrWord - EProgCounter() - 2))))
          {
            AdrWord -= EProgCounter() + 2;
            if (!AbleToSign(AdrWord)) WrStrErrorPos(ErrNum_DistTooBig, &ArgStr[1]);
            else
            {
              BAsmCode[0] = 0xeb;
              BAsmCode[1] = Lo(AdrWord);
              CodeLen = 2;
            }
          }
          else
          {
            AdrWord -= EProgCounter() + 3;
            BAsmCode[0] = 0xe8 | Index;
            BAsmCode[1] = Lo(AdrWord);
            BAsmCode[2] = Hi(AdrWord);
            CodeLen = 3;
            AdrWord++;
          }
          break;
        default:
          break;
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodePUSHPOP(Word Index)
 * \brief  handle PUSH/POP instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodePUSHPOP(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    OpSize = eSymbolSize16Bit;
    switch (DecodeAdr(&ArgStr[1], MTypeReg16 | MTypeMem | MTypeRegSeg | ((Index == 1) ? 0 : MTypeImm)))
    {
      case TypeReg16:
        BAsmCode[CodeLen] = 0x50 |  AdrMode | (Index << 3);
        CodeLen++;
        break;
      case TypeRegSeg:
        if (AdrMode >= 4) /* V55 DS2/DS3 */
        {
          BAsmCode[CodeLen++] = 0x0f;
          BAsmCode[CodeLen++] = 0x76 | ((AdrMode & 1) << 3) | Index;
        }
        else
        {
          BAsmCode[CodeLen] = 0x06 | (AdrMode << 3) | Index;
          CodeLen++;
        }
        break;
      case TypeMem:
        BAsmCode[CodeLen] = 0x8f;
        BAsmCode[CodeLen + 1] = AdrMode;
        if (Index == 0)
        {
          BAsmCode[CodeLen] += 0x70;
          BAsmCode[CodeLen + 1] += 0x30;
        }
        copy_adr_vals(2);
        CodeLen += 2 + AdrCnt;
        break;
      case TypeImm:
        if (check_core_mask(e_core_all_186))
        {
          BAsmCode[CodeLen] = 0x68;
          BAsmCode[CodeLen + 1] = AdrVals[0];
          if (Sgn(AdrVals[0]) == AdrVals[1])
          {
            BAsmCode[CodeLen] += 2;
            CodeLen += 2;
          }
          else
          {
            BAsmCode[CodeLen + 2] = AdrVals[1];
            CodeLen += 3;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeNOTNEG(Word Index)
 * \brief  handle NOT/NEG instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeNOTNEG(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem);
    MinOneIs0();
    BAsmCode[CodeLen] = 0xf6 | OpSize;
    BAsmCode[CodeLen + 1] = 0x10 | Index;
    switch (AdrType)
    {
      case TypeReg8:
      case TypeReg16:
        BAsmCode[CodeLen + 1] |= 0xc0 | AdrMode;
        CodeLen += 2;
        break;
      case TypeMem:
        if (OpSize == eSymbolSizeUnknown) WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
        else
        {
          BAsmCode[CodeLen + 1] |= AdrMode;
          copy_adr_vals(2);
          CodeLen += 2 + AdrCnt;
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRET(Word Index)
 * \brief  handle RET instruction
 * ------------------------------------------------------------------------ */

static void DecodeRET(Word Index)
{
  Word AdrWord;
  Boolean OK;

  if (!ChkArgCnt(0, 1));
  else if (ArgCnt == 0)
    BAsmCode[CodeLen++] = 0xc3 | Index;
  else
  {
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      BAsmCode[CodeLen++] = 0xc2 | Index;
      BAsmCode[CodeLen++] = Lo(AdrWord);
      BAsmCode[CodeLen++] = Hi(AdrWord);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTEST(Word Index)
 * \brief  handle TEST instruction
 * ------------------------------------------------------------------------ */

static void DecodeTEST(Word Index)
{
  Byte AdrByte;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem))
    {
      case TypeReg8:
      case TypeReg16:
        BAsmCode[CodeLen + 1] = (AdrMode << 3);
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeMem | MTypeImm))
        {
          case TypeReg8:
          case TypeReg16:
            BAsmCode[CodeLen + 1] += 0xc0 | AdrMode;
            BAsmCode[CodeLen] = 0x84 | OpSize;
            CodeLen += 2;
            break;
          case TypeMem:
            BAsmCode[CodeLen + 1] |= AdrMode;
            BAsmCode[CodeLen] = 0x84 | OpSize;
            copy_adr_vals(2);
            CodeLen += 2 + AdrCnt;
            break;
          case TypeImm:
            if (((BAsmCode[CodeLen+1] >> 3) & 7) == 0)
            {
              BAsmCode[CodeLen] = 0xa8 | OpSize;
              copy_adr_vals(1);
              CodeLen += 1 + AdrCnt;
            }
            else
            {
              BAsmCode[CodeLen] = OpSize | 0xf6;
              BAsmCode[CodeLen + 1] = (BAsmCode[CodeLen + 1] >> 3) | 0xc0;
              copy_adr_vals(2);
              CodeLen += 2 + AdrCnt;
            }
            break;
          default:
            break;;
        }
        break;
      case TypeMem:
        BAsmCode[CodeLen + 1] = AdrMode;
        AdrByte = AdrCnt;
        copy_adr_vals(2);
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeImm))
        {
          case TypeReg8:
          case TypeReg16:
            BAsmCode[CodeLen] = 0x84 | OpSize;
            BAsmCode[CodeLen + 1] += (AdrMode << 3);
            CodeLen += 2 + AdrByte;
            break;
          case TypeImm:
            BAsmCode[CodeLen] = OpSize | 0xf6;
            copy_adr_vals(2 + AdrByte);
            CodeLen += 2 + AdrCnt + AdrByte;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeXCHG(Word Index)
 * \brief  handle XCHG instruction
 * ------------------------------------------------------------------------ */

static void DecodeXCHG(Word Index)
{
  Byte AdrByte;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem))
    {
      case TypeReg8:
      case TypeReg16:
        AdrByte = AdrMode;
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeMem))
        {
          case TypeReg8:
          case TypeReg16:
            if ((OpSize == eSymbolSize16Bit) && ((AdrMode == 0) || (AdrByte == 0)))
            {
              BAsmCode[CodeLen] = 0x90 | AdrMode | AdrByte;
              CodeLen++;
            }
            else
            {
              BAsmCode[CodeLen] = 0x86 | OpSize;
              BAsmCode[CodeLen+1] = AdrMode | 0xc0 | (AdrByte << 3);
              CodeLen += 2;
            }
            break;
          case TypeMem:
            BAsmCode[CodeLen] = 0x86 | OpSize;
            BAsmCode[CodeLen+1] = AdrMode | (AdrByte << 3);
            copy_adr_vals(2);
            CodeLen += AdrCnt + 2;
            break;
          default:
            break;
        }
        break;
      case TypeMem:
        BAsmCode[CodeLen + 1] = AdrMode;
        copy_adr_vals(2);
        AdrByte = AdrCnt;
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16))
        {
          case TypeReg8:
          case TypeReg16:
            BAsmCode[CodeLen] = 0x86 | OpSize;
            BAsmCode[CodeLen+1] |= (AdrMode << 3);
            CodeLen += AdrByte + 2;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALLJMPF(Word Index)
 * \brief  handle CALLF/JMPF (inter-segment) instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeCALLJMPF(Word Index)
{
  char *p;
  Word AdrWord;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    p = QuotPos(ArgStr[1].str.p_str, ':');
    if (!p)
    {
      switch (DecodeAdr(&ArgStr[1], MTypeMem))
      {
        case TypeMem:
          BAsmCode[CodeLen] = 0xff;
          BAsmCode[CodeLen + 1] = AdrMode | Hi(Index);
          copy_adr_vals(2);
          CodeLen += 2 + AdrCnt;
          break;
        default:
          break;
      }
    }
    else
    {
      tStrComp SegArg, OffsArg;

      StrCompSplitRef(&SegArg, &OffsArg, &ArgStr[1], p);
      AdrWord = EvalStrIntExpression(&SegArg, UInt16, &OK);
      if (OK)
      {
        BAsmCode[CodeLen + 3] = Lo(AdrWord);
        BAsmCode[CodeLen + 4] = Hi(AdrWord);
        AdrWord = EvalStrIntExpression(&OffsArg, UInt16, &OK);
        if (OK)
        {
          BAsmCode[CodeLen + 1] = Lo(AdrWord);
          BAsmCode[CodeLen + 2] = Hi(AdrWord);
          BAsmCode[CodeLen] = Lo(Index);
          CodeLen += 5;
        }
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeENTER(Word Index)
 * \brief  handle ENTER instruction
 * ------------------------------------------------------------------------ */

static void DecodeENTER(Word Index)
{
  Word AdrWord;
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (check_core_mask(e_core_all_186))
  {
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      BAsmCode[CodeLen + 1] = Lo(AdrWord);
      BAsmCode[CodeLen + 2] = Hi(AdrWord);
      BAsmCode[CodeLen + 3] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
      if (OK)
      {
        BAsmCode[CodeLen] = 0xc8;
        CodeLen += 4;
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Index)
 * \brief  handle instructions without argument
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Index)
{
  const FixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && check_core_mask(pOrder->core_mask))
    PutCode(pOrder->Code);
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeALU2(Word Index)
 * \brief  handle ALU instructions with two arguments
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeALU2(Word Index)
{
  Byte AdrByte;

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem))
    {
      case TypeReg8:
      case TypeReg16:
        BAsmCode[CodeLen + 1] = AdrMode << 3;
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeMem | MTypeImm))
        {
          case TypeReg8:
          case TypeReg16:
            BAsmCode[CodeLen + 1] |= 0xc0 | AdrMode;
            BAsmCode[CodeLen] = (Index << 3) | 2 | OpSize;
            CodeLen += 2;
            break;
          case TypeMem:
            BAsmCode[CodeLen + 1] |= AdrMode;
            BAsmCode[CodeLen] = (Index << 3) | 2 | OpSize;
            copy_adr_vals(2);
            CodeLen += 2 + AdrCnt;
            break;
          case TypeImm:
            if (((BAsmCode[CodeLen+1] >> 3) & 7) == 0)
            {
              BAsmCode[CodeLen] = (Index << 3) | 4 | OpSize;
              copy_adr_vals(1);
              CodeLen += 1 + AdrCnt;
            }
            else
            {
              BAsmCode[CodeLen] = OpSize | 0x80;
              if ((OpSize == eSymbolSize16Bit) && (Sgn(AdrVals[0]) == AdrVals[1]))
              {
                AdrCnt = 1;
                BAsmCode[CodeLen] |= 2;
              }
              BAsmCode[CodeLen + 1] = (BAsmCode[CodeLen + 1] >> 3) + 0xc0 + (Index << 3);
              copy_adr_vals(2);
              CodeLen += 2 + AdrCnt;
            }
            break;
          default:
            break;
        }
        break;
      case TypeMem:
        BAsmCode[CodeLen + 1] = AdrMode;
        AdrByte = AdrCnt;
        copy_adr_vals(2);
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeReg16 | MTypeImm))
        {
          case TypeReg8:
          case TypeReg16:
            BAsmCode[CodeLen] = (Index << 3) | OpSize;
            BAsmCode[CodeLen + 1] |= (AdrMode << 3);
            CodeLen += 2 + AdrByte;
            break;
          case TypeImm:
            BAsmCode[CodeLen] = OpSize | 0x80;
            if ((OpSize == eSymbolSize16Bit) && (Sgn(AdrVals[0]) == AdrVals[1]))
            {
              AdrCnt = 1;
              BAsmCode[CodeLen] += 2;
            }
            BAsmCode[CodeLen + 1] += (Index << 3);
            copy_adr_vals(2 + AdrByte);
            CodeLen += 2 + AdrCnt + AdrByte;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRel(Word Index)
 * \brief  handle relative branches
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeRel(Word Index)
{
  const FixedOrder *pOrder = RelOrders + Index;

  if (ChkArgCnt(1, 1)
   && check_core_mask(pOrder->core_mask))
  {
    PutCode(pOrder->Code);
    append_rel(&ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeASSUME(void)
 * \brief  handle ASSUME instruction
 * ------------------------------------------------------------------------ */

static void DecodeASSUME(void)
{
  Boolean OK;
  int z, z3;
  char *p, empty_str[1] = "";

  if (ChkArgCnt(1, ArgCntMax))
  {
    z = 1 ; OK = True;
    while ((z <= ArgCnt) && (OK))
    {
      Byte seg_reg;
      tStrComp seg_arg, val_arg;

      OK = False;
      p = QuotPos(ArgStr[z].str.p_str, ':');
      if (p)
        StrCompSplitRef(&seg_arg, &val_arg, &ArgStr[z], p);
      else
      {
        StrCompRefRight(&seg_arg, &ArgStr[z], 0);
        StrCompMkTemp(&val_arg, empty_str, sizeof(empty_str));
      }
      if (!decode_seg_reg(seg_arg.str.p_str, &seg_reg)) WrStrErrorPos(ErrNum_UnknownSegReg, &seg_arg);
      else
      {
        z3 = addrspace_lookup(val_arg.str.p_str);
        if (z3 >= SegCount) WrStrErrorPos(ErrNum_UnknownSegment, &val_arg);
        else if ((z3 != SegCode) && (z3 != SegData) && (z3 != SegXData) && (z3 != SegNone)) WrStrErrorPos(ErrNum_InvSegment, &val_arg);
        else
        {
          SegAssumes[seg_reg] = z3;
          OK = True;
        }
      }
      z++;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePORT(Word Code)
 * \brief  handle PORT instruction
 * ------------------------------------------------------------------------ */

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, 0xffff);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPUFixed(Word Code)
 * \brief  handle FPU instructions with no arguments
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFPUFixed(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(0, 0))
  {
    PutCode(Code);
    AddPrefixes();
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPUSt(Word Code)
 * \brief  handle FPU instructions with one register argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFPUSt(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    if (DecodeAdr(&ArgStr[1], MTypeFReg) == TypeFReg)
    {
      PutCode(Code);
      BAsmCode[CodeLen-1] |= AdrMode;
      AddPrefixes();
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFLD(Word Code)
 * \brief  handle FLD instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFLD(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeFReg | MTypeMem))
    {
      case TypeFReg:
        BAsmCode[CodeLen++] = 0xd9;
        BAsmCode[CodeLen++] = 0xc0 | AdrMode;
        CodeLen += 2;
        break;
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize32Bit;
        switch (OpSize)
        {
          case eSymbolSize32Bit:
            BAsmCode[CodeLen++] = 0xd9;
            BAsmCode[CodeLen++] = AdrMode;
            break;
          case eSymbolSize64Bit:
            BAsmCode[CodeLen++] = 0xdd;
            BAsmCode[CodeLen++] = AdrMode;
            break;
          case eSymbolSize80Bit:
            BAsmCode[CodeLen++] = 0xdb;
            BAsmCode[CodeLen++] = AdrMode | 0x28;
            break;
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            CodeLen = 0;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen)
          append_adr_vals();
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFILD(Word Code)
 * \brief  handle FILD instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFILD(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize16Bit;
        switch (OpSize)
        {
          case eSymbolSize16Bit:
            BAsmCode[CodeLen++] = 0xdf;
            BAsmCode[CodeLen++] = AdrMode;
            break;
          case eSymbolSize32Bit:
            BAsmCode[CodeLen++] = 0xdb;
            BAsmCode[CodeLen++] = AdrMode;
            break;
          case eSymbolSize64Bit:
            BAsmCode[CodeLen++] = 0xdf;
            BAsmCode[CodeLen++] = AdrMode | 0x28;
            break;
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            CodeLen = 0;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen)
          append_adr_vals();
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFBLD(Word Code)
 * \brief  handle FBLD instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFBLD(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize80Bit;
        switch (OpSize)
        {
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            CodeLen = 0;
            break;
          case eSymbolSize80Bit:
            BAsmCode[CodeLen++] = 0xdf;
            BAsmCode[CodeLen++] = AdrMode + 0x20;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen > 0)
          append_adr_vals();
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFST_FSTP(Word Code)
 * \brief  handle FST(P) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFST_FSTP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeFReg | MTypeMem))
    {
      case TypeFReg:
        BAsmCode[CodeLen++] = 0xdd;
        BAsmCode[CodeLen++] = Code | AdrMode;
        break;
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize32Bit;
        switch (OpSize)
        {
          case eSymbolSize32Bit:
            BAsmCode[CodeLen++] = 0xd9;
            BAsmCode[CodeLen++] = 0x00;
            break;
          case eSymbolSize64Bit:
            BAsmCode[CodeLen++] = 0xdd;
            BAsmCode[CodeLen++] = 0x00;
            break;
          case eSymbolSize80Bit:
            if (Code == 0xd0)
              goto invalid;
            BAsmCode[CodeLen++] = 0xdb;
            BAsmCode[CodeLen++] = 0x20;
            break;
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            CodeLen = 0;
            break;
          invalid:
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen > 0)
        {
          BAsmCode[CodeLen - 1] |= AdrMode | 0x10 | (Code & 8);
          append_adr_vals();
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFIST_FISTP(Word Code)
 * \brief  handle FIST(P) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFIST_FISTP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize16Bit;
        switch (OpSize)
        {
          case eSymbolSize16Bit:
            BAsmCode[CodeLen++] = 0xdf;
            BAsmCode[CodeLen++] = 0x00;
            break;
          case eSymbolSize32Bit:
            BAsmCode[CodeLen++] = 0xdb;
            BAsmCode[CodeLen++] = 0x00;
            break;
          case eSymbolSize64Bit:
            if (Code == 0x10)
              goto invalid;
            BAsmCode[CodeLen++] = 0xdf;
            BAsmCode[CodeLen++] = 0x20;
            break;
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            break;
          invalid:
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen > 0)
        {
          BAsmCode[CodeLen - 1] |= AdrMode | Code;
          append_adr_vals();
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFBSTP(Word Code)
 * \brief  handle FBSTP instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFBSTP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize16Bit;
        switch (OpSize)
        {
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            break;
          case eSymbolSize80Bit:
            BAsmCode[CodeLen] = 0xdf;
            BAsmCode[CodeLen + 1] = AdrMode | 0x30;
            copy_adr_vals(2);
            CodeLen += 2 + AdrCnt;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFCOM_FCOMP(Word Code)
 * \brief  handle FCOM(P) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFCOM_FCOMP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeFReg | MTypeMem))
    {
      case TypeFReg:
        BAsmCode[CodeLen] = 0xd8;
        BAsmCode[CodeLen+1] = Code | AdrMode;
        CodeLen += 2;
        break;
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize16Bit;
        switch (OpSize)
        {
          case eSymbolSize32Bit:
            BAsmCode[CodeLen++] = 0xd8;
            break;
          case eSymbolSize64Bit:
            BAsmCode[CodeLen++] = 0xdc;
            break;
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            CodeLen = 0;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen > 0)
        {
          BAsmCode[CodeLen++] = AdrMode | 0x10 | (Code & 8);
          append_adr_vals();
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFICOM_FICOMP(Word Code)
 * \brief  handle FICOM(P) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFICOM_FICOMP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
          OpSize = eSymbolSize16Bit;
        switch (OpSize)
        {
          case eSymbolSize16Bit:
            BAsmCode[CodeLen++] = 0xde;
            break;
          case eSymbolSize32Bit:
            BAsmCode[CodeLen++] = 0xda;
            break;
          case eSymbolSizeUnknown:
            WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
            CodeLen = 0;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
            CodeLen = 0;
            break;
        }
        if (CodeLen > 0)
        {
          BAsmCode[CodeLen++] = AdrMode | Code;
          append_adr_vals();
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFADD_FMUL(Word Code)
 * \brief  handle FADD/FMUL instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFADD_FMUL(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ArgCnt == 0)
  {
    BAsmCode[CodeLen] = 0xde;
    BAsmCode[CodeLen + 1] = 0xc1 + Code;
    CodeLen += 2;
  }
  else if (ChkArgCnt(0, 2))
  {
    const tStrComp *pArg1 = &ArgStr[1],
                   *pArg2 = &ArgStr[2];

    if (ArgCnt == 1)
    {
      pArg2 = &ArgStr[1];
      pArg1 = &ArgST;
    }

    switch (DecodeAdr(pArg1, MTypeFReg))
    {
      case TypeFReg:
        OpSize = eSymbolSizeUnknown;
        if (AdrMode != 0)   /* ST(i) ist Ziel */
        {
          BAsmCode[CodeLen + 1] = AdrMode;
          switch (DecodeAdr(pArg2, MTypeFReg))
          {
            case TypeFReg:
              BAsmCode[CodeLen] = 0xdc;
              BAsmCode[CodeLen + 1] += 0xc0 + Code;
              CodeLen += 2;
              break;
            default:
              break;
          }
        }
        else                      /* ST ist Ziel */
        {
          switch (DecodeAdr(pArg2, MTypeFReg | MTypeMem))
          {
            case TypeFReg:
              BAsmCode[CodeLen] = 0xd8;
              BAsmCode[CodeLen + 1] = 0xc0 + AdrMode + Code;
              CodeLen += 2;
              break;
            case TypeMem:
              if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
                OpSize = eSymbolSize32Bit;
              switch (OpSize)
              {
                case eSymbolSize32Bit:
                  BAsmCode[CodeLen++] = 0xd8;
                  break;
                case eSymbolSize64Bit:
                  BAsmCode[CodeLen++] = 0xdc;
                  break;
                case eSymbolSizeUnknown:
                  WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
                  CodeLen = 0;
                  break;
                default:
                  WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
                  CodeLen = 0;
                  break;
              }
              if (CodeLen > 0)
              {
                BAsmCode[CodeLen++] = AdrMode + Code;
                append_adr_vals();
              }
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFIADD_FIMUL(Word Code)
 * \brief  decode FIADD/FIMUL instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFIADD_FIMUL(Word Code)
{
  const tStrComp *pArg1 = &ArgStr[1],
                 *pArg2 = &ArgStr[2];

  if (!FPUEntry(&Code))
    return;

  if (ArgCnt == 1)
  {
    pArg2 = &ArgStr[1];
    pArg1 = &ArgST;
  }
  else if (ChkArgCnt(1, 2))
  {
    switch (DecodeAdr(pArg1, MTypeFReg))
    {
      case TypeFReg:
        if (AdrMode != 0) WrStrErrorPos(ErrNum_InvAddrMode, pArg1);
        else
        {
          OpSize = eSymbolSizeUnknown;
          switch (DecodeAdr(pArg2, MTypeFReg))
          {
            case TypeFReg:
              if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
                OpSize = eSymbolSize16Bit;
              switch (OpSize)
              {
                case eSymbolSize16Bit:
                  BAsmCode[CodeLen++] = 0xde;
                  break;
                case eSymbolSize32Bit:
                  BAsmCode[CodeLen++] = 0xda;
                  break;
                case eSymbolSizeUnknown:
                  WrStrErrorPos(ErrNum_UndefOpSizes, pArg1);
                  CodeLen = 0;
                  break;
                default:
                  WrStrErrorPos(ErrNum_InvOpSize, pArg1);
                  CodeLen = 0;
                  break;
              }
              if (CodeLen > 0)
              {
                BAsmCode[CodeLen++] = AdrMode + Code;
                append_adr_vals();
              }
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFADDP_FMULP(Word Code)
 * \brief  handle FADDP/FMULP instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFADDP_FMULP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[2], MTypeFReg))
    {
      case TypeFReg:
        if (AdrMode != 0) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          switch (DecodeAdr(&ArgStr[1], MTypeFReg))
          {
            case TypeFReg:
              BAsmCode[CodeLen] = 0xde;
              BAsmCode[CodeLen + 1] = 0xc0 + AdrMode + Code;
              CodeLen += 2;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFSUB_FSUBR_FDIV_FDIVR(Word Code)
 * \brief  handle FSUB(R)/FDIV(R) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFSUB_FSUBR_FDIV_FDIVR(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ArgCnt == 0)
  {
    BAsmCode[CodeLen] = 0xde;
    BAsmCode[CodeLen + 1] = 0xe1 + (Code ^ 8);
    CodeLen += 2;
  }
  else if (ChkArgCnt(0, 2))
  {
    const tStrComp *pArg1 = &ArgStr[1],
                   *pArg2 = &ArgStr[2];

    if (ArgCnt == 1)
    {
      pArg1 = &ArgST;
      pArg2 = &ArgStr[1];
    }

    switch (DecodeAdr(pArg1, MTypeFReg))
    {
      case TypeFReg:
        OpSize = eSymbolSizeUnknown;
        if (AdrMode != 0)   /* ST(i) ist Ziel */
        {
          BAsmCode[CodeLen + 1] = AdrMode;
          switch (DecodeAdr(pArg2, MTypeFReg))
          {
            case TypeFReg:
              if (AdrMode != 0) WrStrErrorPos(ErrNum_InvAddrMode, pArg2);
              else
              {
                BAsmCode[CodeLen] = 0xdc;
                BAsmCode[CodeLen + 1] += 0xe0 + (Code ^ 8);
                CodeLen += 2;
              }
              break;
            default:
              break;
          }
        }
        else  /* ST ist Ziel */
        {
          switch (DecodeAdr(pArg2, MTypeFReg | MTypeMem))
          {
            case TypeFReg:
              BAsmCode[CodeLen] = 0xd8;
              BAsmCode[CodeLen + 1] = 0xe0 + AdrMode + Code;
              CodeLen += 2;
              break;
            case TypeMem:
              if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
                OpSize = eSymbolSize32Bit;
              switch (OpSize)
              {
                case eSymbolSize32Bit:
                  BAsmCode[CodeLen++] = 0xd8;
                  break;
                case eSymbolSize64Bit:
                  BAsmCode[CodeLen++] = 0xdc;
                  break;
                case eSymbolSizeUnknown:
                  WrStrErrorPos(ErrNum_UndefOpSizes, pArg2);
                  CodeLen = 0;
                  break;
                default:
                  WrStrErrorPos(ErrNum_InvOpSize, pArg2);
                  CodeLen = 0;
                  break;
              }
              if (CodeLen > 0)
              {
                BAsmCode[CodeLen++] = AdrMode + 0x20 + Code;
                append_adr_vals();
              }
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFISUB_FISUBR_FIDIV_FIDIVR(Word Code)
 * \brief  handle FISUB(R)/FIDIV(R) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFISUB_FISUBR_FIDIV_FIDIVR(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 2))
  {
    const tStrComp *pArg1 = &ArgStr[1],
                   *pArg2 = &ArgStr[2];

    if (ArgCnt == 1)
    {
      pArg1 = &ArgST;
      pArg2 = &ArgStr[1];
    }

    switch (DecodeAdr(pArg1, MTypeFReg))
    {
      case TypeFReg:
        if (AdrMode != 0) WrStrErrorPos(ErrNum_InvAddrMode, pArg1);
        else
        {
          OpSize = eSymbolSizeUnknown;
          switch (DecodeAdr(pArg2, MTypeMem))
          {
            case TypeMem:
              if ((OpSize == eSymbolSizeUnknown) && UnknownFlag)
                OpSize = eSymbolSize16Bit;
              switch (OpSize)
              {
                case eSymbolSize16Bit:
                  BAsmCode[CodeLen++] = 0xde;
                  break;
                case eSymbolSize32Bit:
                  BAsmCode[CodeLen++] = 0xda;
                  break;
                case eSymbolSizeUnknown:
                  WrStrErrorPos(ErrNum_UndefOpSizes, pArg2);
                  CodeLen = 0;
                  break;
                default:
                  WrStrErrorPos(ErrNum_InvOpSize, pArg2);
                  CodeLen = 0;
                  break;
              }
              if (CodeLen > 0)
              {
                BAsmCode[CodeLen++] = AdrMode + 0x20 + Code;
                append_adr_vals();
              }
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    } 
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFSUBP_FSUBRP_FDIVP_FDIVRP(Word Code)
 * \brief  handle FSUB(R)P/FDIV(R)P instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFSUBP_FSUBRP_FDIVP_FDIVRP(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(2, 2))
  {
    switch (DecodeAdr(&ArgStr[2], MTypeFReg))
    {
      case TypeFReg:
        if (AdrMode != 0) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          switch (DecodeAdr(&ArgStr[1], MTypeFReg))
          {
            case TypeFReg:
              BAsmCode[CodeLen] = 0xde;
              BAsmCode[CodeLen+1] = 0xe0 + AdrMode + (Code ^ 8);
              CodeLen += 2;
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPU16(Word Code)
 * \brief  handle FPU instructions with one 16 bit memory argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFPU16(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    OpSize = eSymbolSize16Bit;
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        PutCode(Code);
        BAsmCode[CodeLen - 1] += AdrMode;
        copy_adr_vals(0);
        CodeLen += AdrCnt;
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFSAVE_FRSTOR(Word Code)
 * \brief  handle FSAVE/FRSTOR instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFSAVE_FRSTOR(Word Code)
{
  if (!FPUEntry(&Code))
    return;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeMem))
    {
      case TypeMem:
        BAsmCode[CodeLen] = 0xdd;
        BAsmCode[CodeLen + 1] = AdrMode + Code;
        copy_adr_vals(2);
        CodeLen += 2 + AdrCnt;
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRept(Word Index)
 * \brief  handle repetition instructions
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeRept(Word Index)
{
  const FixedOrder *pOrder = ReptOrders + Index;

  if (ChkArgCnt(1, 1)
   && check_core_mask(pOrder->core_mask))
  {
    unsigned z2;

    for (z2 = 0; z2 < StringOrderCnt; z2++)
      if (!as_strcasecmp(StringOrders[z2].Name, ArgStr[1].str.p_str))
        break;
    if (z2 >= StringOrderCnt) WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
    else if (check_core_mask(StringOrders[z2].core_mask))
    {
      PutCode(pOrder->Code);
      PutCode(StringOrders[z2].Code);
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMul(Word Index)
 * \brief  handle multiplication instructions
 * \param  Index machine code
 * ------------------------------------------------------------------------ */

static void DecodeMul(Word Index)
{
  Boolean OK;
  Word AdrWord;

  if (!ChkArgCnt(1, (1 == Index) ? 3 : 1)) /* IMUL only 2/3 ops */
    return;

  switch (ArgCnt)
  {
    case 1:
      switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem))
      {
        case TypeReg8:
        case TypeReg16:
          BAsmCode[CodeLen] = 0xf6 + OpSize;
          BAsmCode[CodeLen + 1] = 0xe0 + (Index << 3) + AdrMode;
          CodeLen += 2;
          break;
        case TypeMem:
          MinOneIs0();
          if (OpSize == eSymbolSizeUnknown) WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
          else
          {
            BAsmCode[CodeLen] = 0xf6 + OpSize;
            BAsmCode[CodeLen+1] = 0x20 + (Index << 3) + AdrMode;
            copy_adr_vals(2);
            CodeLen += 2 + AdrCnt;
          }
          break;
        default:
          break;
      }
      break;
    case 2:
    case 3:
      if (check_core_mask(e_core_all_186))
      {
        tStrComp *pArg1 = &ArgStr[1],
                 *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];

        BAsmCode[CodeLen] = 0x69;
        switch (DecodeAdr(pArg1, MTypeReg16))
        {
          case TypeReg16:
            BAsmCode[CodeLen + 1] = (AdrMode << 3);
            switch (DecodeAdr(pArg2, MTypeReg16 | MTypeMem))
            {
              case TypeReg16:
                AdrMode += 0xc0;
                /* FALL-THRU */
              case TypeMem:
                BAsmCode[CodeLen + 1] += AdrMode;
                copy_adr_vals(2);
                AdrWord = EvalStrIntExpression(pArg3, Int16, &OK);
                if (OK)
                {
                  BAsmCode[CodeLen + 2 + AdrCnt] = Lo(AdrWord);
                  BAsmCode[CodeLen + 3 + AdrCnt] = Hi(AdrWord);
                  CodeLen += 2 + AdrCnt + 2;
                  if ((AdrWord >= 0xff80) || (AdrWord < 0x80))
                  {
                    CodeLen--;
                    BAsmCode[CodeLen-AdrCnt - 2 - 1] += 2;
                  }
                }
                break;
              default:
                break;
            }
            break;
          default:
            break;
        }
      }
      break;
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeModReg(Word Index)
 * \brief  handle instructions with one mod/reg argument
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeModReg(Word Index)
{
  const ModRegOrder *pOrder = ModRegOrders + Index;

  NoSegCheck = pOrder->no_seg_check;
  if (ChkArgCnt(2, 2)
   && check_core_mask(pOrder->core_mask))
    decode_mod_reg_core(pOrder->Code, pOrder->no_seg_check, 1);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeShift(Word Index)
 * \brief  handle shift instructions
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeShift(Word Index)
{
  const FixedOrder *pOrder = ShiftOrders + Index;

  if (ChkArgCnt(2, 2)
   && check_core_mask(pOrder->core_mask))
  {
    DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem);
    MinOneIs0();
    if (OpSize == eSymbolSizeUnknown) WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
    else switch (AdrType)
    {
      case TypeReg8:
      case TypeReg16:
      case TypeMem:
        BAsmCode[CodeLen] = OpSize;
        BAsmCode[CodeLen + 1] = AdrMode + (pOrder->Code << 3);
        if (AdrType != TypeMem)
          BAsmCode[CodeLen + 1] += 0xc0;
        copy_adr_vals(2);
        if (!as_strcasecmp(ArgStr[2].str.p_str, "CL"))
        {
          BAsmCode[CodeLen] += 0xd2;
          CodeLen += 2 + AdrCnt;
        }
        else
        {
          Boolean OK;

          BAsmCode[CodeLen + 2 + AdrCnt] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
          if (OK)
          {
            if (BAsmCode[CodeLen + 2 + AdrCnt] == 1)
            {
              BAsmCode[CodeLen] += 0xd0;
              CodeLen += 2 + AdrCnt;
            }
            else if (check_core_mask(e_core_all_186))
            {
              BAsmCode[CodeLen] += 0xc0;
              CodeLen += 3 + AdrCnt;
            }
          }
        }
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeROL4_ROR4(Word Code)
 * \brief  handle V20 ROL4/ROR4 instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeROL4_ROR4(Word Code)
{
  if (ChkArgCnt(1, 1)
   && check_core_mask(e_core_all_v))
  {
    BAsmCode[CodeLen    ] = 0x0f;
    BAsmCode[CodeLen + 1] = Code;
    switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeMem))
    {
      case TypeReg8:
        /* TODO: convert mode */
        BAsmCode[CodeLen + 2] = 0xc0 + AdrMode;
        CodeLen += 3;
        break;
      case TypeMem:
        BAsmCode[CodeLen + 2] = AdrMode;
        copy_adr_vals(3);
        CodeLen += 3 + AdrCnt;
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBit1(Word Index)
 * \brief  Handle V30-specific bit instructions (NOT1, CLR1, SET1, TEST1)
 * \param  Index machine code index
 * ------------------------------------------------------------------------ */

static void DecodeBit1(Word Index)
{
  int min_arg_cnt = (Index == 0) ? 2 : 1;
  if (!check_core_mask(e_core_all_v))
    return;

  switch (ArgCnt)
  {
    case 2:
      switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem))
      {
        case TypeReg8:
        case TypeReg16:
          AdrMode += 0xc0;
          /* FALL-THRU */
        case TypeMem:
          MinOneIs0();
          if (OpSize == eSymbolSizeUnknown) WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
          else
          {
            BAsmCode[CodeLen    ] = 0x0f;
            BAsmCode[CodeLen + 1] = 0x10 + (Index << 1) + OpSize;
            BAsmCode[CodeLen + 2] = AdrMode;
            copy_adr_vals(3);
            if (!as_strcasecmp(ArgStr[2].str.p_str, "CL"))
              CodeLen += 3 + AdrCnt;
            else
            {
              Boolean OK;

              BAsmCode[CodeLen + 1] += 8;
              BAsmCode[CodeLen + 3 + AdrCnt] = EvalStrIntExpression(&ArgStr[2], Int4, &OK);
              if (OK)
                CodeLen += 4 + AdrCnt;
            }
          }
          break;
        default:
          break;
      }
      break;

    case 1:
      if (min_arg_cnt > 1)
        goto bad_arg_cnt;
      if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      BAsmCode[CodeLen++] = Index == 3 ? 0xf5 : (Index + 0xf7);
      break;

    bad_arg_cnt:
    default:
      (void)ChkArgCnt(min_arg_cnt, 2);
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBSCH(Word Index)
 * \brief  Handle V55-specific BSCH instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeBSCH(Word code)
{
  if (check_core_mask(e_core_all_v55) && ChkArgCnt(1, 1))
    switch (DecodeAdr(&ArgStr[1], MTypeReg8 | MTypeReg16 | MTypeMem))
    {
      case TypeReg8:
      case TypeReg16:
        AdrMode += 0xc0;
        /* FALL-THRU */
      case TypeMem:
        MinOneIs0();
        if (OpSize == eSymbolSizeUnknown) WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[1]);
        PutCode(code + OpSize);
        BAsmCode[CodeLen++] = AdrMode;
        copy_adr_vals(3);
        break;
      default:
        break;
    }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRSTWDT(Word code)
 * \brief  handle RSTWDT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeRSTWDT(Word code)
{
  if (check_core_mask(e_core_all_v55)
   && ChkArgCnt(2, 2))
  {
    Boolean ok;

    BAsmCode[2] = EvalStrIntExpression(&ArgStr[1], Int8, &ok);
    if (ok)
      BAsmCode[3] = EvalStrIntExpression(&ArgStr[2], Int8, &ok);
    if (ok)
    {
      PutCode(code);
      CodeLen += 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBTCLRL(Word code)
 * \brief  handle BTCLRL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeBTCLRL(Word code)
{
  if (check_core_mask(e_core_all_v55)
   && ChkArgCnt(3, 3))
  {
    Boolean ok;

    PutCode(code);
    BAsmCode[CodeLen++] = EvalStrIntExpression(&ArgStr[1], Int8, &ok);
    if (ok)
      BAsmCode[CodeLen++] = EvalStrIntExpression(&ArgStr[2], Int8, &ok);
    if (ok)
      append_rel(&ArgStr[3]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINS_EXT(Word Code)
 * \brief  Handle V30-specific bit field instructions (INS, EXT)
 * ------------------------------------------------------------------------ */

static void DecodeINS_EXT(Word Code)
{
  if (ChkArgCnt(2, 2)
   && check_core_mask(e_core_all_v))
  {
    if (DecodeAdr(&ArgStr[1], MTypeReg8) == TypeReg8)
    {
      BAsmCode[CodeLen    ] = 0x0f;
      BAsmCode[CodeLen + 1] = Code;
      BAsmCode[CodeLen + 2] = 0xc0 + AdrMode;
      switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeImm))
      {
        case TypeReg8:
          BAsmCode[CodeLen + 2] += (AdrMode << 3);
          CodeLen += 3;
          break;
        case TypeImm:
          if (AdrVals[0] > 15) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
          else
          {
            BAsmCode[CodeLen + 1] += 8;
            BAsmCode[CodeLen + 3] = AdrVals[0];
            CodeLen += 4;
          }
          break;
        default:
          break;
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPO2(Word Code)
 * \brief  handle FPO2 instruction
 * ------------------------------------------------------------------------ */

static void DecodeFPO2(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2)
   && check_core_mask(e_core_all_v))
  {
    Byte AdrByte;
    Boolean OK;

    AdrByte = EvalStrIntExpression(&ArgStr[1], Int4, &OK);
    if (OK)
    {
      BAsmCode[CodeLen    ] = 0x66 + (AdrByte >> 3);
      BAsmCode[CodeLen + 1] = (AdrByte & 7) << 3;
      if (ArgCnt == 1)
      {
        BAsmCode[CodeLen + 1] += 0xc0;
        CodeLen += 2;
      }
      else
      {
        switch (DecodeAdr(&ArgStr[2], MTypeReg8 | MTypeMem))
        {
          case TypeReg8:
            BAsmCode[CodeLen + 1] += 0xc0 + AdrMode;
            CodeLen += 2;
            break;
          case TypeMem:
            BAsmCode[CodeLen + 1] += AdrMode;
            copy_adr_vals(2);
            CodeLen += 2 + AdrCnt;
            break;
          default:
            break;
        }
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBTCLR(Word Code)
 * \brief  handle BTCLR instruction
 * ------------------------------------------------------------------------ */

static void DecodeBTCLR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && check_core_mask(e_core_all_v35))
  {
    Boolean OK;

    BAsmCode[CodeLen  ] = 0x0f;
    BAsmCode[CodeLen + 1] = 0x9c;
    BAsmCode[CodeLen + 2] = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      BAsmCode[CodeLen + 3] = EvalStrIntExpression(&ArgStr[2], UInt3, &OK);
      if (OK)
      {
        Word AdrWord;
        tSymbolFlags Flags;

        AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[3], Int16, &OK, & Flags) - (EProgCounter() + 5);
        if (OK)
        {
          if (!mSymbolQuestionable(Flags) && ((AdrWord > 0x7f) && (AdrWord < 0xff80))) WrStrErrorPos(ErrNum_DistTooBig, &ArgStr[3]);
          else
          {
            BAsmCode[CodeLen + 4] = Lo(AdrWord);
            CodeLen += 5;
          }
        }
      }
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg16(Word Index)
 * \brief  handle instructions with one 16 bit register argument
 * \param  Index index into list of instructions
 * ------------------------------------------------------------------------ */

static void DecodeReg16(Word Index)
{
  const AddOrder *pOrder = Reg16Orders + Index;

  if (ChkArgCnt(1, 1))
  {
    switch (DecodeAdr(&ArgStr[1], MTypeReg16))
    {
      case TypeReg16:
        PutCode(pOrder->Code);
        BAsmCode[CodeLen++] = pOrder->Add + AdrMode;
        break;
      default:
        break;
    }
  }
  AddPrefixes();
}

/*!------------------------------------------------------------------------
 * \fn     DecodeImm16(Word index)
 * \brief  handle instructions with one immediate 16 bit argument
 * \param  index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeImm16(Word index)
{
  const FixedOrder *p_order = &Imm16Orders[index];

  if (ChkArgCnt(1, 1)
   && check_core_mask(p_order->core_mask))
  {
    Word arg;
    Boolean ok;

    PutCode(p_order->Code);
    arg = EvalStrIntExpression(&ArgStr[1], Int16, &ok);
    if (ok)
    {
      BAsmCode[CodeLen++] = Lo(arg);
      BAsmCode[CodeLen++] = Hi(arg);
    }
    else
      CodeLen = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeString(Word Index)
 * \brief  handle string instructions
 * \param  Index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeString(Word Index)
{
  const FixedOrder *pOrder = StringOrders + Index;

  if (ChkArgCnt(0, 0)
   && check_core_mask(pOrder->core_mask))
    PutCode(pOrder->Code);
  AddPrefixes();
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  create/initialize dynamic instruction and hash tables
 * ------------------------------------------------------------------------ */

static void AddFPU(const char *NName, Word NCode, InstProc NProc)
{
  char Instr[30];

  AddInstTable(InstTable, NName, NCode, NProc);
  as_snprintf(Instr, sizeof(Instr), "%cN%s", *NName, NName + 1);
  AddInstTable(InstTable, Instr, NCode | NO_FWAIT_FLAG, NProc);
}

static void AddFixed(const char *NName, Byte core_mask, Word NCode)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].core_mask = core_mask;
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddBrk(const char *NName, Byte core_mask, Word NCode)
{
  order_array_rsv_end(BrkOrders, FixedOrder);
  BrkOrders[InstrZ].core_mask = core_mask;
  BrkOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeBrk);
}

static void AddFPUFixed(const char *NName, Word NCode)
{
  AddFPU(NName, NCode, DecodeFPUFixed);
}

static void AddFPUSt(const char *NName, Word NCode)
{
  AddFPU(NName, NCode, DecodeFPUSt);
}

static void AddFPU16(const char *NName, Word NCode)
{
  AddFPU(NName, NCode, DecodeFPU16);
}

static void AddString(const char *NName, Byte core_mask, Word NCode)
{
  order_array_rsv_end(StringOrders, FixedOrder);
  StringOrders[InstrZ].Name = NName;
  StringOrders[InstrZ].core_mask = core_mask;
  StringOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeString);
}

static void AddRept(const char *NName, Byte core_mask, Word NCode)
{
  order_array_rsv_end(ReptOrders, FixedOrder);
  ReptOrders[InstrZ].Code = NCode;
  ReptOrders[InstrZ].core_mask = core_mask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRept);
}

static void AddRel(const char *NName, Byte core_mask, Word NCode)
{
  order_array_rsv_end(RelOrders, FixedOrder);
  RelOrders[InstrZ].core_mask = core_mask;
  RelOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRel);
}

static void AddModReg(const char *NName, Byte core_mask, Word NCode, Boolean no_seg_check)
{
  order_array_rsv_end(ModRegOrders, ModRegOrder);
  ModRegOrders[InstrZ].core_mask = core_mask;
  ModRegOrders[InstrZ].Code = NCode;
  ModRegOrders[InstrZ].no_seg_check = no_seg_check;
  AddInstTable(InstTable, NName, InstrZ++, DecodeModReg);
}

static void AddShift(const char *NName, Byte core_mask, Word NCode)
{
  order_array_rsv_end(ShiftOrders, FixedOrder);
  ShiftOrders[InstrZ].core_mask = core_mask;
  ShiftOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeShift);
}

static void AddReg16(const char *NName, CPUVar NMin, Word NCode, Byte NAdd)
{
  order_array_rsv_end(Reg16Orders, AddOrder);
  Reg16Orders[InstrZ].MinCPU = NMin;
  Reg16Orders[InstrZ].Code = NCode;
  Reg16Orders[InstrZ].Add = NAdd;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg16);
}

static void AddImm16(const char *NName, Byte core_mask, Word code)
{
  order_array_rsv_end(Imm16Orders, FixedOrder);
  Imm16Orders[InstrZ].core_mask = core_mask;
  Imm16Orders[InstrZ].Code = code;
  AddInstTable(InstTable, NName, InstrZ++, DecodeImm16);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(403);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "MOV"  , 0, DecodeMOV);
  AddInstTable(InstTable, "INC"  , 0, DecodeINCDEC);
  AddInstTable(InstTable, "DEC"  , 8, DecodeINCDEC);
  AddInstTable(InstTable, "INT"  , 0, DecodeINT);
  AddInstTable(InstTable, "IN"   , 0, DecodeINOUT);
  AddInstTable(InstTable, "OUT"  , 2, DecodeINOUT);
  AddInstTable(InstTable, "CALL" , 0, DecodeCALLJMP);
  AddInstTable(InstTable, "JMP"  , 1, DecodeCALLJMP);
  AddInstTable(InstTable, "PUSH" , 0, DecodePUSHPOP);
  AddInstTable(InstTable, "POP"  , 1, DecodePUSHPOP);
  AddInstTable(InstTable, "NOT"  , 0, DecodeNOTNEG);
  AddInstTable(InstTable, "NEG"  , 8, DecodeNOTNEG);
  AddInstTable(InstTable, "RET"  , 0, DecodeRET);
  AddInstTable(InstTable, "RETF" , 8, DecodeRET);
  AddInstTable(InstTable, "TEST" , 0, DecodeTEST);
  AddInstTable(InstTable, "XCHG" , 0, DecodeXCHG);
  AddInstTable(InstTable, "CALLF", 0x189a, DecodeCALLJMPF);
  AddInstTable(InstTable, "JMPF" , 0x28ea, DecodeCALLJMPF);
  AddInstTable(InstTable, "ENTER", 0, DecodeENTER);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);
  AddInstTable(InstTable, "ROL4", 0x28, DecodeROL4_ROR4);
  AddInstTable(InstTable, "ROR4", 0x2a, DecodeROL4_ROR4);
  AddInstTable(InstTable, "INS", 0x31, DecodeINS_EXT);
  AddInstTable(InstTable, "EXT", 0x33, DecodeINS_EXT);
  AddInstTable(InstTable, "FPO2", 0, DecodeFPO2);
  AddInstTable(InstTable, "BTCLR", 0, DecodeBTCLR);
  AddFPU("FLD", 0, DecodeFLD);
  AddFPU("FILD", 0, DecodeFILD);
  AddFPU("FBLD", 0, DecodeFBLD);
  AddFPU("FST", 0xd0, DecodeFST_FSTP);
  AddFPU("FSTP", 0xd8, DecodeFST_FSTP);
  AddFPU("FIST", 0x10, DecodeFIST_FISTP);
  AddFPU("FISTP", 0x18, DecodeFIST_FISTP);
  AddFPU("FBSTP", 0, DecodeFBSTP);
  AddFPU("FCOM", 0xd0, DecodeFCOM_FCOMP);
  AddFPU("FCOMP", 0xd8, DecodeFCOM_FCOMP);
  AddFPU("FICOM", 0x10, DecodeFICOM_FICOMP);
  AddFPU("FICOMP", 0x18, DecodeFICOM_FICOMP);
  AddFPU("FADD", 0, DecodeFADD_FMUL);
  AddFPU("FMUL", 8, DecodeFADD_FMUL);
  AddFPU("FIADD", 0, DecodeFIADD_FIMUL);
  AddFPU("FIMUL", 8, DecodeFIADD_FIMUL);
  AddFPU("FADDP", 0, DecodeFADDP_FMULP);
  AddFPU("FMULP", 8, DecodeFADDP_FMULP);
  AddFPU("FDIV" , 16, DecodeFSUB_FSUBR_FDIV_FDIVR);
  AddFPU("FDIVR", 24, DecodeFSUB_FSUBR_FDIV_FDIVR);
  AddFPU("FSUB" ,  0, DecodeFSUB_FSUBR_FDIV_FDIVR);
  AddFPU("FSUBR",  8, DecodeFSUB_FSUBR_FDIV_FDIVR);
  AddFPU("FIDIV" , 16, DecodeFISUB_FISUBR_FIDIV_FIDIVR);
  AddFPU("FIDIVR", 24, DecodeFISUB_FISUBR_FIDIV_FIDIVR);
  AddFPU("FISUB" ,  0, DecodeFISUB_FISUBR_FIDIV_FIDIVR);
  AddFPU("FISUBR",  8, DecodeFISUB_FISUBR_FIDIV_FIDIVR);
  AddFPU("FDIVP" , 16, DecodeFSUBP_FSUBRP_FDIVP_FDIVRP);
  AddFPU("FDIVRP", 24, DecodeFSUBP_FSUBRP_FDIVP_FDIVRP);
  AddFPU("FSUBP" ,  0, DecodeFSUBP_FSUBRP_FDIVP_FDIVRP);
  AddFPU("FSUBRP",  8, DecodeFSUBP_FSUBRP_FDIVP_FDIVRP);
  AddFPU("FSAVE" , 0x30, DecodeFSAVE_FRSTOR);
  AddFPU("FRSTOR" , 0x20, DecodeFSAVE_FRSTOR);

  InstrZ = 0;
  AddFixed("AAA",   e_core_all,     0x0037);  AddFixed("AAS",   e_core_all,     0x003f);
  AddFixed("AAM",   e_core_all,     0xd40a);  AddFixed("AAD",   e_core_all,     0xd50a);
  AddFixed("CBW",   e_core_all,     0x0098);  AddFixed("CLC",   e_core_all,     0x00f8);
  AddFixed("CLD",   e_core_all,     0x00fc);  AddFixed("CLI",   e_core_all,     0x00fa);
  AddFixed("CMC",   e_core_all,     0x00f5);  AddFixed("CWD",   e_core_all,     0x0099);
  AddFixed("DAA",   e_core_all,     0x0027);  AddFixed("DAS",   e_core_all,     0x002f);
  AddFixed("HLT",   e_core_all,     0x00f4);  AddFixed("INTO",  e_core_all,     0x00ce);
  AddFixed("IRET",  e_core_all,     0x00cf);  AddFixed("LAHF",  e_core_all,     0x009f);
  AddFixed("LOCK",  e_core_all,     0x00f0);  AddFixed("NOP",   e_core_all,     0x0090);
  AddFixed("POPF",  e_core_all,     0x009d);  AddFixed("PUSHF", e_core_all,     0x009c);
  AddFixed("SAHF",  e_core_all,     0x009e);  AddFixed("STC",   e_core_all,     0x00f9);
  AddFixed("STD",   e_core_all,     0x00fd);  AddFixed("STI",   e_core_all,     0x00fb);
  AddFixed("WAIT",  e_core_all,     0x009b);  AddFixed("XLAT",  e_core_all,     0x00d7);
  AddFixed("LEAVE", e_core_all_186, 0x00c9);  AddFixed("PUSHA", e_core_all_186, 0x0060);
  AddFixed("POPA",  e_core_all_186, 0x0061);  AddFixed("ADD4S", e_core_all_v,   0x0f20);
  AddFixed("SUB4S", e_core_all_v,   0x0f22);  AddFixed("CMP4S", e_core_all_v,   0x0f26);
  AddFixed("STOP",  e_core_all_v35, 0x0f9e);  AddFixed("RETRBI",e_core_all_v35, 0x0f91);
  AddFixed("FINT",  e_core_all_v35, 0x0f92);  AddFixed("MOVSPA",e_core_all_v35, 0x0f25);
  AddFixed("SEGES", e_core_all,     0x0026);  AddFixed("SEGCS", e_core_all,     0x002e);
  AddFixed("SEGSS", e_core_all,     0x0036);  AddFixed("SEGDS", e_core_all,     0x003e);
  AddFixed("SEGDS2",e_core_all_v55, 0x0063);  AddFixed("SEGDS3",e_core_all_v55, 0x00d6);
  AddFixed("FWAIT", e_core_all,     0x009b);  AddFixed("IDLE",  e_core_v55sc,   0x0f9f);
  AddFixed("ALBIT", e_core_v55pi,   0x0f9a);  AddFixed("COLTRP",e_core_v55pi,   0x0f9b);
  AddFixed("MHENC", e_core_v55pi,   0x0f93);  AddFixed("MRENC", e_core_v55pi,   0x0f97);
  AddFixed("SCHEOL",e_core_v55pi,   0x0f78);  AddFixed("GETBIT",e_core_v55pi,   0x0f79);
  AddFixed("MHDEC", e_core_v55pi,   0x0f7c);  AddFixed("MRDEC", e_core_v55pi,   0x0f7d);
  AddFixed("CNVTRP",e_core_v55pi,   0x0f7a);  AddFixed("IRAM",  e_core_all_v55, 0x00f1);

  InstrZ = 0;
  AddBrk("BRKEM", e_core_v30, 0x0fff);
  AddBrk("BRKXA", e_core_v33, 0x0fe0);
  AddBrk("RETXA", e_core_v33, 0x0ff0);
  AddBrk("BRKS",  e_core_v35, 0x00f1);
  AddBrk("BRKN",  e_core_v35, 0x0063);

  AddFPUFixed("FCOMPP", 0xded9); AddFPUFixed("FTST",   0xd9e4);
  AddFPUFixed("FXAM",   0xd9e5); AddFPUFixed("FLDZ",   0xd9ee);
  AddFPUFixed("FLD1",   0xd9e8); AddFPUFixed("FLDPI",  0xd9eb);
  AddFPUFixed("FLDL2T", 0xd9e9); AddFPUFixed("FLDL2E", 0xd9ea);
  AddFPUFixed("FLDLG2", 0xd9ec); AddFPUFixed("FLDLN2", 0xd9ed);
  AddFPUFixed("FSQRT",  0xd9fa); AddFPUFixed("FSCALE", 0xd9fd);
  AddFPUFixed("FPREM",  0xd9f8); AddFPUFixed("FRNDINT",0xd9fc);
  AddFPUFixed("FXTRACT",0xd9f4); AddFPUFixed("FABS",   0xd9e1);
  AddFPUFixed("FCHS",   0xd9e0); AddFPUFixed("FPTAN",  0xd9f2);
  AddFPUFixed("FPATAN", 0xd9f3); AddFPUFixed("F2XM1",  0xd9f0);
  AddFPUFixed("FYL2X",  0xd9f1); AddFPUFixed("FYL2XP1",0xd9f9);
  AddFPUFixed("FINIT",  0xdbe3); AddFPUFixed("FENI",   0xdbe0);
  AddFPUFixed("FDISI",  0xdbe1); AddFPUFixed("FCLEX",  0xdbe2);
  AddFPUFixed("FINCSTP",0xd9f7); AddFPUFixed("FDECSTP",0xd9f6);
  AddFPUFixed("FNOP",   0xd9d0);

  AddFPUSt("FXCH",  0xd9c8);
  AddFPUSt("FFREE", 0xddc0);

  AddFPU16("FLDCW",  0xd928);
  AddFPU16("FSTCW",  0xd938);
  AddFPU16("FSTSW",  0xdd38);
  AddFPU16("FSTENV", 0xd930);
  AddFPU16("FLDENV", 0xd920);

  InstrZ = 0;
  AddString("CMPSB", e_core_all,     0x00a6);
  AddString("CMPSW", e_core_all,     0x00a7);
  AddString("LODSB", e_core_all,     0x00ac);
  AddString("LODSW", e_core_all,     0x00ad);
  AddString("MOVSB", e_core_all,     0x00a4);
  AddString("MOVSW", e_core_all,     0x00a5);
  AddString("SCASB", e_core_all,     0x00ae);
  AddString("SCASW", e_core_all,     0x00af);
  AddString("STOSB", e_core_all,     0x00aa);
  AddString("STOSW", e_core_all,     0x00ab);
  AddString("INSB",  e_core_all_186, 0x006c);
  AddString("INSW",  e_core_all_186, 0x006d);
  AddString("OUTSB", e_core_all_186, 0x006e);
  AddString("OUTSW", e_core_all_186, 0x006f);
  StringOrderCnt = InstrZ;

  InstrZ = 0;
  AddRept("REP",   e_core_all,   0x00f3);
  AddRept("REPE",  e_core_all,   0x00f3);
  AddRept("REPZ",  e_core_all,   0x00f3);
  AddRept("REPNE", e_core_all,   0x00f2);
  AddRept("REPNZ", e_core_all,   0x00f2);
  AddRept("REPC",  e_core_all_v, 0x0065);
  AddRept("REPNC", e_core_all_v, 0x0064);

  InstrZ = 0;
  AddRel("JA",    e_core_all, 0x0077); AddRel("JNBE",  e_core_all, 0x0077);
  AddRel("JAE",   e_core_all, 0x0073); AddRel("JNB",   e_core_all, 0x0073);
  AddRel("JB",    e_core_all, 0x0072); AddRel("JNAE",  e_core_all, 0x0072);
  AddRel("JBE",   e_core_all, 0x0076); AddRel("JNA",   e_core_all, 0x0076);
  AddRel("JC",    e_core_all, 0x0072); AddRel("JCXZ",  e_core_all, 0x00e3);
  AddRel("JE",    e_core_all, 0x0074); AddRel("JZ",    e_core_all, 0x0074);
  AddRel("JG",    e_core_all, 0x007f); AddRel("JNLE",  e_core_all, 0x007f);
  AddRel("JGE",   e_core_all, 0x007d); AddRel("JNL",   e_core_all, 0x007d);
  AddRel("JL",    e_core_all, 0x007c); AddRel("JNGE",  e_core_all, 0x007c);
  AddRel("JLE",   e_core_all, 0x007e); AddRel("JNG",   e_core_all, 0x007e);
  AddRel("JNC",   e_core_all, 0x0073); AddRel("JNE",   e_core_all, 0x0075);
  AddRel("JNZ",   e_core_all, 0x0075); AddRel("JNO",   e_core_all, 0x0071);
  AddRel("JNS",   e_core_all, 0x0079); AddRel("JNP",   e_core_all, 0x007b);
  AddRel("JPO",   e_core_all, 0x007b); AddRel("JO",    e_core_all, 0x0070);
  AddRel("JP",    e_core_all, 0x007a); AddRel("JPE",   e_core_all, 0x007a);
  AddRel("JS",    e_core_all, 0x0078); AddRel("LOOP",  e_core_all, 0x00e2);
  AddRel("LOOPE", e_core_all, 0x00e1); AddRel("LOOPZ", e_core_all, 0x00e1);
  AddRel("LOOPNE",e_core_all, 0x00e0); AddRel("LOOPNZ",e_core_all, 0x00e0);

  InstrZ = 0;
  AddModReg("LDS",   e_core_all,     0x00c5, False);
  AddModReg("LEA",   e_core_all,     0x008d, True );
  AddModReg("LES",   e_core_all,     0x00c4, False);
  AddModReg("BOUND", e_core_all_186, 0x0062, False);
  AddModReg("LDS3",  e_core_all,     0x0f36, False);
  AddModReg("LDS2",  e_core_all,     0x0f3e, False);

  InstrZ = 0;
  AddShift("SHL",   e_core_all, 4); AddShift("SAL",   e_core_all, 4);
  AddShift("SHR",   e_core_all, 5); AddShift("SAR",   e_core_all, 7);
  AddShift("ROL",   e_core_all, 0); AddShift("ROR",   e_core_all, 1);
  AddShift("RCL",   e_core_all, 2); AddShift("RCR",   e_core_all, 3);

  InstrZ = 0;
  AddReg16("BRKCS" , e_core_all_v35, 0x0f2d, 0xc0);
  AddReg16("TSKSW" , e_core_all_v35, 0x0f94, 0xf8);
  AddReg16("MOVSPB", e_core_all_v35, 0x0f95, 0xf8);

  InstrZ = 0;
  AddInstTable(InstTable, "ADD", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "OR" , InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "ADC", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "SBB", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "AND", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "SUB", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "XOR", InstrZ++, DecodeALU2);
  AddInstTable(InstTable, "CMP", InstrZ++, DecodeALU2);

  InstrZ = 0;
  AddInstTable(InstTable, "MUL" , InstrZ++, DecodeMul);
  AddInstTable(InstTable, "IMUL", InstrZ++, DecodeMul);
  AddInstTable(InstTable, "DIV" , InstrZ++, DecodeMul);
  AddInstTable(InstTable, "IDIV", InstrZ++, DecodeMul);

  InstrZ = 0;
  AddInstTable(InstTable, "TEST1", InstrZ++, DecodeBit1);
  AddInstTable(InstTable, "CLR1" , InstrZ++, DecodeBit1);
  AddInstTable(InstTable, "SET1" , InstrZ++, DecodeBit1);
  AddInstTable(InstTable, "NOT1" , InstrZ++, DecodeBit1);

  AddInstTable(InstTable, "BSCH" , 0x0f3c, DecodeBSCH);
  AddInstTable(InstTable, "RSTWDT", 0x0f96, DecodeRSTWDT);
  AddInstTable(InstTable, "BTCLRL", 0x0f9d, DecodeBTCLRL);

  InstrZ = 0;
  AddImm16("QHOUT",  e_core_all_v55, 0x0fe0);
  AddImm16("QOUT",   e_core_all_v55, 0x0fe1);
  AddImm16("QTIN",   e_core_all_v55, 0x0fe2);
}

/*!------------------------------------------------------------------------
 * \fn     DeinitFields(void)
 * \brief  dispose instructions fields after switch from target
 * ------------------------------------------------------------------------ */

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(BrkOrders);
  order_array_free(ReptOrders);
  order_array_free(ShiftOrders);
  order_array_free(StringOrders);
  order_array_free(ModRegOrders);
  order_array_free(Reg16Orders);
  order_array_free(RelOrders);
  order_array_free(Imm16Orders);
}

/*!------------------------------------------------------------------------
 * \fn     MakeCode_86(void)
 * \brief  parse/encode one instruction
 * ------------------------------------------------------------------------ */

static void MakeCode_86(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = eSymbolSizeUnknown;
  PrefixLen = 0;
  NoSegCheck = False;
  UnknownFlag = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  /* vermischtes */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     InitCode_86(void)
 * \brief  y86-specific initializations prior to pass
 * ------------------------------------------------------------------------ */

static void InitCode_86(void)
{
  SegAssumes[0] = SegNone; /* ASSUME ES:NOTHING */
  SegAssumes[1] = SegCode; /* ASSUME CS:CODE */
  SegAssumes[2] = SegNone; /* ASSUME SS:NOTHING */
  SegAssumes[3] = SegData; /* ASSUME DS:DATA */
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_86(void)
 * \brief  does instruction consume label field?
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsDef_86(void)
{
  return (Memo("PORT"));
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_86(void *p_user)
 * \brief  switch to x86 target
 * \param  p_user * to properties of specific variant
 * ------------------------------------------------------------------------ */

static void SwitchTo_86(void *p_user)
{
  p_curr_cpu_props = (const cpu_props_t*)p_user;

  TurnWords = False; SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = 0x42; NOPCode = 0x90;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegXData) | (1 << SegIO);
  Grans[SegCode ] = 1; ListGrans[SegCode ] = 1; SegInits[SegCode ] = 0;
  SegLimits[SegCode ] = 0xffff;
  Grans[SegData ] = 1; ListGrans[SegData ] = 1; SegInits[SegData ] = 0;
  SegLimits[SegData ] = 0xffff;
  Grans[SegXData] = 1; ListGrans[SegXData] = 1; SegInits[SegXData] = 0;
  SegLimits[SegXData] = 0xffff;
  Grans[SegIO   ] = 1; ListGrans[SegIO   ] = 1; SegInits[SegIO   ] = 0;
  SegLimits[SegIO   ] = 0xffff;

  pASSUMEOverride = DecodeASSUME;

  MakeCode = MakeCode_86; IsDef = IsDef_86;
  SwitchFrom = DeinitFields; InitFields();
  onoff_fpu_add();
}

/*!------------------------------------------------------------------------
 * \fn     code86_init(void)
 * \brief  register x86 target
 * ------------------------------------------------------------------------ */

static const cpu_props_t cpu_props[] =
{
  { "8088"   , e_core_86    },
  { "8086"   , e_core_86    },
  { "80188"  , e_core_186   },
  { "80186"  , e_core_186   },
  { "V20"    , e_core_v30   },
  { "V25"    , e_core_v35   },
  { "V30"    , e_core_v30   },
  { "V33"    , e_core_v33   },
  { "V35"    , e_core_v35   },
  { "V40"    , e_core_v30   },
  { "V50"    , e_core_v30   },
  { "V53"    , e_core_v33   },
  { "V55"    , e_core_v55   },
  { "V55SC"  , e_core_v55sc },
  { "V55PI"  , e_core_v55pi },
  { ""       , e_core_86    }
};

void code86_init(void)
{
  const cpu_props_t *p_prop;
  for (p_prop = cpu_props; p_prop->name[0]; p_prop++)
    (void)AddCPUUser(p_prop->name, SwitchTo_86, (void*)p_prop, NULL);

  AddInitPassProc(InitCode_86);
}
