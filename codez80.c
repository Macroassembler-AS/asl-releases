/* codez80.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Zilog Z80/180/280/380                                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmcode.h"
#include "asmallg.h"
#include "nlmessages.h"
#include "as.rsc"
#include "onoff_common.h"
#include "assume.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "cpu2phys.h"
#include "function.h"
#include "onoff_common.h"
#include "errmsg.h"
#include "headids.h"

#include "codez80.h"

/*-------------------------------------------------------------------------*/
/* Praefixtyp */

typedef enum
{
  ePrefixNone,
  ePrefixN,   /* default processing */
  ePrefixW,   /* word processing */
  ePrefixLW,  /* long word processing */
  ePrefixIN,  /* no extra bytes in argument */
  ePrefixIB,  /* one byte more in argument */
  ePrefixIW   /* one word more in argument */
} tOpPrefix;

typedef struct
{
  tOpPrefix s_prefix, i_prefix;
} ddir_prefix_pair_t;

typedef enum
{
  e_core_sharp,
  e_core_z80,
  e_core_z80u,
  e_core_z180,
  e_core_z280,
  e_core_r2000,
  e_core_ez80,
  e_core_z380
} cpu_core_t;

typedef enum
{
  e_core_mask_sharp = 1 << e_core_sharp,
  e_core_mask_z80 = 1 << e_core_z80,
  e_core_mask_z80u = 1 << e_core_z80u,
  e_core_mask_z180 = 1 << e_core_z180,
  e_core_mask_z280 = 1 << e_core_z280,
  e_core_mask_r2000 = 1 << e_core_r2000,
  e_core_mask_ez80 = 1 << e_core_ez80,
  e_core_mask_z380 = 1 << e_core_z380,
  e_core_mask_no_sharp = e_core_mask_z80 | e_core_mask_z80u | e_core_mask_z180 | e_core_mask_z280 | e_core_mask_r2000 | e_core_mask_ez80 | e_core_mask_z380,
  e_core_mask_all = e_core_mask_sharp | e_core_mask_no_sharp,
  e_core_mask_min_z180 = e_core_mask_z180 | e_core_mask_r2000 | e_core_mask_ez80 | e_core_mask_z380,

  e_core_mask_z280_io_privileged = 1 << 14,
  e_core_mask_z280_privileged = 1 << 15
} cpu_core_mask_t;

typedef enum
{
  e_core_flag_none = 0,
  e_core_flag_i_8bit = 1 << 0,
  e_core_flag_no_xio = 1 << 1
} cpu_core_flags_t;

typedef struct
{
  const char *p_name;
  cpu_core_t core;
  cpu_core_flags_t core_flags;
} cpu_props_t;

#ifdef __cplusplus
# include "codez80.hpp"
#endif

#define LWordFlagName  "INLWORDMODE"

#define ModNone (-1)
#define ModReg8 1
#define ModReg16 2
#define ModIndReg16 3
#define ModImm 4
#define ModAbs 5
#define ModRef 6
#define ModInt 7
#define ModSPRel 8
#define ModIndReg8 9
#define ModSPAdd 10
#define ModHLInc 11
#define ModHLDec 12
#define ModIOAbs 13
#define ModImmIsAbs 14
#define ModMB 15
/* Z280-specific */
#define ModX 16
#define ModSR 17
#define ModRA 18
#define ModBX 19
#define ModUSP 20

#define MModReg8 (1ul << ModReg8)
#define MModReg16 (1ul << ModReg16)
#define MModIndReg16 (1ul << ModIndReg16)
#define MModImm (1ul << ModImm)
#define MModAbs (1ul << ModAbs)
#define MModRef (1ul << ModRef)
#define MModInt (1ul << ModInt)
#define MModSPRel (1ul << ModSPRel)
#define MModIndReg8 (1ul << ModIndReg8)
#define MModSPAdd (1ul << ModSPAdd)
#define MModHLInc (1ul << ModHLInc)
#define MModHLDec (1ul << ModHLDec)
#define MModIOAbs (1ul << ModIOAbs)
#define MModImmIsAbs (1ul << ModImmIsAbs)
#define MModMB (1ul << ModMB)
#define MModX (1ul << ModX)
#define MModSR (1ul << ModSR)
#define MModRA (1ul << ModRA)
#define MModBX (1ul << ModBX)
#define MModUSP (1ul << ModUSP)

/* These masks deliberately omit the (special) 
   Sharp/Gameboy addressing modes: */

#define MModNoImm (MModReg8 | MModReg16 | MModIndReg16 | MModAbs | MModRef | MModInt | MModSPRel)
#define MModAll (MModReg8 | MModReg16 | MModIndReg16 | MModImm | MModAbs | MModRef | MModInt | MModSPRel)

#define IXPrefix 0xdd
#define IYPrefix 0xfd

#define AccReg 7
#define MReg 6
#define HReg 4
#define LReg 5
#define CReg 1

#define BCReg 0
#define DEReg 1
#define HLReg 2
#define SPReg 3

/*-------------------------------------------------------------------------*/
/* Instruktionsgruppendefinitionen */

typedef struct
{
  cpu_core_mask_t core_mask;
  Word Code;
} BaseOrder;

typedef struct
{
  const char *Name;
  Byte Code;
} Condition;

/*-------------------------------------------------------------------------*/

typedef struct
{
  ShortInt mode;
  LongWord value;
  LongInt disp_acc;
  int count;
  tSymbolFlags value_flags;
  Byte part;
  Byte values[4];
} adr_vals_t;

#define Z280_NUM_PDR 16

typedef struct
{
  as_assume_rec_t *p_assumes;
  size_t assumes_cnt;
  LongInt reg_i, reg_mmumcr, reg_upd[Z280_NUM_PDR], reg_spd[Z280_NUM_PDR];
  Boolean translation_enabled, program_data_separation, computed_for_supmode;
} z280_data_t;

static Byte PrefixCnt;
static tSymbolSize OpSize;
static IntType code_segment_int_type;

static BaseOrder *FixedOrders;
static BaseOrder *AccOrders;
static BaseOrder *HLOrders;
static Condition *Conditions;

static const cpu_props_t *p_curr_cpu_props;

static z280_data_t *p_z280_data;

#define is_sharp() (p_curr_cpu_props->core == e_core_sharp)
#define is_z80u() (p_curr_cpu_props->core == e_core_z80u)
#define is_z180() (p_curr_cpu_props->core == e_core_z180)
#define is_r2000() (p_curr_cpu_props->core == e_core_r2000)
#define is_ez80() (p_curr_cpu_props->core == e_core_ez80)
#define is_z280() (p_curr_cpu_props->core == e_core_z280)
#define is_z380() (p_curr_cpu_props->core == e_core_z380)

static Boolean MayLW,             /* Instruktion erlaubt 32 Bit */
               ExtFlag,           /* Prozessor im 4GByte-Modus ? */
               LWordFlag;         /* 32-Bit-Verarbeitung ? */

static ddir_prefix_pair_t curr_prefix_pair, /* Mom. explizit erzeugter Praefix */
                          last_prefix_pair; /* Von der letzten Anweisung generierter Praefix */

static LongInt Reg_CBAR,
               Reg_BBR,
               Reg_CBR,
               Reg_ADL,
               Reg_MBASE;
static const char Reg8Names[] = "BCDEHL*A";
static int Reg16Cnt;
static const char Reg16Names[][3] = { "BC", "DE", "HL", "SP", "IX", "IY" };

/*--------------------------------------------------------------------------*/
/* Praefix dazuaddieren */

static tOpPrefix DecodePrefix(const char *pArg)
{
  const char PrefNames[][3] = { "N", "W", "LW", "IN", "IB", "IW", "" };
  tOpPrefix Result;

  for (Result = ePrefixN; PrefNames[Result - 1][0]; Result++)
    if (!as_strcasecmp(pArg, PrefNames[Result - 1]))
      return Result;
  return ePrefixNone;
}

/*!------------------------------------------------------------------------
 * \fn     reset_adr_vals(adr_vals_t *p_vals)
 * \brief  initialize/clear structure for decoded address expression
 * \param  p_vals structure to init
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = ModNone;
  p_vals->value = 0;
  p_vals->disp_acc = 0;
  p_vals->count = 0;
  p_vals->value_flags = eSymbolFlag_None;
  p_vals->part = 0;
}

/*--------------------------------------------------------------------------*/
/* Code fuer Praefix bilden */

static void GetPrefixCode(Byte *p_dest, const ddir_prefix_pair_t *p_pair)
{
  switch (p_pair->i_prefix)
  {
    case ePrefixIW:
      switch (p_pair->s_prefix)
      {
        case ePrefixLW:
          p_dest[0] = 0xfd; p_dest[1] = 0xc2; break;
        case ePrefixW:
          p_dest[0] = 0xdd; p_dest[1] = 0xc2; break;
        case ePrefixN:
        case ePrefixNone:
          p_dest[0] = 0xfd; p_dest[1] = 0xc3; break;
        default:
          assert(0);
      }
      break;
    case ePrefixIB:
      switch (p_pair->s_prefix)
      {
        case ePrefixLW:
          p_dest[0] = 0xfd; p_dest[1] = 0xc1; break;
        case ePrefixW:
          p_dest[0] = 0xdd; p_dest[1] = 0xc1; break;
        case ePrefixN:
        case ePrefixNone:
          p_dest[0] = 0xdd; p_dest[1] = 0xc3; break;
        default:
          assert(0);
      }
      break;
    case ePrefixIN:
    case ePrefixNone:
      switch (p_pair->s_prefix)
      {
        case ePrefixLW:
          p_dest[0] = 0xfd; p_dest[1] = 0xc0; break;
        case ePrefixW:
          p_dest[0] = 0xdd; p_dest[1] = 0xc0; break;
        default:
          assert(0);
      }
      break;
    default:
      assert(0);
  }
}

static void prefix_pair_clear(ddir_prefix_pair_t *p_pair)
{
  p_pair->i_prefix = p_pair->s_prefix = ePrefixNone;
}

static Boolean prefix_pair_has_overrides(const ddir_prefix_pair_t *p_pair)
{
  return ((p_pair->i_prefix != ePrefixIN) && (p_pair->i_prefix != ePrefixNone))
      || ((p_pair->s_prefix != ePrefixN) && (p_pair->s_prefix != ePrefixNone));
}

static Boolean prefix_pair_equal(const ddir_prefix_pair_t *p_pair1, const ddir_prefix_pair_t *p_pair2)
{
  return (p_pair1->i_prefix == p_pair2->i_prefix)
      && (p_pair1->s_prefix == p_pair2->s_prefix);
}

static void prefix_pair_update(ddir_prefix_pair_t *p_pair, tOpPrefix prefix)
{
  if ((prefix == ePrefixIN) || (prefix == ePrefixIB) || (prefix == ePrefixIW))
    p_pair->i_prefix = prefix;
  else if ((prefix == ePrefixN) || (prefix == ePrefixW) || (prefix == ePrefixLW))
    p_pair->s_prefix = prefix;
}

/*--------------------------------------------------------------------------*/
/* DD-Praefix addieren, nur EINMAL pro Instruktion benutzen! */

static void ChangeDDPrefix(tOpPrefix Prefix)
{
  ddir_prefix_pair_t act_pair;

  act_pair = last_prefix_pair;
  prefix_pair_update(&act_pair, Prefix);
  if (!prefix_pair_equal(&last_prefix_pair, &act_pair))
  {
    if (prefix_pair_has_overrides(&last_prefix_pair))
      RetractWords(2);
    if (prefix_pair_has_overrides(&act_pair))
    {
      memmove(&BAsmCode[2], &BAsmCode[0], PrefixCnt);
      PrefixCnt += 2;
      GetPrefixCode(BAsmCode + 0, &act_pair);
    }
  }
}

/*--------------------------------------------------------------------------*/
/* IX/IY used ? */

static Boolean IndexPrefix(void)
{
  return   ((PrefixCnt > 0)
         && ((BAsmCode[PrefixCnt - 1] == IXPrefix)
          || (BAsmCode[PrefixCnt - 1] == IYPrefix)));
}

/*--------------------------------------------------------------------------*/
/* Wortgroesse ? */

static Boolean InLongMode(void)
{
  switch (last_prefix_pair.s_prefix)
  {
    case ePrefixW:
      return False;
    case ePrefixLW:
      return MayLW;
    case ePrefixN:
    case ePrefixNone:
      return LWordFlag && MayLW;
    default:
      return False;
  }
}

static IntType get_code_segment_int_type(void)
{
  return (is_z380() && !ExtFlag) ? UInt16 : code_segment_int_type;
}

/*!------------------------------------------------------------------------
 * \fn     check_ez80_mpage(LongWord address, unsigned eval_flags, tSymbolSize op_size, const tStrComp *p_arg)
 * \brief  check whether address is in current 64K Z80 mode page
 * \param  address address to check
 * \param  eval_flags result flags from address evaluation
 * \param  op_size actual address size in code
 * \param  p_arg related source argument
 * ------------------------------------------------------------------------ */

static void check_ez80_mpage(LongWord address, unsigned eval_flags, tSymbolSize op_size, const tStrComp *p_arg)
{
  if ((op_size != eSymbolSize24Bit)
   && !mFirstPassUnknownOrQuestionable(eval_flags)
   && (((address >> 16) & 0xff) != (unsigned)Reg_MBASE))
    WrStrErrorPos(ErrNum_InAccPage, p_arg);
}

/*!------------------------------------------------------------------------
 * \fn     EvalAbsAdrExpression(const tStrComp *pArg, tEvalResult *pEvalResult)
 * \brief  evaluate absolute address, range is targent-dependant
 * \param  pArg source argument
 * \param  pEvalResult sideband params
 * \return address value
 * ------------------------------------------------------------------------ */

static LongWord EvalAbsAdrExpression(const tStrComp *pArg, tEvalResult *pEvalResult)
{
  LongWord ret = EvalStrIntExpressionWithResult(pArg, get_code_segment_int_type(), pEvalResult);

  if (is_ez80() && pEvalResult->OK)
    check_ez80_mpage(ret, pEvalResult->Flags, AttrPartOpSize[1], pArg);

  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     valid_cbar(void)
 * \brief  allowed Z180 CBAR value?
 * \return True if valid
 * ------------------------------------------------------------------------ */

static Boolean valid_cbar(void)
{
  return ((Reg_CBAR & 0x0f) <= ((Reg_CBAR >> 4) & 0x0f));
}

/*!------------------------------------------------------------------------
 * \fn     update_z180_areas(void)
 * \brief  recompute Z180 mapped areas
 * ------------------------------------------------------------------------ */

static void update_z180_areas(void)
{
  if (valid_cbar())
  {
    Word common_area_start = ((Reg_CBAR >> 4) & 0x0f) << 12,
         bank_area_start = (Reg_CBAR & 0x0f) << 12;

    cpu_2_phys_area_clear(SegCode);
    cpu_2_phys_area_clear(SegData);

    /* Common Area 0 */

    if (bank_area_start > 0)
    {
      cpu_2_phys_area_add(SegCode, 0, 0, bank_area_start);
      cpu_2_phys_area_add(SegData, 0, 0, bank_area_start);
    }

    /* Bank Area */

    if (common_area_start > bank_area_start)
    {
      cpu_2_phys_area_add(SegCode, bank_area_start, (Reg_BBR << 12) + bank_area_start, common_area_start - bank_area_start);
      cpu_2_phys_area_add(SegData, bank_area_start, (Reg_BBR << 12) + bank_area_start, common_area_start - bank_area_start);
    }

    /* Common Area 1 - always present since upper nibble of CBAR is always < 0x10 */

    cpu_2_phys_area_add(SegCode, common_area_start, (Reg_CBR << 12) + common_area_start, 0x10000ul - common_area_start);
    cpu_2_phys_area_add(SegData, common_area_start, (Reg_CBR << 12) + common_area_start, 0x10000ul - common_area_start);

    /* this *SHOULD* be a NOP, since completely filled the 64K CPU space: */

    cpu_2_phys_area_fill(SegCode, 0, 0xffff);
    cpu_2_phys_area_fill(SegData, 0, 0xffff);
  }
}

/*!------------------------------------------------------------------------
 * \fn     check_cbar(void)
 * \brief  check for valid Z180 CBAR value
 * ------------------------------------------------------------------------ */

static void check_cbar(void)
{
  if (valid_cbar())
    update_z180_areas();
  else
    WrError(ErrNum_InvCBAR);
}

/*!------------------------------------------------------------------------
 * \fn     update_z280_areas(void)
 * \brief  recompute Z280 MMU mapping
 * ------------------------------------------------------------------------ */

static void check_z280_pdr(as_addrspace_t space, LongInt pdr_value, unsigned index, LongWord page_size)
{
  if (pdr_value & 0x08)
    cpu_2_phys_area_add(space, index * page_size, (pdr_value & ~((page_size >> 8) - 1)) << 8, page_size);
}

static void update_z280_areas(void)
{
  Word te_pd = p_z280_data->reg_mmumcr >> (SupAllowed ? 10 : 14);

  cpu_2_phys_area_clear(SegCode);
  cpu_2_phys_area_clear(SegData);

  p_z280_data->translation_enabled = !!(te_pd & 2);
  p_z280_data->program_data_separation = p_z280_data->translation_enabled && !!(te_pd & 1);

  if (p_z280_data->translation_enabled)
  {
    int z;
    const LongInt *p_pdr = SupAllowed ? p_z280_data->reg_spd : p_z280_data->reg_upd;

    /* No data/code separation */

    if (p_z280_data->program_data_separation)
    {
      for (z = 0; z < Z280_NUM_PDR / 2; z++)
      {
        check_z280_pdr(SegData, p_pdr[z], z, 8192);
        check_z280_pdr(SegCode, p_pdr[z + (Z280_NUM_PDR / 2)], z, 8192);
      }
    }
    else
    {
      for (z = 0; z < Z280_NUM_PDR; z++)
      {
        check_z280_pdr(SegCode, p_pdr[z], z, 4096);
        check_z280_pdr(SegData, p_pdr[z], z, 4096);
      }
    }
    cpu_2_phys_area_set_cpu_end(SegCode, 0xffff);
    cpu_2_phys_area_set_cpu_end(SegData, 0xffff);
  }
  else
  {
    cpu_2_phys_area_add(SegData, 0, 0, 0xffff);
    cpu_2_phys_area_add(SegCode, 0, 0, 0xffff);
  }
  p_z280_data->computed_for_supmode = SupAllowed;
}

/*!------------------------------------------------------------------------
 * \fn     check_z280_areas_current(void)
 * \brief  check if user/supervisor mode changed since last MMU mapping computation
 * ------------------------------------------------------------------------ */

static void check_z280_areas_current(void)
{
  if (SupAllowed != p_z280_data->computed_for_supmode)
    update_z280_areas();
}

/*!------------------------------------------------------------------------
 * \fn     phys_2_cpu(as_addrspace_t addr_space, LongWord *p_address, tSymbolFlags flags, const tStrComp *p_arg)
 * \brief  translate address in physical address space to CPU address space
 * \param  addr_space CODE/DATA
 * \param  p_address address to translate (in/out)
 * \param  flags expression evaluation flags
 * \param  p_arg associated source argument
 * \return True if translation succeeded
 * ------------------------------------------------------------------------ */

static Boolean phys_2_cpu(as_addrspace_t addr_space, LongWord *p_address, tSymbolFlags flags, const tStrComp *p_arg)
{
  LargeWord address = *p_address;

  /* Physical address in range? */

  if (!mFirstPassUnknownOrQuestionable(flags)
   && !ChkRangeByType(*p_address, code_segment_int_type, p_arg))
    return False;

  /* Actual translation: */

  if ((is_z180() || p_z280_data->translation_enabled)
   && !def_phys_2_cpu(addr_space, &address))
  {
    if (p_arg)
      WrStrErrorPos(ErrNum_InAccPageErr, p_arg);
    else
      WrError(ErrNum_CurrPCInInAccPageErr);
    return False;
  }
  *p_address = address;

  /* Check for logical 64K address ranges is done by caller */

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     get_logical_prog_counter(LongWord *p_log_pc, LongInt pcrel_offset)
 * \brief  retrieve value of current PC in CPU address space
 * \param  p_log_pc returns logical address
 * \param  pcrel_offset offset to current instruction's start address
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean get_logical_prog_counter(LongWord *p_log_pc, LongInt pcrel_offset)
{
  *p_log_pc = EProgCounter() + pcrel_offset;
  return phys_2_cpu(SegCode, p_log_pc, eSymbolFlag_None, NULL);
}

/*==========================================================================*/
/* Adressparser */

/*!------------------------------------------------------------------------
 * \fn     DecodeReg8Core(const char *p_asc, Byte *p_ret)
 * \brief  parse 8 bit register
 * \param  p_asc source argument
 * \param  p_ret return buffer
 * \return true if valid register name
 * ------------------------------------------------------------------------ */

#define EncIXReg ((IXPrefix & 0xf0) | HLReg)
#define EncIYReg ((IYPrefix & 0xf0) | HLReg)

static Boolean DecodeReg8Core(const char *p_asc, Byte *p_ret)
{
  const char *p_pos;

  switch (strlen(p_asc))
  {
    case 1:
      p_pos = strchr(Reg8Names, as_toupper(p_asc[0]));
      if (!p_pos)
        return False;
      *p_ret = p_pos - Reg8Names;
      return (*p_ret != 6);
    case 3:
    {
      char ix = toupper(p_asc[1]);

      if ((toupper(p_asc[0]) != 'I')
       || ((ix != 'X') && (ix != 'Y')))
        return False;
      switch (toupper(p_asc[2]))
      {
        case 'L':
          *p_ret = LReg | (((ix == 'X') ? IXPrefix : IYPrefix) & 0xf0);
          return True;
        case 'H':
          /* do not allow IXH/IYH on Z380 */
          if (is_z380())
            return False;
          else
            goto ir_high;
        case 'U':
          /* do not allow IXU/IYU on eZ80/Z280 */
          if (is_ez80() || is_z280())
            return False;
          else
            goto ir_high;
        ir_high:
          *p_ret = HReg | (((ix == 'X') ? IXPrefix : IYPrefix) & 0xf0);
          return True;
        default:
          return False;
      }
    }  
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg16Core(const char *p_asc, Byte *p_ret)
 * \brief  parse 16 bit register
 * \param  p_asc source argument
 * \param  p_ret return buffer
 * \return true if valid register name
 * ------------------------------------------------------------------------ */

static Boolean DecodeReg16Core(const char *p_asc, Byte *p_ret)
{
  int z;

  for (z = 0; z < Reg16Cnt; z++)
    if (!as_strcasecmp(p_asc, Reg16Names[z]))
    {
      if (z <= 3)
        *p_ret = z;
      else
        *p_ret = HLReg | (((z == 4) ? IXPrefix : IYPrefix) & 0xf0);
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *p_arg, Byte *p_ret, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
 * \brief  check whether argument is a CPU register or user-defined register alias
 * \param  p_arg argument
 * \param  p_value resulting register # if yes
 * \param  p_size resulting register size if yes
 * \param  req_size requested register size
 * \param  must_be_reg expecting register or maybe not?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static Boolean chk_reg_size(tSymbolSize req_size, tSymbolSize act_size)
{
  return (req_size == eSymbolSizeUnknown)
      || (req_size == act_size);
}

static tRegEvalResult DecodeReg(const tStrComp *p_arg, Byte *p_ret, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
{
  tRegEvalResult reg_eval_result;
  tEvalResult eval_result;
  tRegDescr reg_descr;

  if (DecodeReg8Core(p_arg->str.p_str, p_ret))
  {
    eval_result.DataSize = eSymbolSize8Bit;
    reg_eval_result = eIsReg;
  }
  else if (DecodeReg16Core(p_arg->str.p_str, p_ret))
  {
    eval_result.DataSize = eSymbolSize16Bit;
    reg_eval_result = eIsReg;
  }
  else
  {
    reg_eval_result = EvalStrRegExpressionAsOperand(p_arg, &reg_descr, &eval_result, eSymbolSizeUnknown, must_be_reg);
    if (reg_eval_result == eIsReg)
      *p_ret = reg_descr.Reg;
  }

  if (reg_eval_result == eIsReg)
  {
    if (!chk_reg_size(req_size, eval_result.DataSize))
    {
      WrStrErrorPos(ErrNum_InvOpSize, p_arg);
      reg_eval_result = must_be_reg ? eIsNoReg : eRegAbort;
    }
  }

  if (p_size) *p_size = eval_result.DataSize;
  return reg_eval_result;
}

static Boolean IsSym(char ch)
{
  return ((ch == '_')
       || ((ch >= '0') && (ch <= '9'))
       || ((ch >= 'A') && (ch <= 'Z'))
       || ((ch >= 'a') && (ch <= 'z')));
}

typedef struct
{
  as_eval_cb_data_t cb_data;
  Byte addr_reg;
  tSymbolSize addr_reg_size;
} z80_eval_cb_data_t;

#define EncHL_IXReg ((IXPrefix & 0xf0) | HLReg | 8)
#define EncHL_IYReg ((IYPrefix & 0xf0) | HLReg | 8)
#define EncIX_IYReg ((IYPrefix & 0xf0) | 8)
#define EncPCReg 7

DECLARE_AS_EVAL_CB(z80_eval_cb)
{
  z80_eval_cb_data_t *p_z80_eval_cb_data = (z80_eval_cb_data_t*)p_data;
  tSymbolSize this_reg_size;
  Byte this_reg;
  tRegEvalResult eval_result;

  /* special case for GameBoy/Sharp: FF00 always allowed, independent of radix: */

  if (!as_strcasecmp(p_arg->str.p_str, "FF00"))
  {
    as_tempres_set_int(p_res, 0xff00);
    return e_eval_ok;
  }

  /* For the moment, I do not want to allow PC as register like any else.
     Its use for (PC+d) on Z280 is too specific: */

  if (!as_strcasecmp(p_arg->str.p_str, "PC"))
  {
    eval_result = eIsReg;
    this_reg = EncPCReg;
    this_reg_size = eSymbolSize16Bit;
  }
  else
    eval_result = DecodeReg(p_arg, &this_reg, &this_reg_size, eSymbolSizeUnknown, False);
  switch (eval_result)
  {
    case eIsReg:
      if (!as_eval_cb_data_stack_plain_add(p_data->p_stack))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return e_eval_fail;
      }
      switch (p_z80_eval_cb_data->addr_reg)
      {
        case 0xff:
          p_z80_eval_cb_data->addr_reg = this_reg;
          p_z80_eval_cb_data->addr_reg_size = this_reg_size;
          as_tempres_set_int(p_res, 0);
          return e_eval_ok;
        case HLReg: /* (HL) -> ... */
          if ((p_z80_eval_cb_data->addr_reg_size != eSymbolSize16Bit) || (this_reg_size != eSymbolSize16Bit))
            goto bad_reg;
          else if (this_reg == EncIXReg) /* ... -> (HL+IX) */
          {
            p_z80_eval_cb_data->addr_reg = EncHL_IXReg;
            as_tempres_set_int(p_res, 0);
            return e_eval_ok;
          }
          else if (this_reg == EncIYReg) /* ... -> (HL+IY) */
          {
            p_z80_eval_cb_data->addr_reg = EncHL_IYReg;
            as_tempres_set_int(p_res, 0);
            return e_eval_ok;
          }
          else
            goto bad_reg;
        case EncIXReg: /* (IX) -> ... */
          if ((p_z80_eval_cb_data->addr_reg_size != eSymbolSize16Bit) || (this_reg_size != eSymbolSize16Bit))
            goto bad_reg;
          else if (this_reg == HLReg) /* ... -> (HL+IX) */
          {
            p_z80_eval_cb_data->addr_reg = EncHL_IXReg;
            as_tempres_set_int(p_res, 0);
            return e_eval_ok;
          }
          else if (this_reg == EncIYReg) /* ... -> (IX+IY) */
          {
            p_z80_eval_cb_data->addr_reg = EncIX_IYReg;
            as_tempres_set_int(p_res, 0);
            return e_eval_ok;
          }
          else
            goto bad_reg;
        case EncIYReg: /* (IY) -> ... */
          if ((p_z80_eval_cb_data->addr_reg_size != eSymbolSize16Bit) || (this_reg_size != eSymbolSize16Bit))
            goto bad_reg;
          else if (this_reg == HLReg) /* ... -> (HL+IY) */
          {
            p_z80_eval_cb_data->addr_reg = EncHL_IYReg;
            as_tempres_set_int(p_res, 0);
            return e_eval_ok;
          }
          else if (this_reg == EncIXReg) /* ... -> (IX+IY) */
          {
            p_z80_eval_cb_data->addr_reg = EncIX_IYReg;
            as_tempres_set_int(p_res, 0);
            return e_eval_ok;
          }
          else
            goto bad_reg;
        default:
        bad_reg:
          WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
          return e_eval_fail;
      }
    case eRegAbort:
      return e_eval_fail;
    default:
       return e_eval_none;
  }
}

static void z380_extend_ib_iw(adr_vals_t *p_vals, LongWord value)
{
  tOpPrefix this_op_prefix = ePrefixNone;

  /* A previously given, explicit DDIR instruction may
     request a longer operand than needed by value: */

  if ((value > 0xfffffful) || (last_prefix_pair.i_prefix == ePrefixIW))
    this_op_prefix = ePrefixIW;
  else if ((value > 0xffffu) || (last_prefix_pair.i_prefix == ePrefixIB))
    this_op_prefix = ePrefixIB;
  if (this_op_prefix != ePrefixNone)
  {
    p_vals->values[p_vals->count++] = (value >> 16) & 0xff;
    if (ePrefixIW == this_op_prefix)
      p_vals->values[p_vals->count++] = ((value >> 24) & 0xff);
    ChangeDDPrefix(this_op_prefix);
  }
}

static void abs_2_adrvals(adr_vals_t *p_vals, LongWord address)
{
  p_vals->value = address;
  p_vals->values[0] = address & 0xff;
  p_vals->values[1] = (address >> 8) & 0xff;
  p_vals->count = 2;

  /* Z380: add more bytes and IB/IW prefix if necessary: */

  if (ExtFlag)
    z380_extend_ib_iw(p_vals, address);

  /* eZ80: 24 bits if .IL mode */

  else if (is_ez80() && (AttrPartOpSize[1] == eSymbolSize24Bit))
    p_vals->values[p_vals->count++] = (address >> 16) & 0xff;
}

static void append_prefix(Byte prefix)
{
  BAsmCode[PrefixCnt++] = prefix;
}

static Boolean construct_ra(const tStrComp *p_arg, adr_vals_t *p_vals, LongInt pcrel_offset)
{
  /* Do not call twice and not for any other addressing mode */

  if ((p_vals->count > 0) || (p_vals->mode != ModRA))
    return True;

  if (pcrel_offset)
  {
    LongWord log_pc;
    
    if (!get_logical_prog_counter(&log_pc, pcrel_offset))
      return False;
    p_vals->disp_acc -= log_pc;
  }

  /* PC and target address are both in 16 bit CPU address space.
     So the distance can never be too large: */

#if 0
  if (!mFirstPassUnknownOrQuestionable(p_vals->value_flags)
   && !ChkRangeByType(p_vals->disp_acc, SInt16, p_arg))
    return False;
#endif
  (void)p_arg;

  p_vals->values[0] = p_vals->disp_acc & 0xff;
  p_vals->values[1] = (p_vals->disp_acc >> 8) & 0xff;
  p_vals->count = 2;
  p_vals->mode = ModRA;

  /* Many instructions encode RA like X with xx=000: */

  p_vals->part = 0;
  return True;
}

static ShortInt decode_adr_rel(const tStrComp *pArg, adr_vals_t *p_vals, LongWord ModeMask, LongInt pcrel_offset)
{
  Integer AdrInt;
#if 0
  int z, l;
  LongInt AdrLong;
#endif
  Boolean OK, is_indirect, is_pcrel;
  tEvalResult EvalResult;

  reset_adr_vals(p_vals);

  /* 0. Sonderregister */

  if (!as_strcasecmp(pArg->str.p_str, "R"))
  {
    p_vals->mode = ModRef;
    goto found;
  }

  if (!as_strcasecmp(pArg->str.p_str, "I"))
  {
    p_vals->mode = ModInt;
    goto found;
  }

  if (is_ez80() && !as_strcasecmp(pArg->str.p_str, "MB"))
  {
    p_vals->mode = ModMB;
    goto found;
  }

  if (is_z280() && !as_strcasecmp(pArg->str.p_str, "USP"))
  {
    p_vals->mode = ModUSP;
    goto found;
  }

  /* 1. 8/16 bit registers ? */

  switch (DecodeReg(pArg, &p_vals->part, &EvalResult.DataSize, eSymbolSizeUnknown, False))
  {
    case eRegAbort:
      goto found;
    case eIsReg:
      if (p_vals->part & 0xf0)
        append_prefix((p_vals->part & 0xf0) | 0x0d);
      p_vals->part &= (EvalResult.DataSize == eSymbolSize8Bit) ? 7 : 3;
      p_vals->mode = (EvalResult.DataSize == eSymbolSize8Bit) ? ModReg8 : ModReg16;
      goto found;
    default:
      break;
  }

  /* 2. SP+d8 (Gameboy specific) */

  if ((ModeMask & MModSPAdd)
   && (strlen(pArg->str.p_str) >= 4)
   && !as_strncasecmp(pArg->str.p_str, "SP", 2)
   && !IsSym(pArg->str.p_str[2]))
  {
    p_vals->values[0] = EvalStrIntExpressionOffsWithFlags(pArg, 2, SInt8, &OK, &p_vals->value_flags);
    if (OK)
    {
      p_vals->count = 1;
      p_vals->mode = ModSPAdd;
    }
    goto found;
  }

  /* all types of indirect expressions: (...), <...> for Z280: */

  is_pcrel = is_z280() && IsIndirectGen(pArg->str.p_str, "<>");
  is_indirect = IsIndirect(pArg->str.p_str);
  if (is_pcrel || is_indirect || (ModeMask & MModImmIsAbs))
  {
    tStrComp arg;
    tEvalResult disp_eval_result;
    z80_eval_cb_data_t z80_eval_cb_data;

    /* strip outer braces and spaces */
 
    StrCompRefRight(&arg, pArg, is_indirect || is_pcrel);
    StrCompShorten(&arg, is_indirect || is_pcrel);
    KillPrefBlanksStrCompRef(&arg);
    KillPostBlanksStrComp(&arg);

    /* special cases: */

    if ((ModeMask & MModHLInc) && (!as_strcasecmp(arg.str.p_str, "HL+") || !as_strcasecmp(arg.str.p_str, "HLI")))
    {
      p_vals->mode = ModHLInc;
      goto found;
    }
    if ((ModeMask & MModHLDec) && (!as_strcasecmp(arg.str.p_str, "HL-") || !as_strcasecmp(arg.str.p_str, "HLD")))
    {
      p_vals->mode = ModHLDec;
      goto found;
    }

    /* otherwise, walk through the components : */

    as_eval_cb_data_ini(&z80_eval_cb_data.cb_data, z80_eval_cb);
    z80_eval_cb_data.addr_reg = 0xff;
    z80_eval_cb_data.addr_reg_size = eSymbolSizeUnknown;
    p_vals->disp_acc = EvalStrIntExprWithResultAndCallback(&arg, Int32, &disp_eval_result, &z80_eval_cb_data.cb_data);
    if (!disp_eval_result.OK)
      goto found;

    /* now we have parsed the expression, see what we can do with it: */

    switch (z80_eval_cb_data.addr_reg)
    {
      /* no register: absolute */

      case 0xff:
      {
        if (is_pcrel)
        {
          LongWord address;

          address = p_vals->disp_acc;
          if (is_z280() && !phys_2_cpu(SegCode, &address, disp_eval_result.Flags, pArg))
            return p_vals->mode;
          p_vals->disp_acc = address;

          if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags)
           && !ChkRangeByType(p_vals->disp_acc, code_segment_int_type, pArg))
            return p_vals->mode;

          p_vals->value_flags = disp_eval_result.Flags;
          p_vals->mode = ModRA;
          if (pcrel_offset)
          {
            if (!construct_ra(pArg, p_vals, pcrel_offset))
              return p_vals->mode;
            else
              goto found;
          }
          else /* pcrel_offset depends on other operand not yet known, complete later */
            goto found;
        }
        else
        {
          LongWord address = p_vals->disp_acc;

          if (ModeMask & MModAbs)
          {
            /* Z380: if previous prefix explicitly defined 16 or 24 bit address, check for it: */

            if (ExtFlag)
            {
              switch (last_prefix_pair.i_prefix)
              {
                case ePrefixIB:
                  if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags)
                   && !ChkRangeByType(address, UInt24, pArg))
                    return p_vals->mode;
                  break;
                case ePrefixIN:
                  if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags)
                   && !ChkRangeByType(address, UInt16, pArg))
                    return p_vals->mode;
                  break;
                default:
                  break;
              }
            }
            else if (is_ez80())
            {
              if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags))
              {
                if (!ChkRangeByType(address, UInt24, pArg))
                  return p_vals->mode;

                /* .SIL, .SIS: use MBASE as bits 16..24, ignore bits 16.24 from .SIL instruction: */

                if (AttrPartOpSize[0] != eSymbolSize24Bit)
                  check_ez80_mpage(address, disp_eval_result.Flags, AttrPartOpSize[0], pArg);

                /* .LIS -> fetch 16 bits, use 0x00 as bits 16..24: */

                else if (AttrPartOpSize[1] != eSymbolSize24Bit)
                {
                  if (((address >> 16) & 0xff) != 0)
                    WrStrErrorPos(ErrNum_InAccPage, pArg);
                }

                /* .LIL: linear 24 bit address */
              }
            }
            else
            {
              if ((is_z280() || is_z180()) && !phys_2_cpu((ModeMask & MModImmIsAbs) ? SegCode : SegData, &address, disp_eval_result.Flags, pArg))
                return p_vals->mode;
              if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags)
               && !ChkRangeByType(address, UInt16, pArg))
                return p_vals->mode;
            }
            ChkSpace(SegCode, disp_eval_result.AddrSpaceMask);
            abs_2_adrvals(p_vals, address);
            p_vals->value = address;
            p_vals->value_flags = disp_eval_result.Flags;
            if (is_z280()) p_vals->part = 7; /* for generic handling in append_z280() */
            p_vals->mode = ModAbs;
            goto found;
          }
          else if (ModeMask & MModIOAbs)
          {
            if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags) && !ChkRangeByType(p_vals->disp_acc, UInt8, pArg))
              return p_vals->mode;
            ChkSpace(SegIO, disp_eval_result.AddrSpaceMask);
            p_vals->values[0] = address & 0xff;
            p_vals->value_flags = disp_eval_result.Flags;
            p_vals->count = 1;
            p_vals->mode = ModIOAbs;
            goto found;
          }
          else
            goto inv_mode;
        }
      }

      case 0:
        if ((z80_eval_cb_data.addr_reg_size != eSymbolSize16Bit) || p_vals->disp_acc || is_pcrel) /* no (B), (BC+d), <B> */
          goto wrong;
        else /* (BC) */
        {
          p_vals->mode = ModIndReg16;
          p_vals->part = BCReg;
          goto found;
        }

      case 1:
        if (is_pcrel) /* no <DE> */
          goto wrong;
        if (z80_eval_cb_data.addr_reg_size == eSymbolSize16Bit) /* (DE) */
        {
          if (p_vals->disp_acc)
            goto wrong;
          p_vals->mode = ModIndReg16;
          p_vals->part = DEReg;
          goto found;
        }
        else /* (C), (FF00+C) on Sharp/GB */
        {
          if (!p_vals->disp_acc || (is_sharp() && (p_vals->disp_acc == 0xff00)))
          {
            p_vals->mode = ModIndReg8;
            p_vals->part = CReg;
            goto found;
          }
          else
            goto wrong;
        }

      case 2:
        if ((z80_eval_cb_data.addr_reg_size != eSymbolSize16Bit) || is_pcrel) /* no (D), <HL> */
          goto wrong;
        else if (!p_vals->disp_acc) /* (HL) */
        {
          p_vals->mode = ModReg8; /* (HL) is M-Reg */
          p_vals->part = MReg;
          goto found;
        }
        else if (!(ModeMask & MModX)) /* (HL+dddd) */
          goto wrong;
        else if (!ChkRangeByType(p_vals->disp_acc, Int16, pArg))
          return p_vals->mode;
        else
        {
          p_vals->mode = ModX;
          p_vals->part = 3;
          p_vals->values[0] = Lo(p_vals->disp_acc);
          p_vals->values[1] = Hi(p_vals->disp_acc);
          p_vals->value_flags = disp_eval_result.Flags;
          p_vals->count = 2;
          goto found;
        }

      case EncPCReg: /* (PC+d) */
        if (is_pcrel)
          goto wrong;
        else
        {
          p_vals->mode = ModRA;
          if (!construct_ra(pArg, p_vals, 0))
            return p_vals->mode;
          else
            goto found;
        }

      case EncIXReg: /* (IX+d) */
      case EncIYReg: /* (IY+d) */
      case SPReg: /* (SP+d) */
      {
        Boolean is_s8;

        if (is_pcrel) /* no <XY>, <SP> */
          goto wrong;
        if (!mFirstPassUnknownOrQuestionable(disp_eval_result.Flags) && !ChkRangeByType(p_vals->disp_acc, is_z380() ? SInt24 : (is_z280() ? Int16 : SInt8), pArg))
          return p_vals->mode;
        is_s8 = (p_vals->disp_acc >= -0x80l) && (p_vals->disp_acc <= 0x7fl);
        if (z80_eval_cb_data.addr_reg == SPReg)
          p_vals->mode = (is_z280() && (ModeMask & MModSR)) ? ModSR : ModSPRel;
        else
        {
          if (is_z280() && !is_s8)
          {
            p_vals->mode = ModX;
            p_vals->part = ((z80_eval_cb_data.addr_reg >> 4) - 11) >> 1;
          }
          else
          {
            p_vals->mode = ModReg8;
            p_vals->part = MReg;
            append_prefix(0x0d | (z80_eval_cb_data.addr_reg & 0xf0));
          }
        }
        p_vals->values[0] = p_vals->disp_acc & 0xff;
        p_vals->value_flags = disp_eval_result.Flags;
        p_vals->count = 1;
        switch (p_vals->mode)
        {
          case ModX:
          case ModSR:
            p_vals->values[p_vals->count++] = (p_vals->disp_acc >> 8) & 0xff;
            break;
          default:
            if (((p_vals->disp_acc < -0x80l) || (p_vals->disp_acc > 0x7fl)) && is_z380())
            {
              p_vals->values[p_vals->count++] = (p_vals->disp_acc >> 8) & 0xff;
              if ((p_vals->disp_acc >= -0x8000l) && (p_vals->disp_acc <= 0x7fffl))
                ChangeDDPrefix(ePrefixIB);
              else
              {
                p_vals->values[p_vals->count++] = (p_vals->disp_acc >> 16) & 0xff;
                ChangeDDPrefix(ePrefixIW);
              }
            }
            break;
        }
        goto found;
      }

      case EncHL_IXReg:
        if ((z80_eval_cb_data.addr_reg_size != eSymbolSize16Bit) || p_vals->disp_acc || is_pcrel) /* no (HL+IX+d), <HL+IX> */
          goto wrong;
        p_vals->mode = ModBX;
        p_vals->part = 1;
        goto found;

      case EncHL_IYReg:
        if ((z80_eval_cb_data.addr_reg_size != eSymbolSize16Bit) || p_vals->disp_acc || is_pcrel) /* no (HL+IY+d), <HL+IY> */
          goto wrong;
        p_vals->mode = ModBX;
        p_vals->part = 2;
        goto found;

      case EncIX_IYReg:
        if ((z80_eval_cb_data.addr_reg_size != eSymbolSize16Bit) || p_vals->disp_acc || is_pcrel) /* no (IX+IY+d), <IX+IY> */
          goto wrong;
        p_vals->mode = ModBX;
        p_vals->part = 3;
        goto found;

      wrong:
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, pArg);
        return p_vals->mode;
    }
  }

  /* ...immediate */

  if (!(ModeMask & MModImm))
    goto inv_mode;
  switch (OpSize)
  {
    case eSymbolSizeUnknown:
      if (ModeMask & MModImm)
        WrStrErrorPos(ErrNum_UndefOpSizes, pArg);
      else
        p_vals->mode = ModImm;  /* will fail on test @ label found */
      break;
    case eSymbolSize8Bit:
      p_vals->values[0] = EvalStrIntExpressionWithFlags(pArg, Int8, &OK, &p_vals->value_flags);
      if (OK)
      {
        p_vals->mode = ModImm;
        p_vals->count = 1;
      }
      break;
    case eSymbolSize16Bit:
      if (InLongMode())
      {
        IntType range_type;
        LongWord ImmVal;

        switch (last_prefix_pair.i_prefix)
        {
          case ePrefixIN:
            range_type = UInt16;
            break;
          case ePrefixIB:
            range_type = UInt24;
            break;
          default:
            range_type = Int32;
        }
        ImmVal = EvalStrIntExpressionWithFlags(pArg, range_type, &OK, &p_vals->value_flags);
        if (OK)
        {
          p_vals->values[0] = Lo(ImmVal);
          p_vals->values[1] = Hi(ImmVal);
          p_vals->mode = ModImm;
          p_vals->count = 2;
          z380_extend_ib_iw(p_vals, ImmVal);
        }
      }
      else if (is_ez80() && AttrPartOpSize[1] == eSymbolSize24Bit)
      {
        LongWord ImmVal = EvalStrIntExpressionWithFlags(pArg, Int24, &OK, &p_vals->value_flags);
        if (OK)
        {
          p_vals->values[0] = (ImmVal >>  0) & 0xff;
          p_vals->values[1] = (ImmVal >>  8) & 0xff;
          p_vals->values[2] = (ImmVal >> 16) & 0xff;
          p_vals->mode = ModImm;
          p_vals->count = 3;
        }
      }
      else
      {
        AdrInt = EvalStrIntExpressionWithFlags(pArg, Int16, &OK, &p_vals->value_flags);
        if (OK)
        {
          p_vals->values[0] = Lo(AdrInt);
          p_vals->values[1] = Hi(AdrInt);
          p_vals->mode = ModImm;
          p_vals->count = 2;
        }
      }
      break;
    default:
      WrStrErrorPos(ErrNum_InvOpSize, pArg);
  }

found:
  if ((p_vals->mode != ModNone) && !((ModeMask >> p_vals->mode) & 1))
    goto inv_mode;
  return p_vals->mode;

inv_mode:
  WrStrErrorPos(ErrNum_InvAddrMode, pArg);
  p_vals->mode = ModNone;
  return p_vals->mode;
}

#define DecodeAdr(pArg, p_vals, ModeMask) \
        decode_adr_rel(pArg, p_vals, ModeMask, 0)

/*!------------------------------------------------------------------------
 * \fn     convert_sx_to_x(adr_vals_t *p_adr_vals, Byte index_prefix)
 * \brief  For Z280, convert SX mode (XY+d8) to X mode (R16+d16)
 * \param  p_adr_vals encoded address expression to convert
 * \param  index_prefix prefix byte describing index register (0xdd/0xfd)
 * \return new addressing mode
 * ------------------------------------------------------------------------ */

static ShortInt convert_sx_to_x(adr_vals_t *p_adr_vals, Byte index_prefix)
{
  p_adr_vals->mode = ModX;
  p_adr_vals->part = ((index_prefix & 0x20) >> 5) + 1; /* DD/FD ->1/2 */
  p_adr_vals->values[1] = p_adr_vals->values[0] & 0x80 ? 0xff : 0x00;
  p_adr_vals->count++;
  return p_adr_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     convert_ihl_to_x(adr_vals_t *p_adr_vals)
 * \brief  For Z280, convert (HL) mode to X mode (R16+d16)
 * \param  p_adr_vals encoded address expression to convert
 * \return new addressing mode
 * ------------------------------------------------------------------------ */

static ShortInt convert_ihl_to_x(adr_vals_t *p_adr_vals)
{
  p_adr_vals->mode = ModX;
  p_adr_vals->part = 3;
  p_adr_vals->values[1] = p_adr_vals->values[0] = 0x00;
  p_adr_vals->count = 2;
  return p_adr_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr_A(const tStrComp *p_arg)
 * \brief  check whether argument is accumulator (including possible register aliases)
 * \param  p_arg source argument
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr_A(const tStrComp *p_arg)
{
  adr_vals_t adr_vals;

  if (DecodeAdr(p_arg, &adr_vals, MModReg8) != ModReg8)
    return False;
  if (adr_vals.part != AccReg)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr_HL(const tStrComp *p_arg)
 * \brief  check whether argument is HL (including possible register aliases)
 * \param  p_arg source argument
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr_HL(const tStrComp *p_arg)
{
  adr_vals_t adr_vals;

  if (DecodeAdr(p_arg, &adr_vals, MModReg16) != ModReg16)
    return False;
  if ((adr_vals.part != HLReg) || (PrefixCnt > 0))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr_DEHL(const tStrComp *p_arg)
 * \brief  check whether argument is DEHL (no register aliases for that)
 * \param  p_arg source argument
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr_DEHL(const tStrComp *p_arg)
{
  if (as_strcasecmp(p_arg->str.p_str, "DEHL"))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     store_prefix(prefix_store_t *p_store, Byte old_prefix_cnt)
 * \brief  check whether another (index) prefix was added, and store it
 * \param  p_store place to store
 * \param  old_prefix_cnt prefix count before possible addition
 * ------------------------------------------------------------------------ */

typedef struct
{
  Byte cnt, value;
  Boolean present;
} prefix_store_t;

static void store_prefix(prefix_store_t *p_store, Byte old_prefix_cnt)
{
  p_store->cnt = PrefixCnt;
  p_store->present = p_store->cnt > old_prefix_cnt;
  p_store->value = p_store->present ? BAsmCode[p_store->cnt - 1] : 0x00;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdrWithF(const tStrComp *pArg, adr_vals_t *p_vals, LongWord ModeMask, LongInt pcrel_offset, Boolean AllowF)
 * \brief  Handle address expression, treating F as 8th register
 * \param  pArg source argument
 * \param  p_vals dest buffer for decoded result
 * \param  ModeMask bit mask of allowed modes
 * \param  pcrel_offset PC offset for RA mode
 * \param  allow 'F' at all?
 * ------------------------------------------------------------------------ */

static ShortInt DecodeAdrWithF(const tStrComp *pArg, adr_vals_t *p_vals, LongWord ModeMask, LongInt pcrel_offset, Boolean AllowF)
{
  Boolean applies_to_cpu = is_z80u() || is_z180() || is_z280() || is_z380();
  Byte orig_prefix_cnt = PrefixCnt;
  prefix_store_t new_prefix;

  if (applies_to_cpu
   && AllowF
   && !as_strcasecmp(pArg->str.p_str, "F"))
  {
    reset_adr_vals(p_vals);
    p_vals->part = 6;
    return ModReg8;
  }

  orig_prefix_cnt = PrefixCnt;
  decode_adr_rel(pArg, p_vals, ModeMask, pcrel_offset);
  store_prefix(&new_prefix, orig_prefix_cnt);

  /* if 110 denotes F, it cannot denote (HL) */

  if (applies_to_cpu
   && (p_vals->mode == ModReg8)
   && (p_vals->part == MReg))
  {
    if (ModeMask & MModX)
      p_vals->mode = new_prefix.present ? convert_sx_to_x(p_vals, BAsmCode[--PrefixCnt]) : convert_ihl_to_x(p_vals);
    else
    {
      reset_adr_vals(p_vals);
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    }
  }
  return p_vals->mode;
}

static Boolean ImmIs8(const adr_vals_t *p_vals)
{
  Word tmp;

  if (p_vals->count < 2)
    return True;

  tmp = (Word) p_vals->values[p_vals->count - 2];

  return ((tmp <= 255) || (tmp >= 0xff80));
}

static Boolean ImmIsS8(const adr_vals_t *p_vals)
{
  Word tmp;

  if (p_vals->count < 2)
    return True;

  tmp = p_vals->values[1];
  tmp = (tmp << 8) | p_vals->values[0];

  return ((tmp <= 127) || (tmp >= 0xff80));
}

/*!------------------------------------------------------------------------
 * \fn     append_opcode(Byte opcode)
 * \brief  append 8 bit opcode to prefixes and set CodeLen
 * \param  code code to append
 * ------------------------------------------------------------------------ */

static void append_opcode(Byte opcode)
{
  CodeLen = PrefixCnt;
  BAsmCode[CodeLen++] = opcode;
}

/*!------------------------------------------------------------------------
 * \fn     append_opcode16(Byte opcode)
 * \brief  append 8/16 bit opcode to prefixes and set CodeLen
 * \param  code code to append (2 bytes if MSB != 0)
 * ------------------------------------------------------------------------ */

static void append_opcode16(Word opcode)
{
  CodeLen = PrefixCnt;
  if (Hi(opcode))
    BAsmCode[CodeLen++] = Hi(opcode);
  BAsmCode[CodeLen++] = Lo(opcode);
}

static void AppendVals(const Byte *pVals, unsigned ValLen)
{
  memcpy(BAsmCode + CodeLen, pVals, ValLen);
  CodeLen += ValLen;
}

static void append_adr_vals_count(const adr_vals_t *p_adr_vals, unsigned count)
{
  set_b_guessed(p_adr_vals->value_flags, CodeLen, p_adr_vals->count, 0xff);
  AppendVals(p_adr_vals->values, count);
}
#define append_adr_vals(p_adr_vals) append_adr_vals_count(p_adr_vals, (p_adr_vals)->count);


static Boolean ParPair(const char *Name1, const char *Name2)
{
  return (((!as_strcasecmp(ArgStr[1].str.p_str, Name1)) && (!as_strcasecmp(ArgStr[2].str.p_str, Name2))) ||
          ((!as_strcasecmp(ArgStr[1].str.p_str, Name2)) && (!as_strcasecmp(ArgStr[2].str.p_str, Name1))));
}

static void append_z280(const adr_vals_t *p_adr_vals, Boolean use_addr_prefix, Word opcode, int part_shift)
{
  Byte mode_bit = ((p_adr_vals->mode == ModX) || (p_adr_vals->mode == ModRA)) ? 4 : 0;
  if (use_addr_prefix)
  {
    append_prefix(0xdd | (mode_bit << 3));
    mode_bit = 0;
  }
  append_opcode16(opcode | (p_adr_vals->part | mode_bit) << part_shift);
  append_adr_vals(p_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     has_index_prefix(void)
 * \brief  Check if HL has been replaced by IX/IY via prefix
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean has_index_prefix(void)
{
  return (PrefixCnt > 0)
      && ((BAsmCode[PrefixCnt - 1] == IXPrefix)
       || (BAsmCode[PrefixCnt - 1] == IYPrefix));
}

/*!------------------------------------------------------------------------
 * \fn     remove_prefix(int byte_index)
 * \brief  remove a code prefix
 * \param  byte_index 0 -> remove last prefix
 *                    1 -> remove second last prefix
 * \return value of removed prefix
 * ------------------------------------------------------------------------ */

static Byte remove_prefix(int byte_index)
{
  Byte ret;

  if (PrefixCnt < (byte_index + 1))
    WrError(ErrNum_InternalError);
  ret = BAsmCode[PrefixCnt - 1 - byte_index];
  memmove(&BAsmCode[PrefixCnt - 1 - byte_index],
          &BAsmCode[PrefixCnt     - byte_index],
          byte_index);
  PrefixCnt--;
  return ret;
}

/*-------------------------------------------------------------------------*/
/* Bedingung entschluesseln */

static Boolean DecodeCondition(const char *Name, int *Erg)
{
  int z;

  for (z = 0; Conditions[z].Name; z++)
    if (!as_strcasecmp(Conditions[z].Name, Name))
    {
      *Erg = Conditions[z].Code;
      return True;
    }
  *Erg = 0;
  return False;
}

/*-------------------------------------------------------------------------*/
/* Sonderregister dekodieren */

static Boolean DecodeSFR(char *Inp, Byte *Erg)
{
  if (!as_strcasecmp(Inp, "SR"))
    *Erg = 1;
  else if (!as_strcasecmp(Inp, "XSR"))
    *Erg = 5;
  else if (!as_strcasecmp(Inp, "DSR"))
    *Erg = 6;
  else if (!as_strcasecmp(Inp, "YSR"))
    *Erg = 7;
  else
    return False;
  return True;
}

/*==========================================================================*/
/* Adressbereiche */

static IntType PortIntType(void)
{
  if (is_z380())
    return ExtFlag ? UInt32 : UInt16;
  else if (is_ez80())
    return UInt16;
  else
    return UInt8;
}

/*==========================================================================*/
/* instruction decoders */

/*!------------------------------------------------------------------------
 * \fn     chk_addr_core_mask(unsigned core_mask)
 * \brief  check for CPU core and report invalid addressing mode if not
 * \return True if condition given
 * ------------------------------------------------------------------------ */

static void core_mask_2_str(char *p_dest, size_t dest_cap, unsigned core_mask)
{
  static const char sep[2] = "/";

  *p_dest = '\0';
  if (core_mask & e_core_mask_z280)
    as_snprcatf(p_dest, dest_cap, "%s%s", sep + !*p_dest, "Z280");
  if (core_mask & e_core_mask_ez80)
    as_snprcatf(p_dest, dest_cap, "%s%s", sep + !*p_dest, "eZ80");
  if (core_mask & e_core_mask_z380)
    as_snprcatf(p_dest, dest_cap, "%s%s", sep + !*p_dest, "Z380");
}

static Boolean chk_addr_core_mask(unsigned core_mask)
{
  if (!((core_mask >> p_curr_cpu_props->core) & 1))
  {
    char mask_str[50], str[100];

    core_mask_2_str(mask_str, sizeof(mask_str), core_mask);
    as_snprintf(str, sizeof(str), getmessage(Num_ErrMsgMinCPUSupported), mask_str);
    WrXError(ErrNum_AddrModeNotSupported, str);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Boolean chk_core_mask(cpu_core_mask_t core_mask)
 * \brief  check whether current core fulfills requirement
 * \param  core_mask bit mask of supported cores
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean chk_core_mask(cpu_core_mask_t core_mask)
{
  Boolean perm_ok;

  /* basic check of availibility */

  if (!((core_mask >> p_curr_cpu_props->core) & 1))
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }

  /* additional supervisor mode checks on Z280 (warning only): */

  if (!is_z280() || SupAllowed)
    perm_ok = True;
  else if (core_mask & e_core_mask_z280_privileged)
    perm_ok = False;
  else if ((core_mask & e_core_mask_z280_io_privileged) && p_z280_data->reg_i)
    perm_ok = False;
  else
    perm_ok = True;
  if (!perm_ok)
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Boolean a_chk_core_mask_pos(cpu_core_mask_t core_mask, const tStrComp *p_arg)
 * \brief  check whether current core fulfills requirement for addressing mode
 * \param  core_mask bit mask of supported cores
 * \param  p_arg related source code argument
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean a_chk_core_mask_pos(cpu_core_mask_t core_mask, const tStrComp *p_arg)
{
  if (!((core_mask >> p_curr_cpu_props->core) & 1))
  {
    WrStrErrorPos(ErrNum_AddrModeNotSupported, p_arg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     chk_no_core_flags(cpu_core_flags_t flags)
 * \brief  check that current target does NOT have certain properties
 * \param  flags flags that must not be set
 * ------------------------------------------------------------------------ */

static Boolean chk_no_core_flags(cpu_core_flags_t flags)
{
  if (p_curr_cpu_props->core_flags & flags)
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Index)
 * \brief  handle instructions without arguments
 * \param  Index * to instruction description
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Index)
{
  BaseOrder *POrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && chk_core_mask(POrder->core_mask))
  {
    append_opcode16(POrder->Code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ez80_xio(Word code)
 * \brief  handle string I/O instructions not present on all eZ80 variants
 * \param  code machine opcode
 * ------------------------------------------------------------------------ */

static void decode_ez80_xio(Word code)
{
  if (ChkArgCnt(0, 0)
   && chk_core_mask(e_core_mask_ez80)
   && chk_no_core_flags(e_core_flag_no_xio))
    append_opcode16(code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAcc(Word Index)
 * \brief  handle instructions with accumulator as argument
 * \param  Index * to instruction description
 * ------------------------------------------------------------------------ */

static void DecodeAcc(Word Index)
{
  BaseOrder *POrder = AccOrders + Index;

  if (!ChkArgCnt(0, 1)
   || !chk_core_mask(POrder->core_mask))
    return;

  if (ArgCnt && !DecodeAdr_A(&ArgStr[1]))
    return;

  append_opcode16(POrder->Code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeHL(Word Index)
 * \brief  handle instructions with HL as argument
 * \param  Index * to instruction description
 * ------------------------------------------------------------------------ */

static void DecodeHL(Word Index)
{
  BaseOrder *POrder = HLOrders + Index;

  if (!ChkArgCnt(0, 1)
   || !chk_core_mask(POrder->core_mask))
    return;

  if (ArgCnt && !DecodeAdr_HL(&ArgStr[1]))
    return;

  append_opcode16(POrder->Code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLD(Word IsLDW)
 * \brief  handle LD(W) instruction
 * \param  IsLDW LD or LDW?
 * ------------------------------------------------------------------------ */

static void DecodeLD(Word IsLDW)
{
  if (ChkArgCnt(2, 2))
  {
    LongWord dest_mask;
    adr_vals_t adr_vals;

    dest_mask = MModReg8 | MModReg16 | MModIndReg16 | MModAbs | MModSPRel;
    if (is_sharp())
      dest_mask |= MModIndReg8 | MModHLInc | MModHLDec;
    else
    {
      dest_mask |= MModRef | MModInt;
      if (is_ez80())
        dest_mask |= MModMB;
      if (is_z280())
        dest_mask |= MModX | MModRA | MModSR | MModBX;
    }

    /* LD(W) <addr>... may be 4, 5, or 6 bytes long, so the PC-relative
       offset of dest operand cannot be computed until we know the
       source operand's type: */

    switch (decode_adr_rel(&ArgStr[1], &adr_vals, dest_mask, 0))
    {
      case ModReg8:
        if (adr_vals.part == AccReg) /* LD A, ... */
        {
          LongWord src_mask;

          OpSize = eSymbolSize8Bit;
          src_mask = MModReg8 | MModReg16 | MModIndReg16 | MModImm | MModAbs | MModSPRel;
          if (is_sharp())
            src_mask |= MModIndReg8 | MModHLInc | MModHLDec;
          else
          {
            src_mask |= MModRef | MModInt;
            if (is_ez80())
              src_mask |= MModMB;
            else if (is_z280())
              src_mask |= MModX | MModRA | MModSR | MModBX;
          }
          switch (decode_adr_rel(&ArgStr[2], &adr_vals, src_mask, 4))
          {
            case ModReg8: /* LD A, R8/RX8/(HL)/(XY+D) */
              append_opcode(0x78 + adr_vals.part);
              append_adr_vals(&adr_vals);
              break;
            case ModIndReg8: /* LD A,(FF00+C) */
              BAsmCode[0] = 0xf2;
              CodeLen = 1;
              break;
            case ModHLInc: /* LD A,(HLI) */
              BAsmCode[0] = 0x2a;
              CodeLen = 1;
              break;
            case ModHLDec: /* LD A,(HLD) */
              BAsmCode[0] = 0x3a;
              CodeLen = 1;
              break;
            case ModIndReg16: /* LD A, (BC)/(DE) */
              append_opcode(0x0a + (adr_vals.part << 4));
              break;
            case ModImm: /* LD A, imm8 */
              append_opcode(0x3e);
              append_adr_vals(&adr_vals);
              break;
            case ModAbs: /* LD a, (adr) */
              if (is_sharp() && (adr_vals.values[1] == 0xff))
              {
                CodeLen = 0;
                BAsmCode[CodeLen++] = 0xf0;
                append_adr_vals_count(&adr_vals, 1);
              }
              else
              {
                append_opcode(is_sharp() ? 0xfa : 0x3a);
                append_adr_vals(&adr_vals);
              }
              break;
            case ModRef: /* LD A, R */
              append_prefix(0xed);
              append_opcode(0x5f);
              break;
            case ModInt: /* LD A, I */
              append_prefix(0xed);
              append_opcode(0x57);
              break;
            case ModMB: /* LD A, MB */
              append_prefix(0xed);
              append_opcode(0x6e);
              break;
            case ModRA:
            case ModX:
            case ModBX:
            case ModSR:
              append_z280(&adr_vals, True, 0x78, 0);
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        else if ((adr_vals.part != MReg) && !has_index_prefix()) /* LD R8, ... */
        {
          Byte dest_reg = adr_vals.part;
          OpSize = eSymbolSize8Bit;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8 | MModImm))
          {
            case ModReg8: /* LD R8, R8/RX8/(HL)/(XY+D) */
              /* if (I(XY)+d) as source, cannot use H/L as target ! */
              if (((dest_reg == HReg) || (dest_reg == LReg)) && IndexPrefix() && (adr_vals.count == 0)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              /* LD x,x for x==B...E used as prefixes on eZ80: */
              else if ((dest_reg < 4) && (adr_vals.part == dest_reg) && is_ez80())
              {
                WrError(ErrNum_ReplacedByNOP);
                append_opcode(NOPCode);
              }
              else
              {
                append_opcode(0x40 + (dest_reg << 3) + adr_vals.part);
                append_adr_vals(&adr_vals);
              }
              break;
            case ModImm: /* LD R8, imm8 */
              CodeLen = 0;
              BAsmCode[CodeLen++] = 0x06 + (dest_reg << 3);
              append_adr_vals_count(&adr_vals, 1);
              break;
            default:
              break;
          }
        }
        else if ((adr_vals.part == HReg) || (adr_vals.part == LReg)) /* LD RX8, ... */
        {
          Byte dest_reg = adr_vals.part;
          OpSize = eSymbolSize8Bit;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAll))
          {
            case ModReg8: /* LD RX8, R8/RX8 */
              if (adr_vals.part == MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);        /* stopped here */
              else if ((adr_vals.part >= HReg) && (adr_vals.part <= LReg) && (PrefixCnt != 2)) WrError(ErrNum_InvAddrMode);
              else if ((adr_vals.part >= HReg) && (adr_vals.part <= LReg) && (BAsmCode[0] != BAsmCode[1])) WrError(ErrNum_InvAddrMode);
              else
              {
                if (PrefixCnt == 2) PrefixCnt--;
                append_opcode(0x40 + (dest_reg << 3) + adr_vals.part);
              }
              break;
            case ModImm: /* LD RX8,imm8 */
              append_opcode(0x06 + (dest_reg << 3));
              append_adr_vals_count(&adr_vals, 1);
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        else /* LD (HL)/(XY+d),... */
        {
          adr_vals_t arg1_adr_vals = adr_vals;
          Byte src_prefix_cnt = PrefixCnt;

          if (!src_prefix_cnt && IsLDW)
          {
            OpSize = eSymbolSize16Bit;
            MayLW = True;
          }
          else
            OpSize = eSymbolSize8Bit;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAll))
          {
            case ModReg8: /* LD (HL)/(XY+D),R8 */
              if ((PrefixCnt != src_prefix_cnt) || (adr_vals.part == MReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else
              {
                append_opcode(0x70 + adr_vals.part);
                append_adr_vals(&arg1_adr_vals);
              }
              break;
            case ModImm: /* LD (HL)/(XY+D),imm8:16:32 */
              if (!src_prefix_cnt && IsLDW)
              {
                if (chk_core_mask(e_core_mask_z380 | e_core_mask_z280))
                {
                  if (is_z380())
                    append_opcode16(0xed36);
                  else
                  {
                    append_prefix(0xdd);
                    append_opcode(0x01);
                  }
                  append_adr_vals(&adr_vals);
                }
              }
              else
              {
                append_opcode(0x36);
                append_adr_vals(&arg1_adr_vals);
                append_adr_vals(&adr_vals);
              }
              break;
            case ModReg16: /* LD (HL)/(XY+D),R16/XY */
              if (!chk_addr_core_mask(e_core_mask_ez80 | e_core_mask_z280 | e_core_mask_z380));
              else if ((adr_vals.part == SPReg) && !is_z280()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (arg1_adr_vals.count == 0) /* LD (HL), */
              {
                if (PrefixCnt == src_prefix_cnt) /* LD (HL),R16 */
                {
                  if (is_z280())
                    append_opcode16(0xed0e | (adr_vals.part << 4));
                  else
                  {
                    if ((adr_vals.part == HLReg) && is_z380())
                      adr_vals.part = 3;
                    append_prefix(is_ez80() ? 0xed : 0xfd);
                    append_opcode(0x0f + (adr_vals.part << 4));
                  }
                }
                else /* LD (HL),XY */
                {
                  if (is_z380())
                    append_opcode(0x31);
                  else if (is_z280()) /* Z280: convert to (HL+0x0000) */
                  {
                    append_opcode16(0xed3d);
                    BAsmCode[CodeLen++] = 0x00;
                    BAsmCode[CodeLen++] = 0x00;
                  }
                  else /* is_ez80() */
                  {
                    Byte index_prefix = remove_prefix(0);
                    append_prefix(0xed);
                    append_opcode((index_prefix == IYPrefix) ? 0x3e : 0x3f);
                  }
                }
              }
              else
              {
                if (PrefixCnt == src_prefix_cnt) /* LD (XY+D),R16 */
                {
                  if (is_z380())
                  {
                    if (adr_vals.part == HLReg)
                      adr_vals.part = 3;
                    append_opcode(0xcb);
                    append_adr_vals(&arg1_adr_vals);
                    BAsmCode[CodeLen++] = 0x0b + (adr_vals.part << 4);
                  }
                  else if (is_z280())
                  {
                    append_opcode16(0xed0e | (adr_vals.part << 4));
                    append_adr_vals(&arg1_adr_vals);
                  }
                  else /* is_ez80() */
                  {
                    append_opcode(0x0f | (adr_vals.part << 4));
                    append_adr_vals(&arg1_adr_vals);
                  }
                }
                else /* LD (XY+D), IX/Y */
                {
                  if (is_z380())
                  {
                    if (BAsmCode[0] == BAsmCode[1]) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
                    else
                    {
                      (void)remove_prefix(0);
                      append_opcode(0xcb);
                      append_adr_vals(&arg1_adr_vals);
                      BAsmCode[CodeLen++] = 0x2b;
                    }
                  }
                  else if (is_z280())
                  {
                    /* assert(PrefixCnt == 2) */
                    convert_sx_to_x(&arg1_adr_vals, remove_prefix(1));
                    append_opcode16(0xed25 | (arg1_adr_vals.part << 3));
                    append_adr_vals(&arg1_adr_vals);
                  }
                  else /* is_ez80() */
                  {
                    Byte src_index_prefix = remove_prefix(0);
                    append_opcode(((src_index_prefix == IYPrefix) ? 0x3e : 0x3f)
                                ^ ((BAsmCode[PrefixCnt - 1] == IYPrefix) ? 0x01 : 0x00));
                    append_adr_vals(&arg1_adr_vals);
                  }
                }
              }
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        break;
      case ModReg16:
        if (adr_vals.part == SPReg) /* LD SP,... */
        {
          OpSize = eSymbolSize16Bit;
          MayLW = True;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAll))
          {
            case ModReg8:
              if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (chk_core_mask(e_core_mask_z280))
              {
                append_opcode16(0xed36);
                append_adr_vals(&adr_vals);
              }
              break;
            case ModReg16: /* LD SP,HL/XY */
              if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else
                append_opcode(0xf9);
              break;
            case ModImm: /* LD SP,imm16:32 */
              append_opcode(0x31);
              append_adr_vals(&adr_vals);
              break;
            case ModAbs: /* LD SP,(adr) */
              if (a_chk_core_mask_pos(e_core_mask_no_sharp, &ArgStr[2]))
              {
                append_prefix(0xed);
                append_opcode(0x7b);
                append_adr_vals(&adr_vals);
              }
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        else if (!has_index_prefix()) /* LD R16,... */
        {
          LongWord ModeMask = MModAll;
          unsigned after_dest_prefixcnt = PrefixCnt;
          Byte orig_dest_reg = adr_vals.part,
               dest_reg = (adr_vals.part == HLReg) ? 3 : adr_vals.part;

          OpSize = eSymbolSize16Bit;
          MayLW = True;
          if (is_sharp() && (adr_vals.part == HLReg))
            ModeMask |= MModSPAdd;
          if (is_z280() && (adr_vals.part == HLReg))
            ModeMask |= MModX | MModRA | MModSR | MModBX;
          switch (decode_adr_rel(&ArgStr[2], &adr_vals, ModeMask, 4))
          {
            case ModInt: /* LD HL,I */
              if (dest_reg != 3) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
              else if (is_z380())
              {
                BAsmCode[0] = 0xdd;
                BAsmCode[1] = 0x57;
                CodeLen = 2;
              }
              else if (is_ez80())
              {
                if (chk_no_core_flags(e_core_flag_i_8bit))
                {
                  BAsmCode[0] = 0xed;
                  BAsmCode[1] = 0xd7;
                  CodeLen = 2;
                }
              }
              break;
            case ModReg8:
              if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (!is_z380() && !is_z280() && !is_ez80()) WrError(ErrNum_InvAddrMode);
              else if (PrefixCnt == after_dest_prefixcnt) /* LD R16,(HL) */
              {
                if (is_z380())
                {
                  BAsmCode[0] = 0xdd;
                  BAsmCode[1] = 0x0f + (dest_reg << 4);
                  CodeLen = 2;
                }
                else if (is_z280())
                  append_opcode16(0xed06 | (orig_dest_reg << 4));
                else /* if (is_ez80()) */
                {
                  if (dest_reg == 3) dest_reg = 2;
                  append_prefix(0xed);
                  append_opcode(0x07 | (dest_reg << 4));
                }
              }
              else /* LD R16,(XY+d) */
              {
                if (is_z380())
                {
                  append_opcode(0xcb);
                  append_adr_vals(&adr_vals);
                  BAsmCode[CodeLen++] = 0x03 + (dest_reg << 4);
                }
                else if (is_z280())
                {
                  append_opcode16(0xed06 | (orig_dest_reg << 4));
                  append_adr_vals(&adr_vals);
                }
                else /* if (is_ez80()) */
                {
                  if (dest_reg == 3) dest_reg = 2;
                  append_opcode(0x07 | (dest_reg << 4));
                  append_adr_vals(&adr_vals);
                }
              }
              break;
            case ModReg16:
              if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (!chk_core_mask(e_core_mask_z380));
              else if (PrefixCnt == 0) /* LD R16,R16 */
              {
                if (adr_vals.part == HLReg)
                  adr_vals.part = 3;
                else if (adr_vals.part == BCReg)
                  adr_vals.part = 2;
                BAsmCode[0] = 0xcd + (adr_vals.part << 4);
                BAsmCode[1] = 0x02 + (dest_reg << 4);
                CodeLen = 2;
              }
              else /* LD R16,XY */
                append_opcode(0x0b + (dest_reg << 4));
              break;
            case ModIndReg16: /* LD R16,(R16) */
              if (chk_core_mask(e_core_mask_z380))
              {
                CodeLen = 2;
                BAsmCode[0] = 0xdd;
                BAsmCode[1] = 0x0c + (dest_reg << 4) + adr_vals.part;
              }
              break;
            case ModImm: /* LD R16,imm */
              if (dest_reg == 3)
                dest_reg = 2;
              append_opcode(0x01 + (dest_reg << 4));
              append_adr_vals(&adr_vals);
              break;
            case ModAbs: /* LD R16,(adr) */
              if (!a_chk_core_mask_pos(e_core_mask_no_sharp, &ArgStr[2]));
              else if (dest_reg == 3)
              {
                append_opcode(0x2a);
                append_adr_vals(&adr_vals);
              }
              else
              {
                append_prefix(0xed);
                append_opcode(0x4b + (dest_reg << 4));
                append_adr_vals(&adr_vals);
              }
              break;
            case ModSPAdd:
              BAsmCode[0] = 0xf8;
              BAsmCode[1] = adr_vals.values[0];
              CodeLen = 2;
              break;
            case ModSPRel: /* LD R16,(SP+D) */
              if (chk_core_mask(e_core_mask_z380))
              {
                append_prefix(0xdd);
                append_opcode(0xcb);
                append_adr_vals(&adr_vals);
                BAsmCode[CodeLen++] = 0x01 + (dest_reg << 4);
              }
              break;
            case ModX: /* LD HL,(XX+dd) */
            case ModRA: /* LD HL,<addr> */
            case ModSR: /* LD HL,(SP+dd) */
            case ModBX: /* LD HL,(XXA+XXB) */
              append_z280(&adr_vals, False, 0xed04, 3);
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        else /* LD XY,... */
        {
          OpSize = eSymbolSize16Bit;
          MayLW = True;
          switch (decode_adr_rel(&ArgStr[2], &adr_vals, MModAll | (is_z280() ? (MModX | MModSR | MModRA | MModBX) : 0), 5))
          {
            case ModReg8:
              if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (!is_z380() && !is_z280() && !is_ez80()) WrError(ErrNum_InvAddrMode);
              else if (adr_vals.count == 0) /* LD XY,(HL) */
              {
                if (is_z380())
                  append_opcode(0x33);
                else if (is_z280())
                {
                  convert_ihl_to_x(&adr_vals);
                  goto xy_src_is_modx;
                }
                else /* is_ez80() */
                {
                  Byte index_prefix = remove_prefix(0);
                  append_prefix(0xed);
                  append_opcode((index_prefix == IYPrefix) ? 0x31 : 0x37);
                }
              }
              else /* LD XY,(XY+D) */
              {
                if (is_z380())
                {
                  if (BAsmCode[0] == BAsmCode[1]) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
                  else
                  {
                    (void)remove_prefix(1);
                    append_opcode(0xcb);
                    append_adr_vals(&adr_vals);
                    BAsmCode[CodeLen++] = 0x23;
                  }
                }
                else if (is_z280())
                {
                  convert_sx_to_x(&adr_vals, remove_prefix(0));
                  goto xy_src_is_modx;
                }
                else
                {
                  Byte dest_index_prefix = remove_prefix(1);
                  append_opcode(((dest_index_prefix == IYPrefix) ? 0x31 : 0x37)
                              ^ ((BAsmCode[PrefixCnt - 1] == IYPrefix) ? 0x06 : 0x00));
                  append_adr_vals(&adr_vals);
                }
              }
              break;
            case ModReg16:
              if (!chk_core_mask(e_core_mask_z380));
              else if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (PrefixCnt == 1) /* LD XY,R16 */
              {
                if (adr_vals.part == HLReg) adr_vals.part = 3;
                append_opcode(0x07 + (adr_vals.part << 4));
              }
              else if (BAsmCode[0] == BAsmCode[1]) WrError(ErrNum_InvAddrMode);
              else /* LD XY,XY */
              {
                (void)remove_prefix(0);
                append_opcode(0x27);
              }
              break;
            case ModIndReg16:
              if (chk_core_mask(e_core_mask_z380)) /* LD XY,(R16) */
                append_opcode(0x03 + (adr_vals.part << 4));
              break;
            case ModImm: /* LD XY,imm16:32 */
              append_opcode(0x21);
              append_adr_vals(&adr_vals);
              break;
            case ModAbs: /* LD XY,(adr) */
              append_opcode(0x2a);
              append_adr_vals(&adr_vals);
              break;
            case ModSPRel: /* LD XY,(SP+D) */
              if (chk_core_mask(e_core_mask_z380))
              {
                append_opcode(0xcb);
                append_adr_vals(&adr_vals);
                BAsmCode[CodeLen++] = 0x21;
              }
              break;
            case ModX: /* LD XY,(XX+dd) */
            case ModRA: /* LD XY,<addr> */
            case ModSR: /* LD XY,(SP+dd) */
            case ModBX: /* LD XY,(XXA+XXB */
            xy_src_is_modx:
              append_z280(&adr_vals, False, 0xed04, 3);
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        break;
      case ModIndReg8:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8))
        {
          case ModReg8:
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[0] = 0xe2;
              CodeLen = 1;
            }
            break;
          default:
            break;
        }
        break;
      case ModHLInc:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8))
        {
          case ModReg8:
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[0] = 0x22;
              CodeLen = 1;
            }
            break;
          default:
            break;
        }
        break;
      case ModHLDec:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8))
        {
          case ModReg8:
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[0] = 0x32;
              CodeLen = 1;
            }
            break;
          default:
            break;
        }
        break;
      case ModIndReg16: /* LD (R16),... */
      {
        Byte dest_reg = adr_vals.part;

        if (IsLDW)
        {
          OpSize = eSymbolSize16Bit;
          MayLW = True;
        }
        else
          OpSize = eSymbolSize8Bit;
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAll))
        {
          case ModReg8: /* LD (R16),A */
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
              append_opcode(0x02 + (dest_reg << 4));
            break;
          case ModReg16:
            if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (!chk_core_mask(e_core_mask_z380));
            else if (PrefixCnt == 0) /* LD (R16),R16 */
            {
              if (adr_vals.part == HLReg)
                adr_vals.part = 3;
              BAsmCode[0] = 0xfd;
              BAsmCode[1] = 0x0c + dest_reg + (adr_vals.part << 4);
              CodeLen = 2;
            }
            else /* LD (R16),XY */
              append_opcode(0x01 + (dest_reg << 4));
            break;
          case ModImm:
            if (!IsLDW) WrError(ErrNum_InvAddrMode);
            else if (chk_core_mask(e_core_mask_z380))
            {
              append_prefix(0xed);
              append_opcode(0x06 + (dest_reg << 4));
              append_adr_vals(&adr_vals);
            }
            break;
          case ModNone:
            break;
          default:
            WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        }
        break;
      }
      case ModAbs:
      {
        adr_vals_t arg1_adr_vals = adr_vals;

        OpSize = IsLDW ? eSymbolSize16Bit : eSymbolSize8Bit;
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8 | MModReg16 | (is_z280() ? MModImm : 0)))
        {
          case ModReg8: /* LD (adr),A */
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (is_sharp() && (arg1_adr_vals.values[1] == 0xff))
            {
              BAsmCode[0] = 0xe0;
              BAsmCode[1] = arg1_adr_vals.values[0];
              CodeLen = 2;
            }
            else
            {
              append_opcode(is_sharp() ? 0xea : 0x32);
              append_adr_vals(&arg1_adr_vals);
            }
            break;
          case ModReg16:
            if ((adr_vals.part == SPReg) && is_sharp())
            {
              BAsmCode[0] = 0x08;
              CodeLen = 1;
              append_adr_vals(&arg1_adr_vals);
            }
            else if (!a_chk_core_mask_pos(e_core_mask_no_sharp, &ArgStr[1]));
            else if (adr_vals.part == HLReg) /* LD (adr),HL/XY */
            {
              append_opcode(0x22);
              append_adr_vals(&arg1_adr_vals);
            }
            else /* LD (adr),R16 */
            {
              append_prefix(0xed);
              append_opcode(0x43 + (adr_vals.part << 4));
              append_adr_vals(&arg1_adr_vals);
            }
            break;
          case ModImm:
            if ((OpSize == eSymbolSize16Bit) && !chk_core_mask(e_core_mask_z280))
              return;
            append_prefix(0xdd);
            append_opcode((OpSize == eSymbolSize16Bit) ? 0x11 : 0x3e);
            append_adr_vals(&arg1_adr_vals);
            append_adr_vals(&adr_vals);
            break;
          case ModNone:
            break;
          default:
            WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        }
        break;
      }
      case ModInt:
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8 | MModReg16))
        {
          case ModReg8: /* LD I,A */
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0xed;
              BAsmCode[1] = 0x47;
            }
            break;
          case ModReg16: /* LD I,HL */
            if ((adr_vals.part != HLReg) || PrefixCnt) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (is_z380())
            {
              CodeLen = 2;
              BAsmCode[0] = 0xdd;
              BAsmCode[1] = 0x47;
            }
            else if (is_ez80())
            {
              if (chk_no_core_flags(e_core_flag_i_8bit))
              {
                CodeLen = 2;
                BAsmCode[0] = 0xed;
                BAsmCode[1] = 0xc7;
              }
            }
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
        break;
      case ModRef:
        if (DecodeAdr_A(&ArgStr[2])) /* LD R,A */
        {
          CodeLen = 2;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0x4f;
        }
        else WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        break;
      case ModMB:
        if (DecodeAdr_A(&ArgStr[2])) /* LD MB,A */
        {
          CodeLen = 2;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0x6d;
        }
        else WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        break;
      case ModSPRel:
        if (chk_core_mask(e_core_mask_z380))
        {
          adr_vals_t arg1_adr_vals = adr_vals;
          Byte dest_prefix_cnt = PrefixCnt;
          OpSize = eSymbolSize8Bit;
          switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAll))
          {
            case ModReg16:
              if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (PrefixCnt == dest_prefix_cnt) /* LD (SP+D),R16 */
              {
                if (adr_vals.part == HLReg)
                  adr_vals.part = 3;
                append_prefix(0xdd);
                append_opcode(0xcb);
                append_adr_vals(&arg1_adr_vals);
                BAsmCode[CodeLen++] = 0x09 + (adr_vals.part << 4);
              }
              else /* LD (SP+D),XY */
              {
                append_opcode(0xcb);
                append_adr_vals(&arg1_adr_vals);
                BAsmCode[CodeLen++] = 0x29;
              }
              break;
            case ModNone:
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
        }
        break;
      case ModRA:
      case ModX:
      case ModSR:
      case ModBX:
      {
        adr_vals_t src_adr_vals;

        OpSize = IsLDW ? eSymbolSize16Bit : eSymbolSize8Bit;
        switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg8 | MModReg16 | MModImm))
        {
          case ModReg8:
            if (src_adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (!construct_ra(&ArgStr[1], &adr_vals, 4));  /* LD <addr>,A is 4 bytes long */
            else
              append_z280(&adr_vals, False, 0xed03, 3);
            break;
          case ModReg16:
            if (src_adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (!construct_ra(&ArgStr[1], &adr_vals, 4 + PrefixCnt));  /* LDW <addr>,HL/XY is 4/5 bytes long */
            else
              append_z280(&adr_vals, False, 0xed05, 3);
            break;
          case ModImm:
            if (OpSize == eSymbolSize8Bit)
            {
              if (construct_ra(&ArgStr[1], &adr_vals, 5))   /* LD <addr>,d is 5 bytes long */
                append_z280(&adr_vals, True, 0x06, 3);
            }
            else if (adr_vals.mode == ModRA)
            {
              if (construct_ra(&ArgStr[1], &adr_vals, 6))   /* LDW <addr>,dd is 6 bytes long */
              {
                append_prefix(0xdd);
                append_opcode(0x31);
                append_adr_vals(&adr_vals);
              }
            }
            else
            {
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              return;
            }
            append_adr_vals(&src_adr_vals);
            break;
          default:
            break;
        }
        break;
      }
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }  /* outer switch */
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDHL(Word Code)
 * \brief  decode LDHL instruction (Sharp cores only)
 * ------------------------------------------------------------------------ */

static void DecodeLDHL(Word Code)
{
  Boolean OK;
  tSymbolFlags symbol_flags;
  adr_vals_t adr_vals;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2)
   || !chk_core_mask(e_core_mask_sharp)
   || (DecodeAdr(&ArgStr[1], &adr_vals, MModReg16) != ModReg16))
    return;
  if (adr_vals.part != SPReg)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    return;    
  }
  BAsmCode[1] = EvalStrIntExpressionWithFlags(&ArgStr[2], SInt8, &OK, &symbol_flags);
  if (OK)
  {
    set_b_guessed(symbol_flags, 1, 1, 0xff);
    BAsmCode[0] = 0xf8;
    CodeLen = 2;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDH(Word code)
 * \brief  Decode LDH instruction (Sharp cores only)
 * ------------------------------------------------------------------------ */

static Boolean ChkAbsUpperPage(Byte *p_dest, const adr_vals_t *p_vals)
{
  /* allow just lower byte (00..ff) or full address (ff00..ffff): */

  if ((p_vals->count == 2) && ((p_vals->values[1] == 0x00) || (p_vals->values[1] == 0xff)))
  {
    *p_dest = p_vals->values[0];
    return True;
  }
  else
    return False;
}

static void DecodeLDH(Word code)
{
  adr_vals_t adr_vals;

  UNUSED(code);

  if (!ChkArgCnt(2, 2)
   || !chk_core_mask(e_core_mask_sharp))
    return;

  OpSize = eSymbolSize8Bit;
  switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg8 | MModIndReg8 | MModAbs))
  {
    case ModReg8:
      if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else switch (DecodeAdr(&ArgStr[2], &adr_vals, MModIndReg8 | MModAbs))
      {
        case ModIndReg8:
          BAsmCode[0] = 0xf2;
          CodeLen = 1;
          break;
        case ModAbs:
          if (ChkAbsUpperPage(&BAsmCode[1], &adr_vals))
          {
            BAsmCode[0] = 0xf0;
            CodeLen = 2;
          }
          break;
        default:
          break;
      }
      break;
    case ModIndReg8:
      if (DecodeAdr_A(&ArgStr[2]))
      {
        BAsmCode[0] = 0xe2;
        CodeLen = 1;
      }
      break;
    case ModAbs:
      if (ChkAbsUpperPage(&BAsmCode[1], &adr_vals))
      {
        if (DecodeAdr_A(&ArgStr[2]))
        {
          BAsmCode[0] = 0xe0;
          CodeLen = 2;
        }
      }
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDX(Word Code)
 * \brief  decode LDX instruction (Sharp cores only)
 * ------------------------------------------------------------------------ */

static void DecodeLDX(Word Code)
{
  adr_vals_t dest_adr_vals, src_adr_vals;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2)
   || !chk_core_mask(e_core_mask_sharp))
    return;
  switch (DecodeAdr(&ArgStr[1], &dest_adr_vals, MModReg8 | MModAbs))
  {
    case ModReg8:
      if (dest_adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      else
      {
        if (DecodeAdr(&ArgStr[2], &src_adr_vals, MModAbs) == ModAbs)
        {
          BAsmCode[0] = 0xfa;
          CodeLen = 1;
          append_adr_vals(&src_adr_vals);
        }
      }
      break;
    case ModAbs:
      if (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg8) == ModReg8)
      {
        if (src_adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
        else
        {
          BAsmCode[0] = 0xea;
          CodeLen = 1;
          append_adr_vals(&dest_adr_vals);
        }
      }
      break;
    default:
      break;
  }
}

static void DecodeALU8(Word Code)
{
  adr_vals_t adr_vals;
  ShortInt adr_mode;

  switch (ArgCnt)
  {
    case 1:
      reset_adr_vals(&adr_vals);
      adr_mode = ModReg8;
      adr_vals.part = AccReg;
      break;
    case 2:
      adr_mode = DecodeAdr(&ArgStr[1], &adr_vals, MModReg8 | (is_z380() ? MModReg16 : 0));
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  switch (adr_mode)
  {
    case ModReg16:
      if (Code != 2) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else if (adr_vals.part == HLReg)
      {
        OpSize = eSymbolSize16Bit;
        if (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModAbs) == ModAbs)
        {
          append_prefix(0xed);
          append_opcode(0xd6);
          append_adr_vals(&adr_vals);
        }
      }
      else if (adr_vals.part == SPReg)
      {
        OpSize = eSymbolSize16Bit; MayLW = True;
        if (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModImm) == ModImm)
        {
          append_prefix(0xed);
          append_opcode(0x92);
          append_adr_vals(&adr_vals);
          break;
        }
      }
      else
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      break;
    case ModReg8:
      if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        OpSize = eSymbolSize8Bit;
        switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModReg8 | MModImm | (is_z280() ? (MModAbs | MModX | MModRA | MModSR | MModBX) : 0), 4))
        {
          case ModReg8:
            append_opcode(0x80 + (Code << 3) + adr_vals.part);
            append_adr_vals(&adr_vals);
            break;
          case ModImm:
            if (!ImmIs8(&adr_vals)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[ArgCnt]);
            else
            {
              append_opcode(0xc6 + (Code << 3));
              append_adr_vals_count(&adr_vals, 1);
            }
            break;
          case ModAbs:
          case ModX:
          case ModRA:
          case ModSR:
          case ModBX:
            append_z280(&adr_vals, True, 0x80 | (Code << 3), 0);
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

static void DecodeALU16(Word Code)
{
  cpu_core_mask_t core_mask = e_core_mask_z380;
  switch (Code)  /* allow SUBW/CPW on Z280 */
  {
    case 2:
      if (is_z280())
        Code = 0xe;
      /* FALL-THRU */
    case 7:
      core_mask |= e_core_mask_z280;
      break;
    default:
      break;
  }

  if (ChkArgCnt(1, 2)
   && chk_core_mask(core_mask)
   && ((ArgCnt == 1) || DecodeAdr_HL(&ArgStr[1])))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize16Bit;
    switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModAll | (is_z280() ? (MModX | MModRA) : 0), 5))
    {
      case ModReg16:
        if (is_z380())
        {
          if (PrefixCnt > 0)      /* wenn Register, dann nie DDIR! */
            append_opcode(0x87 + (Code << 3));
          else if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
          else
          {
            if (adr_vals.part == HLReg)
              adr_vals.part = 3;
            BAsmCode[0] = 0xed;
            BAsmCode[1] = 0x84 + (Code << 3) + adr_vals.part;
            CodeLen = 2;
          }
        }
        else /* is_z280() */
        {
          append_prefix(0xed);
          append_opcode(0xc6 | (adr_vals.part << 4) | Code);
        }
        break;
      case ModReg8:
        if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else if (is_z380())
        {
          if (adr_vals.count == 0) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
          else
          {
            append_opcode(0xc6 + (Code << 3));
            append_adr_vals(&adr_vals);
          }
        }
        else /* is_z280() */
        {
          if (has_index_prefix())
          {
            convert_sx_to_x(&adr_vals, BAsmCode[--PrefixCnt]);
            goto is_modx;
          }
          else
          {
            append_prefix(0xdd);
            append_prefix(0xed);
            append_opcode(0xc0 | Code);
            append_adr_vals(&adr_vals);
          }
        }
        break;
      case ModX:
      is_modx:
        if (adr_vals.part == 3) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else
        {
          append_prefix(0xfd);
          append_prefix(0xed);
          append_opcode(0xc0 | Code | ((adr_vals.part & 2) << 3));
          append_adr_vals(&adr_vals);
        }
        break;
      case ModRA:
        append_prefix(0xdd);
        append_prefix(0xed);
        append_opcode(0xf0 | Code);
        append_adr_vals(&adr_vals);
        break;
      case ModAbs:
        append_prefix(0xdd);
        append_prefix(0xed);
        append_opcode(0xd0 | Code);
        append_adr_vals(&adr_vals);
        break;
      case ModImm:
        if (is_z380())
        {
          append_prefix(0xed);
          append_opcode(0x86 + (Code << 3));
        }
        else /* is_z280() */
        {
          append_prefix(0xfd);
          append_prefix(0xed);
          append_opcode(0xf0 | Code);
        }
        append_adr_vals(&adr_vals);
        break;
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
    }
  }
}

static void DecodeADD(Word Index)
{
  ShortInt adr_mode;
  Byte raw_prefix_cnt = PrefixCnt;
  adr_vals_t adr_vals;

  UNUSED(Index);

  switch (ArgCnt)
  {
    case 1:
      reset_adr_vals(&adr_vals);
      adr_mode = ModReg8;
      adr_vals.part = AccReg;
      break;
    case 2:
      adr_mode = DecodeAdr(&ArgStr[1], &adr_vals, MModNoImm);
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  switch (adr_mode)
  {
    case ModReg8:
      if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        OpSize = eSymbolSize8Bit;
        switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModReg8 | MModImm | (is_z280() ? (MModAbs | MModX | MModRA | MModSR | MModBX) : 0), 4))
        {
          case ModReg8:
            append_opcode(0x80 + adr_vals.part);
            append_adr_vals(&adr_vals);
            break;
          case ModAbs:
          case ModX:
          case ModRA:
          case ModSR:
          case ModBX:
            append_z280(&adr_vals, True, 0x80, 0);
            break;
          case ModImm:
            append_opcode(0xc6);
            append_adr_vals(&adr_vals);
            break;
          default:
            break;
        }
      }
      break;
    case ModReg16:
      if (adr_vals.part == SPReg)
      {
        OpSize = is_z380() ? eSymbolSize16Bit : eSymbolSize8Bit;
        MayLW = is_z380();
        switch (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModAll))
        {
          case ModImm:
            if (is_z380())
            {
              append_prefix(0xed);
              append_opcode(0x82);
              append_adr_vals(&adr_vals);
              break;
            }
            else if (is_r2000())
            {
              BAsmCode[0] = 0x27; BAsmCode[1] = 0[adr_vals.values];
              CodeLen = 2;
            }
            else if (is_sharp())
            {
              if (!ImmIsS8(&adr_vals)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
              else
              {
                BAsmCode[0] = 0xe8;
                BAsmCode[1] = 0[adr_vals.values];
                CodeLen = 2;
              }
            }
            else
              WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
            break;
          case ModNone:
            break;
          default:
            WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        }
      }
      else if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        prefix_store_t dest_prefix;
        store_prefix(&dest_prefix, raw_prefix_cnt);

        OpSize = eSymbolSize16Bit;
        switch (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModAll))
        {
          case ModReg8:
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (chk_core_mask(e_core_mask_z280))
            {
              append_prefix(0xed);
              append_opcode(0x6d);
              append_adr_vals(&adr_vals);
            }
            break;
          case ModReg16:
          {
            prefix_store_t src_prefix;
            store_prefix(&src_prefix, dest_prefix.cnt);

            if ((adr_vals.part == HLReg) && ((dest_prefix.present != src_prefix.present) || (dest_prefix.value != src_prefix.value))) WrError(ErrNum_InvAddrMode);
            else
            {
              if (dest_prefix.present && src_prefix.present)
                PrefixCnt--;
              append_opcode(0x09 + (adr_vals.part << 4));
            }
            break;
          }
          case ModAbs:
            if (dest_prefix.present) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (chk_core_mask(e_core_mask_z380))
            {
              append_prefix(0xed);
              append_opcode(0xc6);
              append_adr_vals(&adr_vals);
            }
            break;
          case ModNone:
            break;
          default:
            WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        }
      }
      break;
    case ModNone:
      break;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  }
}

static void DecodeADDW(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 2)
   && chk_core_mask(e_core_mask_z380 | e_core_mask_z280)
   && ((ArgCnt == 1) || DecodeAdr_HL(&ArgStr[1])))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize16Bit;
    switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModReg16 | MModReg8 | MModImm | (is_z280() ? (MModAbs | MModX | MModRA) : 0), 5))
    {
      case ModReg16:
        if (is_z380())
        {
          if (PrefixCnt > 0)      /* wenn Register, dann nie DDIR! */
            append_opcode(0x87);
          else if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
          else
          {
            if (adr_vals.part == HLReg)
              adr_vals.part = 3;
            append_prefix(0xed);
            append_opcode(0x84 + adr_vals.part);
          }
        }
        else /* is_z280() */
        {
          append_prefix(0xed);
          append_opcode(0xc6 | (adr_vals.part << 4));
        }
        break;
      case ModReg8:
        if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else if (is_z380())
        {
          if (adr_vals.count == 0) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
          else
          {
            append_opcode(0xc6);
            append_adr_vals(&adr_vals);
          }
        }
        else /* is_z280() */
        {
          if (has_index_prefix())
          {
            convert_sx_to_x(&adr_vals, BAsmCode[--PrefixCnt]);
            goto is_modx;
          }
          else
          {
            append_prefix(0xdd);
            append_prefix(0xed);
            append_opcode(0xc6);
            append_adr_vals(&adr_vals);
          }
        }
        break;
      case ModX:
      is_modx:
        if (adr_vals.part == 3) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else
        {
          append_prefix(0xfd);
          append_prefix(0xed);
          append_opcode(0xc6 | ((adr_vals.part & 2) << 3));
          append_adr_vals(&adr_vals);
        }
        break;
      case ModRA:
        append_prefix(0xdd);
        append_prefix(0xed);
        append_opcode(0xf6);
        append_adr_vals(&adr_vals);
        break;
      case ModAbs:
        append_prefix(0xdd);
        append_prefix(0xed);
        append_opcode(0xd6);
        append_adr_vals(&adr_vals);
        break;
      case ModImm:
        if (is_z380())
        {
          append_prefix(0xed);
          append_opcode(0x86);
        }
        else /* is_z280() */
        {
          append_prefix(0xfd);
          append_prefix(0xed);
          append_opcode(0xf6);
        }
        append_adr_vals(&adr_vals);
        break;
      default:
        break;
    }
  }
}

static void DecodeADC_SBC(Word IsSBC)
{
  adr_vals_t adr_vals;
  Byte raw_prefix_cnt = PrefixCnt;

  switch (ArgCnt)
  {
    case 1:
      reset_adr_vals(&adr_vals);
      adr_vals.mode = ModReg8;
      adr_vals.part = AccReg;
      break;
    case 2:
      DecodeAdr(&ArgStr[1], &adr_vals, MModReg8 | (is_sharp() ? 0 : MModReg16));
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  switch (adr_vals.mode)
  {
    case ModReg8:
      if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        OpSize = eSymbolSize8Bit;
        switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModReg8 | MModImm | (is_z280() ? (MModAbs | MModX | MModRA | MModSR | MModBX) : 0), 4))
        {
          case ModReg8:
            append_opcode(0x88 + adr_vals.part + (IsSBC ? 0x10 : 0x00));
            append_adr_vals(&adr_vals);
            break;
          case ModAbs:
          case ModX:
          case ModRA:
          case ModSR:
          case ModBX:
            append_z280(&adr_vals, True, 0x88 | (IsSBC ? 0x10 : 0x00), 0);
            break;
          case ModImm:
            append_opcode(0xce + (IsSBC ? 0x10 : 0x00));
            append_adr_vals(&adr_vals);
            break;
          default:
            break;
        }
      }
      break;
    case ModReg16:
      if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else if (has_index_prefix() && !is_z280()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        prefix_store_t dest_prefix;
        store_prefix(&dest_prefix, raw_prefix_cnt);

        OpSize = eSymbolSize16Bit;
        switch (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModAll))
        {
          case ModReg16:
          {
            prefix_store_t src_prefix;
            store_prefix(&src_prefix, dest_prefix.cnt);

            if ((adr_vals.part == HLReg) && ((dest_prefix.present != src_prefix.present) || (dest_prefix.value != src_prefix.value))) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              if (dest_prefix.present && src_prefix.present)
                PrefixCnt--;
              append_prefix(0xed);
              append_opcode(0x42 + (adr_vals.part << 4) + (IsSBC ? 0 : 8));
            }
            break;
          }
          case ModNone:
            break;
          default:
            WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        }
      }
      break;
    default:
      break;
  }
}

static void DecodeADCW_SBCW(Word Code)
{
  if (ChkArgCnt(1, 2)
   && chk_core_mask(e_core_mask_z380)
   && ((ArgCnt == 1) || DecodeAdr_HL(&ArgStr[1])))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize16Bit;
    switch (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModAll))
    {
      case ModReg16:
        if (PrefixCnt > 0)      /* wenn Register, dann nie DDIR! */
          append_opcode(0x8f + Code);
        else if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else
        {
          if (adr_vals.part == HLReg)
            adr_vals.part = 3;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0x8c + Code + adr_vals.part;
          CodeLen = 2;
        }
        break;
      case ModReg8:
        if ((adr_vals.part != MReg) || (adr_vals.count == 0)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else
        {
          append_opcode(0xce + Code);
          append_adr_vals(&adr_vals);
        }
        break;
      case ModImm:
        CodeLen = 0;
        BAsmCode[CodeLen++] = 0xed;
        BAsmCode[CodeLen++] = 0x8e + Code;
        append_adr_vals(&adr_vals);
        break;
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
    }
  }
}

static void DecodeINC_DEC(Word Index)
{
  Word IsDEC = (Index & 1), IsWord = (Index & 2);

  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    switch (decode_adr_rel(&ArgStr[1], &adr_vals, MModReg8 | MModReg16 | (is_z280() ? (MModAbs | MModX | MModRA | (IsWord ? 0 : (MModSR | MModBX))) : 0), 4))
    {
      case ModReg8:
        if (IsWord)
        {
          if (!is_z280() || (adr_vals.part != MReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else if (has_index_prefix())
          {
            convert_sx_to_x(&adr_vals, BAsmCode[--PrefixCnt]);
            goto is_modx;
          }
          else
          {
            append_prefix(0xdd);
            append_opcode(0x03 | (IsDEC << 3));
          }
        }
        else
        {
          append_opcode(0x04 + (adr_vals.part << 3) + IsDEC);
          append_adr_vals(&adr_vals);
        }
        break;
      case ModReg16:
        append_opcode(0x03 + (adr_vals.part << 4) + (IsDEC << 3));
        break;
      case ModAbs:
        if (IsWord)
        {
          append_prefix(0xdd);
          append_opcode(0x13 | (IsDEC << 3));
          append_adr_vals(&adr_vals);
        }
        else
          append_z280(&adr_vals, True, 0x04 | IsDEC, 3);;
        break;
      case ModX:
      is_modx:
        if (IsWord)
        {
          if (adr_vals.part == 3) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else
          {
            append_prefix(0xfd);
            append_opcode(0x03 | (IsDEC << 3) | ((adr_vals.part & 2) << 3));
            append_adr_vals(&adr_vals);
          }
        }
        else
          append_z280(&adr_vals, True, 0x04 | IsDEC, 3);
        break;
      case ModRA:
        if (IsWord)
        {
          append_prefix(0xdd);
          append_opcode(0x33 | (IsDEC << 3));
          append_adr_vals(&adr_vals);
        }
        else
          append_z280(&adr_vals, True, 0x04 | IsDEC, 3);
        break;
      case ModSR:
      case ModBX:
        append_z280(&adr_vals, True, 0x04 | IsDEC, 3);
        break;
      default:
        break;
    }
  }
}

static void DecodeShift8(Word Code)
{
  Byte reg_num = 0;
  int mem_arg_index;
  adr_vals_t adr_vals;

  if (!ChkArgCnt(1, is_z80u() ? 2 : 1))
    return;
  if ((Code == 6) && !chk_core_mask(e_core_mask_z80u)) /* SLI(A)/SL1/SLS undok. Z80 */
    return;

  /* dual arg (Z80 undoc): which is the extra destination register? This must be a 'simple' register (A,B,C,D,E,H,L): */

  if (ArgCnt >= 2)
  {
    if (DecodeReg8Core(ArgStr[1].str.p_str, &reg_num) && !(reg_num & 0xc0))
      mem_arg_index = 2;
    else if (DecodeReg8Core(ArgStr[2].str.p_str, &reg_num) && !(reg_num & 0xc0))
      mem_arg_index = 1;
    else
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      return;
    }
  }

  /* single arg (documented version): */

  else
    mem_arg_index = 1;

  /* now decode the 'official argument': */

  OpSize = eSymbolSize8Bit;
  if (DecodeAdr(&ArgStr[mem_arg_index], &adr_vals, MModReg8) != ModReg8)
    return;

  /* forbid IXL..IYU: */

  if ((PrefixCnt > 0) && (adr_vals.part != MReg))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[mem_arg_index]);
    return;
  }

  /* replace adr_vals.part for undocumented version.  Addressing mode must be IXd/IYd: */

  if (ArgCnt >= 2)
  {
    if ((adr_vals.part != MReg) || (PrefixCnt != 1))
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[mem_arg_index]);
      return;
    }
    adr_vals.part = reg_num;
  }

  /* assemble instruction: */

  append_opcode(0xcb);
  append_adr_vals(&adr_vals);
  BAsmCode[CodeLen++] = (Code << 3) | adr_vals.part;
}

static void DecodeShift16(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (chk_core_mask(e_core_mask_z380))
  {
    adr_vals_t adr_vals;

    OpSize = eSymbolSize16Bit;
    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModNoImm))
    {
      case ModReg16:
        if (PrefixCnt > 0)
        {
          BAsmCode[2] = 0x04 + (Code << 3) + ((BAsmCode[0] >> 5) & 1);
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0xcb;
          CodeLen = 3;
        }
        else if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          if (adr_vals.part == HLReg)
            adr_vals.part = 3;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0xcb;
          BAsmCode[2] = (Code << 3) + adr_vals.part;
          CodeLen = 3;
        }
        break;
      case ModReg8:
        if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          if (adr_vals.count == 0)
          {
            BAsmCode[0] = 0xed;
            PrefixCnt = 1;
          }
          append_opcode(0xcb);
          append_adr_vals(&adr_vals);
          BAsmCode[CodeLen++] = 0x02 + (Code << 3);
        }
        break;
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
}

static void DecodeBit(Word Code)
{
  Byte reg_num = 0;
  int mem_arg_index, bit_arg_index;
  Boolean ok;
  tSymbolFlags symbol_flags;
  adr_vals_t adr_vals;

  /* extra undocumented dest register is not allowed for BIT */

  if (!ChkArgCnt(1, (is_z80u() && (Code != 0)) ? 3 : 2))
    return;

  /* triple arg (Z80 undoc): which is the extra destination register? This must be a 'simple' register (A,B,C,D,E,H,L): */

  if (ArgCnt >= 3)
  {
    if (DecodeReg8Core(ArgStr[1].str.p_str, &reg_num) && !(reg_num & 0xc0))
    {
      mem_arg_index = 3;
      bit_arg_index = 2;
    }
    else if (DecodeReg8Core(ArgStr[3].str.p_str, &reg_num) && !(reg_num & 0xc0))
    {
      mem_arg_index = 2;
      bit_arg_index = 1;
    }
    else
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      return;
    }
  }

  /* single arg (documented version): */

  else
  {
    mem_arg_index = 2;
    bit_arg_index = 1;
  }

  /* now decode the 'official arguments': */

  OpSize = eSymbolSize8Bit;
  if (DecodeAdr(&ArgStr[mem_arg_index], &adr_vals, MModReg8) != ModReg8)
    return;

  /* forbid IXL..IYU: */

  if ((PrefixCnt > 0) && (adr_vals.part != MReg))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[mem_arg_index]);
    return;
  }

  /* parse bit # and form machine code: */

  Code = ((Code + 1) << 6) | (EvalStrIntExpressionWithFlags(&ArgStr[bit_arg_index], UInt3, &ok, &symbol_flags) << 3);
  if (!ok)
    return;

  /* replace adr_vals.part for undocumented version.  Addressing mode must be IXd/IYd: */

  if (ArgCnt >= 3)
  {
    if ((adr_vals.part != MReg) || (PrefixCnt != 1))
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[mem_arg_index]);
      return;
    }
    adr_vals.part = reg_num;
  }

  /* assemble instruction: */

  append_opcode(0xcb);
  append_adr_vals(&adr_vals);
  set_b_guessed(symbol_flags, CodeLen, 1, 0x38);
  BAsmCode[CodeLen++] = Code | adr_vals.part;
}

static void DecodeMLT(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (chk_core_mask(e_core_mask_min_z180))
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[1], &adr_vals, MModReg16) != ModReg16);
    else if (has_index_prefix()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    else
    {
      append_prefix(0xed);
      append_opcode(0x4c + (adr_vals.part << 4));
    }
  }
}

static void DecodeMULT_DIV_Z380(Word Code)
{
  const tStrComp *pSrcArg;
  adr_vals_t adr_vals;

  if (!chk_core_mask(e_core_mask_z380)
   || !ChkArgCnt(1, 2))
    return;

  if (2 == ArgCnt)
  {
    if (!DecodeAdr_HL(&ArgStr[1]))
      return;
  }

  OpSize = eSymbolSize16Bit;
  pSrcArg = &ArgStr[ArgCnt];
  switch (DecodeAdr(pSrcArg, &adr_vals, MModReg8 | MModReg16 | MModImm))
  {
    case ModReg8:
      if ((adr_vals.part != MReg) || (PrefixCnt == 0)) WrStrErrorPos(ErrNum_InvAddrMode, pSrcArg);
      else
      {
        append_opcode(0xcb);
        append_adr_vals(&adr_vals);
        BAsmCode[CodeLen++] = 0x92 | Code;
      }
      break;
    case ModReg16:
      if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, pSrcArg);
      else if (PrefixCnt == 0)
      {
        if (adr_vals.part == HLReg)
          adr_vals.part = 3;
        BAsmCode[0] = 0xed;
        BAsmCode[1] = 0xcb;
        BAsmCode[2] = 0x90 + adr_vals.part + Code;
        CodeLen = 3;
      }
      else
      {
        BAsmCode[2] = 0x94 + ((BAsmCode[0] >> 5) & 1) + Code;
        BAsmCode[0] = 0xed;
        BAsmCode[1] = 0xcb;
        CodeLen = 3;
      }
      break;
    case ModImm:
      CodeLen = 0;
      BAsmCode[CodeLen++] = 0xed;
      BAsmCode[CodeLen++] = 0xcb;
      BAsmCode[CodeLen++] = 0x97 + Code;
      append_adr_vals(&adr_vals);
      break;
    default:
      break;
  }
}

static void DecodeMULT_DIV_16_Z280(Word code)
{
  adr_vals_t adr_vals;

  switch (ArgCnt)
  {
    case 1:
      break;
    case 2:
      if (!((code & 4) ? DecodeAdr_HL(&ArgStr[1]) : DecodeAdr_A(&ArgStr[1])))
        return;
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  OpSize = eSymbolSize8Bit;
  switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModReg8 | MModImm | MModAbs | MModX | MModRA | MModSR | MModBX, 5))
  {
    case ModReg8:
      append_prefix(0xed);
      append_opcode(0xc0 | (adr_vals.part << 3) | code);
      append_adr_vals(&adr_vals);
      break;
    case ModImm:
      append_prefix(0xfd);
      append_prefix(0xed);
      append_opcode(0xf8 | code);
      append_adr_vals(&adr_vals);
      break;
    case ModAbs:
    case ModX:
    case ModRA:
    case ModSR:
    case ModBX:
      append_z280(&adr_vals, True, 0xedc0 | code, 3);
      break;
  }
}

static void DecodeMULT_DIV_32_Z280(Word code)
{
   adr_vals_t adr_vals;

  switch (ArgCnt)
  {
    case 1:
      break;
    case 2:
      if (!((code & 8) ? DecodeAdr_DEHL(&ArgStr[1]) : DecodeAdr_HL(&ArgStr[1])))
        return;
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  OpSize = eSymbolSize16Bit;
  switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModReg8 | MModReg16 | MModAbs | MModImm | MModX | MModRA, 5))
  {
    case ModReg8:
      if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else if (has_index_prefix())
      {
        convert_sx_to_x(&adr_vals, BAsmCode[--PrefixCnt]);
        goto is_modx;
      }
      else
      {
        append_prefix(0xdd);
        append_prefix(0xed);
        append_opcode(0xc0 | code);
      }
      break;
    case ModReg16:
      append_prefix(0xed);
      append_opcode(0xc0 | (adr_vals.part << 4) | code);
      break;
    case ModImm:
      append_prefix(0xfd);
      append_prefix(0xed);
      append_opcode(0xf0 | code);
      append_adr_vals(&adr_vals);
      break;
    case ModAbs:
      append_prefix(0xdd);
      append_prefix(0xed);
      append_opcode(0xd0 | code);
      append_adr_vals(&adr_vals);
      break;
    case ModX:
    is_modx:
      if (adr_vals.part == 3) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
      {
        append_prefix(0xfd);
        append_prefix(0xed);
        append_opcode(0xc0 | ((adr_vals.part & 2) << 3) | code);
        append_adr_vals(&adr_vals);
      }
      break;
    case ModRA:
      append_prefix(0xdd);
      append_prefix(0xed);
      append_opcode(0xf0 | code);
      append_adr_vals(&adr_vals);
    default:
      break;
  }
}

static void DecodeTST(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 2));
  else if (chk_core_mask(e_core_mask_min_z180))
  {
    adr_vals_t adr_vals;

    if (ArgCnt == 2)
    {
      if (DecodeAdr(&ArgStr[1], &adr_vals, MModReg8) != ModReg8)
        return;
      if (adr_vals.part != AccReg)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        return;
      }
    }
    OpSize = eSymbolSize8Bit;
    switch (DecodeAdr(&ArgStr[ArgCnt], &adr_vals, MModAll))
    {
      case ModReg8:
        if (has_index_prefix()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
        else
        {
          append_prefix(0xed);
          append_opcode(4 + (adr_vals.part << 3));
        }
        break;
      case ModImm:
        append_prefix(0xed);
        append_opcode(0x64);
        append_adr_vals(&adr_vals);
        break;
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
    }
  }
}

static void DecodeSWAP(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (chk_core_mask(e_core_mask_z380 | e_core_mask_sharp))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(&ArgStr[1], &adr_vals, is_z380() ? MModReg16 : MModReg8))
    {
      case ModReg16:
        if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else if (PrefixCnt == 0)
        {
          if (adr_vals.part == HLReg)
            adr_vals.part = 3;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0x0e + (adr_vals.part << 4); /*?*/
          CodeLen = 2;
        }
        else
          append_opcode(0x3e);
        break;
      case ModReg8:
        BAsmCode[0] = 0xcb;
        BAsmCode[1] = 0x30 | adr_vals.part;
        CodeLen = 2;
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePUSH_POP(Word Code)
 * \brief  handle PUSH/POP instructions
 * \param  Code machine code (4 = PUSH family, 0 = POP family)
 * ------------------------------------------------------------------------ */

static void DecodePUSH_POP(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "SR"))
  {
    if (chk_core_mask(e_core_mask_z380))
    {
      CodeLen = 2;
      BAsmCode[0] = 0xed;
      BAsmCode[1] = 0xc1 + Code;
    }
  }
  else
  {
    adr_vals_t adr_vals;
    ShortInt adr_mode;

    OpSize = eSymbolSize16Bit; MayLW = True;
    if (!as_strcasecmp(ArgStr[1].str.p_str, "AF"))
    {
      reset_adr_vals(&adr_vals);
      adr_vals.part = SPReg;
      adr_mode = ModReg16;
    }
    else
    {
      LongWord mask = MModReg16;

      if ((Code == 4) && (is_z380() || is_z280()))
        mask |= MModImm;
      if (is_z280())
        mask |= MModAbs | MModRA | MModReg8;
      adr_mode = decode_adr_rel(&ArgStr[1], &adr_vals, mask, 4);
    }
    switch (adr_mode)
    {
      case ModReg8:
        if ((adr_vals.part != MReg) || PrefixCnt) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          append_prefix(0xdd);
          append_opcode(0xc1 | Code);
          append_adr_vals(&adr_vals);
        }
        break;
      case ModReg16:
        append_opcode(0xc1 + (adr_vals.part << 4) + Code);
        break;
      case ModImm:
        append_prefix(0xfd);
        append_opcode(0xf5);
        append_adr_vals(&adr_vals);
        break;
      case ModAbs:
        append_prefix(0xdd);
        append_opcode(0xd1 | Code);
        append_adr_vals(&adr_vals);
        break;
      case ModRA:
        append_prefix(0xdd);
        append_opcode(0xf1 | Code);
        append_adr_vals(&adr_vals);
        break;
      default:
        break;
    }
  }
}

static void DecodeEX(Word Index)
{
  Boolean src_had_quote;
  adr_vals_t adr_vals;
  Byte raw_prefix_cnt;
  int src_len;

  UNUSED(Index);

  /* No EX at all on GBZ80 */

  if (!chk_core_mask(e_core_mask_no_sharp))
    return;

  /* work around the parser problem related to the ' character */

  if (!as_strncasecmp(ArgStr[2].str.p_str, "AF\'", 3))
    ArgStr[2].str.p_str[3] = '\0';

  if (!ChkArgCnt(2, 2))
    return;
  else if (ParPair("AF", "AF\'"))
  {
    append_opcode(0x08);
    return;
    
  }
  else if (ParPair("AF", "AF`"))
  {
    append_opcode(0x08);
    return;
  }

  src_len = strlen(ArgStr[2].str.p_str);

  if ((src_len > 0) && (ArgStr[2].str.p_str[src_len - 1] == '\''))
  {
    src_had_quote = True;
    ArgStr[2].str.p_str[--src_len] = '\0';
  }
  else
    src_had_quote = False;

  raw_prefix_cnt = PrefixCnt;
  switch (decode_adr_rel(&ArgStr[1], &adr_vals, MModReg8 | MModReg16 | MModSPRel | MModIndReg16 | (is_z280() ? (MModAbs | MModX | MModRA | MModSR | MModBX) : 0), 5))
  {
    case ModReg8:
    {
      Byte dest_reg = adr_vals.part;
      prefix_store_t dest_prefix;
      store_prefix(&dest_prefix, raw_prefix_cnt);

      if (dest_reg == MReg) /* EX (HL),... EX ; (XY+d),... */ 
      {
        if (dest_prefix.present && !is_z280()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else if (DecodeAdr_A(&ArgStr[2]) && chk_core_mask(e_core_mask_z280 | e_core_mask_z380)) /* EX (HL),A EX ; (XY+d),A */
        {
          append_prefix(0xed);
          append_opcode(0x37);
          append_adr_vals(&adr_vals);
        }
      }
      else /* EX R,... */
      {
        switch (decode_adr_rel(&ArgStr[2], &adr_vals, MModReg8 | (is_z280() ? (MModAbs | MModX | MModRA | MModSR | MModBX) : 0), 5))
        {
          case ModReg8:
          {
            prefix_store_t src_prefix;
            store_prefix(&src_prefix, dest_prefix.cnt);

            if (adr_vals.part == MReg) /* EX R,(HL) ; EX R,(XY+d) */
            {
              if ((dest_reg != AccReg) || (src_prefix.present && !is_z280())) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else if (chk_core_mask(e_core_mask_z280 | e_core_mask_z380))
              {
                append_prefix(0xed);
                append_opcode(0x37);
                append_adr_vals(&adr_vals);
              }
            }
            else if ((dest_reg == AccReg) && !src_had_quote) /* EX A,R */
            {
              if (chk_core_mask(e_core_mask_z380 | e_core_mask_z280))
              {
                if (src_prefix.present && !is_z280()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]); /* EX A,IXYHL only on Z280 */
                else
                {
                  append_prefix(0xed);
                  append_opcode(0x07 + (adr_vals.part << 3));
                }
              }
            }
            else if ((adr_vals.part == AccReg) && !src_had_quote) /* EX R,A */
            {
              if (chk_core_mask(e_core_mask_z380 | e_core_mask_z280))
              {
                if (dest_prefix.present && !is_z280()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]); /* EX IXYHL,A only on Z280 */
                else
                {
                  append_prefix(0xed);
                  append_opcode(0x07 + (dest_reg << 3));
                }
              }
            }
            else if (src_had_quote && (adr_vals.part == dest_reg)) /* EX R',R */
            {
              if (chk_core_mask(e_core_mask_z380))
              {
                if (src_prefix.present) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
                else
                {
                  BAsmCode[0] = 0xcb;
                  BAsmCode[1] = 0x30 + adr_vals.part;
                  CodeLen = 2;
                }
              }
            }
            else if ((dest_reg == HReg && adr_vals.part == LReg) /* EX H,L */
                  || (dest_reg == LReg && adr_vals.part == HReg)) /* EX L,H */
            {
              if (chk_core_mask(e_core_mask_z280))
              {
                if (dest_prefix.present) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
                else if (src_prefix.present) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
                else
                {
                  append_prefix(0xed);
                  append_opcode(0xef);
                }
              }
            }
            else WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          }
          case ModAbs:
          case ModX:
          case ModRA:
          case ModSR:
          case ModBX:
            if (dest_reg != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
            else
              append_z280(&adr_vals, True, 0xed07, 3);
            break;
          default:
            break;
        }
      }
      break;
    }
    case ModReg16:
    {
      Byte dest_reg;
      prefix_store_t dest_prefix;
      store_prefix(&dest_prefix, raw_prefix_cnt);

      if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else if (!dest_prefix.present) /* EX R16,... */
      {
        dest_reg = (adr_vals.part == HLReg) ? SPReg : adr_vals.part;
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg16 | ((adr_vals.part == HLReg) ? MModSPRel : 0)))
        {
          case ModReg16:
          {
            prefix_store_t src_prefix;
            store_prefix(&src_prefix, dest_prefix.cnt);
            
            /* For DE <-> IX/IY, use the DD/FD prefix and DE<->HL on Z80/Z280, but the newer coding on Z380 */

            if (((dest_reg == DEReg) && (adr_vals.part == HLReg) && (!PrefixCnt || !is_z380())) /* DE <-> HL */
             || ((dest_reg == SPReg) && (adr_vals.part == DEReg) && (!PrefixCnt || !is_z380()))) /* HL(->SP) <-> DE */
              append_opcode(0xeb);

            /* HL <-> IX/IY on Z280: */

            else if ((is_z280()) && (dest_reg == SPReg) && (adr_vals.part == HLReg) && src_prefix.present)
              append_opcode(0xeb);

            else if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (!chk_core_mask(e_core_mask_z380));
            else if (src_had_quote)
            {
              if (adr_vals.part == HLReg)
                adr_vals.part = 3;
              if ((PrefixCnt != 0) || (adr_vals.part != dest_reg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else
              {
                BAsmCode[0] = 0xed;
                BAsmCode[1] = 0xcb;
                BAsmCode[2] = 0x30 + dest_reg;
                CodeLen = 3;
              }
            }
            else if (PrefixCnt == 0)
            {
              if (dest_reg == BCReg)
              {
                if (adr_vals.part == HLReg)
                  adr_vals.part = 3;
                BAsmCode[0] = 0xed;
                BAsmCode[1] = 0x01 + (adr_vals.part << 2);
                CodeLen = 2;
              }
              else if (adr_vals.part == BCReg)
              {
                BAsmCode[0] = 0xed;
                BAsmCode[1] = 0x01 + (dest_reg << 2);
                CodeLen = 2;
              }
            }
            else
            {
              if (adr_vals.part == HLReg)
                adr_vals.part = 3;
              BAsmCode[1] = 0x03 + ((BAsmCode[0] >> 2) & 8) + (dest_reg << 4);
              BAsmCode[0] = 0xed;
              CodeLen = 2;
            }
            break;
          }
          case ModSPRel:
            if ((adr_vals.count == 1) && !adr_vals.values[0]) /* HL <-> (SP) */
              append_opcode(0xe3);
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
      }
      else /* EX XY,... */
      {
        switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg16 | MModSPRel))
        {
          case ModReg16:
          {
            prefix_store_t src_prefix;
            store_prefix(&src_prefix, dest_prefix.cnt);

            if (adr_vals.part == SPReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            /* IX/IY <-> HL on Z280: */
            else if (is_z280() && (adr_vals.part == HLReg) && !src_prefix.present)
              append_opcode(0xeb);
            else if (!chk_core_mask(e_core_mask_z380));
            else if (src_had_quote)
            {
              if ((PrefixCnt != 2) || (BAsmCode[0] != BAsmCode[1])) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
              else
              {
                BAsmCode[2] = ((BAsmCode[0] >> 5) & 1) + 0x34;
                BAsmCode[0] = 0xed;
                BAsmCode[1] = 0xcb;
                CodeLen = 3;
              }
            }
            else if (PrefixCnt == 1)
            {
              if (adr_vals.part == HLReg)
                adr_vals.part = 3;
              BAsmCode[1] = ((BAsmCode[0] >> 2) & 8) + 3 + (adr_vals.part << 4);
              BAsmCode[0] = 0xed;
              CodeLen = 2;
            }
            else if (BAsmCode[0] == BAsmCode[1]) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[0] = 0xed;
              BAsmCode[1] = 0x2b;
              CodeLen = 2;
            }
            break;
          }
          case ModSPRel:
            if ((adr_vals.count == 1) && !adr_vals.values[0]) /* IX/IX <-> (SP) */
              append_opcode(0xe3);
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          default:
            break;
        }
      }
      break;
    }
    case ModSPRel:
      if ((adr_vals.count != 1) || adr_vals.values[0]) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg16))
      {
        case ModReg16: /* (SP) <-> HL/IX/IX */
          if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          else
            append_opcode(0xe3);
          break;
        default:
          break;
      }
      break;
    case ModAbs:
      if (DecodeAdr_A(&ArgStr[2]))
      {
        append_prefix(0xdd);
        append_prefix(0xed);
        append_opcode(0x3f);
        append_adr_vals(&adr_vals);
      }
      break;
    case ModX:
    case ModRA:
    case ModBX:
      if (DecodeAdr_A(&ArgStr[2]))
        append_z280(&adr_vals, True, 0xed07, 3);
      break;
    case ModSR:
    {
      adr_vals_t src_adr_vals;

      switch (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg8 | MModReg16))
      {
        case ModReg8: /* EX (SP+dd),A */
          if (src_adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            append_z280(&adr_vals, True, 0xed07, 3);
          break;
        case ModReg16: /* EX (SP),RR ends up here on Z280 */
          if ((adr_vals.count != 2) || adr_vals.values[0] || adr_vals.values[1]) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else if (src_adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          else
            append_opcode(0xe3);
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

static void decode_inw_outw_c_core(Byte reg, const tStrComp *p_reg_arg, Word is_out)
{
  if ((reg == SPReg) || (PrefixCnt > 0)) WrStrErrorPos(ErrNum_InvAddrMode, p_reg_arg);
  else if (is_z380())
  {
    switch (reg)
    {
      case DEReg: reg = 2; break;
      case HLReg: reg = 7; break;
    }
    BAsmCode[0] = 0xdd;
    BAsmCode[1] = 0x40 + (reg << 3) + !!is_out;
    CodeLen = 2;
  }
  else if (reg != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, p_reg_arg);
  else
  {
    append_prefix(0xed);
    append_opcode(0xb7 | (!!is_out << 3));
  }
}

static void DecodeIN_OUT(Word IsOUT)
{
  adr_vals_t adr_vals;

  if ((ArgCnt == 1) && !IsOUT)
  {
    if (chk_core_mask(e_core_mask_z80u)
     && (DecodeAdr(&ArgStr[1], &adr_vals, MModIndReg8) == ModIndReg8))
    {
      BAsmCode[0] = 0xed;
      BAsmCode[1] = 0x70;
      CodeLen = 2;
    }
  }
  else if (ChkArgCnt(2, 2) && chk_core_mask(e_core_mask_no_sharp | e_core_mask_z280_io_privileged))
  {
    const tStrComp *pPortArg = IsOUT ? &ArgStr[1] : &ArgStr[2],
                   *pRegArg = IsOUT ? &ArgStr[2] : &ArgStr[1];

    /* allow absolute I/O address also without (...) */

    /* TODO: check I/O privilege on Z280 */

    OpSize = eSymbolSize8Bit;
    switch (DecodeAdr(pPortArg, &adr_vals, (is_ez80() ? MModIndReg16 : 0) | MModIndReg8 | MModIOAbs | MModImm))
    {
      case ModIndReg16:
        if (BCReg != adr_vals.part) WrStrErrorPos(ErrNum_InvAddrMode, pPortArg);
        else if (ModReg8 == DecodeAdr(pRegArg, &adr_vals, MModReg8))
        {
          CodeLen = 2;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0x40 | (adr_vals.part << 3) | !!IsOUT;
        }
        break;
      case ModIndReg8:
        switch (DecodeAdrWithF(pRegArg, &adr_vals, MModAll | (is_z280() ? (MModX | MModRA | MModSR | MModBX) : 0), 5, !IsOUT))
        {
          case ModReg8:
            if ((PrefixCnt != 0) && !is_z280()) WrStrErrorPos(ErrNum_InvAddrMode, pPortArg);
            else
            {
              append_prefix(0xed);
              append_opcode(0x40 + (adr_vals.part << 3) + !!IsOUT);
            }
            break;
          case ModReg16:
            decode_inw_outw_c_core(adr_vals.part, pRegArg, IsOUT);
            break;
          case ModImm:
            if (!IsOUT) WrStrErrorPos(ErrNum_InvAddrMode, pPortArg);
            else if (is_z80u() && (adr_vals.values[0] == 0))
            {
              BAsmCode[0] = 0xed;
              BAsmCode[1] = 0x71;
              CodeLen = 2;
            }
            else if (chk_core_mask(e_core_mask_z380))
            {
              BAsmCode[0] = 0xed;
              BAsmCode[1] = 0x71;
              BAsmCode[2] = adr_vals.values[0];
              CodeLen = 3;
            }
            break;
          case ModAbs:
          case ModX:
          case ModRA:
          case ModSR:
          case ModBX:
            append_z280(&adr_vals, True, 0xed40 | !!IsOUT, 3);
            break;
          case ModNone:
            break;
          default:
            WrStrErrorPos(ErrNum_InvAddrMode, pRegArg);
        }
        break;
      case ModIOAbs:
      case ModImm:
        if (DecodeAdr_A(pRegArg))
        {
          CodeLen = 2;
          BAsmCode[0] = IsOUT ? 0xd3 : 0xdb;
          BAsmCode[1] = adr_vals.values[0];
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeINW_OUTW(Word IsOUTW)
{
  const tStrComp *pPortArg, *pRegArg;
  adr_vals_t adr_vals;

  if (!ChkArgCnt(2, 2) || !chk_core_mask(e_core_mask_z380 | e_core_mask_z280))
    return;

  pPortArg = IsOUTW ? &ArgStr[1] : &ArgStr[2];
  pRegArg  = IsOUTW ? &ArgStr[2] : &ArgStr[1];

  if (DecodeAdr(pPortArg, &adr_vals, MModIndReg8) != ModIndReg8)
    return;

  OpSize = eSymbolSize16Bit;
  switch (DecodeAdr(pRegArg, &adr_vals, MModReg16 | ((IsOUTW && is_z380()) ? MModImm : 0)))
  {
    case ModReg16:
      decode_inw_outw_c_core(adr_vals.part, pRegArg, IsOUTW);
      break;
    case ModImm:
      CodeLen = 0;
      BAsmCode[CodeLen++] = 0xfd;
      BAsmCode[CodeLen++] = 0x79;
      append_adr_vals(&adr_vals);
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIN0_OUT0(Word IsOUT0)
 * \brief  Handle IN0/OUT0 instructions on Z180++
 * \param  IsOUT0 1 for OUT0, 0 for IN0
 * ------------------------------------------------------------------------ */

static void DecodeIN0_OUT0(Word IsOUT0)
{
  /* 'IN0 (C)' better should not be allowed at all, because it was a copy'n'waste from
     the undocumented Z80 'IN (C)' which should better have been named 'IN F,(C)'.  But
     I will leave it in for upward compatibility, and not implicitly assume A as register: */

  if (ChkArgCnt(IsOUT0 ? 2 : 1, 2)
   && chk_core_mask(e_core_mask_min_z180))
  {
    Boolean OK = False;
    const tStrComp *pRegArg, *pPortArg;
    adr_vals_t adr_vals;

    if (IsOUT0)
    {
      pRegArg = (ArgCnt == 2) ? &ArgStr[2] : NULL;
      pPortArg = &ArgStr[1];
    }
    else
    {
      pRegArg = (ArgCnt == 2) ? &ArgStr[1] : NULL;
      pPortArg = &ArgStr[ArgCnt];
    }
    OpSize = eSymbolSize8Bit;
    if (!pRegArg)
    {
      reset_adr_vals(&adr_vals);
      adr_vals.part = MReg;
      OK = True;
    }
    else
    {
      switch (DecodeAdrWithF(pRegArg, &adr_vals, MModAll, 0, !IsOUT0))
      {
        case ModReg8:
          if (PrefixCnt != 0) WrStrErrorPos(ErrNum_InvAddrMode, pRegArg);
          else
            OK = True;
          break;
        case ModNone:
          break;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, pRegArg);
      }
    }
    if (OK)
    {
      tSymbolFlags symbol_flags;

      BAsmCode[2] = EvalStrIntExpressionWithFlags(pPortArg, UInt8, &OK, &symbol_flags);
      if (OK)
      {
        set_b_guessed(symbol_flags, 2, 1, 0xff);
        BAsmCode[0] = 0xed;
        BAsmCode[1] = adr_vals.part << 3;
        if (IsOUT0)
          BAsmCode[1]++;
        CodeLen = 3;
      }
    }
  }
}

static void DecodeINA_INAW_OUTA_OUTAW(Word Code)
{
  Word IsIn = Code & 8;
  LongWord AdrLong;
  tStrComp *pRegArg, *pPortArg;
  tEvalResult EvalResult;

  if (!ChkArgCnt(2, 2) || !chk_core_mask(e_core_mask_z380))
    return;

  pRegArg = IsIn ? &ArgStr[1] : &ArgStr[2];
  pPortArg = IsIn ? &ArgStr[2] : &ArgStr[1];

  OpSize = (tSymbolSize)(Code & 1);
  if (!(OpSize ? DecodeAdr_HL(pRegArg) : DecodeAdr_A(pRegArg)))
    return;
  
  AdrLong = EvalStrIntExpressionWithResult(pPortArg, ExtFlag ? Int32 : UInt8, &EvalResult);
  if (EvalResult.OK)
  {
    ChkSpace(SegIO, EvalResult.AddrSpaceMask);
    if (AdrLong > 0xfffffful)
      ChangeDDPrefix(ePrefixIW);
    else if (AdrLong > 0xfffful)
      ChangeDDPrefix(ePrefixIB);
    append_prefix(0xed + (OpSize << 4));
    append_opcode(0xd3 + IsIn);
    set_b_guessed(EvalResult.Flags, CodeLen, 2, 0xff);
    BAsmCode[CodeLen++] = AdrLong & 0xff;
    BAsmCode[CodeLen++] = (AdrLong >> 8) & 0xff;
    if (AdrLong > 0xfffful)
    {
      set_b_guessed(EvalResult.Flags, CodeLen, 1, 0xff);
      BAsmCode[CodeLen++] = (AdrLong >> 16) & 0xff;
    }
    if (AdrLong > 0xfffffful)
    {
      set_b_guessed(EvalResult.Flags, CodeLen, 1, 0xff);
      BAsmCode[CodeLen++] = (AdrLong >> 24) & 0xff;
    }
  }
}

static void DecodeTSTIO(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && chk_core_mask(e_core_mask_min_z180))
  {
    Boolean OK;
    tSymbolFlags symbol_flags;

    BAsmCode[2] = EvalStrIntExpressionWithFlags(&ArgStr[1], Int8, &OK, &symbol_flags);
    if (OK)
    {
      set_b_guessed(symbol_flags, 2, 1, 0xff);
      BAsmCode[0] = 0xed;
      BAsmCode[1] = 0x74;
      CodeLen = 3;
    }
  }
}

static void DecodeRET(Word Code)
{
  int Cond;

  UNUSED(Code);

  /* TODO: allow only no and .L as eZ80 attribute */

  if (ArgCnt == 0)
    append_opcode(0xc9);
  else if (!ChkArgCnt(0, 1));
  else if (!DecodeCondition(ArgStr[1].str.p_str, &Cond)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
  else
    append_opcode(0xc0 + (Cond << 3));
}

static void encode_jp_core(Byte condition, const adr_vals_t *p_vals)
{
  append_opcode(0xc2 + condition);
  append_adr_vals(p_vals);
}

static IntType get_jr_dist(LongWord dest, LongInt *p_dist)
{
  LongWord curr_pc;

  if (!get_logical_prog_counter(&curr_pc, 2))
    return UInt0;

  *p_dist = dest - curr_pc;
  if (RangeCheck(*p_dist, SInt8))
    return SInt8;
  if (is_z380())
  {
    *p_dist -= 2;
    if (RangeCheck(*p_dist, SInt16))
      return SInt16;
    (*p_dist)--;
    if (RangeCheck(*p_dist, SInt24))
      return SInt24;
  }
  return UInt0;
}

static void DecodeJP(Word Code)
{
  int Cond;
  Boolean check_jr = False;
  adr_vals_t adr_vals;
  LongWord mask;

  UNUSED(Code);

  switch (ArgCnt)
  {
    case 1:
      Cond = 1;
      check_jr = True;
      break;
    case 2:
      if (!DecodeCondition(ArgStr[1].str.p_str, &Cond))
      {
        WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        return;
      }
      check_jr = (Cond <= 3);
      Cond <<= 3;
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  mask = MModImmIsAbs | MModAbs;
  if (is_z280())
    mask |= MModRA;
  if ((Cond == 1) || is_z280())
    mask |= MModReg8;
  switch(decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, mask, 4))
  {
    case ModReg8:
      if ((adr_vals.part != MReg) || ((adr_vals.count > 0) && adr_vals.values[0])) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else if (Cond == 1)
        append_opcode(0xe9);
      else
      {
        append_prefix(0xdd);
        append_opcode(0xc2 | Cond);
      }
      break;
    case ModAbs:
    {
      LongInt dist;

      /* JP.SIL nnnn, JP.LIS nnnn are illegal on eZ80: */

      if (is_ez80()
       && (((AttrPartOpSize[0] == eSymbolSize16Bit) && (AttrPartOpSize[1] == eSymbolSize24Bit))
        || ((AttrPartOpSize[0] == eSymbolSize24Bit) && (AttrPartOpSize[1] == eSymbolSize16Bit))))
      {
        WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
        return;
      }

      if (check_jr 
       && (get_jr_dist(adr_vals.value, &dist) != UInt0)
       && !mFirstPassUnknownOrQuestionable(adr_vals.value_flags))
        WrStrErrorPos(ErrNum_RelJumpPossible, &ArgStr[ArgCnt]);
      encode_jp_core(Cond, &adr_vals);
      break;
    }
    case ModRA:
      append_prefix(0xfd);
      encode_jp_core(Cond, &adr_vals);
      break;
  }
}

static void DecodeCALL(Word Code)
{
  int Condition;
  adr_vals_t adr_vals;

  UNUSED(Code);

  switch (ArgCnt)
  {
    case 1:
      Condition = 9;
      break;
    case 2:
      if (!DecodeCondition(ArgStr[1].str.p_str, &Condition))
      {
        WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        return;
      }
      Condition <<= 3;
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  /* For the eZ80, a 'CALL.IL' sets ADL to 1, and a
     'CALL.IS' sets ADL to 0. So we have to adapt the
     first operand size to what the new ADL mode will
     be: */

  if (is_ez80())
  {
    if (!Reg_ADL && (AttrPartOpSize[1] == eSymbolSize24Bit))
      AttrPartOpSize[0] = eSymbolSize24Bit;
    else if (Reg_ADL && (AttrPartOpSize[1] == eSymbolSize16Bit))
      AttrPartOpSize[0] = eSymbolSize16Bit;
  }

  switch (decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, MModImmIsAbs | MModAbs | (is_z280() ? (MModReg8 | MModRA) : 0), 4))
  {
    case ModReg8:
      if ((adr_vals.part != MReg) || has_index_prefix()) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else
      {
        append_prefix(0xdd);
        goto common;
      }
      break;
    case ModRA:
      append_prefix(0xfd);
      goto common;
    case ModAbs:
    common:
      append_opcode(0xc4 + Condition);
      append_adr_vals(&adr_vals);
      break;
  }
}

static void encode_jr_core(IntType dist_size, Byte condition, LongInt dist, tSymbolFlags symbol_flags)
{
  switch (dist_size)
  {
    case SInt8:
      CodeLen = 2;
      BAsmCode[0] = condition << 3;
      BAsmCode[1] = dist & 0xff;
      set_b_guessed(symbol_flags, 1, 1, 0xff);
      break;
    case SInt16:
      CodeLen = 4;
      BAsmCode[0] = 0xdd;
      BAsmCode[1] = condition << 3;
      BAsmCode[2] = dist & 0xff;
      BAsmCode[3] = (dist >> 8) & 0xff;
      set_b_guessed(symbol_flags, 2, 2, 0xff);
      break;
    case SInt24:
      CodeLen = 5;
      BAsmCode[0] = 0xfd;
      BAsmCode[1] = condition << 3;
      BAsmCode[2] = dist & 0xff;
      BAsmCode[3] = (dist >> 8) & 0xff;
      BAsmCode[4] = (dist >> 16) & 0xff;
      set_b_guessed(symbol_flags, 2, 3, 0xff);
      break;
    default:
      break;
  }
}

static void DecodeJR(Word Code)
{
  Boolean OK;
  int Condition;
  LongWord dest;
  tEvalResult EvalResult;
  LongInt dist;
  IntType dist_type;

  UNUSED(Code);

  switch (ArgCnt)
  {
    case 1:
      Condition = 3;
      OK = True;
      break;
    case 2:
      OK = DecodeCondition(ArgStr[1].str.p_str, &Condition);
      if (OK && (Condition > 3))
        OK = False;
      if (OK)
        Condition += 4;
      else
        WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      break;
    default:
      (void)ChkArgCnt(1, 2);
      OK = False;
  }
  if (!OK)
    return;

  dest = EvalAbsAdrExpression(&ArgStr[ArgCnt], &EvalResult);
  if (!EvalResult.OK)
    return;

  dist_type = get_jr_dist(dest, &dist);
  if (dist_type == UInt0)
  {
    if (mFirstPassUnknownOrQuestionable(EvalResult.Flags))
      dist_type = is_z380() ? SInt24 : SInt8;
    else
    {
      WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[ArgCnt]);
      return;
    }
  }
    
  encode_jr_core(dist_type, Condition, dist, EvalResult.Flags);
}

static void DecodeJ(Word Code)
{
  int condition;
  adr_vals_t adr_vals;
  LongWord mask;

  UNUSED(Code);

  switch (ArgCnt)
  {
    case 1:
      condition = 0xff;
      break;
    case 2:
      if (!DecodeCondition(ArgStr[1].str.p_str, &condition))
      {
        WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        return;
      }
      break;
    default:
      (void)ChkArgCnt(1, 2);
      return;
  }

  mask = MModImmIsAbs | MModAbs;
  if (is_z280())
    mask |= MModRA;
  if ((condition == 0xff) || is_z280())
    mask |= MModReg8;
  switch(decode_adr_rel(&ArgStr[ArgCnt], &adr_vals, mask, 4))
  {
    case ModReg8:
      if ((adr_vals.part != MReg) || ((adr_vals.count > 0) && adr_vals.values[0])) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt]);
      else if (condition == 0xff)
        append_opcode(0xe9);
      else
      {
        append_prefix(0xdd);
        append_opcode(0xc2 | (condition << 3));
      }
      break;
    case ModAbs:
      if ((condition <= 3) || (condition == 0xff))
      {
        LongInt dist;
        IntType dist_type = get_jr_dist(adr_vals.value, &dist);

        if (dist_type != UInt0)
        {
          encode_jr_core(dist_type, (condition == 0xff) ? 3 : (condition + 4), dist, adr_vals.value_flags);
          return;
        }
      }
      encode_jp_core((condition == 0xff) ? 1 : (condition << 3), &adr_vals);
      break;
    case ModRA:
      append_prefix(0xfd);
      encode_jp_core((condition == 0xff) ? 1 : (condition << 3), &adr_vals);
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jaf_jar(Word code)
 * \brief  handle JAF/JAR instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jaf_jar(Word code)
{
  if (ChkArgCnt(1, 1) && chk_core_mask(e_core_mask_z280))
  {
    LongWord dest, curr_pc;
    tEvalResult eval_result;

    dest = EvalAbsAdrExpression(&ArgStr[1], &eval_result);
    if (eval_result.OK && get_logical_prog_counter(&curr_pc, 2 + !!Hi(code)))
    {
      LongInt dist = dest - curr_pc;
      if (((dist > 0x7fl) || (dist < -0x80l)) && !mFirstPassUnknownOrQuestionable(eval_result.Flags)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        append_opcode16(code);
        set_b_guessed(eval_result.Flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = dist & 0xff;
      }
    }
  }
}

static void DecodeCALR(Word Code)
{
  Boolean OK;
  int Condition;

  UNUSED(Code);

  switch (ArgCnt)
  {
    case 1:
      Condition = 9;
      OK = True;
      break;
    case 2:
      OK = DecodeCondition(ArgStr[1].str.p_str, &Condition);
      if (OK)
        Condition <<= 3;
      else
        WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      break;
    default:
      (void)ChkArgCnt(1, 2);
      OK = False;
  }

  if (OK)
  {
    if (chk_core_mask(e_core_mask_z380))
    {
      LongWord dest, curr_log_pc;
      tEvalResult EvalResult;

      dest = EvalAbsAdrExpression(&ArgStr[ArgCnt], &EvalResult);
      if (EvalResult.OK && get_logical_prog_counter(&curr_log_pc, 3))
      {
        LongInt dist = dest - curr_log_pc;
        if ((dist <= 0x7fl) && (dist >= -0x80l))
        {
          CodeLen = 3;
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0xc4 | Condition;
          set_b_guessed(EvalResult.Flags, 2, 1, 0xff);
          BAsmCode[2] = dist & 0xff;
        }
        else
        {
          dist--;
          if ((dist <= 0x7fffl) && (dist >= -0x8000l))
          {
            CodeLen = 4;
            BAsmCode[0] = 0xdd;
            BAsmCode[1] = 0xc4 + Condition;
            set_b_guessed(EvalResult.Flags, 2, 2, 0xff);
            BAsmCode[2] = dist & 0xff;
            BAsmCode[3] = (dist >> 8) & 0xff;
          }
          else
          {
            dist--;
            if ((dist <= 0x7fffffl) && (dist >= -0x800000l))
            {
              CodeLen = 5;
              BAsmCode[0] = 0xfd;
              BAsmCode[1] = 0xc4 + Condition;
              set_b_guessed(EvalResult.Flags, 2, 3, 0xff);
              BAsmCode[2] = dist & 0xff;
              BAsmCode[3] = (dist >> 8) & 0xff;
              BAsmCode[4] = (dist >> 16) & 0xff;
            }
            else WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[ArgCnt]);
          }
        }
      }
    }
  }
}

static void DecodeDJNZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1) && chk_core_mask(e_core_mask_no_sharp))
  {
    tEvalResult EvalResult;
    LongWord dest, curr_log_pc;

    dest = EvalAbsAdrExpression(&ArgStr[1], &EvalResult);
    if (EvalResult.OK && get_logical_prog_counter(&curr_log_pc, 2))
    {
      LongInt dist = dest - curr_log_pc;
      if ((dist <= 0x7fl) & (dist >= -0x80l))
      {
        CodeLen = 2;
        BAsmCode[0] = 0x10;
        set_b_guessed(EvalResult.Flags, 2, 1, 0xff);
        BAsmCode[1] = Lo(dist);
      }
      else if (!is_z380()) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        dist -= 2;
        if ((dist <= 0x7fffl) && (dist >= -0x8000l))
        {
          CodeLen = 4;
          BAsmCode[0] = 0xdd;
          BAsmCode[1] = 0x10;
          set_b_guessed(EvalResult.Flags, 2, 2, 0xff);
          BAsmCode[2] = dist & 0xff;
          BAsmCode[3] = (dist >> 8) & 0xff;
        }
        else
        {
          dist--;
          if ((dist <= 0x7fffffl) && (dist >= -0x800000l))
          {
            CodeLen = 5;
            BAsmCode[0] = 0xfd;
            BAsmCode[1] = 0x10;
            set_b_guessed(EvalResult.Flags, 2, 3, 0xff);
            BAsmCode[2] = dist & 0xff;
            BAsmCode[3] = (dist >> 8) & 0xff;
            BAsmCode[4] = (dist >> 16) & 0xff;
          }
          else WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
        }
      }
    }
  }
}

static void DecodeRST(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Byte vector;
    int SaveRadixBase = RadixBase;

#if 0
    /* some people like to regard the RST argument as a literal
       and leave away the 'h' to mark 38 as a hex number... */
    RadixBase = 16;
#endif

    vector = EvalStrIntExpressionWithFlags(&ArgStr[1], Int8, &OK, &Flags);
    RadixBase = SaveRadixBase;

    if (mFirstPassUnknown(Flags))
      vector &= 0x38;
    if (OK)
    {
      if ((vector > 0x38) || (vector & 7)) WrStrErrorPos(ErrNum_NotFromThisAddress, &ArgStr[1]);
      else
      {
        append_opcode(0xc7 + vector);
        set_b_guessed(Flags, CodeLen - 1, 1, 0x38);
      }
    }
  }
}

static void DecodeEI_DI(Word Code)
{
  if (is_z280() && !SupAllowed)
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);

  switch (ArgCnt)
  {
    case 0:
      append_opcode(0xf3 + Code);
      break;
    case 1:
      if (chk_core_mask(e_core_mask_z380 | e_core_mask_z280))
      {
        Boolean OK;
        tSymbolFlags Flags;

        if (is_z380())
        {
          append_prefix(0xdd);
          append_opcode(0xf3 + Code);
          BAsmCode[CodeLen] = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt8, &OK, &Flags);
        }
        else
        {
          append_prefix(0xed);
          append_opcode(0x77 | Code);
          BAsmCode[CodeLen] = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt7, &OK, &Flags);
        }
        if (OK)
        {
          set_b_guessed(Flags, CodeLen, 1, 0xff);
          CodeLen++;
        }
        else
          CodeLen = 0;
      }
      break;
    default:
      (void)ChkArgCnt(0, 1);
  }
}

static void DecodeIM(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && chk_core_mask(e_core_mask_no_sharp))
  {
    Byte mode;
    Boolean OK;
    tSymbolFlags Flags;

    mode = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt2, &OK, &Flags);
    if (OK)
    {
      if (mode > 3) WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
      else if ((mode == 3) && (!chk_core_mask(e_core_mask_z280 | e_core_mask_z380)));
      else
      {
        if (is_z280() && !SupAllowed)
          WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
        if (mode == 3)
          mode = 1;
        else if (mode >= 1)
          mode++;
        CodeLen = 2;
        BAsmCode[0] = 0xed;
        BAsmCode[1] = 0x46 + (mode << 3);
        set_b_guessed(Flags, 1, 1, 0x18);
      }
    }
  }
}

static void code_ldctl_z380(Word Code)
{
  Byte sfr;
  adr_vals_t adr_vals;

  UNUSED(Code);

  OpSize = eSymbolSize8Bit;
  if (!ChkArgCnt(2, 2));
  else if (!chk_core_mask(e_core_mask_z380));
  else if (DecodeSFR(ArgStr[1].str.p_str, &sfr))
  {
    switch (DecodeAdr(&ArgStr[2], &adr_vals, MModAll))
    {
      case ModReg8:
        if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          BAsmCode[0] = 0xcd + ((sfr & 3) << 4);
          BAsmCode[1] = 0xc8 + ((sfr & 4) << 2);
          CodeLen = 2;
        }
        break;
      case ModReg16:
        if ((sfr != 1) || (adr_vals.part != HLReg) || (PrefixCnt != 0)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0xc8;
          CodeLen = 2;
        }
        break;
      case ModImm:
        BAsmCode[0] = 0xcd + ((sfr & 3) << 4);
        BAsmCode[1] = 0xca + ((sfr & 4) << 2);
        BAsmCode[2] = adr_vals.values[0];
        CodeLen = 3;
        break;
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
    }
  }
  else if (DecodeSFR(ArgStr[2].str.p_str, &sfr))
  {
    switch (DecodeAdr(&ArgStr[1], &adr_vals, MModAll))
    {
      case ModReg8:
        if ((adr_vals.part != AccReg) || (sfr == 1)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          BAsmCode[0] = 0xcd + ((sfr & 3) << 4);
          BAsmCode[1] = 0xd0;
          CodeLen = 2;
        }
        break;
      case ModReg16:
        if ((sfr != 1) || (adr_vals.part != HLReg) || (PrefixCnt != 0)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          BAsmCode[0] = 0xed;
          BAsmCode[1] = 0xc0;
          CodeLen = 2;
        }
        break;
      case ModNone:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
}

static void DecodeRESC_SETC(Word Code)
{
  if (ChkArgCnt(1, 1)
   && chk_core_mask(e_core_mask_z380))
  {
    Byte creg = 0xff;

    NLS_UpString(ArgStr[1].str.p_str);
    if (!strcmp(ArgStr[1].str.p_str, "LW")) creg = 1;
    else if (!strcmp(ArgStr[1].str.p_str, "LCK")) creg = 2;
    else if (!strcmp(ArgStr[1].str.p_str, "XM")) creg = 3;
    else WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
    if (creg != 0xff)
    {
      CodeLen = 2;
      BAsmCode[0] = 0xcd + (creg << 4);
      BAsmCode[1] = 0xf7 + Code;
    }
  }
}

static void DecodeDDIR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2)
   && chk_core_mask(e_core_mask_z380))
  {
    Boolean OK;
    ddir_prefix_pair_t pair;
    tStrComp *p_arg;

    OK = True;
    prefix_pair_clear(&pair);
    forallargs (p_arg, OK)
    {
      tOpPrefix this_prefix = DecodePrefix(p_arg->str.p_str);
      if (ePrefixNone == this_prefix)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return;
      }
      prefix_pair_update(&pair, this_prefix);
    }
    if (OK)
    {
      curr_prefix_pair = pair;
      if (prefix_pair_has_overrides(&curr_prefix_pair))
      {
        GetPrefixCode(BAsmCode + 0, &curr_prefix_pair);
        CodeLen = 2;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeLEA(Word Code)
 * \brief  handle LEA instruction
 * ------------------------------------------------------------------------ */

static void CodeLEA(Word Code)
{
  Byte dest_reg, ini_prefix_cnt = PrefixCnt,
       dest_prefix, src_prefix;
  adr_vals_t adr_vals;

  UNUSED(Code);

  if (!is_ez80())
  {
    char Str[100];
    as_snprintf(Str, sizeof(Str), "%seZ80%s", getmessage(Num_ErrMsgOnlyCPUSupported1), getmessage(Num_ErrMsgOnlyCPUSupported2));
    WrXError(ErrNum_InstructionNotSupported, Str);
    return;
  }
  if (!ChkArgCnt(2, 2))
    return;

  if (DecodeAdr(&ArgStr[1], &adr_vals, MModReg16) != ModReg16)
    return;
  if (adr_vals.part == SPReg)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    return;
  }
  dest_reg = adr_vals.part;
  dest_prefix = (PrefixCnt > ini_prefix_cnt) ? remove_prefix(0) : 0x00;
  if (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8 | MModImmIsAbs) != ModReg8)
    return;
  if ((adr_vals.part != MReg) || (PrefixCnt == ini_prefix_cnt))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
    return;
  }
  src_prefix = remove_prefix(0);
  append_prefix(0xed);
  if ((dest_prefix == IXPrefix) && (src_prefix == IXPrefix))
    append_opcode(0x32);
  else if ((dest_prefix == IYPrefix) && (src_prefix == IXPrefix))
    append_opcode(0x55);
  else if ((dest_prefix == IXPrefix) && (src_prefix == IYPrefix))
    append_opcode(0x54);
  else if ((dest_prefix == IYPrefix) && (src_prefix == IYPrefix))
    append_opcode(0x33);
  else if (src_prefix == IXPrefix)
    append_opcode(0x02 | (dest_reg << 4));
  else
    append_opcode(0x03 | (dest_reg << 4));
  append_adr_vals(&adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     code_lda(Word code)
 * \brief  handle LDA instruction (Z280)
 * ------------------------------------------------------------------------ */

static void code_lda(Word code)
{
  adr_vals_t adr_vals;
  Byte ini_prefix_cnt;
  prefix_store_t dest_prefix;
  UNUSED(code);

  if (!ChkArgCnt(2, 2) || !chk_core_mask(e_core_mask_z280))
    return;

  ini_prefix_cnt = PrefixCnt;
  if (DecodeAdr(&ArgStr[1], &adr_vals, MModReg16) != ModReg16)
    return;
  if (adr_vals.part != HLReg)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    return;
  }
  store_prefix(&dest_prefix, ini_prefix_cnt);

  switch (decode_adr_rel(&ArgStr[2], &adr_vals, MModAbs | MModX | MModReg8 | MModSR | MModRA | MModBX, 4 + PrefixCnt))
  {
    case ModReg8:
    {
      prefix_store_t src_prefix;

      store_prefix(&src_prefix, dest_prefix.cnt);
      if ((adr_vals.part != MReg) || !src_prefix.present) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      else
      {
        convert_sx_to_x(&adr_vals, BAsmCode[--PrefixCnt]);
        goto is_modx;
      }
      break;
    }
    case ModAbs:
      append_opcode(0x21);
      append_adr_vals(&adr_vals);
      break;
    case ModX:
    is_modx:
    case ModRA:
    case ModSR:
    case ModBX:
      append_z280(&adr_vals, False, 0xed02, 3);
      break;
          
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     code_ldctl_z280(Word code)
 * \brief  handle LDCTL (Z280 version)
 * ------------------------------------------------------------------------ */

static void code_ldctl_z280(Word code)
{
  adr_vals_t adr_vals;

  UNUSED(code);

  if (!ChkArgCnt(2, 2)
   || !chk_core_mask(e_core_mask_z280 | e_core_mask_z280_privileged))
    return;

  switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg16 | MModIndReg8 | MModUSP))
  {
    case ModReg16:
      if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else switch (DecodeAdr(&ArgStr[2], &adr_vals, MModIndReg8 | MModUSP))
      {
        case ModIndReg8:
          append_opcode16(0xed66);
          break;
        case ModUSP:
          append_opcode16(0xed87);
          break;
        default:
          break;
      }
      break;
    case ModIndReg8:
      switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg16))
      {
        case ModReg16:
          if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          else
            append_opcode16(0xed6e);
          break;
      }
      break;
    case ModUSP:
      switch (DecodeAdr(&ArgStr[2], &adr_vals, MModReg16))
      {
        case ModReg16:
          if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          else
            append_opcode16(0xed8f);
          break;
      }
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     code_ldud_ldup(Word code)
 * \brief  handle LDUD/LDUP
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void code_ldud_ldup(Word code)
{
  adr_vals_t dest_adr_vals, src_adr_vals;

  if (!ChkArgCnt(2, 2)
   || !chk_core_mask(e_core_mask_z280 | e_core_mask_z280_privileged))
    return;

  if (DecodeAdr(&ArgStr[1], &dest_adr_vals, MModReg8) != ModReg8)
    return;
  switch (dest_adr_vals.part)
  {
    case AccReg:
      if (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg8) != ModReg8)
        return;
      if (src_adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      else
      {
        append_opcode16(code);
        append_adr_vals(&src_adr_vals);
      }
      break;
    case MReg:
      if (DecodeAdr(&ArgStr[2], &src_adr_vals, MModReg8) != ModReg8)
        return;
      if (src_adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      else
      {
        append_opcode16(code | 0x0008);
        append_adr_vals(&dest_adr_vals);
      }
      break;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     code_sc(Word code)
 * \brief  handle SC instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void code_sc(Word code)
{
  Word value;
  tEvalResult eval_result;

  if (!ChkArgCnt(1, 1)
   || !chk_core_mask(e_core_mask_z280))
    return;
  value = EvalStrIntExpressionWithResult(&ArgStr[1], Int16, &eval_result);
  if (eval_result.OK)
  {
    append_opcode16(code);
    set_b_guessed(eval_result.Flags, CodeLen, 2, 0xff);
    BAsmCode[CodeLen++] = Lo(value);
    BAsmCode[CodeLen++] = Hi(value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     code_test(Word code)
 * \brief  handle TSET instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void code_tset(Word code)
{
  adr_vals_t adr_vals;

  if (!ChkArgCnt(1, 1)
   || !chk_core_mask(e_core_mask_z280))
    return;

  switch (DecodeAdr(&ArgStr[1], &adr_vals, MModReg8))
  {
    case ModReg8:
      append_opcode(Hi(code));
      append_adr_vals(&adr_vals);
      BAsmCode[CodeLen++] = Lo(code) | adr_vals.part;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     code_tsti(Word code)
 * \brief  handle TSTI instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void code_tsti(Word code)
{
  adr_vals_t adr_vals;

  if (!chk_core_mask(e_core_mask_z80u | e_core_mask_z280))
    return;

  switch(ArgCnt)
  {
    case 0:
      reset_adr_vals(&adr_vals);
      adr_vals.mode = ModIndReg8;
      adr_vals.part = CReg;
      break;
    case 1:
      DecodeAdr(&ArgStr[1], &adr_vals, MModIndReg8);
      break;
    default:
      (void)ChkArgCnt(1, 1);
      return;
  }

  switch (adr_vals.mode)
  {
    case ModIndReg8:
      if (adr_vals.part != CReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
        append_opcode16(code);
      break;
    default:
      break;
  }
}           

/*!------------------------------------------------------------------------
 * \fn     decode_neg(Word is_word)
 * \brief  handle NEG/NEGW instruction
 * \param  is_word 1 for NEGW
 * ------------------------------------------------------------------------ */

static void decode_neg(Word is_word)
{
  adr_vals_t adr_vals;

  if (!chk_core_mask(is_word ? (e_core_mask_z380 | e_core_mask_z280) : e_core_mask_no_sharp))
    return;
  switch (ArgCnt)
  {
    case 0:
      reset_adr_vals(&adr_vals);
      adr_vals.mode = is_word ? ModReg16 : ModReg8;
      adr_vals.part = is_word ? HLReg : AccReg;
      break;
    case 1:
      DecodeAdr(&ArgStr[1], &adr_vals, ((is_z380() || is_z280()) ? MModReg16 : 0) | (is_word ? 0 : MModReg8)); 
      break;
    default:
      (void)ChkArgCnt(0, 1);
      break;
  }

  switch (adr_vals.mode)
  {
    case ModReg8:
      if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
        append_opcode16(0xed44);
      break;
    case ModReg16:
      if (adr_vals.part != HLReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
        append_opcode16(is_z380() ? 0xed54 : 0xed4c);
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodePEA(Word Code)
 * \brief  handle PEA instruction
 * ------------------------------------------------------------------------ */

static void CodePEA(Word Code)
{
  Byte ini_prefix_cnt = PrefixCnt,
       src_prefix;
  adr_vals_t adr_vals;

  UNUSED(Code);

  if (!is_ez80())
  {
    char Str[100];
    as_snprintf(Str, sizeof(Str), "%seZ80%s", getmessage(Num_ErrMsgOnlyCPUSupported1), getmessage(Num_ErrMsgOnlyCPUSupported2));
    WrXError(ErrNum_InstructionNotSupported, Str);
    return;
  }
  if (!ChkArgCnt(1, 1))
    return;

  if (DecodeAdr(&ArgStr[1], &adr_vals, MModReg8 | MModImmIsAbs) != ModReg8)
    return;
  if ((adr_vals.part != MReg) || (PrefixCnt == ini_prefix_cnt))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    return;
  }
  src_prefix = remove_prefix(0);
  append_prefix(0xed);
  append_opcode((src_prefix == IXPrefix) ? 0x65 : 0x66);
  append_adr_vals(&adr_vals);
}

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  code_equate_type(SegIO, PortIntType());
}

static void DecodeLDI_LDD(Word Code)
{
  if (ChkArgCnt(2,2)
   && chk_core_mask(e_core_mask_sharp))
  {
    adr_vals_t adr_vals;

    if (DecodeAdr(&ArgStr[1], &adr_vals, MModReg8) == ModReg8)
      switch (adr_vals.part)
      {
        case AccReg:
          if (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8) == ModReg8)
          {
            if (adr_vals.part != MReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[0] = Code | 0x08;
              CodeLen = 1;
            }
          }
          break;
        case MReg:
          if (DecodeAdr(&ArgStr[2], &adr_vals, MModReg8) == ModReg8)
          {
            if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[0] = Code;
              CodeLen = 1;
            }
          }
          break;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_z280_exts(Word code)
 * \brief  Handle Z280 EXTS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_z280_exts(Word code)
{
  ShortInt adr_mode;
  adr_vals_t adr_vals;

  switch (ArgCnt)
  {
    case 0:
      reset_adr_vals(&adr_vals);
      adr_vals.part = AccReg;
      adr_mode = ModReg8;
      break;
    case 1:
      adr_mode = DecodeAdr(&ArgStr[1], &adr_vals, MModReg8 | MModReg16);
      break;
    default:
      (void)ChkArgCnt(0, 1);
      return;
  }

  switch (adr_mode)
  {
    case ModReg8:
      if (adr_vals.part != AccReg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      else
      {
        append_prefix(Hi(code));
        append_opcode(Lo(code));
      }
      break;
    case ModReg16:
      if ((adr_vals.part != HLReg) || (PrefixCnt > 0)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      else
      {
        append_prefix(Hi(code));
        append_opcode(Lo(code) | 8);
      }
      break;
    default:
      break;
  }
}

static void DecodePRWINS(Word Code)
{
  UNUSED(Code);

  if (chk_core_mask(e_core_mask_z180 | e_core_mask_z280))
  {
    Boolean code_data_separate = is_z280() && p_z280_data && p_z280_data->program_data_separation;

    if (is_z180())
      printf("\nCBAR 0%02xh BBR 0%02xh CBR 0%02xh\n",
             (unsigned)Reg_CBAR, (unsigned)Reg_BBR, (unsigned)Reg_CBR);

    if (code_data_separate)
      printf("CODE:\n");
    cpu_2_phys_area_dump(SegCode, stdout);
    if (code_data_separate)
    {
      printf("DATA:\n");
      cpu_2_phys_area_dump(SegData, stdout);
    }
  }
}

/*==========================================================================*/
/* Codetabellenerzeugung */

static void AddFixed(const char *NewName, cpu_core_mask_t core_mask, Word NewCode)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].core_mask = core_mask;
  FixedOrders[InstrZ].Code = NewCode;
  AddInstTable(InstTable, NewName, InstrZ++, DecodeFixed);
}

static void AddAcc(const char *NewName, cpu_core_mask_t core_mask, Word NewCode)
{
  order_array_rsv_end(AccOrders, BaseOrder);
  AccOrders[InstrZ].core_mask = core_mask;
  AccOrders[InstrZ].Code = NewCode;
  AddInstTable(InstTable, NewName, InstrZ++, DecodeAcc);
}

static void AddHL(const char *NewName, cpu_core_mask_t core_mask, Word NewCode)
{
  order_array_rsv_end(HLOrders, BaseOrder);
  HLOrders[InstrZ].core_mask = core_mask;
  HLOrders[InstrZ].Code = NewCode;
  AddInstTable(InstTable, NewName, InstrZ++, DecodeHL);
}

static void AddALU(const char *Name8, const char *Name16, Byte Code)
{
  AddInstTable(InstTable, Name8 , Code, DecodeALU8);
  AddInstTable(InstTable, Name16, Code, DecodeALU16);
}

static void AddShift(const char *Name8, const char *Name16, Byte Code)
{
  AddInstTable(InstTable, Name8 , Code, DecodeShift8);
  if (Name16)
    AddInstTable(InstTable, Name16, Code, DecodeShift16);
}

static void AddBit(const char *NName, Word Code)
{
  AddInstTable(InstTable, NName, Code, DecodeBit);
}

static void AddCondition(const char *NewName, Byte NewCode)
{
  order_array_rsv_end(Conditions, Condition);
  Conditions[InstrZ].Name = NewName;
  Conditions[InstrZ++].Code = NewCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);

  add_null_pseudo(InstTable);

  AddInstTable(InstTable, "LD" , 0, DecodeLD);
  AddInstTable(InstTable, "LDW", 1, DecodeLD);
  AddInstTable(InstTable, "LDHL", 0, DecodeLDHL);
  AddInstTable(InstTable, "LDH", 0, DecodeLDH);
  AddInstTable(InstTable, "LDX", 0, DecodeLDX);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "ADDW", 0, DecodeADDW);
  AddInstTable(InstTable, "ADC" , 0, DecodeADC_SBC);
  AddInstTable(InstTable, "SBC" , 1, DecodeADC_SBC);
  AddInstTable(InstTable, "ADCW", 0, DecodeADCW_SBCW);
  AddInstTable(InstTable, "SBCW",16, DecodeADCW_SBCW);
  AddInstTable(InstTable, "INC" , 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC" , 1, DecodeINC_DEC);
  AddInstTable(InstTable, "INCW", 2, DecodeINC_DEC);
  AddInstTable(InstTable, "DECW", 3, DecodeINC_DEC);
  AddInstTable(InstTable, "MLT" , 0, DecodeMLT);
  if (is_z380())
  {
    AddInstTable(InstTable, "DIVUW" , 0x28, DecodeMULT_DIV_Z380);
    AddInstTable(InstTable, "MULTW" , 0x00, DecodeMULT_DIV_Z380);
    AddInstTable(InstTable, "MULTUW", 0x08, DecodeMULT_DIV_Z380);
  }
  else if (is_z280())
  {
    AddInstTable(InstTable, "DIV", 0x04, DecodeMULT_DIV_16_Z280);
    AddInstTable(InstTable, "DIVU", 0x05, DecodeMULT_DIV_16_Z280);
    AddInstTable(InstTable, "MULT", 0x00, DecodeMULT_DIV_16_Z280);
    AddInstTable(InstTable, "MULTU", 0x01, DecodeMULT_DIV_16_Z280);
    AddInstTable(InstTable, "DIVW", 0x0a, DecodeMULT_DIV_32_Z280);
    AddInstTable(InstTable, "DIVUW", 0x0b, DecodeMULT_DIV_32_Z280);
    AddInstTable(InstTable, "MULTW", 0x02, DecodeMULT_DIV_32_Z280);
    AddInstTable(InstTable, "MULTUW", 0x03, DecodeMULT_DIV_32_Z280);
  }
  AddInstTable(InstTable, "TST", 0, DecodeTST);
  AddInstTable(InstTable, "SWAP", 0, DecodeSWAP);
  AddInstTable(InstTable, "PUSH", 4, DecodePUSH_POP);
  AddInstTable(InstTable, "POP" , 0, DecodePUSH_POP);
  AddInstTable(InstTable, "EX"  , 0, DecodeEX);
  AddInstTable(InstTable, "TSTI", 0xed70, code_tsti);
  AddInstTable(InstTable, "IN"  , 0, DecodeIN_OUT);
  AddInstTable(InstTable, "OUT" , 1, DecodeIN_OUT);
  AddInstTable(InstTable, "INW"  , 0, DecodeINW_OUTW);
  AddInstTable(InstTable, "OUTW" , 1, DecodeINW_OUTW);
  AddInstTable(InstTable, "IN0"  , 0, DecodeIN0_OUT0);
  AddInstTable(InstTable, "OUT0" , 1, DecodeIN0_OUT0);
  AddInstTable(InstTable, "INA"  , 8, DecodeINA_INAW_OUTA_OUTAW);
  AddInstTable(InstTable, "INAW" , 9, DecodeINA_INAW_OUTA_OUTAW);
  AddInstTable(InstTable, "OUTA" , 0, DecodeINA_INAW_OUTA_OUTAW);
  AddInstTable(InstTable, "OUTAW", 1, DecodeINA_INAW_OUTA_OUTAW);
  AddInstTable(InstTable, "TSTIO", 0, DecodeTSTIO);
  AddInstTable(InstTable, "RET" , 0, DecodeRET);
  AddInstTable(InstTable, "JP" , 0, DecodeJP);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "JR" , 0, DecodeJR);
  AddInstTable(InstTable, "J" , 0, DecodeJ);
  AddInstTable(InstTable, "CALR", 0, DecodeCALR);
  AddInstTable(InstTable, "DJNZ", 0, DecodeDJNZ);
  AddInstTable(InstTable, "RST", 0, DecodeRST);
  AddInstTable(InstTable, "DI", 0, DecodeEI_DI);
  AddInstTable(InstTable, "EI", 8, DecodeEI_DI);
  AddInstTable(InstTable, "IM", 0, DecodeIM);
  AddInstTable(InstTable, "LDCTL", 0, is_z280() ? code_ldctl_z280 : code_ldctl_z380);
  AddInstTable(InstTable, "RESC", 8, DecodeRESC_SETC);
  AddInstTable(InstTable, "SETC", 0, DecodeRESC_SETC);
  AddInstTable(InstTable, "DDIR", 0, DecodeDDIR);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);
  AddInstTable(InstTable, "DEFB", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDB);
  AddInstTable(InstTable, "DEFW", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_AllowFloat, DecodeIntelDW);

  InstrZ = 0;
  AddCondition("NZ", 0); AddCondition("Z" , 1);
  AddCondition("NC", 2); AddCondition("C" , 3);
  if (!is_sharp())
  {
    AddCondition("PO", 4); AddCondition("NV", 4);
    AddCondition("PE", 5); AddCondition("V" , 5);
    AddCondition("P" , 6); AddCondition("NS", 6);
    AddCondition("M" , 7); AddCondition("S" , 7);
  }
  AddCondition(NULL, 0);

  InstrZ = 0;
  AddFixed("EXX"   , e_core_mask_no_sharp  , 0x00d9);
  if (is_sharp())
  {
    AddInstTable(InstTable, "LDI", 0x22, DecodeLDI_LDD);
    AddInstTable(InstTable, "LDD", 0x32, DecodeLDI_LDD);
  }
  else
  {
    AddFixed("LDI"  , e_core_mask_no_sharp  , 0xeda0);
    AddFixed("LDD"  , e_core_mask_no_sharp  , 0xeda8);
  }
  AddFixed("LDIR"  , e_core_mask_no_sharp  , 0xedb0);
  AddFixed("LDDR"  , e_core_mask_no_sharp  , 0xedb8);
  AddFixed("CPI"   , e_core_mask_no_sharp  , 0xeda1);
  AddFixed("CPIR"  , e_core_mask_no_sharp  , 0xedb1);
  AddFixed("CPD"   , e_core_mask_no_sharp  , 0xeda9);
  AddFixed("CPDR"  , e_core_mask_no_sharp  , 0xedb9);
  AddFixed("RLCA"  , e_core_mask_all       , 0x0007);
  AddFixed("RRCA"  , e_core_mask_all       , 0x000f);
  AddFixed("RLA"   , e_core_mask_all       , 0x0017);
  AddFixed("RRA"   , e_core_mask_all       , 0x001f);
  AddFixed("RLD"   , e_core_mask_no_sharp  , 0xed6f);
  AddFixed("RRD"   , e_core_mask_no_sharp  , 0xed67);
  AddFixed("DAA"   , e_core_mask_all       , 0x0027);
  AddFixed("CCF"   , e_core_mask_all       , 0x003f);
  AddFixed("SCF"   , e_core_mask_all       , 0x0037);
  AddFixed("NOP"   , e_core_mask_all       , 0x0000);
  AddFixed("HALT"  , e_core_mask_all | e_core_mask_z280_privileged, 0x0076);
  /* TODO: allow only no and .L as eZ80 attribute */
  AddFixed("RETI"  , e_core_mask_all  | e_core_mask_z280_privileged, is_sharp() ? 0x00d9 : 0xed4d);
  /* TODO: allow only no and .L as eZ80 attribute */
  AddFixed("RETN"  , e_core_mask_no_sharp | e_core_mask_z280_privileged , 0xed45);
  AddFixed("INI"   , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xeda2);
  AddFixed("INIR"  , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xedb2);
  AddFixed("IND"   , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xedaa);
  AddFixed("INDR"  , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xedba);
  AddFixed("IND2"  , e_core_mask_ez80      , 0xed8c);
  AddFixed("IND2R" , e_core_mask_ez80      , 0xed9c);
  AddFixed("INDM"  , e_core_mask_ez80      , 0xed8a);
  AddFixed("INDMR" , e_core_mask_ez80      , 0xed9a);
  AddFixed("INI2"  , e_core_mask_ez80      , 0xed84);
  AddFixed("INI2R" , e_core_mask_ez80      , 0xed94);
  AddFixed("INIM"  , e_core_mask_ez80      , 0xed82);
  AddFixed("INIMR" , e_core_mask_ez80      , 0xed92);
  AddFixed("OTD2R" , e_core_mask_ez80      , 0xedbc);
  AddFixed("OTI2R" , e_core_mask_ez80      , 0xedb4);
  AddFixed("OUTD2" , e_core_mask_ez80      , 0xedac);
  AddFixed("OUTI2" , e_core_mask_ez80      , 0xeda4);
  AddFixed("RSMIX" , e_core_mask_ez80      , 0xed7e);
  AddFixed("STMIX" , e_core_mask_ez80      , 0xed7d);
  AddFixed("OUTI"  , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xeda3);
  AddFixed("OTIR"  , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xedb3);
  AddFixed("OUTD"  , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xedab);
  AddFixed("OTDR"  , e_core_mask_no_sharp | e_core_mask_z280_io_privileged , 0xedbb);
  AddFixed("EXA"   , e_core_mask_no_sharp  , 0x0008);
  AddFixed("EXD"   , e_core_mask_no_sharp  , 0x00eb);
  AddFixed("SLP"   , e_core_mask_min_z180  , 0xed76);
  AddFixed("OTIM"  , e_core_mask_min_z180  , 0xed83);
  AddFixed("OTIMR" , e_core_mask_min_z180  , 0xed93);
  AddFixed("OTDM"  , e_core_mask_min_z180  , 0xed8b);
  AddFixed("OTDMR" , e_core_mask_min_z180  , 0xed9b);
  AddFixed("BTEST" , e_core_mask_z380      , 0xedcf);
  AddFixed("EXALL" , e_core_mask_z380      , 0xedd9);
  AddFixed("EXXX"  , e_core_mask_z380      , 0xddd9);
  AddFixed("EXXY"  , e_core_mask_z380      , 0xfdd9);
  AddFixed("INDW"  , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xedea : 0xed8a);
  AddFixed("INDRW" , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xedfa : 0xed9a);
  AddFixed("INIW"  , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xede2 : 0xed82);
  AddFixed("INIRW" , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xedf2 : 0xed92);
  AddFixed("LDDW"  , e_core_mask_z380      , 0xede8);
  AddFixed("LDDRW" , e_core_mask_z380      , 0xedf8);
  AddFixed("LDIW"  , e_core_mask_z380      , 0xede0);
  AddFixed("LDIRW" , e_core_mask_z380      , 0xedf0);
  AddFixed("MTEST" , e_core_mask_z380      , 0xddcf);
  AddFixed("OTDRW" , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xedfb : 0xed9b);
  AddFixed("OTIRW" , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xedf3 : 0xed93);
  AddFixed("OUTDW" , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xedeb : 0xed8b);
  AddFixed("OUTIW" , e_core_mask_z380 | e_core_mask_z280 | e_core_mask_z280_io_privileged, is_z380() ? 0xede3 : 0xed83);
  AddFixed("RETB"  , e_core_mask_z380      , 0xed55);
  AddFixed("STOP"  , e_core_mask_sharp     , 0x0010);
  AddFixed("PCACHE", e_core_mask_z280      , 0xed65);
  AddFixed("RETIL" , e_core_mask_z280 | e_core_mask_z280_privileged, 0xed55);

  AddInstTable(InstTable, "INDRX", 0xedca, decode_ez80_xio);
  AddInstTable(InstTable, "INIRX", 0xedc2, decode_ez80_xio);
  AddInstTable(InstTable, "OTDRX", 0xedcb, decode_ez80_xio);
  AddInstTable(InstTable, "OTIRX", 0xedc3, decode_ez80_xio);

  InstrZ = 0;
  AddAcc("CPL"  , e_core_mask_all , 0x002f);

  if (is_z280())
    AddInstTable(InstTable, "EXTS", 0xed64, decode_z280_exts);
  else
    AddAcc("EXTS" , e_core_mask_z380  , 0xed65);

  AddInstTable(InstTable, "LDUD", 0xed86, code_ldud_ldup);
  AddInstTable(InstTable, "LDUP", 0xed96, code_ldud_ldup);
  AddInstTable(InstTable, "SC", 0xed71, code_sc);
  AddInstTable(InstTable, "TSET", 0xcb30, code_tset);

  AddInstTable(InstTable, "NEG", 0, decode_neg);
  AddInstTable(InstTable, "NEGW", 1, decode_neg);

  InstrZ = 0;
  AddHL("CPLW" , e_core_mask_z380, 0xdd2f);
  AddHL("EXTSW", e_core_mask_z380, 0xed75);

  AddALU("SUB", "SUBW", 2); AddALU("AND", "ANDW", 4);
  AddALU("OR" , "ORW" , 6); AddALU("XOR", "XORW", 5);
  AddALU("CP" , "CPW" , 7);

  AddShift("RLC" , "RLCW" , 0); AddShift("RRC", "RRCW", 1);
  AddShift("RL"  , "RLW"  , 2); AddShift("RR" , "RRW" , 3);
  AddShift("SLA" , "SLAW" , 4); AddShift("SRA", "SRAW", 5);
  AddShift("SLIA", NULL   , 6); AddShift("SRL", "SRLW", 7);
  AddShift("SLS" , NULL   , 6); AddShift("SLI", NULL  , 6);
  AddShift("SL1" , NULL   , 6);

  AddBit("BIT", 0); AddBit("RES", 1); AddBit("SET", 2);

  AddInstTable(InstTable, "LEA", 0, CodeLEA);
  AddInstTable(InstTable, "LDA", 0, code_lda);
  AddInstTable(InstTable, "PEA", 0, CodePEA);

  AddInstTable(InstTable, "JAF", 0xdd28, decode_jaf_jar);
  AddInstTable(InstTable, "JAR", 0xdd20, decode_jaf_jar);

  AddInstTable(InstTable, "REG" , 0, CodeREG);

  AddInstTable(InstTable, "PRWINS", 0, DecodePRWINS);

  AddIntelPseudo(InstTable, eIntPseudoFlag_LittleEndian);
}

static void DeinitFields(void)
{
  order_array_free(Conditions);
  order_array_free(FixedOrders);
  order_array_free(AccOrders);
  order_array_free(HLOrders);

  DestroyInstTable(InstTable);
}

/*!------------------------------------------------------------------------
 * \fn     create_z280_assumes(void)
 * \brief  dynamically create list of Z280 assume registers
 * ------------------------------------------------------------------------ */

static void create_z280_assumes(void)
{
  if (!p_z280_data->p_assumes)
  {
    char name[20];
    size_t z;

    assume_append(&p_z280_data->p_assumes, &p_z280_data->assumes_cnt,
                  "I", &p_z280_data->reg_i, 0, 1, 0, NULL);
    assume_append(&p_z280_data->p_assumes, &p_z280_data->assumes_cnt,
                  "MMUMCR", &p_z280_data->reg_mmumcr, 0, 0xffffu, 0, update_z280_areas);
    for (z = 0; z < Z280_NUM_PDR; z++)
    {
      as_snprintf(name, sizeof(name), "UPD%u", (unsigned)z);
      assume_append(&p_z280_data->p_assumes, &p_z280_data->assumes_cnt,
                    name, &p_z280_data->reg_upd[z], 0, 0xffff, 0, update_z280_areas);
      as_snprintf(name, sizeof(name), "SPD%u", (unsigned)z);
      assume_append(&p_z280_data->p_assumes, &p_z280_data->assumes_cnt,
                    name, &p_z280_data->reg_spd[z], 0, 0xffff, 0, update_z280_areas);
    }
  }
}

/*=========================================================================*/

static void StripPref(const char *Arg, Byte Opcode)
{
  char *ptr, *ptr2;

  /* do we have a prefix ? */

  if (!strcmp(OpPart.str.p_str, Arg))
  {
    /* add to code */

    BAsmCode[PrefixCnt++] = Opcode;
    StrCompReset(&OpPart);

    /* cut true opcode out of next argument */

    if (ArgCnt)
    {
      /* look for end of string */

      for (ptr = ArgStr[1].str.p_str; *ptr; ptr++)
        if (as_isspace(*ptr))
          break;

      /* look for beginning of next string */

      for (ptr2 = ptr; *ptr2; ptr2++)
        if (!as_isspace(*ptr2))
          break;

      /* copy out new opcode */

      OpPart.Pos.StartCol = ArgStr[1].Pos.StartCol;
      OpPart.Pos.Len = strmemcpy(OpPart.str.p_str, STRINGSIZE, ArgStr[1].str.p_str, ptr - ArgStr[1].str.p_str);
      NLS_UpString(OpPart.str.p_str);

      /* cut down arg or eliminate it completely */

      if (*ptr2)
      {
        strmov(ArgStr[1].str.p_str, ptr2);
        ArgStr[1].Pos.StartCol += ptr2 - ArgStr[1].str.p_str;
        ArgStr[1].Pos.Len -= ptr2 - ArgStr[1].str.p_str;
      }
      else
      {
        int z;

        for (z = 1; z < ArgCnt; z++)
          StrCompCopy(&ArgStr[z], &ArgStr[z + 1]);
        ArgCnt--;
      }
    }

    /* if no further argument, that's all folks */

    else
      CodeLen = PrefixCnt;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_attrpart_ez80(void)
 * \brief  decode eZ80-style attribute part for ADL mode overrides
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_attrpart_ez80(void)
{
  int attr_index = 0;
  const char *p_attr;

  AttrPartOpSize[0] =
  AttrPartOpSize[1] = eSymbolSizeUnknown;
  for (p_attr = AttrPart.str.p_str; *p_attr; p_attr++)
    switch (as_toupper(*p_attr))
    {
      case 'S':
      case 'L':
        if (AttrPartOpSize[attr_index] != eSymbolSizeUnknown)
          return False;
        AttrPartOpSize[attr_index] = (as_toupper(*p_attr) == 'L') ? eSymbolSize24Bit : eSymbolSize16Bit;
        break;
      case 'I':
        attr_index = 1;
        break;
      default:
        return False;
    }
  return True;
}

static void MakeCode_Z80(void)
{
  PrefixCnt = 0;
  OpSize = eSymbolSizeUnknown;
  MayLW = False;

  switch (p_curr_cpu_props->core)
  {
    case e_core_r2000:
      /* Rabbit 2000 prefixes */
      StripPref("ALTD", 0x76);
      if (!*OpPart.str.p_str) return;
      break;
    case e_core_z380:
      /* Z380: letzten Praefix umkopieren */
      last_prefix_pair = curr_prefix_pair;
      prefix_pair_clear(&curr_prefix_pair);
      break;
    case e_core_ez80:
    {
      int z, pref_index = 0;
      Boolean need_prefix = False;
      tSymbolSize def_size = Reg_ADL ? eSymbolSize24Bit : eSymbolSize16Bit;

      /* eZ80: Suffix Completion */
      for (z = 0; z < 2; z++)
      {
        if (AttrPartOpSize[z] == eSymbolSizeUnknown)
          AttrPartOpSize[z] = def_size;
        else
          need_prefix = True;
        pref_index |= (AttrPartOpSize[z] == eSymbolSize24Bit) << z;
      }
      if (need_prefix)
        BAsmCode[PrefixCnt++] = 0x40 | (pref_index << 3) | pref_index;
      break;
    }
    default:
      break;
  }

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_Z80(void)
{
  Reg_CBAR = 0xf0;
  Reg_CBR = Reg_BBR = 0x00;
  Reg_ADL = 0;
}

static Boolean IsDef_Z80(void)
{
  return Memo("PORT") || Memo("REG");
}

/* Treat special case of AF' which is no quoting: */

static Boolean QualifyQuote_Z80(const char *pStart, const char *pQuotePos)
{
  if ((*pQuotePos == '\'')
   && (pQuotePos >= pStart + 2)
   && (as_toupper(*(pQuotePos - 2)) == 'A')
   && (as_toupper(*(pQuotePos - 1)) == 'F'))
    return False;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_Z80(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - Z80 variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void DissectReg_Z80(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize8Bit:
      if ((value & 0xf0) == (IXPrefix & 0xf0))
        as_snprintf(p_dest, dest_size, "%s%c", Reg16Names[4], (value & 1) ? 'L' : (is_z80u() ? 'H' : 'U'));
      else if ((value & 0xf0) == (IYPrefix & 0xf0))
        as_snprintf(p_dest, dest_size, "%s%c", Reg16Names[5], (value & 1) ? 'L' : (is_z80u() ? 'H' : 'U'));
      else if ((value < 8) && (value != 6))
        as_snprintf(p_dest, dest_size, "%c", Reg8Names[value]);
      else
        goto none;
      break;
    case eSymbolSize16Bit:
      if ((value & 0xf0) == (IXPrefix & 0xf0))
        as_snprintf(p_dest, dest_size, Reg16Names[4]);
      else if ((value & 0xf0) == (IYPrefix & 0xf0))
        as_snprintf(p_dest, dest_size, Reg16Names[5]);
      else if (value < 4)
        as_snprintf(p_dest, dest_size, "%s", Reg16Names[value]);
      else
        goto none;
      break;
    none:
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_Z80(char *p_arg, TempResult *p_result)
 * \brief  handle built-in (register) symbols for Z80
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_Z80(char *p_arg, TempResult *p_result)
{
  Byte reg_num;

  if (DecodeReg8Core(p_arg, &reg_num))
  {
    p_result->Typ = TempReg;
    p_result->DataSize = eSymbolSize8Bit;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = DissectReg_Z80;
    p_result->Contents.RegDescr.compare = NULL;
  }
  else if (DecodeReg16Core(p_arg, &reg_num))
  {
    p_result->Typ = TempReg;
    p_result->DataSize = eSymbolSize16Bit;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = DissectReg_Z80;
    p_result->Contents.RegDescr.compare = NULL;
  }
}

static Boolean ChkMoreOneArg(void)
{
  return (ArgCnt > 1);
}

static Boolean chk_pc_z380(LargeWord addr)
{
  switch (ActPC)
  {
    case SegCode:
      return (addr < (ExtFlag ? 0xfffffffful : 0xffffu));
    default:
      return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_Z80(void *p_user)
 * \brief  switch to Z80 target
 * \param  p_user properties of CPU
 * ------------------------------------------------------------------------ */

static void SwitchTo_Z80(void *p_user)
{
  const TFamilyDescr *p_descr = FindFamilyByName("Zx80");

  p_curr_cpu_props = (const cpu_props_t*)p_user;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SetIsOccupiedFnc = ChkMoreOneArg;

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;

  switch (p_curr_cpu_props->core)
  {
    case e_core_z380:
      /* Allow negative addresses on Z380 to allow a few tests to pass: */

      code_segment_int_type = Int32;
      ChkPC = chk_pc_z380;

      /* Extended Modes only on Z380 */

      if (!onoff_test_and_set(e_onoff_reg_extmode))
        SetFlag(&ExtFlag, ExtModeSymName, False);
      AddONOFF(ExtModeCmdName, &ExtFlag, ExtModeSymName, False);
      if (!onoff_test_and_set(e_onoff_reg_lwordmode))
        SetFlag(&LWordFlag, LWordModeSymName, False);
      AddONOFF(LWordModeCmdName, &LWordFlag , LWordModeSymName , False);
      break;
    case e_core_ez80:
    {
      static const as_assume_rec_t assume_ez80[] =
      {
        { "ADL"  , &Reg_ADL  , 0, 1   , 0, NULL },
        { "MBASE", &Reg_MBASE, 0, 0xff, 0, NULL }
      };

      code_segment_int_type = UInt24;
      AttrChars = "."; HasAttrs = True;
      DecodeAttrPart = decode_attrpart_ez80;
      assume_set(assume_ez80, as_array_size(assume_ez80));
      break;
    }
    case e_core_z180:
    {
      static const as_assume_rec_t ASSUMEZ180s[] =
      {
        { "CBAR" , &Reg_CBAR , 0,  0xff, 0xf0, check_cbar },
        { "CBR"  , &Reg_CBR  , 0,  0xff, 0   , update_z180_areas },
        { "BBR"  , &Reg_BBR  , 0,  0xff, 0   , update_z180_areas },
      };

      assume_set(ASSUMEZ180s, as_array_size(ASSUMEZ180s));
      update_z180_areas();
      code_segment_int_type = UInt19;
      break;
    }
    case e_core_z280:
    {
      create_z280_assumes();
      assume_set(p_z280_data->p_assumes, p_z280_data->assumes_cnt);
      onoff_supmode_add();
      cpu_2_phys_set_current_check(check_z280_areas_current);
      update_z280_areas();
      code_segment_int_type = UInt24;
      break;
    }
    default:
      code_segment_int_type = UInt16;
  }
  SegLimits[SegCode] = IntTypeDefs[code_segment_int_type].Max;

  /* Gameboy Z80 does not have I/O space, and no IX/IY, do not test for them and allow as normal symbols: */

  if (is_sharp())
    Reg16Cnt = 4;
  else
  {
    ValidSegs |= 1 << SegIO;
    Grans[SegIO  ] = 1; ListGrans[SegIO  ] = 1; SegInits[SegIO  ] = 0;
    SegLimits[SegIO  ] = IntTypeDefs[PortIntType()].Max;
    Reg16Cnt = 6;
  }

  MakeCode = MakeCode_Z80;
  IsDef = IsDef_Z80;
  QualifyQuote = QualifyQuote_Z80;
  InternSymbol = InternSymbol_Z80;
  SwitchFrom = DeinitFields; InitFields();
  DissectReg = DissectReg_Z80;
  
  asmerr_warn_relative_add();
}

static const cpu_props_t cpu_props[] =
{
  { "GBZ80"     , e_core_sharp, e_core_flag_none },
  { "LR35902"   , e_core_sharp, e_core_flag_none },
  { "Z80"       , e_core_z80  , e_core_flag_none },
  { "Z80UNDOC"  , e_core_z80u , e_core_flag_none },
  { "Z180"      , e_core_z180 , e_core_flag_none },
  { "RABBIT2000", e_core_r2000, e_core_flag_none },
  { "Z280"      , e_core_z280 , e_core_flag_none },
  { "eZ80190"   , e_core_ez80 , e_core_flag_i_8bit | e_core_flag_no_xio },
  { "eZ80L92"   , e_core_ez80 , e_core_flag_i_8bit },
  { "eZ80F91"   , e_core_ez80 , e_core_flag_none },
  { "eZ80F92"   , e_core_ez80 , e_core_flag_i_8bit },
  { "eZ80F93"   , e_core_ez80 , e_core_flag_i_8bit },
  { "Z380"      , e_core_z380 , e_core_flag_none }
};

void codez80_init(void)
{
  const cpu_props_t *p_props;

  for (p_props = cpu_props; p_props < cpu_props + as_array_size(cpu_props); p_props++)
    (void)AddCPUUser(p_props->p_name, SwitchTo_Z80, (void*)p_props, NULL);

  p_z280_data = (z280_data_t*)calloc(1, sizeof(*p_z280_data));

  AddInitPassProc(InitCode_Z80);
}
