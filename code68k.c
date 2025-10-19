/* code68k.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 680x0-Familie                                               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "bpemu.h"
#include "be_le.h"
#include "ieeefloat.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmcode.h"
#include "motpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "errmsg.h"
#include "codepseudo.h"
#include "headids.h"

#include "code68k.h"

/* If set to one, the implicit subtraction of the current program counter
   for PC-relative addressing is only done if the base symbol is from
   CODE address space: */

#define PCREL_ONLY_ON_CODESEG 0

/* If set to one, allow An/PC-relative displacements of 0x8000...0xffff resp.
   0x80 to 0xff, though the argument is a signed 16 resp. 8 bit value: */

#define AN_PCREL_OUTDISP_ALLOW_SIGNEXT 0

typedef enum
{
  e68KGen1a, /* 68008/68000 */
  e68KGen1b, /* 68010/68012 */
  eColdfire,
  eCPU32,
  e68KGen2,  /* 68020/68030 */
  e68KGen3   /* 68040 */
} tFamily;

#define ExtAddrFamilyMask ((1 << e68KGen3) | (1 << e68KGen2) | (1 << eCPU32))

typedef enum
{
  eCfISA_None,
  eCfISA_A,
  eCfISA_APlus,
  eCfISA_B,
  eCfISA_C
} tCfISA;

typedef enum
{
  eFlagNone = 0,
  eFlagLogCCR = 1 << 0,
  eFlagIdxScaling = 1 << 1,
  eFlagCALLM_RTM = 1 << 2,
  eFlagIntFPU = 1 << 3,
  eFlagExtFPU = 1 << 4,
  eFlagIntPMMU = 1 << 5,
  eFlagBranch32 = 1 << 6,
  eFlagMAC = 1 << 7,
  eFlagEMAC = 1 << 8
} tSuppFlags;

#define eSymbolSizeShiftCnt ((tSymbolSize)8)

#ifdef __cplusplus
# include "code68k.hpp"
#endif

enum
{
  Std_Variant = 0,
  I_Variant = 4,
  A_Variant = 8,
  VariantMask = 12
};

typedef struct
{
  const char *Name;
  Word Code;
} tCtReg;

#define MAX_CTREGS_GROUPS 4

typedef struct
{
  const char *pName;
  LongWord AddrSpaceMask;
  tFamily Family;
  tCfISA CfISA;
  tSuppFlags SuppFlags;
  const tCtReg *pCtRegs[MAX_CTREGS_GROUPS];
} tCPUProps;

typedef struct
{
  Word Code;
  Boolean MustSup;
  Word FamilyMask;
} FixedOrder;

typedef struct
{
  Byte Code;
  Boolean Dya;
  tSuppFlags NeedsSuppFlags;
} FPUOp;

typedef struct
{
  const char *pName;
  tSymbolSize Size;
  Word Code;
} PMMUReg;

#define EMACAvailName  "HASEMAC"

#define REG_SP 15
#define REG_FPCTRL 8
#define REG_FPCR 4
#define REG_FPSR 2
#define REG_FPIAR 1

typedef enum
{
  ModNone = 0,
  ModData = 1,
  ModAdr = 2,
  ModAdrI = 3,
  ModPost = 4,
  ModPre = 5,
  ModDAdrI = 6,
  ModAIX = 7,
  ModPC = 8,
  ModPCIdx = 9,
  ModAbs = 10,
  ModImm = 11,
  ModFPn = 12,
  ModFPCR = 13
} adrmode_t;

enum
{
  MModData = 1 << (ModData - 1),
  MModAdr = 1 << (ModAdr - 1),
  MModAdrI = 1 << (ModAdrI - 1),
  MModPost = 1 << (ModPost - 1),
  MModPre = 1 << (ModPre - 1),
  MModDAdrI = 1 << (ModDAdrI - 1),
  MModAIX = 1 << (ModAIX - 1),
  MModPC = 1 << (ModPC - 1),
  MModPCIdx = 1 << (ModPCIdx - 1),
  MModAbs = 1 << (ModAbs - 1),
  MModImm = 1 << (ModImm - 1),
  MModFPn = 1 << (ModFPn - 1),
  MModFPCR = 1 << (ModFPCR - 1)
};

typedef struct
{
  adrmode_t AdrMode;
  Word AdrPart;
  Word Vals[10], guess_masks[10];
  tSymbolFlags ImmSymFlags;
  int Cnt;
} tAdrResult;

static tSymbolSize OpSize;
static ShortInt RelPos;
static Boolean FullPMMU;                /* voller PMMU-Befehlssatz? */

static FixedOrder *FixedOrders;
static FPUOp *FPUOps;
static PMMUReg *PMMURegs;

static const tCPUProps *pCurrCPUProps;
static tSymbolSize NativeFloatSize;

static const Byte FSizeCodes[10] =
{
  6, 4, 0, 7, 0, 1, 5, 2, 0, 3
};

/*-------------------------------------------------------------------------*/
/* Unterroutinen */

static Boolean CheckFamilyCore(unsigned FamilyMask)
{
  return !!((FamilyMask >> pCurrCPUProps->Family) & 1);
}

static Boolean CheckFamily(unsigned FamilyMask)
{
  if (CheckFamilyCore(FamilyMask))
    return True;
  WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  CodeLen = 0;
  return False;
}

static Boolean CheckISA(unsigned ISAMask)
{
  if ((ISAMask >> pCurrCPUProps->CfISA) & 1)
    return True;
  WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  CodeLen = 0;
  return False;
}

static Boolean CheckNoFamily(unsigned FamilyMask)
{
  if (!CheckFamilyCore(FamilyMask))
    return True;
  WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  CodeLen = 0;
  return False;
}

static void CheckSup(void)
{
  if (!SupAllowed)
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
}

static Boolean CheckColdSize(void)
{
  if ((OpSize > eSymbolSize32Bit) || ((pCurrCPUProps->Family == eColdfire) && (OpSize < eSymbolSize32Bit)))
  {
    WrError(ErrNum_InvOpSize);
    return False;
  }
  else
    return True;
}

static Boolean CheckFloatSize(void)
{
  if (!*AttrPart.str.p_str)
    OpSize = NativeFloatSize;

  switch (OpSize)
  {
    case eSymbolSize8Bit:
    case eSymbolSize16Bit:
    case eSymbolSize32Bit:
    case eSymbolSizeFloat32Bit:
    case eSymbolSizeFloat64Bit:
      return True;
    case eSymbolSizeFloat96Bit:
    case eSymbolSizeFloatDec96Bit:
      if (pCurrCPUProps->Family != eColdfire)
        return True;
      /* else fall-through */
    default:
      WrError(ErrNum_InvOpSize);
      return False;
  }
}

static Boolean FloatOpSizeFitsDataReg(tSymbolSize OpSize)
{
  return (OpSize <= eSymbolSize32Bit) || (OpSize == eSymbolSizeFloat32Bit);
}

static Boolean ValReg(char Ch)
{
  return ((Ch >= '0') && (Ch <= '7'));
}

/*-------------------------------------------------------------------------*/
/* Register Symbols */

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Word *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Word *pResult)
{
  if (!as_strcasecmp(pArg, "SP"))
  {
    *pResult = REG_SP | REGSYM_FLAG_ALIAS;
    return True;
  }

  if (strlen(pArg) != 2)
    return False;
  if ((*pResult = pArg[1] - '0') > 7)
    return False;

  switch (as_toupper(*pArg))
  {
    case 'D':
      return True;
    case 'A':
      *pResult |= 8;
      return True;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPRegCore(const char *pArg, Word *pResult)
 * \brief  check whether argument is an FPU register
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeFPRegCore(const char *pArg, Word *pResult)
{
  if (!as_strcasecmp(pArg, "FPCR"))
  {
    *pResult = REG_FPCTRL | REG_FPCR;
    return True;
  }
  if (!as_strcasecmp(pArg, "FPSR"))
  {
    *pResult = REG_FPCTRL | REG_FPSR;
    return True;
  }
  if (!as_strcasecmp(pArg, "FPIAR"))
  {
    *pResult = REG_FPCTRL | REG_FPIAR;
    return True;
  }

  if (strlen(pArg) != 3)
    return False;
  if (as_strncasecmp(pArg, "FP", 2))
    return False;
  if ((*pResult = pArg[2] - '0') > 7)
    return False;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_68K(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - 68K variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_68K(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  if (InpSize == NativeFloatSize)
  {
    switch (Value)
    {
      case REG_FPCTRL | REG_FPCR:
        as_snprintf(pDest, DestSize, "FPCR");
        break;
      case REG_FPCTRL | REG_FPSR:
        as_snprintf(pDest, DestSize, "FPSR");
        break;
      case REG_FPCTRL | REG_FPIAR:
        as_snprintf(pDest, DestSize, "FPIAR");
        break;
      default:
        as_snprintf(pDest, DestSize, "FP%u", (unsigned)Value);
    }
  }
  else if (InpSize == eSymbolSize32Bit)
  {
    switch (Value)
    {
      case REGSYM_FLAG_ALIAS | REG_SP:
        as_snprintf(pDest, DestSize, "SP");
        break;
      default:
        as_snprintf(pDest, DestSize, "%c%u", Value & 8 ? 'A' : 'D', (unsigned)(Value & 7));
    }
  }
  else
    as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
}

/*!------------------------------------------------------------------------
 * \fn     compare_reg_68k(tRegInt reg1_num, tSymbolSize reg1_size, tRegInt reg2_num, tRegInt reg2_size)
 * \brief  compare two register symbols
 * \param  reg1_num 1st register's number
 * \param  reg1_size 1st register's data size
 * \param  reg2_num 2nd register's number
 * \param  reg2_size 2nd register's data size
 * \return 0, -1, 1, -2
 * ------------------------------------------------------------------------ */

static int compare_reg_68k(tRegInt reg1_num, tSymbolSize reg1_size, tRegInt reg2_num, tSymbolSize reg2_size)
{
  /* FP and Integer registers in different register files: */

  if (reg1_size != reg2_size)
    return -2;

  if (reg1_size == NativeFloatSize)
  {
    /* only FP data registers have an ordering: */

    if ((reg1_num & REG_FPCTRL) || (reg2_num & REG_FPCTRL))
      return (reg1_num == reg2_num) ? 0 : -2;
  }
  else if (reg1_size == eSymbolSize32Bit)
  {
    reg1_num &= ~REGSYM_FLAG_ALIAS;
    reg2_num &= ~REGSYM_FLAG_ALIAS;
  }

  if (reg1_num < reg2_num)
    return -1;
  else if (reg1_num > reg2_num)
    return 1;
  else
    return 0;
}

/*-------------------------------------------------------------------------*/
/* Adressparser */

typedef enum
{
  PC, AReg, Index, indir, Disp, None
} CompType;

/* static const char *CompNames[] = { "PC", "AReg", "Index", "indir", "Disp", "None" }; */

typedef struct
{
  tStrComp Comp;
  CompType Art;
  Word ANummer, INummer;
  Boolean Long;
  Word Scale;
  tSymbolSize Size;
  LongInt Wert;
} AdrComp;

static void ClrAdrVals(tAdrResult *pResult)
{
  pResult->AdrMode = ModNone;
  pResult->AdrPart = 0;
  pResult->Cnt = 0;
  pResult->ImmSymFlags = eSymbolFlag_None;
  memset(pResult->guess_masks, 0, sizeof(pResult->guess_masks));
}

static void append_adr_vals(tAdrResult *p_result, Word value, Word value_mask, tSymbolFlags flags)
{
  if (mFirstPassUnknownOrQuestionable(flags))
    p_result->guess_masks[p_result->Cnt >> 1] = value_mask;
  p_result->Vals[p_result->Cnt >> 1] = value;
  p_result->Cnt += 2;
}

static void append_adr_vals_const(tAdrResult *p_result, Word value)
{
  append_adr_vals(p_result, value, 0xffff, eSymbolFlag_None);
}

static void append_adr_vals_8(tAdrResult *p_result, Byte value, tSymbolFlags flags)
{
  append_adr_vals(p_result, value, 0x00ff, flags);
}

static void append_adr_vals_16(tAdrResult *p_result, Word value, tSymbolFlags flags)
{
  append_adr_vals(p_result, value, 0xffff, flags);
}

static void append_adr_vals_32(tAdrResult *p_result, LongWord value, tSymbolFlags flags)
{
  append_adr_vals(p_result, value >> 16, 0xffff, flags);
  append_adr_vals(p_result, value & 0xffff, 0xffff, flags);
}

static void CopyAdrVals(unsigned dest_index, const tAdrResult *p_adr_result)
{
  int z;

  memcpy(&WAsmCode[dest_index], p_adr_result->Vals, p_adr_result->Cnt);
  for (z = 0; z < p_adr_result->Cnt >> 1; z++)
    or_wasmcode_guessed(dest_index + z, 1, p_adr_result->guess_masks[z]);
}

static Boolean ACheckFamily(unsigned FamilyMask, const tStrComp *pAdrComp, tAdrResult *pResult)
{
  if (CheckFamilyCore(FamilyMask))
    return True;
  WrStrErrorPos(ErrNum_AddrModeNotSupported, pAdrComp);
  ClrAdrVals(pResult);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Word *pErg, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \param  MustBeReg argument is expected to be a register
 * \return RegEvalResult
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
  *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
 * \brief  check whether argument is a FPU register or register alias
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \param  MustBeReg argument is expected to be a register
 * \return RegEvalResult
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeFPReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeFPRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, NativeFloatSize, MustBeReg);
  *pResult = RegDescr.Reg;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegOrFPReg(const tStrComp *pArg, Word *pErg, tSymbolSize *pSize, Boolean MustBeReg)
 * \brief  check whether argument is an CPU/FPU register or register alias
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \param  pSize size of register if yes
 * \param  MustBeReg argument is expected to be a register
 * \return RegEvalResult
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeRegOrFPReg(const tStrComp *pArg, Word *pResult, tSymbolSize *pSize, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    *pSize = eSymbolSize32Bit;
    return eIsReg;
  }
  if (DecodeFPRegCore(pArg->str.p_str, pResult))
  {
    *pSize = NativeFloatSize;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
  *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

static Boolean DecodeRegPair(tStrComp *pArg, Word *Erg1, Word *Erg2)
{
  char *pSep = strchr(pArg->str.p_str, ':');
  tStrComp Left, Right;

  if (!pSep)
    return False;
  StrCompSplitRef(&Left, &Right, pArg, pSep);
  return (DecodeReg(&Left, Erg1, False) == eIsReg)
      && (*Erg1 <= 7)
      && (DecodeReg(&Right, Erg2, False) == eIsReg)
      && (*Erg2 <= 7);
}

static Boolean CodeIndRegPair(tStrComp *pArg, Word *Erg1, Word *Erg2)
{
  char *pSep = strchr(pArg->str.p_str, ':');
  tStrComp Left, Right;

  if (!pSep)
    return False;
  StrCompSplitRef(&Left, &Right, pArg, pSep);

  if (!IsIndirect(Left.str.p_str) || !IsIndirect(Right.str.p_str))
    return False;
  StrCompShorten(&Left, 1);
  StrCompIncRefLeft(&Left, 1);
  StrCompShorten(&Right, 1);
  StrCompIncRefLeft(&Right, 1);

  return (DecodeReg(&Left, Erg1, False) == eIsReg)
      && (DecodeReg(&Right, Erg2, False) == eIsReg);
}

static Boolean CodeCache(char *Asc, Word *Erg)
{
   if (!as_strcasecmp(Asc, "NC"))
     *Erg = 0;
   else if (!as_strcasecmp(Asc, "IC"))
     *Erg = 2;
   else if (!as_strcasecmp(Asc, "DC"))
     *Erg = 1;
   else if (!as_strcasecmp(Asc, "IC/DC"))
     *Erg = 3;
   else if (!as_strcasecmp(Asc, "DC/IC"))
     *Erg = 3;
   else
     return False;
   return True;
}

static Boolean DecodeCtrlReg(char *Asc, Word *Erg)
{
  int Grp;
  String Asc_N;
  const tCtReg *pReg;

  strmaxcpy(Asc_N, Asc, STRINGSIZE);
  NLS_UpString(Asc_N);
  Asc = Asc_N;

  for (Grp = 0; Grp < MAX_CTREGS_GROUPS; Grp++)
  {
    pReg = pCurrCPUProps->pCtRegs[Grp];
    if (!pReg)
      return False;
    for (; pReg->Name; pReg++)
      if (!strcmp(pReg->Name, Asc))
      {
        *Erg = pReg->Code;
        return True;
      }
  }
  return False;
}

static Boolean decode_one_bit_field(const tStrComp *p_arg, Word *p_ret, tSymbolFlags *p_flags, Boolean range_1_32)
{
  switch (DecodeReg(p_arg, p_ret, False))
  {
    case eIsReg:
      *p_flags = eSymbolFlag_None;
      if (*p_ret > 7)
        return False;
      *p_ret |= 0x20;
      return True;
    case eIsNoReg:
    {
      Boolean val_ok;
      Word offs = !!range_1_32;

      *p_ret = EvalStrIntExpressionWithFlags(p_arg, Int8, &val_ok, p_flags);
      if (!val_ok)
        return False;
      if (mFirstPassUnknownOrQuestionable(*p_flags))
        *p_ret = (*p_ret & 31) + offs;
      if ((*p_ret < offs) || (*p_ret > offs + 31))
        return False;
      if (range_1_32 && (*p_ret == 32))
        *p_ret = 0;
      return True;
    }
    default:
      *p_flags = eSymbolFlag_None;
      return False;
  }
}

static Boolean SplitBitField(tStrComp *pArg, Word *Erg, Word *p_guess_mask)
{
  char *p;
  Word OfsVal;
  tStrComp FieldArg, OffsArg, WidthArg;
  tSymbolFlags flags;

  p = strchr(pArg->str.p_str, '{');
  if (!p)
    return False;
  StrCompSplitRef(pArg, &FieldArg, pArg, p);
  if ((!*FieldArg.str.p_str) || (FieldArg.str.p_str[strlen(FieldArg.str.p_str) - 1] != '}'))
    return False;
  StrCompShorten(&FieldArg, 1);

  p = strchr(FieldArg.str.p_str, ':');
  if (!p)
    return False;
  StrCompSplitRef(&OffsArg, &WidthArg, &FieldArg, p);
  *p_guess_mask = 0;
  if (!decode_one_bit_field(&OffsArg, &OfsVal, &flags, False))
    return False;
  if (mFirstPassUnknownOrQuestionable(flags))
    *p_guess_mask |= 0x3f << 6;
  if (!decode_one_bit_field(&WidthArg, Erg, &flags, True))
    return False;
  if (mFirstPassUnknownOrQuestionable(flags))
    *p_guess_mask |= 0x3f;
  *Erg += OfsVal << 6;
  return True;
}

static Boolean SplitSize(tStrComp *pArg, tSymbolSize *DispLen, unsigned OpSizeMask)
{
  tSymbolSize NewLen = eSymbolSizeUnknown;
  int ArgLen = strlen(pArg->str.p_str);

  if ((ArgLen > 2) && (pArg->str.p_str[ArgLen - 2] == '.'))
  {
    switch (as_toupper(pArg->str.p_str[ArgLen - 1]))
    {
      case 'B':
        if (OpSizeMask & 1)
          NewLen = eSymbolSize8Bit;
        else
          goto wrong;
        break;
      case 'W':
        if (OpSizeMask & 2)
          NewLen = eSymbolSize16Bit;
        else
          goto wrong;
        break;
      case 'L':
        if (OpSizeMask & 2)
          NewLen = eSymbolSize32Bit;
        else
          goto wrong;
        break;
      default:
      wrong:
        WrError(ErrNum_InvOpSize);
        return False;
    }
    if ((*DispLen != eSymbolSizeUnknown) && (*DispLen != NewLen))
    {
      WrError(ErrNum_ConfOpSizes);
      return False;
    }
    *DispLen = NewLen;
    StrCompShorten(pArg, 2);
  }

  return True;
}

static Boolean ClassComp(AdrComp *C)
{
  int comp_len = strlen(C->Comp.str.p_str), reg_len;
  char save, c_scale, c_size;

  C->Art = None;
  C->ANummer = C->INummer = 0;
  C->Long = False;
  C->Scale = 0;
  C->Size = eSymbolSizeUnknown;
  C->Wert = 0;

  if ((*C->Comp.str.p_str == '[') && (C->Comp.str.p_str[comp_len - 1] == ']'))
  {
    C->Art = indir;
    return True;
  }

  if (!as_strcasecmp(C->Comp.str.p_str, "PC"))
  {
    C->Art = PC;
    return True;
  }

  /* assume register, splitting off scale & size first: */

  reg_len = comp_len;
  c_scale = c_size = '\0';
  if ((reg_len > 2) && (C->Comp.str.p_str[reg_len - 2] == '*'))
  {
    c_scale = C->Comp.str.p_str[reg_len - 1];
    reg_len -= 2;
  }
  if ((reg_len > 2) && (C->Comp.str.p_str[reg_len - 2] == '.'))
  {
    c_size = C->Comp.str.p_str[reg_len - 1];
    reg_len -= 2;
  }
  save = C->Comp.str.p_str[reg_len];
  C->Comp.str.p_str[reg_len] = '\0';
  switch (DecodeReg(&C->Comp, &C->ANummer, False))
  {
    case eRegAbort:
      return False;
    case eIsReg:
      C->Comp.str.p_str[reg_len] = save;
      break;
    default: /* eIsNoReg */
      C->Comp.str.p_str[reg_len] = save;
      goto is_disp;
  }

  /* OK, we know it's a register, with optional scale & size: */

  if ((C->ANummer > 7) && !c_scale && !c_size)
  {
    C->Art = AReg;
    C->ANummer -= 8;
    return True;
  }

  if (c_size)
  {
    switch (as_toupper(c_size))
    {
      case 'L':
        C->Long = True;
        break;
      case 'W':
        C->Long = False;
        break;
      default:
        return False;
    }
  }
  else
    C->Long = (pCurrCPUProps->Family == eColdfire);

  if (c_scale)
  {
    switch (c_scale)
    {
      case '1':
        C->Scale = 0;
        break;
      case '2':
        C->Scale = 1;
        break;
      case '4':
        C->Scale = 2;
        break;
      case '8':
        if (pCurrCPUProps->Family == eColdfire)
          return False;
        C->Scale = 3;
        break;
      default:
        return False;
    }
  }
  else
    C->Scale = 0;
  C->INummer = C->ANummer;
  C->Art = Index;
  return True;

is_disp:
  C->Art = Disp;
  if ((comp_len >= 2) && (C->Comp.str.p_str[comp_len - 2] == '.'))
  {
    switch (as_toupper(C->Comp.str.p_str[comp_len - 1]))
    {
      case 'L':
        C->Size = eSymbolSize32Bit;
        break;
      case 'W':
        C->Size = eSymbolSize16Bit;
        break;
      default:
        return False;
    }
    StrCompShorten(&C->Comp, 2);
  }
  else
    C->Size = eSymbolSizeUnknown;
  C->Art = Disp;
  return True;
}

static void SwapAdrComps(AdrComp *pComp1, AdrComp *pComp2)
{
  AdrComp Tmp;

  Tmp = *pComp1;
  *pComp1 = *pComp2;
  *pComp2 = Tmp;
}

static void AdrCompToIndex(AdrComp *pComp)
{
  pComp->Art = Index;
  pComp->INummer = pComp->ANummer + 8;
  pComp->Long = False;
  pComp->Scale = 0;
}

static Boolean IsShortAdr(LongInt Addr)
{
  LongWord OrigAddr = (LongWord)Addr, ExtAddr;

  /* Assuming we would code this address as short address... */

  ExtAddr = OrigAddr & 0xffff;
  if (ExtAddr & 0x8000)
    ExtAddr |= 0xffff0000ul;

  /* ...would this result in the same address on the bus? */

  return (ExtAddr & pCurrCPUProps->AddrSpaceMask) == (OrigAddr & pCurrCPUProps->AddrSpaceMask);
}

static Boolean IsDisp8(LongInt Disp)
{
  return ((Disp >= -128) && (Disp <= 127));
}

static Boolean IsDisp16(LongInt Disp)
{
  return ((Disp >= -32768) && (Disp <= 32767));
}

tSymbolSize GetDispLen(LongInt Disp)
{
  if (IsDisp8(Disp))
    return eSymbolSize8Bit;
  else if (IsDisp16(Disp))
    return eSymbolSize16Bit;
  else
    return eSymbolSize32Bit;
}

static void ChkEven(LongInt Adr)
{
  switch (pCurrCPUProps->Family)
  {
    case e68KGen1a:
    case e68KGen1b:
    case eColdfire:
      if (Odd(Adr))
        WrError(ErrNum_AddrNotAligned);
      break;
    default:
      break;
  }
}

static void DecodeAbs(const tStrComp *pArg, tSymbolSize Size, tAdrResult *pResult)
{
  Boolean ValOK;
  tSymbolFlags Flags;
  LongInt HVal;
  Integer HVal16;

  pResult->Cnt = 0;

  HVal = EvalStrIntExpressionWithFlags(pArg, Int32, &ValOK, &Flags);

  if (ValOK)
  {
    if (!mFirstPassUnknown(Flags) && (OpSize > eSymbolSize8Bit))
      ChkEven(HVal);
    HVal16 = HVal;

    if (Size == eSymbolSizeUnknown)
      Size = (IsShortAdr(HVal)) ? eSymbolSize16Bit : eSymbolSize32Bit;
    pResult->AdrMode = ModAbs;

    if (Size == eSymbolSize16Bit)
    {
      if (!IsShortAdr(HVal))
      {
        WrError(ErrNum_NoShortAddr);
        pResult->AdrMode = ModNone;
      }
      else
      {
        pResult->AdrPart = 0x38;
        append_adr_vals_16(pResult, HVal16, Flags);
      }
    }
    else
    {
      pResult->AdrPart = 0x39;
      append_adr_vals_32(pResult, HVal, Flags);
    }
  }
}

static tSymbolSize deduce_outdisp_size_32_16(LongInt *p_disp, const tEvalResult *p_eval_result, const tStrComp *p_arg)
{
  if (IsDisp16(*p_disp))
    return eSymbolSize16Bit;
  else if (CheckFamilyCore(ExtAddrFamilyMask))
    return eSymbolSize32Bit;
  else if (mSymbolQuestionable(p_eval_result->Flags))
  {
    *p_disp &= 0x7fff;
    return eSymbolSize16Bit;
  }
#if AN_PCREL_OUTDISP_ALLOW_SIGNEXT
  else if ((*p_disp >= 0x8000) && (*p_disp <= 0xffff))
  {
    char str[40];
    LargeWord v1 = *p_disp, v2 = v1 | 0xffff0000ul;

    as_snprintf(str, sizeof(str), "%lllx -> %lllx", v1, v2);
    WrXErrorPos(ErrNum_SignExtension, str, &p_arg->Pos);
    *p_disp -= 0x10000ul;
    return eSymbolSize16Bit;
  }
#endif
  else
  {
    WrStrErrorPos((*p_disp > 0) ? ErrNum_OverRange : ErrNum_UnderRange, p_arg);
    return eSymbolSizeUnknown;
  }
}

static tSymbolSize deduce_outdisp_size_32_8(LongInt *p_disp, const tEvalResult *p_eval_result, const tStrComp *p_arg)
{
  if (IsDisp8(*p_disp))
    return eSymbolSize8Bit;
  else if (CheckFamilyCore(ExtAddrFamilyMask))
    return GetDispLen(*p_disp);
  else if (mSymbolQuestionable(p_eval_result->Flags))
  {
    *p_disp &= 0x7f;
    return eSymbolSize8Bit;
  }
#if AN_PCREL_OUTDISP_ALLOW_SIGNEXT
  else if ((*p_disp >= 0x80) && (*p_disp <= 0xff))
  {
    char str[40];
    LargeWord v1 = *p_disp, v2 = v1 | 0xffffff00ul;

    as_snprintf(str, sizeof(str), "%lllx -> %lllx", v1, v2);
    WrXErrorPos(ErrNum_SignExtension, str, &p_arg->Pos);
    *p_disp -= 0x100u;
    return eSymbolSize8Bit;
  }
#endif
  else
  {
    WrStrErrorPos((*p_disp > 0) ? ErrNum_OverRange : ErrNum_UnderRange, p_arg);
    return eSymbolSizeUnknown;
  }
}

/* Additional qualifier to detect patterns where the program counter ('*') is the outer
   displacement or part of it:
   - *(...      -> outer displacement
   - 12345*(... -> no outer displacement
   - sym*(...   -> no outer displacement
   - sym+*(...  -> outer displacement
   - (...)*(... -> no outer displacement
 */

static Boolean isalnum_ubar_par(char ch)
{
  return as_isalnum_ubar(ch) || (ch == ')');
}

static int pc_outdisp_qualifier(const char *p_arg, int last_non_blank_pos, int split_pos)
{
  int z;

  if ((last_non_blank_pos < 0) || (p_arg[last_non_blank_pos] != '*'))
    return -1;

  for (z = last_non_blank_pos - 1; z >= 0; z--)
  {
    if (as_isspace(p_arg[z]))
      continue;
    break;
  }
  return ((z < 0) || !isalnum_ubar_par(p_arg[z])) ? split_pos : -1;
}

static Byte DecodeAdr(const tStrComp *pArg, Word Erl, tAdrResult *pResult)
{
  Byte i;
  int ArgLen;
  int outdisp_split_pos;
  Word rerg;
  Boolean doklamm;

  AdrComp AdrComps[3], OneComp;
  Byte CompCnt;
  tSymbolSize OutDispLen = eSymbolSizeUnknown;
  Boolean PreInd;

  LongInt HVal;
  Integer HVal16;
  ShortInt HVal8;
  as_float_t DVal;
  int ret;
  Boolean ValOK;
  tSymbolFlags symbol_flags;
  Word SwapField[6];
  String ArgStr;
  tStrComp Arg;
  String CReg;
  tStrComp CRegArg;
  IntType DispIntType;
  tSymbolSize RegSize;

  /* some insns decode the same arg twice, so we must keep the original string intact. */

  StrCompMkTemp(&Arg, ArgStr, sizeof(ArgStr));
  StrCompCopy(&Arg, pArg);
  KillPrefBlanksStrComp(&Arg);
  KillPostBlanksStrComp(&Arg);
  ArgLen = strlen(Arg.str.p_str);
  ClrAdrVals(pResult);

  StrCompMkTemp(&CRegArg, CReg, sizeof(CReg));

  /* immediate : */

  if (*Arg.str.p_str == '#')
  {
    tStrComp ImmArg;
    tEvalResult eval_result;
    int z;

    StrCompRefRight(&ImmArg, &Arg, 1);
    KillPrefBlanksStrComp(&ImmArg);

    switch (OpSize)
    {
      case eSymbolSize8Bit:
        HVal8 = EvalStrIntExpressionWithResult(&ImmArg, Int8, &eval_result);
        if (eval_result.OK)
          append_adr_vals_8(pResult, (Byte) HVal8, eval_result.Flags);
        break;
      case eSymbolSize16Bit:
        HVal16 = EvalStrIntExpressionWithResult(&ImmArg, Int16, &eval_result);
        if (eval_result.OK)
          append_adr_vals_16(pResult, (Word) HVal16, eval_result.Flags);
        break;
      case eSymbolSize32Bit:
        HVal = EvalStrIntExpressionWithResult(&ImmArg, Int32, &eval_result);
        if (eval_result.OK)
          append_adr_vals_32(pResult, HVal, eval_result.Flags);
        break;
      case eSymbolSize64Bit:
      {
        LargeInt QVal = EvalStrIntExpressionWithResult(&ImmArg, LargeIntType, &eval_result);
        if (eval_result.OK)
        {
          append_adr_vals_32(pResult,
#ifdef HAS64
                             QVal >> 32,
#else
                             (QVal & 0x80000000ul) ? 0xfffffffful : 0x00000000ul,
#endif
                             eval_result.Flags);
          append_adr_vals_32(pResult, QVal & 0xfffffffful, eval_result.Flags);
        }
        break;
      }
      case eSymbolSizeFloat32Bit:
        DVal = EvalStrFloatExpressionWithResult(&ImmArg, &eval_result);
        if (eval_result.OK)
        {
          if ((ret = as_float_2_ieee4(DVal, (Byte *) SwapField, HostBigEndian)) < 0)
          {
            asmerr_check_fp_dispose_result(ret, &ImmArg);
            eval_result.OK = False;
          }
        }
        if (eval_result.OK)
        {
          if (HostBigEndian)
            DWSwap((Byte *) SwapField, 4);
          for (z = 1; z >= 0; z--)
            append_adr_vals_16(pResult, SwapField[z], eval_result.Flags);
        }
        break;
      case eSymbolSizeFloat64Bit:
        DVal = EvalStrFloatExpressionWithResult(&ImmArg, &eval_result);
        if (eval_result.OK)
        {
          if ((ret = as_float_2_ieee8(DVal, (Byte *) SwapField, HostBigEndian)) < 0)
          {
            asmerr_check_fp_dispose_result(ret, &ImmArg);
            eval_result.OK = False;
          }
        }
        if (eval_result.OK)
        {
          if (HostBigEndian)
            QWSwap((Byte *) SwapField, 8);
          for (z = 3; z >= 0; z--)
            append_adr_vals_16(pResult, SwapField[z], eval_result.Flags);
        }
        break;
      case eSymbolSizeFloat96Bit:
        DVal = EvalStrFloatExpressionWithResult(&ImmArg, &eval_result);
        if (eval_result.OK)
        {
          if ((ret = as_float_2_ieee10(DVal, (Byte *) SwapField, False)) < 0)
          {
            asmerr_check_fp_dispose_result(ret, &ImmArg);
            eval_result.OK = False;
          }
        }
        if (eval_result.OK)
        {
          if (HostBigEndian)
            WSwap((Byte *) SwapField, 10);
          append_adr_vals_16(pResult, SwapField[4], eval_result.Flags);
          append_adr_vals_const(pResult, 0x0000);
          for (z = 3; z >= 0; z--)
            append_adr_vals_16(pResult, SwapField[z], eval_result.Flags);
        }
        break;
      case eSymbolSizeFloatDec96Bit:
        DVal = EvalStrFloatExpressionWithResult(&ImmArg, &eval_result);
        if (eval_result.OK)
        {
          ConvertMotoFloatDec(DVal, (Byte *) SwapField, False);
          for (z = 5; z >= 0; z--)
            append_adr_vals_16(pResult, SwapField[z], eval_result.Flags);
        }
        break;
      case eSymbolSizeShiftCnt: /* special arg 1..8 */
        HVal8 = EvalStrIntExpressionWithResult(&ImmArg, UInt4, &eval_result);
        if (eval_result.OK)
        {
          if (mFirstPassUnknown(pResult->ImmSymFlags))
            HVal8 = 1;
          eval_result.OK = ChkRange(HVal8, 1, 8);
        }
        if (eval_result.OK)
          append_adr_vals(pResult, (Word)((Byte) HVal8), 0x0007, eval_result.Flags);
        break;
      default:
        eval_result.OK = False;
    }
    if (eval_result.OK)
    {
      pResult->AdrMode = ModImm;
      pResult->AdrPart = 0x3c;
      pResult->ImmSymFlags = eval_result.Flags;
    }
    goto chk;
  }

  /* CPU/FPU-Register direkt: */

  switch (DecodeRegOrFPReg(&Arg, &pResult->AdrPart, &RegSize, False))
  {
    case eIsReg:
      pResult->Cnt = 0;
      if (RegSize == NativeFloatSize)
      {
        pResult->AdrMode = (pResult->AdrPart > 7) ? ModFPCR : ModFPn;
        pResult->AdrPart &= 7;
      }
      else
        pResult->AdrMode = (pResult->AdrPart >> 3) ? ModAdr : ModData;
      /* fall-through */
    case eRegAbort:
      goto chk;
    default:
      break;
  }

  /* Adressregister indirekt mit Predekrement: */

  if ((ArgLen >= 4) && (*Arg.str.p_str == '-') && (Arg.str.p_str[1] == '(') && (Arg.str.p_str[ArgLen - 1] == ')'))
  {
    StrCompCopySub(&CRegArg, &Arg, 2, ArgLen - 3);
    if ((DecodeReg(&CRegArg, &rerg, False) == eIsReg) && (rerg > 7))
    {
      pResult->AdrPart = rerg + 24;
      pResult->Cnt = 0;
      pResult->AdrMode = ModPre;
      goto chk;
    }
  }

  /* Adressregister indirekt mit Postinkrement */

  if ((ArgLen >= 4) && (*Arg.str.p_str == '(') && (Arg.str.p_str[ArgLen - 2] == ')') && (Arg.str.p_str[ArgLen - 1] == '+'))
  {
    StrCompCopySub(&CRegArg, &Arg, 1, ArgLen - 3);
    if ((DecodeReg(&CRegArg, &rerg, False) == eIsReg) && (rerg > 7))
    {
      pResult->AdrPart = rerg + 16;
      pResult->Cnt = 0;
      pResult->AdrMode = ModPost;
      goto chk;
    }
  }

  /* Unterscheidung direkt<->indirekt: */

  outdisp_split_pos = FindDispBaseSplitWithQualifier(Arg.str.p_str, &ArgLen, pc_outdisp_qualifier, "()");
  if (outdisp_split_pos >= 0)
  {
    tStrComp OutDisp, IndirComps, Remainder;
    char *pCompSplit;

    /* aeusseres Displacement abspalten, Klammern loeschen: */

    StrCompSplitRef(&OutDisp, &IndirComps, &Arg, &Arg.str.p_str[outdisp_split_pos]);
    OutDispLen = eSymbolSizeUnknown;
    if (!SplitSize(&OutDisp, &OutDispLen, 7))
      return ModNone;
    StrCompShorten(&IndirComps, 1);

    /* in Komponenten zerteilen: */

    CompCnt = 0;
    do
    {
      doklamm = True;
      pCompSplit = IndirComps.str.p_str;
      do
      {
        if (*pCompSplit == '[')
          doklamm = False;
        else if (*pCompSplit == ']')
          doklamm = True;
        pCompSplit++;
      }
      while (((!doklamm) || (*pCompSplit != ',')) && (*pCompSplit != '\0'));

      if (*pCompSplit == '\0')
      {
        AdrComps[CompCnt].Comp = IndirComps;
        pCompSplit = NULL;
      }
      else
      {
        StrCompSplitRef(&AdrComps[CompCnt].Comp, &Remainder, &IndirComps, pCompSplit);
        IndirComps = Remainder;
      }

      KillPrefBlanksStrCompRef(&AdrComps[CompCnt].Comp);
      KillPostBlanksStrComp(&AdrComps[CompCnt].Comp);

      /* ignore empty component */

      if (!AdrComps[CompCnt].Comp.str.p_str[0])
        continue;
      if (!ClassComp(&AdrComps[CompCnt]))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &AdrComps[CompCnt].Comp);
        return ModNone;
      }

      /* Base register position is already occupied and we get another one: */

      if ((CompCnt == 1) && ((AdrComps[CompCnt].Art == AReg) || (AdrComps[CompCnt].Art == PC)))
      {
        /* Index register at "base position": just swap comp 0 & 1, so we get (An,Xi) or (PC,Xi): */

        if (AdrComps[0].Art == Index)
          SwapAdrComps(&AdrComps[CompCnt], &AdrComps[0]);

        /* Address register at "base position" and we add PC: also swap and convert it to index so we get again (PC,Xi): */

        else if ((AdrComps[0].Art == AReg) && (AdrComps[CompCnt].Art == PC))
        {
          SwapAdrComps(&AdrComps[CompCnt], &AdrComps[0]);
          AdrCompToIndex(&AdrComps[CompCnt]);
        }

        /* Otherwise, convert address to general index register.  Result may require 68020++ modes: */

        else
          AdrCompToIndex(&AdrComps[CompCnt]);

        CompCnt++;
      }

      /* a displacement found inside (...), but outside [...].  Explicit
         sizes must be consistent, implicitly checked by SplitSize(). */

      else if (AdrComps[CompCnt].Art == Disp)
      {
        if (*OutDisp.str.p_str)
        {
          WrError(ErrNum_InvAddrMode);
          return ModNone;
        }
        OutDisp = AdrComps[CompCnt].Comp;
        OutDispLen = AdrComps[CompCnt].Size;
      }

      /* no second index */

      else if ((AdrComps[CompCnt].Art != Index) && (CompCnt != 0))
      {
        WrError(ErrNum_InvAddrMode);
        return ModNone;
      }

      else
        CompCnt++;
    }
    while (pCompSplit);

    if ((CompCnt > 2) || ((CompCnt > 1) && (AdrComps[0].Art == Index)))
    {
      WrError(ErrNum_InvAddrMode);
      return ModNone;
    }

    /* 0. Absolut in Klammern (d) */

    if (CompCnt == 0)
    {
      DecodeAbs(&OutDisp, OutDispLen, pResult);
    }

    /* 1. Variante (An....), d(An....) */

    else if (AdrComps[0].Art == AReg)
    {

      /* 1.1. Variante (An), d(An) */

      if (CompCnt == 1)
      {
        /* 1.1.1. Variante (An) */

        if ((*OutDisp.str.p_str == '\0') && ((MModAdrI & Erl) != 0))
        {
          pResult->AdrPart = 0x10 + AdrComps[0].ANummer;
          pResult->AdrMode = ModAdrI;
          pResult->Cnt = 0;
          goto chk;
        }

        /* 1.1.2. Variante d(An) */

        else
        {
          tEvalResult eval_result;

          /* only try 32-bit displacement if explicitly requested, or 68020++ and no size given */

          if (OutDispLen == eSymbolSizeUnknown)
            DispIntType = CheckFamilyCore(ExtAddrFamilyMask) ? SInt32
#if AN_PCREL_OUTDISP_ALLOW_SIGNEXT
                        : Int16;
#else
                        : SInt16;
#endif
          else
            DispIntType = (OutDispLen >= eSymbolSize32Bit) ? SInt32 : SInt16;

          HVal = EvalStrIntExpressionWithResult(&OutDisp, DispIntType, &eval_result);
          if (!eval_result.OK)
            return ModNone;
          if ((HVal == 0) && ((MModAdrI & Erl) != 0) && (OutDispLen == eSymbolSizeUnknown))
          {
            pResult->AdrPart = 0x10 + AdrComps[0].ANummer;
            pResult->AdrMode = ModAdrI;
            pResult->Cnt = 0;
            goto chk;
          }
          if (OutDispLen == eSymbolSizeUnknown)
          {
            OutDispLen = deduce_outdisp_size_32_16(&HVal, &eval_result, &OutDisp);
            if (OutDispLen == eSymbolSizeUnknown)
              return ModNone;
          }
          switch (OutDispLen)
          {
            case eSymbolSize16Bit:    /* d16(An) */
              pResult->AdrPart = 0x28 + AdrComps[0].ANummer;
              pResult->AdrMode = ModDAdrI;
              append_adr_vals_16(pResult, HVal & 0xffff, eval_result.Flags);
              goto chk;
            case eSymbolSize32Bit:    /* d32(An) */
              pResult->AdrPart = 0x30 + AdrComps[0].ANummer;
              pResult->AdrMode = ModAIX;
              append_adr_vals_const(pResult, 0x0170);
              append_adr_vals_32(pResult, HVal, eval_result.Flags);
              ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
              goto chk;
            default:
              WrError(ErrNum_InternalError);
              break;
          }
        }
      }

      /* 1.2. Variante d(An,Xi) */

      else
      {
        tEvalResult eval_result;
        Word ext_word = (AdrComps[1].INummer << 12) + (Ord(AdrComps[1].Long) << 11) + (AdrComps[1].Scale << 9);

        pResult->AdrPart = 0x30 + AdrComps[0].ANummer;

        /* only try 32-bit displacement if explicitly requested, or 68020++ and no size given */

        if (OutDispLen == eSymbolSizeUnknown)
          DispIntType = CheckFamilyCore(ExtAddrFamilyMask) ? SInt32
#if AN_PCREL_OUTDISP_ALLOW_SIGNEXT
                      : Int8;
#else
                      : SInt8;
#endif
        else
          DispIntType = (OutDispLen >= eSymbolSize32Bit)
                      ? SInt32
                      : ((OutDispLen >= eSymbolSize16Bit) ? SInt16 : SInt8);
        HVal = EvalStrIntExpressionWithResult(&OutDisp, DispIntType, &eval_result);
        if (!eval_result.OK)
          return ModNone;
        if (OutDispLen == eSymbolSizeUnknown)
        {
          OutDispLen = deduce_outdisp_size_32_8(&HVal, &eval_result, &OutDisp);
          if (OutDispLen == eSymbolSizeUnknown)
            return ModNone;
        }
        switch (OutDispLen)
        {
          case eSymbolSize8Bit:
            pResult->AdrMode = ModAIX;
            append_adr_vals(pResult, ext_word | (HVal & 0xff), 0x00ff, eval_result.Flags);
            if ((AdrComps[1].Scale != 0) && (!(pCurrCPUProps->SuppFlags & eFlagIdxScaling)))
            {
              WrStrErrorPos(ErrNum_AddrModeNotSupported, &AdrComps[1].Comp);
              ClrAdrVals(pResult);
            }
            goto chk;
          case eSymbolSize16Bit:
            pResult->AdrMode = ModAIX;
            append_adr_vals_const(pResult, ext_word | 0x120);
            append_adr_vals_16(pResult, HVal & 0xffff, eval_result.Flags);
            ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
            goto chk;
          case eSymbolSize32Bit:
            pResult->AdrMode = ModAIX;
            append_adr_vals_const(pResult, ext_word | 0x130);
            append_adr_vals_32(pResult, HVal, eval_result.Flags);
            ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
            goto chk;
          default:
            WrError(ErrNum_InternalError);
            break;
        }
      }
    }

    /* 2. Variante d(PC....) */

    else if (AdrComps[0].Art == PC)
    {
      /* 2.1. Variante d(PC) */

      if (CompCnt == 1)
      {
        tEvalResult eval_result;

        if (OutDisp.str.p_str[0])
        {
          HVal = EvalStrIntExpressionWithResult(&OutDisp, Int32, &eval_result);
#if PCREL_ONLY_ON_CODESEG
          if (eval_result.AddrSpaceMask & (1 << SegCode))
#endif
            HVal -= (EProgCounter() + RelPos);
        }
        else
        {
          HVal = 0;
          eval_result.Flags = eSymbolFlag_None;
          eval_result.AddrSpaceMask = 0;
          eval_result.OK = True;
        }
        if (!eval_result.OK)
          return ModNone;
        if (OutDispLen == eSymbolSizeUnknown)
        {
          OutDispLen = deduce_outdisp_size_32_16(&HVal, &eval_result, &OutDisp); 
          if (OutDispLen == eSymbolSizeUnknown)
            return ModNone;
        }
        switch (OutDispLen)
        {
          case eSymbolSize16Bit:
            pResult->AdrPart = 0x3a;
            if (!mSymbolQuestionable(eval_result.Flags) && !IsDisp16(HVal))
            {
              WrError(ErrNum_DistTooBig);
              return ModNone;
            }
            pResult->AdrMode = ModPC;
            append_adr_vals_16(pResult, HVal & 0xffff, eval_result.Flags);
            goto chk;
          case eSymbolSize32Bit:
            pResult->AdrPart = 0x3b;
            pResult->AdrMode = ModPCIdx;
            append_adr_vals_const(pResult, 0x170);
            append_adr_vals_32(pResult, HVal, eval_result.Flags);
            ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
            goto chk;
          default:
            WrError(ErrNum_InternalError);
            break;
        }
      }

      /* 2.2. Variante d(PC,Xi) */

      else
      {
        tEvalResult eval_result;
        Word ext_word = (AdrComps[1].INummer << 12) + (Ord(AdrComps[1].Long) << 11) + (AdrComps[1].Scale << 9);

        if (OutDisp.str.p_str[0])
        {
          HVal = EvalStrIntExpressionWithResult(&OutDisp, Int32, &eval_result);
#if PCREL_ONLY_ON_CODESEG
          if (eval_result.AddrSpaceMask & (1 << SegCode))
#endif
            HVal -= (EProgCounter() + RelPos);
        }
        else
        {
          HVal = 0;
          eval_result.Flags = eSymbolFlag_None;
          eval_result.AddrSpaceMask = 0;
          eval_result.OK = True;
        }
        if (!eval_result.OK)
          return ModNone;
        if (OutDispLen == eSymbolSizeUnknown)
        {
          OutDispLen = deduce_outdisp_size_32_8(&HVal, &eval_result, &OutDisp);
          if (OutDispLen == eSymbolSizeUnknown)
            return ModNone;
        }
        pResult->AdrPart = 0x3b;
        switch (OutDispLen)
        {
          case eSymbolSize8Bit:
            if (!mSymbolQuestionable(eval_result.Flags) && !IsDisp8(HVal))
            {
              WrError(ErrNum_DistTooBig);
              return ModNone;
            }
            append_adr_vals(pResult, ext_word | (HVal & 0xff), 0x00ff, eval_result.Flags);
            pResult->AdrMode = ModPCIdx;
            if ((AdrComps[1].Scale != 0) && (!(pCurrCPUProps->SuppFlags & eFlagIdxScaling)))
            {
              WrStrErrorPos(ErrNum_AddrModeNotSupported, &AdrComps[1].Comp);
              ClrAdrVals(pResult);
            }
            goto chk;
          case eSymbolSize16Bit:
            if (!mSymbolQuestionable(eval_result.Flags) && !IsDisp16(HVal))
            {
              WrError(ErrNum_DistTooBig);
              return ModNone;
            }
            append_adr_vals_const(pResult, ext_word | 0x120);
            append_adr_vals_16(pResult, HVal & 0xffff, eval_result.Flags);
            pResult->AdrMode = ModPCIdx;
            ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
            goto chk;
          case eSymbolSize32Bit:
            append_adr_vals_const(pResult, ext_word | 0x130);
            append_adr_vals_32(pResult, HVal, eval_result.Flags);
            pResult->AdrMode = ModPCIdx;
            ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
            goto chk;
          default:
            WrError(ErrNum_InternalError);
            break;
        }
      }
    }

    /* 3. Variante (Xi), d(Xi) */

    else if (AdrComps[0].Art == Index)
    {
      Word ext_word = (AdrComps[0].INummer << 12) + (Ord(AdrComps[0].Long) << 11) + (AdrComps[0].Scale << 9) + 0x180;

      pResult->AdrPart = 0x30;
      if (*OutDisp.str.p_str == '\0')
      {
        append_adr_vals_const(pResult, ext_word | 0x0010);
        pResult->AdrMode = ModAIX;
        ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
        goto chk;
      }
      else
      {
        tEvalResult eval_result;

        HVal = EvalStrIntExpressionWithResult(&OutDisp, (OutDispLen != eSymbolSize16Bit) ? SInt32 : SInt16, &eval_result);
        if (eval_result.OK)
        {
          if (OutDispLen == eSymbolSizeUnknown)
            OutDispLen = IsDisp16(HVal) ? eSymbolSize16Bit : eSymbolSize32Bit;
          switch (OutDispLen)
          {
            case eSymbolSize8Bit:
            case eSymbolSize16Bit:
              append_adr_vals_const(pResult, ext_word | 0x0020);
              append_adr_vals_16(pResult, HVal & 0xffff, eval_result.Flags);
              pResult->AdrMode = ModAIX;
              ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
              goto chk;
            case eSymbolSize32Bit:
              append_adr_vals_const(pResult, ext_word | 0x0030);
              append_adr_vals_32(pResult, HVal, eval_result.Flags);
              pResult->AdrMode = ModAIX;
              ACheckFamily(ExtAddrFamilyMask, pArg, pResult);
              goto chk;
            default:
              WrError(ErrNum_InternalError);
              break;
          }
        }
      }
    }

    /* 4. Variante indirekt: */

    else if (AdrComps[0].Art == indir)
    {
      Word ext_word;

      /* erst ab 68020 erlaubt */

      if (!ACheckFamily((1 << e68KGen3) | (1 << e68KGen2), pArg, pResult))
        return ModNone;

      /* Unterscheidung Vor- <---> Nachindizierung: */

      if (CompCnt == 2)
      {
        PreInd = False;
        AdrComps[2] = AdrComps[1];
      }
      else
      {
        PreInd = True;
        AdrComps[2].Art = None;
      }

      /* indirektes Argument herauskopieren: */

      StrCompRefRight(&IndirComps, &AdrComps[0].Comp, 1);
      StrCompShorten(&IndirComps, 1);

      /* Felder loeschen: */

      for (i = 0; i < 2; AdrComps[i++].Art = None);

      /* indirekten Ausdruck auseinanderfieseln: */

      do
      {
        /* abschneiden & klassifizieren: */

        pCompSplit = strchr(IndirComps.str.p_str, ',');
        if (!pCompSplit)
          OneComp.Comp = IndirComps;
        else
        {
          StrCompSplitRef(&OneComp.Comp, &Remainder, &IndirComps, pCompSplit);
          IndirComps = Remainder;
        }
        KillPrefBlanksStrCompRef(&OneComp.Comp);
        KillPostBlanksStrComp(&OneComp.Comp);
        if (!ClassComp(&OneComp))
        {
          WrError(ErrNum_InvAddrMode);
          return ModNone;
        }

        /* passend einsortieren: */

        if ((AdrComps[1].Art != None) && (OneComp.Art == AReg))
        {
          OneComp.Art = Index;
          OneComp.INummer = OneComp.ANummer + 8;
          OneComp.Long = False;
          OneComp.Scale = 0;
        }
        switch (OneComp.Art)
        {
          case Disp:
            i = 0;
            break;
          case AReg:
          case PC:
            i = 1;
            break;
          case Index:
            i = 2;
            break;
          default:
            i = 3;
        }
        if ((i >= 3) || AdrComps[i].Art != None)
        {
          WrError(ErrNum_InvAddrMode);
          return ModNone;
        }
        else
          AdrComps[i] = OneComp;
      }
      while (pCompSplit);

      /* extension word: 68020 format */

      ext_word = 0x100;

      /* bit 2 = post-indexed. */

      if (!PreInd)
        ext_word |= 0x0004;

      /* 68K PRM says that IS=1 and I/IS=1xx is a reserved combination.
         If index register is suppressed, pre-indexing must be used.  Do
         not set bit 2: */

      if (AdrComps[2].Art == None)
        ext_word |= 0x0040;
      else
        ext_word |= (AdrComps[2].INummer << 12) + (Ord(AdrComps[2].Long) << 11) + (AdrComps[2].Scale << 9);

      /* 4.1 Variante d([...PC...]...) */

      if (AdrComps[1].Art == PC)
      {
        if (AdrComps[0].Art == None)
        {
          pResult->AdrPart = 0x3b;
          append_adr_vals_const(pResult, ext_word | 0x10);
          pResult->AdrMode = ModAIX;
        }
        else
        {
          HVal = EvalStrIntExpressionWithFlags(&AdrComps[0].Comp, Int32, &ValOK, &symbol_flags);
          HVal -= EProgCounter() + RelPos;
          if (!ValOK)
            return ModNone;
          switch (AdrComps[0].Size)
          {
            case eSymbolSizeUnknown:
             if (!HVal)
               goto PCIs0;
             if (IsDisp16(HVal))
               goto PCIs16;
             else
               goto PCIs32;
            case eSymbolSize16Bit:
              if (!IsDisp16(HVal) && !mFirstPassUnknownOrQuestionable(symbol_flags))
              {
                WrError(ErrNum_DistTooBig);
                return ModNone;
              }
              goto PCIs16;
            PCIs0:
              pResult->AdrPart = 0x3b;
              append_adr_vals_const(pResult, ext_word | 0x10);
              pResult->AdrMode = ModAIX;
              break;
            PCIs16:
              pResult->AdrPart = 0x3b;
              append_adr_vals_const(pResult, ext_word | 0x20);
              append_adr_vals_16(pResult, HVal & 0xffff, symbol_flags);
              pResult->AdrMode = ModAIX;
              break;
            case eSymbolSize32Bit:
            PCIs32:
              pResult->AdrPart = 0x3b;
              append_adr_vals_const(pResult, ext_word | 0x30);
              append_adr_vals_32(pResult, HVal, symbol_flags);
              pResult->AdrMode = ModAIX;
              break;
            default:
              WrError(ErrNum_InternalError);
              break;
          }
        }
      }

      /* 4.2 Variante d([...An...]...) */

      else
      {
        if (AdrComps[1].Art == None)
        {
          pResult->AdrPart = 0x30;
          ext_word += 0x80;
        }
        else
          pResult->AdrPart = 0x30 + AdrComps[1].ANummer;

        if (AdrComps[0].Art == None)
        {
          pResult->AdrMode = ModAIX;
          append_adr_vals_const(pResult, ext_word | 0x10);
        }
        else
        {
          HVal = EvalStrIntExpressionWithFlags(&AdrComps[0].Comp, Int32, &ValOK, &symbol_flags);
          if (!ValOK)
            return ModNone;
          switch (AdrComps[0].Size)
          {
            case eSymbolSizeUnknown:
              if (IsDisp16(HVal))
                goto AnIs16;
              else
                goto AnIs32;
            case eSymbolSize16Bit:
              if (!IsDisp16(HVal))
              {
                WrError(ErrNum_DistTooBig);
                return ModNone;
              }
            AnIs16:
              append_adr_vals_const(pResult, ext_word | 0x20);
              append_adr_vals_16(pResult, HVal & 0xffff, symbol_flags);
              pResult->AdrMode = ModAIX;
              break;
            case eSymbolSize32Bit:
            AnIs32:
              append_adr_vals_const(pResult, ext_word | 0x30);
              append_adr_vals_32(pResult, HVal, symbol_flags);
              pResult->AdrMode = ModAIX;
              break;
            default:
              WrError(ErrNum_InternalError);
              break;
          }
        }
      }

      /* aeusseres Displacement: */

      if (OutDisp.str.p_str[0])
        HVal = EvalStrIntExpressionWithFlags(&OutDisp, (OutDispLen == eSymbolSize16Bit) ? SInt16 : SInt32, &ValOK, &symbol_flags);
      else
      {
        HVal = 0;
        ValOK = True;
        symbol_flags = eSymbolFlag_None;
      }
      if (!ValOK)
      {
        ClrAdrVals(pResult);
        return ModNone;
      }
      if (OutDispLen == eSymbolSizeUnknown)
        OutDispLen = IsDisp16(HVal) ? eSymbolSize16Bit : eSymbolSize32Bit;
      if (*OutDisp.str.p_str == '\0')
      {
        pResult->Vals[0] |= 1;
        goto chk;
      }
      else
        switch (OutDispLen)
        {
          case eSymbolSize8Bit:
          case eSymbolSize16Bit:
            append_adr_vals_16(pResult, HVal & 0xffff, symbol_flags);
            pResult->Vals[0] |= 2;
            break;
          case eSymbolSize32Bit:
            append_adr_vals_32(pResult, HVal, symbol_flags);
            pResult->Vals[0] |= 3;
            break;
          default:
            WrError(ErrNum_InternalError);
            break;
        }

      goto chk;
    }
  }

  /* absolut: */

  else
  {
    if (!SplitSize(&Arg, &OutDispLen, 6))
      return ModNone;
    DecodeAbs(&Arg, OutDispLen, pResult);
  }

chk:
  if ((pResult->AdrMode > 0) && (!(Erl & (1 << (pResult->AdrMode - 1)))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    ClrAdrVals(pResult);
  }
  return pResult->AdrMode;
}

static Boolean DecodeMACACC(const char *pArg, Word *pResult)
{
  /* interprete ACC like ACC0, independent of MAC or EMAC: */

  if (!as_strcasecmp(pArg, "ACC"))
    *pResult = 0;
  else if (!as_strncasecmp(pArg, "ACC", 3) && (strlen(pArg) == 4) && (pArg[3] >= '0') && (pArg[3] <= '3'))
    *pResult = pArg[3] - '0';
  else
    return False;

  /* allow ACC1..3 only on EMAC: */

  if ((!(pCurrCPUProps->SuppFlags & eFlagEMAC)) && *pResult)
    return False;
  return True;
}

static Boolean DecodeMACReg(const char *pArg, Word *pResult)
{
  if (!as_strcasecmp(pArg, "MACSR"))
  {
    *pResult = 4;
    return True;
  }
  if (!as_strcasecmp(pArg, "MASK"))
  {
    *pResult = 6;
    return True;
  }

  /* ACCEXT01/23 only on EMAC: */

  if (pCurrCPUProps->SuppFlags & eFlagEMAC)
  {
    if (!as_strcasecmp(pArg, "ACCEXT01"))
    {
      *pResult = 5;
      return True;
    }
    if (!as_strcasecmp(pArg, "ACCEXT23"))
    {
      *pResult = 7;
      return True;
    }
  }
  return DecodeMACACC(pArg, pResult);
}

static Boolean DecodeRegList(const tStrComp *pArg, Word *Erg)
{
  Word h, h2;
  Byte z;
  char *p, *p2;
  String ArgStr;
  tStrComp Arg, Remainder, From, To;

  StrCompMkTemp(&Arg, ArgStr, sizeof(ArgStr));
  StrCompCopy(&Arg, pArg);

  *Erg = 0;
  do
  {
    p = strchr(Arg.str.p_str, '/');
    if (p)
      StrCompSplitRef(&Arg, &Remainder, &Arg, p);
    p2 = strchr(Arg.str.p_str, '-');
    if (!p2)
    {
      if (DecodeReg(&Arg, &h, False) != eIsReg)
        return False;
      *Erg |= 1 << h;
    }
    else
    {
      StrCompSplitRef(&From, &To, &Arg, p2);
      if (!*From.str.p_str || !*To.str.p_str)
        return False;
      if ((DecodeReg(&From, &h, False) != eIsReg)
       || (DecodeReg(&To, &h2, False) != eIsReg))
        return False;
      if (h <= h2)
      {
        for (z = h; z <= h2; z++)
          *Erg |= 1 << z;
      }
      else
      {
        for (z = h; z <= 15; z++)
          *Erg |= 1 << z;
        for (z = 0; z <= h2; z++)
          *Erg |= 1 << z;
      }
    }
    if (p)
      Arg = Remainder;
  }
  while (p);
  return True;
}

static Boolean DecodeMACScale(const tStrComp *pArg, Word *pResult, Word *p_guess_mask)
{
  int l = strlen(pArg->str.p_str);
  tStrComp ShiftArg;
  Boolean Left = False, OK;
  Word ShiftCnt;
  tSymbolFlags flags;

  /* allow empty argument */

  if (!l)
  {
    *pResult = 0;
    return True;
  }
  /* left or right? */

  if (l < 2)
    return False;
  if (!strncmp(pArg->str.p_str, "<<", 2))
    Left = True;
  else if (!strncmp(pArg->str.p_str, ">>", 2))
    Left = False;
  else
    return False;

  /* evaluate shift cnt - empty count counts as one */

  StrCompRefRight(&ShiftArg, pArg, 2);
  KillPrefBlanksStrCompRef(&ShiftArg);
  if (!*ShiftArg.str.p_str)
  {
    ShiftCnt = 1;
    OK = True;
  }
  else
    ShiftCnt = EvalStrIntExpressionWithFlags(&ShiftArg, UInt1, &OK, &flags);
  if (!OK)
    return False;

  /* codify */

  if (ShiftCnt)
    *pResult = Left ? 1 : 3;
  else
    *pResult = 0;
  *p_guess_mask = mFirstPassUnknownOrQuestionable(flags) ? 3 : 0;
  return True;
}

static Boolean SplitMACUpperLower(Word *pResult, tStrComp *pArg)
{
  char *pSplit;
  tStrComp HalfComp;

  *pResult = 0;
  pSplit = strrchr(pArg->str.p_str, '.');
  if (!pSplit)
  {
    WrStrErrorPos(ErrNum_InvReg, pArg);
    return False;
  }

  StrCompSplitRef(pArg, &HalfComp, pArg, pSplit);
  KillPostBlanksStrComp(pArg);
  if (!as_strcasecmp(HalfComp.str.p_str, "L"))
    *pResult = 0;
  else if (!as_strcasecmp(HalfComp.str.p_str, "U"))
    *pResult = 1;
  else
  {
    WrStrErrorPos(ErrNum_InvReg, &HalfComp);
    return False;
  }
  return True;
}

static Boolean SplitMACANDMASK(Word *pResult, tStrComp *pArg)
{
  char *pSplit, Save;
  tStrComp MaskComp, AddrComp;

  *pResult = 0;
  pSplit = strrchr(pArg->str.p_str, '&');
  if (!pSplit)
    return True;

  Save = StrCompSplitRef(&AddrComp, &MaskComp, pArg, pSplit);
  KillPrefBlanksStrCompRef(&MaskComp);

  /* if no MASK argument, be sure to revert pArg to original state: */

  if (!strcmp(MaskComp.str.p_str, "") || !as_strcasecmp(MaskComp.str.p_str, "MASK"))
  {
    KillPostBlanksStrComp(&AddrComp);
    *pArg = AddrComp;
    *pResult = 1;
  }
  else
    *pSplit = Save;
  return True;
}

/*-------------------------------------------------------------------------*/
/* Dekodierroutinen: Integer-Einheit */

/* 0=MOVE 1=MOVEA */

static void DecodeMOVE(Word Index)
{
  Word MACReg;
  unsigned Variant = Index & VariantMask;

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "USP"))
  {
    if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
    else if ((pCurrCPUProps->Family != eColdfire) || CheckISA((1 << eCfISA_APlus) | (1 << eCfISA_B) | (1 << eCfISA_C)))
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[2], MModAdr, &AdrResult))
      {
        CodeLen = 2;
        WAsmCode[0] = 0x4e68 | (AdrResult.AdrPart & 7);
        CheckSup();
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "USP"))
  {
    if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
    else if ((pCurrCPUProps->Family != eColdfire) || CheckISA((1 << eCfISA_APlus) | (1 << eCfISA_B) | (1 << eCfISA_C)))
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModAdr, &AdrResult))
      {
        CodeLen = 2;
        WAsmCode[0] = 0x4e60 | (AdrResult.AdrPart & 7);
        CheckSup();
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "SR"))
  {
    if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[2], MModData | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs), &AdrResult))
      {
        CodeLen = 2 + AdrResult.Cnt;
        WAsmCode[0] = 0x40c0 | AdrResult.AdrPart;
        CopyAdrVals(1, &AdrResult);
        if (pCurrCPUProps->Family != e68KGen1a)
          CheckSup();
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CCR"))
  {
    if (*AttrPart.str.p_str && (OpSize > eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
    else if (!CheckNoFamily(1 << e68KGen1a));
    else
    {
      tAdrResult AdrResult;

      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[2], MModData | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs), &AdrResult))
      {
        CodeLen = 2 + AdrResult.Cnt;
        WAsmCode[0] = 0x42c0 | AdrResult.AdrPart;
        CopyAdrVals(1, &AdrResult);
      }
    }
  }
  else if ((pCurrCPUProps->SuppFlags & eFlagMAC) && (DecodeMACReg(ArgStr[1].str.p_str, &MACReg)))
  {
    Word DestMACReg;

    if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
    else if ((MACReg == 4) && (!as_strcasecmp(ArgStr[2].str.p_str, "CCR")))
    {
      WAsmCode[0] = 0xa9c0;
      CodeLen = 2;
    }
    else if ((MACReg < 4) && DecodeMACReg(ArgStr[2].str.p_str, &DestMACReg) && (DestMACReg < 4) && (pCurrCPUProps->SuppFlags & eFlagEMAC))
    {
      WAsmCode[0] = 0xa110 | (DestMACReg << 9) | (MACReg << 0);
      CodeLen = 2;
    }
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
      {
        CodeLen = 2;
        WAsmCode[0] = 0xa180 | (AdrResult.AdrPart & 15) | (MACReg << 9);
      }
    }
  }
  else if ((pCurrCPUProps->SuppFlags & eFlagMAC) && (DecodeMACReg(ArgStr[2].str.p_str, &MACReg)))
  {
    if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModData | MModAdr | MModImm, &AdrResult))
      {
        CodeLen = 2 + AdrResult.Cnt;
        WAsmCode[0] = 0xa100 | (AdrResult.AdrPart) | (MACReg << 9);
        CopyAdrVals(1, &AdrResult);
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "SR"))
  {
    if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModData | MModImm | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs), &AdrResult))
      {
        CodeLen = 2 + AdrResult.Cnt;
        WAsmCode[0] = 0x46c0 | AdrResult.AdrPart;
        CopyAdrVals(1, &AdrResult);
        CheckSup();
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "CCR"))
  {
    if (*AttrPart.str.p_str && (OpSize > eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModData | MModImm | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs), &AdrResult))
      {
        CodeLen = 2 + AdrResult.Cnt;
        WAsmCode[0] = 0x44c0 | AdrResult.AdrPart;
        CopyAdrVals(1, &AdrResult);
      }
    }
  }
  else
  {
    if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
    else
    {
      tAdrResult AdrResult;

      DecodeAdr(&ArgStr[1], ((Variant == I_Variant) ? 0 : MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs) | MModImm, &AdrResult);

      /* Is An as source in byte mode allowed for ColdFire? No corresponding footnote in CFPRM... */

      if ((AdrResult.AdrMode == ModAdr) && (OpSize == eSymbolSize8Bit) && (pCurrCPUProps->Family != eColdfire)) WrError(ErrNum_InvOpSize);
      else if (AdrResult.AdrMode != ModNone)
      {
        unsigned SrcAdrNum = AdrResult.AdrMode;

        CodeLen = 2 + AdrResult.Cnt;
        CopyAdrVals(1, &AdrResult);
        if (OpSize == eSymbolSize8Bit)
          WAsmCode[0] = 0x1000;
        else if (OpSize == eSymbolSize16Bit)
          WAsmCode[0] = 0x3000;
        else
          WAsmCode[0] = 0x2000;
        WAsmCode[0] |= AdrResult.AdrPart;
        DecodeAdr(&ArgStr[2], ((Variant == A_Variant) ? 0 : MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs) | MModAdr, &AdrResult);
        if ((AdrResult.AdrMode == ModAdr) && (OpSize == eSymbolSize8Bit))
        {
          CodeLen = 0;
          WrError(ErrNum_InvOpSize);
        }
        else if (AdrResult.AdrMode == ModNone)
          CodeLen = 0;
        else
        {
          Boolean CombinationOK;

          /* ColdFire does not allow all combinations of src+dest: */

          if (pCurrCPUProps->Family == eColdfire)
            switch (SrcAdrNum)
            {
              case ModData: /* Dn */
              case ModAdr: /* An */
              case ModAdrI: /* (An) */
              case ModPost: /* (An)+ */
              case ModPre: /* -(An) */
                CombinationOK = True;
                break;
              case ModDAdrI: /* (d16,An) */
              case ModPC: /* (d16,PC) */
                CombinationOK = (AdrResult.AdrMode != ModAIX)   /* no (d8,An,Xi) */
                             && (AdrResult.AdrMode != ModAbs); /* no (xxx).W/L */
                break;
              case ModAIX: /* (d8,An,Xi) */
              case ModPCIdx: /* (d8,PC,Xi) */
              case ModAbs: /* (xxx).W/L */
                CombinationOK = (AdrResult.AdrMode != ModDAdrI)   /* no (d16,An) */
                             && (AdrResult.AdrMode != ModAIX)   /* no (d8,An,Xi) */
                             && (AdrResult.AdrMode != ModAbs); /* no (xxx).W/L */
                break;
              case ModImm: /* #xxx */
                if (AdrResult.AdrMode == ModDAdrI) /* (d16,An) OK for 8/16 bit starting with ISA B */
                  CombinationOK = (pCurrCPUProps->CfISA >= eCfISA_B) && (OpSize <= eSymbolSize16Bit);
                else
                  CombinationOK = (AdrResult.AdrMode != ModAIX)   /* no (d8,An,Xi) */
                               && (AdrResult.AdrMode != ModAbs); /* no (xxx).W/L */
                break;
              default: /* should not occur */
                CombinationOK = False;
            }
          else
            CombinationOK = True;
          if (!CombinationOK)
          {
            WrError(ErrNum_InvAddrMode);
            CodeLen = 0;
          }
          else
          {
            AdrResult.AdrPart = ((AdrResult.AdrPart & 7) << 3) | (AdrResult.AdrPart >> 3);
            WAsmCode[0] |= AdrResult.AdrPart << 6;
            CopyAdrVals(CodeLen >> 1, &AdrResult);
            CodeLen += AdrResult.Cnt;
          }
        }
      }
    }
  }
}

static void DecodeLEA(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (!ChkArgCnt(2, 2));
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModAdr, &AdrResult))
    {
      OpSize = eSymbolSize8Bit;
      WAsmCode[0] = 0x41c0 | ((AdrResult.AdrPart & 7) << 9);
      if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
      {
        WAsmCode[0] |= AdrResult.AdrPart;
        CodeLen = 2 + AdrResult.Cnt;
        CopyAdrVals(1, &AdrResult);
      }
    }
  }
}

/* 0=ASR 1=ASL 2=LSR 3=LSL 4=ROXR 5=ROXL 6=ROR 7=ROL */

static void DecodeShift(Word Index)
{
  Boolean ValOK;
  Byte HVal8;
  Word LFlag = (Index >> 2), Op = Index & 3;

  if (!ChkArgCnt(1, 2));
  else if ((*OpPart.str.p_str == 'R') && (!CheckNoFamily(1 << eColdfire)));
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[ArgCnt], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult) == ModData)
    {
      if (CheckColdSize())
      {
        WAsmCode[0] = 0xe000 | AdrResult.AdrPart | (Op << 3) | (OpSize << 6) | (LFlag << 8);
        OpSize = eSymbolSizeShiftCnt;
        if (ArgCnt == 2)
          DecodeAdr(&ArgStr[1], MModData | MModImm, &AdrResult);
        else
        {
          AdrResult.AdrMode = ModImm;
          AdrResult.Vals[0] = 1;
        }
        if ((AdrResult.AdrMode == ModData) || ((AdrResult.AdrMode == ModImm) && (Lo(AdrResult.Vals[0]) >= 1) && (Lo(AdrResult.Vals[0]) <= 8)))
        {
          CodeLen = 2;
          WAsmCode[0] |= (AdrResult.AdrMode == ModData) ? 0x20 | (AdrResult.AdrPart << 9) : ((AdrResult.Vals[0] & 7) << 9);
          or_wasmcode_guessed(0, 1, (AdrResult.guess_masks[0] & 7) << 9);
        }
        else
          WrStrErrorPos(ErrNum_InvShiftArg, &ArgStr[1]);
      }
    }
    else if (AdrResult.AdrMode != ModNone)
    {
      if (pCurrCPUProps->Family == eColdfire) WrError(ErrNum_InvAddrMode);
      else
      {
        if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
        else
        {
          WAsmCode[0] = 0xe0c0 | AdrResult.AdrPart | (Op << 9) | (LFlag << 8);
          CopyAdrVals(1, &AdrResult);
          if (2 == ArgCnt)
          {
            HVal8 = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), Int8, &ValOK);
          }
          else
          {
            HVal8 = 1;
            ValOK = True;
          }
          if ((ValOK) && (HVal8 == 1))
            CodeLen = 2 + AdrResult.Cnt;
          else
            WrStrErrorPos(ErrNum_Only1, &ArgStr[1]);
        }
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeADDQSUBQ(Word Index)
 * \brief  Handle ADDQ/SUBQ Instructions
 * \param  Index ADDQ=0 SUBQ=1
 * ------------------------------------------------------------------------ */

static void DecodeADDQSUBQ(Word Index)
{
  LongWord ImmVal;
  Boolean ValOK;
  tSymbolFlags Flags;
  tAdrResult AdrResult;

  if (!CheckColdSize())
    return;

  if (!ChkArgCnt(2, 2))
    return;

  if (!DecodeAdr(&ArgStr[2], MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    return;

  if ((ModAdr == AdrResult.AdrMode) && (eSymbolSize8Bit == OpSize))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }

  WAsmCode[0] = 0x5000 | AdrResult.AdrPart | (OpSize << 6) | (Index << 8);
  CopyAdrVals(1, &AdrResult);
  ImmVal = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), UInt32, &ValOK, &Flags);
  if (mFirstPassUnknownOrQuestionable(Flags))
    ImmVal = 1;
  if (ValOK && ((ImmVal < 1) || (ImmVal > 8)))
  {
    WrError(ErrNum_Range18);
    ValOK = False;
  }
  if (ValOK)
  {
    CodeLen = 2 + AdrResult.Cnt;
    WAsmCode[0] |= (ImmVal & 7) << 9;
    or_w_guessed(Flags, 0, 1, 7 << 9);
  }
}

/* 0=SUBX 1=ADDX */

static void DecodeADDXSUBX(Word Index)
{
  if (CheckColdSize())
  {
    if (ChkArgCnt(2, 2))
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModData | MModPre, &AdrResult))
      {
        WAsmCode[0] = 0x9100 | (OpSize << 6) | (AdrResult.AdrPart & 7) | (Index << 14);
        if (AdrResult.AdrMode == ModPre)
          WAsmCode[0] |= 8;
        if (DecodeAdr(&ArgStr[2], 1 << (AdrResult.AdrMode - 1), &AdrResult))
        {
          CodeLen = 2;
          WAsmCode[0] |= (AdrResult.AdrPart & 7) << 9;
        }
      }
    }
  }
}

static void DecodeCMPM(Word Index)
{
  UNUSED(Index);

  if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckNoFamily(1 << eColdfire))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModPost, &AdrResult) == ModPost)
    {
      WAsmCode[0] = 0xb108 | (OpSize << 6) | (AdrResult.AdrPart & 7);
      if (DecodeAdr(&ArgStr[2], MModPost, &AdrResult) == ModPost)
      {
        WAsmCode[0] |= (AdrResult.AdrPart & 7) << 9;
        CodeLen = 2;
      }
    }
  }
}

/* 0=SUB 1=CMP 2=ADD +4=..I +8=..A */

static void DecodeADDSUBCMP(Word Index)
{
  Word Op = Index & 3, Reg;
  unsigned Variant = Index & VariantMask;
  Word DestMask, SrcMask;
  Boolean OpSizeOK;

  if (I_Variant == Variant)
    SrcMask = MModImm;
  else
    SrcMask = MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm;

  if (A_Variant == Variant)
    DestMask = MModAdr;
  else
  {
    DestMask = MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs;

    /* Since CMP only reads operands, PC-relative addressing is also
       allowed for the second operand on 68020++ */

    if ((as_toupper(*OpPart.str.p_str) == 'C')
     && (pCurrCPUProps->Family > e68KGen1b))
      DestMask |= MModPC | MModPCIdx;
  }

  /* ColdFire ISA B ff. allows 8/16 bit operand size of CMP: */

  if (OpSize > eSymbolSize32Bit)
    OpSizeOK = False;
  else if (OpSize == eSymbolSize32Bit)
    OpSizeOK = True;
  else
    OpSizeOK = (pCurrCPUProps->Family != eColdfire)
            || ((pCurrCPUProps->CfISA >= eCfISA_B) && (Op == 1));

  if (!OpSizeOK) WrError(ErrNum_InvOpSize);
  else
  {
    if (ChkArgCnt(2, 2))
    {
      tAdrResult AdrResult;

      switch (DecodeAdr(&ArgStr[2], DestMask, &AdrResult))
      {
        case ModAdr: /* ADDA/SUBA/CMPA ? */
          if (OpSize == eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
          else
          {
            WAsmCode[0] = 0x90c0 | ((AdrResult.AdrPart & 7) << 9) | (Op << 13);
            if (OpSize == eSymbolSize32Bit) WAsmCode[0] |= 0x100;
            if (DecodeAdr(&ArgStr[1], SrcMask, &AdrResult))
            {
              WAsmCode[0] |= AdrResult.AdrPart;
              CodeLen = 2 + AdrResult.Cnt;
              CopyAdrVals(1, &AdrResult);
            }
          }
          break;

        case ModData: /* ADD/SUB/CMP <ea>,Dn ? */
          WAsmCode[0] = 0x9000 | (OpSize << 6) | ((Reg = AdrResult.AdrPart) << 9) | (Op << 13);
          DecodeAdr(&ArgStr[1], SrcMask, &AdrResult);

          /* CMP.B An,Dn allowed for Coldfire? */

          if ((AdrResult.AdrMode == ModAdr) && (OpSize == eSymbolSize8Bit) && (pCurrCPUProps->Family != eColdfire)) WrError(ErrNum_InvOpSize);
          if (AdrResult.AdrMode != ModNone)
          {
            if ((AdrResult.AdrMode == ModImm) && (Variant == I_Variant))
            {
              if (Op == 1) Op = 8;
              WAsmCode[0] = 0x400 | (OpSize << 6) | (Op << 8) | Reg;
            }
            else
              WAsmCode[0] |= AdrResult.AdrPart;
            CopyAdrVals(1, &AdrResult);
            CodeLen = 2 + AdrResult.Cnt;
          }
          break;

        case ModNone:
          break;

        default: /* CMP/ADD/SUB <ea>, Dn */
          if (DecodeAdr(&ArgStr[1], MModData | MModImm, &AdrResult) == ModImm)        /* ADDI/SUBI/CMPI ? */
          {
            /* we have to set the PC offset before we decode the destination operand.  Luckily,
               this is only needed afterwards for an immediate source operand, so we know the
               # of words ahead: */

            if (*ArgStr[1].str.p_str == '#')
              RelPos += (OpSize == eSymbolSize32Bit) ? 4 : 2;

            if (Op == 1) Op = 8;
            WAsmCode[0] = 0x400 | (OpSize << 6) | (Op << 8);
            CodeLen = 2 + AdrResult.Cnt;
            CopyAdrVals(1, &AdrResult);
            if (DecodeAdr(&ArgStr[2], (pCurrCPUProps->Family == eColdfire) ? (Word)MModData : DestMask, &AdrResult))
            {
              WAsmCode[0] |= AdrResult.AdrPart;
              CopyAdrVals(CodeLen >> 1, &AdrResult);
              CodeLen += AdrResult.Cnt;
            }
            else
              CodeLen = 0;
          }
          else if (AdrResult.AdrMode != ModNone)    /* ADD Dn,<EA> ? */
          {
            if (Op == 1) WrError(ErrNum_InvCmpMode);
            else
            {
              WAsmCode[0] = 0x9100 | (OpSize << 6) | (AdrResult.AdrPart << 9) | (Op << 13);
              if (DecodeAdr(&ArgStr[2], DestMask, &AdrResult))
              {
                CodeLen = 2 + AdrResult.Cnt;
                CopyAdrVals(1, &AdrResult);
                WAsmCode[0] |= AdrResult.AdrPart;
              }
            }
          }
      }
    }
  }
}

/* 0=OR 1=AND +4=..I */

static void DecodeANDOR(Word Index)
{
  Word Op = Index & 3, Reg;
  char Variant = Index & VariantMask;
  tAdrResult AdrResult;

  if (!ChkArgCnt(2, 2));
  else if (CheckColdSize())
  {
    if (!as_strcasecmp(ArgStr[2].str.p_str, "CCR"))     /* AND #...,CCR */
    {
      if (*AttrPart.str.p_str && (OpSize != eSymbolSize8Bit)) WrError(ErrNum_InvOpSize);
      else if (!(pCurrCPUProps->SuppFlags & eFlagLogCCR)) WrError(ErrNum_InstructionNotSupported);
      {
        WAsmCode[0] = 0x003c | (Op << 9);
        OpSize = eSymbolSize8Bit;
        if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
        {
          CodeLen = 4;
          WAsmCode[1] = AdrResult.Vals[0];
        }
      }
    }
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "SR")) /* AND #...,SR */
    {
      if (*AttrPart.str.p_str && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
      else if (CheckNoFamily(1 << eColdfire))
      {
        WAsmCode[0] = 0x007c | (Op << 9);
        OpSize = eSymbolSize16Bit;
        if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
        {
          CodeLen = 4;
          WAsmCode[1] = AdrResult.Vals[0];
          CheckSup();
        }
      }
    }
    else
    {
      DecodeAdr(&ArgStr[2], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult);
      if (AdrResult.AdrMode == ModData)                 /* AND <EA>,Dn */
      {
        Reg = AdrResult.AdrPart;
        WAsmCode[0] = 0x8000 | (OpSize << 6) | (Reg << 9) | (Op << 14);
        if (DecodeAdr(&ArgStr[1], ((Variant == I_Variant) ? 0 : MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs) | MModImm, &AdrResult))
        {
          if ((AdrResult.AdrMode == ModImm) && (Variant == I_Variant))
            WAsmCode[0] = (OpSize << 6) | (Op << 9) | Reg;
          else
            WAsmCode[0] |= AdrResult.AdrPart;
          CodeLen = 2 + AdrResult.Cnt;
          CopyAdrVals(1, &AdrResult);
        }
      }
      else if (AdrResult.AdrMode != ModNone)                 /* AND ...,<EA> */
      {
        if (DecodeAdr(&ArgStr[1], MModData | MModImm, &AdrResult) == ModImm)                   /* AND #..,<EA> */
        {
          WAsmCode[0] = (OpSize << 6) | (Op << 9);
          CodeLen = 2 + AdrResult.Cnt;
          CopyAdrVals(1, &AdrResult);
          if (DecodeAdr(&ArgStr[2], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
          {
            WAsmCode[0] |= AdrResult.AdrPart;
            CopyAdrVals(CodeLen >> 1, &AdrResult);
            CodeLen += AdrResult.Cnt;
          }
          else
            CodeLen = 0;
        }
        else if (AdrResult.AdrMode != ModNone)               /* AND Dn,<EA> ? */
        {
          WAsmCode[0] = 0x8100 | (OpSize << 6) | (AdrResult.AdrPart << 9) | (Op << 14);
          if (DecodeAdr(&ArgStr[2], MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
          {
            CodeLen = 2 + AdrResult.Cnt;
            CopyAdrVals(1, &AdrResult);
            WAsmCode[0] |= AdrResult.AdrPart;
          }
        }
      }
    }
  }
}

/* 0=EOR 4=EORI */

static void DecodeEOR(Word Index)
{
  unsigned Variant = Index | VariantMask;
  tAdrResult AdrResult;

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "CCR"))
  {
    if (*AttrPart.str.p_str && (OpSize != eSymbolSize8Bit)) WrError(ErrNum_InvOpSize);
    else if (CheckNoFamily(1 << eColdfire))
    {
      WAsmCode[0] = 0xa3c;
      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
      {
        CodeLen = 4;
        WAsmCode[1] = AdrResult.Vals[0];
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "SR"))
  {
    if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else if (CheckNoFamily(1 << eColdfire))
    {
      WAsmCode[0] = 0xa7c;
      if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
      {
        CodeLen = 4;
        WAsmCode[1] = AdrResult.Vals[0];
        CheckSup();
      }
    }
  }
  else if (CheckColdSize())
  {
    if (DecodeAdr(&ArgStr[1], ((Variant == I_Variant) ? 0 : MModData) | MModImm, &AdrResult) == ModData)
    {
      WAsmCode[0] = 0xb100 | (AdrResult.AdrPart << 9) | (OpSize << 6);
      if (DecodeAdr(&ArgStr[2], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
      {
        CodeLen = 2 + AdrResult.Cnt;
        CopyAdrVals(1, &AdrResult);
        WAsmCode[0] |= AdrResult.AdrPart;
      }
    }
    else if (AdrResult.AdrMode == ModImm)
    {
      WAsmCode[0] = 0x0a00 | (OpSize << 6);
      CopyAdrVals(1, &AdrResult);
      CodeLen = 2 + AdrResult.Cnt;
      if (DecodeAdr(&ArgStr[2], MModData | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs), &AdrResult))
      {
        CopyAdrVals(CodeLen >> 1, &AdrResult);
        CodeLen += AdrResult.Cnt;
        WAsmCode[0] |= AdrResult.AdrPart;
      }
      else CodeLen = 0;
    }
  }
}

static void DecodePEA(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0x4840 | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
    }
  }
}

static void DecodeCLRTST(Word IsTST)
{
  if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1))
  {
    Word w1 = MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs;
    tAdrResult AdrResult;

    switch (pCurrCPUProps->Family)
    {
      case eCPU32:
      case e68KGen2:
      case e68KGen3:
        if (IsTST)
        {
          w1 |= MModPC | MModPCIdx | MModImm;
          if (OpSize != eSymbolSize8Bit)
            w1 |= MModAdr;
        }
        break;
      default:
        break;
    }
    if (DecodeAdr(&ArgStr[1], w1, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0x4200 | (IsTST << 11) | (OpSize << 6) | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
    }
  }
}

/* 0=JSR 1=JMP */

static void DecodeJSRJMP(Word Index)
{
  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0x4e80 | (Index << 6) | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
    }
  }
}

/* 0=TAS 1=NBCD */

static void DecodeNBCDTAS(Word Index)
{
  Boolean Allowed;

  /* TAS is allowed on ColdFire ISA B ff. ... */

  if (pCurrCPUProps->Family != eColdfire)
    Allowed = True;
  else
    Allowed = Index ? False : (pCurrCPUProps->CfISA >= eCfISA_B);

  if (*AttrPart.str.p_str && (OpSize != eSymbolSize8Bit)) WrError(ErrNum_InvOpSize);
  else if (!Allowed) WrError(ErrNum_InstructionNotSupported);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;

    /* ...but not on data register: */

    if (DecodeAdr(&ArgStr[1], ((pCurrCPUProps->Family == eColdfire) ? 0 : MModData) | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = (Index == 1) ? 0x4800 : 0x4ac0;
      WAsmCode[0] |= AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
    }
  }
}

/* 0=NEGX 2=NEG 3=NOT */

static void DecodeNEGNOT(Word Index)
{
  if (ChkArgCnt(1, 1)
   && CheckColdSize())
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], (pCurrCPUProps->Family == eColdfire) ? MModData : (MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs), &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0x4000 | (Index << 9) | (OpSize << 6) | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
    }
  }
}

static void DecodeSWAP(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
    {
      CodeLen = 2;
      WAsmCode[0] = 0x4840 | AdrResult.AdrPart;
    }
  }
}

static void DecodeUNLK(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdr, &AdrResult))
    {
      CodeLen = 2;
      WAsmCode[0] = 0x4e58 | AdrResult.AdrPart;
    }
  }
}

static void DecodeEXT(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if ((OpSize == eSymbolSize8Bit) || (OpSize > eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData, &AdrResult) == ModData)
    {
      WAsmCode[0] = 0x4880 | AdrResult.AdrPart | (((Word)OpSize - 1) << 6);
      CodeLen = 2;
    }
  }
}

static void DecodeWDDATA(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (CheckFamily(1 << eColdfire))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      WAsmCode[0] = 0xf400 + (OpSize << 6) + AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
      CodeLen = 2 + AdrResult.Cnt;
      CheckSup();
    }
  }
}

static void DecodeWDEBUG(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && CheckFamily(1 << eColdfire)
   && CheckColdSize())
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI, &AdrResult))
    {
      WAsmCode[0] = 0xfbc0 + AdrResult.AdrPart;
      WAsmCode[1] = 0x0003;
      CopyAdrVals(2, &AdrResult);
      CodeLen = 4 + AdrResult.Cnt;
      CheckSup();
    }
  }
}

static void DecodeFixed(Word Index)
{
  FixedOrder *FixedZ = FixedOrders + Index;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(0, 0)
        && CheckFamily(FixedZ->FamilyMask))
  {
    CodeLen = 2;
    WAsmCode[0] = FixedZ->Code;
    if (FixedZ->MustSup)
      CheckSup();
  }
}

static void DecodeMOVEM(Word Index)
{
  int z;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if ((OpSize < eSymbolSize16Bit) || (OpSize > eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if ((pCurrCPUProps->Family == eColdfire) && (OpSize == 1)) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    RelPos = 4;
    if (DecodeRegList(&ArgStr[2], WAsmCode + 1))
    {
      if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModPost | MModAIX | MModPC | MModPCIdx | MModAbs), &AdrResult))
      {
        WAsmCode[0] = 0x4c80 | AdrResult.AdrPart | ((OpSize - 1) << 6);
        CodeLen = 4 + AdrResult.Cnt; CopyAdrVals(2, &AdrResult);
      }
    }
    else if (DecodeRegList(&ArgStr[1], WAsmCode + 1))
    {
      if (DecodeAdr(&ArgStr[2], MModAdrI | MModDAdrI  | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModPre | MModAIX | MModAbs), &AdrResult))
      {
        WAsmCode[0] = 0x4880 | AdrResult.AdrPart | ((OpSize - 1) << 6);
        CodeLen = 4 + AdrResult.Cnt; CopyAdrVals(2, &AdrResult);
        if (AdrResult.AdrMode == ModPre)
        {
          Word Tmp = WAsmCode[1];

          WAsmCode[1] = 0;
          for (z = 0; z < 16; z++)
          {
            WAsmCode[1] = (WAsmCode[1] << 1) + (Tmp & 1);
            Tmp >>= 1;
          }
        }
      }
    }
    else WrError(ErrNum_InvRegList);
  }
}

static void DecodeMOVEQ(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if ((*AttrPart.str.p_str) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (*ArgStr[1].str.p_str != '#') WrStrErrorPos(ErrNum_OnlyImmAddr, &ArgStr[1]);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModData, &AdrResult))
    {
      Boolean OK;
      tSymbolFlags Flags;
      LongWord Value;

      WAsmCode[0] = 0x7000 | (AdrResult.AdrPart << 9);
      Value = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, Int32, &OK, &Flags);
      if (mFirstPassUnknown(Flags))
        Value &= 0x7f;
      if ((Value > 0xff) && (Value < 0xffffff80ul))
        WrStrErrorPos((Value & 0x80000000ul) ? ErrNum_UnderRange : ErrNum_OverRange, &ArgStr[1]);
      else
      {
        if ((Value >= 0x80) && (Value <= 0xff))
        {
          char str[40];
          LargeWord v1 = Value, v2 = Value | 0xffffff00ul;

          as_snprintf(str, sizeof(str), "%llx -> %llx", v1, v2);
          WrXErrorPos(ErrNum_SignExtension, str, &ArgStr[1].Pos);
        }
        CodeLen = 2;
        WAsmCode[0] |= Value & 0xff;
        set_w_guessed(Flags, 0, 1, 0x00ff);
      }
    }
  }
}

static void DecodeSTOP(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean ValOK;
    tSymbolFlags flags;
    Word HVal = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, Int16, &ValOK, &flags);
    if (ValOK)
    {
      CodeLen = 4;
      WAsmCode[0] = 0x4e72;
      WAsmCode[1] = HVal;
      set_w_guessed(flags, 1, 1, 0xffff);
      CheckSup();
    }
  }
}

static void DecodeLPSTOP(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (!CheckFamily(1 << eCPU32));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean ValOK;
    tSymbolFlags flags;
    Word HVal = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, Int16, &ValOK, &flags);
    if (ValOK)
    {
      CodeLen = 6;
      WAsmCode[0] = 0xf800;
      WAsmCode[1] = 0x01c0;
      WAsmCode[2] = HVal;
      set_w_guessed(flags, 2, 1, 0xffff);
      CheckSup();
    }
  }
}

static void DecodeTRAP(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean ValOK;
    tSymbolFlags flags;
    Byte HVal8 = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, Int4, &ValOK, &flags);
    if (ValOK)
    {
      CodeLen = 2;
      WAsmCode[0] = 0x4e40 + (HVal8 & 15);
      set_w_guessed(flags, 0, 1, 0x000f);
    }
  }
}

static void DecodeBKPT(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (!CheckNoFamily((1 << e68KGen1a) | (1 << eColdfire)));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean ValOK;
    tSymbolFlags flags;
    Byte HVal8 = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, UInt3, &ValOK, &flags);
    if (ValOK)
    {
      CodeLen = 2;
      WAsmCode[0] = 0x4848 + (HVal8 & 7);
      set_w_guessed(flags, 0, 1, 0x0007);
    }
  }
  UNUSED(Index);
}

static void DecodeRTD(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (!CheckNoFamily((1 << e68KGen1a) | (1 << eColdfire)));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean ValOK;
    tSymbolFlags flags;
    Word HVal = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, Int16, &ValOK, &flags);
    if (ValOK)
    {
      CodeLen = 4;
      WAsmCode[0] = 0x4e74;
      WAsmCode[1] = HVal;
      set_w_guessed(flags, 1, 1, 0xffff);
    }
  }
}

static void DecodeEXG(Word Index)
{
  Word HReg;
  UNUSED(Index);

  if ((*AttrPart.str.p_str) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckNoFamily(1 << eColdfire))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData | MModAdr, &AdrResult) == ModData)
    {
      WAsmCode[0] = 0xc100 | (AdrResult.AdrPart << 9);
      if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult) == ModData)
      {
        WAsmCode[0] |= 0x40 | AdrResult.AdrPart;
        CodeLen = 2;
      }
      else if (AdrResult.AdrMode == ModAdr)
      {
        WAsmCode[0] |= 0x88 | (AdrResult.AdrPart & 7);
        CodeLen = 2;
      }
    }
    else if (AdrResult.AdrMode == ModAdr)
    {
      WAsmCode[0] = 0xc100;
      HReg = AdrResult.AdrPart & 7;
      if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult) == ModData)
      {
        WAsmCode[0] |= 0x88 | (AdrResult.AdrPart << 9) | HReg;
        CodeLen = 2;
      }
      else
      {
        WAsmCode[0] |= 0x48 | (HReg << 9) | (AdrResult.AdrPart & 7);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeMOVE16(Word Index)
{
  Word z, z2, w1, w2;
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2)
        && CheckFamily(1 << e68KGen3))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModPost | MModAdrI | MModAbs, &AdrResult))
    {
      w1 = AdrResult.AdrMode;
      z = AdrResult.AdrPart & 7;
      if ((w1 == ModAbs) && (AdrResult.Cnt == 2))
      {
        AdrResult.Vals[1] = AdrResult.Vals[0];
        AdrResult.Vals[0] = 0 - (AdrResult.Vals[1] >> 15);
      }
      if (DecodeAdr(&ArgStr[2], MModPost | MModAdrI | MModAbs, &AdrResult))
      {
        w2 = AdrResult.AdrMode;
        z2 = AdrResult.AdrPart & 7;
        if ((w2 == ModAbs) && (AdrResult.Cnt == 2))
        {
          AdrResult.Vals[1] = AdrResult.Vals[0];
          AdrResult.Vals[0] = 0 - (AdrResult.Vals[1] >> 15);
        }
        if ((w1 == ModPost) && (w2 == ModPost))
        {
          WAsmCode[0] = 0xf620 + z;
          WAsmCode[1] = 0x8000 + (z2 << 12);
          CodeLen = 4;
        }
        else
        {
          WAsmCode[1] = AdrResult.Vals[0];
          WAsmCode[2] = AdrResult.Vals[1];
          CodeLen = 6;
          if ((w1 == ModPost) && (w2 == ModAbs))
            WAsmCode[0] = 0xf600 + z;
          else if ((w1 == ModAbs) && (w2 == ModPost))
            WAsmCode[0] = 0xf608 + z2;
          else if ((w1 == ModAdrI) && (w2 == ModAbs))
            WAsmCode[0] = 0xf610 + z;
          else if ((w1 == ModAbs) && (w2 == ModAdrI))
            WAsmCode[0] = 0xf618 + z2;
          else
          {
            WrError(ErrNum_InvAddrMode);
            CodeLen = 0;
          }
        }
      }
    }
  }
}

static void DecodeCacheAll(Word Index)
{
  Word w1;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(1, 1));
  else if (!CheckFamily(1 << e68KGen3));
  else if (!CodeCache(ArgStr[1].str.p_str, &w1)) WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
  else
  {
    WAsmCode[0] = 0xf418 + (w1 << 6) + (Index << 5);
    CodeLen = 2;
    CheckSup();
  }
}

static void DecodeCache(Word Index)
{
  Word w1;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkArgCnt(2, 2));
  else if (!CheckFamily(1 << e68KGen3));
  else if (!CodeCache(ArgStr[1].str.p_str, &w1)) WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModAdrI, &AdrResult))
    {
      WAsmCode[0] = 0xf400 + (w1 << 6) + (Index << 3) + (AdrResult.AdrPart & 7);
      CodeLen = 2;
      CheckSup();
    }
  }
}

static void DecodeMUL_DIV(Word Code)
{
  tAdrResult AdrResult;
  Boolean is_mul = !(Code & 1);

  if (!ChkArgCnt(2, 2));
  else if (!is_mul && !CheckNoFamily(1 << eColdfire));
  else if (OpSize == eSymbolSize16Bit)
  {
    if (DecodeAdr(&ArgStr[2], MModData, &AdrResult))
    {
      WAsmCode[0] = 0x80c0 | (AdrResult.AdrPart << 9) | (Code & 0x0100);
      if (!(Code & 1))
        WAsmCode[0] |= 0x4000;
      if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm, &AdrResult))
      {
        WAsmCode[0] |= AdrResult.AdrPart;
        CodeLen = 2 + AdrResult.Cnt; CopyAdrVals(1, &AdrResult);
      }
    }
  }
  else if (OpSize == eSymbolSize32Bit)
  {
    Word dh, dl;
    Boolean dest_reg_pair = !!strchr(ArgStr[2].str.p_str, ':');

    if (dest_reg_pair)
    {
      if (!DecodeRegPair(&ArgStr[2], &dh, &dl))
      {
        WrStrErrorPos(ErrNum_InvRegPair, &ArgStr[2]);
        return;
      }
      if ((dh == dl) && is_mul)
        WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[2]);
    }
    else
    {
      if (!DecodeReg(&ArgStr[2], &dl, True) || (dl >= 8))
      {
        WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
        return;
      }
      dh = dl;
    }
    WAsmCode[1] = (dl << 12) | ((Code & 0x0100) << 3);
    if (dest_reg_pair)
      WAsmCode[1] |= 0x400;
    if (dest_reg_pair || !is_mul)
      WAsmCode[1] |= dh;
    RelPos = 4;
    if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm, &AdrResult))
    {
      WAsmCode[0] = 0x4c00 + AdrResult.AdrPart + (Lo(Code) << 6);
      CopyAdrVals(2, &AdrResult);
      CodeLen = 4 + AdrResult.Cnt;
      CheckFamily((1 << e68KGen3) | (1 << e68KGen2) | (1 << eCPU32) | (dest_reg_pair ? 0 : (1 << eColdfire)));
    }
  }
  else
    WrError(ErrNum_InvOpSize);
}

static void DecodeDIVL(Word Index)
{
  Word w1, w2;

  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize32Bit;
  if (!ChkArgCnt(2, 2));
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2) | (1 << eCPU32)));
  else if (OpSize != eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (!DecodeRegPair(&ArgStr[2], &w1, &w2)) WrStrErrorPos(ErrNum_InvRegPair, &ArgStr[2]);
  else
  {
    tAdrResult AdrResult;

    RelPos = 4;
    WAsmCode[1] = w1 | (w2 << 12) | (Index << 11);
    if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm, &AdrResult))
    {
      WAsmCode[0] = 0x4c40 + AdrResult.AdrPart;
      CopyAdrVals(2, &AdrResult);
      CodeLen = 4 + AdrResult.Cnt;
    }
  }
}

static void DecodeASBCD(Word Index)
{
  if ((OpSize != eSymbolSize8Bit) && *AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckNoFamily(1 << eColdfire))
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModData | MModPre, &AdrResult))
    {
      WAsmCode[0] = 0x8100 | (AdrResult.AdrPart & 7) | (Index << 14) | ((AdrResult.AdrMode == ModPre) ? 8 : 0);
      if (DecodeAdr(&ArgStr[2], 1 << (AdrResult.AdrMode - 1), &AdrResult))
      {
        CodeLen = 2;
        WAsmCode[0] |= (AdrResult.AdrPart & 7) << 9;
      }
    }
  }
}

static void DecodeCHK(Word Index)
{
  UNUSED(Index);

  if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckNoFamily(1 << eColdfire))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm, &AdrResult))
    {
      WAsmCode[0] = 0x4000 | AdrResult.AdrPart | ((4 - OpSize) << 7);
      CodeLen = 2 + AdrResult.Cnt;
      CopyAdrVals(1, &AdrResult);
      if (DecodeAdr(&ArgStr[2], MModData, &AdrResult) == ModData)
        WAsmCode[0] |= WAsmCode[0] | (AdrResult.AdrPart << 9);
      else
        CodeLen = 0;
    }
  }
}

static void DecodeLINK(Word Index)
{
  UNUSED(Index);

  if (!*AttrPart.str.p_str && (pCurrCPUProps->Family == eColdfire)) OpSize = eSymbolSize16Bit;
  if ((OpSize < 1) || (OpSize > 2)) WrError(ErrNum_InvOpSize);
  else if ((OpSize == eSymbolSize32Bit) && !CheckFamily((1 << eCPU32) | (1 << e68KGen2) | (1 << e68KGen3)));
  else if (ChkArgCnt(2, 2))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdr, &AdrResult))
    {
      WAsmCode[0] = (OpSize == eSymbolSize16Bit) ? 0x4e50 : 0x4808;
      WAsmCode[0] += AdrResult.AdrPart & 7;
      if (DecodeAdr(&ArgStr[2], MModImm, &AdrResult) == ModImm)
      {
        CodeLen = 2 + AdrResult.Cnt;
        memcpy(WAsmCode + 1, AdrResult.Vals, AdrResult.Cnt);
      }
    }
  }
}

static void DecodeMOVEP(Word Index)
{
  UNUSED(Index);

  if ((OpSize == eSymbolSize8Bit) || (OpSize > eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckNoFamily(1 << eColdfire))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData | MModDAdrI, &AdrResult) == ModData)
    {
      WAsmCode[0] = 0x188 | ((OpSize - 1) << 6) | (AdrResult.AdrPart << 9);
      if (DecodeAdr(&ArgStr[2], MModDAdrI, &AdrResult) == ModDAdrI)
      {
        WAsmCode[0] |= AdrResult.AdrPart & 7;
        CodeLen = 4;
        WAsmCode[1] = AdrResult.Vals[0];
      }
    }
    else if (AdrResult.AdrMode == ModDAdrI)
    {
      WAsmCode[0] = 0x108 | ((OpSize - 1) << 6) | (AdrResult.AdrPart & 7);
      WAsmCode[1] = AdrResult.Vals[0];
      if (DecodeAdr(&ArgStr[2], MModData, &AdrResult) == ModData)
      {
        WAsmCode[0] |= (AdrResult.AdrPart & 7) << 9;
        CodeLen = 4;
      }
    }
  }
}

static void DecodeMOVEC(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2))
  {
    tAdrResult AdrResult;

    if (DecodeCtrlReg(ArgStr[1].str.p_str, WAsmCode + 1))
    {
      if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
      {
        CodeLen = 4;
        WAsmCode[0] = 0x4e7a;
        WAsmCode[1] |= AdrResult.AdrPart << 12;
        CheckSup();
      }
    }
    else if (DecodeCtrlReg(ArgStr[2].str.p_str, WAsmCode + 1))
    {
      if (DecodeAdr(&ArgStr[1], MModData | MModAdr, &AdrResult))
      {
        CodeLen = 4;
        WAsmCode[0] = 0x4e7b;
        WAsmCode[1] |= AdrResult.AdrPart << 12; CheckSup();
      }
    }
    else
      WrError(ErrNum_InvCtrlReg);
  }
}

static void DecodeMOVES(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (CheckNoFamily((1 << e68KGen1a) | (1 << eColdfire)))
  {
    tAdrResult AdrResult;

    switch (DecodeAdr(&ArgStr[1], MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      case ModData:
      case ModAdr:
      {
        WAsmCode[1] = 0x800 | (AdrResult.AdrPart << 12);
        if (DecodeAdr(&ArgStr[2], MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
        {
          WAsmCode[0] = 0xe00 | AdrResult.AdrPart | (OpSize << 6);
          CodeLen = 4 + AdrResult.Cnt;
          CopyAdrVals(2, &AdrResult);
          CheckSup();
        }
        break;
      }
      case ModNone:
        break;
      default:
      {
        WAsmCode[0] = 0xe00 | AdrResult.AdrPart | (OpSize << 6);
        CodeLen = 4 + AdrResult.Cnt;
        CopyAdrVals(2, &AdrResult);
        if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
        {
          WAsmCode[1] = AdrResult.AdrPart << 12;
          CheckSup();
        }
        else
          CodeLen = 0;
      }
    }
  }
}

static void DecodeCALLM(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!(pCurrCPUProps->SuppFlags & eFlagCALLM_RTM)) WrError(ErrNum_InstructionNotSupported);
  else if (ChkArgCnt(2, 2))
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
    {
      WAsmCode[1] = AdrResult.Vals[0];
      RelPos = 4;
      if (DecodeAdr(&ArgStr[2], MModAdrI | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
      {
        WAsmCode[0] = 0x06c0 + AdrResult.AdrPart;
        CopyAdrVals(2, &AdrResult);
        CodeLen = 4 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeCAS(Word Index)
{
  UNUSED(Index);

  if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(3, 3)
        && CheckFamily((1 << e68KGen3) | (1 << e68KGen2)))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
    {
      WAsmCode[1] = AdrResult.AdrPart;
      if (DecodeAdr(&ArgStr[2], MModData, &AdrResult))
      {
        RelPos = 4;
        WAsmCode[1] += (((Word)AdrResult.AdrPart) << 6);
        if (DecodeAdr(&ArgStr[3], MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
        {
          WAsmCode[0] = 0x08c0 + AdrResult.AdrPart + (((Word)OpSize + 1) << 9);
          CopyAdrVals(2, &AdrResult);
          CodeLen = 4 + AdrResult.Cnt;
        }
      }
    }
  }
}

static void DecodeCAS2(Word Index)
{
  Word w1, w2;
  UNUSED(Index);

  if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (!ChkArgCnt(3, 3));
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2)));
  else if (!DecodeRegPair(&ArgStr[1], WAsmCode + 1, WAsmCode + 2)) WrStrErrorPos(ErrNum_InvRegPair, &ArgStr[1]);
  else if (!DecodeRegPair(&ArgStr[2], &w1, &w2)) WrStrErrorPos(ErrNum_InvRegPair, &ArgStr[2]);
  else
  {
    WAsmCode[1] += (w1 << 6);
    WAsmCode[2] += (w2 << 6);
    if (!CodeIndRegPair(&ArgStr[3], &w1, &w2)) WrStrErrorPos(ErrNum_InvRegPair, &ArgStr[3]);
    else
    {
      WAsmCode[1] += (w1 << 12);
      WAsmCode[2] += (w2 << 12);
      WAsmCode[0] = 0x0cfc + (((Word)OpSize - 1) << 9);
      CodeLen = 6;
    }
  }
}

static void DecodeCMPCHK2(Word Index)
{
  if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2) | (1 << eCPU32)));
  else if (ChkArgCnt(2, 2))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
    {
      RelPos = 4;
      WAsmCode[1] = (((Word)AdrResult.AdrPart) << 12) | (Index << 11);
      if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
      {
        WAsmCode[0] = 0x00c0 + (((Word)OpSize) << 9) + AdrResult.AdrPart;
        CopyAdrVals(2, &AdrResult);
        CodeLen = 4 + AdrResult.Cnt;
      }
    }
  }
}

static void DecodeEXTB(Word Index)
{
  UNUSED(Index);

  if ((OpSize != eSymbolSize32Bit) && *AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2) | (1 << eCPU32)));
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
    {
      WAsmCode[0] = 0x49c0 + AdrResult.AdrPart;
      CodeLen = 2;
    }
  }
}

static void DecodePACK(Word Index)
{
  if (!ChkArgCnt(3, 3));
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2)));
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData | MModPre, &AdrResult))
    {
      WAsmCode[0] = (0x8140 + (Index << 6)) | (AdrResult.AdrPart & 7);
      if (AdrResult.AdrMode == ModPre)
        WAsmCode[0] += 8;
      if (DecodeAdr(&ArgStr[2], 1 << (AdrResult.AdrMode - 1), &AdrResult))
      {
        WAsmCode[0] |= ((AdrResult.AdrPart & 7) << 9);
        if (DecodeAdr(&ArgStr[3], MModImm, &AdrResult))
        {
          WAsmCode[1] = AdrResult.Vals[0];
          CodeLen = 4;
        }
      }
    }
  }
}

static void DecodeRTM(Word Index)
{
  UNUSED(Index);

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!(pCurrCPUProps->SuppFlags & eFlagCALLM_RTM)) WrError(ErrNum_InstructionNotSupported);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData | MModAdr, &AdrResult))
    {
      WAsmCode[0] = 0x06c0 + AdrResult.AdrPart;
      CodeLen = 2;
    }
  }
}

static void DecodeTBL(Word Index)
{
  char *p;
  Word w2, Mode;

  if (!ChkArgCnt(2, 2));
  else if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else if (CheckFamily(1 << eCPU32))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModData, &AdrResult))
    {
      Mode = AdrResult.AdrPart;
      p = strchr(ArgStr[1].str.p_str, ':');
      if (!p)
      {
        RelPos = 4;
        if (DecodeAdr(&ArgStr[1], MModAdrI | MModDAdrI | MModAIX| MModAbs | MModPC | MModPCIdx, &AdrResult))
        {
          WAsmCode[0] = 0xf800 | AdrResult.AdrPart;
          WAsmCode[1] = 0x0100 | (OpSize << 6) | (Mode << 12) | (Index << 10);
          memcpy(WAsmCode + 2, AdrResult.Vals, AdrResult.Cnt);
          CodeLen = 4 + AdrResult.Cnt;
        }
      }
      else
      {
        strcpy(ArgStr[3].str.p_str, p + 1);
        *p = '\0';
        if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
        {
          w2 = AdrResult.AdrPart;
          if (DecodeAdr(&ArgStr[3], MModData, &AdrResult))
          {
            WAsmCode[0] = 0xf800 | w2;
            WAsmCode[1] = 0x0000 | (OpSize << 6) | (Mode << 12) | AdrResult.AdrPart;
            if (OpPart.str.p_str[3] == 'S')
              WAsmCode[1] |= 0x0800;
            if (OpPart.str.p_str[strlen(OpPart.str.p_str) - 1] == 'N')
              WAsmCode[1] |= 0x0400;
            CodeLen = 4;
          }
        }
      }
    }
  }
}

/* 0=BTST 1=BCHG 2=BCLR 3=BSET */

static void DecodeBits(Word Index)
{
  Word Mask, BitNum, BitMax;
  tSymbolSize SaveOpSize;
  unsigned ResCodeLen;
  Boolean BitNumUnknown = False;
  tAdrResult AdrResult;

  if (!ChkArgCnt(2, 2))
    return;

  WAsmCode[0] = (Index << 6);
  ResCodeLen = 1;

  SaveOpSize = OpSize;
  OpSize = eSymbolSize8Bit;
  switch (DecodeAdr(&ArgStr[1], MModData | MModImm, &AdrResult))
  {
    case ModData:
      WAsmCode[0] |= 0x100 | (AdrResult.AdrPart << 9);
      BitNum = 0; /* implicitly suppresses bit pos check */
      break;
    case ModImm:
      WAsmCode[0] |= 0x800;
      WAsmCode[ResCodeLen++] = BitNum = AdrResult.Vals[0];
      BitNumUnknown = mFirstPassUnknown(AdrResult.ImmSymFlags);
      break;
    default:
      return;
  }

  OpSize = SaveOpSize;
  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize8Bit;

  Mask = MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs;
  if (!Index)
    Mask |= MModPC | MModPCIdx | MModImm;
  RelPos = ResCodeLen << 1;
  DecodeAdr(&ArgStr[2], Mask, &AdrResult);

  if (!*AttrPart.str.p_str)
    OpSize = (AdrResult.AdrMode == ModData) ? eSymbolSize32Bit : eSymbolSize8Bit;
  if (!AdrResult.AdrMode)
    return;
  if (((AdrResult.AdrMode == ModData) && (OpSize != eSymbolSize32Bit)) || ((AdrResult.AdrMode != ModData) && (OpSize != eSymbolSize8Bit)))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }

  BitMax = (AdrResult.AdrMode == ModData) ? 31 : 7;
  WAsmCode[0] |= AdrResult.AdrPart;
  CopyAdrVals(ResCodeLen, &AdrResult);
  CodeLen = (ResCodeLen << 1) + AdrResult.Cnt;
  if (!BitNumUnknown && (BitNum > BitMax))
    WrError(ErrNum_BitNumberTruncated);
}

/* 0=BFTST 1=BFCHG 2=BFCLR 3=BFSET */

static void DecodeFBits(Word Index)
{
  Word guess_mask;

  if (!ChkArgCnt(1, 1));
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2)));
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!SplitBitField(&ArgStr[1], WAsmCode + 1, &guess_mask)) WrError(ErrNum_InvBitMask);
  else
  {
    tAdrResult AdrResult;

    RelPos = 4;
    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModDAdrI | MModAIX | MModAbs | (Memo("BFTST") ? (MModPC | MModPCIdx) : 0), &AdrResult))
    {
      WAsmCode[0] = 0xe8c0 | AdrResult.AdrPart | (Index << 9);
      or_wasmcode_guessed(1, 1, guess_mask);
      CopyAdrVals(2, &AdrResult);
      CodeLen = 4 + AdrResult.Cnt;
    }
  }
}

/* 0=BFEXTU 1=BFEXTS 2=BFFFO */

static void DecodeEBits(Word Index)
{
  Word guess_mask;

  if (!ChkArgCnt(2, 2));
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2)));
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!SplitBitField(&ArgStr[1], WAsmCode + 1, &guess_mask)) WrError(ErrNum_InvBitMask);
  else
  {
    tAdrResult AdrResult;

    RelPos = 4;
    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
    {
      LongInt ThisCodeLen = 4 + AdrResult.Cnt;

      WAsmCode[0] = 0xe9c0 + AdrResult.AdrPart + (Index << 9);
      or_wasmcode_guessed(1, 1, guess_mask);
      CopyAdrVals(2, &AdrResult);
      if (DecodeAdr(&ArgStr[2], MModData, &AdrResult))
      {
        WAsmCode[1] |= AdrResult.AdrPart << 12;
        CodeLen = ThisCodeLen;
      }
    }
  }
}

static void DecodeBFINS(Word Index)
{
  Word guess_mask;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!CheckFamily((1 << e68KGen3) | (1 << e68KGen2)));
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!SplitBitField(&ArgStr[2], WAsmCode + 1, &guess_mask)) WrError(ErrNum_InvBitMask);
  else
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[2], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      LongInt ThisCodeLen = 4 + AdrResult.Cnt;

      WAsmCode[0] = 0xefc0 + AdrResult.AdrPart;
      or_wasmcode_guessed(1, 1, guess_mask);
      CopyAdrVals(2, &AdrResult);
      if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
      {
        WAsmCode[1] |= AdrResult.AdrPart << 12;
        CodeLen = ThisCodeLen;
      }
    }
  }
}

/* bedingte Befehle */

static void DecodeBcc(Word CondCode)
{
  /* .W, .S, .L, .X erlaubt */

  if ((OpSize > eSymbolSize32Bit) && (OpSize != eSymbolSizeFloat32Bit) && (OpSize != eSymbolSizeFloat96Bit)) WrError(ErrNum_InvOpSize);

  /* nur ein Operand erlaubt */

  else if (ChkArgCnt(1, 1))
  {
    LongInt HVal;
    Integer HVal16;
    ShortInt HVal8;
    Boolean ValOK, IsBSR = (1 == CondCode);
    tSymbolFlags Flags;

    /* Zieladresse ermitteln, zum Programmzaehler relativieren */

    HVal = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ValOK, &Flags);
    HVal = HVal - (EProgCounter() + 2);

    /* Bei Automatik Groesse festlegen */

    if (!*AttrPart.str.p_str)
    {
      if (IsDisp8(HVal))
      {
        /* BSR with zero displacement cannot be converted to NOP.  Generate a
           16 bit displacement instead. */

        if (!HVal && IsBSR)
          OpSize = eSymbolSize32Bit;

        /* if the jump target is the address right behind the BSR, keep
           16 bit displacement to avoid oscillating back and forth between
           8 and 16 bits: */

        else if ((Flags & eSymbolFlag_NextLabelAfterBSR) && (HVal == 2) && IsBSR)
          OpSize = eSymbolSize32Bit;
        else
          OpSize = eSymbolSizeFloat32Bit;
      }
      else if (IsDisp16(HVal))
        OpSize = eSymbolSize32Bit;
      else
        OpSize = eSymbolSizeFloat96Bit;
    }

    if (ValOK)
    {
      /* 16 Bit (.L or .W) ? */

      if ((OpSize == eSymbolSize32Bit) || (OpSize == eSymbolSize16Bit))
      {
        /* zu weit ? */

        HVal16 = HVal;
        if (!IsDisp16(HVal) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          /* Code erzeugen */

          CodeLen = 4;
          WAsmCode[0] = 0x6000 | (CondCode << 8);
          WAsmCode[1] = HVal16;
          set_w_guessed(Flags, 1, 1, 0xffff);
        }
      }

      /* 8 Bit (.S or .B) ? */

      else if ((OpSize == eSymbolSizeFloat32Bit) || (OpSize == eSymbolSize8Bit))
      {
        /* zu weit ? */

        HVal8 = HVal;
        if (!IsDisp8(HVal) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);

        /* cannot generate short BSR with zero displacement, and BSR cannot
           be replaced with NOP -> error */

        else if ((HVal == 0) && IsBSR && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);

        /* Code erzeugen */

        else
        {
          CodeLen = 2;
          if ((HVal8 != 0) || IsBSR)
          {
            WAsmCode[0] = 0x6000 | (CondCode << 8) | ((Byte)HVal8);
            set_w_guessed(Flags, 0, 1, 0x00ff);
          }
          else
          {
            WAsmCode[0] = NOPCode;
            if ((!Repass) && *AttrPart.str.p_str)
              WrError(ErrNum_DistNull);
          }
        }
      }

      /* 32 Bit ?  Complain about non-supported instruction only if .X
         was requested explicitly: */

      else if (!(pCurrCPUProps->SuppFlags & eFlagBranch32)) WrError(*AttrPart.str.p_str ? ErrNum_InstructionNotSupported : ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 6;
        WAsmCode[0] = 0x60ff | (CondCode << 8);
        WAsmCode[1] = HVal >> 16;
        WAsmCode[2] = HVal & 0xffff;
        set_w_guessed(Flags, 1, 2, 0xffff);
      }
    }

    if ((CodeLen > 0) && IsBSR)
      AfterBSRAddr = EProgCounter() + CodeLen;
  }
}

static void DecodeScc(Word CondCode)
{
  if (*AttrPart.str.p_str && (OpSize != eSymbolSize8Bit)) WrError(ErrNum_InvOpSize);
  else if (ArgCnt != 1) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModData | ((pCurrCPUProps->Family == eColdfire) ? 0 : MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs), &AdrResult))
    {
      WAsmCode[0] = 0x50c0 | (CondCode << 8) | AdrResult.AdrPart;
      CodeLen = 2 + AdrResult.Cnt;
      CopyAdrVals(1, &AdrResult);
    }
  }
}

static void DecodeDBcc(Word CondCode)
{
  if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(2, 2)
        && CheckNoFamily(1 << eColdfire))
  {
    Boolean ValOK;
    tSymbolFlags Flags;
    LongInt HVal = EvalStrIntExpressionWithFlags(&ArgStr[2], Int32, &ValOK, &Flags);
    Integer HVal16;

    if (ValOK)
    {
      HVal -= (EProgCounter() + 2);
      HVal16 = HVal;
      if (!IsDisp16(HVal) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        tAdrResult AdrResult;

        CodeLen = 4;
        WAsmCode[0] = 0x50c8 | (CondCode << 8);
        WAsmCode[1] = HVal16;
        set_w_guessed(Flags, 1, 1, 0xffff);
        if (DecodeAdr(&ArgStr[1], MModData, &AdrResult) == ModData)
          WAsmCode[0] |= AdrResult.AdrPart;
        else
          CodeLen = 0;
      }
    }
  }
}

static void DecodeTRAPcc(Word CondCode)
{
  int ExpectArgCnt;

  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize8Bit;
  ExpectArgCnt = (OpSize == eSymbolSize8Bit) ? 0 : 1;
  if (OpSize > 2) WrError(ErrNum_InvOpSize);
  else if (!ChkArgCnt(ExpectArgCnt, ExpectArgCnt));
  else if ((CondCode != 1) && !CheckNoFamily(1 << eColdfire));
  else
  {
    WAsmCode[0] = 0x50f8 + (CondCode << 8);
    if (OpSize == eSymbolSize8Bit)
    {
      WAsmCode[0] += 4;
      CodeLen = 2;
    }
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
      {
        WAsmCode[0] += OpSize + 1;
        CopyAdrVals(1, &AdrResult);
        CodeLen = 2 + AdrResult.Cnt;
      }
    }
    CheckFamily((1 << eColdfire) | (1 << eCPU32) | (1 << e68KGen2) | (1 << e68KGen3));
  }
}

/*-------------------------------------------------------------------------*/
/* Dekodierroutinen Gleitkommaeinheit */

enum { eFMovemTypNone = 0, eFMovemTypDyn = 1, eFMovemTypStatic = 2, eFMovemTypCtrl = 3 };

static void DecodeFRegList(const tStrComp *pArg, Byte *pTyp, Byte *pList)
{
  Word hw, Reg, RegFrom, RegTo;
  Byte z;
  char *p, *p2;
  String ArgStr;
  tStrComp Arg, Remainder, From, To;

  StrCompMkTemp(&Arg, ArgStr, sizeof(ArgStr));
  StrCompCopy(&Arg, pArg);

  *pTyp = eFMovemTypNone;
  if (*Arg.str.p_str == '\0')
    return;

  switch (DecodeReg(&Arg, &Reg, False))
  {
    case eIsReg:
      if (Reg & 8)
        return;
      *pTyp = eFMovemTypDyn;
      *pList = Reg << 4;
      return;
    case eRegAbort:
      return;
    default:
      break;
  }

  hw = 0;
  do
  {
    p = strchr(Arg.str.p_str, '/');
    if (p)
      StrCompSplitRef(&Arg, &Remainder, &Arg, p);
    p2 = strchr(Arg.str.p_str, '-');
    if (p2)
    {
      StrCompSplitRef(&From, &To, &Arg, p2);
      if (!strlen(To.str.p_str)
       || (DecodeFPReg(&From, &RegFrom, False) != eIsReg)
       || (RegFrom & REG_FPCTRL)
       || (DecodeFPReg(&To, &RegTo, False) != eIsReg)
       || (RegTo & REG_FPCTRL))
        return;
      if (RegFrom <= RegTo)
        for (z = RegFrom; z <= RegTo; z++) hw |= (1 << (7 - z));
      else
      {
        for (z = RegFrom; z <= 7; z++) hw |= (1 << (7 - z));
        for (z = 0; z <= RegTo; z++) hw |= (1 << (7 - z));
      }
    }
    else
    {
      if (DecodeFPReg(&Arg, &Reg, False) != eIsReg)
        return;
      if (Reg & REG_FPCTRL)
        hw |= (Reg & 7) << 8;
      else
        hw |= (1 << (7 - Reg));
    }
    if (p)
      Arg = Remainder;
  }
  while (p);
  if (Hi(hw) == 0)
  {
    *pTyp = eFMovemTypStatic;
    *pList = Lo(hw);
  }
  else if (Lo(hw) == 0)
  {
    *pTyp = eFMovemTypCtrl;
    *pList = Hi(hw);
  }
}

static Byte Mirror8(Byte List)
{
  Byte hList;
  int z;

  hList = List; List = 0;
  for (z = 0; z < 8; z++)
  {
    List = List << 1;
    if (hList & 1)
      List |= 1;
    hList = hList >> 1;
  }
  return List;
}

static void GenerateMovem(Byte Typ, Byte List, tAdrResult *pResult)
{
  if (pResult->AdrMode == ModNone)
    return;
  CodeLen = 4 + pResult->Cnt;
  CopyAdrVals(2, pResult);
  WAsmCode[0] = 0xf200 | pResult->AdrPart;
  switch (Typ)
  {
    case eFMovemTypDyn:
    case eFMovemTypStatic:
      WAsmCode[1] |= 0xc000;
      if (Typ == eFMovemTypDyn)
        WAsmCode[1] |= 0x800;
      if (pResult->AdrMode != ModPre)
        WAsmCode[1] |= 0x1000;
      if ((pResult->AdrMode == ModPre) && (Typ == eFMovemTypStatic))
        List = Mirror8(List);
      WAsmCode[1] |= List;
      break;
    case eFMovemTypCtrl:
      WAsmCode[1] |= 0x8000 | (((Word)List) << 10);
      break;
  }
}

/*-------------------------------------------------------------------------*/

static void DecodeFPUOp(Word Index)
{
  FPUOp *Op = FPUOps + Index;
  tStrComp *pArg2 = &ArgStr[2];

  if ((ArgCnt == 1) && (!Op->Dya))
  {
    pArg2 = &ArgStr[1];
    ArgCnt = 2;
  }

  if (!CheckFloatSize());
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if ((pCurrCPUProps->SuppFlags & Op->NeedsSuppFlags) != Op->NeedsSuppFlags) WrError(ErrNum_InstructionNotSupported);
  else if (ChkArgCnt(2, 2))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(pArg2, MModFPn, &AdrResult) == ModFPn)
    {
      Word SrcMask;

      WAsmCode[0] = 0xf200;
      WAsmCode[1] = Op->Code | (AdrResult.AdrPart << 7);
      RelPos = 4;

      SrcMask = MModAdrI | MModDAdrI | MModPost | MModPre | MModPC | MModFPn;
      if (FloatOpSizeFitsDataReg(OpSize))
        SrcMask |= MModData;
      if (pCurrCPUProps->Family != eColdfire)
        SrcMask |= MModAIX | MModAbs | MModPCIdx | MModImm;
      if (DecodeAdr(&ArgStr[1], SrcMask, &AdrResult) == ModFPn)
      {
        WAsmCode[1] |= AdrResult.AdrPart << 10;
        if (OpSize == NativeFloatSize)
          CodeLen = 4;
        else
          WrError(ErrNum_InvOpSize);
      }
      else if (AdrResult.AdrMode != ModNone)
      {
        CodeLen = 4 + AdrResult.Cnt;
        CopyAdrVals(2, &AdrResult);
        WAsmCode[0] |= AdrResult.AdrPart;
        WAsmCode[1] |= 0x4000 | (((Word)FSizeCodes[OpSize]) << 10);
      }
    }
  }
}

static void DecodeFSAVE(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0xf300 | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
      CheckSup();
    }
  }
}

static void DecodeFRESTORE(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    RelPos = 4;
    if (DecodeAdr(&ArgStr[1], MModAdrI | MModPost | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0xf340 | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
      CheckSup();
    }
  }
}

static void DecodeFNOP(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(0, 0));
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else
  {
    CodeLen = 4;
    WAsmCode[0] = 0xf280;
    WAsmCode[1] = 0;
  }
}

/* TODO: does a { } suffix to <dest> as k factor conflict
   with other features, like stringification?  Maybe better
   check whether this is a valid k factor (register (symbol)
   or immediate) and only cut off if yes.  We might not be
   able to use DecodeAdr() for this any more: */

static char *split_k(tStrComp *p_arg, tStrComp *p_k)
{
  int l = strlen(p_arg->str.p_str);
  char *p_sep;

  if ((l < 2) || (p_arg->str.p_str[l - 1] != '}'))
    return NULL;
  p_sep = RQuotPos(p_arg->str.p_str, '{');
  if (!p_sep)
    return NULL;

  StrCompSplitRef(p_arg, p_k, p_arg, p_sep);
  StrCompShorten(p_k, 1);
  KillPostBlanksStrComp(p_arg);
  KillPrefBlanksStrCompRef(p_k);
  return p_sep;
}

static void DecodeFMOVE(Word Code)
{
  Word DestMask, SrcMask;
  tAdrResult DestAdrResult, SrcAdrResult;
  tStrComp KArg;
  Boolean op_size_implicit = !*AttrPart.str.p_str;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2))
    return;
  if (!FPUAvail)
  {
    WrError(ErrNum_FPUNotEnabled);
    return;
  }
  if (!CheckFloatSize())
    return;

  /* k-Faktor abspalten */

  LineCompReset(&KArg.Pos);
  if (OpSize == eSymbolSizeFloatDec96Bit)
  {
    if (!split_k(&AttrPart, &KArg))
      split_k(&ArgStr[2], &KArg);
  }

  DestMask = MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModFPCR | MModFPn;
  if (pCurrCPUProps->Family != eColdfire)
    DestMask |= MModAIX | MModAbs | MModImm;
  if (FloatOpSizeFitsDataReg(OpSize) || op_size_implicit)
    DestMask |= MModData;
  switch (DecodeAdr(&ArgStr[2], DestMask, &DestAdrResult))
  {
    case ModFPn: /* FMOVE.x <ea>/FPm,FPn ? */
    {
      WAsmCode[0] = 0xf200;
      WAsmCode[1] = DestAdrResult.AdrPart << 7;
      RelPos = 4;
      SrcMask = MModAdrI | MModPost | MModPre | MModDAdrI | MModPC | MModFPn;
      if (pCurrCPUProps->Family != eColdfire)
        SrcMask |= MModAIX | MModAbs | MModImm | MModPCIdx;
      if (FloatOpSizeFitsDataReg(OpSize))
        SrcMask |= MModData;
      switch (DecodeAdr(&ArgStr[1], SrcMask, &SrcAdrResult))
      {
        case ModFPn: /* FMOVE.X FPm,FPn ? */
        {
          if (OpSize != NativeFloatSize)
          {
            WrError(ErrNum_InvOpSize);
            return;
          }
          WAsmCode[1] |= SrcAdrResult.AdrPart << 10;
          CodeLen = 4;
          break;
        }
        case ModNone:
          break;
        default: /* FMOVE.x <ea>,FPn ? */
          CodeLen = 4 + SrcAdrResult.Cnt;
          CopyAdrVals(2, &SrcAdrResult);
          WAsmCode[0] |= SrcAdrResult.AdrPart;
          WAsmCode[1] |= 0x4000 | (((Word)FSizeCodes[OpSize]) << 10);
      }
      break;
    }
    case ModFPCR: /* FMOVE.L <ea>,FPcr ? */
    {
      if ((OpSize != eSymbolSize32Bit) && !op_size_implicit)
      {
        WrError(ErrNum_InvOpSize);
        return;
      }
      RelPos = 4;
      WAsmCode[0] = 0xf200;
      WAsmCode[1] = 0x8000 | (DestAdrResult.AdrPart << 10);
      SrcMask = MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModPC;
      if (pCurrCPUProps->Family != eColdfire)
        SrcMask |= MModAIX | MModAbs | MModImm | MModPCIdx;
      if (DestAdrResult.AdrPart == REG_FPIAR) /* only for FPIAR */
        SrcMask |= MModAdr;
      if (DecodeAdr(&ArgStr[1], SrcMask, &SrcAdrResult))
      {
        WAsmCode[0] |= SrcAdrResult.AdrPart;
        CodeLen = 4 + SrcAdrResult.Cnt;
        CopyAdrVals(2, &SrcAdrResult);
      }
      break;
    }
    case ModNone:
      break;
    default: /* FMOVE.x ????,<ea> ? */
    {
      WAsmCode[0] = 0xf200 | DestAdrResult.AdrPart;
      CodeLen = 4 + DestAdrResult.Cnt;
      CopyAdrVals(2, &DestAdrResult);
      switch (DecodeAdr(&ArgStr[1], (DestAdrResult.AdrMode == ModAdr) ? MModFPCR : MModFPn | MModFPCR, &SrcAdrResult))
      {
        case ModFPn:                       /* FMOVE.x FPn,<ea> ? */
        {
          if (DestAdrResult.AdrMode == ModAdr)
          {
            WrError(ErrNum_InvAddrMode);
            CodeLen = 0;
            return;
          }
          WAsmCode[1] = 0x6000 | (((Word)FSizeCodes[OpSize]) << 10) | (SrcAdrResult.AdrPart << 7);
          if (OpSize == eSymbolSizeFloatDec96Bit)
          {
            if (KArg.Pos.Len > 0)
            {
              tAdrResult KResult;

              OpSize = eSymbolSize8Bit;
              switch (DecodeAdr(&KArg, MModData | MModImm, &KResult))
              {
                case ModData:
                  WAsmCode[1] |= (KResult.AdrPart << 4) | 0x1000;
                  break;
                case ModImm:
                  WAsmCode[1] |= (KResult.Vals[0] & 127);
                  break;
                default:
                  CodeLen = 0;
              }
            }
            else
              WAsmCode[1] |= 17;
          }
          break;
        }
        case ModFPCR:                  /* FMOVE.L FPcr,<ea> ? */
        {
          if (!op_size_implicit && (OpSize != eSymbolSize32Bit))
          {
            WrError(ErrNum_InvOpSize);
            CodeLen = 0;
            return;
          }
          if ((SrcAdrResult.AdrPart != REG_FPIAR) && (DestAdrResult.AdrMode == ModAdr))
          {
            WrError(ErrNum_InvAddrMode);
            CodeLen = 0;
            return;
          }
          WAsmCode[1] = 0xa000 | (SrcAdrResult.AdrPart << 10);
          break;
        }
        default:
          CodeLen = 0;
      }
    }
  }
}

static void DecodeFMOVECR(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (!CheckNoFamily(1 << eColdfire));
  else if (*AttrPart.str.p_str && (OpSize != eSymbolSizeFloat96Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModFPn, &AdrResult) == ModFPn)
    {
      WAsmCode[0] = 0xf200;
      WAsmCode[1] = 0x5c00 | (AdrResult.AdrPart << 7);
      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult) == ModImm)
      {
        if (AdrResult.Vals[0] > 63) WrError(ErrNum_RomOffs063);
        else
        {
          CodeLen = 4;
          WAsmCode[1] |= AdrResult.Vals[0];
        }
      }
    }
  }
}

static void DecodeFTST(Word Code)
{
  UNUSED(Code);

  if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (!CheckFloatSize());
  else if (ChkArgCnt(1, 1))
  {
    Word Mask;
    tAdrResult AdrResult;

    RelPos = 4;
    Mask = MModAdrI | MModPost | MModPre | MModDAdrI | MModPC | MModFPn;
    if (pCurrCPUProps->Family != eColdfire)
      Mask |= MModAIX | MModPCIdx | MModAbs | MModImm;
    if (FloatOpSizeFitsDataReg(OpSize))
      Mask |= MModData;
    if (DecodeAdr(&ArgStr[1], Mask, &AdrResult) == ModFPn)
    {
      WAsmCode[0] = 0xf200;
      WAsmCode[1] = 0x3a | (AdrResult.AdrPart << 10);
      CodeLen = 4;
    }
    else if (AdrResult.AdrMode != ModNone)
    {
      WAsmCode[0] = 0xf200 | AdrResult.AdrPart;
      WAsmCode[1] = 0x403a | (((Word)FSizeCodes[OpSize]) << 10);
      CodeLen = 4 + AdrResult.Cnt;
      CopyAdrVals(2, &AdrResult);
    }
  }
}

static void DecodeFSINCOS(Word Code)
{
  UNUSED(Code);

  if (!*AttrPart.str.p_str)
    OpSize = NativeFloatSize;
  if (OpSize == 3) WrError(ErrNum_InvOpSize);
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (!CheckNoFamily(1 << eColdfire));
  else if (ChkArgCnt(2, 3))
  {
    tStrComp *pArg2, *pArg3, Arg2, Arg3;
    tAdrResult AdrResult;

    if (3 == ArgCnt)
    {
      pArg2 = &ArgStr[2];
      pArg3 = &ArgStr[3];
    }
    else
    {
      char *pKSep = strrchr(ArgStr[2].str.p_str, ':');

      if (!pKSep)
      {
        WrError(ErrNum_WrongArgCnt);
        return;
      }
      StrCompSplitRef(&Arg2, &Arg3, &ArgStr[2], pKSep);
      pArg2 = &Arg2;
      pArg3 = &Arg3;
    }
    if (DecodeAdr(pArg2, MModFPn, &AdrResult) == ModFPn)
    {
      WAsmCode[1] = AdrResult.AdrPart | 0x30;
      if (DecodeAdr(pArg3, MModFPn, &AdrResult) == ModFPn)
      {
        WAsmCode[1] |= (AdrResult.AdrPart << 7);
        RelPos = 4;
        switch (DecodeAdr(&ArgStr[1], ((OpSize <= eSymbolSize32Bit) || (OpSize == eSymbolSizeFloat32Bit))
                                     ? MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm | MModFPn
                                     : MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm | MModFPn, &AdrResult))
        {
          case ModFPn:
            WAsmCode[0] = 0xf200;
            WAsmCode[1] |= (AdrResult.AdrPart << 10);
            CodeLen = 4;
            break;
          case ModNone:
            break;
          default:
            WAsmCode[0] = 0xf200 | AdrResult.AdrPart;
            WAsmCode[1] |= 0x4000 | (((Word)FSizeCodes[OpSize]) << 10);
            CodeLen = 4 + AdrResult.Cnt;
            CopyAdrVals(2, &AdrResult);
        }
      }
    }
  }
}

static void DecodeFDMOVE_FSMOVE(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (CheckFamily((1 << e68KGen3) | (1 << eColdfire)))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModFPn, &AdrResult) == ModFPn)
    {
      unsigned Mask;

      WAsmCode[0] = 0xf200;
      WAsmCode[1] = Code | AdrResult.AdrPart << 7;
      RelPos = 4;
      if (!*AttrPart.str.p_str)
        OpSize = NativeFloatSize;
      Mask = MModFPn | MModAdrI | MModPost | MModPre | MModDAdrI | MModPC;
      if (pCurrCPUProps->Family != eColdfire)
        Mask |= MModAIX | MModAbs | MModPCIdx | MModImm;
      if (FloatOpSizeFitsDataReg(OpSize))
        Mask |= MModData;
      if (DecodeAdr(&ArgStr[1], Mask, &AdrResult) == ModFPn)
      {
        CodeLen = 4;
        WAsmCode[1] |= (AdrResult.AdrPart << 10);
      }
      else if (AdrResult.AdrMode != ModNone)
      {
        CodeLen = 4 + AdrResult.Cnt;
        CopyAdrVals(2, &AdrResult);
        WAsmCode[0] |= AdrResult.AdrPart;
        WAsmCode[1] |= 0x4000 | (((Word)FSizeCodes[OpSize]) << 10);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFMOVEM(Word Code)
 * \brief  handle FMOVEM instruction
 * ------------------------------------------------------------------------ */

static void DecodeFMOVEM(Word Code)
{
  Byte Typ, List;
  Word Mask;
  tAdrResult AdrResult;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2))
    return;
  if (!FPUAvail)
  {
    WrError(ErrNum_FPUNotEnabled);
    return;
  }

  /* NOTE: An expression of 'Dn' may be the source or destination of a (single)
     control register FMOVEM, but also a dynamic FPn data register list.  This
     makes the decision tree a bit more complex: */

  DecodeFRegList(&ArgStr[2], &Typ, &List);
  switch (Typ)
  {
    case eFMovemTypStatic:
    case eFMovemTypCtrl:
      goto fp_list_is_dest;

    case eFMovemTypDyn:
    {
      Byte src_typ, src_list;
      DecodeFRegList(&ArgStr[1], &src_typ, &src_list);
      switch (src_typ)
      {
        case eFMovemTypStatic:
        case eFMovemTypCtrl:
          Typ = src_typ;
          List = src_list;
          goto fp_list_is_src;
        case eFMovemTypDyn:
          WrError(ErrNum_InvRegList);
          return;
        default:
          goto fp_list_is_dest;
      }
    }

    default: /* eFMovemTypNone */
      DecodeFRegList(&ArgStr[1], &Typ, &List);
      if (Typ == eFMovemTypNone)
      {
        WrError(ErrNum_InvRegList);
        return;
      }
      /* Fall-Thru */

    fp_list_is_src:
      if (*AttrPart.str.p_str && (((Typ < eFMovemTypCtrl) && (OpSize != NativeFloatSize)) || ((Typ == eFMovemTypCtrl) && (OpSize != eSymbolSize32Bit)))) WrError(ErrNum_InvOpSize);
      else if ((Typ != eFMovemTypStatic) && (pCurrCPUProps->Family == eColdfire)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        Mask = MModAdrI | MModDAdrI;
        if (pCurrCPUProps->Family != eColdfire)
          Mask |= MModPre | MModAIX | MModAbs;
        if (Typ == eFMovemTypCtrl)   /* Steuerregister auch Postinkrement */
        {
          Mask |= MModPost;
          if ((List == REG_FPCR) | (List == REG_FPSR) | (List == REG_FPIAR)) /* nur ein Register */
            Mask |= MModData;
          if (List == REG_FPIAR) /* nur FPIAR */
            Mask |= MModAdr;
        }
        if (DecodeAdr(&ArgStr[2], Mask, &AdrResult))
        {
          WAsmCode[1] = 0x2000;
          GenerateMovem(Typ, List, &AdrResult);
        }
      }
      break;

    fp_list_is_dest:
      if (*AttrPart.str.p_str
      && (((Typ < eFMovemTypCtrl) && (OpSize != NativeFloatSize))
        || ((Typ == eFMovemTypCtrl) && (OpSize != eSymbolSize32Bit))))
        WrError(ErrNum_InvOpSize);
      else if ((Typ != eFMovemTypStatic) && (pCurrCPUProps->Family == eColdfire))
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      else
      {
        RelPos = 4;
        Mask = MModAdrI | MModDAdrI | MModPC;
        if (pCurrCPUProps->Family != eColdfire)
          Mask |= MModPost | MModAIX | MModPCIdx | MModAbs;
        if (Typ == eFMovemTypCtrl)   /* Steuerregister auch Predekrement */
        {
          Mask |= MModPre;
          if ((List == REG_FPCR) | (List == REG_FPSR) | (List == REG_FPIAR)) /* nur ein Register */
            Mask |= MModData | MModImm;
          if (List == REG_FPIAR) /* nur FPIAR */
            Mask |= MModAdr;
        }
        if (DecodeAdr(&ArgStr[1], Mask, &AdrResult))
        {
          WAsmCode[1] = 0x0000;
          GenerateMovem(Typ, List, &AdrResult);
        }
      }
      break;
  }
}

static void DecodeFBcc(Word CondCode)
{
  if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else
  {
    if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit) && (OpSize != eSymbolSizeFloat96Bit)) WrError(ErrNum_InvOpSize);
    else if (ChkArgCnt(1, 1))
    {
      LongInt HVal;
      Integer HVal16;
      Boolean ValOK;
      tSymbolFlags Flags;

      HVal = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ValOK, &Flags) - (EProgCounter() + 2);
      HVal16 = HVal;

      if (!*AttrPart.str.p_str)
      {
        OpSize = (IsDisp16(HVal)) ? eSymbolSize32Bit : eSymbolSizeFloat96Bit;
      }

      if ((OpSize == eSymbolSize32Bit) || (OpSize == eSymbolSize16Bit))
      {
        if (!IsDisp16(HVal) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 4;
          WAsmCode[0] = 0xf280 | CondCode;
          WAsmCode[1] = HVal16;
          set_w_guessed(Flags, 1, 1, 0xffff);
        }
      }
      else
      {
        CodeLen = 6;
        WAsmCode[0] = 0xf2c0 | CondCode;
        WAsmCode[2] = HVal & 0xffff;
        WAsmCode[1] = HVal >> 16;
        set_w_guessed(Flags, 1, 2, 0xffff);
        if (IsDisp16(HVal) && (PassNo > 1) && !*AttrPart.str.p_str)
        {
          WrError(ErrNum_ShortJumpPossible);
          WAsmCode[0] ^= 0x40;
          CodeLen -= 2;
          WAsmCode[1] = WAsmCode[2];
          StopfZahl++;
        }
      }
    }
  }
}

static void DecodeFDBcc(Word CondCode)
{
  if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (CheckNoFamily(1 << eColdfire))
  {
    if ((OpSize != eSymbolSize16Bit) && *AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
    else if (ChkArgCnt(2, 2))
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
      {
        LongInt HVal;
        Integer HVal16;
        Boolean ValOK;
        tSymbolFlags Flags;

        WAsmCode[0] = 0xf248 | AdrResult.AdrPart;
        WAsmCode[1] = CondCode;
        HVal = EvalStrIntExpressionWithFlags(&ArgStr[2], Int32, &ValOK, &Flags) - (EProgCounter() + 4);
        if (ValOK)
        {
          HVal16 = HVal;
          WAsmCode[2] = HVal16;
          set_w_guessed(Flags, 2, 1, 0xffff);
          if (!IsDisp16(HVal) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
            else CodeLen = 6;
        }
      }
    }
  }
}

static void DecodeFScc(Word CondCode)
{
  if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (!CheckNoFamily(1 << eColdfire));
  else if ((OpSize != eSymbolSize8Bit) && *AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      CodeLen = 4 + AdrResult.Cnt;
      WAsmCode[0] = 0xf240 | AdrResult.AdrPart;
      WAsmCode[1] = CondCode;
      CopyAdrVals(2, &AdrResult);
    }
  }
}

static void DecodeFTRAPcc(Word CondCode)
{
  if (!FPUAvail) WrError(ErrNum_FPUNotEnabled);
  else if (!CheckNoFamily(1 << eColdfire));
  else
  {
    if (!*AttrPart.str.p_str)
      OpSize = eSymbolSize8Bit;
    if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
    else if (ChkArgCnt(OpSize ? 1 : 0, OpSize ? 1 : 0))
    {
      WAsmCode[0] = 0xf278;
      WAsmCode[1] = CondCode;
      if (OpSize == eSymbolSize8Bit)
      {
        WAsmCode[0] |= 4;
        CodeLen = 4;
      }
      else
      {
        tAdrResult AdrResult;

        if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
        {
          WAsmCode[0] |= (OpSize + 1);
          CopyAdrVals(2, &AdrResult);
          CodeLen = 4 + AdrResult.Cnt;
        }
      }
    }
  }
}

/*-------------------------------------------------------------------------*/
/* Hilfsroutinen MMU: */

static Boolean DecodeFC(const tStrComp *pArg, Word *erg, Word *p_guess_mask)
{
  Boolean OK;
  Word Val;

  *p_guess_mask = 0;

  if (!as_strcasecmp(pArg->str.p_str, "SFC"))
  {
    *erg = 0;
    return True;
  }

  if (!as_strcasecmp(pArg->str.p_str, "DFC"))
  {
    *erg = 1;
    return True;
  }

  switch (DecodeReg(pArg, erg, False))
  {
    case eIsReg:
      if (*erg < 8)
      {
        *erg += 8;
        return True;
      }
      break;
    case eIsNoReg:
      break;
    default:
      return False;
  }

  if (*pArg->str.p_str == '#')
  {
    tSymbolFlags flags;

    Val = EvalStrIntExpressionOffsWithFlags(pArg, 1, UInt3, &OK, &flags);
    if (OK)
    {
      *erg = Val + 16;
      *p_guess_mask = mFirstPassUnknownOrQuestionable(flags) ? 7 : 0;
    }
    return OK;
  }

  return False;
}

static Boolean DecodePMMUReg(char *Asc, Word *erg, tSymbolSize *pSize)
{
  Byte z;

  if ((strlen(Asc) == 4) && (!as_strncasecmp(Asc, "BAD", 3)) && ValReg(Asc[3]))
  {
    *pSize = eSymbolSize16Bit;
    *erg = 0x7000 + ((Asc[3] - '0') << 2);
    return True;
  }
  if ((strlen(Asc) == 4) && (!as_strncasecmp(Asc, "BAC", 3)) && ValReg(Asc[3]))
  {
    *pSize = eSymbolSize16Bit;
    *erg = 0x7400 + ((Asc[3] - '0') << 2);
    return True;
  }

  for (z = 0; PMMURegs[z].pName; z++)
    if (!as_strcasecmp(Asc, PMMURegs[z].pName))
    {
      *pSize = PMMURegs[z].Size;
      *erg = PMMURegs[z].Code << 10;
      return True;
    }
  return False;
}

/*-------------------------------------------------------------------------*/

static void DecodePSAVE(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0xf100 | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
      CheckSup();
    }
  }
}

static void DecodePRESTORE(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
  else
  {
    tAdrResult AdrResult;

    RelPos = 2;
    if (DecodeAdr(&ArgStr[1], MModAdrI | MModPost | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs, &AdrResult))
    {
      CodeLen = 2 + AdrResult.Cnt;
      WAsmCode[0] = 0xf140 | AdrResult.AdrPart;
      CopyAdrVals(1, &AdrResult);
      CheckSup();
    }
  }
}

static void DecodePFLUSHA(Word Code)
{
  UNUSED(Code);

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (ChkArgCnt(0, 0))
  {
    switch (pCurrCPUProps->Family)
    {
      case e68KGen3:
        CodeLen = 2;
        WAsmCode[0] = 0xf518;
        break;
      default:
        CodeLen = 4;
        WAsmCode[0] = 0xf000;
        WAsmCode[1] = 0x2400;
        break;
    }
    CheckSup();
  }
}

static void DecodePFLUSHAN(Word Code)
{
  UNUSED(Code);

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (ChkArgCnt(0, 0)
        && CheckFamily(1 << e68KGen3))
  {
    CodeLen = 2;
    WAsmCode[0] = 0xf510;
    CheckSup();
  }
}

static void DecodePFLUSH_PFLUSHS(Word Code)
{
  tAdrResult AdrResult;
  Word guess_fc_mask;

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (pCurrCPUProps->Family == e68KGen3)
  {
    if (Code) WrError(ErrNum_FullPMMUNotEnabled);
    else if (ChkArgCnt(1, 1))
    {
      if (DecodeAdr(&ArgStr[1], MModAdrI, &AdrResult))
      {
        WAsmCode[0] = 0xf508 + (AdrResult.AdrPart & 7);
        CodeLen = 2;
        CheckSup();
      }
    }
  }
  else if (!ChkArgCnt(2, 3));
  else if ((Code) && (!FullPMMU)) WrError(ErrNum_FullPMMUNotEnabled);
  else if (!DecodeFC(&ArgStr[1], WAsmCode + 1, &guess_fc_mask)) WrError(ErrNum_InvFCode);
  else
  {
    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[2], MModImm, &AdrResult))
    {
      if (AdrResult.Vals[0] > 15) WrError(ErrNum_InvFMask);
      else
      {
        or_wasmcode_guessed(1, 1, guess_fc_mask);
        WAsmCode[1] |= (AdrResult.Vals[0] << 5) | 0x3000 | Code;
        WAsmCode[0] = 0xf000;
        CodeLen = 4;
        CheckSup();
        if (ArgCnt == 3)
        {
          WAsmCode[1] |= 0x800;
          if (!DecodeAdr(&ArgStr[3], MModAdrI | MModDAdrI | MModAIX | MModAbs, &AdrResult))
            CodeLen = 0;
          else
          {
            WAsmCode[0] |= AdrResult.AdrPart;
            CodeLen += AdrResult.Cnt;
            CopyAdrVals(2, &AdrResult);
          }
        }
      }
    }
  }
}

static void DecodePFLUSHN(Word Code)
{
  UNUSED(Code);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (ChkArgCnt(1, 1)
        && CheckFamily(1 << e68KGen3))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI, &AdrResult))
    {
      WAsmCode[0] = 0xf500 + (AdrResult.AdrPart & 7);
      CodeLen = 2;
      CheckSup();
    }
  }
}

static void DecodePFLUSHR(Word Code)
{
  UNUSED(Code);

  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize64Bit;
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  if (OpSize != eSymbolSize64Bit) WrError(ErrNum_InvOpSize);
  else if (!ChkArgCnt(1, 1));
  else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
  else
  {
    tAdrResult AdrResult;

    RelPos = 4;
    if (DecodeAdr(&ArgStr[1], MModAdrI | MModPre | MModPost | MModDAdrI | MModAIX | MModPC | MModPCIdx | MModAbs | MModImm, &AdrResult))
    {
      WAsmCode[0] = 0xf000 | AdrResult.AdrPart;
      WAsmCode[1] = 0xa000;
      CopyAdrVals(2, &AdrResult);
      CodeLen = 4 + AdrResult.Cnt; CheckSup();
    }
  }
}

static void DecodePLOADR_PLOADW(Word Code)
{
  Word guess_fc_mask;

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (!ChkArgCnt(2, 2));
  else if (!DecodeFC(&ArgStr[1], WAsmCode + 1, &guess_fc_mask)) WrError(ErrNum_InvFCode);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModAdrI | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      or_wasmcode_guessed(1, 1, guess_fc_mask);
      WAsmCode[0] = 0xf000 | AdrResult.AdrPart;
      WAsmCode[1] |= Code;
      CodeLen = 4 + AdrResult.Cnt;
      CopyAdrVals(2, &AdrResult);
      CheckSup();
    }
  }
}

static void DecodePMOVE_PMOVEFD(Word Code)
{
  tSymbolSize RegSize;
  unsigned Mask;
  tAdrResult AdrResult;

  if (!ChkArgCnt(2, 2));
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else
  {
    if (DecodePMMUReg(ArgStr[1].str.p_str, WAsmCode + 1, &RegSize))
    {
      WAsmCode[1] |= 0x200;
      if (!*AttrPart.str.p_str)
        OpSize = RegSize;
      if (OpSize != RegSize) WrError(ErrNum_InvOpSize);
      else
      {
        Mask = MModAdrI | MModDAdrI | MModAIX | MModAbs;
        if (FullPMMU)
        {
          Mask += MModPost | MModPre;
          if (RegSize != eSymbolSize64Bit)
            Mask += MModData | MModAdr;
        }
        if (DecodeAdr(&ArgStr[2], Mask, &AdrResult))
        {
          WAsmCode[0] = 0xf000 | AdrResult.AdrPart;
          CodeLen = 4 + AdrResult.Cnt;
          CopyAdrVals(2, &AdrResult);
          CheckSup();
        }
      }
    }
    else if (DecodePMMUReg(ArgStr[2].str.p_str, WAsmCode + 1, &RegSize))
    {
      if (!*AttrPart.str.p_str)
        OpSize = RegSize;
      if (OpSize != RegSize) WrError(ErrNum_InvOpSize);
      else
      {
        RelPos = 4;
        Mask = MModAdrI | MModDAdrI | MModAIX | MModAbs;
        if (FullPMMU)
        {
          Mask += MModPost | MModPre | MModPC | MModPCIdx | MModImm;
          if (RegSize != eSymbolSize64Bit)
            Mask += MModData | MModAdr;
        }
        if (DecodeAdr(&ArgStr[1], Mask, &AdrResult))
        {
          WAsmCode[0] = 0xf000 | AdrResult.AdrPart;
          CodeLen = 4 + AdrResult.Cnt;
          CopyAdrVals(2, &AdrResult);
          WAsmCode[1] += Code;
          CheckSup();
        }
      }
    }
    else
      WrError(ErrNum_InvMMUReg);
  }
}

static void DecodePTESTR_PTESTW(Word Code)
{
  tAdrResult AdrResult;

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (pCurrCPUProps->Family == e68KGen3)
  {
    if (ChkArgCnt(1, 1))
    {
      if (DecodeAdr(&ArgStr[1], MModAdrI, &AdrResult))
      {
        WAsmCode[0] = 0xf548 + (AdrResult.AdrPart & 7) + (Code << 5);
        CodeLen = 2;
        CheckSup();
      }
    }
  }
  else if (ChkArgCnt(3, 4))
  {
    Word guess_fc_mask;

    if (!DecodeFC(&ArgStr[1], WAsmCode + 1, &guess_fc_mask)) WrError(ErrNum_InvFCode);
    else
    {
      if (DecodeAdr(&ArgStr[2], MModAdrI | MModDAdrI | MModAIX | MModAbs, &AdrResult))
      {
        WAsmCode[0] = 0xf000 | AdrResult.AdrPart;
        CodeLen = 4 + AdrResult.Cnt;
        or_wasmcode_guessed(1, 1, guess_fc_mask);
        WAsmCode[1] |= 0x8000 | (Code << 9);
        CopyAdrVals(2, &AdrResult);
        if (DecodeAdr(&ArgStr[3], MModImm, &AdrResult))
        {
          if (AdrResult.Vals[0] > 7)
          {
            WrError(ErrNum_Level07);
            CodeLen = 0;
          }
          else
          {
            WAsmCode[1] |= AdrResult.Vals[0] << 10;
            if (ArgCnt == 4)
            {
              if (!DecodeAdr(&ArgStr[4], MModAdr, &AdrResult))
                CodeLen = 0;
              else
                WAsmCode[1] |= AdrResult.AdrPart << 5;
              CheckSup();
            }
          }
        }
        else
          CodeLen = 0;
      }
    }
  }
}

static void DecodePVALID(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
  else if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModAdrI | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    {
      WAsmCode[0] = 0xf000 | AdrResult.AdrPart;
      WAsmCode[1] = 0x2800;
      CodeLen = 4 + AdrResult.Cnt;
      CopyAdrVals(2, &AdrResult);
      if (!as_strcasecmp(ArgStr[1].str.p_str, "VAL"));
      else
      {
        if (DecodeAdr(&ArgStr[1], MModAdr, &AdrResult))
          WAsmCode[1] |= 0x400 | (AdrResult.AdrPart & 7);
        else
          CodeLen = 0;
      }
    }
  }
}

static void DecodePBcc(Word CondCode)
{
  if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else
  {
    if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit) && (OpSize != eSymbolSizeFloat96Bit)) WrError(ErrNum_InvOpSize);
    else if (!ChkArgCnt(1, 1));
    else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
    else
    {
      LongInt HVal;
      Integer HVal16;
      Boolean ValOK;
      tSymbolFlags Flags;

      HVal = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ValOK, &Flags) - (EProgCounter() + 2);
      HVal16 = HVal;

      if (!*AttrPart.str.p_str)
        OpSize = (IsDisp16(HVal)) ? eSymbolSize32Bit : eSymbolSizeFloat96Bit;

      if ((OpSize == eSymbolSize32Bit) || (OpSize == eSymbolSize16Bit))
      {
        if (!IsDisp16(HVal) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 4;
          WAsmCode[0] = 0xf080 | CondCode;
          WAsmCode[1] = HVal16;
          set_w_guessed(Flags, 1, 1, 0xffff);
          CheckSup();
        }
      }
      else
      {
        CodeLen = 6;
        WAsmCode[0] = 0xf0c0 | CondCode;
        WAsmCode[2] = HVal & 0xffff;
        WAsmCode[1] = HVal >> 16;
        set_w_guessed(Flags, 1, 2, 0xffff);
        CheckSup();
      }
    }
  }
}

static void DecodePDBcc(Word CondCode)
{
  if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else
  {
    if ((OpSize != eSymbolSize16Bit) && *AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
    else if (!ChkArgCnt(2, 2));
    else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
    else
    {
      tAdrResult AdrResult;

      if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
      {
        LongInt HVal;
        Integer HVal16;
        Boolean ValOK;
        tSymbolFlags Flags;

        WAsmCode[0] = 0xf048 | AdrResult.AdrPart;
        WAsmCode[1] = CondCode;
        HVal = EvalStrIntExpressionWithFlags(&ArgStr[2], Int32, &ValOK, &Flags) - (EProgCounter() + 4);
        if (ValOK)
        {
          HVal16 = HVal;
          WAsmCode[2] = HVal16;
          set_w_guessed(Flags, 2, 1, 0xffff);
          if ((!IsDisp16(HVal)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
            CodeLen = 6;
          CheckSup();
        }
      }
    }
  }
}

static void DecodePScc(Word CondCode)
{
  if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else
  {
    if ((OpSize != eSymbolSize8Bit) && *AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
    else if (!ChkArgCnt(1, 1));
    else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
    else
    {
      tAdrResult AdrResult;

      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[1], MModData | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
      {
        CodeLen = 4 + AdrResult.Cnt;
        WAsmCode[0] = 0xf040 | AdrResult.AdrPart;
        WAsmCode[1] = CondCode;
        CopyAdrVals(2, &AdrResult);
        CheckSup();
      }
    }
  }
}

static void DecodePTRAPcc(Word CondCode)
{
  if (!PMMUAvail) WrError(ErrNum_PMMUNotEnabled);
  else
  {
    if (!*AttrPart.str.p_str)
      OpSize = eSymbolSize8Bit;
    if (OpSize > 2) WrError(ErrNum_InvOpSize);
    else if (!ChkArgCnt(OpSize ? 1 : 0, OpSize ? 1 : 0));
    else if (!FullPMMU) WrError(ErrNum_FullPMMUNotEnabled);
    else
    {
      WAsmCode[0] = 0xf078;
      WAsmCode[1] = CondCode;
      if (OpSize == eSymbolSize8Bit)
      {
        WAsmCode[0] |= 4;
        CodeLen = 4;
        CheckSup();
      }
      else
      {
        tAdrResult AdrResult;

        if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
        {
          WAsmCode[0] |= (OpSize + 1);
          CopyAdrVals(2, &AdrResult);
          CodeLen = 4 + AdrResult.Cnt;
          CheckSup();
        }
      }
    }
  }
}

static void DecodeColdBit(Word Code)
{
  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize32Bit;
  if (ChkArgCnt(1, 1)
   && CheckColdSize()
   && CheckFamily(1 << eColdfire)
   && CheckISA((1 << eCfISA_APlus) | (1 << eCfISA_C)))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
    {
      CodeLen = 2;
      WAsmCode[0] = Code | (AdrResult.AdrPart & 7);
    }
  }
}

static void DecodeSTLDSR(Word Code)
{
  UNUSED(Code);

  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize16Bit;
  if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1)
        && CheckFamily(1 << eColdfire)
        && CheckISA((1 << eCfISA_APlus) | (1 << eCfISA_C)))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModImm, &AdrResult))
    {
      CodeLen = 6;
      WAsmCode[0] = 0x40e7;
      WAsmCode[1] = 0x46fc;
      WAsmCode[2] = AdrResult.Vals[0];
    }
  }
}

static void DecodeINTOUCH(Word Code)
{
  UNUSED(Code);

  if (*AttrPart.str.p_str) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1)
        && CheckFamily(1 << eColdfire)
        && (pCurrCPUProps->CfISA >= eCfISA_B))
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[1], MModAdrI, &AdrResult))
    {
      CodeLen = 2;
      WAsmCode[0] = 0xf428 | (AdrResult.AdrPart & 7);
      CheckSup();
    }
  }
}

static void DecodeMOV3Q(Word Code)
{
  Boolean OK;
  tSymbolFlags Flags;
  ShortInt Val;
  tAdrResult AdrResult;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2)
   || !CheckFamily(1 << eColdfire)
   || (pCurrCPUProps->CfISA < eCfISA_B)
   || !CheckColdSize())
    return;

  if (!DecodeAdr(&ArgStr[2], MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs, &AdrResult))
    return;

  if (*ArgStr[1].str.p_str != '#')
  {
    WrStrErrorPos(ErrNum_OnlyImmAddr, &ArgStr[1]);
    return;
  }

  Val = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, SInt4, &OK, &Flags);
  if (!OK)
    return;
  if (mFirstPassUnknown(Flags))
    Val = 1;

  if (Val == -1)
    Val = 0;
  else if (!ChkRange(Val, 1, 7))
    return;

  WAsmCode[0] = 0xa140 | ((Val & 7) << 9) | AdrResult.AdrPart;
  set_w_guessed(Flags, 0, 1, 7 << 9);
  CopyAdrVals(1, &AdrResult);
  CodeLen = 2 + AdrResult.Cnt;
}

static void DecodeMVS_MVZ(Word Code)
{
  Word DestReg;
  tAdrResult AdrResult;

  if (!ChkArgCnt(2, 2)
   || !CheckFamily(1 << eColdfire)
   || (pCurrCPUProps->CfISA < eCfISA_B))
    return;

  if (!*AttrPart.str.p_str)
    OpSize = eSymbolSize16Bit;
  if (OpSize > eSymbolSize16Bit)
  {
    WrError(ErrNum_InvOpSize);
    return;
  }

  if (!DecodeAdr(&ArgStr[2], MModData, &AdrResult))
    return;
  DestReg = AdrResult.AdrPart & 7;

  if (DecodeAdr(&ArgStr[1], MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI | MModAIX | MModAbs | MModImm | MModPC | MModPCIdx, &AdrResult))
  {
    WAsmCode[0] = Code | (DestReg << 9) | (OpSize << 6) | AdrResult.AdrPart;
    CopyAdrVals(1, &AdrResult);
    CodeLen = 2 + AdrResult.Cnt;
  }
}

static void DecodeSATS(Word Code)
{
  tAdrResult AdrResult;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1)
   || !CheckFamily(1 << eColdfire)
   || (pCurrCPUProps->CfISA < eCfISA_B)
   || !CheckColdSize())
    return;

  if (DecodeAdr(&ArgStr[1], MModData, &AdrResult))
  {
    WAsmCode[0] = 0x4c80 | (AdrResult.AdrPart & 7);
    CodeLen = 2;
  }
}

static void DecodeMAC_MSAC(Word Code)
{
  Word Rx, Ry, Rw, Ux = 0, Uy = 0, Scale = 0, Mask, AccNum = 0, scale_guess_mask = 0;
  int CurrArg, RemArgCnt;
  Boolean ExplicitLoad = !!(Code & 0x8000);
  tAdrResult AdrResult;

  Code &= 0x7fff;

  if (!(pCurrCPUProps->SuppFlags & eFlagMAC))
  {
    WrError(ErrNum_InstructionNotSupported);
    return;
  }

  if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }

  /* 2 args is the absolute minimum.  6 is the maximum (Ry, Rx, scale, <ea>, Rw, ACC) */

  if (!ChkArgCnt(2, 6))
    return;

  /* Ry and Rx are always present, and are always the first arguments: */

  if (OpSize == eSymbolSize16Bit)
  {
    if (!SplitMACUpperLower(&Uy, &ArgStr[1])
     || !SplitMACUpperLower(&Ux, &ArgStr[2]))
      return;
  }

  if (!DecodeAdr(&ArgStr[1], MModData | MModAdr, &AdrResult))
    return;
  Ry = AdrResult.AdrPart & 15;
  if (!DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
    return;
  Rx = AdrResult.AdrPart & 15;
  CurrArg = 3;

  /* Is a scale given as next argument? */

  if ((ArgCnt >= CurrArg) && DecodeMACScale(&ArgStr[CurrArg], &Scale, &scale_guess_mask))
    CurrArg++;

  /* We now have between 0 and 3 args left:
     0 -> no load, ACC0
     1 -> ACCn
     2 -> load, ACC0
     3 -> load, ACCn
     If the 'L' variant (MACL, MSACL) was given, a parallel
     load was specified explicitly and there MUST be the <ea> and Rw arguments: */

  RemArgCnt = ArgCnt - CurrArg + 1;
  if ((RemArgCnt > 3)
   || (ExplicitLoad && (RemArgCnt < 2)))
  {
    WrError(ErrNum_WrongArgCnt);
    return;
  }

  /* assumed ACC(0) if no accumulator given */

  if (Odd(RemArgCnt))
  {
    if (!DecodeMACACC(ArgStr[ArgCnt].str.p_str, &AccNum))
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[ArgCnt]);
      return;
    }
  }

  /* If parallel load, bit 7 of first word is set for MAC.  This bit is
     used on EMAC to store accumulator # LSB.  To keep things upward-compatible,
     accumulator # LSB is stored inverted on EMAC if a parallel load is present.
     Since MAC only uses accumulator #0, this works for either target: */

  if (RemArgCnt >= 2)
    AccNum ^= 1;

  /* Common things for variant with and without parallel load: */

  WAsmCode[0] = 0xa000 | ((AccNum & 1) << 7);
  WAsmCode[1] = ((OpSize - 1) << 11) | (Scale << 9) | Code | (Ux << 7) | (Uy << 6) | ((AccNum & 2) << 3);
  or_wasmcode_guessed(1, 1, scale_guess_mask << 9);

  /* With parallel load? */

  if (RemArgCnt >= 2)
  {
    tStrComp CurrArgStr;

    if (!DecodeAdr(&ArgStr[CurrArg + 1], MModData | MModAdr, &AdrResult))
      return;
    Rw = AdrResult.AdrPart & 15;

    StrCompRefRight(&CurrArgStr, &ArgStr[CurrArg], 0);
    if (!SplitMACANDMASK(&Mask, &CurrArgStr))
      return;
    if (!DecodeAdr(&CurrArgStr, MModAdrI | MModPre | MModPost | MModDAdrI, &AdrResult))
      return;

    WAsmCode[0] |= ((Rw & 7) << 9) | ((Rw & 8) << 3) | AdrResult.AdrPart;
    WAsmCode[1] |= (Mask << 5) | (Rx << 12) | (Ry << 0);
    CodeLen = 4 + AdrResult.Cnt;
    CopyAdrVals(2, &AdrResult);
  }

  /* multiply/accumulate only */

  else
  {
    WAsmCode[0] |= Ry | ((Rx & 7) << 9) | ((Rx & 8) << 3);
    CodeLen = 4;
  }
}

static void DecodeMOVCLR(Word Code)
{
  Word ACCReg;

  UNUSED(Code);

  if (!ChkArgCnt(2,2));
  else if (*AttrPart.str.p_str && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
  else if (!(pCurrCPUProps->SuppFlags & eFlagEMAC)) WrError(ErrNum_InstructionNotSupported);
  else if (!DecodeMACACC(ArgStr[1].str.p_str, &ACCReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    tAdrResult AdrResult;

    if (DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
    {
      WAsmCode[0] = 0xa1c0 | AdrResult.AdrPart | (ACCReg << 9);
      CodeLen = 2;
    }
  }
}

static void DecodeMxxAC(Word Code)
{
  Word Rx, Ry, Ux, Uy, Scale = 0, ACCx, ACCw, scale_guess_mask = 0;
  tAdrResult AdrResult;

  if (!(pCurrCPUProps->SuppFlags & eFlagEMAC)
    || (pCurrCPUProps->CfISA < eCfISA_B))
  {
    WrError(ErrNum_InstructionNotSupported);
    return;
  }

  if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }

  if (!ChkArgCnt(4, 5))
    return;

  if (!DecodeMACACC(ArgStr[ArgCnt - 1].str.p_str, &ACCx))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[ArgCnt - 1]);
    return;
  }
  if (!DecodeMACACC(ArgStr[ArgCnt].str.p_str, &ACCw))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[ArgCnt]);
    return;
  }

  if (5 == ArgCnt)
  {
    if (!DecodeMACScale(&ArgStr[3], &Scale, &scale_guess_mask))
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[3]);
      return;
    }
  }

  if (OpSize == eSymbolSize16Bit)
  {
    if (!SplitMACUpperLower(&Uy, &ArgStr[1])
     || !SplitMACUpperLower(&Ux, &ArgStr[2]))
      return;
  }
  else
    Ux = Uy = 0;

  if (!DecodeAdr(&ArgStr[1], MModData | MModAdr, &AdrResult))
    return;
  Ry = AdrResult.AdrPart & 15;
  if (!DecodeAdr(&ArgStr[2], MModData | MModAdr, &AdrResult))
    return;
  Rx = AdrResult.AdrPart & 15;

  WAsmCode[0] = 0xa000 | ((Rx & 7) << 9) | ((Rx & 8) << 3) | Ry | ((ACCx & 1) << 7);
  WAsmCode[1] = Code | ((OpSize - 1) << 11) | (Scale << 9) | (Ux << 7) | (Uy << 6) | ((ACCx & 2) << 3) | (ACCw << 2);
  or_wasmcode_guessed(1, 1, scale_guess_mask << 9);
  CodeLen = 4;
}

static void DecodeCPBCBUSY(Word Code)
{
  if (pCurrCPUProps->CfISA == eCfISA_None) WrError(ErrNum_InstructionNotSupported);
  else if (*AttrPart.str.p_str && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
  else if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt Dist;

    Dist = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt32, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && !IsDisp16(Dist)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        WAsmCode[0] = Code;
        WAsmCode[1] = Dist & 0xffff;
        set_w_guessed(Flags, 1, 1, 0xffff);
        CodeLen = 4;
      }
    }
  }
}

static void DecodeCPLDST(Word Code)
{
  if (pCurrCPUProps->CfISA == eCfISA_None) WrError(ErrNum_InstructionNotSupported);
  else if (ChkArgCnt(1, 4))
  {
    Boolean OK;
    Word Reg;
    const tStrComp *pEAArg = NULL, *pRnArg = NULL, *pETArg = NULL;
    tSymbolFlags flags;

    WAsmCode[0] = Code | (OpSize << 6);

    /* CMD is always present and i bits 0..8 - immediate marker is optional
       since it is always a constant. */

    WAsmCode[1] = EvalStrIntExpressionOffsWithFlags(&ArgStr[ArgCnt], !!(*ArgStr[ArgCnt].str.p_str == '#'), UInt16, &OK, &flags);
    if (!OK)
      return;

    if (ArgCnt >= 2)
      pEAArg = &ArgStr[1];
    switch (ArgCnt)
    {
      case 4:
        pRnArg = &ArgStr[2];
        pETArg = &ArgStr[3];
        break;
      case 3:
        if (DecodeReg(&ArgStr[2], &Reg, False) == eIsReg)
          pRnArg = &ArgStr[2];
        else
          pETArg = &ArgStr[2];
        break;
     }

    if (pRnArg)
    {
      if (DecodeReg(pRnArg, &Reg, True) != eIsReg)
        return;
      WAsmCode[1] |= Reg << 12;
    }
    if (pETArg)
    {
      Word ET;

      ET = EvalStrIntExpression(pETArg, UInt3, &OK);
      if (!OK)
        return;
      WAsmCode[1] |= ET << 9;
    }

    set_w_guessed(flags, 1, 1, 0xffff);
    if (pEAArg)
    {
      tAdrResult AdrResult;

      if (!DecodeAdr(pEAArg, MModData | MModAdr | MModAdrI | MModPost | MModPre | MModDAdrI, &AdrResult))
        return;
      WAsmCode[0] |= AdrResult.AdrPart;
      CopyAdrVals(2, &AdrResult);
      CodeLen = 4 + AdrResult.Cnt;
    }
    else
      CodeLen = 4;
  }
}

static void DecodeCPNOP(Word Code)
{
  if (pCurrCPUProps->CfISA == eCfISA_None) WrError(ErrNum_InstructionNotSupported);
  else if (ChkArgCnt(0, 1))
  {
    WAsmCode[0] = Code | (OpSize << 6);

    /* CMD is always present and i bits 0..8 - immediate marker is optional
       since it is always a constant. */

    if (ArgCnt > 0)
    {
      Word ET;
      Boolean OK;
      tSymbolFlags flags;

      ET = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt3, &OK, &flags);
      if (!OK)
        return;
      WAsmCode[1] |= ET << 9;
      or_w_guessed(flags, 1, 1, 7 << 9);
    }

    CodeLen = 4;
  }
}

/*-------------------------------------------------------------------------*/
/* Dekodierroutinen Pseudoinstruktionen: */

static void PutByte(Byte b)
{
  if ((CodeLen & 1) && !HostBigEndian)
  {
    BAsmCode[CodeLen] = BAsmCode[CodeLen - 1];
    BAsmCode[CodeLen - 1] = b;
  }
  else
  {
    BAsmCode[CodeLen] = b;
  }
  CodeLen++;
}

static void DecodeSTR(Word Index)
{
  int l, z;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (((l = strlen(ArgStr[1].str.p_str)) < 2)
        || (*ArgStr[1].str.p_str != '\'')
        || (ArgStr[1].str.p_str[l - 1] != '\'')) WrStrErrorPos(ErrNum_ExpectString, &ArgStr[1]);
  else
  {
    PutByte(l - 2);
    for (z = 1; z < l - 1; z++)
      PutByte(as_chartrans_xlate(CurrTransTable->p_table, ((usint) ArgStr[1].str.p_str[z]) & 0xff));
  }
}

static void assure_pc_even(Word index)
{
  UNUSED(index);

  if (Odd(EProgCounter()))
  {
    if (DoPadding)
      InsertPadding(1, False);
    else
      WrError(ErrNum_AddrNotAligned);
  }
}

/*-------------------------------------------------------------------------*/
/* Codetabellenverwaltung */

static void AddFixed(const char *NName, Word NCode, Boolean NSup, unsigned NMask)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].MustSup = NSup;
  FixedOrders[InstrZ].FamilyMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddCond(const char *NName, Byte NCode)
{
  char TmpName[30];

  if (NCode >= 2) /* BT is BRA and BF is BSR */
  {
    as_snprintf(TmpName, sizeof(TmpName), "B%s", NName);
    AddInstTable(InstTable, TmpName, NCode, DecodeBcc);
  }
  as_snprintf(TmpName, sizeof(TmpName), "S%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeScc);
  as_snprintf(TmpName, sizeof(TmpName), "DB%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeDBcc);
  as_snprintf(TmpName, sizeof(TmpName), "TRAP%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeTRAPcc);
}

static void AddFPUOp(const char *NName, Byte NCode, Boolean NDya, tSuppFlags NeedFlags)
{
  order_array_rsv_end(FPUOps, FPUOp);
  FPUOps[InstrZ].Code = NCode;
  FPUOps[InstrZ].Dya = NDya;
  FPUOps[InstrZ].NeedsSuppFlags = NeedFlags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFPUOp);
}

static void AddFPUCond(const char *NName, Byte NCode)
{
  char TmpName[30];

  as_snprintf(TmpName, sizeof(TmpName), "FB%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeFBcc);
  as_snprintf(TmpName, sizeof(TmpName), "FDB%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeFDBcc);
  as_snprintf(TmpName, sizeof(TmpName), "FS%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeFScc);
  as_snprintf(TmpName, sizeof(TmpName), "FTRAP%s", NName);
  AddInstTable(InstTable, TmpName, NCode, DecodeFTRAPcc);
}

static void AddPMMUCond(const char *NName)
{
  char TmpName[30];

  as_snprintf(TmpName, sizeof(TmpName), "PB%s", NName);
  AddInstTable(InstTable, TmpName, InstrZ, DecodePBcc);
  as_snprintf(TmpName, sizeof(TmpName), "PDB%s", NName);
  AddInstTable(InstTable, TmpName, InstrZ, DecodePDBcc);
  as_snprintf(TmpName, sizeof(TmpName), "PS%s", NName);
  AddInstTable(InstTable, TmpName, InstrZ, DecodePScc);
  as_snprintf(TmpName, sizeof(TmpName), "PTRAP%s", NName);
  AddInstTable(InstTable, TmpName, InstrZ, DecodePTRAPcc);
  InstrZ++;
}

static void AddPMMUReg(const char *Name, tSymbolSize Size, Word Code)
{
  order_array_rsv_end(PMMURegs, PMMUReg);
  PMMURegs[InstrZ].pName = Name;
  PMMURegs[InstrZ].Size = Size;
  PMMURegs[InstrZ++].Code = Code;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(607);
  SetDynamicInstTable(InstTable);

  inst_table_set_prefix_proc(InstTable, assure_pc_even, 0);

  AddInstTable(InstTable, "MOVE"   , Std_Variant, DecodeMOVE);
  AddInstTable(InstTable, "MOVEA"  , A_Variant, DecodeMOVE);
  AddInstTable(InstTable, "MOVEI"  , I_Variant, DecodeMOVE);
  AddInstTable(InstTable, "LEA"    , 0, DecodeLEA);
  AddInstTable(InstTable, "ASR"    , 0, DecodeShift);
  AddInstTable(InstTable, "ASL"    , 4, DecodeShift);
  AddInstTable(InstTable, "LSR"    , 1, DecodeShift);
  AddInstTable(InstTable, "LSL"    , 5, DecodeShift);
  AddInstTable(InstTable, "ROXR"   , 2, DecodeShift);
  AddInstTable(InstTable, "ROXL"   , 6, DecodeShift);
  AddInstTable(InstTable, "ROR"    , 3, DecodeShift);
  AddInstTable(InstTable, "ROL"    , 7, DecodeShift);
  AddInstTable(InstTable, "ADDQ"   , 0, DecodeADDQSUBQ);
  AddInstTable(InstTable, "SUBQ"   , 1, DecodeADDQSUBQ);
  AddInstTable(InstTable, "ADDX"   , 1, DecodeADDXSUBX);
  AddInstTable(InstTable, "SUBX"   , 0, DecodeADDXSUBX);
  AddInstTable(InstTable, "CMPM"   , 0, DecodeCMPM);
  AddInstTable(InstTable, "SUB"    , Std_Variant + 0, DecodeADDSUBCMP);
  AddInstTable(InstTable, "CMP"    , Std_Variant + 1, DecodeADDSUBCMP);
  AddInstTable(InstTable, "ADD"    , Std_Variant + 2, DecodeADDSUBCMP);
  AddInstTable(InstTable, "SUBI"   , I_Variant + 0, DecodeADDSUBCMP);
  AddInstTable(InstTable, "CMPI"   , I_Variant + 1, DecodeADDSUBCMP);
  AddInstTable(InstTable, "ADDI"   , I_Variant + 2, DecodeADDSUBCMP);
  AddInstTable(InstTable, "SUBA"   , A_Variant + 0, DecodeADDSUBCMP);
  AddInstTable(InstTable, "CMPA"   , A_Variant + 1, DecodeADDSUBCMP);
  AddInstTable(InstTable, "ADDA"   , A_Variant + 2, DecodeADDSUBCMP);
  AddInstTable(InstTable, "AND"    , Std_Variant + 1, DecodeANDOR);
  AddInstTable(InstTable, "OR"     , Std_Variant + 0, DecodeANDOR);
  AddInstTable(InstTable, "ANDI"   , I_Variant + 1, DecodeANDOR);
  AddInstTable(InstTable, "ORI"    , I_Variant + 0, DecodeANDOR);
  AddInstTable(InstTable, "EOR"    , Std_Variant, DecodeEOR);
  AddInstTable(InstTable, "EORI"   , I_Variant, DecodeEOR);
  AddInstTable(InstTable, "PEA"    , 0, DecodePEA);
  AddInstTable(InstTable, "CLR"    , 0, DecodeCLRTST);
  AddInstTable(InstTable, "TST"    , 1, DecodeCLRTST);
  AddInstTable(InstTable, "JSR"    , 0, DecodeJSRJMP);
  AddInstTable(InstTable, "JMP"    , 1, DecodeJSRJMP);
  AddInstTable(InstTable, "TAS"    , 0, DecodeNBCDTAS);
  AddInstTable(InstTable, "NBCD"   , 1, DecodeNBCDTAS);
  AddInstTable(InstTable, "NEGX"   , 0, DecodeNEGNOT);
  AddInstTable(InstTable, "NEG"    , 2, DecodeNEGNOT);
  AddInstTable(InstTable, "NOT"    , 3, DecodeNEGNOT);
  AddInstTable(InstTable, "SWAP"   , 0, DecodeSWAP);
  AddInstTable(InstTable, "UNLK"   , 0, DecodeUNLK);
  AddInstTable(InstTable, "EXT"    , 0, DecodeEXT);
  AddInstTable(InstTable, "WDDATA" , 0, DecodeWDDATA);
  AddInstTable(InstTable, "WDEBUG" , 0, DecodeWDEBUG);
  AddInstTable(InstTable, "MOVEM"  , 0, DecodeMOVEM);
  AddInstTable(InstTable, "MOVEQ"  , 0, DecodeMOVEQ);
  AddInstTable(InstTable, "STOP"   , 0, DecodeSTOP);
  AddInstTable(InstTable, "LPSTOP" , 0, DecodeLPSTOP);
  AddInstTable(InstTable, "TRAP"   , 0, DecodeTRAP);
  AddInstTable(InstTable, "BKPT"   , 0, DecodeBKPT);
  AddInstTable(InstTable, "RTD"    , 0, DecodeRTD);
  AddInstTable(InstTable, "EXG"    , 0, DecodeEXG);
  AddInstTable(InstTable, "MOVE16" , 0, DecodeMOVE16);
  AddInstTable(InstTable, "MULU"   , 0x0000, DecodeMUL_DIV);
  AddInstTable(InstTable, "MULS"   , 0x0100, DecodeMUL_DIV);
  AddInstTable(InstTable, "DIVU"   , 0x0001, DecodeMUL_DIV);
  AddInstTable(InstTable, "DIVS"   , 0x0101, DecodeMUL_DIV);
  AddInstTable(InstTable, "DIVUL"  , 0, DecodeDIVL);
  AddInstTable(InstTable, "DIVSL"  , 1, DecodeDIVL);
  AddInstTable(InstTable, "ABCD"   , 1, DecodeASBCD);
  AddInstTable(InstTable, "SBCD"   , 0, DecodeASBCD);
  AddInstTable(InstTable, "CHK"    , 0, DecodeCHK);
  AddInstTable(InstTable, "LINK"   , 0, DecodeLINK);
  AddInstTable(InstTable, "MOVEP"  , 0, DecodeMOVEP);
  AddInstTable(InstTable, "MOVEC"  , 0, DecodeMOVEC);
  AddInstTable(InstTable, "MOVES"  , 0, DecodeMOVES);
  AddInstTable(InstTable, "CALLM"  , 0, DecodeCALLM);
  AddInstTable(InstTable, "CAS"    , 0, DecodeCAS);
  AddInstTable(InstTable, "CAS2"   , 0, DecodeCAS2);
  AddInstTable(InstTable, "CMP2"   , 0, DecodeCMPCHK2);
  AddInstTable(InstTable, "CHK2"   , 1, DecodeCMPCHK2);
  AddInstTable(InstTable, "EXTB"   , 0, DecodeEXTB);
  AddInstTable(InstTable, "PACK"   , 0, DecodePACK);
  AddInstTable(InstTable, "UNPK"   , 1, DecodePACK);
  AddInstTable(InstTable, "RTM"    , 0, DecodeRTM);
  AddInstTable(InstTable, "TBLU"   , 0, DecodeTBL);
  AddInstTable(InstTable, "TBLUN"  , 1, DecodeTBL);
  AddInstTable(InstTable, "TBLS"   , 2, DecodeTBL);
  AddInstTable(InstTable, "TBLSN"  , 3, DecodeTBL);
  AddInstTable(InstTable, "BTST"   , 0, DecodeBits);
  AddInstTable(InstTable, "BSET"   , 3, DecodeBits);
  AddInstTable(InstTable, "BCLR"   , 2, DecodeBits);
  AddInstTable(InstTable, "BCHG"   , 1, DecodeBits);
  AddInstTable(InstTable, "BFTST"  , 0, DecodeFBits);
  AddInstTable(InstTable, "BFSET"  , 3, DecodeFBits);
  AddInstTable(InstTable, "BFCLR"  , 2, DecodeFBits);
  AddInstTable(InstTable, "BFCHG"  , 1, DecodeFBits);
  AddInstTable(InstTable, "BFEXTU" , 0, DecodeEBits);
  AddInstTable(InstTable, "BFEXTS" , 1, DecodeEBits);
  AddInstTable(InstTable, "BFFFO"  , 2, DecodeEBits);
  AddInstTable(InstTable, "BFINS"  , 0, DecodeBFINS);
  AddInstTable(InstTable, "CINVA"  , 0, DecodeCacheAll);
  AddInstTable(InstTable, "CPUSHA" , 1, DecodeCacheAll);
  AddInstTable(InstTable, "CINVL"  , 1, DecodeCache);
  AddInstTable(InstTable, "CPUSHL" , 5, DecodeCache);
  AddInstTable(InstTable, "CINVP"  , 2, DecodeCache);
  AddInstTable(InstTable, "CPUSHP" , 6, DecodeCache);
  AddInstTable(InstTable, "STR"    , 0, DecodeSTR);

  InstrZ = 0;
  AddFixed("NOP"    , 0x4e71, False, (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32) | (1 << eColdfire));
  AddFixed("RESET"  , 0x4e70, True,  (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32));
  AddFixed("ILLEGAL", 0x4afc, False, (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32) | (1 << eColdfire));
  AddFixed("TRAPV"  , 0x4e76, False, (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32));
  AddFixed("RTE"    , 0x4e73, True , (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32) | (1 << eColdfire));
  AddFixed("RTR"    , 0x4e77, False, (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32));
  AddFixed("RTS"    , 0x4e75, False, (1 << e68KGen1a) | (1 << e68KGen1b) | (1 << e68KGen2) | (1 << e68KGen3) | (1 << eCPU32) | (1 << eColdfire));
  AddFixed("BGND"   , 0x4afa, False, (1 << eCPU32));
  AddFixed("HALT"   , 0x4ac8, True , (1 << eColdfire));
  AddFixed("PULSE"  , 0x4acc, True , (1 << eColdfire));

  AddCond("T" , 0);  AddCond("F" , 1);  AddCond("HI", 2);  AddCond("LS", 3);
  AddCond("CC", 4);  AddCond("CS", 5);  AddCond("NE", 6);  AddCond("EQ", 7);
  AddCond("VC", 8);  AddCond("VS", 9);  AddCond("PL",10);  AddCond("MI",11);
  AddCond("GE",12);  AddCond("LT",13);  AddCond("GT",14);  AddCond("LE",15);
  AddCond("HS", 4);  AddCond("LO", 5);
  AddInstTable(InstTable, "BRA", 0, DecodeBcc);
  AddInstTable(InstTable, "BSR", 1, DecodeBcc);
  AddInstTable(InstTable, "DBRA", 1, DecodeDBcc);

  InstrZ = 0;
  AddFPUOp("FINT"   , 0x01, False, eFlagNone  );  AddFPUOp("FSINH"  , 0x02, False, eFlagExtFPU);
  AddFPUOp("FINTRZ" , 0x03, False, eFlagNone  );  AddFPUOp("FSQRT"  , 0x04, False, eFlagNone  );
  AddFPUOp("FSSQRT" , 0x41, False, eFlagIntFPU);  AddFPUOp("FDSQRT" , 0x45, False, eFlagIntFPU);
  AddFPUOp("FLOGNP1", 0x06, False, eFlagExtFPU);  AddFPUOp("FETOXM1", 0x08, False, eFlagExtFPU);
  AddFPUOp("FTANH"  , 0x09, False, eFlagExtFPU);  AddFPUOp("FATAN"  , 0x0a, False, eFlagExtFPU);
  AddFPUOp("FASIN"  , 0x0c, False, eFlagExtFPU);  AddFPUOp("FATANH" , 0x0d, False, eFlagExtFPU);
  AddFPUOp("FSIN"   , 0x0e, False, eFlagExtFPU);  AddFPUOp("FTAN"   , 0x0f, False, eFlagExtFPU);
  AddFPUOp("FETOX"  , 0x10, False, eFlagExtFPU);  AddFPUOp("FTWOTOX", 0x11, False, eFlagExtFPU);
  AddFPUOp("FTENTOX", 0x12, False, eFlagExtFPU);  AddFPUOp("FLOGN"  , 0x14, False, eFlagExtFPU);
  AddFPUOp("FLOG10" , 0x15, False, eFlagExtFPU);  AddFPUOp("FLOG2"  , 0x16, False, eFlagExtFPU);
  AddFPUOp("FABS"   , 0x18, False, eFlagNone  );  AddFPUOp("FSABS"  , 0x58, False, eFlagIntFPU);
  AddFPUOp("FDABS"  , 0x5c, False, eFlagIntFPU);  AddFPUOp("FCOSH"  , 0x19, False, eFlagExtFPU);
  AddFPUOp("FNEG"   , 0x1a, False, eFlagNone  );  AddFPUOp("FSNEG"  , 0x5a, False, eFlagIntFPU);
  AddFPUOp("FDNEG"  , 0x5e, False, eFlagIntFPU);  AddFPUOp("FACOS"  , 0x1c, False, eFlagExtFPU);
  AddFPUOp("FCOS"   , 0x1d, False, eFlagExtFPU);  AddFPUOp("FGETEXP", 0x1e, False, eFlagExtFPU);
  AddFPUOp("FGETMAN", 0x1f, False, eFlagExtFPU);  AddFPUOp("FDIV"   , 0x20, True , eFlagNone  );
  AddFPUOp("FSDIV"  , 0x60, False, eFlagIntFPU);  AddFPUOp("FDDIV"  , 0x64, True , eFlagIntFPU);
  AddFPUOp("FMOD"   , 0x21, True , eFlagExtFPU);  AddFPUOp("FADD"   , 0x22, True , eFlagNone  );
  AddFPUOp("FSADD"  , 0x62, True , eFlagIntFPU);  AddFPUOp("FDADD"  , 0x66, True , eFlagIntFPU);
  AddFPUOp("FMUL"   , 0x23, True , eFlagNone  );  AddFPUOp("FSMUL"  , 0x63, True , eFlagIntFPU);
  AddFPUOp("FDMUL"  , 0x67, True , eFlagIntFPU);  AddFPUOp("FSGLDIV", 0x24, True , eFlagExtFPU);
  AddFPUOp("FREM"   , 0x25, True , eFlagExtFPU);  AddFPUOp("FSCALE" , 0x26, True , eFlagExtFPU);
  AddFPUOp("FSGLMUL", 0x27, True , eFlagExtFPU);  AddFPUOp("FSUB"   , 0x28, True , eFlagNone  );
  AddFPUOp("FSSUB"  , 0x68, True , eFlagIntFPU);  AddFPUOp("FDSUB"  , 0x6c, True , eFlagIntFPU);
  AddFPUOp("FCMP"   , 0x38, True , eFlagNone   );

  AddInstTable(InstTable, "FSAVE", 0, DecodeFSAVE);
  AddInstTable(InstTable, "FRESTORE", 0, DecodeFRESTORE);
  AddInstTable(InstTable, "FNOP", 0, DecodeFNOP);
  AddInstTable(InstTable, "FMOVE", 0, DecodeFMOVE);
  AddInstTable(InstTable, "FMOVECR", 0, DecodeFMOVECR);
  AddInstTable(InstTable, "FTST", 0, DecodeFTST);
  AddInstTable(InstTable, "FSINCOS", 0, DecodeFSINCOS);
  AddInstTable(InstTable, "FDMOVE", 0x0044, DecodeFDMOVE_FSMOVE);
  AddInstTable(InstTable, "FSMOVE", 0x0040, DecodeFDMOVE_FSMOVE);
  AddInstTable(InstTable, "FMOVEM", 0, DecodeFMOVEM);

  AddFPUCond("EQ"  , 0x01); AddFPUCond("NE"  , 0x0e);
  AddFPUCond("GT"  , 0x12); AddFPUCond("NGT" , 0x1d);
  AddFPUCond("GE"  , 0x13); AddFPUCond("NGE" , 0x1c);
  AddFPUCond("LT"  , 0x14); AddFPUCond("NLT" , 0x1b);
  AddFPUCond("LE"  , 0x15); AddFPUCond("NLE" , 0x1a);
  AddFPUCond("GL"  , 0x16); AddFPUCond("NGL" , 0x19);
  AddFPUCond("GLE" , 0x17); AddFPUCond("NGLE", 0x18);
  AddFPUCond("OGT" , 0x02); AddFPUCond("ULE" , 0x0d);
  AddFPUCond("OGE" , 0x03); AddFPUCond("ULT" , 0x0c);
  AddFPUCond("OLT" , 0x04); AddFPUCond("UGE" , 0x0b);
  AddFPUCond("OLE" , 0x05); AddFPUCond("UGT" , 0x0a);
  AddFPUCond("OGL" , 0x06); AddFPUCond("UEQ" , 0x09);
  AddFPUCond("OR"  , 0x07); AddFPUCond("UN"  , 0x08);
  AddFPUCond("F"   , 0x00); AddFPUCond("T"   , 0x0f);
  AddFPUCond("SF"  , 0x10); AddFPUCond("ST"  , 0x1f);
  AddFPUCond("SEQ" , 0x11); AddFPUCond("SNE" , 0x1e);

  InstrZ = 0;
  AddPMMUCond("BS"); AddPMMUCond("BC"); AddPMMUCond("LS"); AddPMMUCond("LC");
  AddPMMUCond("SS"); AddPMMUCond("SC"); AddPMMUCond("AS"); AddPMMUCond("AC");
  AddPMMUCond("WS"); AddPMMUCond("WC"); AddPMMUCond("IS"); AddPMMUCond("IC");
  AddPMMUCond("GS"); AddPMMUCond("GC"); AddPMMUCond("CS"); AddPMMUCond("CC");

  AddInstTable(InstTable, "PSAVE", 0, DecodePSAVE);
  AddInstTable(InstTable, "PRESTORE", 0, DecodePRESTORE);
  AddInstTable(InstTable, "PFLUSHA", 0, DecodePFLUSHA);
  AddInstTable(InstTable, "PFLUSHAN", 0, DecodePFLUSHAN);
  AddInstTable(InstTable, "PFLUSH", 0x0000, DecodePFLUSH_PFLUSHS);
  AddInstTable(InstTable, "PFLUSHS", 0x0400, DecodePFLUSH_PFLUSHS);
  AddInstTable(InstTable, "PFLUSHN", 0, DecodePFLUSHN);
  AddInstTable(InstTable, "PFLUSHR", 0, DecodePFLUSHR);
  AddInstTable(InstTable, "PLOADR", 0x2200, DecodePLOADR_PLOADW);
  AddInstTable(InstTable, "PLOADW", 0x2000, DecodePLOADR_PLOADW);
  AddInstTable(InstTable, "PMOVE", 0x0000, DecodePMOVE_PMOVEFD);
  AddInstTable(InstTable, "PMOVEFD", 0x0100, DecodePMOVE_PMOVEFD);
  AddInstTable(InstTable, "PTESTR", 1, DecodePTESTR_PTESTW);
  AddInstTable(InstTable, "PTESTW", 0, DecodePTESTR_PTESTW);
  AddInstTable(InstTable, "PVALID", 0, DecodePVALID);

  AddInstTable(InstTable, "BITREV", 0x00c0, DecodeColdBit);
  AddInstTable(InstTable, "BYTEREV", 0x02c0, DecodeColdBit);
  AddInstTable(InstTable, "FF1", 0x04c0, DecodeColdBit);
  AddInstTable(InstTable, "STLDSR", 0x0000, DecodeSTLDSR);
  AddInstTable(InstTable, "INTOUCH", 0x0000, DecodeINTOUCH);
  AddInstTable(InstTable, "MOV3Q", 0x0000, DecodeMOV3Q);
  /* MOVEI? */
  AddInstTable(InstTable, "MVS", 0x7100, DecodeMVS_MVZ);
  AddInstTable(InstTable, "MVZ", 0x7180, DecodeMVS_MVZ);
  AddInstTable(InstTable, "SATS", 0x0000, DecodeSATS);
  AddInstTable(InstTable, "MAC" , 0x0000, DecodeMAC_MSAC);
  AddInstTable(InstTable, "MSAC", 0x0100, DecodeMAC_MSAC);
  AddInstTable(InstTable, "MACL" , 0x8000, DecodeMAC_MSAC);
  AddInstTable(InstTable, "MSACL", 0x8100, DecodeMAC_MSAC);
  AddInstTable(InstTable, "MOVCLR" , 0x0000, DecodeMOVCLR);
  AddInstTable(InstTable, "MAAAC" , 0x0001, DecodeMxxAC);
  AddInstTable(InstTable, "MASAC" , 0x0003, DecodeMxxAC);
  AddInstTable(InstTable, "MSAAC" , 0x0101, DecodeMxxAC);
  AddInstTable(InstTable, "MSSAC" , 0x0103, DecodeMxxAC);

  AddInstTable(InstTable, "CP0BCBUSY", 0xfcc0, DecodeCPBCBUSY);
  AddInstTable(InstTable, "CP1BCBUSY", 0xfec0, DecodeCPBCBUSY);
  AddInstTable(InstTable, "CP0LD", 0xfc00, DecodeCPLDST);
  AddInstTable(InstTable, "CP1LD", 0xfe00, DecodeCPLDST);
  AddInstTable(InstTable, "CP0ST", 0xfd00, DecodeCPLDST);
  AddInstTable(InstTable, "CP1ST", 0xff00, DecodeCPLDST);
  AddInstTable(InstTable, "CP0NOP", 0xfc00, DecodeCPNOP);
  AddInstTable(InstTable, "CP1NOP", 0xfe00, DecodeCPNOP);

  InstrZ = 0;
  AddPMMUReg("TC"   , eSymbolSize32Bit, 16); AddPMMUReg("DRP"  , eSymbolSize64Bit, 17);
  AddPMMUReg("SRP"  , eSymbolSize64Bit, 18); AddPMMUReg("CRP"  , eSymbolSize64Bit, 19);
  AddPMMUReg("CAL"  , eSymbolSize8Bit, 20);  AddPMMUReg("VAL"  , eSymbolSize8Bit, 21);
  AddPMMUReg("SCC"  , eSymbolSize8Bit, 22);  AddPMMUReg("AC"   , eSymbolSize16Bit, 23);
  AddPMMUReg("PSR"  , eSymbolSize16Bit, 24); AddPMMUReg("PCSR" , eSymbolSize16Bit, 25);
  AddPMMUReg("TT0"  , eSymbolSize32Bit,  2); AddPMMUReg("TT1"  , eSymbolSize32Bit,  3);
  AddPMMUReg("MMUSR", eSymbolSize16Bit, 24); AddPMMUReg(NULL   , eSymbolSizeUnknown, 0);

  inst_table_set_prefix_proc(InstTable, NULL, 0);

  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddMoto16Pseudo(InstTable, e_moto_pseudo_flags_be);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(FPUOps);
  order_array_free(PMMURegs);
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_68K(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for 68K
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_68K(char *pArg, TempResult *pResult)
{
  Word RegNum;

  if (DecodeRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_68K;
    pResult->Contents.RegDescr.compare = compare_reg_68k;
  }
  else if (DecodeFPRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = NativeFloatSize;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_68K;
    pResult->Contents.RegDescr.compare = compare_reg_68k;
  }
}

static Boolean DecodeAttrPart_68K(void)
{
  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_68K(void)
{
  if (AttrPartOpSize[0] == eSymbolSizeUnknown)
    AttrPartOpSize[0] = ((pCurrCPUProps->Family == eColdfire) ? eSymbolSize32Bit : eSymbolSize16Bit);
  OpSize = AttrPartOpSize[0];
  RelPos = 2;

  /* Nullanweisung */

  if ((*OpPart.str.p_str == '\0') && !*AttrPart.str.p_str && (ArgCnt == 0))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_68K(void)
{
  return Memo("REG");
}

static void SwitchTo_68K(void *pUser)
{
  const TFamilyDescr *p_descr = FindFamilyByName("680x0");

  TurnWords = True;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = 0x4e71;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 2;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

  pCurrCPUProps = (const tCPUProps*)pUser;

  DecodeAttrPart = DecodeAttrPart_68K;
  MakeCode = MakeCode_68K;
  IsDef = IsDef_68K;
  DissectReg = DissectReg_68K;
  InternSymbol = InternSymbol_68K;

  SwitchFrom = DeinitFields;
  InitFields();
  onoff_fpu_add();
  onoff_pmmu_add();
  onoff_supmode_add();
  if (onoff_test_and_set(e_onoff_reg_fullpmmu))
    SetFlag(&FullPMMU, FullPMMUName, True);
  AddONOFF(FullPMMUName, &FullPMMU  , FullPMMUName  , False);
  AddMoto16PseudoONOFF(True);

  SetFlag(&FullPMMU, FullPMMUName, !(pCurrCPUProps->SuppFlags & eFlagIntPMMU));
  NativeFloatSize = (pCurrCPUProps->Family == eColdfire) ? eSymbolSizeFloat64Bit : eSymbolSizeFloat96Bit;
}

static const tCtReg CtRegs_40[] =
{
  { "TC"   , 0x003 },
  { "ITT0" , 0x004 },
  { "ITT1" , 0x005 },
  { "DTT0" , 0x006 },
  { "DTT1" , 0x007 },
  { "MMUSR", 0x805 },
  { "URP"  , 0x806 },
  { "SRP"  , 0x807 },
  { "IACR0", 0x004 },
  { "IACR1", 0x005 },
  { "DACR0", 0x006 },
  { "DACR1", 0x007 },
  { NULL   , 0x000 },
},
CtRegs_2030[] =
{
  { "CAAR" , 0x802 },
  { NULL   , 0x000 },
},
CtRegs_2040[] =
{
  { "CACR" , 0x002 },
  { "MSP"  , 0x803 },
  { "ISP"  , 0x804 },
  { NULL   , 0x000 },
},
CtRegs_1040[] =
{
  { "SFC"  , 0x000 },
  { "DFC"  , 0x001 },
  { "USP"  , 0x800 },
  { "VBR"  , 0x801 },
  { NULL   , 0x000 },
};

static const tCtReg CtRegs_5202[] =
{
  { "CACR"   , 0x002 },
  { "ACR0"   , 0x004 },
  { "ACR1"   , 0x005 },
  { "VBR"    , 0x801 },
  { "SR"     , 0x80e },
  { "PC"     , 0x80f },
  { NULL     , 0x000 },
};

static const tCtReg CtRegs_5202_5204[] =
{
  { "RAMBAR" , 0xc04 },
  { "MBAR"   , 0xc0f },
  { NULL     , 0x000 },
};

static const tCtReg CtRegs_5202_5208[] =
{
  { "RGPIOBAR", 0x009},
  { "RAMBAR" , 0xc05 },
  { NULL     , 0x000 },
};

static const tCtReg CtRegs_5202_5307[] =
{
  { "ACR2"   , 0x006 },
  { "ACR3"   , 0x007 },
  { "RAMBAR0", 0xc04 },
  { "RAMBAR1", 0xc05 },
  { NULL     , 0x000 },
};

static const tCtReg CtRegs_5202_5329[] =
{
  { "RAMBAR" , 0xc05 },
  { NULL     , 0x000 },
};

static const tCtReg CtRegs_5202_5407[] =
{
  { "ACR2"   , 0x006 },
  { "ACR3"   , 0x007 },
  { "RAMBAR0", 0xc04 },
  { "RAMBAR1", 0xc05 },
  { "MBAR"   , 0xc0f },
  { NULL     , 0x000 },
};

static const tCtReg CtRegs_Cf_CPU[] =
{
  { "D0_LOAD"  , 0x080 },
  { "D1_LOAD"  , 0x081 },
  { "D2_LOAD"  , 0x082 },
  { "D3_LOAD"  , 0x083 },
  { "D4_LOAD"  , 0x084 },
  { "D5_LOAD"  , 0x085 },
  { "D6_LOAD"  , 0x086 },
  { "D7_LOAD"  , 0x087 },
  { "A0_LOAD"  , 0x088 },
  { "A1_LOAD"  , 0x089 },
  { "A2_LOAD"  , 0x08a },
  { "A3_LOAD"  , 0x08b },
  { "A4_LOAD"  , 0x08c },
  { "A5_LOAD"  , 0x08d },
  { "A6_LOAD"  , 0x08e },
  { "A7_LOAD"  , 0x08f },
  { "D0_STORE" , 0x180 },
  { "D1_STORE" , 0x181 },
  { "D2_STORE" , 0x182 },
  { "D3_STORE" , 0x183 },
  { "D4_STORE" , 0x184 },
  { "D5_STORE" , 0x185 },
  { "D6_STORE" , 0x186 },
  { "D7_STORE" , 0x187 },
  { "A0_STORE" , 0x188 },
  { "A1_STORE" , 0x189 },
  { "A2_STORE" , 0x18a },
  { "A3_STORE" , 0x18b },
  { "A4_STORE" , 0x18c },
  { "A5_STORE" , 0x18d },
  { "A6_STORE" , 0x18e },
  { "A7_STORE" , 0x18f },
  { "OTHER_A7" , 0x800 },
  { NULL       , 0x000 },
};

static const tCtReg CtRegs_Cf_EMAC[] =
{
  { "MACSR"    , 0x804 },
  { "MASK"     , 0x805 },
  { "ACC0"     , 0x806 },
  { "ACCEXT01" , 0x807 },
  { "ACCEXT23" , 0x808 },
  { "ACC1"     , 0x809 },
  { "ACC2"     , 0x80a },
  { "ACC3"     , 0x80b },
  { NULL       , 0x000 },
};

static const tCtReg CtRegs_MCF51[] =
{
  { "VBR"      , 0x801 },
  { "CPUCR"    , 0x802 },
  { NULL       , 0x000 },
};

static const tCPUProps CPUProps[] =
{
  /* 68881/68882 may be attached memory-mapped and emulated on pre-68020 devices */
  { "68008",    0x000ffffful, e68KGen1a, eCfISA_None  , eFlagExtFPU | eFlagLogCCR, { NULL } },
  { "68000",    0x00fffffful, e68KGen1a, eCfISA_None  , eFlagExtFPU | eFlagLogCCR, { NULL } },
  { "68010",    0x00fffffful, e68KGen1b, eCfISA_None  , eFlagExtFPU | eFlagLogCCR, { CtRegs_1040 } },
  { "68012",    0x7ffffffful, e68KGen1b, eCfISA_None  , eFlagExtFPU | eFlagLogCCR, { CtRegs_1040 } },
  { "MCF5202",  0xfffffffful, eColdfire, eCfISA_A     , eFlagIntFPU | eFlagIdxScaling, { CtRegs_5202 } },
  { "MCF5204",  0xfffffffful, eColdfire, eCfISA_A     , eFlagIntFPU | eFlagIdxScaling, { CtRegs_5202, CtRegs_5202_5204 } },
  { "MCF5206",  0xfffffffful, eColdfire, eCfISA_A     , eFlagIntFPU | eFlagIdxScaling, { CtRegs_5202, CtRegs_5202_5204 } },
  { "MCF5208",  0xfffffffful, eColdfire, eCfISA_APlus , eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5208, CtRegs_Cf_CPU, CtRegs_Cf_EMAC } }, /* V2 */
  { "MCF52274", 0xfffffffful, eColdfire, eCfISA_APlus , eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5208, CtRegs_Cf_CPU, CtRegs_Cf_EMAC } }, /* V2 */
  { "MCF52277", 0xfffffffful, eColdfire, eCfISA_APlus , eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5208, CtRegs_Cf_CPU, CtRegs_Cf_EMAC } }, /* V2 */
  { "MCF5307",  0xfffffffful, eColdfire, eCfISA_A     , eFlagIntFPU | eFlagIdxScaling | eFlagMAC, { CtRegs_5202, CtRegs_5202_5307 } }, /* V3 */
  { "MCF5329",  0xfffffffful, eColdfire, eCfISA_APlus , eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5329 } }, /* V3 */
  { "MCF5373",  0xfffffffful, eColdfire, eCfISA_APlus , eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5329 } }, /* V3 */
  { "MCF5407",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4 */
  { "MCF5470",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4e */
  { "MCF5471",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4e */
  { "MCF5472",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4e */
  { "MCF5473",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4e */
  { "MCF5474",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4e */
  { "MCF5475",  0xfffffffful, eColdfire, eCfISA_B     , eFlagBranch32 | eFlagIntFPU | eFlagIdxScaling | eFlagMAC | eFlagEMAC, { CtRegs_5202, CtRegs_5202_5407 } }, /* V4e */
  { "MCF51QM",  0xfffffffful, eColdfire, eCfISA_C     , eFlagBranch32 | eFlagMAC | eFlagIdxScaling | eFlagEMAC, { CtRegs_MCF51 } }, /* V1 */
  { "68332",    0xfffffffful, eCPU32   , eCfISA_None  , eFlagBranch32 | eFlagLogCCR | eFlagIdxScaling, { CtRegs_1040 } },
  { "68340",    0xfffffffful, eCPU32   , eCfISA_None  , eFlagBranch32 | eFlagLogCCR | eFlagIdxScaling, { CtRegs_1040 } },
  { "68360",    0xfffffffful, eCPU32   , eCfISA_None  , eFlagBranch32 | eFlagLogCCR | eFlagIdxScaling, { CtRegs_1040 } },
  { "68020",    0xfffffffful, e68KGen2 , eCfISA_None  , eFlagBranch32 | eFlagLogCCR | eFlagIdxScaling | eFlagExtFPU | eFlagCALLM_RTM, { CtRegs_1040, CtRegs_2040, CtRegs_2030 } },
  { "68030",    0xfffffffful, e68KGen2 , eCfISA_None  , eFlagBranch32 | eFlagLogCCR | eFlagIdxScaling | eFlagExtFPU | eFlagIntPMMU, { CtRegs_1040, CtRegs_2040, CtRegs_2030 } },
  /* setting eFlagExtFPU assumes instructions of external FPU are emulated/provided by M68040FPSP! */
  { "68040",    0xfffffffful, e68KGen3 , eCfISA_None  , eFlagBranch32 | eFlagLogCCR | eFlagIdxScaling | eFlagIntPMMU | eFlagExtFPU | eFlagIntFPU, { CtRegs_1040, CtRegs_2040, CtRegs_40 } },
  { NULL   ,    0           , e68KGen1a, eCfISA_None  , eFlagNone, { NULL } },
};

void code68k_init(void)
{
  const tCPUProps *pProp;
  for (pProp = CPUProps; pProp->pName; pProp++)
    (void)AddCPUUser(pProp->pName, SwitchTo_68K, (void*)pProp, NULL);
}
