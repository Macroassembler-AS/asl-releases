/* code42c00.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Target Toshiba TLCS-42                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmerr.h"
#include "asmitree.h"
#include "headids.h"
#include "cpulist.h"
#include "codevars.h"
#include "intpseudo.h"
#include "codepseudo.h"
#include "code42c00.h"

typedef enum
{
  e_cpu_flag_none = 0,
  e_cpu_flag_cmos = 1 << 0,
  e_cpu_flag_rom_1k = 1 << 1,
  e_cpu_flag_io_23 = 1 << 2
} cpu_flags_t;

#ifdef __cplusplus
# include "code42c00.hpp"
#endif

typedef struct
{
  char name[7];
  Byte flags;
} cpu_props_t;

typedef enum
{
  e_mod_none,
  e_mod_a,
  e_mod_h,
  e_mod_l,
  e_mod_mbr,
  e_mod_d,
  e_mod_p,
  e_mod_cf,
  e_mod_ihl,
  e_mod_idc,
  e_mod_imm,
  e_mod_abs,
  e_mod_port
} adr_mode_t;

#define MModA (1 << e_mod_a)
#define MModH (1 << e_mod_h)
#define MModL (1 << e_mod_l)
#define MModMBR (1 << e_mod_mbr)
#define MModD (1 << e_mod_d)
#define MModP (1 << e_mod_p)
#define MModCF (1 << e_mod_cf)
#define MModIHL (1 << e_mod_ihl)
#define MModIDC (1 << e_mod_idc)
#define MModImm (1 << e_mod_imm)
#define MModAbs (1 << e_mod_abs)
#define MModPort (1 << e_mod_port)

typedef struct
{
  adr_mode_t mode;
  Byte value;
} adr_vals_t;

static const cpu_props_t *p_curr_cpu_props;

/*!------------------------------------------------------------------------
 * \fn     decode_adr(adr_vals_t *p_vals, const tStrComp *p_arg, Word mode_mask)
 * \brief  decode operand/addressing mode expreesion
 * \param  p_vals destination for decoded expression
 * \param  p_arg source argument
 * \param  mode_mask bit mask of allowed modes
 * \return deduced addressing mode
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_mod_none;
  p_vals->value = 0;
}

static adr_mode_t decode_adr(adr_vals_t *p_vals, const tStrComp *p_arg, Word mode_mask)
{
  static const char mode_strings[][4] =
  {
    "A", "H", "L", "MBR", "D", "P", "CF", "@HL", "@DC"
  };
  tEvalResult eval_result;

  reset_adr_vals(p_vals);

  switch (*p_arg->str.p_str)
  {
    case '#':
      p_vals->value = EvalStrIntExpressionOffsWithResult(p_arg, 1, Int4, &eval_result);
      if (eval_result.OK)
      {
        p_vals->mode = e_mod_imm;
        goto check;
      }
      break;
    case '%':
      p_vals->value = EvalStrIntExpressionOffsWithResult(p_arg, 1, UInt3, &eval_result);
      if (!eval_result.OK);
      else if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && (p_vals->value > SegLimits[SegIO]))
        WrStrErrorPos(ErrNum_OverRange, p_arg);
      else
      {
        ChkSpace(SegIO, eval_result.AddrSpaceMask);
        p_vals->mode = e_mod_port;
        goto check;
      }
      break;
    default:
    {
      size_t z;
      for (z = 0; z < as_array_size(mode_strings); z++)
      {
        /* P should be available as ordinary symbol on non-CMOS variants */
        if (!(p_curr_cpu_props->flags & e_cpu_flag_cmos) && ((z + 1) == e_mod_p))
          continue;
        if (!as_strcasecmp(p_arg->str.p_str, mode_strings[z]))
        {
          p_vals->mode = (adr_mode_t) (z + 1);
          goto check;
        }
      }
      /* This assures that we get the same error message (invalid addressing mode)
         in pass 1 and pass 2: */
      if ((mode_mask >> e_mod_abs) & 1)
      {
        p_vals->value = EvalStrIntExpressionWithResult(p_arg, UInt3, &eval_result);
        if (eval_result.OK)
        {
          ChkSpace(SegData, eval_result.AddrSpaceMask);
          p_vals->mode = e_mod_abs;
        }
      }
      else
      {
        /* will trigger error below: */
        p_vals->mode = e_mod_abs;
      }
      goto check;
    }
  }

check:
  if (p_vals->mode && !((mode_mask >> p_vals->mode) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    reset_adr_vals(p_vals);
  }
  return p_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     eval_code_address(tEvalResult *p_eval_result, IntType range)
 * \brief  evaluate address in code space
 * \param  p_eval_result sideband evaluation results
 * \param  range address range
 * \return address value
 * ------------------------------------------------------------------------ */

static Word eval_code_address(tEvalResult *p_eval_result, IntType range)
{
  Word ret = EvalStrIntExpressionWithResult(&ArgStr[1], range, p_eval_result);
  if (p_eval_result->OK)
    ChkSpace(SegCode, p_eval_result->Flags);
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_ld(Word code)
 * \brief  handle LD instruction
 * ------------------------------------------------------------------------ */

static void decode_ld(Word code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&dest_adr_vals, &ArgStr[1], MModA | MModL | MModMBR))
    {
      case e_mod_a:
        switch (decode_adr(&src_adr_vals, &ArgStr[2], MModImm | MModAbs | MModIHL))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x10 | src_adr_vals.value;
            break;
          case e_mod_abs:
            BAsmCode[CodeLen++] = 0x90 | src_adr_vals.value;
            break;
          case e_mod_ihl:
            BAsmCode[CodeLen++] = 0x06;
            break;
          default:
            break;
        }
        break;
      case e_mod_l:
        switch (decode_adr(&src_adr_vals, &ArgStr[2], MModImm))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x20 | src_adr_vals.value;
            break;
          default:
            break;
        }
        break;
      case e_mod_mbr:
        switch (decode_adr(&src_adr_vals, &ArgStr[2], MModImm))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0xb0 | src_adr_vals.value;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ldx(Word code)
 * \brief  handle LDL/LDH instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ldx(Word code)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(2, 2)
   && decode_adr(&adr_vals, &ArgStr[1], MModA)
   && decode_adr(&adr_vals, &ArgStr[2], MModIDC))
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_st(Word code)
 * \brief  handle ST instruction
 * ------------------------------------------------------------------------ */

static void decode_st(Word code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&src_adr_vals, &ArgStr[1], MModA | MModImm))
    {
      case e_mod_a:
        switch (decode_adr(&dest_adr_vals, &ArgStr[2], MModAbs | MModIHL))
        {
          case e_mod_abs:
            BAsmCode[CodeLen++] = 0x98 | dest_adr_vals.value;
            break;
          case e_mod_ihl:
            BAsmCode[CodeLen++] = 0x76;
            break;
          default:
            break;
        }
        break;
      case e_mod_imm:
        switch (decode_adr(&dest_adr_vals, &ArgStr[2], MModIHL))
        {
          case e_mod_ihl:
            BAsmCode[CodeLen++] = 0x30 | src_adr_vals.value;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mov(Word code)
 * \brief  handle MOV instruction
 * ------------------------------------------------------------------------ */

static void decode_mov(Word code)
{
  adr_vals_t adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&adr_vals, &ArgStr[1], MModA | MModL | MModD))
    {
      case e_mod_a:
        switch (decode_adr(&adr_vals, &ArgStr[2], MModL | MModD | MModP))
        {
          case e_mod_l:
            BAsmCode[CodeLen++] = 0x0c;
            break;
          case e_mod_d:
            BAsmCode[CodeLen++] = 0x0d;
            break;
          case e_mod_p:
            BAsmCode[CodeLen++] = 0x7e;
            break;
          default:
            break;
        }
        break;
      case e_mod_l:
        switch (decode_adr(&adr_vals, &ArgStr[2], MModA))
        {
          case e_mod_a:
            BAsmCode[CodeLen++] = 0x0f;
            break;
          default:
            break;
        }
        break;
      case e_mod_d:
        switch (decode_adr(&adr_vals, &ArgStr[2], MModA))
        {
          case e_mod_a:
            BAsmCode[CodeLen++] = 0x0e;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_in_out(Word code)
 * \brief  handle IN/OUT instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_in_out(Word code)
{
  adr_vals_t reg_adr_vals, io_adr_vals;
  int is_out = !!(code & 0x10);

  if (ChkArgCnt(2, 2)
   && decode_adr(&io_adr_vals, &ArgStr[1 + is_out], MModPort))
    switch (decode_adr(&reg_adr_vals, &ArgStr[2 - is_out], MModA | MModIHL))
    {
      case e_mod_a:
        BAsmCode[CodeLen++] = code | io_adr_vals.value;
        break;
      case e_mod_ihl:
        BAsmCode[CodeLen++] = code | 0x08 | io_adr_vals.value;
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_add(Word code)
 * \brief  handle ADD instruction
 * ------------------------------------------------------------------------ */

static void decode_add(Word code)
{
  adr_vals_t adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&adr_vals, &ArgStr[1], MModA | MModL))
    {
      case e_mod_a:
        switch (decode_adr(&adr_vals, &ArgStr[2], MModImm | MModIHL))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x40 | adr_vals.value;
            break;
          case e_mod_ihl:
            BAsmCode[CodeLen++] = 0x03;
            break;
          default:
            break;
        }
        break;
      case e_mod_l:
        switch (decode_adr(&adr_vals, &ArgStr[2], MModImm))
        {
          case e_mod_imm:
            BAsmCode[CodeLen++] = 0x50 | adr_vals.value;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ari(Word code)
 * \brief  handle arithmetic/logic instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ari(Word code)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(2, 2)
   && decode_adr(&adr_vals, &ArgStr[1], MModA)
   && decode_adr(&adr_vals, &ArgStr[2], MModIHL))
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_inc_dec(Word code)
 * \brief  handle INC/DEC instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_inc_dec(Word code)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(1, 1))
    switch (decode_adr(&adr_vals, &ArgStr[1], MModD | MModIHL))
    {
      case e_mod_d:
        BAsmCode[CodeLen++] = code | 0x02;
        break;
      case e_mod_ihl:
        BAsmCode[CodeLen++] = code;
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mem_bit(Word code)
 * \brief  handle instructions operating on bit in data memory
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mem_bit(Word code)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(2, 2)
   && decode_adr(&adr_vals, &ArgStr[1], MModIHL))
  {
    Boolean ok;
    BAsmCode[CodeLen] = Lo(code) | EvalStrIntExpression(&ArgStr[2], UInt2, &ok);
    if (ok)
      CodeLen++;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_clr_set(Word code)
 * \brief  handle CLR/SET instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_clr_set(Word code)
{
  adr_vals_t adr_vals;

  switch (ArgCnt)
  {
    case 1:
      switch (decode_adr(&adr_vals, &ArgStr[1], MModCF | MModH))
      {
        case e_mod_cf:
          BAsmCode[CodeLen++] = Hi(code) | 0x01;
          break;
        case e_mod_h:
          BAsmCode[CodeLen++] = Hi(code);
          break;
        default:
          break;
      }
      break;
    case 2:
      decode_mem_bit(Lo(code));
      break;
    default:
      (void)ChkArgCnt(1, 2);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_testp(Word code)
 * \brief  handle TESTP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_testp(Word code)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(1, 1)
   && decode_adr(&adr_vals, &ArgStr[1], MModCF))
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_bss(Word code)
 * \brief  handle BSS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_bss(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word dest = eval_code_address(&eval_result, (p_curr_cpu_props->flags & e_cpu_flag_rom_1k) ? UInt10 : UInt9);

    if (eval_result.OK
     && ChkSamePage(dest, (EProgCounter() + 1) & SegLimits[SegCode], 6, eval_result.Flags))
      BAsmCode[CodeLen++] = code | (dest & 0x3f);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_calls(Word code)
 * \brief  handle CALLS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_calls(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word dest = eval_code_address(&eval_result, UInt5);

    if (!eval_result.OK);
    else if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && (dest & 1)) WrStrErrorPos(ErrNum_AddrMustBeEven, &ArgStr[1]);
    else
      BAsmCode[CodeLen++] = code | ((dest >> 1) & 15);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_port(Word code)
 * \brief  handle PORT instruction
 * ------------------------------------------------------------------------ */

static void decode_port(Word code)
{
  UNUSED(code);
  code_equate_segment(SegIO);
}

/*!------------------------------------------------------------------------
 * \fn     check_code_segment(Word code)
 * \brief  checks whether code generation it attempted outside of code segment
 * ------------------------------------------------------------------------ */

static void check_code_segment(Word code)
{
  UNUSED(code);

  if (ActPC != SegCode)
    WrError(ErrNum_CodeNotInCodeSegment);
}

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  construct instruction dispatcher table
 * ------------------------------------------------------------------------ */

static void init_fields(void)
{
  InstTable = CreateInstTable(103);

  add_null_pseudo(InstTable);

  inst_table_set_prefix_proc(InstTable, check_code_segment, 0);
  AddInstTable(InstTable, "LD", 0, decode_ld);
  AddInstTable(InstTable, "LDL", 0x67, decode_ldx);
  AddInstTable(InstTable, "LDH", 0x66, decode_ldx);
  AddInstTable(InstTable, "ST", 0, decode_st);
  AddInstTable(InstTable, "MOV", 0, decode_mov);
  AddInstTable(InstTable, "IN", 0x60, decode_in_out);
  AddInstTable(InstTable, "OUT", 0x70, decode_in_out);
  AddInstTable(InstTable, "ADD", 0, decode_add);
  AddInstTable(InstTable, "ADDC", 0x04, decode_ari);
  AddInstTable(InstTable, "SUBRC", 0x05, decode_ari);
  AddInstTable(InstTable, "AND", 0x00, decode_ari);
  AddInstTable(InstTable, "OR", 0x01, decode_ari);
  AddInstTable(InstTable, "XOR", 0x02, decode_ari);
  AddInstTable(InstTable, "INC", 0x09, decode_inc_dec);
  AddInstTable(InstTable, "DEC", 0x08, decode_inc_dec);
  AddInstTable(InstTable, "SET", 0x8880, decode_clr_set);
  AddInstTable(InstTable, "CLR", 0x8a84, decode_clr_set);
  AddInstTable(InstTable, "TEST", 0x8c, decode_mem_bit);
  AddInstTable(InstTable, "TESTP", 0x77, decode_testp);
  AddInstTable(InstTable, "BSS", 0xc0, decode_bss);
  AddInstTable(InstTable, "CALLS", 0xa0, decode_calls);
  AddInstTable(InstTable, "NOP", NOPCode, decode_fixed);
  AddInstTable(InstTable, "RET", 0x6e, decode_fixed);
  if (p_curr_cpu_props->flags & e_cpu_flag_cmos)
    AddInstTable(InstTable, "HOLD", 0x07, decode_fixed);

  inst_table_set_prefix_proc(InstTable, NULL, 0);
  AddInstTable(InstTable, "PORT", 0, decode_port);
  AddIntelPseudo(InstTable, eIntPseudoFlag_LittleEndian);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  dissolve instruction dispatcher table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  DestroyInstTable(InstTable);
}

/*!------------------------------------------------------------------------
 * \fn     make_code_42c00(void)
 * \brief  actual code generator
 * ------------------------------------------------------------------------ */

void make_code_42c00(void)
{
  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_42c00(void)
 * \brief  does instruction consume label field?
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_42c00(void)
{
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_42c00(void)
 * \brief  switch away from target
 * ------------------------------------------------------------------------ */

static void switch_from_42c00(void)
{
  deinit_fields();
  p_curr_cpu_props = NULL;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_42c00(void *p_user)
 * \brief  switch to target
 * \param  p_user target properties
 * ------------------------------------------------------------------------ */

static Boolean TrueFnc(void)
{
  return True;
}

static void switch_to_42c00(void *p_user)
{
  const TFamilyDescr *p_descr = FindFamilyByName("TLCS-42xx");

  p_curr_cpu_props = (const cpu_props_t*)p_user;
  SetIntConstMode(eIntConstModeIntel);
  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x7f;
  DivideChars = ",";
  HasAttrs = False;
  SetIsOccupiedFnc = TrueFnc;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);

  Grans[SegCode] = ListGrans[SegCode] = 1;
  SegInits[SegCode] = SegInits[SegData] = SegInits[SegIO] = 0;
  SegLimits[SegCode] = (p_curr_cpu_props->flags & e_cpu_flag_rom_1k) ? 1023 : 511;

  Grans[SegData] = ListGrans[SegData] = 1;
  list_grans_bits_unused[SegData] = grans_bits_unused[SegData] = 4;
  SegLimits[SegData] = 31;

  Grans[SegIO] = ListGrans[SegIO] = 1;
  list_grans_bits_unused[SegIO] = grans_bits_unused[SegIO] = 4;
  SegLimits[SegIO] = (p_curr_cpu_props->flags & e_cpu_flag_io_23) ? 5 : 2;

  MakeCode = make_code_42c00;
  IsDef = is_def_42c00;
  SwitchFrom = switch_from_42c00;
  init_fields();
}

/*!------------------------------------------------------------------------
 * \fn     code42c00_init(void)
 * \brief  register targets
 * ------------------------------------------------------------------------ */

static const cpu_props_t cpu_props[] =
{
  { "4240P"  , (Byte)(e_cpu_flag_none                                       ) },
  { "4250N"  , (Byte)(                                      e_cpu_flag_io_23) },
  { "4260P"  , (Byte)(                  e_cpu_flag_rom_1k                   ) },
  { "4270N"  , (Byte)(                  e_cpu_flag_rom_1k | e_cpu_flag_io_23) },
  { "42C00Y" , (Byte)(e_cpu_flag_cmos | e_cpu_flag_rom_1k                   ) },
  { "42C40P" , (Byte)(e_cpu_flag_cmos                                       ) },
  { "42C50N" , (Byte)(e_cpu_flag_cmos                     | e_cpu_flag_io_23) },
  { "42C60P" , (Byte)(e_cpu_flag_cmos | e_cpu_flag_rom_1k                   ) },
  { "42C70N" , (Byte)(e_cpu_flag_cmos | e_cpu_flag_rom_1k | e_cpu_flag_io_23) }
};

void code42c00_init(void)
{
  const cpu_props_t *p_props;

  for (p_props = cpu_props; p_props < cpu_props + as_array_size(cpu_props); p_props++)
      (void)AddCPUUser(p_props->name, switch_to_42c00, (void*)p_props, NULL);
}
