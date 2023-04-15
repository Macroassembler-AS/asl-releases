/* codepdp11.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator PDP-11                                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmcode.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "errmsg.h"
#include "intpseudo.h"
#include "motpseudo.h"
#include "onoff_common.h"
#include "cpu2phys.h"
#include "decfloat.h"
#include "codepdp11.h"

#define default_regsyms_name "DEFAULT_REGSYMS"

#define REG_PC 7
#define REG_SP 6

#define APR_COUNT 8
#define ASSUME_COUNT (2 * APR_COUNT)

typedef enum
{
  ModReg = 0,
  ModImm = 1,
  ModMem = 2
} adr_mode_t;

#define MModReg (1 << ModReg)
#define MModImm (1 << ModImm)
#define MModMem (1 << ModMem)

#define CODE_FLAG_GEN_IMM (1 << 0)
#define CODE_FLAG_16BIT (1 << 1)

/* only used for FP11 insns */

#define CODE_FLAG_32BIT (1 << 2)
#define CODE_FLAG_F64BIT (1 << 3)
#define CODE_FLAG_ARGSWAP (1 << 4)

typedef struct
{
  Word mode;
  unsigned count;
  Word vals[4];
} adr_vals_t;

enum
{
  e_ext_eis = 0, /* MUL, DIV, ASH, ASHC */
  e_ext_fis = 1, /* FADD, FSUB, FDIV, FMUL */
  e_ext_fp11 = 2,
  e_ext_cis = 3,
  e_ext_opt_cnt,
  e_ext_sob_sxt = e_ext_opt_cnt,
  e_ext_xor = 5,
  e_ext_rtt = 6,
  e_ext_mark = 7,
  e_ext_mfpt = 8,
  e_ext_mfp_mtp = 9,
  e_ext_spl = 10,
  e_ext_csm = 11,
  e_ext_wrtlck = 12,
  e_ext_tstset = 13,
  e_ext_mfps_mtps = 14,
  e_ext_wd16 = 15, /* not a real extension, but an instruction set discriminator */
  e_ext_cnt
};

#define e_cpu_flag_eis (1 << e_ext_eis)
#define e_cpu_flag_fis (1 << e_ext_fis)
#define e_cpu_flag_fp11 (1 << e_ext_fp11)
#define e_cpu_flag_cis (1 << e_ext_cis)
#define e_cpu_flag_sob_sxt (1 << e_ext_sob_sxt)
#define e_cpu_flag_xor (1 << e_ext_xor)
#define e_cpu_flag_rtt (1 << e_ext_rtt)
#define e_cpu_flag_mark (1 << e_ext_mark)
#define e_cpu_flag_mfpt (1 << e_ext_mfpt)
#define e_cpu_flag_mfp_mtp (1 << e_ext_mfp_mtp)
#define e_cpu_flag_spl (1 << e_ext_spl)
#define e_cpu_flag_csm (1 << e_ext_csm)
#define e_cpu_flag_wrtlck (1 << e_ext_wrtlck)
#define e_cpu_flag_tstset (1 << e_ext_tstset)
#define e_cpu_flag_mfps_mtps (1 << e_ext_mfps_mtps)
#define e_cpu_flag_wd16 (1 << e_ext_wd16)

typedef struct
{
  char name[15];
  Byte addr_space, opt_flags;
  Word flags;
} cpu_props_t;

static const cpu_props_t *p_curr_cpu_props;
static tSymbolSize op_size;
static Boolean default_regsyms;
static LongInt *reg_par, *reg_pdr;

static Boolean is_wd16(void)
{
  return !!(p_curr_cpu_props->flags & e_cpu_flag_wd16);
}

/*-------------------------------------------------------------------------*/
/* Register Symbols */

/*!------------------------------------------------------------------------
 * \fn     decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
 * \brief  check whether argument is a CPU register
 * \param  p_arg argument to check
 * \param  p_result numeric register value if yes
 * \param  p_size returns register size
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
{
  if (!as_strcasecmp(p_arg, "PC") && default_regsyms)
  {
    *p_result = REG_PC | REGSYM_FLAG_ALIAS;
    *p_size = eSymbolSize16Bit;
    return True;
  }
  else if (!as_strcasecmp(p_arg, "SP") && default_regsyms)
  {
    *p_result = REG_SP | REGSYM_FLAG_ALIAS;
    *p_size = eSymbolSize16Bit;
    return True;
  }

  switch (strlen(p_arg))
  {
    case 2:
      if ((((as_toupper(*p_arg) == 'R') && default_regsyms)
        || (*p_arg == '%'))
       && isdigit(p_arg[1])
       && (p_arg[1] < '8'))
      {
        *p_result = p_arg[1] - '0';
        *p_size = eSymbolSize16Bit;
        return True;
      }
      break;
    case 3:
      if ((as_toupper(*p_arg) == 'A')
       && (as_toupper(p_arg[1]) == 'C')
       && isdigit(p_arg[2])
       && (p_arg[2] < '6'))
      {
        *p_result = p_arg[2] - '0';
        *p_size = eSymbolSizeFloat64Bit;
        return True;
      }
      break;
    default:
      break;
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_pdp11(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - PDP-11 variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void dissect_reg_pdp11(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize16Bit:
      switch (value)
      {
        case REGSYM_FLAG_ALIAS | REG_PC:
          as_snprintf(p_dest, dest_size, "PC");
          break;
        case REGSYM_FLAG_ALIAS | REG_SP:
          as_snprintf(p_dest, dest_size, "SP");
          break;
        default:
          as_snprintf(p_dest, dest_size, "R%u", (unsigned)(value & 7));
      }
      break;
    case eSymbolSizeFloat64Bit:
      as_snprintf(p_dest, dest_size, "AC%u", (unsigned)(value & 7));
      break;
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*--------------------------------------------------------------------------*/
/* PDP-11 specific flags */

/*!------------------------------------------------------------------------
 * \fn     onoff_ext_add(Boolean def_value)
 * \brief  register on/off command for specific instruction set extension
 * \param  def_value default value of flag upon first use
 * ------------------------------------------------------------------------ */

static Byte ext_registered = 0;

unsigned ext_test_and_set(unsigned mask)
{
  unsigned curr = ext_registered;

  ext_registered |= mask;
  return curr & mask;
}

static const char ext_names[e_ext_opt_cnt][5] = { "EIS", "FIS", "FP11", "CIS" };
static Boolean ext_avail[e_ext_opt_cnt];

/* default value setting only upon first registration, similar to scheme in
   onoff_common.c: */

static void onoff_ext_add(unsigned ext, Boolean def_value)
{
  Byte ext_mask = 1 << ext;

  if (!ext_test_and_set(ext_mask))
    SetFlag(&ext_avail[ext], ext_names[ext], def_value);
  AddONOFF(ext_names[ext], &ext_avail[ext], ext_names[ext], False);
}

/*!------------------------------------------------------------------------
 * \fn     check_cpu_ext(unsigned ext)
 * \brief  check whether instruction is allowed on CPU
 * \param  ext request instruction set extension
 * \return True if usage is OK
 * ------------------------------------------------------------------------ */

static Boolean check_cpu_ext(unsigned ext)
{
  unsigned mask = 1 << ext;

  if (p_curr_cpu_props->flags & mask)
    return True;
  else if ((ext < e_ext_opt_cnt) && (p_curr_cpu_props->opt_flags & mask) && ext_avail[ext])
    return True;
  else
  {
    if (ext < e_ext_opt_cnt)
      WrXErrorPos(ErrNum_InstructionNotSupported, ext_names[ext], &OpPart.Pos);
    else
      WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }
}

/*--------------------------------------------------------------------------*/
/* Address Mode Parser */

/*!------------------------------------------------------------------------
 * \fn     check_sup_mode(void)
 * \brief  check for CPU in supervisor mode
 * \return constant true
 * ------------------------------------------------------------------------ */

static Boolean check_sup_mode(void)
{
  if (!SupAllowed)
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     reset_adr_vals(adr_vals_t *p_vals)
 * \brief  clear encoded addressing mode
 * \param  p_vals encoded mode to reset
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = 0;
  p_vals->count = 0;
  p_vals->vals[0] = 0;
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  p_arg argument to check
 * \param  p_result numeric register value if yes
 * \param  p_size returns operand size of register (I16 or F64)
 * \param  req_size request specific operand/register size
 * \param  must_be_reg argument is expected to be a register
 * \return RegEvalResult
 * ------------------------------------------------------------------------ */

static tErrorNum chk_reg_size(tSymbolSize req_size, tSymbolSize act_size)
{
  if ((act_size == eSymbolSizeUnknown)
   || (req_size == eSymbolSizeUnknown)
   || (req_size == act_size))
    return ErrNum_None;
  else if ((req_size == eSymbolSize16Bit) && (act_size == eSymbolSizeFloat64Bit))
    return ErrNum_IntButFloat;
  else if ((req_size == eSymbolSizeFloat64Bit) && (act_size == eSymbolSize16Bit))
    return ErrNum_FloatButInt;
  else
    return ErrNum_InvOpSize;
}

static tRegEvalResult decode_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
{
  tRegDescr reg_descr;
  tEvalResult eval_result;
  tRegEvalResult reg_eval_result;

  if (decode_reg_core(p_arg->str.p_str, p_result, &eval_result.DataSize))
  {
    reg_descr.Reg = *p_result;
    reg_eval_result = eIsReg;
  }
  else
    reg_eval_result = EvalStrRegExpressionAsOperand(p_arg, &reg_descr, &eval_result, eSymbolSizeUnknown, must_be_reg);

  if (reg_eval_result == eIsReg)
  {
    tErrorNum error_num = chk_reg_size(req_size, eval_result.DataSize);

    if (error_num)
    {
      WrStrErrorPos(error_num, p_arg);
      reg_eval_result = must_be_reg ? eIsNoReg : eRegAbort;
    }
  }

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  if (p_size) *p_size = eval_result.DataSize;
  return reg_eval_result;
}

static Boolean decode_reg_or_const(const tStrComp *p_arg, Word *p_result)
{
  switch (decode_reg(p_arg, p_result, NULL, eSymbolSize16Bit, False))
  {
    case eIsReg:
      return True;
    case eIsNoReg:
    {
      Boolean ok;
      *p_result = EvalStrIntExpression(p_arg, UInt3, &ok);
      return ok;
    }
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(tStrComp *p_arg, adr_vals_t *p_result, Word pc_value, unsigned mode_mask)
 * \brief  parse address expression
 * \param  p_arg source argument
 * \param  p_result parsed result
 * \param  pc_value value of PC to be used in PC-relative calculation
 * \param  mode_mask bit mask of allowed addressing modes
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean is_pre_decrement(const tStrComp *p_arg, Word *p_result, tRegEvalResult *p_reg_eval_result)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (p_arg->str.p_str[0] != '-')
   || (p_arg->str.p_str[1] != '(')
   || (p_arg->str.p_str[arg_len - 1] != ')'))
    return False;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 2, arg_len - 3);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, NULL, eSymbolSize16Bit, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static Boolean is_post_increment(const tStrComp *p_arg, Word *p_result, tRegEvalResult *p_reg_eval_result)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (p_arg->str.p_str[0] != '(')
   || (p_arg->str.p_str[arg_len - 2] != ')')
   || (p_arg->str.p_str[arg_len - 1] != '+'))
    return False;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 1, arg_len - 3);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, NULL, eSymbolSize16Bit, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static Boolean decode_abs(const tStrComp *p_arg, Word *p_addr)
{
  Boolean ok;

  *p_addr = EvalStrIntExpression(p_arg, UInt16, &ok);
  return ok;
}

static Boolean decode_imm8(const tStrComp *p_arg, Word *p_value)
{
  Boolean ok;

  *p_value = EvalStrIntExpression(p_arg, Int8, &ok) & 0xff;
  return ok;
}

static Boolean check_mode_mask(unsigned mode_mask, unsigned act_mask, tStrComp *p_arg)
{
  if (!(mode_mask & act_mask))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  else
    return True;
}

static Boolean decode_adr(tStrComp *p_arg, adr_vals_t *p_result, Word pc_value, unsigned mode_mask)
{
  tEvalResult eval_result;
  tRegEvalResult reg_eval_result;
  Boolean deferred;
  tStrComp arg;
  int arg_len, split_pos;

  reset_adr_vals(p_result);

  /* split off deferred flag? */

  deferred = (p_arg->str.p_str[0] == '@');
  StrCompRefRight(&arg, p_arg, deferred);
  if (deferred)
    KillPrefBlanksStrCompRef(&arg);

  /* Rn, @Rn, ACn: */

  switch (decode_reg(&arg, &p_result->mode, NULL, (deferred || (op_size < eSymbolSizeFloat32Bit)) ? eSymbolSize16Bit : eSymbolSizeFloat64Bit, False))
  {
    case eIsReg:
      p_result->mode |= deferred ? 010 : 000;
      return check_mode_mask(mode_mask, deferred ? MModMem : MModReg, p_arg);
    case eRegAbort:
      return False;
    default:
      break;
  }

  /* #imm, @#abs */

  if (*arg.str.p_str == '#')
  {
    tStrComp imm_arg;

    if (!deferred && !check_mode_mask(mode_mask, deferred ? MModMem : MModImm, p_arg))
      return False;
    StrCompRefRight(&imm_arg, &arg, 1);

    if (deferred)
    {
      eval_result.OK = decode_abs(&imm_arg, &p_result->vals[0]);
      if (eval_result.OK)
        p_result->count = 2;
    }
    else switch (op_size)
    {
      case eSymbolSize8Bit:
        eval_result.OK = decode_imm8(&imm_arg, &p_result->vals[0]);
        if (eval_result.OK)
          p_result->count = 2;
        break;
      case eSymbolSize16Bit:
        p_result->vals[0] = EvalStrIntExpressionWithResult(&imm_arg, Int16, &eval_result);
        if (eval_result.OK)
          p_result->count = 2;
        break;
      case eSymbolSize32Bit:
      {
        LongWord l_val = EvalStrIntExpressionWithResult(&imm_arg, Int32, &eval_result);
        if (eval_result.OK)
        {
          p_result->vals[0] = (l_val >> 16) & 0xffff;
          p_result->vals[1] = l_val & 0xffff;
          p_result->count = 4;
        }
        break;
      }
      case eSymbolSizeFloat32Bit:
      {
        Double f_val = EvalStrFloatExpressionWithResult(&imm_arg, Float64, &eval_result);
        if (eval_result.OK)
        {
          int ret = Double_2_dec4(f_val, p_result->vals);
          eval_result.OK = check_dec_fp_dispose_result(ret, &imm_arg);
        }
        if (eval_result.OK)
          p_result->count = 4;
        break;
      }  
      case eSymbolSizeFloat64Bit:
      {
        Double f_val = EvalStrFloatExpressionWithResult(&imm_arg, Float64, &eval_result);
        if (eval_result.OK)
        {
          int ret = Double_2_dec8(f_val, p_result->vals);
          eval_result.OK = check_dec_fp_dispose_result(ret, &imm_arg);
        }
        if (eval_result.OK)
          p_result->count = 8;
        break;
      }  
      case eSymbolSizeUnknown:
        WrStrErrorPos(ErrNum_UndefOpSizes, p_arg);
        eval_result.OK = False;
        break;
      default:
        WrStrErrorPos(ErrNum_InvOpSize, p_arg);
        eval_result.OK = False;
    }
    if (eval_result.OK)
    {
      /* immediate is actually (PC)+, absolute is actually @(PC)+ */

      p_result->mode = (deferred ? 030 : 020) | REG_PC;
    }
    return eval_result.OK;
  }

  /* (Rn)+, @(Rn)+ */

  if (is_post_increment(&arg, &p_result->mode, &reg_eval_result))
  {
    if (eRegAbort == reg_eval_result)
      return False;
    p_result->mode |= deferred ? 030 : 020;
    return check_mode_mask(mode_mask, MModMem, p_arg);
  }

  /* -(Rn), @-(Rn) */

  if (is_pre_decrement(&arg, &p_result->mode, &reg_eval_result))
  {
    if (eRegAbort == reg_eval_result)
      return False;
    p_result->mode |= (deferred ? 050 : 040);
    return check_mode_mask(mode_mask, MModMem, p_arg);
  }

  /* (Rn), X(Rn) */

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp disp_arg, reg_arg;

    StrCompSplitRef(&disp_arg, &reg_arg, &arg, &arg.str.p_str[split_pos]);
    KillPostBlanksStrComp(&disp_arg);
    KillPrefBlanksStrCompRef(&reg_arg);
    StrCompShorten(&reg_arg, 1);
    KillPostBlanksStrComp(&reg_arg);

    if (!decode_reg_or_const(&reg_arg, &p_result->mode))
      return False;
    if (!*disp_arg.str.p_str && !deferred)
      p_result->mode |= 010;
    else
    {
      p_result->vals[0] = EvalStrIntExpressionWithResult(&disp_arg, Int16, &eval_result);
      if (!eval_result.OK)
        return False;
      p_result->mode |= deferred ? 070 : 060;
      p_result->count = 2;
    }
    return check_mode_mask(mode_mask, MModMem, p_arg);
  }

  /* Remains: rel, @rel
     PC value is the PC value after displacement was loaded: */

  p_result->vals[0] = EvalStrIntExpressionWithResult(&arg, UInt16, &eval_result) - (pc_value + 2);
  if (!eval_result.OK)
    return False;
  p_result->mode = REG_PC | (deferred ? 070 : 060);
  p_result->count = 2;
  return check_mode_mask(mode_mask, MModMem, p_arg);
}

/*!------------------------------------------------------------------------
 * \fn     decode_ac_03(tStrComp *p_arg, Word *p_result)
 * \brief  handle FP11 argument that may only refer AC0...AC3
 * \param  p_arg source argument
 * \param  p_result resulting AC number
 * \return True if valid argument
 * ------------------------------------------------------------------------ */

static Boolean decode_ac_03(tStrComp *p_arg, Word *p_result)
{
  adr_vals_t adr_vals;

  /* operand size must have been set to a floating point type before */
  if (!decode_adr(p_arg, &adr_vals, EProgCounter() + 2, MModReg))
    return False;
  *p_result = adr_vals.mode & 7;
  if (*p_result > 3)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  return True;
}

/*--------------------------------------------------------------------------*/
/* Instruction Handler Helpers */

static void append_word(Word code)
{
  WAsmCode[CodeLen >> 1] = code;
  CodeLen += 2;
}

static void append_adr_vals(const adr_vals_t *p_vals)
{
  if (p_vals->count >= 2)
    append_word(p_vals->vals[0]);
  if (p_vals->count >= 4)
    append_word(p_vals->vals[1]);
  if (p_vals->count >= 6)
    append_word(p_vals->vals[2]);
  if (p_vals->count >= 8)
    append_word(p_vals->vals[3]);
}

static unsigned imm_mask(Word code)
{
  return (code & CODE_FLAG_GEN_IMM) ? MModImm : 0;
}

/*--------------------------------------------------------------------------*/
/* Instruction Handlers */

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    append_word(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_fixed_sup(Word code)
 * \brief  handle privileged instructions without argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed_sup(Word code)
{
  if (ChkArgCnt(0, 0) && check_sup_mode())
    append_word(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_mfpt(Word code)
 * \brief  handle MFPT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mfpt(Word code)
{
  if (ChkArgCnt(0, 0) && check_cpu_ext(e_ext_mfpt))
    append_word(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_rtt(Word code)
 * \brief  handle RTT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_rtt(Word code)
{
  if (ChkArgCnt(0, 0) && check_cpu_ext(e_ext_rtt))
    append_word(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_one_reg(Word code)
 * \brief  handle instructions with one register as argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_one_reg(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    adr_vals_t reg_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &reg_adr_vals, EProgCounter() + 2, MModReg))
      append_word((code & 0xfff8) | reg_adr_vals.mode);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_two_reg(Word code)
 * \brief  handle instructions with two registers as argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_two_reg(Word code)
{
  if (ChkArgCnt(2, 2))
  {
    adr_vals_t src_adr_vals, dest_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &src_adr_vals, EProgCounter() + 2, MModReg)
     && decode_adr(&ArgStr[2], &dest_adr_vals, EProgCounter() + 2, MModReg))
      append_word((code & 0xffc0) | ((src_adr_vals.mode & 7) << 3) | (dest_adr_vals.mode & 7));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_one_arg(Word code)
 * \brief  handle instructions with one generic arg
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_one_arg(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_tstset(Word code)
 * \brief  handle TSTSET instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_tstset(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_tstset))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mfp_mtp(Word code)
 * \brief  handle MFP/MTP instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mfp_mtp(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_mfp_mtp))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mfps_mtps(Word code)
 * \brief  handle MFPS/MTPS instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mfps_mtps(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_mfps_mtps))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_csm(Word code)
 * \brief  handle CSM instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_csm(Word code)
{
  /* TODO: not in kernel mode */
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_csm))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_sxt(Word code)
 * \brief  handle SXT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_sxt(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_sob_sxt))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_fp11_f1_f3(Word code)
 * \brief  handle FP11 instructions with two arguments (AC is dest)
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fp11_f1_f3(Word code)
{
  if (ChkArgCnt(2, 2) && check_cpu_ext(e_ext_fp11))
  {
    adr_vals_t adr_vals;
    Word ac_num;
    int ac_arg_index = (code & CODE_FLAG_ARGSWAP) ? 1 : 2;

    if (code & CODE_FLAG_16BIT)
      op_size = eSymbolSize16Bit;
    else if (code & CODE_FLAG_32BIT)
      op_size = eSymbolSize32Bit;
    else if (code & CODE_FLAG_F64BIT)
      op_size = eSymbolSizeFloat64Bit;
    else
      op_size = eSymbolSizeFloat32Bit;
    if (decode_adr(&ArgStr[3 - ac_arg_index], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      op_size = (code & CODE_FLAG_F64BIT) ? eSymbolSizeFloat64Bit : eSymbolSizeFloat32Bit;
      if (decode_ac_03(&ArgStr[ac_arg_index], &ac_num))
      {
        append_word((code & 0177400) | (ac_num << 6) | adr_vals.mode);
        append_adr_vals(&adr_vals);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_fp11_f2(Word code)
 * \brief  handle FP11 instructions with one (float) arg
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fp11_f2(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_fp11))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_F64BIT) ? eSymbolSizeFloat64Bit : eSymbolSizeFloat32Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     void decode_fp11_f4(Word code)
 * \brief  handle FP11 instructions with one (int) arg
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fp11_f4(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_fp11))
  {
    adr_vals_t adr_vals;

    if (code & CODE_FLAG_16BIT)
      op_size = eSymbolSize16Bit;
    else if (code & CODE_FLAG_32BIT)
      op_size = eSymbolSize32Bit;
    else
      op_size = eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_fp11_f5(Word code)
 * \brief  handle FP11 instructions with no arg
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fp11_f5(Word code)
{
  if (ChkArgCnt(0, 0) && check_cpu_ext(e_ext_fp11))
    append_word(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_cis_0(Word code)
 * \brief  handle CIS instructions with no arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_0(Word code)
{
  if (ChkArgCnt(0, 0) && check_cpu_ext(e_ext_cis))
    append_word(code);
}

/* TODO: CIS instructions take address of descriptors.  Should
   this be written as absolute addressing? */

/*!------------------------------------------------------------------------
 * \fn     decode_cis_1i(Word code)
 * \brief  handle CIS instructions with one address and one immediate argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_1i(Word code)
{
  Word src, imm;

  if (ChkArgCnt(2, 2)
   && check_cpu_ext(e_ext_cis)
   && decode_abs(&ArgStr[1], &src)
   && decode_imm8(&ArgStr[2], &imm))
  {
    append_word(code);
    append_word(src);
    append_word(imm);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cis_2(Word code)
 * \brief  handle CIS instructions with two address arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_2(Word code)
{
  Word src, dest;

  if (ChkArgCnt(2, 2)
   && check_cpu_ext(e_ext_cis)
   && decode_abs(&ArgStr[1], &src)
   && decode_abs(&ArgStr[2], &dest))
  {
    append_word(code);
    append_word(src);
    append_word(dest);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cis_2i(Word code)
 * \brief  handle CIS instructions with two address and one immediate argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_2i(Word code)
{
  Word src, dest, imm;

  if (ChkArgCnt(3, 3)
   && check_cpu_ext(e_ext_cis)
   && decode_abs(&ArgStr[1], &src)
   && decode_abs(&ArgStr[2], &dest)
   && decode_imm8(&ArgStr[3], &imm))
  {
    append_word(code);
    append_word(src);
    append_word(dest);
    append_word(imm);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cis_2i1(Word code)
 * \brief  handle CIS instructions with two src address, one immediate,
           and one dest address argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_2i1(Word code)
{
  Word src1, src2, dest, imm;

  if (ChkArgCnt(4, 4)
   && check_cpu_ext(e_ext_cis)
   && decode_abs(&ArgStr[1], &src1)
   && decode_abs(&ArgStr[2], &src2)
   && decode_imm8(&ArgStr[3], &imm)
   && decode_abs(&ArgStr[4], &dest))
  {
    append_word(code);
    append_word(src1);
    append_word(src2);
    append_word(imm);
    append_word(dest);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cis_3(Word code)
 * \brief  handle CIS instructions with three address arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_3(Word code)
{
  Word src1, src2, dest;

  if (ChkArgCnt(3, 3)
   && check_cpu_ext(e_ext_cis)
   && decode_abs(&ArgStr[1], &src1)
   && decode_abs(&ArgStr[2], &src2)
   && decode_abs(&ArgStr[3], &dest))
  {
    append_word(code);
    append_word(src1);
    append_word(src2);
    append_word(dest);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cis_ld(Word code)
 * \brief  handle CIS instructions with one post-inc argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cis_ld(Word code)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(1, 1)
   && check_cpu_ext(e_ext_cis)
   && decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModMem))
  {
    if ((adr_vals.mode & 070) != 020) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    else
      append_word(code | (adr_vals.mode & 07));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_wrtlck(Word code)
 * \brief  handle WRTLCK instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_wrtlck(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_wrtlck))
  {
    adr_vals_t adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModMem | imm_mask(code)))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_two_arg(Word code)
 * \brief  handle instructions with two generic args
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_two_arg(Word code)
{
  if (ChkArgCnt(2, 2))
  {
    adr_vals_t src_adr_vals, dest_adr_vals;

    op_size = (code & CODE_FLAG_16BIT) ? eSymbolSize16Bit : eSymbolSize8Bit;
    if (decode_adr(&ArgStr[1], &src_adr_vals, EProgCounter() + 2, MModReg | MModMem | MModImm)
     && decode_adr(&ArgStr[2], &dest_adr_vals, EProgCounter() + 2 + src_adr_vals.count, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0170000) | (src_adr_vals.mode << 6) | dest_adr_vals.mode);
      append_adr_vals(&src_adr_vals);
      append_adr_vals(&dest_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_eis(Word code)
 * \brief  handle EIS instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_eis(Word code)
{
  if (ChkArgCnt(2, 2) && check_cpu_ext(e_ext_eis))
  {
    adr_vals_t src_adr_vals, dest_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &src_adr_vals, EProgCounter() + 2, MModReg | MModMem | MModImm)
     && decode_adr(&ArgStr[2], &dest_adr_vals, EProgCounter() + 2 + src_adr_vals.count, MModReg))
    {
      append_word((code & 0177000) | ((dest_adr_vals.mode & 7) << 6) | src_adr_vals.mode);
      append_adr_vals(&src_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_gen(Word code)
 * \brief  handle instructions with one register and one general argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_gen(Word code)
{
  if (ChkArgCnt(2, 2))
  {
    adr_vals_t reg_adr_vals, dest_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &reg_adr_vals, EProgCounter() + 2, MModReg)
     && decode_adr(&ArgStr[2], &dest_adr_vals, EProgCounter() + 2 + reg_adr_vals.count, MModReg | MModMem | MModImm))
    {
      append_word((code & 0177000) | ((reg_adr_vals.mode & 7) << 6) | dest_adr_vals.mode);
      append_adr_vals(&dest_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_xor(Word code)
 * \brief  handle XOR instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_xor(Word code)
{
  if (ChkArgCnt(2, 2) && check_cpu_ext(e_ext_xor))
  {
    adr_vals_t src_adr_vals, dest_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &src_adr_vals, EProgCounter() + 2, MModReg)
     && decode_adr(&ArgStr[2], &dest_adr_vals, EProgCounter() + 2 + src_adr_vals.count, MModReg | MModMem | imm_mask(code)))
    {
      append_word((code & 0177000) | ((src_adr_vals.mode & 7) << 6) | dest_adr_vals.mode);
      append_adr_vals(&dest_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_fis(Word code)
 * \brief  handle FIS instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fis(Word code)
{
  adr_vals_t reg_adr_vals;

  if (ChkArgCnt(1, 1)
   && check_cpu_ext(e_ext_fis)
   && decode_adr(&ArgStr[1], &reg_adr_vals, EProgCounter() + 2, MModReg))
    append_word((code & 0177770) | (reg_adr_vals.mode & 7));
}

/*!------------------------------------------------------------------------
 * \fn     decode_branch(Word code)
 * \brief  handle branch instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_branch(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    LongInt dist = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &eval_result) - (EProgCounter() + 2);

    if (eval_result.OK)
    {
      if ((dist & 1) && !mFirstPassUnknownOrQuestionable(eval_result.Flags)) WrStrErrorPos(ErrNum_DistIsOdd, &ArgStr[1]);
      else
      {
        dist /= 2;
        if (!RangeCheck(dist, SInt8) && !mFirstPassUnknownOrQuestionable(eval_result.Flags)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
        else
          append_word((code & 0xff00) | (dist & 0x00ff));
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_sob(Word code)
 * \brief  handle SOB instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_sob(Word code)
{
  adr_vals_t reg_adr_vals;

  if (ChkArgCnt(2, 2)
   && check_cpu_ext(e_ext_sob_sxt)
   && decode_adr(&ArgStr[1], &reg_adr_vals, EProgCounter() + 2, MModReg))
  {
    tEvalResult eval_result;
    LongInt dist = EvalStrIntExpressionWithResult(&ArgStr[2], UInt16, &eval_result) - (EProgCounter() + 2);

    if ((dist & 1) && !mFirstPassUnknownOrQuestionable(eval_result.Flags)) WrStrErrorPos(ErrNum_DistIsOdd, &ArgStr[1]);
    else
    {
      dist /= 2;
      if (((dist > 0) || (dist < -63)) && !mFirstPassUnknownOrQuestionable(eval_result.Flags)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
        append_word((code & 0177000) | ((reg_adr_vals.mode & 7) << 6) | ((-dist) & 077));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jmp(Word code)
 * \brief  handle JMP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jmp(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + 2, MModMem | MModImm))
    {
      append_word((code & 0177700) | adr_vals.mode);
      append_adr_vals(&adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jsr_core(Word code, Word reg)
 * \brief  JSR instruction common core
 * \param  code machine code
 * \param  reg register operand
 * ------------------------------------------------------------------------ */

static void decode_jsr_core(Word code, Word reg)
{
  adr_vals_t addr_adr_vals;

  op_size = eSymbolSize16Bit;
  if (decode_adr(&ArgStr[ArgCnt], &addr_adr_vals, EProgCounter() + 2, (is_wd16() ? 0 : MModReg) | MModMem | MModImm))
  {
    append_word((code & 0177000) | ((reg & 7) << 6) | addr_adr_vals.mode);
    append_adr_vals(&addr_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jsr(Word code)
 * \brief  handle JSR instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jsr(Word code)
{
  if (ChkArgCnt(2, 2))
  {
    adr_vals_t reg_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &reg_adr_vals, EProgCounter() + 2, MModReg))
      decode_jsr_core(code, reg_adr_vals.mode & 7);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_call(Word code)
 * \brief  handle CALL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_call(Word code)
{
  if (ChkArgCnt(1, 1))
    decode_jsr_core(code, REG_PC);
}

/*!------------------------------------------------------------------------
 * \fn     decode_rts(Word code)
 * \brief  handle RTS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_rts(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    adr_vals_t reg_adr_vals;

    op_size = eSymbolSize16Bit;
    if (decode_adr(&ArgStr[1], &reg_adr_vals, EProgCounter() + 2, MModReg))
      append_word((code & 0177770) | (reg_adr_vals.mode & 7));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm6(Word code)
 * \brief  handle instructions with 6 bit numeric argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm6(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;
    Word num = EvalStrIntExpression(&ArgStr[1], UInt6, &ok);

    if (ok)
      append_word(code | (num & 63));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mark(Word code)
 * \brief  handle MARK instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mark(Word code)
{
  if (check_cpu_ext(e_ext_mark))
    decode_imm6(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm4p1_reg(Word code)
 * \brief  handle instructions with 4 bit immediate and register argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm4p1_reg(Word code)
{
  adr_vals_t reg_adr_vals;

  if (ChkArgCnt(2, 2)
   && decode_adr(&ArgStr[2], &reg_adr_vals, EProgCounter() + 2, MModReg))
  {
    tEvalResult eval_result;
    Word imm_val = EvalStrIntExpressionWithResult(&ArgStr[1], UInt5, &eval_result);

    if (eval_result.OK)
    {
      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        imm_val = 1;
      if (ChkRange(imm_val, 1, 16))
        append_word(code | ((reg_adr_vals.mode & 7) << 6) | ((imm_val - 1) & 15));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_spl(Word code)
 * \brief  handle SPL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_spl(Word code)
{
  if (ChkArgCnt(1, 1) && check_cpu_ext(e_ext_spl))
  {
    Boolean ok;
    Word num = EvalStrIntExpression(&ArgStr[1], UInt3, &ok);

    if (ok)
      append_word(code | (num & 7));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_trap(Word code)
 * \brief  handle trap instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_trap(Word code)
{
  if (ChkArgCnt(0, 1))
  {
    Boolean ok = True;
    Word num = (ArgCnt >= 1) ? EvalStrIntExpression(&ArgStr[1], UInt8, &ok) : 0;

    if (ok)
      append_word(code | (num & 255));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_flags(Word code)
 * \brief  handle generic flag set/clear instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_flags(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok = True;
    Word num = EvalStrIntExpression(&ArgStr[1], UInt4, &ok);

    if (ok)
      append_word(code | (num & 15));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_lcc(Word code)
 * \brief  handle LCC instruction (WD16)
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_lcc(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;
    Word num = EvalStrIntExpression(&ArgStr[1], UInt4, &ok);

    if (ok)
      append_word(code | (num & 15));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_format11(Word code)
 * \brief  handle format 11 instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static Boolean decode_adr_01(tStrComp *p_arg, adr_vals_t *p_vals)
{
  if (!decode_adr(p_arg, p_vals, EProgCounter() + 2, MModReg | MModMem))
    return False;

  switch (p_vals->mode & 070)
  {
    case 010:
      p_vals->mode = (p_vals->mode & 007) | 000;
      break;
    case 070:
      if (p_vals->vals[0] == 0)
      {
        p_vals->count = 0;
        p_vals->mode = (p_vals->mode & 007) | 010;
        break;
      }
      /* else fall-through */
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      reset_adr_vals(p_vals);
      return False;
  }
  return True;
}

static void decode_format11(Word code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && decode_adr_01(&ArgStr[1], &src_adr_vals)
   && decode_adr_01(&ArgStr[2], &dest_adr_vals))
    append_word(code | ((src_adr_vals.mode & 15) << 4) | (dest_adr_vals.mode& 15));
}


/*!------------------------------------------------------------------------
 * \fn     decode_pseudo(void)
 * \brief  handle pseudo instructions
 * \return True if handled
 * ------------------------------------------------------------------------ */

static Boolean decode_pseudo(void)
{
  if (Memo("REG"))
  {
    if (LabPart.str.p_str[0])
      CodeREG(0);
    else if (ChkArgCnt(1, 1))
    {
      Boolean IsON;

      if (CheckONOFFArg(&ArgStr[1], &IsON))
        SetFlag(&default_regsyms, default_regsyms_name, IsON);
    }
    return True;
  }

  if (Memo("BYTE"))
  {
    DecodeIntelDB(eIntPseudoFlag_DECFormat | eIntPseudoFlag_AllowString);
    return True;
  }

  if (Memo("WORD"))
  {
    DecodeIntelDW(eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_DECFormat);
    return True;
  }

  if (is_wd16())
  {
    if (Memo("FLT3"))
    {
      DecodeIntelDM(eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat);
      return True;
    }
  }
  {
    if (Memo("FLT2"))
    {
      DecodeIntelDD(eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat);
      return True;
    }

    if (Memo("FLT4"))
    {
      DecodeIntelDQ(eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat);
      return True;
    }
  }

  if (Memo("PRWINS"))
  {
    cpu_2_phys_area_dump(SegCode, stdout);
    return True;
  }

  return False;
}

/*--------------------------------------------------------------------------*/
/* Memory Management */

/*!------------------------------------------------------------------------
 * \fn     update_apr(void)
 * \brief  compute the CPU -> physical mapping from the current APR values
 * ------------------------------------------------------------------------ */

static void update_apr(void)
{
  int z;
  LargeWord base, size;
  Word acf;

  cpu_2_phys_area_clear(SegCode);
  for (z = 0, base = 0; z < APR_COUNT; z++, base += 0x2000)
  {
    /* page at least read only? */

    acf = reg_pdr[z] & 7;
    if ((acf == 00) || (acf == 03) || (acf == 07))
      continue;

    /* extract size field */

    size = (reg_pdr[z] >> 8) & 0xff;

    /* expand downward -> size field is 2s complement of number of blocks */

    if (reg_pdr[z] & 0x08)
    {
      size = ((size ^ 0xff) + 1) << 6;
      cpu_2_phys_area_add(SegCode, base + 0x2000 - size, (reg_par[z] << 6) + 0x2000 - size, size);
    }

    /* expand upward -> size field is highest block number */

    else
    {
      size = ((size & 0x7f) + 1) << 6;
      cpu_2_phys_area_add(SegCode, base, reg_par[z] << 6, size);
    }
  }
  cpu_2_phys_area_set_cpu_end(SegCode, 0xffff);
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     init_branches(void)
 * \brief  add branch instructions to lookup table (same on PDP-11 and WD16)
 * ------------------------------------------------------------------------ */

static void init_branches(void)
{
  AddInstTable(InstTable, "BCC" , 0103000, decode_branch);
  AddInstTable(InstTable, "BCS" , 0103400, decode_branch);
  AddInstTable(InstTable, "BEQ" , 0001400, decode_branch);
  AddInstTable(InstTable, "BGE" , 0002000, decode_branch);
  AddInstTable(InstTable, "BGT" , 0003000, decode_branch);
  AddInstTable(InstTable, "BHI" , 0101000, decode_branch);
  AddInstTable(InstTable, "BHIS", 0103000, decode_branch);
  AddInstTable(InstTable, "BLE" , 0003400, decode_branch);
  AddInstTable(InstTable, "BLO" , 0103400, decode_branch);
  AddInstTable(InstTable, "BLOS", 0101400, decode_branch);
  AddInstTable(InstTable, "BLT" , 0002400, decode_branch);
  AddInstTable(InstTable, "BMI" , 0100400, decode_branch);
  AddInstTable(InstTable, "BNE" , 0001000, decode_branch);
  AddInstTable(InstTable, "BPL" , 0100000, decode_branch);
  AddInstTable(InstTable, "BR"  , 0000400, decode_branch);
  AddInstTable(InstTable, "BVC" , 0102000, decode_branch);
  AddInstTable(InstTable, "BVS" , 0102400, decode_branch);
}

/*!------------------------------------------------------------------------
 * \fn     init_fields_pdp11(void)
 * \brief  create lookup table - PDP-11 encoding
 * ------------------------------------------------------------------------ */

static void add_one_arg(const char *p_name, Word code)
{
  char name[10];

  AddInstTable(InstTable, p_name, code | CODE_FLAG_16BIT, decode_one_arg);
  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, 0100000 | code, decode_one_arg);
}

static void add_two_arg(const char *p_name, Word code)
{
  char name[10];

  AddInstTable(InstTable, p_name, code | CODE_FLAG_16BIT, decode_two_arg);
  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, 0100000 | code, decode_two_arg);
}

static void add_fp11(const char *p_name, Word code, InstProc proc)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sF", p_name);
  AddInstTable(InstTable, name, code, proc);
  as_snprintf(name, sizeof(name), "%sD", p_name);
  AddInstTable(InstTable, name, code | CODE_FLAG_F64BIT, proc);
}

static void add_cis(const char *p_name, Word code, InstProc inline_proc)
{
  char name[10];

  AddInstTable(InstTable, p_name, code, decode_cis_0);
  as_snprintf(name, sizeof(name), "%sI", p_name);
  AddInstTable(InstTable, name, code | 0000100, inline_proc);
}

static void init_fields_pdp11(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "BPT",   000003, decode_fixed);
  AddInstTable(InstTable, "CCC",   000257, decode_fixed);
  AddInstTable(InstTable, "CLC",   000241, decode_fixed);
  AddInstTable(InstTable, "CLN",   000250, decode_fixed);
  AddInstTable(InstTable, "CLV",   000242, decode_fixed);
  AddInstTable(InstTable, "CLZ",   000244, decode_fixed);
  AddInstTable(InstTable, "HALT",  000000, decode_fixed_sup);
  AddInstTable(InstTable, "IOT",   000004, decode_fixed);
  AddInstTable(InstTable, "MFPT",  000007, decode_mfpt);
  AddInstTable(InstTable, "NOP",   NOPCode, decode_fixed);
  AddInstTable(InstTable, "RESET", 000005, decode_fixed_sup);
  AddInstTable(InstTable, "RTI",   000002, decode_fixed);
  AddInstTable(InstTable, "RTT",   000006, decode_rtt);
  AddInstTable(InstTable, "SCC",   000277, decode_fixed);
  AddInstTable(InstTable, "SEC",   000261, decode_fixed);
  AddInstTable(InstTable, "SEN",   000270, decode_fixed);
  AddInstTable(InstTable, "SEV",   000262, decode_fixed);
  AddInstTable(InstTable, "SEZ",   000264, decode_fixed);
  AddInstTable(InstTable, "WAIT",  000001, decode_fixed);

  add_one_arg("ADC", 005500);
  add_one_arg("ASL", 006300);
  add_one_arg("ASR", 006200);
  add_one_arg("CLR", 005000);
  add_one_arg("COM", 005100);
  add_one_arg("NEG", 005400);
  add_one_arg("DEC", 005300);
  add_one_arg("INC", 005200);
  add_one_arg("ROL", 006100);
  add_one_arg("ROR", 006000);
  add_one_arg("SBC", 005600);
  AddInstTable(InstTable, "SWAB", 000300, decode_one_arg);
  AddInstTable(InstTable, "SXT", 006700, decode_sxt);
  add_one_arg("TST", 005700);
  AddInstTable(InstTable, "TSTSET", 0007200, decode_tstset);
  AddInstTable(InstTable, "WRTLCK", 0007300, decode_wrtlck);
  AddInstTable(InstTable, "CSM"   , 0007000 | CODE_FLAG_16BIT | CODE_FLAG_GEN_IMM, decode_csm);
  AddInstTable(InstTable, "MFPD"  , 0006500 | CODE_FLAG_16BIT | CODE_FLAG_GEN_IMM, decode_mfp_mtp);
  AddInstTable(InstTable, "MFPI"  , 0106500 | CODE_FLAG_16BIT | CODE_FLAG_GEN_IMM, decode_mfp_mtp);
  AddInstTable(InstTable, "MFPS"  , 0106700 , decode_mfps_mtps);
  AddInstTable(InstTable, "MTPD"  , 0006600 | CODE_FLAG_16BIT, decode_mfp_mtp);
  AddInstTable(InstTable, "MTPI"  , 0106600 | CODE_FLAG_16BIT, decode_mfp_mtp);
  AddInstTable(InstTable, "MTPS"  , 0106400 | CODE_FLAG_GEN_IMM, decode_mfps_mtps);

  AddInstTable(InstTable, "FADD"  , 0075000, decode_fis);
  AddInstTable(InstTable, "FSUB"  , 0075010, decode_fis);
  AddInstTable(InstTable, "FMUL"  , 0075020, decode_fis);
  AddInstTable(InstTable, "FDIV"  , 0075030, decode_fis);

  add_fp11("ADD" , 0172000 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  add_fp11("CMP" , 0173400 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  add_fp11("DIV" , 0174400 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDCDF", 0177400 | CODE_FLAG_GEN_IMM | CODE_FLAG_F64BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDCFD", 0177400 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDCIF", 0177000 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDCID", 0177000 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT | CODE_FLAG_F64BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDCLF", 0177000 | CODE_FLAG_GEN_IMM | CODE_FLAG_32BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDCLD", 0177000 | CODE_FLAG_GEN_IMM | CODE_FLAG_32BIT | CODE_FLAG_F64BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "STCFI", 0175400 | CODE_FLAG_ARGSWAP | CODE_FLAG_16BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "STCDI", 0175400 | CODE_FLAG_ARGSWAP | CODE_FLAG_16BIT | CODE_FLAG_F64BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "STCFL", 0175400 | CODE_FLAG_ARGSWAP | CODE_FLAG_32BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "STCDL", 0175400 | CODE_FLAG_ARGSWAP | CODE_FLAG_32BIT | CODE_FLAG_F64BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "LDEXP", 0176400 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_fp11_f1_f3);
  AddInstTable(InstTable, "STEXP", 0175000 | CODE_FLAG_ARGSWAP | CODE_FLAG_16BIT, decode_fp11_f1_f3);
  add_fp11("LD"  , 0172400 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  add_fp11("ST"  , 0174000 | CODE_FLAG_ARGSWAP, decode_fp11_f1_f3);
  add_fp11("MOD" , 0171400 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  add_fp11("MUL" , 0171000 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);
  add_fp11("SUB" , 0173000 | CODE_FLAG_GEN_IMM, decode_fp11_f1_f3);

  add_fp11("ABS" , 0170600, decode_fp11_f2);
  add_fp11("CLR" , 0170400, decode_fp11_f2);
  add_fp11("NEG" , 0170700, decode_fp11_f2);
  add_fp11("TST" , 0170500 | CODE_FLAG_GEN_IMM, decode_fp11_f2);

  AddInstTable(InstTable, "LDFPS", 0170100 | CODE_FLAG_16BIT | CODE_FLAG_GEN_IMM, decode_fp11_f4);
  AddInstTable(InstTable, "STFPS", 0170200 | CODE_FLAG_16BIT, decode_fp11_f4);
  AddInstTable(InstTable, "STST" , 0170300 | CODE_FLAG_32BIT, decode_fp11_f4);

  AddInstTable(InstTable, "CFCC", 0170000, decode_fp11_f5);
  AddInstTable(InstTable, "SETF", 0170001, decode_fp11_f5);
  AddInstTable(InstTable, "SETD", 0170011, decode_fp11_f5);
  AddInstTable(InstTable, "SETI", 0170002, decode_fp11_f5);
  AddInstTable(InstTable, "SETL", 0170012, decode_fp11_f5);

  add_two_arg("CMP", 0020000 | CODE_FLAG_GEN_IMM);
  add_two_arg("BIC", 0040000);
  add_two_arg("BIS", 0050000);
  add_two_arg("BIT", 0030000 | CODE_FLAG_GEN_IMM);
  add_two_arg("MOV", 0010000);
  AddInstTable(InstTable, "ADD", 0060000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "SUB", 0160000 | CODE_FLAG_16BIT, decode_two_arg);

  AddInstTable(InstTable, "ASH" , 0072000, decode_eis);
  AddInstTable(InstTable, "ASHC", 0073000, decode_eis);
  AddInstTable(InstTable, "DIV" , 0071000, decode_eis);
  AddInstTable(InstTable, "MUL" , 0070000, decode_eis);
  AddInstTable(InstTable, "XOR" , 0074000, decode_xor);

  init_branches();
  AddInstTable(InstTable, "SOB" , 0077000, decode_sob);

  AddInstTable(InstTable, "JMP" , 000100, decode_jmp);
  AddInstTable(InstTable, "JSR" , 004000, decode_jsr);
  AddInstTable(InstTable, "CALL", 004000, decode_call);
  AddInstTable(InstTable, "RTS" , 000200, decode_rts);
  AddInstTable(InstTable, "MARK", 006400, decode_mark);

  AddInstTable(InstTable, "EMT"  , 0104000, decode_trap);
  AddInstTable(InstTable, "TRAP" , 0104400, decode_trap);
  AddInstTable(InstTable, "SPL"  , 0000230, decode_spl);

  AddInstTable(InstTable, "C" , 000240, decode_flags);
  AddInstTable(InstTable, "S" , 000260, decode_flags);

  add_cis("ADDN" , 0076050, decode_cis_3);
  add_cis("ADDP" , 0076070, decode_cis_3);
  add_cis("ASHN" , 0076056, decode_cis_2i);
  add_cis("ASHP" , 0076076, decode_cis_2i);
  add_cis("CMPC" , 0076044, decode_cis_2i);
  add_cis("CMPN" , 0076052, decode_cis_2);
  add_cis("CMPP" , 0076072, decode_cis_2);
  add_cis("CVTLN", 0076057, decode_cis_2);
  add_cis("CVTLP", 0076077, decode_cis_2);
  add_cis("CVTNL", 0076053, decode_cis_2);
  add_cis("CVTPL", 0076073, decode_cis_2);
  add_cis("CVTNP", 0076055, decode_cis_2);
  add_cis("CVTPN", 0076054, decode_cis_2);
  add_cis("DIVP" , 0076075, decode_cis_3);
  add_cis("LOCC" , 0076040, decode_cis_2i);
  AddInstTable(InstTable, "L2DR" , 0076020, decode_cis_ld);
  AddInstTable(InstTable, "L3DR" , 0076060, decode_cis_ld);
  add_cis("MATC" , 0076045, decode_cis_2);
  add_cis("MOVC" , 0076030, decode_cis_2i);
  add_cis("MOVRC", 0076031, decode_cis_2i);
  add_cis("MOVTC", 0076032, decode_cis_2i1);
  add_cis("MULP" , 0076074, decode_cis_3);
  add_cis("SCANC", 0076042, decode_cis_2);
  add_cis("SKPC" , 0076041, decode_cis_1i);
  add_cis("SPANC", 0076043, decode_cis_2);
  add_cis("SUBN" , 0076051, decode_cis_3);
  add_cis("SUBP" , 0076071, decode_cis_3);
}

/*!------------------------------------------------------------------------
 * \fn     init_fields_wd16(void)
 * \brief  create lookup table - WD16 encoding
 * ------------------------------------------------------------------------ */

static Boolean TrueFnc(void)
{
  return True;
}

static void init_fields_wd16(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "NOP"  , NOPCode, decode_fixed);
  AddInstTable(InstTable, "RESET", 0x0001 , decode_fixed);
  AddInstTable(InstTable, "IEN"  , 0x0002 , decode_fixed);
  AddInstTable(InstTable, "IDS"  , 0x0003 , decode_fixed);
  AddInstTable(InstTable, "HALT" , 0x0004 , decode_fixed);
  AddInstTable(InstTable, "XCT"  , 0x0005 , decode_fixed);
  AddInstTable(InstTable, "BPT"  , 0x0006 , decode_fixed);
  AddInstTable(InstTable, "WFI"  , 0x0007 , decode_fixed);
  AddInstTable(InstTable, "RSVC" , 0x0008 , decode_fixed);
  AddInstTable(InstTable, "RRTT" , 0x0009 , decode_fixed);
  AddInstTable(InstTable, "SAVE" , 0x000a , decode_fixed); SaveIsOccupiedFnc = TrueFnc;
  AddInstTable(InstTable, "SAVS" , 0x000b , decode_fixed);
  AddInstTable(InstTable, "REST" , 0x000c , decode_fixed);
  AddInstTable(InstTable, "RRTN" , 0x000d , decode_fixed);
  AddInstTable(InstTable, "RSTS" , 0x000e , decode_fixed);
  AddInstTable(InstTable, "RTT"  , 0x000f , decode_fixed);

  AddInstTable(InstTable, "IAK"  , 0x0010 , decode_one_reg);
  AddInstTable(InstTable, "RTN"  , 0x0018 , decode_one_reg);
  AddInstTable(InstTable, "MSKO" , 0x0020 , decode_one_reg);
  AddInstTable(InstTable, "PRTN" , 0x0028 , decode_one_reg);

  AddInstTable(InstTable, "LCC"  , 0x0030 , decode_lcc);

  AddInstTable(InstTable, "SVCA" , 0x0040 , decode_imm6);
  AddInstTable(InstTable, "SVCB" , 0x0080 , decode_imm6);
  AddInstTable(InstTable, "SVCC" , 0x00c0 , decode_imm6);

  AddInstTable(InstTable, "ADDI" , 0x0800 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SUBI" , 0x0810 , decode_imm4p1_reg);
  AddInstTable(InstTable, "BICI" , 0x0820 , decode_imm4p1_reg);
  AddInstTable(InstTable, "MOVI" , 0x0830 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SSRR" , 0x8800 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SSLR" , 0x8810 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SSRA" , 0x8820 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SSLA" , 0x8830 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SDRR" , 0x8e00 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SDLR" , 0x8e10 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SDRA" , 0x8e20 , decode_imm4p1_reg);
  AddInstTable(InstTable, "SDLA" , 0x8e30 , decode_imm4p1_reg);

  add_one_arg("ROR", 0x0a00);
  add_one_arg("ROL", 0x0a40);
  add_one_arg("TST", 0x0a80);
  add_one_arg("ASL", 0x0ac0);
  add_one_arg("SET", 0x0b00); SetIsOccupiedFnc = TrueFnc;
  add_one_arg("CLR", 0x0b40);
  add_one_arg("ASR", 0x0b80);
  add_one_arg("COM", 0x0c00);
  add_one_arg("NEG", 0x0c40);
  add_one_arg("INC", 0x0c80);
  add_one_arg("DEC", 0x0cc0);
  AddInstTable(InstTable, "SWAB" , 0x0bc0, decode_one_arg);
  AddInstTable(InstTable, "SWAD" , 0x8bc0, decode_one_arg);
  AddInstTable(InstTable, "IW2"  , 0x0d00, decode_one_arg);
  AddInstTable(InstTable, "SXT"  , 0x0d40, decode_one_arg);
  AddInstTable(InstTable, "TCALL", 0x0d80 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_one_arg);
  AddInstTable(InstTable, "TJMP" , 0x0dc0 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_one_arg);
  AddInstTable(InstTable, "LSTS" , 0x8d00 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_one_arg);
  AddInstTable(InstTable, "SSTS" , 0x8d40, decode_one_arg);
  AddInstTable(InstTable, "ADC"  , 0x8d80, decode_one_arg);
  AddInstTable(InstTable, "SBC"  , 0x8dc0, decode_one_arg);

  AddInstTable(InstTable, "MBWU" , 0x0e00, decode_two_reg);
  AddInstTable(InstTable, "MBWD" , 0x0e40, decode_two_reg);
  AddInstTable(InstTable, "MBBU" , 0x0e80, decode_two_reg);
  AddInstTable(InstTable, "MBBD" , 0x0ec0, decode_two_reg);
  AddInstTable(InstTable, "MBWA" , 0x0f00, decode_two_reg);
  AddInstTable(InstTable, "MBBA" , 0x0f40, decode_two_reg);
  AddInstTable(InstTable, "MABW" , 0x0f80, decode_two_reg);
  AddInstTable(InstTable, "MABB" , 0x0fc0, decode_two_reg);

  AddInstTable(InstTable, "JSR" , 0x7000, decode_jsr);
  AddInstTable(InstTable, "CALL", 0x7000, decode_call);
  AddInstTable(InstTable, "LEA" , 0x7200, decode_jsr);
  AddInstTable(InstTable, "JMP" , 0x7200, decode_call);
  AddInstTable(InstTable, "ASH" , 0x7400, decode_reg_gen);
  AddInstTable(InstTable, "SOB" , 0x7600, decode_sob);
  AddInstTable(InstTable, "XCH" , 0x7800, decode_reg_gen);
  AddInstTable(InstTable, "ASHC", 0x7a00, decode_reg_gen);
  AddInstTable(InstTable, "MUL" , 0x7c00, decode_reg_gen);
  AddInstTable(InstTable, "DIV" , 0x7e00, decode_reg_gen);

  AddInstTable(InstTable, "ADD" , 0x1000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "SUB" , 0x2000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "AND" , 0x3000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "BIC" , 0x4000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "BIS" , 0x5000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "XOR" , 0x6000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "CMP" , 0x9000 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "BIT" , 0xa000 | CODE_FLAG_GEN_IMM | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "MOV" , 0xb000 | CODE_FLAG_16BIT, decode_two_arg);
  AddInstTable(InstTable, "CMPB", 0xc000 | CODE_FLAG_GEN_IMM, decode_two_arg);
  AddInstTable(InstTable, "MOVB", 0xd000, decode_two_arg);
  AddInstTable(InstTable, "BISB", 0xe000, decode_two_arg);

  AddInstTable(InstTable, "FADD", 0xf000, decode_format11);
  AddInstTable(InstTable, "FSUB", 0xf100, decode_format11);
  AddInstTable(InstTable, "FMUL", 0xf200, decode_format11);
  AddInstTable(InstTable, "FDIV", 0xf300, decode_format11);
  AddInstTable(InstTable, "FCMP", 0xf400, decode_format11);

  init_branches();
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  destroy/cleanup lookup table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     intern_symbol_pdp11(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for PDP-11
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_pdp11(char *p_arg, TempResult *p_result)
{
  Word reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_pdp11;
    p_result->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     make_code_pdp11(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void make_code_pdp11(void)
{
  CodeLen = 0; DontPrint = False;
  op_size = eSymbolSizeUnknown;

  /* to be ignored */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (decode_pseudo())
    return;

  /* machine instructions may not begin on odd addresses */

  if (Odd(EProgCounter()))
  {
    if (DoPadding)
      InsertPadding(1, False);
    else
      WrError(ErrNum_AddrNotAligned);
  }

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_pdp11(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_pdp11(void)
{
  return Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     initpass_pdp11(void)
 * \brief  pre-initialize APRs to 1:1 mapping
 * ------------------------------------------------------------------------ */

static void initpass_pdp11(void)
{
  /* if the PDP-11 target is never used, the memory never gets allocated: */

  if (reg_par)
  {
    int z;

    /* initialize APRs to 1:1 mapping */

    for (z = 0; z < APR_COUNT; z++)
    {
      reg_par[z] = z << 7;
      reg_pdr[z] = 0x7f05;
    }
  }
  ext_registered = 0;
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_pdp11(void)
 * \brief  deinitialize as target
 * ------------------------------------------------------------------------ */

static void switch_from_pdp11(void)
{
  deinit_fields();
  p_curr_cpu_props = NULL;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_pdp11(void *p_user)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_pdp11(void *p_user)
{
  static char *p_assume_reg_names = NULL;
  static ASSUMERec *p_assumes = NULL;
  const TFamilyDescr *p_descr;

  p_curr_cpu_props = (const cpu_props_t*)p_user;
  p_descr = FindFamilyByName(is_wd16() ? "WD16" : "PDP-11");
  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = is_wd16() ? 0x0000 : 000240;
  DivideChars = ",";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 2;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[p_curr_cpu_props->addr_space].Max;

  MakeCode = make_code_pdp11;
  IsDef = is_def_pdp11;
  SwitchFrom = switch_from_pdp11;
  InternSymbol = intern_symbol_pdp11;
  DissectReg = dissect_reg_pdp11;
  multi_char_le = True;

  if (!is_wd16())
    onoff_supmode_add();
  AddONOFF(DoPaddingName, &DoPadding, DoPaddingName, False);
  if (p_curr_cpu_props->opt_flags & e_cpu_flag_eis)
    onoff_ext_add(e_ext_eis, False);
  if (p_curr_cpu_props->opt_flags & e_cpu_flag_fis)
    onoff_ext_add(e_ext_fis, False);
  if (p_curr_cpu_props->opt_flags & e_cpu_flag_fp11)
    onoff_ext_add(e_ext_fp11, False);
  if (p_curr_cpu_props->opt_flags & e_cpu_flag_cis)
    onoff_ext_add(e_ext_cis, False);
  if (!ext_test_and_set(0x80))
    SetFlag(&default_regsyms, default_regsyms_name, True);

  /* create list of PDP-11 paging registers upon first use */

  if (!is_wd16())
  {
    if (!reg_par)
    {
      reg_par = (LongInt*)calloc(APR_COUNT, sizeof(*reg_par));
      reg_pdr = (LongInt*)calloc(APR_COUNT, sizeof(*reg_pdr));
      initpass_pdp11();
    }
    if (!p_assumes)
    {
      int apr_index, assume_index, l;
      char *p_reg_name;

      if (!p_assume_reg_names)
        p_assume_reg_names = (char*)malloc(ASSUME_COUNT * (4 + 1));
      p_assumes = (ASSUMERec*)calloc(ASSUME_COUNT, sizeof(*p_assumes));

      p_reg_name = p_assume_reg_names;
      for (apr_index = 0; apr_index < APR_COUNT; apr_index++)
      {
        l = as_snprintf(p_reg_name, 6, "PAR%c", apr_index + '0');
        p_assumes[apr_index * 2].Name = p_reg_name;
        p_assumes[apr_index * 2].Dest = &reg_par[apr_index];
        p_reg_name += l + 1;
        l = as_snprintf(p_reg_name, 6, "PDR%c", apr_index + '0');
        p_assumes[apr_index * 2 + 1].Name = p_reg_name;
        p_assumes[apr_index * 2 + 1].Dest = &reg_pdr[apr_index];
        p_reg_name += l + 1;
      }
      for (assume_index = 0; assume_index < ASSUME_COUNT; assume_index++)
      {
        p_assumes[assume_index].Min = 0x0000;
        p_assumes[assume_index].Max = 0xffff;
        p_assumes[assume_index].NothingVal = 0x0000;
        p_assumes[assume_index].pPostProc = update_apr;
      }
    }
    pASSUMERecs = p_assumes;
    ASSUMERecCnt = ASSUME_COUNT;
    update_apr();
  }

  if (is_wd16())
    init_fields_wd16();
  else
    init_fields_pdp11();
}

/*!------------------------------------------------------------------------
 * \fn     codepdp11_init(void)
 * \brief  register PDP-11 target
 * ------------------------------------------------------------------------ */

/* NOTE: the KEV-11C implements DIS, which is actually a subset of CIS, but
   since noone knows which subset, we just treat DIS as CIS: */

#define opt_cpu_flags_lsi11 (e_cpu_flag_eis | e_cpu_flag_fis | e_cpu_flag_cis)
#define cpu_flags_lsi11 (e_cpu_flag_sob_sxt | e_cpu_flag_xor | e_cpu_flag_rtt | e_cpu_flag_mark | e_cpu_flag_mfps_mtps)

#define opt_cpu_flags_f11 (e_cpu_flag_fp11)
#define cpu_flags_f11 (e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_xor | e_cpu_flag_mfpt | e_cpu_flag_eis | e_cpu_flag_mfp_mtp | e_cpu_flag_mfps_mtps)

#define opt_cpu_flags_t11 0
#define cpu_flags_t11 (e_cpu_flag_sob_sxt | e_cpu_flag_rtt | e_cpu_flag_mfps_mtps)

#define opt_cpu_flags_j11 0
#define cpu_flags_j11 (e_cpu_flag_eis | e_cpu_flag_fp11 | e_cpu_flag_sob_sxt | e_cpu_flag_xor | e_cpu_flag_rtt | e_cpu_flag_mark | e_cpu_flag_mfpt | e_cpu_flag_mfp_mtp | e_cpu_flag_mfps_mtps | e_cpu_flag_spl | e_cpu_flag_csm | e_cpu_flag_wrtlck | e_cpu_flag_tstset)

static const cpu_props_t cpu_props[] =
{
  {      "PDP-11/03" , UInt16, opt_cpu_flags_lsi11              , cpu_flags_lsi11 },
  {      "PDP-11/04" , UInt16, 0                                , e_cpu_flag_rtt },
  {      "PDP-11/05" , UInt16, 0                                , 0 }, /* OEM version of PDP-11/10 */
  {      "PDP-11/10" , UInt16, 0                                , 0 },
  {      "PDP-11/15" , UInt16, 0                                , 0 }, /* OEM version of PDP-11/20 */
  {      "PDP-11/20" , UInt16, 0                                , 0 },
  {      "PDP-11/23" , UInt22, opt_cpu_flags_f11                , cpu_flags_f11 },
  {      "PDP-11/24" , UInt22, opt_cpu_flags_f11                , cpu_flags_f11 },
  {      "PDP-11/34" , UInt18, e_cpu_flag_fp11                  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor                   | e_cpu_flag_mfp_mtp | e_cpu_flag_mfps_mtps },
  {      "PDP-11/35" , UInt18, e_cpu_flag_eis | e_cpu_flag_fis  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt                  | e_cpu_flag_xor                   | e_cpu_flag_mfp_mtp }, /* OEM version of PDP-11/40 */
  {      "PDP-11/40" , UInt18, e_cpu_flag_eis | e_cpu_flag_fis  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt                  | e_cpu_flag_xor                   | e_cpu_flag_mfp_mtp },
  {      "PDP-11/44" , UInt22, e_cpu_flag_fp11 | e_cpu_flag_cis ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor | e_cpu_flag_mfpt | e_cpu_flag_mfp_mtp | e_cpu_flag_spl | e_cpu_flag_csm },
  {      "PDP-11/45" , UInt18, e_cpu_flag_fp11                  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor                                        | e_cpu_flag_spl },
  {      "PDP-11/50" , UInt18, e_cpu_flag_fp11                  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor                                        | e_cpu_flag_spl },
  { "MicroPDP-11/53" , UInt22, opt_cpu_flags_j11                , cpu_flags_j11 },
  {      "PDP-11/55" , UInt18, e_cpu_flag_fp11                  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor                                        | e_cpu_flag_spl },
  {      "PDP-11/60" , UInt18, 0                                , e_cpu_flag_fp11 | e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor                   | e_cpu_flag_mfp_mtp },
  {      "PDP-11/70" , UInt22, e_cpu_flag_fp11                  ,                   e_cpu_flag_sob_sxt | e_cpu_flag_mark | e_cpu_flag_rtt | e_cpu_flag_eis | e_cpu_flag_xor                   | e_cpu_flag_mfp_mtp | e_cpu_flag_spl },
  { "MicroPDP-11/73" , UInt22, opt_cpu_flags_j11                , cpu_flags_j11 },
  { "MicroPDP-11/83" , UInt22, opt_cpu_flags_j11                , cpu_flags_j11 },
  {      "PDP-11/84" , UInt22, opt_cpu_flags_j11                , cpu_flags_j11 },
  { "MicroPDP-11/93" , UInt22, opt_cpu_flags_j11                , cpu_flags_j11 },
  {      "PDP-11/94" , UInt22, opt_cpu_flags_j11                , cpu_flags_j11 },
  {           "T-11" , UInt16, opt_cpu_flags_t11                , cpu_flags_t11 },

  /* The WD16 is basically an LSI-11 with 
     - different microcode,
     - different opcodes,
     - different floating point format
     but same architecture: */

  {           "WD16" , UInt16, 0                                , e_cpu_flag_wd16 | e_cpu_flag_sob_sxt },
};

void codepdp11_init(void)
{
  const cpu_props_t *p_prop;

  for (p_prop = cpu_props; p_prop < cpu_props + as_array_size(cpu_props); p_prop++)
    (void)AddCPUUserWithArgs(p_prop->name, switch_to_pdp11, (void*)p_prop, NULL, NULL);

  AddInitPassProc(initpass_pdp11);
}
