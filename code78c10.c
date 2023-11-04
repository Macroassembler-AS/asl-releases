/* code78c10.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator NEC uPD78(C)(0|1)x                                          */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmstructs.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"
#include "headids.h"

#include "code78c10.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  const char *p_name;
  Byte code;
} intflag_t;

typedef struct
{
  const char name[5];
  Byte code, flags;
} reg_t;

typedef struct
{
  const char *p_name;
  Byte code, flags, core_mask;
} sreg_t;

typedef struct
{
  const char *pName;
  Byte Code;
  Byte MayIndirect;
} tAdrMode;

typedef enum
{
  eCoreNone,
  eCore7800Low,
  eCore7800High,
  eCore7807,
  eCore7810
} tCore;

#define core_mask_no_low ((1 << eCore7800High) | (1 << eCore7807) | (1 << eCore7810))
#define core_mask_7800 ((1 << eCore7800Low) | (1 << eCore7800High))
#define core_mask_7800_low (1 << eCore7800Low)
#define core_mask_7800_high (1 << eCore7800High)
#define core_mask_7807 (1 << eCore7807)
#define core_mask_7810 (1 << eCore7810)
#define core_mask_7807_7810 ((1 << eCore7807) | (1 << eCore7810))
#define core_mask_all ((1 << eCore7800Low) | (1 << eCore7800High) | (1 << eCore7807) | (1 << eCore7810))
#define core_mask_cmos 0x80

enum
{
  eFlagHasV = 1 << 0,
  eFlagCMOS = 1 << 1,
  eFlagSR   = 1 << 2, /* sr  -> may be written from A */
  eFlagSR1  = 1 << 3, /* sr1 -> may be read to A */
  eFlagSR2  = 1 << 4, /* sr2 -> load or read/modify/write with immediate */
  eFlagSR3  = 1 << 5, /* sr3 -> may be written from EA */
  eFlagSR4  = 1 << 6  /* sr4 -> may be read to EA */
};

/* Flags in high byte of ALU operations: */

#define ALUImm_SR (1 << 0)
#define ALUReg_Src (1 << 1)
#define ALUReg_Dest (1 << 2)
#define ALUReg_MayZ80 (1 << 3)

typedef struct
{
  char Name[6];
  Byte Core;
  Byte Flags;
} tCPUProps;

typedef enum { e_decode_reg_unknown, e_decode_reg_ok, e_decode_reg_error } decode_reg_res_t;

typedef struct
{
  Word code;
  unsigned core_mask;
} order_t;

typedef enum
{
  e_mod_none = -1,
  e_mod_reg8 = 0,
  e_mod_reg16 = 1,
  e_mod_imm = 2,
  e_mod_indir = 3,
  e_mod_wa = 4,
  e_mod_abs = 5,
  e_mod_sreg8 = 6,
  e_mod_sreg16 = 7
} z80_adr_mode_t;

#define MModReg8 (1 << e_mod_reg8)
#define MModReg16 (1 << e_mod_reg16)
#define MModImm (1 << e_mod_imm)
#define MModIndir (1 << e_mod_indir)
#define MModWA (1 << e_mod_wa)
#define MModAbs (1 << e_mod_abs)
#define MModSReg8 (1 << e_mod_sreg8)
#define MModSReg16 (1 << e_mod_sreg16)

#define REG_V 0
#define REG_A 1
#define REG_B 2
#define REG_C 3
#define REG_D 4
#define REG_H 6
#define REG_EAH 8
#define REG_BC 1
#define REG_DE 2
#define REG_HL 3
#define REG_EA 4

typedef struct
{
  z80_adr_mode_t mode;
  unsigned count;
  Boolean force_long;
  Byte val, vals[2];
} z80_adr_vals_t;

static Boolean is_7807_781x;

static LongInt WorkArea;

static const tCPUProps *pCurrCPUProps;

static order_t *fixed_orders, *reg2_orders;
static sreg_t *s_regs8, *s_regs16;
static intflag_t *int_flags;
static tSymbolSize z80_op_size, z80_def_op_size;

/*--------------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     check_core(unsigned core_mask)
 * \brief  check whether active target uses given core
 * \param  core_mask list of allowed cores
 * \return True if OK
 * ------------------------------------------------------------------------ */

static Boolean check_core(unsigned core_mask)
{
  if ((core_mask & core_mask_cmos) && !(pCurrCPUProps->Flags & eFlagCMOS))
    return False;
  return !!((core_mask >> pCurrCPUProps->Core) & 1);
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg8(const char *p_arg, Byte *p_res)
 * \brief  decode 8 bit register
 * \param  p_arg source argument
 * \param  p_res encoded name
 * \return True if valid name
 * ------------------------------------------------------------------------ */

static Boolean decode_reg8(const char *p_arg, Byte *p_res)
{
  switch (strlen(p_arg))
  {
    case 1:
    {
      static const char names[] = "VABCDEHL";
      const char *p;
      int no_v = !(pCurrCPUProps->Flags & eFlagHasV);

      p = strchr(&names[no_v], as_toupper(*p_arg));
      if (!p)
        return False;
      *p_res = p - names;
      return True;
    }
    case 3:
      if (!is_7807_781x
       || (as_toupper(p_arg[0]) != 'E')
       || (as_toupper(p_arg[1]) != 'A'))
        return False;
      if (as_toupper(p_arg[2]) == 'L')
        *p_res = REG_EAH + 1;
      else if (as_toupper(p_arg[2]) == 'H')
        *p_res = REG_EAH;
      else
        return False;
      return True;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_r(const tStrComp *p_arg, Byte *p_res)
 * \brief  decode name of 8 bit register
 * \param  p_arg source argument
 * \param  p_res return buffer
 * \return e_decode_reg_ok -> register known & OK
 * ------------------------------------------------------------------------ */

static decode_reg_res_t decode_r(const tStrComp *p_arg, Byte *p_res)
{
  if (!decode_reg8(p_arg->str.p_str, p_res))
    return e_decode_reg_unknown;
  if (*p_res >= 8)
  {
    WrStrErrorPos(ErrNum_InvReg, p_arg);
    return e_decode_reg_error;
  }
  else
    return e_decode_reg_ok;
}

/*!------------------------------------------------------------------------
 * \fn     decode_r1(const tStrComp *p_arg, Byte *p_res)
 * \brief  decode name of 8 bit register, plus EAL/H on 78C1x
 * \param  p_arg source argument
 * \param  p_res return buffer
 * \return e_decode_reg_ok -> register known & OK
 * ------------------------------------------------------------------------ */

static decode_reg_res_t decode_r1(const tStrComp *p_arg, Byte *p_res)
{
  if (!decode_reg8(p_arg->str.p_str, p_res))
    return e_decode_reg_unknown;
  if (*p_res < 2)
  {
    WrStrErrorPos(ErrNum_InvReg, p_arg);
    return e_decode_reg_error;
  }
  *p_res &= 7;
  return e_decode_reg_ok;
}

/*!------------------------------------------------------------------------
 * \fn     decode_r2(const tStrComp *p_arg, Byte *p_res)
 * \brief  decode name of 8 bit register A, B, or C
 * \param  p_arg source argument
 * \param  p_res return buffer
 * \return e_decode_reg_ok -> register known & OK
 * ------------------------------------------------------------------------ */

static decode_reg_res_t decode_r2(const tStrComp *p_arg, Byte *p_res)
{
  if (!decode_reg8(p_arg->str.p_str, p_res))
    return e_decode_reg_unknown;
  if ((*p_res == 0) || (*p_res >= 4))
  {
    WrStrErrorPos(ErrNum_InvReg, p_arg);
    return e_decode_reg_error;
  }
  return e_decode_reg_ok;
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg16(char *p_arg, Byte *p_res, Boolean allow_single_letter)
 * \brief  decode 16 bit register argument
 * \param  p_arg source argument
 * \param  p_res result value
 * \param  allow_single_letter allow register names with single letters (not for Z80 syntax)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_reg16(char *p_arg, Byte *p_res, Boolean allow_single_letter)
{
  static const reg_t regs[] =
  {
    { "SP" , 0, 0 },
    { "B"  , REG_BC, 0 },
    { "BC" , REG_BC, 0 },
    { "D"  , REG_DE, 0 },
    { "DE" , REG_DE, 0 },
    { "H"  , REG_HL, 0 },
    { "HL" , REG_HL, 0 },
    { "EA" , REG_EA, 0 },
    { ""   , 0, 0 },
  };

  for (*p_res = 0; regs[*p_res].name[0]; (*p_res)++)
    if (!as_strcasecmp(p_arg, regs[*p_res].name))
    {
      if (!p_arg[1] && !allow_single_letter)
        return False;
      *p_res = regs[*p_res].code;
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rp(char *Asc, Byte *Erg)
 * \brief  decode rp2 argument (SP/BC/DE/HL/EA)
 * \param  Asc source argument
 * \param  Erg resulting register #
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_rp2(char *p_arg, Byte *p_res)
{
  return decode_reg16(p_arg, p_res, True);
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rp(char *Asc, Byte *Erg)
 * \brief  decode rp argument (SP/BC/DE/HL)
 * \param  Asc source argument
 * \param  Erg resulting register #
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_rp(char *Asc, Byte *Erg)
{
  if (!Decode_rp2(Asc, Erg)) return False;
  return (*Erg < 4);
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rp1(char *Asc, Byte *Erg)
 * \brief  decode rp1 argument (VA/BC/DE/HL/EA)
 * \param  Asc source argument
 * \param  Erg resulting register #
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_rp1(char *Asc, Byte *Erg)
{
  if (!as_strcasecmp(Asc, "V")) *Erg = 0;
  else
  {
    if (!Decode_rp2(Asc, Erg)) return False;
    return (*Erg != 0);
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rp3(char *Asc, Byte *Erg)
 * \brief  decode rp3 argument (BC/DE/HL)
 * \param  Asc source argument
 * \param  Erg resulting register #
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_rp3(char *Asc, Byte *Erg)
{
  if (!Decode_rp2(Asc, Erg)) return False;
  return ((*Erg < 4) && (*Erg > 0));
}

/*!------------------------------------------------------------------------
 * \fn     parse_rpa(const tStrComp *p_arg, z80_adr_vals_t *p_vals, unsigned mode_mask, int *p_auto_val, Boolean force)
 * \brief  parse indirect address expression
 * \param  p_arg source argument
 * \param  p_vals buffer for parse result
 * \param  mode_mask allowed addressing modes
 * \param  p_auto_val returns auto-in/decrement value
 * \param  force assume expression is indirect, even without outer (...) 
 * \return True if expression was detected as indirect (parsing may still have failed)
 * ------------------------------------------------------------------------ */

static int split_auto_val(tStrComp *p_arg)
{
  int l = strlen(p_arg->str.p_str);

  if (l >= 2)
  {
    if (!strcmp(p_arg->str.p_str + l - 2, "--"))
    {
      StrCompShorten(p_arg, 2);
      return - 2;
    }
    if (!strcmp(p_arg->str.p_str + l - 2, "++"))
    {
      StrCompShorten(p_arg, 2);
      return + 2;
    }
  }

  if (l >= 1)
  {
    if (p_arg->str.p_str[l - 1] == '-')
    {
      StrCompShorten(p_arg, 1);
      return -1;
    }
    if (p_arg->str.p_str[l - 1] == '+')
    {
      StrCompShorten(p_arg, 1);
      return +1;
    }
  }

  return 0;
}

static void parse_indirect_list(z80_adr_vals_t *p_vals, const tStrComp *p_arg, unsigned mode_mask, int auto_val)
{
  char *p;
  tStrComp rem, arg;
  Byte reg, base = 0, index = 0;
  LongInt disp_acc = 0;
  Boolean bad_reg, first_unknown = False, this_minus, next_minus;

  StrCompRefRight(&arg, p_arg, 0);
  this_minus = False;
  do
  {
    /* split off one component */

    KillPrefBlanksStrCompRef(&arg);
    next_minus = False;
    p = QuotMultPos(arg.str.p_str, "+-"); /* TODO: parentheses */
    if (p)
    {
      next_minus = (*p == '-');
      StrCompSplitRef(&arg, &rem, &arg, p);
      KillPostBlanksStrComp(&arg);
    }
    bad_reg = False;

    /* 8 bit register? Note that a B/D/H may actually mean BC/DE/HL
       in 'old syntax': */

    if (decode_reg8(arg.str.p_str, &reg))
    {
      if (this_minus)
        bad_reg = True;
      else switch (reg)
      {
        case REG_A:
          if (index)
            bad_reg = True;
          else
            index = reg;
          break;
        case REG_B:
          if (!index)
            index = reg;
          else if (!base)
            base = REG_BC;
          else
            bad_reg = True;
          break;
        case REG_D:
          if (!base)
            base = REG_DE;
          else
            bad_reg = True;
          break;
        case REG_H:
          if (!base)
            base = REG_HL;
          else
            bad_reg = True;
          break;
        default:
          bad_reg = True;
      }
    }

    /* 16 bit register? */

    else if (decode_reg16(arg.str.p_str, &reg, False))
    {
      if (this_minus)
        bad_reg = True;
      else switch (reg)
      {
        case REG_EA:
          if (index)
            bad_reg = True;
          else
            index = reg;
          break;
        case REG_BC:
        case REG_DE:
        case REG_HL:
          if (base)
            bad_reg = True;
          else
            base = reg;
          break;
        default:
          bad_reg = True;
      }
    }
    else
    {
      tEvalResult eval_result;
      Word value;

      value = EvalStrIntExpressionWithResult(&arg, UInt16, &eval_result);
      if (!eval_result.OK)
        return;
      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        first_unknown = True;
      disp_acc = this_minus ? (disp_acc - value) : (disp_acc + value);
    }

    if (bad_reg)
    {
      WrStrErrorPos(ErrNum_InvReg, &arg);
      return;
    }

    if (p)
    {
      arg = rem;
      this_minus = next_minus;
    }
  }
  while (p);

  /* Dissolve ambiguities */

  if ((index == REG_B) && !base)
  {
    index = 0;
    base = REG_BC;
  }

  /* For auto-in/decrement, only a plain base register DE/HL is allowed.
     Furthermore, (..)-- is not implemented: */ 

  if (auto_val)
  {
    if (index || (base < REG_BC) || (base > REG_HL) || disp_acc || (auto_val == -2))
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    else
    {
      p_vals->val = (base + 2) + (2 * !!(auto_val < 0));
      p_vals->mode = e_mod_indir;
    }
  }
  else
  {
    /* (BC) (DE) (HL) */
    if ((base >= REG_BC) && (base <= REG_HL) && !index && !disp_acc)
    {
      p_vals->val = base;
      p_vals->mode = e_mod_indir;
    }
    /* (DE+byte) (HL+byte) */
    else if ((base >= REG_DE) && (base <= REG_HL) && !index)
    {
      if (first_unknown)
        disp_acc &= 0xff;
      if ((disp_acc > 0xff) || (disp_acc < -0x80))
      {
        WrStrErrorPos(ErrNum_OverRange, p_arg);
        return;
      }
      p_vals->val = (base << 2) + 3;
      p_vals->mode = e_mod_indir;
      p_vals->vals[p_vals->count++] = Lo(disp_acc);
    }
    /* (HL+A) (HL+B) */
    else if ((base == REG_HL) && (index >= REG_A) && (index <= REG_B) && !disp_acc)
    {
      p_vals->val = index + 11;
      p_vals->mode = e_mod_indir;
    }
    /* (HL+EA) */
    else if ((base == REG_HL) && (index == REG_EA) && !disp_acc)
    {
      p_vals->val = 14;
      p_vals->mode = e_mod_indir;
    }
    /* abs/wa */
    else if (!base && !index)
    {
      Boolean is_wa = (Hi(disp_acc) == WorkArea),
              may_wa = !!(mode_mask & MModWA);

      p_vals->vals[p_vals->count++] = Lo(disp_acc);
      if ((may_wa && is_wa) || (!may_wa && first_unknown))
        p_vals->mode = e_mod_wa;
      else
      {
        p_vals->vals[p_vals->count++] = Hi(disp_acc);
        p_vals->mode = e_mod_abs;
      }
    }
    else
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
  }
}

static Boolean parse_rpa(const tStrComp *p_arg, z80_adr_vals_t *p_vals, unsigned mode_mask, int *p_auto_val, Boolean force)
{
  tStrComp arg;
  Boolean is_indirect;

  /* split off auto-in/decrement */

  StrCompRefRight(&arg, p_arg, 0);
  *p_auto_val = split_auto_val(&arg);

  /* Indirect? */

  is_indirect = IsIndirect(arg.str.p_str);
  if (is_indirect)
  {
    StrCompIncRefLeft(&arg, 1);
    StrCompShorten(&arg, 1);
    KillPostBlanksStrComp(&arg);
    if (!*p_auto_val)
      *p_auto_val = split_auto_val(&arg);
  }

  /* Auto-in/decrement enforces indirect parsing: */

  if (is_indirect || *p_auto_val || force)
  {
    parse_indirect_list(p_vals, &arg, mode_mask, *p_auto_val);
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     reset_z80_adr_vals(z80_adr_vals_t *p_vals)
 * \brief  clear encoded addressing mode container
 * \param  p_vals container to clear
 * ------------------------------------------------------------------------ */

static void reset_z80_adr_vals(z80_adr_vals_t *p_vals)
{
  p_vals->mode = e_mod_none;
  p_vals->force_long = 0;
  p_vals->val = 0;
  p_vals->count = 0;
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rpa2(const tStrComp *pArg, Byte *Erg, Byte *Disp)
 * \brief  Decode rpa2 argument (indirect 8 bit argument in memory)
 * \param  pArg source argument
 * \param  Erg resulting addressing mode
 * \param  Disp resulting addressing displacement
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_rpa2(const tStrComp *pArg, Byte *Erg, Byte *Disp)
{
  z80_adr_vals_t vals;
  int auto_val;

  /* We force parsing as indirect, so the return value is always true: */

  reset_z80_adr_vals(&vals);
  (void)parse_rpa(pArg, &vals, MModIndir, &auto_val, True);
  switch (vals.mode)
  {
    case e_mod_indir:
      if ((auto_val < -1) || (auto_val > 1))
        goto bad_mode;
      *Erg = vals.val;
      *Disp = (vals.count > 0) ? vals.vals[0] : 0;
      return True;
    case e_mod_wa:
    case e_mod_abs:
    bad_mode:
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return False;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rpa(const tStrComp *pArg, Byte *Erg)
 * \brief  Decode rpa argument (indirect 8 bit argument in memory, without index)
 * \param  pArg source argument
 * \param  Erg resulting addressing mode
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean is_rpa(Byte value)
{
  return (value >= 1) && (value <= 7);
}

static Boolean Decode_rpa(const tStrComp *pArg, Byte *Erg)
{
  Byte Dummy;

  if (!Decode_rpa2(pArg, Erg, &Dummy)) return False;
  if (!is_rpa(*Erg))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rpa1(const tStrComp *pArg, Byte *Erg)
 * \brief  Decode rpa1 argument (indirect 8 bit argument in memory, without index or auto-in/decrement)
 * \param  pArg source argument
 * \param  Erg resulting addressing mode
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean is_rpa1(Byte value)
{
  return (value >= 1) && (value <= 3);
}

static Boolean Decode_rpa1(const tStrComp *pArg, Byte *Erg)
{
  Byte Dummy;

  if (!Decode_rpa2(pArg, Erg, &Dummy)) return False;
  if (!is_rpa1(*Erg))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Decode_rpa3(const tStrComp *pArg, Byte *Erg, ShortInt *Disp)
 * \brief  Decode rpa3 argument (indirect 16 bit argument in memory)
 * \param  pArg source argument
 * \param  Erg resulting addressing mode
 * \param  Disp resulting addressing displacement
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean is_rpa3(Byte value)
{
  return ((value >= 2) && (value <= 5))
      || ((value >= 11) && (value <= 15));
}

static Boolean Decode_rpa3(const tStrComp *pArg, Byte *Erg, Byte *Disp)
{
  z80_adr_vals_t vals;
  int auto_val;

  /* We force parsing as indirect, so the return value is always true: */

  reset_z80_adr_vals(&vals);
  (void)parse_rpa(pArg, &vals, MModIndir, &auto_val, True);
  switch (vals.mode)
  {
    case e_mod_indir:
      if ((auto_val != -2) && (auto_val != 0) && (auto_val != 2))
        goto bad_mode;
      if (!is_rpa3(vals.val))
        goto bad_mode;
      *Erg = vals.val;
      *Disp = (vals.count > 0) ? vals.vals[0] : 0;
      return True;
    case e_mod_wa:
    case e_mod_abs:
    bad_mode:
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return False;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     Decode_f(char *Asc, Byte *Erg)
 * \brief  descode status flag
 * \param  Asc source argument
 * \param  Erg returns encoded flag
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_f(char *Asc, Byte *Erg)
{
#define FlagCnt 3
  static const char Flags[FlagCnt][3] = {"CY", "HC", "Z"};

  for (*Erg = 0; *Erg < FlagCnt; (*Erg)++)
   if (!as_strcasecmp(Flags[*Erg], Asc)) break;
  *Erg += 2; return (*Erg <= 4);
}

/*!------------------------------------------------------------------------
 * \fn     decode_sr_core(const tStrComp *p_arg)
 * \brief  core register list decode
 * \param  p_arg source argument
 * \return * to record of decoded register
 * ------------------------------------------------------------------------ */

static const sreg_t *decode_sr_core(const sreg_t *p_sregs, const tStrComp *p_arg)
{
  for (; p_sregs->p_name; p_sregs++)
    if (check_core(p_sregs->core_mask) && !as_strcasecmp(p_arg->str.p_str, p_sregs->p_name))
      return p_sregs;
  return NULL;
}

/*!------------------------------------------------------------------------
 * \fn     check_sr_flag(Byte reg_flags, Byte req_flag, const tStrComp *p_arg)
 * \brief  check whether special register's flags fulfill requirement, and
           issue error if not
 * \param  reg_flags register's suppoerted flags
 * \param  req_flag required flag
 * \param  p_arg source argument
 * \return True if OK
 * ------------------------------------------------------------------------ */

static Boolean check_sr_flag(Byte reg_flags, Byte req_flag, const tStrComp *p_arg)
{
  if (reg_flags & req_flag)
    return True;
  WrStrErrorPos(reg_flags ? ErrNum_InvOpOnReg : ErrNum_InvReg, p_arg);
  return False;
}

static decode_reg_res_t decode_sr_flag(const tStrComp *p_arg, const sreg_t *p_sregs, Byte *p_res, Byte flag)
{
  const sreg_t *p_reg = decode_sr_core(p_sregs, p_arg);

  if (p_reg)
  {
    if (check_sr_flag(p_reg->flags, flag, p_arg))
    {
      *p_res = p_reg->code;
      return e_decode_reg_ok;
    }
    else
      return e_decode_reg_error;
  }
  else
    return e_decode_reg_unknown;
}

#define decode_sr1(p_arg, p_res) decode_sr_flag(p_arg, s_regs8, p_res, eFlagSR1)
#define decode_sr(p_arg, p_res) decode_sr_flag(p_arg, s_regs8, p_res, eFlagSR)
#define decode_sr2(p_arg, p_res) decode_sr_flag(p_arg, s_regs8, p_res, eFlagSR2)

/*!------------------------------------------------------------------------
 * \fn     Decode_irf(const tStrComp *p_arg, ShortInt *p_res)
 * \brief  decode interrupt flag argument
 * \param  p_arg source argument
 * \param  p_res resulting flag #
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_irf(const tStrComp *p_arg, ShortInt *p_res)
{
  for (*p_res = 0; int_flags[*p_res].p_name; (*p_res)++)
    if (!as_strcasecmp(int_flags[*p_res].p_name, p_arg->str.p_str))
    {
      *p_res = int_flags[*p_res].code;
      return True;
    }
  WrStrErrorPos(ErrNum_UnknownInt, p_arg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     Decode_wa(const tStrComp *pArg, Byte *Erg, Byte max_address)
 * \brief  decode working area address argument
 * \param  pArg source argument
 * \param  Erg resulting (short) address
 * \param  max_address range limit
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean Decode_wa(const tStrComp *pArg, Byte *Erg, Byte max_address)
{
  Word Adr;
  Boolean OK;
  tSymbolFlags Flags;

  Adr = EvalStrIntExpressionWithFlags(pArg, Int16, &OK, &Flags);
  if (!OK)
    return False;

  if (!mFirstPassUnknown(Flags) && (Hi(Adr) != WorkArea)) WrStrErrorPos(ErrNum_InAccPage, pArg);
  *Erg = Lo(Adr);

  if (mFirstPassUnknownOrQuestionable(Flags))
    *Erg &= max_address;

  return ChkRangePos(*Erg, 0, max_address, pArg);
}

static Boolean HasDisp(ShortInt Mode)
{
  return ((Mode & 11) == 11);
}

/*!------------------------------------------------------------------------
 * \fn     set_op_size(tSymbolSize new_op_size, const tStrComp *p_arg)
 * \brief  Set operand size and throw error in case of conflict
 * \param  new_op_size operand size to set
 * \param  p_arg source argument carrying this size
 * ------------------------------------------------------------------------ */

static Boolean set_op_size(tSymbolSize new_op_size, const tStrComp *p_arg)
{
  if (z80_op_size == eSymbolSizeUnknown)
    z80_op_size = new_op_size;
  else if (z80_op_size != new_op_size)
  {
    WrStrErrorPos(ErrNum_ConfOpSizes, p_arg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_z80_adr(const tStrComp *p_arg, z80_adr_vals_t *p_vals, unsigned mode_mask)
 * \brief  decode Z80 style address expression
 * \param  p_arg source argument
 * \param  p_vals encoded values
 * \param  mode_mask bit mask of allowe daddressing modes
 * \return resulting addressing mode
 * ------------------------------------------------------------------------ */

static z80_adr_mode_t decode_z80_adr(const tStrComp *p_arg, z80_adr_vals_t *p_vals, unsigned mode_mask)
{
  Boolean ok;
  int auto_val;
  const sreg_t *p_sreg;

  reset_z80_adr_vals(p_vals);

  /* Registers: */

  if (!as_strcasecmp(p_arg->str.p_str, ">A"))
  {
    if (set_op_size(eSymbolSize8Bit, p_arg))
    {
      p_vals->force_long = True;
      p_vals->val = REG_A;
      p_vals->mode = e_mod_reg8;
    }
    goto found;
  }

  if (decode_reg8(p_arg->str.p_str, &p_vals->val))
  {
    /* No V register on low function core */
    if ((pCurrCPUProps->Core == eCore7800Low) && (p_vals->val == REG_V));
    /* EA register only on 7807++ */
    else if ((pCurrCPUProps->Core < eCore7807) && (p_vals->val >= REG_EAH));
    else
    {
      if (set_op_size(eSymbolSize8Bit, p_arg))
        p_vals->mode = e_mod_reg8;
      goto found;
    }
  }

  if (decode_reg16(p_arg->str.p_str, &p_vals->val, False))
  {
    /* EA register only on 7807++ */
    if ((pCurrCPUProps->Core < eCore7807) && (p_vals->val >= REG_EA));
    else
    {
      if (set_op_size(eSymbolSize16Bit, p_arg))
        p_vals->mode = e_mod_reg16;
      goto found;
    }
  }

  p_sreg = decode_sr_core(s_regs8, p_arg);
  if (p_sreg)
  {
    if (set_op_size(eSymbolSize8Bit, p_arg))
    {
      p_vals->mode = e_mod_sreg8;
      p_vals->val = p_sreg->code;
      p_vals->vals[0] = p_sreg->flags;
    }
    goto found;
  }

  p_sreg = decode_sr_core(s_regs16, p_arg);
  if (p_sreg)
  {
    if (set_op_size(eSymbolSize16Bit, p_arg))
    {
      p_vals->mode = e_mod_sreg16;
      p_vals->val = p_sreg->code;
      p_vals->vals[0] = p_sreg->flags;
    }
    goto found;
  }

  /* indirect stuff: */

  if (parse_rpa(p_arg, p_vals, mode_mask, &auto_val, False))
  {
    /* single or double auto-in/decrement implicitly describes operand size: */

    if (auto_val)
    {
      if (!set_op_size(((auto_val == 1) || (auto_val == -1)) ? eSymbolSize8Bit : eSymbolSize16Bit, p_arg))
        reset_z80_adr_vals(p_vals);
    }
    goto found;
  }

  /* Immediate: */

  if (!(mode_mask & MModImm))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    goto found;
  }
  if (z80_op_size == eSymbolSizeUnknown)
  {
    z80_op_size = z80_def_op_size;
    z80_def_op_size = eSymbolSizeUnknown;
  }
  switch (z80_op_size)
  {
    case eSymbolSize8Bit:
      p_vals->vals[0] = EvalStrIntExpression(p_arg, Int8, &ok);
      if (ok)
      {
        p_vals->count = 1;
        p_vals->mode = e_mod_imm;
      }
      break;
    case eSymbolSize16Bit:
    {
      Word tmp = EvalStrIntExpression(p_arg, Int16, &ok);
      if (ok)
      {
        p_vals->vals[p_vals->count++] = Lo(tmp);
        p_vals->vals[p_vals->count++] = Hi(tmp);
        p_vals->mode = e_mod_imm;
      }
      break;
    }
    default:
      WrStrErrorPos(ErrNum_UndefOpSizes, p_arg);
  }

found:
  if ((p_vals->mode != e_mod_none) && !((mode_mask >> p_vals->mode) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    reset_z80_adr_vals(p_vals);
  }
  return p_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals(const z80_adr_vals_t *p_vals)
 * \brief  append encoded addess extension bytes to instruction
 * \param  p_vals contains bytes to append
 * ------------------------------------------------------------------------ */

static void append_adr_vals(const z80_adr_vals_t *p_vals)
{
  memcpy(&BAsmCode[CodeLen], p_vals->vals, p_vals->count);
  CodeLen += p_vals->count;
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling (uPD7807...7809 only) */

/*
 * Compact representation of bits in symbol table:
 * bits 0..2: bit position
 * bits 3...6/18: address in I/O or memory space
 * bit 19: 0 for memory space, 1 for I/O space
 * bits 20..23: core type
 */

/*!------------------------------------------------------------------------
 * \fn     eval_bit_position(const tStrComp *p_arg, Boolean *p_ok)
 * \brief  evaluate bit position
 * \param  bit position argument
 * \param  p_ok parsing OK?
 * \return numeric bit position
 * ------------------------------------------------------------------------ */

static LongWord eval_bit_position(const tStrComp *p_arg, Boolean *p_ok)
{
  return EvalStrIntExpression(p_arg, UInt3, p_ok);
}

/*!------------------------------------------------------------------------
 * \fn     assemble_bit_symbol(Byte core, Boolean is_io, Word address, Byte bit_pos)
 * \brief  transform bit symbol components into compact representation
 * \param  core core used to define this bit
 * \param  is_io special register or memory bit?
 * \param  address (I/O) register address
 * \param  bit_pos bit position
 * \return compact storage value
 * ------------------------------------------------------------------------ */

static LongWord assemble_bit_symbol(Byte core, Boolean is_io, Word address, Byte bit_pos)
{
  LongWord result = bit_pos | ((LongWord)address << 3);

  if (is_io)
    result |= 1ul << 19;
  result |= ((LongWord)core) << 20;
  return result;
}

/*!------------------------------------------------------------------------
 * \fn     decode_bit_arg_2(LongWord *p_result, const tStrComp *p_reg_arg, const tStrComp *p_bit_arg)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  p_result resulting encoded bit
 * \param  p_reg_arg register argument
 * \param  p_bit_arg bit argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_bit_arg_2(LongWord *p_result, const tStrComp *p_reg_arg, const tStrComp *p_bit_arg)
{
  Boolean ok;
  LongWord addr;
  Boolean is_io;
  Byte bit_pos;
  Byte s_reg;

  bit_pos = eval_bit_position(p_bit_arg, &ok);
  if (!ok)
    return False;

  if (decode_sr_flag(p_reg_arg, s_regs8, &s_reg, eFlagSR2) == e_decode_reg_ok)
  {
    if (s_reg > 0x0f)
    {
      WrStrErrorPos(ErrNum_InvReg, p_reg_arg);
      return False;
    }
    is_io = True;
    addr = s_reg;
  }

  else
  {
    addr = EvalStrIntExpression(p_reg_arg, UInt16, &ok);
    if (!ok)
      return False;
    is_io = False;
  }

  *p_result = assemble_bit_symbol(pCurrCPUProps->Core, is_io, addr, bit_pos);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_bit_arg(LongWord *p_result, int start, int stop)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  p_result resulting encoded bit
 * \param  start first argument
 * \param  stop last argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_bit_arg(LongWord *p_result, int start, int stop)
{
  *p_result = 0;

  /* Just one argument -> parse as bit argument */

  if (start == stop)
  {
    char *p_sep = RQuotPos(ArgStr[start].str.p_str, '.');

    if (p_sep)
    {
      tStrComp reg_comp, bit_comp;

      StrCompSplitRef(&reg_comp, &bit_comp, &ArgStr[start], p_sep);
      return decode_bit_arg_2(p_result, &reg_comp, &bit_comp);
    }
    else
    {
      tEvalResult eval_result;

      *p_result = EvalStrIntExpressionWithResult(&ArgStr[start], UInt24, &eval_result);
      if (eval_result.OK)
        ChkSpace(SegBData, eval_result.AddrSpaceMask);
      return eval_result.OK;
    }
  }

  /* register & bit position are given as separate arguments */

  else if (stop == start + 1)
    return decode_bit_arg_2(p_result, &ArgStr[start], &ArgStr[stop]);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     dissect_bit_symbol(LongWord bit_symbol, Byte *p_core, Boolean *p_is_io, Word *p_address, Byte *p_bit_pos)
 * \brief  transform compact representation of bit (field) symbol into components
 * \param  bit_symbol compact storage
 * \param  p_core core used to define bit
 * \param  p_is_io special register or memory bit?
 * \param  p_address (I/O) register address
 * \param  p_bit_pos (start) bit position
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean dissect_bit_symbol(LongWord bit_symbol, Byte *p_core, Boolean *p_is_io, Word *p_address, Byte *p_bit_pos)
{
  *p_core = (bit_symbol >> 20) & 15;
  *p_is_io = (bit_symbol >> 19) & 1;
  *p_address = (bit_symbol >> 3) & (*p_is_io ? 0xf : 0xffff);
  *p_bit_pos = bit_symbol & 7;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_bit_7807(char *p_dest, size_t dest_size, LargeWord inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  p_dest destination for ASCII representation
 * \param  dest_size destination buffer size
 * \param  inp compact storage
 * ------------------------------------------------------------------------ */

static void dissect_bit_7807(char *p_dest, size_t dest_size, LargeWord inp)
{
  Byte bit_pos, core;
  Word address;
  Boolean is_io;

  dissect_bit_symbol(inp, &core, &is_io, &address, &bit_pos);

  if (is_io)
  {
    const sreg_t *p_sreg;

    for (p_sreg = s_regs8; p_sreg->p_name; p_sreg++)
    {
      if (!((p_sreg->core_mask >> core) & 1))
        continue;
      if (address == p_sreg->code)
      {
        as_snprintf(p_dest, dest_size, "%s.%u", p_sreg->p_name, (unsigned)bit_pos);
        return;
      }
    }
    as_snprintf(p_dest, dest_size, "SR%u.%u", address, (unsigned)bit_pos);
  }
  else
    as_snprintf(p_dest, dest_size, "%~.*u%s.%u",
                ListRadixBase, (unsigned)address, GetIntConstIntelSuffix(ListRadixBase),
                (unsigned)bit_pos);
}

/*!------------------------------------------------------------------------
 * \fn     expand_bit_7807(const tStrComp *p_var_name, const struct sStructElem *p_struct_elem, LargeWord base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  p_var_name desired symbol name
 * \param  p_struct_elem element definition
 * \param  base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void expand_bit_7807(const tStrComp *p_var_name, const struct sStructElem *p_struct_elem, LargeWord base)
{
  LongWord address = base + p_struct_elem->Offset;

  if (pInnermostNamedStruct)
  {
    PStructElem p_elem = CloneStructElem(p_var_name, p_struct_elem);

    if (!p_elem)
      return;
    p_elem->Offset = address;
    AddStructElem(pInnermostNamedStruct->StructRec, p_elem);
  }
  else
  {
    if (!ChkRange(address, 0, 0xffff)
     || !ChkRange(p_struct_elem->BitPos, 0, 7))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(p_var_name, assemble_bit_symbol(pCurrCPUProps->Core, False, address, p_struct_elem->BitPos), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*--------------------------------------------------------------------------------*/

static Boolean check_core_and_error(unsigned core_mask)
{
  Boolean ret = check_core(core_mask);

  if (!ret)
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  return ret;
}

static void PutCode(Word Code)
{
  if (Hi(Code) != 0)
    BAsmCode[CodeLen++] = Hi(Code);
  BAsmCode[CodeLen++] = Lo(Code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_bit_7807_core(const tStrComp *p_arg, Byte code)
 * \brief  core routine for uPD7807 bit-oriented instructions
 * \param  p_arg source bit argument
 * \param  code machine code of instruction
 * ------------------------------------------------------------------------ */

static void decode_bit_7807_core(int arg_idx, Byte code)
{
  LongWord packed_bit;
  Boolean is_io;
  Word address;
  Byte bit_pos, core;

  if (!decode_bit_arg(&packed_bit, arg_idx, arg_idx))
    return;
  dissect_bit_symbol(packed_bit, &core, &is_io, &address, &bit_pos);

  if (!is_io)
  {
    if ((Hi(address) != WorkArea) || (Lo(address) > 15))
      WrStrErrorPos(ErrNum_InAccPage, &ArgStr[arg_idx]);
  }

  BAsmCode[1] = (is_io ? 0x80 : 0x00)
              | ((address & 15) << 3)
              | (bit_pos & 7);
  BAsmCode[0] = code;
  CodeLen = 2;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  Handle instructions without arguments
 * \param  index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word index)
{
  const order_t *p_order = &fixed_orders[index];

  if (ChkArgCnt(0, 0) && check_core_and_error(p_order->core_mask))
    PutCode(p_order->code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOV(Word Code)
 * \brief  handle MOV instruction
 * ------------------------------------------------------------------------ */

static void DecodeMOV(Word Code)
{
  Boolean OK;
  Byte HReg;
  Integer AdrInt;
  decode_reg_res_t res1;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
  {
    decode_reg_res_t res2;

    if ((res2 = decode_sr1(&ArgStr[2], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 2;
        BAsmCode[0] = 0x4c;
        BAsmCode[1] = 0xc0 + HReg;
      }
    }
    else if ((res2 = decode_r1(&ArgStr[2], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 1;
        BAsmCode[0] = 0x08 + HReg;
      }
    }
    else
    {
      AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
      if (OK)
      {
        CodeLen = 4;
        BAsmCode[0] = 0x70;
        BAsmCode[1] = 0x69;
        BAsmCode[2] = Lo(AdrInt);
        BAsmCode[3] = Hi(AdrInt);
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
  {
    decode_reg_res_t res2;

    if ((res2 = decode_sr(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 2;
        BAsmCode[0] = 0x4d;
        BAsmCode[1] = 0xc0 + HReg;
      }
    }
    else if ((res2 = decode_r1(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 1;
        BAsmCode[0] = 0x18 + HReg;
      }
    }
    else
    {
      AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
      if (OK)
      {
        CodeLen = 4;
        BAsmCode[0] = 0x70;
        BAsmCode[1] = 0x79;
        BAsmCode[2] = Lo(AdrInt);
        BAsmCode[3] = Hi(AdrInt);
      }
    }
  }
  else if ((pCurrCPUProps->Core == eCore7807) && !as_strcasecmp(ArgStr[1].str.p_str, "CY"))
    decode_bit_7807_core(2, 0x5f);
  else if ((pCurrCPUProps->Core == eCore7807) && !as_strcasecmp(ArgStr[2].str.p_str, "CY"))
    decode_bit_7807_core(1, 0x5a);
  else if ((res1 = decode_r(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
  {
    if (res1 == e_decode_reg_ok)
    {
      AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
      if (OK)
      {
        CodeLen = 4;
        BAsmCode[0] = 0x70;
        BAsmCode[1] = 0x68 + HReg;
        BAsmCode[2] = Lo(AdrInt);
        BAsmCode[3] = Hi(AdrInt);
      }
    }
  }
  else if ((res1 = decode_r(&ArgStr[2], &HReg)) != e_decode_reg_unknown)
  {
    if (res1 == e_decode_reg_ok)
    {
      AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
      if (OK)
      {
        CodeLen = 4;
        BAsmCode[0] = 0x70;
        BAsmCode[1] = 0x78 + HReg;
        BAsmCode[2] = Lo(AdrInt);
        BAsmCode[3] = Hi(AdrInt);
      }
    }
  }

  /* Cannot say in this case if src or dest is invalid register: */

  else
    WrError(ErrNum_InvReg);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMVI(Word Code)
 * \brief  handle MVI instruction
 * ------------------------------------------------------------------------ */

static void DecodeMVI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Byte HReg;
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      decode_reg_res_t res;

      if ((res = decode_r(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
      {
        if (res == e_decode_reg_ok)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x68 + HReg;
        }
      }
      else if ((res = decode_sr2(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
      {
        if (res == e_decode_reg_ok)
        {
          if (!is_7807_781x) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else
          {
            CodeLen = 3;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[0] = 0x64;
            BAsmCode[1] = (HReg & 7) + ((HReg & 8) << 4);
          }
        }
      }
      else WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMVIW(Word Code)
 * \brief  handle MVIW instruction
 * ------------------------------------------------------------------------ */

static void DecodeMVIW(Word Code)
{
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2) || !check_core_and_error(core_mask_no_low));
  else if (Decode_wa(&ArgStr[1], BAsmCode + 1, 0xff))
  {
    BAsmCode[2] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x71;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMVIX(Word Code)
 * \brief  handle MVIX instruction
 * ------------------------------------------------------------------------ */

static void DecodeMVIX(Word Code)
{
  Boolean OK;
  Byte HReg;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2) || !check_core_and_error(core_mask_no_low));
  else if (Decode_rpa1(&ArgStr[1], &HReg))
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x48 + HReg;
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDAX_STAX(Word Code)
 * \brief  handle LDAX/STAX instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDAX_STAX(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(1, 1))
  {
    Boolean ok = is_7807_781x
               ? Decode_rpa2(&ArgStr[1], &HReg, &BAsmCode[1])
               : Decode_rpa(&ArgStr[1], &HReg);

    if (ok)
    {
      CodeLen = 1 + Ord(HasDisp(HReg));
      BAsmCode[0] = Code + ((HReg & 8) << 4) + (HReg & 7);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDEAX_STEAX(Word Code)
 * \brief  handle LDEAX/STEAX instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDEAX_STEAX(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(1, 1)
   && check_core_and_error(core_mask_7807_7810)
   && Decode_rpa3(&ArgStr[1], &HReg, &BAsmCode[2]))
  {
    CodeLen = 2 + Ord(HasDisp(HReg));
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Code + HReg;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLXI(Word Code)
 * \brief  Handle LXI instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLXI(Word Code)
{
  Byte HReg;
  Integer AdrInt;
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2))
    return;
  OK = (pCurrCPUProps->Core >= eCore7807)
     ? Decode_rp2(ArgStr[1].str.p_str, &HReg)
     : Decode_rp(ArgStr[1].str.p_str, &HReg);
  if (!OK) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
  {
    AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x04 + (HReg << 4);
      BAsmCode[1] = Lo(AdrInt);
      BAsmCode[2] = Hi(AdrInt);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePUSH_POP(Word Code)
 * \brief  Handle PUSH/POP instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodePUSH_POP(Word Code)
{
  Byte HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_rp1(ArgStr[1].str.p_str, &HReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
    PutCode(Code | (HReg << (Hi(Code) ? 4 : 0)));
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDMOV(Word Code)
 * \brief  Handle DMOV instruction
 * ------------------------------------------------------------------------ */

static void DecodeDMOV(Word Code)
{
  Byte HReg, SReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2) && check_core_and_error(core_mask_7807_7810))
  {
    Boolean Swap = as_strcasecmp(ArgStr[1].str.p_str, "EA") || False;
    const tStrComp *pArg1 = Swap ? &ArgStr[2] : &ArgStr[1],
                   *pArg2 = Swap ? &ArgStr[1] : &ArgStr[2];

    if (as_strcasecmp(pArg1->str.p_str, "EA")) WrStrErrorPos(ErrNum_InvAddrMode, pArg1);
    else if (Decode_rp3(pArg2->str.p_str, &HReg))
    {
      CodeLen = 1;
      BAsmCode[0] = 0xa4 + HReg;
      if (Swap)
        BAsmCode[0] += 0x10;
    }
    else switch (decode_sr_flag(pArg2, s_regs16, &SReg, Swap ? eFlagSR3 : eFlagSR4))
    {
      case e_decode_reg_ok:
        CodeLen = 2;
        BAsmCode[0] = 0x48;
        BAsmCode[1] = 0xc0 + SReg;
        if (Swap)
          BAsmCode[1] += 0x12;
        break;
      case e_decode_reg_error:
        WrStrErrorPos(ErrNum_InvCtrlReg, pArg2);
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, pArg2);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_alu_z80(Word code)
 * \brief  handle ALU instruction, Z80-style
 * \param  code machine code & flags
 * ------------------------------------------------------------------------ */

static void decode_alu_z80(Word code)
{
  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntaxZ80))
  {
    const Byte flags = Hi(code);
    z80_adr_vals_t src_adr_vals, dest_adr_vals;
    code = Lo(code);

    switch (decode_z80_adr(&ArgStr[1], &dest_adr_vals,
                           MModReg8 | MModReg16 | MModSReg8
                         | ((code & 1) ? MModWA : 0)))
    {
      case e_mod_reg8:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals,
                               MModReg8
                             | MModImm
                             | ((dest_adr_vals.val == REG_A) ? MModIndir : 0)
                             | (((dest_adr_vals.val == REG_A) && (pCurrCPUProps->Core >= eCore7800High)) ? MModWA : 0)))
        {
          case e_mod_reg8:
            if ((dest_adr_vals.val == REG_A) && !dest_adr_vals.force_long && (src_adr_vals.val < 8) && (flags & ALUReg_Src))
            {
              BAsmCode[CodeLen++] = 0x60;
              BAsmCode[CodeLen++] = 0x80 | (code << 3) | src_adr_vals.val;
            }
            else if ((src_adr_vals.val == REG_A) && !src_adr_vals.force_long && (dest_adr_vals.val  < 8) && (flags & ALUReg_Dest))
            {
              BAsmCode[CodeLen++] = 0x60;
              BAsmCode[CodeLen++] = (code << 3) | dest_adr_vals.val;
              /* ONA/OFFA only exist as A,r, but since binary AND is commutative, we can just swap operands: */
              if ((code == 9) || (code == 11))
                BAsmCode[CodeLen - 1] |= 0x80;
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
          case e_mod_imm:
            if ((dest_adr_vals.val == REG_A) && !dest_adr_vals.force_long)
              BAsmCode[CodeLen++] = 0x06 | ((code & 14) << 3) | (code & 1);
            else if (pCurrCPUProps->Core < eCore7800High) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
            else
            {
              BAsmCode[CodeLen++] = 0x74;
              BAsmCode[CodeLen++] = (code << 3) | dest_adr_vals.val;
            }
            if (CodeLen > 0)
              BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            break;
          case e_mod_indir:
            if (!is_rpa(src_adr_vals.val)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x70;
              BAsmCode[CodeLen++] = 0x80 | (code << 3) | src_adr_vals.val;
            }
            break;
          case e_mod_wa:
            BAsmCode[CodeLen++] = 0x74;
            BAsmCode[CodeLen++] = 0x80 | (code << 3);
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            break;
          default:
            break;
        }
        break;
      case e_mod_reg16:
      {
        unsigned mask = MModReg16;

        if (dest_adr_vals.val != REG_EA)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          return;
        }
        if ((code == 8) || (code == 12))
        {
          mask |= MModReg8;
          z80_op_size = eSymbolSizeUnknown;
        }
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, mask))
        {
          case e_mod_reg8:
            if ((src_adr_vals.val < REG_A) || (src_adr_vals.val > REG_C)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x70;
              BAsmCode[CodeLen++] = (code << 3) | src_adr_vals.val;
            }
            break;
          case e_mod_reg16:
            if ((src_adr_vals.val < REG_BC) || (src_adr_vals.val > REG_HL)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x74;
              BAsmCode[CodeLen++] = 0x84 | (code << 3) | src_adr_vals.val;
            }
            break;
          default:
            break;
        }
        break;
      }
      case e_mod_sreg8:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, (flags & ALUImm_SR) ? MModImm : 0))
        {
          case e_mod_imm:
            if (check_sr_flag(dest_adr_vals.vals[0], eFlagSR2, &ArgStr[1]))
            {
              BAsmCode[CodeLen++] = 0x64;
              BAsmCode[CodeLen++] = (code << 3)
                                  | ((pCurrCPUProps->Core < eCore7800High) ? 0x80 : 0x00)
                                  | (dest_adr_vals.val & 7)
                                  | ((dest_adr_vals.val & 8) << 4);
              BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            }
            break;
          default:
            break;
        }
        break;
      case e_mod_wa:
        z80_def_op_size = eSymbolSize8Bit;
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, MModImm))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = ((code & 0x0e) << 3) | 0x05;
            BAsmCode[CodeLen++] = dest_adr_vals.vals[0];
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
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

/*!------------------------------------------------------------------------
 * \fn     DecodeALUImm(Word Code)
 * \brief  handle 8 Bit ALU instructions with immediate source
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeALUImm(Word Code)
{
  ShortInt HVal8;
  Byte HReg;
  Boolean OK;
  Byte Flags;

  Flags = Hi(Code);
  Code = Lo(Code);

  if (ChkArgCnt(2, 2))
  {
    HVal8 = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      tStrComp Arg1;
      decode_reg_res_t res;

      /* allow >A to enforce long addressing */

      StrCompRefRight(&Arg1, &ArgStr[1], as_strcasecmp(ArgStr[1].str.p_str, ">A") ? 0 : 1);
      if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      {
        CodeLen = 2;
        BAsmCode[0] = 0x06 + ((Code & 14) << 3) + (Code & 1);
        BAsmCode[1] = HVal8;
      }
      else if ((res = decode_r(&Arg1, &HReg)) != e_decode_reg_unknown)
      {
        if (res != e_decode_reg_ok);
        else if (pCurrCPUProps->Core == eCore7800Low) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          CodeLen = 3;
          BAsmCode[0] = is_7807_781x ? 0x74 : 0x64;
          BAsmCode[2] = HVal8;
          BAsmCode[1] = HReg + (Code << 3);
        }
      }
      else if ((res = decode_sr2(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
      {
        if (res != e_decode_reg_ok);
        else if (!(Flags & ALUImm_SR)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          CodeLen = 3;
          BAsmCode[0] = 0x64;
          BAsmCode[1] = (HReg & 7) | (Code << 3)
                      | (is_7807_781x ? ((HReg & 8) << 4) : 0x80);
          BAsmCode[2] = HVal8;
        }
      }
      else WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeALUReg(Word Code)
 * \brief  Handle ALU instructions with A and register as argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeALUReg(Word Code)
{
  Byte HReg;
  Byte Flags;

  Flags = Hi(Code);
  if ((CurrZ80Syntax & eSyntaxZ80) && (Flags & ALUReg_MayZ80))
  {
    decode_alu_z80(Code);
    return;
  }
  Code = Lo(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean NoSwap = !as_strcasecmp(ArgStr[1].str.p_str, "A");
    tStrComp *pArg1 = NoSwap ? &ArgStr[1] : &ArgStr[2],
             *pArg2 = NoSwap ? &ArgStr[2] : &ArgStr[1],
             Arg2;
    decode_reg_res_t res;

    /* allow >A to enforce <op> r,A instrad of <op> A,r */

    StrCompRefRight(&Arg2, pArg2, as_strcasecmp(pArg2->str.p_str, ">A") ? 0 : 1);

    if (as_strcasecmp(pArg1->str.p_str, "A")) WrStrErrorPos(ErrNum_InvAddrMode, pArg1);
    else if ((res = decode_r(&Arg2, &HReg)) == e_decode_reg_unknown) WrStrErrorPos(ErrNum_InvReg, &Arg2);
    else if (res == e_decode_reg_ok)
    {
      if (NoSwap && !(Flags & ALUReg_Src)) WrStrErrorPos(ErrNum_InvAddrMode, &Arg2);
      else if (!NoSwap && !(Flags & ALUReg_Dest)) WrStrErrorPos(ErrNum_InvAddrMode, &Arg2);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0x60;
        BAsmCode[1] = (Code << 3) + HReg;
        if ((NoSwap) || (Memo("ONA")) || (Memo("OFFA")))
          BAsmCode[1] += 0x80;
      }
    }
  }
}

static void DecodeALURegW(Word Code)
{
  if (ChkArgCnt(1, 1)
   && check_core_and_error(core_mask_no_low)
   && Decode_wa(&ArgStr[1], BAsmCode + 2, 0xff))
  {
    CodeLen = 3;
    BAsmCode[0] = 0x74;
    BAsmCode[1] = 0x80 + (Code << 3);
  }
}

static void DecodeALURegX(Word Code)
{
  Byte HReg;

  if (!ChkArgCnt(1, 1));
  else if (Decode_rpa(&ArgStr[1], &HReg))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x70;
    BAsmCode[1] = 0x80 + (Code << 3) + HReg;
  }
}

static void DecodeALUEA(Word Code)
{
  Byte HReg;

  if (!ChkArgCnt(2, 2));
  else if (!check_core_and_error(core_mask_7807_7810));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else if (!Decode_rp3(ArgStr[2].str.p_str, &HReg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x74;
    BAsmCode[1] = 0x84 + (Code << 3) + HReg;
  }
}

static void DecodeALUImmW(Word Code)
{
  Boolean OK;

  if (!ChkArgCnt(2, 2));
  else if (Decode_wa(&ArgStr[1], BAsmCode + 1, 0xff))
  {
    BAsmCode[2] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x05 + ((Code >> 1) << 4);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAbs(Word Code)
 * \brief  Handle instructions with absolute address as argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeAbs(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else
  {
    Boolean OK;
    Integer AdrInt;

    AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      PutCode(Code);
      BAsmCode[CodeLen++] = Lo(AdrInt);
      BAsmCode[CodeLen++] = Hi(AdrInt);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg2(Word index)
 * \brief  Handle instructions with r2 as argument
 * \param  index index into instruction table
 * ------------------------------------------------------------------------ */

static void DecodeReg2(Word index)
{
  const order_t *p_order = &reg2_orders[index];
  Byte HReg;
  decode_reg_res_t res;

  if (!ChkArgCnt(1, 1) || !check_core_and_error(p_order->core_mask));
  else if ((res = decode_r2(&ArgStr[1], &HReg)) == e_decode_reg_unknown) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else if (res == e_decode_reg_ok)
    PutCode(p_order->code + HReg);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeWA(Word Code)
 * \brief  Handle instructions with work area address as argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeWork(Word Code)
{
  if (ChkArgCnt(1, 1)
   && Decode_wa(&ArgStr[1], BAsmCode + 1, 0xff))
  {
    CodeLen = 2;
    BAsmCode[0] = Code;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeA(Word Code)
 * \brief  Handle instructions with A as argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeA(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
    PutCode(Code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEA(Word Code)
 * \brief  Handle instructions with EA as argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeEA(Word Code)
{
  if (!ChkArgCnt(1, 1) || !check_core_and_error(core_mask_7807_7810));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDCX_INX(Word Code)
 * \brief  Handle INX/DCX Instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeDCX_INX(Word Code)
{
  Byte HReg;

  if (!ChkArgCnt(1, 1));
  else if (check_core(core_mask_7807_7810) && !as_strcasecmp(ArgStr[1].str.p_str, "EA"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0xa8 + Code;
  }
  else if (Decode_rp(ArgStr[1].str.p_str, &HReg))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x02 + Code + (HReg << 4);
  }
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

/*!------------------------------------------------------------------------
 * \fn     decode_inc_dec(word code)
 * \brief  Handle Z80-style INC/DEC instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_inc_dec(Word code)
{
  if (ChkArgCnt(1, 1)
   && ChkZ80Syntax(eSyntaxZ80))
  {
    z80_adr_vals_t adr_vals;

    switch (decode_z80_adr(&ArgStr[1], &adr_vals, MModReg8 | MModReg16 | MModWA))
    {
      case e_mod_reg8:
        if ((adr_vals.val < REG_A) || (adr_vals.val > REG_C)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
          BAsmCode[CodeLen++] = 0x40 | (code << 4) | adr_vals.val;
        break;
      case e_mod_reg16:
        BAsmCode[CodeLen++] = (adr_vals.val == REG_EA)
                            ? 0xa8 | code
                            : 0x02 | code | (adr_vals.val << 4);
        break;
      case e_mod_wa:
        BAsmCode[CodeLen++] = 0x20 | (code << 4);
        BAsmCode[CodeLen++] = adr_vals.vals[0];
        break;
      default:
        break;
    }
  }
}

static void DecodeEADD_ESUB(Word Code)
{
  Byte HReg;
  decode_reg_res_t res;

  if (!ChkArgCnt(2, 2) || !check_core_and_error(core_mask_7807_7810));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else if ((res = decode_r2(&ArgStr[2], &HReg)) == e_decode_reg_unknown) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
  else if (res == e_decode_reg_ok)
  {
    CodeLen = 2;
    BAsmCode[0] = 0x70;
    BAsmCode[1] = Code + HReg;
  }
}

static void DecodeJ_JR_JRE(Word Type)
{
  Boolean OK;
  Integer AdrInt;
  tSymbolFlags Flags;

  if (!ChkArgCnt(1, 1))
    return;

  AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags) - (EProgCounter() + 1);
  if (!OK)
    return;

  if (!Type) /* generic J */
    Type = RangeCheck(AdrInt, SInt6) ? 1 : 2;

  switch (Type)
  {
    case 1: /* JR */
      if (!mSymbolQuestionable(Flags) && !RangeCheck(AdrInt, SInt6)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 0xc0 + (AdrInt & 0x3f);
      }
      break;
    case 2:
      AdrInt--; /* JRE is 2 bytes long */
      if (!mSymbolQuestionable(Flags) && !RangeCheck(AdrInt, SInt9)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0x4e + (Hi(AdrInt) & 1);
        BAsmCode[1] = Lo(AdrInt);
      }
      break;
  }
}

static void DecodeCALF(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Integer AdrInt;
    tSymbolFlags Flags;

    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (OK)
    {
      if (!mFirstPassUnknown(Flags) && ((AdrInt >> 11) != 1)) WrStrErrorPos(ErrNum_NotFromThisAddress, &ArgStr[1]);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = Hi(AdrInt) + 0x70;
        BAsmCode[1] = Lo(AdrInt);
      }
    }
  }
}

static void DecodeCALT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Integer AdrInt;
    tSymbolFlags Flags;

    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (OK)
    {
      Word AdrMask = is_7807_781x ? 0xffc1 : 0xff81;

      if (!mFirstPassUnknown(Flags) && ((AdrInt & AdrMask) != 0x80)) WrStrErrorPos(ErrNum_NotFromThisAddress, &ArgStr[1]);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 0x80 + ((AdrInt & ~AdrMask) >> 1);
      }
    }
  }
}

static void DecodeBIT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2) && check_core_and_error(core_mask_no_low))
  {
    Boolean OK;
    ShortInt HReg;

    HReg = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
    if (OK)
     if (Decode_wa(&ArgStr[2], BAsmCode + 1, 0xff))
     {
       CodeLen = 2; BAsmCode[0] = 0x58 + HReg;
     }
  }
}

static void DecodeSK_SKN(Word Code)
{
  Byte HReg;

  if (!ChkArgCnt(1, 1) || !check_core_and_error(Hi(Code)));
  else if (Decode_f(ArgStr[1].str.p_str, &HReg))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Lo(Code) + HReg;
  }
  else if (pCurrCPUProps->Core == eCore7807)
    decode_bit_7807_core(1, (Code & 0x10) ? 0x50 :0x5d);
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

static void DecodeSKIT_SKNIT(Word Code)
{
  ShortInt HReg;

  if (ChkArgCnt(1, 1)
   && check_core_and_error(Hi(Code))
   && Decode_irf(&ArgStr[1], &HReg))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Lo(Code) + HReg;
  }
}

static void DecodeIN_OUT(Word Code)
{
  const tStrComp *p_port_arg;

  switch (ArgCnt)
  {
    case 2:
    {
      const tStrComp *p_acc_arg = &ArgStr[(Code & 1) + 1];

      if (as_strcasecmp(p_acc_arg->str.p_str, "A"))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_acc_arg);
        return;
      }
      p_port_arg = &ArgStr[2 - (Code & 1)];
      goto common;
    }
    case 1:
      p_port_arg = &ArgStr[1];
      /* fall-thru */
    common:
    {
      Boolean OK;

      BAsmCode[1] = EvalStrIntExpression(p_port_arg, UInt8, &OK);
      if (OK)
      {
        BAsmCode[0] = Code;
        CodeLen = 2;
      }
      break;
    }
    default:
      (void)ChkArgCnt(1, 2);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBLOCK_7807(Word code)
 * \brief  Handle 7807-style BLOCK instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeBLOCK_7807(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Byte mode;

    if (Decode_rpa(&ArgStr[1], &mode))
    {
      if ((mode != 4) && (mode != 6)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      else
      {
        BAsmCode[0] = code | ((mode >> 1) & 1);
        CodeLen = 1;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBit1_7807(Word code)
 * \brief  handle 7807-specific bit operations with one operand
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeBit1_7807(Word code)
{
  if (!ChkArgCnt(1, 1));
  else if (!check_core_and_error(core_mask_7807));
  else
    decode_bit_7807_core(1, code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBit2_7807(Word code)
 * \brief  handle 7807-specific bit operations with two operands,
           and Z80-style logical instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeBit2_7807(Word code)
{
  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    if (check_core_and_error(core_mask_7807))
      decode_bit_7807_core(2, Lo(code));
  }
  else if (CurrZ80Syntax & eSyntaxZ80)
  {
    Word Flags = ALUReg_Src;
    if (pCurrCPUProps->Core != eCore7800Low)
      Flags |= ALUReg_Dest | ALUImm_SR;
    else if (Hi(code) != 2)
      Flags |= ALUImm_SR;
    decode_alu_z80((Flags << 8) | Hi(code));
  }
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDEFBIT(Word code)
 * \brief  handle DEFBIT instruction (7807...7809 only)
 * ------------------------------------------------------------------------ */

static void DecodeDEFBIT(Word code)
{
  LongWord BitSpec;

  UNUSED(code);

  /* if in structure definition, add special element to structure */

  if (ActPC == StructSeg)
  {
    Boolean OK;
    Byte BitPos;
    PStructElem pElement;

    if (!ChkArgCnt(2, 2))
      return;
    BitPos = eval_bit_position(&ArgStr[2], &OK);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = eSymbolSize8Bit;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = expand_bit_7807;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (decode_bit_arg(&BitSpec, 1, ArgCnt))
    {
      *ListLine = '=';
      dissect_bit_7807(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ld(Word code)
 * \brief  handle LD (Z80 style) instruction
 * ------------------------------------------------------------------------ */

static void decode_ld(Word code)
{
  UNUSED(code);

  if (ChkArgCnt(2, 2)
   && ChkZ80Syntax(eSyntaxZ80))
  {
    z80_adr_vals_t dest_adr_vals, src_adr_vals;

    switch (decode_z80_adr(&ArgStr[1], &dest_adr_vals, MModReg8 | MModReg16 | MModAbs | MModWA | MModIndir | MModSReg8 | MModSReg16))
    {
      case e_mod_reg8:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals,
                               MModReg8
                             | ((dest_adr_vals.val < 8) ? MModImm : 0)
                             | ((dest_adr_vals.val < 8) ? MModAbs : 0)
                             | ((dest_adr_vals.val == REG_A) ? MModWA : 0)
                             | ((dest_adr_vals.val == REG_A) ? MModIndir : 0)
                             | ((dest_adr_vals.val == REG_A) ? MModSReg8 : 0)))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x68 + dest_adr_vals.val;
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            break;
          case e_mod_abs:
            BAsmCode[CodeLen++] = 0x70;
            BAsmCode[CodeLen++] = 0x68 + dest_adr_vals.val;
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            BAsmCode[CodeLen++] = src_adr_vals.vals[1];
            break;
          case e_mod_wa:
            BAsmCode[CodeLen++] = (pCurrCPUProps->Core >= eCore7800High) ? 0x01 : 0x28;
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            break;
          case e_mod_reg8:
            if ((src_adr_vals.val == REG_A) && (dest_adr_vals.val >= 2))
              BAsmCode[CodeLen++] = 0x18 | (dest_adr_vals.val & 7);
            else if ((dest_adr_vals.val == REG_A) && (src_adr_vals.val >= 2))
              BAsmCode[CodeLen++] = 0x08 | (src_adr_vals.val & 7);
            else
              WrError(ErrNum_InvAddrMode);
            break;
          case e_mod_indir:
            if ((pCurrCPUProps->Core <= eCore7800High) && !is_rpa(src_adr_vals.val)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x28 | (src_adr_vals.val & 0x07) | ((src_adr_vals.val & 0x08) << 4);
              append_adr_vals(&src_adr_vals);
            }
            break;
          case e_mod_sreg8:
            if (check_sr_flag(src_adr_vals.vals[0], eFlagSR1, &ArgStr[2]))
            {
              BAsmCode[CodeLen++] = 0x4c;
              BAsmCode[CodeLen++] = 0xc0 | src_adr_vals.val;
            }
            break;
          default:
            break;
        }
        break;
      case e_mod_reg16:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, 
                               MModImm
                             | MModReg16
                             | ((dest_adr_vals.val < REG_EA) ? MModAbs : 0)
                             | ((dest_adr_vals.val == REG_EA) ? MModIndir : 0)
                             | ((dest_adr_vals.val == REG_EA) ? MModSReg16 : 0)))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x04 | (dest_adr_vals.val << 4);
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            BAsmCode[CodeLen++] = src_adr_vals.vals[1];
            break;
          case e_mod_reg16:
            if ((dest_adr_vals.val == REG_EA) && (src_adr_vals.val >= REG_BC) && (src_adr_vals.val <= REG_HL))
              BAsmCode[CodeLen++] = 0xa4 | src_adr_vals.val;
            else if ((src_adr_vals.val == REG_EA) && (dest_adr_vals.val >= REG_BC) && (dest_adr_vals.val <= REG_HL))
              BAsmCode[CodeLen++] = 0xb4 | dest_adr_vals.val;
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          case e_mod_abs:
            BAsmCode[CodeLen++] = 0x70;
            BAsmCode[CodeLen++] = 0x0f | (dest_adr_vals.val << 4);
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            BAsmCode[CodeLen++] = src_adr_vals.vals[1];
            break;
          case e_mod_indir:
            if (!is_rpa3(src_adr_vals.val)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x48;
              BAsmCode[CodeLen++] = 0x80 | src_adr_vals.val;
              append_adr_vals(&src_adr_vals);
            }
            break;
          case e_mod_sreg16:
            if (check_sr_flag(src_adr_vals.vals[0], eFlagSR4, &ArgStr[2]))
            {
              BAsmCode[CodeLen++] = 0x48;
              BAsmCode[CodeLen++] = 0xc0 | src_adr_vals.val;
            }
            break;
          default:
            break;
        }
        break;
      case e_mod_abs:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, MModReg8 | MModReg16))
        {
          case e_mod_reg8:
            if (src_adr_vals.val > 7) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x70;
              BAsmCode[CodeLen++] = 0x78 | src_adr_vals.val;
              append_adr_vals(&dest_adr_vals);
            }
            break;
          case e_mod_reg16:
            if (src_adr_vals.val >= REG_EA) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x70;
              BAsmCode[CodeLen++] = 0x0e | (src_adr_vals.val << 4);
              append_adr_vals(&dest_adr_vals);
            }
            break;
          default:
            break;
        }
        break;
      case e_mod_wa:
        z80_def_op_size = eSymbolSize8Bit;
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, MModReg8 | MModReg16
                               | ((pCurrCPUProps->Core >= eCore7800High) ? MModImm : 0)
                              ))
        {
          case e_mod_reg8:
            if (src_adr_vals.val == REG_A)
            {
              BAsmCode[CodeLen++] = (pCurrCPUProps->Core >= eCore7800High) ? 0x63 : 0x38;
              BAsmCode[CodeLen++] = dest_adr_vals.vals[0];
            }
            else if (src_adr_vals.val <= 7) /* map to absolute addressing */
            {
              BAsmCode[CodeLen++] = 0x70;
              BAsmCode[CodeLen++] = 0x78 | src_adr_vals.val;
              BAsmCode[CodeLen++] = dest_adr_vals.vals[0];
              BAsmCode[CodeLen++] = WorkArea;
            }
            else
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            break;
          case e_mod_reg16: /* map to absolute addressing */
            if (src_adr_vals.val >= REG_EA) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else
            {
              BAsmCode[CodeLen++] = 0x70;
              BAsmCode[CodeLen++] = 0x0e | (src_adr_vals.val << 4);
              BAsmCode[CodeLen++] = dest_adr_vals.vals[0];
              BAsmCode[CodeLen++] = WorkArea;
            }
            break;
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x71;
            BAsmCode[CodeLen++] = dest_adr_vals.vals[0];
            BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            break;
          default:
            break;
        }
        break;
      case e_mod_indir:
        z80_def_op_size = eSymbolSize8Bit;
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, MModReg8 | MModReg16
                            | ((pCurrCPUProps->Core >= eCore7800High) ? MModImm : 0)))
        {
          case e_mod_reg8:
            if (src_adr_vals.val != REG_A) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if ((pCurrCPUProps->Core <= eCore7800High) && !is_rpa(dest_adr_vals.val)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
            else
            {
              BAsmCode[CodeLen++] = 0x38 | (dest_adr_vals.val & 0x07) | ((dest_adr_vals.val & 0x08) << 4);
              append_adr_vals(&dest_adr_vals);
            }
            break;
          case e_mod_reg16:
            if (src_adr_vals.val != REG_EA) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (!is_rpa3(dest_adr_vals.val)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
            else
            {
              BAsmCode[CodeLen++] = 0x48;
              BAsmCode[CodeLen++] = 0x90 | dest_adr_vals.val;
              append_adr_vals(&dest_adr_vals);
            }
            break;
          case e_mod_imm:
            if (!is_rpa1(dest_adr_vals.val)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
            else
            {
              BAsmCode[CodeLen++] = 0x48 | dest_adr_vals.val;
              BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            }
            break;
          default:
            break;
        }
        break;
      case e_mod_sreg8:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, MModReg8 | MModImm))
        {
          case e_mod_reg8:
            if (src_adr_vals.val != REG_A) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (check_sr_flag(dest_adr_vals.vals[0], eFlagSR, &ArgStr[1]))
            {
              BAsmCode[CodeLen++] = 0x4d;
              BAsmCode[CodeLen++] = 0xc0 | dest_adr_vals.val;
            }
            break;
          case e_mod_imm:
            if (check_sr_flag(dest_adr_vals.vals[0], eFlagSR2, &ArgStr[1]))
            {
              BAsmCode[CodeLen++] = 0x64;
              BAsmCode[CodeLen++] = (dest_adr_vals.val & 0x07) | ((dest_adr_vals.val & 0x08) << 4);
              BAsmCode[CodeLen++] = src_adr_vals.vals[0];
            }
          default:
            break;
        }
        break;
      case e_mod_sreg16:
        switch (decode_z80_adr(&ArgStr[2], &src_adr_vals, MModReg16))
        {
          case e_mod_reg16:
            if (src_adr_vals.val != REG_EA) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
            else if (check_sr_flag(dest_adr_vals.vals[0], eFlagSR3, &ArgStr[1]))
            {
              BAsmCode[CodeLen++] = 0x48;
              BAsmCode[CodeLen++] = 0xd2 | dest_adr_vals.val;
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
}

/*--------------------------------------------------------------------------------*/
/* Dynamic Code Table Handling */

static void AddFixed(const char *p_name, Word code, unsigned core_mask)
{
  order_array_rsv_end(fixed_orders, order_t);
  fixed_orders[InstrZ].code = code;
  fixed_orders[InstrZ].core_mask = core_mask;
  AddInstTable(InstTable, p_name, InstrZ++, DecodeFixed);
}

static void AddIntFlag(const char *p_name, Byte code)
{
  order_array_rsv_end(int_flags, intflag_t);
  int_flags[InstrZ].p_name = p_name;
  int_flags[InstrZ++].code = code;
}

static void AddALU(Byte NCode, Word Flags, const char *NNameI, const char *NNameReg, const char *NNameEA, const char *NNameZ80)
{
  char Name[20];

  AddInstTable(InstTable, NNameI, ((Flags & ALUImm_SR) << 8) | NCode, DecodeALUImm);
  AddInstTable(InstTable, NNameReg, ((Flags & (ALUReg_Src | ALUReg_Dest | ALUImm_SR | ALUReg_MayZ80)) << 8) | NCode, DecodeALUReg);
  AddInstTable(InstTable, NNameEA, NCode, DecodeALUEA);
  as_snprintf(Name, sizeof(Name), "%sW", NNameReg);
  AddInstTable(InstTable, Name, NCode, DecodeALURegW);
  as_snprintf(Name, sizeof(Name), "%sX", NNameReg);
  AddInstTable(InstTable, Name, NCode, DecodeALURegX);
  if (NCode & 1)
  {
    as_snprintf(Name, sizeof(Name), "%sW", NNameI);
    AddInstTable(InstTable, Name, NCode, DecodeALUImmW);
  }
  if (NNameZ80)
    AddInstTable(InstTable, NNameZ80, NCode, decode_alu_z80);
}

static void add_alu_z80(const char *p_name, Byte code, Word flags)
{
  AddInstTable(InstTable, p_name, (flags << 8) | code, decode_alu_z80);
}

static void AddAbs(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAbs);
}

static void AddReg2(const char *p_name, Word code, unsigned core_mask)
{
  order_array_rsv_end(reg2_orders, order_t);
  reg2_orders[InstrZ].code = code;
  reg2_orders[InstrZ].core_mask = core_mask;
  AddInstTable(InstTable, p_name, InstrZ++, DecodeReg2);
}

static void add_sreg8(const char *p_name, Byte code, Byte flags, Byte core_mask)
{
  order_array_rsv_end(s_regs8, sreg_t);
  s_regs8[InstrZ].p_name = p_name;
  s_regs8[InstrZ].code = code;
  s_regs8[InstrZ].flags = flags;
  s_regs8[InstrZ].core_mask = core_mask;
  InstrZ++;
}

static void add_sreg16(const char *p_name, Byte code, Byte flags, Byte core_mask)
{
  order_array_rsv_end(s_regs16, sreg_t);
  s_regs16[InstrZ].p_name = p_name;
  s_regs16[InstrZ].code = code;
  s_regs16[InstrZ].flags = flags;
  s_regs16[InstrZ].core_mask = core_mask;
  InstrZ++;
}

static void AddWork(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeWork);
}

static void AddA(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeA);
}

static void AddEA(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeEA);
}

static void InitFields(void)
{
  Boolean IsLow = pCurrCPUProps->Core == eCore7800Low,
          IsHigh = pCurrCPUProps->Core == eCore7800High,
          Is7807 = pCurrCPUProps->Core == eCore7807,
          Is781x = pCurrCPUProps->Core == eCore7810;

  InstTable = CreateInstTable(301);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MVI", 0, DecodeMVI);
  AddInstTable(InstTable, "MVIW", 0, DecodeMVIW);
  AddInstTable(InstTable, "MVIX", 0, DecodeMVIX);
  AddInstTable(InstTable, "LDAX", 0x28, DecodeLDAX_STAX);
  AddInstTable(InstTable, "STAX", 0x38, DecodeLDAX_STAX);
  AddInstTable(InstTable, "LDEAX", 0x80, DecodeLDEAX_STEAX);
  AddInstTable(InstTable, "STEAX", 0x90, DecodeLDEAX_STEAX);
  AddInstTable(InstTable, "LXI", 0, DecodeLXI);
  AddInstTable(InstTable, "PUSH", is_7807_781x ? 0xb0 : 0x480e, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", is_7807_781x ? 0xa0 : 0x480f, DecodePUSH_POP);
  AddInstTable(InstTable, "DMOV", 0, DecodeDMOV);
  AddInstTable(InstTable, "DCX", 1, DecodeDCX_INX);
  AddInstTable(InstTable, "INX", 0, DecodeDCX_INX);
  AddInstTable(InstTable, "EADD", 0x40, DecodeEADD_ESUB);
  AddInstTable(InstTable, "ESUB", 0x60, DecodeEADD_ESUB);
  AddInstTable(InstTable, "JR", 1, DecodeJ_JR_JRE);
  AddInstTable(InstTable, "JRE", 2, DecodeJ_JR_JRE);
  AddInstTable(InstTable, "J", 0, DecodeJ_JR_JRE);
  AddInstTable(InstTable, "CALF", 0, DecodeCALF);
  AddInstTable(InstTable, "CALT", 0, DecodeCALT);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
  AddInstTable(InstTable, "SK" , (core_mask_no_low << 8) | 0x08, DecodeSK_SKN);
  AddInstTable(InstTable, "SKN", (core_mask_all    << 8) | 0x18, DecodeSK_SKN);
  AddInstTable(InstTable, "SKIT" , (core_mask_no_low << 8) | (is_7807_781x ? 0x40 : 0x00), DecodeSKIT_SKNIT);
  AddInstTable(InstTable, "SKNIT", (core_mask_all    << 8) | (is_7807_781x ? 0x60 : 0x10), DecodeSKIT_SKNIT);

  InstrZ = 0;
  AddFixed("EX"   , 0x0010                   , (1 << eCore7800High));
  AddFixed("PEN"  , 0x482c                   , (1 << eCore7800High));
  AddFixed("RCL"  , 0x4832                   , (1 << eCore7800High));
  AddFixed("RCR"  , 0x4833                   , (1 << eCore7800High));
  AddFixed("SHAL" , 0x4834                   , (1 << eCore7800High));
  AddFixed("SHAR" , 0x4835                   , (1 << eCore7800High));
  AddFixed("SHCL" , 0x4836                   , (1 << eCore7800High));
  AddFixed("SHCR" , 0x4837                   , (1 << eCore7800High));
  AddFixed("EXX"  , Is7807 ? 0x48af : 0x0011 , core_mask_no_low);
  AddFixed("EXA"  , Is7807 ? 0x48ac : 0x0010 , core_mask_all);
  AddFixed("EXH"  , Is7807 ? 0x48ae : 0x0050 , core_mask_all);
  AddFixed("EXR"  , 0x48ad                   , core_mask_7807);
  if (Is7807)
    AddInstTable(InstTable, "BLOCK", 0x0010, DecodeBLOCK_7807);
  else
    AddFixed("BLOCK", 0x0031                   , core_mask_no_low);
  AddFixed("TABLE", is_7807_781x ? 0x48a8 : 0x0021 , core_mask_no_low);
  AddFixed("DAA"  , 0x0061                   , core_mask_all);
  AddFixed("STC"  , 0x482b                   , core_mask_all);
  AddFixed("CLC"  , 0x482a                   , core_mask_all);
  AddFixed("CMC"  , 0x48aa                   , core_mask_7807);
  AddFixed("NEGA" , 0x483a                   , core_mask_all);
  AddFixed("RLD"  , 0x4838                   , core_mask_all);
  AddFixed("RRD"  , 0x4839                   , core_mask_all);
  AddFixed("JB"   , is_7807_781x ? 0x0021 : 0x0073 , core_mask_all);
  AddFixed("JEA"  , 0x4828                   , core_mask_all);
  AddFixed("CALB" , is_7807_781x ? 0x4829 : 0x0063 , core_mask_no_low);
  AddFixed("SOFTI", 0x0072                   , core_mask_no_low);
  AddFixed("RET"  , is_7807_781x ? 0x00b8 : 0x0008 , core_mask_all);
  AddFixed("RETS" , is_7807_781x ? 0x00b9 : 0x0018 , core_mask_all);
  AddFixed("RETI" , 0x0062                   , core_mask_all);
  AddFixed("NOP"  , 0x0000                   , core_mask_all);
  AddFixed("EI"   , is_7807_781x ? 0x00aa : 0x4820 , core_mask_all);
  AddFixed("DI"   , is_7807_781x ? 0x00ba : 0x4824 , core_mask_all);
  AddFixed("HLT"  , is_7807_781x ? 0x483b : 0x0001 , core_mask_no_low);
  AddFixed("SIO"  , 0x0009                   , core_mask_7800);
  AddFixed("STM"  , 0x0019                   , core_mask_7800);
  AddFixed("PEX"  , 0x482d                   , core_mask_7800);
  AddFixed("RAL"  , is_7807_781x ? 0x4835 : 0x4830 , core_mask_all);
  AddFixed("RAR"  , 0x4831                   , core_mask_all);
  AddFixed("PER"  , 0x483c                   , core_mask_7800);
  if (pCurrCPUProps->Flags & eFlagCMOS)
    AddFixed("STOP" , 0x48bb, core_mask_all);

  if (is_7807_781x)
  {
    if (Is781x)
    {
    }
    else
    {
    }
    /* 0x28 eFlagSR ? */
  }
  else
  {
  }

  InstrZ = 0;
  if (is_7807_781x)
  {
    AddIntFlag("NMI" , 0);
    AddIntFlag("FT0" , 1);
    AddIntFlag("FT1" , 2);
    AddIntFlag("F1"  , 3);
    AddIntFlag("F2"  , 4);
    AddIntFlag("FE0" , 5);
    AddIntFlag("FE1" , 6);
    AddIntFlag("FEIN", 7);
    AddIntFlag("FSR" , 9);
    AddIntFlag("FST" ,10);
    AddIntFlag("ER"  ,11);
    AddIntFlag("OV"  ,12);
    AddIntFlag("SB"  ,20);
    if (Is781x)
    {
      AddIntFlag("FAD" , 8);
      AddIntFlag("AN4" ,16);
      AddIntFlag("AN5" ,17);
      AddIntFlag("AN6" ,18);
      AddIntFlag("AN7" ,19);
    }
    else
    {
      /* value yet unknown */
      /* AddIntFlag("IFE2", ...); */
    }
  }
  else
  {
    AddIntFlag("F0"  , 0);
    AddIntFlag("FT"  , 1);
    AddIntFlag("F1"  , 2);
    if (IsHigh)
      AddIntFlag("F2"  , 3);
    AddIntFlag("FS"  , 4);
  }
  AddIntFlag(NULL, 0);

  AddALU(10, IsLow ?             ALUReg_Src | ALUReg_MayZ80 : ALUImm_SR | ALUReg_Src | ALUReg_Dest | ALUReg_MayZ80, "ACI"  , "ADC"  , "DADC"  , NULL );
  AddALU( 4, IsLow ?             ALUReg_Src | ALUReg_MayZ80 : ALUImm_SR | ALUReg_Src | ALUReg_Dest | ALUReg_MayZ80, "ADINC", "ADDNC", "DADDNC", NULL );
  AddALU( 8, IsLow ?             ALUReg_Src | ALUReg_MayZ80 : ALUImm_SR | ALUReg_Src | ALUReg_Dest | ALUReg_MayZ80, "ADI"  , "ADD"  , "DADD"  , NULL );
  AddALU( 1, IsLow ? ALUImm_SR | ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "ANI"  , "ANA"  , "DAN"   , NULL );
  AddALU(15, IsLow ?             ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "EQI"  , "EQA"  , "DEQ"   , NULL );
  AddALU( 5, IsLow ?             ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "GTI"  , "GTA"  , "DGT"   , NULL );
  AddALU( 7, IsLow ?             ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "LTI"  , "LTA"  , "DLT"   , NULL );
  AddALU(13, IsLow ?             ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "NEI"  , "NEA"  , "DNE"   , NULL );
  AddALU(11, IsLow ? ALUImm_SR                              : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "OFFI" , "OFFA" , "DOFF"  , NULL );
  AddALU( 9, IsLow ? ALUImm_SR                              : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "ONI"  , "ONA"  , "DON"   , NULL );
  AddALU( 3, IsLow ? ALUImm_SR | ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "ORI"  , "ORA"  , "DOR"   , NULL );
  AddALU(14, IsLow ?             ALUReg_Src | ALUReg_MayZ80 : ALUImm_SR | ALUReg_Src | ALUReg_Dest | ALUReg_MayZ80, "SBI"  , "SBB"  , "DSBB"  , NULL );
  AddALU( 6, IsLow ?             ALUReg_Src | ALUReg_MayZ80 : ALUImm_SR | ALUReg_Src | ALUReg_Dest | ALUReg_MayZ80, "SUINB", "SUBNB", "DSUBNB", NULL );
  AddALU(12, IsLow ?             ALUReg_Src | ALUReg_MayZ80 : ALUImm_SR | ALUReg_Src | ALUReg_Dest | ALUReg_MayZ80, "SUI"  , "SUB"  , "DSUB"  , NULL );
  AddALU( 2, IsLow ?             ALUReg_Src                 : ALUImm_SR | ALUReg_Src | ALUReg_Dest                , "XRI"  , "XRA"  , "DXR"   , NULL );

  AddAbs("CALL", is_7807_781x ? 0x0040 : 0x0044);
  AddAbs("JMP" , 0x0054);
  AddAbs("LBCD", 0x701f);
  AddAbs("LDED", 0x702f);
  AddAbs("LHLD", 0x703f);
  AddAbs("LSPD", 0x700f);
  AddAbs("SBCD", 0x701e);
  AddAbs("SDED", 0x702e);
  AddAbs("SHLD", 0x703e);
  AddAbs("SSPD", 0x700e);

  InstrZ = 0;
  AddReg2("DCR" , 0x0050, core_mask_all);
  AddReg2("DIV" , 0x483c, core_mask_7807_7810);
  AddReg2("INR" , 0x0040, core_mask_all);
  AddReg2("MUL" , 0x482c, core_mask_7807_7810);
  if (is_7807_781x)
  {
    AddReg2("RLL" , 0x4834, core_mask_7807_7810);
    AddReg2("RLR" , 0x4830, core_mask_7807_7810);
  }
  else
  {
    AddA("RLL", 0x4830);
    AddA("RLR", 0x4831);
  }
  AddReg2("SLL" , 0x4824, core_mask_7807_7810);
  AddReg2("SLR" , 0x4820, core_mask_7807_7810);
  AddReg2("SLLC", 0x4804, core_mask_7807_7810);
  AddReg2("SLRC", 0x4800, core_mask_7807_7810);

  AddWork("DCRW", 0x30);
  AddWork("INRW", 0x20);
  AddWork("LDAW", is_7807_781x ? 0x01 : 0x28);
  AddWork("STAW", is_7807_781x ? 0x63 : 0x38);

  AddEA("DRLL", 0x48b4); AddEA("DRLR", 0x48b0);
  AddEA("DSLL", 0x48a4); AddEA("DSLR", 0x48a0);

  if (!is_7807_781x)
  {
    AddInstTable(InstTable, "IN" , 0x4c, DecodeIN_OUT);
    AddInstTable(InstTable, "OUT", 0x4d, DecodeIN_OUT);
  }

  AddInstTable(InstTable, "AND" , 0x0131, DecodeBit2_7807);
  AddInstTable(InstTable, "OR"  , 0x035c, DecodeBit2_7807);
  AddInstTable(InstTable, "XOR" , 0x025e, DecodeBit2_7807);
  AddInstTable(InstTable, "SETB", 0x58, DecodeBit1_7807);
  AddInstTable(InstTable, "CLR" , 0x5b, DecodeBit1_7807);
  AddInstTable(InstTable, "NOT" , 0x59, DecodeBit1_7807);

  /* Array with special regs is created upon first usage and remains
     allocated, because we need it in dissect_bit_7807(), which may
     be called after we switched away fro mtarget (e.g. when printing
     symbol table: */

  if (!s_regs8)
  {
    InstrZ = 0;
    add_sreg8("PA"  , 0x00, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_all);
    add_sreg8("PB"  , 0x01, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_all);
    add_sreg8("PC"  , 0x02, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("PD"  , 0x03, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("PF"  , 0x05, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("MKH" , 0x06, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("MKL" , 0x07, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("SMH" , 0x09, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("SML" , 0x0a, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("EOM" , 0x0b, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("ETMM", 0x0c, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("TMM" , 0x0d, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807_7810);
    add_sreg8("MM"  , 0x10, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("MCC" , 0x11, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("MA"  , 0x12, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("MB"  , 0x13, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("MC"  , 0x14, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("MF"  , 0x17, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("TXB" , 0x18, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("RXB" , 0x19, eFlagSR1                     , core_mask_7807_7810);
    add_sreg8("TM0" , 0x1a, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("TM1" , 0x1b, eFlagSR                      , core_mask_7807_7810);
    add_sreg8("ZCM" , 0x28, eFlagSR                      , core_mask_7807_7810 | core_mask_cmos);
    add_sreg8("ANM" , 0x08, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7810     );
    add_sreg8("CR0" , 0x20, eFlagSR1                     , core_mask_7810     );
    add_sreg8("CR1" , 0x21, eFlagSR1                     , core_mask_7810     );
    add_sreg8("CR2" , 0x22, eFlagSR1                     , core_mask_7810     );
    add_sreg8("CR3" , 0x23, eFlagSR1                     , core_mask_7810     );
    add_sreg8("PT"  , 0x0e, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7807     );
    add_sreg8("WDM" , 0x20, eFlagSR | eFlagSR1           , core_mask_7807     );
    add_sreg8("MT"  , 0x21, eFlagSR | eFlagSR1           , core_mask_7807     );
    add_sreg8("MK"  , 0x03, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7800     );
    add_sreg8("MB"  , 0x04, eFlagSR                      , core_mask_7800     );
    add_sreg8("PC"  , 0x02, eFlagSR | eFlagSR1 | eFlagSR2, core_mask_7800_high);
    add_sreg8("PC"  , 0x02, eFlagSR1 | eFlagSR2          , core_mask_7800_low );
    add_sreg8("MC"  , 0x05, eFlagSR                      , core_mask_7800_high);
    add_sreg8("MC"  , 0x05, 0                            , core_mask_7800_low );
    add_sreg8("TM0" , 0x06, eFlagSR                      , core_mask_7800_high);
    add_sreg8("TM0" , 0x06, 0                            , core_mask_7800_low );
    add_sreg8("TM1" , 0x07, eFlagSR                      , core_mask_7800_high);
    add_sreg8("TM1" , 0x07, 0                            , core_mask_7800_low );
    add_sreg8("TM"  , 0x06, eFlagSR                      , core_mask_7800_low );
    add_sreg8("TM"  , 0x06, 0                            , core_mask_7800_high);
    add_sreg8("SM"  , 0x0a, eFlagSR | eFlagSR1           , core_mask_7800_low );
    add_sreg8("SM"  , 0x0a, 0                            , core_mask_7800_high);
    add_sreg8("SC"  , 0x0b, eFlagSR | eFlagSR1           , core_mask_7800_low );
    add_sreg8("SC"  , 0x0b, 0                            , core_mask_7800_high);
    add_sreg8("S"   , 0x08, eFlagSR | eFlagSR1           , core_mask_7800     );
    add_sreg8("TMM" , 0x09, eFlagSR | eFlagSR1           , core_mask_7800     );
    add_sreg8(NULL  , 0x00, 0                            , 0                  );
  }

  InstrZ = 0;
  add_sreg16("ETM0" , 0x00, eFlagSR3, core_mask_all );
  add_sreg16("ETM1" , 0x01, eFlagSR3, core_mask_all );
  add_sreg16("ECNT" , 0x00, eFlagSR4, core_mask_all );
  add_sreg16("ECPT" , 0x01, eFlagSR4, core_mask_7810);
  add_sreg16("ECPT0", 0x01, eFlagSR4, core_mask_7807);
  add_sreg16("ECPT1", 0x02, eFlagSR4, core_mask_7807);
  add_sreg16(NULL   , 0x00, 0       , 0             );

  if (Is7807)
    AddInstTable(InstTable, "DEFBIT", 0, DecodeDEFBIT);

  AddZ80Syntax(InstTable);
  AddInstTable(InstTable, "LD", 0, decode_ld);
  AddInstTable(InstTable, "INC", 0, decode_inc_dec);
  AddInstTable(InstTable, "DEC", 1, decode_inc_dec);
  add_alu_z80("SKGT", 5, IsLow ? ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest);
  add_alu_z80("SKLT", 7, IsLow ? ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest);
  add_alu_z80("SKNE",13, IsLow ? ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest);
  add_alu_z80("SKEQ",15, IsLow ? ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest);
  add_alu_z80("SKON", 9, IsLow ? ALUImm_SR  : ALUImm_SR | ALUReg_Src | ALUReg_Dest);
  add_alu_z80("SKOFF",11, IsLow ? ALUImm_SR  : ALUImm_SR | ALUReg_Src | ALUReg_Dest);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(int_flags);
  order_array_free(fixed_orders);
  order_array_free(reg2_orders);
  /* See above why s_regs8 does not get freed upon deinit: */
  /*order_array_free(s_regs8);*/
  order_array_free(s_regs16);
}

/*--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     MakeCode_78C10(void)
 * \brief  generate code from source code line
 * ------------------------------------------------------------------------ */

static void MakeCode_78C10(void)
{
  CodeLen = 0;
  DontPrint = False;
  z80_op_size =
  z80_def_op_size = eSymbolSizeUnknown;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     InitCode_78C10(void)
 * \brief  target-specific initializations at begin of pass
 * ------------------------------------------------------------------------ */

static void InitCode_78C10(void)
{
  WorkArea = 0x100;
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_78C10(void)
 * \brief  check whether instruction consumes label field itself
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsDef_78C10(void)
{
  return (pCurrCPUProps->Core == eCore7807) && Memo("DEFBIT");
}

/*!------------------------------------------------------------------------
 * \fn     SwitchFrom_78C10(void)
 * \brief  cleanups when switching away from target
 * ------------------------------------------------------------------------ */

static void SwitchFrom_78C10(void)
{
  DeinitFields();
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_78C10(void *pUser)
 * \brief  initializations when switching to target
 * \param  pUser * to target details
 * ------------------------------------------------------------------------ */

static void SwitchTo_78C10(void *pUser)
{
  const TFamilyDescr *pDescr = FindFamilyByName("78(C)xx");

  pCurrCPUProps = (const tCPUProps*)pUser;
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = pDescr->Id; NOPCode = 0x00;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  if (pCurrCPUProps->Flags & eFlagHasV)
  {
    static ASSUMERec ASSUME78C10s[] =
    {
      {"V" , &WorkArea, 0, 0xff, 0x100, NULL}
    };

    pASSUMERecs = ASSUME78C10s;
    ASSUMERecCnt = sizeof(ASSUME78C10s) / sizeof(*ASSUME78C10s);
  }
  else
    WorkArea = 0xff;
  is_7807_781x = (pCurrCPUProps->Core == eCore7807) || (pCurrCPUProps->Core == eCore7810);

  MakeCode = MakeCode_78C10; IsDef = IsDef_78C10;
  SwitchFrom = SwitchFrom_78C10;
  if (pCurrCPUProps->Core == eCore7807)
    DissectBit = dissect_bit_7807;
  InitFields();
}

/*!------------------------------------------------------------------------
 * \fn     code78c10_init(void)
 * \brief  Attach 78Cxx target
 * ------------------------------------------------------------------------ */

static const tCPUProps CPUProps[] =
{
  { "7800" , eCore7800High, eFlagHasV             }, /* ROMless , 128B RAM */
  { "7801" , eCore7800High, eFlagHasV             }, /* 4KB ROM , 128B RAM */
  { "7802" , eCore7800High, eFlagHasV             }, /* 6KB ROM , 128B RAM */
  { "78C05", eCore7800Low,  0                     }, /* ROMless , 128B RAM */
  { "78C06", eCore7800Low,  0                     }, /* 4KB ROM , 128B RAM */
  { "7807" , eCore7807,     eFlagHasV             }, /* ROMless , 256B RAM */
  { "7808" , eCore7807,     eFlagHasV             }, /* 4KB ROM , 256B RAM */
  { "7809" , eCore7807,     eFlagHasV             }, /* 8KB ROM , 256B RAM */
  { "7810" , eCore7810,     eFlagHasV             }, /* ROMless , 256B RAM */
  { "78C10", eCore7810,     eFlagHasV | eFlagCMOS }, /* ROMless , 256B RAM */
  { "78C11", eCore7810,     eFlagHasV | eFlagCMOS }, /* 4KB ROM , 256B RAM */
  { "78C12", eCore7810,     eFlagHasV | eFlagCMOS }, /* 8KB ROM , 256B RAM */
  { "78C14", eCore7810,     eFlagHasV | eFlagCMOS }, /* 16KB ROM, 256B RAM */
  { "78C17", eCore7810,     eFlagHasV | eFlagCMOS }, /* ROMless ,  1KB RAM */
  { "78C18", eCore7810,     eFlagHasV | eFlagCMOS }, /* 32KB ROM,  1KB RAM */
  { ""     , eCoreNone,     0                     },
};

void code78c10_init(void)
{
  const tCPUProps *pProp;

  for (pProp = CPUProps; pProp->Name[0]; pProp++)
    (void)AddCPUUserWithArgs(pProp->Name, SwitchTo_78C10, (void*)pProp, NULL, NULL);

  AddInitPassProc(InitCode_78C10);
}
