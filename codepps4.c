/* codepps4.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator Rockwell PPS-4                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "headids.h"
#include "asmitree.h"
#include "codevars.h"
#include "asmerr.h"
#include "errmsg.h"
#include "fourpseudo.h"

#include "codepps4.h"

/*---------------------------------------------------------------------------*/
/* Instruction Decoders */

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
 * \fn     decode_adi(Word code)
 * \brief  handle ADI instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_adi(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Byte value = EvalStrIntExpressionWithResult(&ArgStr[1], Int4, &eval_result);

    if (eval_result.OK)
    {
      /* I am not 100% sure I have to insert one's complement of the
         value into the instruction.  But this way, it makes most sense: */

      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        value = 15;
      value = ~value & 15;
      if ((value == 15) || (value == 5))
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
      else
      {
        BAsmCode[0] = code | value;
        CodeLen = 1;
      }
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
 * \fn     decode_t(Word code)
 * \brief  handle T instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_t(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt12, &eval_result);

    if (eval_result.OK
     && ChkSamePage(EProgCounter(), address, 6, eval_result.Flags))
    {
      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      BAsmCode[0] = code | (address & 63);
      CodeLen = 1;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_tm(Word code)
 * \brief  handle TM instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_tm(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt12, &eval_result);

    /* A lower address byte fron locations 0xd0...0xff is accessed, with
       bits 11:8 set to 0x1. We check for a correct vector address: */

    if (eval_result.OK)
    {
      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        address = (address & 0x00f) | 0x0d0;
      if (ChkRange(address, 0xd0, 0xff))
      {
        BAsmCode[0] = code | (address & 0x3f);
        CodeLen = 1;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_tl(Word code)
 * \brief  handle TL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_tl(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt12, &eval_result);

    if (eval_result.OK)
    {
      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      BAsmCode[0] = code | (Hi(address) & 15);
      BAsmCode[1] = Lo(address);
      CodeLen = 2;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_tml(Word code)
 * \brief  handle TML instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_tml(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt12, &eval_result);

    if (eval_result.OK)
    {
      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
        address = (address & 0x0ff) | 0x100;
      if (ChkRange(address, 0x100, 0x3ff))
      {
        BAsmCode[0] = code | (Hi(address) & 0x03);
        BAsmCode[1] = Lo(address);
        CodeLen = 2;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_data_pps4(Word code)
 * \brief  handle DATA instruction
 * ------------------------------------------------------------------------ */

static void decode_data_pps4(Word code)
{
  UNUSED(code);

  DecodeDATA(Int8, Int4);
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

  AddInstTable(InstTable, "AD"   , 0x0b, decode_fixed);
  AddInstTable(InstTable, "ADC"  , 0x0a, decode_fixed);
  AddInstTable(InstTable, "ADSK" , 0x09, decode_fixed);
  AddInstTable(InstTable, "ADCSK", 0x08, decode_fixed);
  AddInstTable(InstTable, "DC"   , 0x65, decode_fixed);
  AddInstTable(InstTable, "AND"  , 0x0d, decode_fixed);
  AddInstTable(InstTable, "OR"   , 0x0f, decode_fixed);
  AddInstTable(InstTable, "EOR"  , 0x0c, decode_fixed);
  AddInstTable(InstTable, "COMP" , 0x0e, decode_fixed);
  AddInstTable(InstTable, "SC"   , 0x20, decode_fixed);
  AddInstTable(InstTable, "RC"   , 0x24, decode_fixed);
  AddInstTable(InstTable, "SF1"  , 0x22, decode_fixed);
  AddInstTable(InstTable, "RF1"  , 0x26, decode_fixed);
  AddInstTable(InstTable, "SF2"  , 0x21, decode_fixed);
  AddInstTable(InstTable, "RF2"  , 0x25, decode_fixed);
  AddInstTable(InstTable, "LAX"  , 0x12, decode_fixed);
  AddInstTable(InstTable, "LXA"  , 0x1b, decode_fixed);
  AddInstTable(InstTable, "LABL" , 0x11, decode_fixed);
  AddInstTable(InstTable, "LBMX" , 0x10, decode_fixed);
  AddInstTable(InstTable, "LBUA" , 0x04, decode_fixed);
  AddInstTable(InstTable, "XABL" , 0x19, decode_fixed);
  AddInstTable(InstTable, "XBMX" , 0x18, decode_fixed);
  AddInstTable(InstTable, "XAX"  , 0x1a, decode_fixed);
  AddInstTable(InstTable, "XS"   , 0x06, decode_fixed);
  AddInstTable(InstTable, "CYS"  , 0x6f, decode_fixed);
  AddInstTable(InstTable, "INCB" , 0x17, decode_fixed);
  AddInstTable(InstTable, "DECB" , 0x1f, decode_fixed);
  AddInstTable(InstTable, "SKC"  , 0x15, decode_fixed);
  AddInstTable(InstTable, "SKZ"  , 0x1e, decode_fixed);
  AddInstTable(InstTable, "SKF1" , 0x16, decode_fixed);
  AddInstTable(InstTable, "SKF2" , 0x14, decode_fixed);
  AddInstTable(InstTable, "RTN"  , 0x05, decode_fixed);
  AddInstTable(InstTable, "RTNSK", 0x07, decode_fixed);
  AddInstTable(InstTable, "DIA"  , 0x27, decode_fixed);
  AddInstTable(InstTable, "DIB"  , 0x23, decode_fixed);
  AddInstTable(InstTable, "DOA"  , 0x1d, decode_fixed);
  AddInstTable(InstTable, "SAG"  , 0x13, decode_fixed);

  AddInstTable(InstTable, "ADI"  , 0x60, decode_adi);
  AddInstTable(InstTable, "LD"   , 0x30, decode_imm_3);
  AddInstTable(InstTable, "EX"   , 0x38, decode_imm_3);
  AddInstTable(InstTable, "EXD"  , 0x28, decode_imm_3);
  AddInstTable(InstTable, "LDI"  , 0x70, decode_imm_4);
  AddInstTable(InstTable, "LB"   , 0xc0, decode_imm_4);
  AddInstTable(InstTable, "LBL"  , 0x00, decode_imm_8);
  AddInstTable(InstTable, "SKBI" , 0x40, decode_imm_4);
  AddInstTable(InstTable, "IOL"  , 0x1c, decode_imm_8);

  AddInstTable(InstTable, "T"    , 0x80, decode_t);
  AddInstTable(InstTable, "TM"   , 0xc0, decode_tm);
  AddInstTable(InstTable, "TL"   , 0x50, decode_tl);
  AddInstTable(InstTable, "TML"  , 0x00, decode_tml);

  AddInstTable(InstTable, "RES",  0, DecodeRES);
  AddInstTable(InstTable, "DS",   0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, decode_data_pps4);
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
 * \fn     make_code_pps4(void)
 * \brief  machine instruction dispatcher
 * ------------------------------------------------------------------------ */

static void make_code_pps4(void)
{
  CodeLen = 0; DontPrint = False;

  /* to be ignored */

  if (Memo("")) return;

  /* pseudo instructions */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_pps4(void)
 * \brief  cleanups after switch from target
 * ------------------------------------------------------------------------ */

static void switch_from_pps4(void)
{
  deinit_fields();
}

/*!------------------------------------------------------------------------
 * \fn     is_def_cp_pps4(void)
 * \brief  does instruction use label field?
 * ------------------------------------------------------------------------ */

static Boolean is_def_pps4(void)
{
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_pps4(void)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_pps4(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("PPS-4");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegLimits[SegCode] = 0xfff;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegLimits[SegData] = 0xfff;

  MakeCode = make_code_pps4;
  SwitchFrom = switch_from_pps4;
  IsDef = is_def_pps4;
  init_fields();
}

/*!------------------------------------------------------------------------
 * \fn     codepps4_init(void)
 * \brief  register PPS-4 target
 * ------------------------------------------------------------------------ */

void codepps4_init(void)
{
  (void)AddCPU("PPS-4"    , switch_to_pps4);
}
