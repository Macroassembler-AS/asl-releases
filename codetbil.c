/* codetbil.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TinyBASIC Intermediate Interpreter                          */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>

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

#include "codetbil.h"

static int PrefCnt;

static void arg_byte(int op)
{
  SetMaxCodeLen(PrefCnt + 1);
  BAsmCode[PrefCnt++] = op;
}

static void instr_byte(int op)
{
  PrefCnt = 0;
  arg_byte(op);
}

static void decode_fixed(Word code)
{
  if (!ChkArgCnt(0, 0)) return;
  instr_byte(code);
}

static void decode_lit3u(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;
    tSymbolFlags flags;
    Integer arg = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt3, &ok, &flags);
    if (ok)
    {
      set_b_guessed(flags, PrefCnt, 1, 0x07);
      instr_byte(code | arg);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     str_arg(tStrComp *p_arg, as_nonz_dynstr_t *p_str, tEvalResult *p_eval_result)
 * \brief  evalate string arg, and interprete caret notation
 * \param  p_arg source argument
 * \param  p_str return buffer
 * \param  evaluation attributes return buffer
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

Boolean str_arg(tStrComp *p_arg, as_nonz_dynstr_t *p_str, tEvalResult *p_eval_result)
{
  EvalStrStringExpressionWithResult(p_arg, p_eval_result, p_str);
  
  if (p_eval_result->OK)
  {
    char *p_src, *p_dest, *p_end;
    size_t len;
    Boolean next_ctrl;

    len = p_str->len;
    if (!len)
    {
      WrStrErrorPos(ErrNum_EmptyArgument, p_arg);
      return False;
    }
    p_end = p_str->p_str + len;

    /* String may only get shorter by interpreting caret
       notation, so no need to check for reallocation: */

    for (p_src = p_dest = p_str->p_str, next_ctrl = False;
         p_src < p_end; p_src++)
    {
      if (next_ctrl)
      {
        if (*p_src == '@')
          *p_dest++ = '\0';
        else if (*p_src == '?')
          *p_dest++ = '\177';
        else if (as_isalpha(*p_src))
          *p_dest++ = as_toupper(*p_src) - 'A' + 1;
        else
        {
          static const char haystack[] = "[\\]^_";
          const char *p_pos = strchr(haystack, *p_src);
          if (p_pos)
            *p_dest++ = p_pos - haystack + '\033';
          else
          {
            *p_dest++ = '^';
            *p_dest++ = *p_src;
          }
        }
        next_ctrl = False;
      }
      else if (*p_src == '^')
        next_ctrl = True;
      else
        *p_dest++ = *p_src;
    }
    p_str->len = p_dest - p_str->p_str;
  }
  return p_eval_result->OK;
}

/*!------------------------------------------------------------------------
 * \fn     append_str(const as_nonz_dynstr_t *p_str, tSymbolFlags flags, const tStrComp *p_arg)
 * \brief  translate & append string to opcode, with bit 7 set in final character
 * \param  p_str string to append
 * \param  flags symbol evaluation flags
 * \param  p_arg corresponding source argument
 * \return True if string could be appended
 * ------------------------------------------------------------------------ */

static Boolean append_str(const as_nonz_dynstr_t *p_str, tSymbolFlags flags, const tStrComp *p_arg)
{
  size_t i;

  for (i = 0; i < p_str->len; i++)
  {
    int trans = as_chartrans_xlate(CurrTransTable->p_table, p_str->p_str[i]);
    if (trans >= 0x80)
    {
      WrStrErrorPos(ErrNum_OverRange, p_arg);
      return False;
    }
    set_b_guessed(flags, PrefCnt, 1, 0xff);
    arg_byte(trans);
  }
  BAsmCode[PrefCnt - 1] |= 0x80;
  return True;
}

static void decode_lits(Word code)
{
  as_nonz_dynstr_t str;
  tEvalResult eval_result;

  as_nonz_dynstr_ini(&str, 0);

  if (ChkArgCnt(1, 1) && str_arg(&ArgStr[1], &str, &eval_result))
  {
    instr_byte(code);

    if (!append_str(&str, eval_result.Flags, &ArgStr[1]))
      PrefCnt = 0;
  }
  as_nonz_dynstr_free(&str);
}

static Boolean decode_rel_core(Word code, IntType dist_type)
{
  tSymbolFlags flags;
  Boolean ok;
  Word adr;
  Integer dist;

  if (!strcmp(ArgStr[1].str.p_str, "*"))
  {
    instr_byte(code | 0);
    return True;
  }

  adr = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &ok, &flags);
  if (!ok)
    return False;

  dist = adr - (EProgCounter() + 1);
  if ((!dist || !RangeCheck(dist, dist_type))
   && !mFirstPassUnknownOrQuestionable(flags))
  {
    WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
    return False;
  }

  set_b_guessed(flags, PrefCnt, 1, IntTypeDefs[dist_type].Mask);
  instr_byte(code | (dist & IntTypeDefs[dist_type].Mask));
  return True;
}

static void decode_rel5us(Word code)
{
  if (ChkArgCnt(2, 2) && decode_rel_core(code, UInt5))
  {
    as_nonz_dynstr_t str;
    tEvalResult eval_result;

    as_nonz_dynstr_ini(&str, 0);
    if (str_arg(&ArgStr[2], &str, &eval_result))
      eval_result.OK = append_str(&str, eval_result.Flags, &ArgStr[2]);
    if (!eval_result.OK)
      PrefCnt = 0;
    as_nonz_dynstr_free(&str);
  }
}

static void decode_rel6s(Word code)
{
  UNUSED(code);
  if (ChkArgCnt(1, 1))
    (void)decode_rel_core(0x60, SInt6); /* dubious - discuss */
}

static void decode_rel5u(Word code)
{
  if (ChkArgCnt(1, 1))
    (void)decode_rel_core(code, UInt5);
}

static void decode_lit8(Word code)
{
  Boolean OK;
  tSymbolFlags Flags;
  Integer arg;

  if (!ChkArgCnt(1, 1)) return;
  arg = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt8, &OK, &Flags);
  if (OK)
  {
    instr_byte(code);
    set_b_guessed(Flags, PrefCnt, 1, 0xff);
    arg_byte(arg);
  }
}

static void decode_lit16(Word code)
{
  Boolean OK;
  tSymbolFlags Flags;
  Integer arg;

  if (!ChkArgCnt(1, 1)) return;
  arg = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);
  if (OK)
  {
    instr_byte(code);
    set_b_guessed(Flags, PrefCnt, 2, 0xff);
    arg_byte(arg >> 8);
    arg_byte(arg & 0xff);
  }
}

static void decode_lit11(Word code)
{
  Boolean OK;
  tSymbolFlags Flags;
  Integer arg;

  if (!ChkArgCnt(1, 1)) return;
  arg = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt11, &OK, &Flags);
  if (OK)
  {
    set_b_guessed(Flags, PrefCnt, 1, 0x07);
    instr_byte(code | (arg >> 8));
    set_b_guessed(Flags, PrefCnt, 1, 0xff);
    arg_byte(arg & 0xff);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(87);
  add_null_pseudo(InstTable);
  AddInstTable(InstTable, "SX"  , 0, decode_lit3u);
  AddInstTable(InstTable, "NO"  , NOPCode, decode_fixed);
  AddInstTable(InstTable, "LB"  , 0x09, decode_lit8);
  AddInstTable(InstTable, "LN"  , 0x0a, decode_lit16);
  AddInstTable(InstTable, "DS"  , 0x0b, decode_fixed);
  AddInstTable(InstTable, "SP"  , 0x0c, decode_fixed);
  AddInstTable(InstTable, "SB"  , 0x10, decode_fixed);
  AddInstTable(InstTable, "RB"  , 0x11, decode_fixed);
  AddInstTable(InstTable, "FV"  , 0x12, decode_fixed);
  AddInstTable(InstTable, "SV"  , 0x13, decode_fixed);
  AddInstTable(InstTable, "GS"  , 0x14, decode_fixed);
  AddInstTable(InstTable, "RS"  , 0x15, decode_fixed);
  AddInstTable(InstTable, "GO"  , 0x16, decode_fixed);
  AddInstTable(InstTable, "NE"  , 0x17, decode_fixed);
  AddInstTable(InstTable, "AD"  , 0x18, decode_fixed);
  AddInstTable(InstTable, "SU"  , 0x19, decode_fixed);
  AddInstTable(InstTable, "MP"  , 0x1a, decode_fixed);
  AddInstTable(InstTable, "DV"  , 0x1b, decode_fixed);
  AddInstTable(InstTable, "CP"  , 0x1c, decode_fixed);
  AddInstTable(InstTable, "NX"  , 0x1d, decode_fixed);
  /* 0x1e missing */
  AddInstTable(InstTable, "LS"  , 0x1f, decode_fixed);
  AddInstTable(InstTable, "PN"  , 0x20, decode_fixed);
  AddInstTable(InstTable, "PQ"  , 0x21, decode_fixed);
  AddInstTable(InstTable, "PT"  , 0x22, decode_fixed);
  AddInstTable(InstTable, "NL"  , 0x23, decode_fixed);
  AddInstTable(InstTable, "PC"  , 0x24, decode_lits);
  /* 0x25 0x26 missing */
  AddInstTable(InstTable, "GL"  , 0x27, decode_fixed);
  /* 0x28 0x29 missing */
  AddInstTable(InstTable, "IL"  , 0x2a, decode_fixed);
  AddInstTable(InstTable, "MT"  , 0x2b, decode_fixed);
  AddInstTable(InstTable, "XQ"  , 0x2c, decode_fixed);
  AddInstTable(InstTable, "WS"  , 0x2d, decode_fixed);
  AddInstTable(InstTable, "US"  , 0x2e, decode_fixed);
  AddInstTable(InstTable, "RT"  , 0x2f, decode_fixed);
  AddInstTable(InstTable, "JS"  , 0x30, decode_lit11);
  AddInstTable(InstTable, "J"   , 0x38, decode_lit11);
  AddInstTable(InstTable, "BR"  , 0x40, decode_rel6s);
  AddInstTable(InstTable, "BC"  , 0x80, decode_rel5us);
  AddInstTable(InstTable, "BV"  , 0xa0, decode_rel5u);
  AddInstTable(InstTable, "BN"  , 0xc0, decode_rel5u);
  AddInstTable(InstTable, "BE"  , 0xe0, decode_rel5u);

  add_moto8_pseudo(InstTable, e_moto_pseudo_flags_be);
}

static void MakeCode_TBIL(void)
{
  PrefCnt = 0;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
  CodeLen = PrefCnt;
}

static Boolean IsDef_TBIL(void)
{
  return False;
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

static void SwitchTo_TBIL(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("TBIL");

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = ".";
  HeaderID = p_descr->Id;
  NOPCode = 0x08;
  DivideChars = ", \t";
  HasAttrs = 0;
  label_leading_colon = True;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0x1fff;

  MakeCode = MakeCode_TBIL;
  IsDef = IsDef_TBIL;
  SwitchFrom = DeinitFields;
  InitFields();
}

static CPUVar CPUTBIL;

void codetbil_init(void)
{
  CPUTBIL = AddCPU("tbil", SwitchTo_TBIL);
  AddCopyright("Tiny BASIC Intermediate Language Generator also (C) 2026 Jeff Epler");
}
