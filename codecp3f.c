/* codecp3f.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator Olympia CP-3F                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codecp3f.h"

/*---------------------------------------------------------------------------*/
/* Address Decoders */

typedef enum
{
  e_mode_none = -1,
  e_mode_a = 0,
  e_mode_v = 1,
  e_mode_w = 2,
  e_mode_x = 3,
  e_mode_y = 4,
  e_mode_s = 5,
  e_mode_t = 6, 
  e_mode_st = 7,
  e_mode_imm = 8,
  e_mode_imm_short = 9,
  e_mode_dir = 10,
  e_mode_q = 11,
  e_mode_z = 12,
  e_mode_iz = 13
} adr_mode_t;

#define MModeA (1 << e_mode_a)
#define MModeV (1 << e_mode_v)
#define MModeW (1 << e_mode_w)
#define MModeX (1 << e_mode_x)
#define MModeY (1 << e_mode_y)
#define MModeS (1 << e_mode_s)
#define MModeT (1 << e_mode_t)
#define MModeST (1 << e_mode_st)
#define MModeQ (1 << e_mode_q)
#define MModeZ (1 << e_mode_z)
#define MModeIZ (1 << e_mode_iz)
#define MModeImm (1 << e_mode_imm)
#define MModeImmShort (1 << e_mode_imm_short)
#define MModeDir (1 << e_mode_dir)

typedef struct
{
  adr_mode_t mode;
  Byte value;
} adr_vals_t;

/*!------------------------------------------------------------------------
 * \fn     reset_adr_vals(adr_vals_t *p_vals)
 * \brief  reset ecoded address argument to none
 * \param  p_vals argument to reset
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_mode_none;
  p_vals->value = 0;
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals, unsigned mask, tSymbolSize op_size)
 * \brief  decode address expression
 * \param  p_arg source argument
 * \param  p_vals encoded mode
 * \param  mask bit mask of allowed modes
 * \return deduced address mode
 * ------------------------------------------------------------------------ */

static int chk_xy(const char *p_arg, const char *p_pattern, Byte *p_reg)
{
  for (; *p_arg && *p_pattern; p_arg++, p_pattern++)
  {
    if (*p_pattern == '*')
    {
      switch (as_toupper(*p_arg))
      {
        case 'X': *p_reg = 0; break;
        case 'Y': *p_reg = 1; break;
        default: return 1;
      }
    }
    else if (as_toupper(*p_arg) != as_toupper(*p_pattern))
      return 1;
  }
  return !(!*p_arg && !*p_pattern);
}

static adr_mode_t decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals, unsigned mask, tSymbolSize op_size)
{
  size_t l = strlen(p_arg->str.p_str);
  tEvalResult eval_result;

  reset_adr_vals(p_vals);

  if (l == 1)
  {
    const char reg_names[] = "AVWXYST";
    const char *p_pos = strchr(reg_names, as_toupper(p_arg->str.p_str[0]));

    if (p_pos)
    {
      p_vals->mode = (adr_mode_t)(e_mode_a + (p_pos - reg_names));
      goto fini;
    }
  }

  if (*p_arg->str.p_str == '#')
  {
    int offset = 1;

    switch (op_size)
    {
      case eSymbolSize24Bit:
        p_vals->value = EvalStrIntExpressionOffsWithResult(p_arg, offset, UInt3, &eval_result);
        if (eval_result.OK)
          p_vals->mode = e_mode_imm;
        goto fini;
      case eSymbolSize8Bit:
      {
        Boolean force_short = False,
                force_long = False,
                opt_short = !!(mask & MModeImmShort);

        if (opt_short)
          switch (p_arg->str.p_str[offset])
          {
            case '<':
              force_short = True;
              offset++;
              break;
            case '>':
              force_long = True;
              offset++;
              break;
            default:
              break;
          }
        p_vals->value = EvalStrIntExpressionOffsWithResult(p_arg, offset, force_short ? UInt4 : Int8, &eval_result);
        if (eval_result.OK)
        {
          if (!opt_short)
            p_vals->mode = e_mode_imm;
          else if (force_long)
            p_vals->mode = e_mode_imm;
          else
            p_vals->mode = (p_vals->value < 16) ? e_mode_imm_short : e_mode_imm;
        }
        goto fini;
      }
      default:
        WrStrErrorPos(ErrNum_UndefOpSizes, p_arg);
        goto fini;
    }
  }

  if (!as_strcasecmp(p_arg->str.p_str, "ST"))
  {
    p_vals->mode = e_mode_st;
    goto fini;
  }
  if (!as_strcasecmp(p_arg->str.p_str, "(ST)"))
  {
    p_vals->mode = e_mode_dir;
    p_vals->value = 12;
    goto fini;
  }
  if (!as_strcasecmp(p_arg->str.p_str, "(ST)-"))
  {
    p_vals->mode = e_mode_dir;
    p_vals->value = 13;
    goto fini;
  }
  if (!as_strcasecmp(p_arg->str.p_str, "(ST)+"))
  {
    p_vals->mode = e_mode_dir;
    p_vals->value = 14;
    goto fini;
  }
  if (!chk_xy(p_arg->str.p_str, "Q(*)", &p_vals->value))
  {
    p_vals->mode = e_mode_q;
    goto fini;
  }
  if (!chk_xy(p_arg->str.p_str, "Z(*)", &p_vals->value))
  {
    p_vals->mode = e_mode_z;
    goto fini;
  }
  if (!chk_xy(p_arg->str.p_str, "(Z(*))", &p_vals->value))
  {
    p_vals->mode = e_mode_iz;
    goto fini;
  }

  p_vals->value = EvalStrIntExpressionWithResult(p_arg, UInt4, &eval_result);
  if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
    p_vals->value &= 7;
  if (ChkRange(p_vals->value, 0, 11))
  {
    ChkSpace(SegData, eval_result.AddrSpaceMask);
    p_vals->mode = e_mode_dir;
    goto fini;
  }

fini:
  if ((p_vals->mode != e_mode_none) && !((mask >> p_vals->mode) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    reset_adr_vals(p_vals);
  }
  return p_vals->mode;
}

/*---------------------------------------------------------------------------*/
/* Instruction Decoders */

/*!------------------------------------------------------------------------
 * \fn     decode_imm_4(Word code)
 * \brief  handle instructions with 4 bit immediate operand
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm_4(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;

    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt4, &ok);
    if (ok)
    {
      BAsmCode[0] = code | (BAsmCode[0] & 0x0f);
      CodeLen = 1;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm_3(Word code)
 * \brief  handle instructions with 3 bit immediate operand
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm_3(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;

    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt3, &ok);
    if (ok)
    {
      BAsmCode[0] = code | (BAsmCode[0] & 0x07);
      CodeLen = 1;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm_8(Word code)
 * \brief  handle instructions with 8 bit immediate operand
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm_8(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int8, &ok);
    if (ok)
    {
      BAsmCode[0] = code;
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions with no argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = code;
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg(Word code)
 * \brief  handle instructions with register argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

/* Original syntax assumes programmer knows that 12..14 refer
   to (ST), (ST)+, (ST)-... */

static void decode_reg(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;

    BAsmCode[0] = EvalStrIntExpressionWithResult(&ArgStr[1], UInt4, &eval_result);
    if (eval_result.OK)
    {
      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        BAsmCode[0] = 0;
      if (ChkRange(BAsmCode[0], 0, 14))
      {
        BAsmCode[0] |= code;
        CodeLen = 1;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_io(Word code)
 * \brief  handle instructions with I/O address operand
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_io(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;

    BAsmCode[0] = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &eval_result);
    if (eval_result.OK)
    {
      ChkSpace(SegIO, eval_result.AddrSpaceMask);
      BAsmCode[0] = code | (BAsmCode[0] & 0x07);
      CodeLen = 1;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_port(Word code)
 * \brief  handle PORt instruction
 * ------------------------------------------------------------------------ */

static void decode_port(Word code)
{
  UNUSED(code);

  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

/*!------------------------------------------------------------------------
 * \fn     decode_jmp(Word code)
 * \brief  handle instructions with target address operand
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jmp(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt14, &eval_result);

    if (eval_result.OK
     && ChkSamePage(EProgCounter(), address, 11, eval_result.Flags))
    {
      BAsmCode[0] = code | (Hi(address) & 7);
      BAsmCode[1] = Lo(address);
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     void decode_ld(Word code)
 * \brief  decode virtual LD instruction
 * ------------------------------------------------------------------------ */

static void decode_ld(Word code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&ArgStr[1], &dest_adr_vals, MModeA | MModeV | MModeW | MModeX | MModeY | MModeS | MModeT | MModeST | MModeDir | MModeQ | MModeZ | MModeIZ, eSymbolSizeUnknown))
    {
      case e_mode_a:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeV | MModeW | MModeX | MModeY | MModeDir | MModeImm | MModeImmShort | MModeIZ, eSymbolSize8Bit))
        {
          case e_mode_v:
          case e_mode_w:
          case e_mode_x:
          case e_mode_y:
            BAsmCode[0] = 0x08 | (src_adr_vals.mode - e_mode_v);
            CodeLen = 1;
            break;
          case e_mode_dir:
            BAsmCode[0] = 0x80 | src_adr_vals.value;
            CodeLen = 1;
            break;
          case e_mode_imm:
            BAsmCode[0] = 0x04;
            BAsmCode[1] = src_adr_vals.value;
            CodeLen = 2;
            break;
          case e_mode_imm_short:
            BAsmCode[0] = 0xf0 | (src_adr_vals.value & 15);
            CodeLen = 1;
            break;
          case e_mode_iz:
            BAsmCode[0] = 0x06 | src_adr_vals.value;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_v:
      case e_mode_w:
      case e_mode_x:
      case e_mode_y:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA, eSymbolSize8Bit))
        {
          case e_mode_a:
            BAsmCode[0] = 0x18 | (dest_adr_vals.mode - e_mode_v);
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_s:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeImm, eSymbolSize24Bit))
        {
          case e_mode_imm:
            BAsmCode[0] = 0x28 | (src_adr_vals.value & 7);
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_t:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA | MModeImm, eSymbolSize24Bit))
        {
          case e_mode_a:
            BAsmCode[0] = 0x01;
            CodeLen = 1;
            break;
          case e_mode_imm:
            BAsmCode[0] = 0x38 | (src_adr_vals.value & 7);
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_st:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA, eSymbolSizeUnknown))
        {
          case e_mode_a:
            BAsmCode[0] = 0x03;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_q:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA, eSymbolSizeUnknown))
        {
          case e_mode_a:
            BAsmCode[0] = 0x16 | dest_adr_vals.value;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_z:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA, eSymbolSizeUnknown))
        {
          case e_mode_a:
            BAsmCode[0] = 0x12 | dest_adr_vals.value;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_iz:
        if (dest_adr_vals.value) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA, eSymbolSizeUnknown))
        {
          case e_mode_a:
            BAsmCode[0] = 0x02 | dest_adr_vals.value;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case e_mode_dir:
        switch (decode_adr(&ArgStr[2], &src_adr_vals, MModeA, eSymbolSize8Bit))
        {
          case e_mode_a:
            BAsmCode[0] = 0x90 | dest_adr_vals.value;
            CodeLen = 1;
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
 * \fn     void decode_ari(Word code)
 * \brief  decode virtual arithmetic instruction
 * ------------------------------------------------------------------------ */

static void decode_ari(Word code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;
  unsigned mask;
  Byte imm_code = Hi(code), dir_code = Lo(code);

  if (!ChkArgCnt(1, 2))
    return;

  if (ArgCnt == 2)
  {
    if (decode_adr(&ArgStr[1], &dest_adr_vals, MModeA, eSymbolSize8Bit) != e_mode_a)
      return;
  }

  mask = ((imm_code == 0xff) ? 0 : MModeImm)
       | ((dir_code == 0xff) ? 0 : MModeDir);
  switch (decode_adr(&ArgStr[ArgCnt], &src_adr_vals, mask, eSymbolSize8Bit))
  {
    case e_mode_dir:
      BAsmCode[0] = dir_code | src_adr_vals.value;
      CodeLen = 1;
      break;
    case e_mode_imm:
      BAsmCode[0] = imm_code;
      BAsmCode[1] = src_adr_vals.value;
      CodeLen = 2;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_srl_sla(Word code)
 * \brief  decode virtual shift instructions
 * \param  code machine code for single bit shift
 * ------------------------------------------------------------------------ */

static void decode_srl_sla(Word code)
{
  int shift_arg_index;
  Byte shift_cnt;

  switch (ArgCnt)
  {
    case 1:
      if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
        goto acc_1;
      shift_arg_index = 1;
      goto eval_shift;
    case 0:
    acc_1:
      shift_cnt = 1;
      break;
    case 2:
    {
      adr_vals_t adr_vals;

      if (decode_adr(&ArgStr[1], &adr_vals, MModeA, eSymbolSize8Bit) != e_mode_a)
        return;
      shift_arg_index = 2;
      /* FALL-THRU */
    }
    eval_shift:
    {
      tEvalResult eval_result;
      shift_cnt = EvalStrIntExpressionWithResult(&ArgStr[shift_arg_index], UInt8, &eval_result);
      if (!eval_result.OK)
        return;
      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        shift_cnt = 1;
      if ((shift_cnt != 1) && (shift_cnt != 4))
      {
        WrStrErrorPos(ErrNum_InvShiftArg, &ArgStr[shift_arg_index]);
        return;
      }
      break;
    }
    default:
      (void)ChkArgCnt(0, 2);
      return;
  }

  BAsmCode[0] = code | ((shift_cnt > 1) ? 2 : 0);
  CodeLen = 1;
}

/*---------------------------------------------------------------------------*/
/* Code Table Handling */

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  build up instruction hash table
 * ------------------------------------------------------------------------ */

static void init_fields(void)
{
  InstTable = CreateInstTable(101);

  AddInstTable(InstTable, "LAS", 0xf0, decode_imm_4);
  AddInstTable(InstTable, "LSS", 0x28, decode_imm_3);
  AddInstTable(InstTable, "LTS", 0x38, decode_imm_3);
  AddInstTable(InstTable, "LAL", 0x04, decode_imm_8);
  AddInstTable(InstTable, "ANL", 0x05, decode_imm_8);
  AddInstTable(InstTable, "EOL", 0x0c, decode_imm_8);
  AddInstTable(InstTable, "ORL", 0x0d, decode_imm_8);
  AddInstTable(InstTable, "ADL", 0x0e, decode_imm_8);
  AddInstTable(InstTable, "CML", 0x0f, decode_imm_8);

  AddInstTable(InstTable, "LAV", 0x08, decode_fixed);
  AddInstTable(InstTable, "LAW", 0x09, decode_fixed);
  AddInstTable(InstTable, "LAX", 0x0a, decode_fixed);
  AddInstTable(InstTable, "LAY", 0x0b, decode_fixed);
  AddInstTable(InstTable, "SAV", 0x18, decode_fixed);
  AddInstTable(InstTable, "SAW", 0x19, decode_fixed);
  AddInstTable(InstTable, "SAX", 0x1a, decode_fixed);
  AddInstTable(InstTable, "SAY", 0x1b, decode_fixed);
  AddInstTable(InstTable, "SAT", 0x01, decode_fixed);
  AddInstTable(InstTable, "SST", 0x03, decode_fixed);
  AddInstTable(InstTable, "ALS", 0x1c, decode_fixed);
  AddInstTable(InstTable, "ARS", 0x1d, decode_fixed);
  AddInstTable(InstTable, "ALF", 0x1e, decode_fixed);
  AddInstTable(InstTable, "ARF", 0x1f, decode_fixed);
  AddInstTable(InstTable, "RET", 0x00, decode_fixed);
  AddInstTable(InstTable, "SIX", 0x02, decode_fixed);
  AddInstTable(InstTable, "LIX", 0x06, decode_fixed);
  AddInstTable(InstTable, "LIY", 0x07, decode_fixed);
  AddInstTable(InstTable, "SQX", 0x16, decode_fixed);
  AddInstTable(InstTable, "SQY", 0x17, decode_fixed);
  AddInstTable(InstTable, "SZX", 0x12, decode_fixed);
  AddInstTable(InstTable, "SZY", 0x13, decode_fixed);

  AddInstTable(InstTable, "LAR", 0x80, decode_reg);
  AddInstTable(InstTable, "SAR", 0x90, decode_reg);
  AddInstTable(InstTable, "ADR", 0xa0, decode_reg);
  AddInstTable(InstTable, "ANR", 0xb0, decode_reg);
  AddInstTable(InstTable, "EOR", 0xc0, decode_reg);
  AddInstTable(InstTable, "DER", 0xd0, decode_reg);
  AddInstTable(InstTable, "DAR", 0xe0, decode_reg);

  AddInstTable(InstTable, "INP", 0x20, decode_io);
  AddInstTable(InstTable, "OUT", 0x30, decode_io);
  AddInstTable(InstTable, "PORT", 0, decode_port);

  AddInstTable(InstTable, "JMP", 0x40, decode_jmp);
  AddInstTable(InstTable, "JAZ", 0x48, decode_jmp);
  AddInstTable(InstTable, "JAN", 0x50, decode_jmp);
  AddInstTable(InstTable, "JAP", 0x58, decode_jmp);
  AddInstTable(InstTable, "JSD", 0x60, decode_jmp);
  AddInstTable(InstTable, "JCN", 0x68, decode_jmp);
  AddInstTable(InstTable, "JCZ", 0x70, decode_jmp);
  AddInstTable(InstTable, "JSB", 0x78, decode_jmp);
  AddInstTable(InstTable, "GOS", 0x78, decode_jmp);

  AddInstTable(InstTable, "LD", 0, decode_ld);
  AddInstTable(InstTable, "ADD", 0x0ea0, decode_ari);
  AddInstTable(InstTable, "AND", 0x05b0, decode_ari);
  AddInstTable(InstTable, "XOR", 0x0cc0, decode_ari);
  AddInstTable(InstTable, "OR" , 0x0dff, decode_ari);
	AddInstTable(InstTable, "CP" , 0x0fff, decode_ari);
  AddInstTable(InstTable, "DEC", 0xffd0, decode_ari);
  AddInstTable(InstTable, "SLA", 0x1c, decode_srl_sla);
  AddInstTable(InstTable, "SRL", 0x1d, decode_srl_sla);

  AddInstTable(InstTable, "DC", 0, DecodeMotoBYT);
  AddInstTable(InstTable, "DS", 0, DecodeMotoDFS);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  tear down instruction hash table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     make_code_cp3f(void)
 * \brief  machine instruction dispatcher
 * ------------------------------------------------------------------------ */

static void make_code_cp3f(void)
{
  CodeLen = 0; DontPrint = False;

  /* to be ignored */

  if (Memo("")) return;

  /* pseudo instructions */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_cp3f(void)
 * \brief  cleanups after switch from target
 * ------------------------------------------------------------------------ */

static void switch_from_cp3f(void)
{
  deinit_fields();
}

/*!------------------------------------------------------------------------
 * \fn     is_def_cp_3f(void)
 * \brief  does instruction use label field?
 * ------------------------------------------------------------------------ */

static Boolean is_def_cp3f(void)
{
  return Memo("PORT");
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_cp_3f(void)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_cp3f(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("CP-3F");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  p_descr = FindFamilyByName("CP-3F");
  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegLimits[SegCode] = 0x3fff;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegLimits[SegData] = 0x2f;
  Grans[SegIO  ] = 1; ListGrans[SegIO  ] = 1; SegLimits[SegIO  ] = 0x7;

  MakeCode = make_code_cp3f;
  SwitchFrom = switch_from_cp3f;
  IsDef = is_def_cp3f;
  init_fields();
}

/*!------------------------------------------------------------------------
 * \fn     codecp3f__init(void)
 * \brief  register CP-3F target
 * ------------------------------------------------------------------------ */

void codecp3f_init(void)
{
  (void)AddCPU("CP-3F"    , switch_to_cp3f);
  (void)AddCPU("M380"     , switch_to_cp3f);
  (void)AddCPU("LP8000"   , switch_to_cp3f);
}
