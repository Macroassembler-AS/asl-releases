/* codeuc43.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator NEC uCOM-43/44/45                                          */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "chunks.h"
#include "bpemu.h"
#include "strutil.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codevars.h"
#include "headids.h"
#include "intpseudo.h"
#include "errmsg.h"

#include "codeuc43.h"

/*--------------------------------------------------------------------------*/
/* Local Types */

typedef enum
{
  e_ucom_45,
  e_ucom_44,
  e_ucom_43
} family_t;

typedef struct
{
  char name[8];
  Word rom_end;
  Byte ram_end, family;
} cpu_props_t;

static const cpu_props_t *p_curr_cpu_props;

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void put_code(Word code)
{
  if (Hi(code))
    BAsmCode[CodeLen++] = Hi(code);
  BAsmCode[CodeLen++] = Lo(code);
}

static Boolean chk_family(family_t min_family)
{
  if (p_curr_cpu_props->family < min_family)
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    put_code(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_fixed_uc43(Word code)
 * \brief  handle instructions without arguments, uCOM-43 only
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed_uc43(Word code)
{
  if (chk_family(e_ucom_43))
    decode_fixed(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm2(Word code)
 * \brief  handle instructions with 2 bit immediate argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm2(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;

    code |= (EvalStrIntExpression(&ArgStr[1], UInt2, &ok) & 3);
    if (ok)
      put_code(code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm2_uc43(Word code)
 * \brief  handle instructions with 2 bit immediate argument, uCOM-43 only
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm2_uc43(Word code)
{
  if (chk_family(e_ucom_43))
    decode_imm2(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm4(Word code)
 * \brief  handle instructions with 4 bit immediate argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_imm4(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;

    code |= (EvalStrIntExpression(&ArgStr[1], Int4, &ok) & 15);
    if (ok)
      put_code(code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_stm(Word code)
 * \brief  handle STM instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_stm(Word code)
{
  if (chk_family(e_ucom_43)
   && ChkArgCnt(1, 1))
  {
    Boolean ok;

    code |= (EvalStrIntExpression(&ArgStr[1], Int6, &ok) & 63);
    if (ok)
      put_code(code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ldi(Word code)
 * \brief  handle LDI instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ldi(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Byte address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt7, &eval_result);

    if (eval_result.OK)
    {
      ChkSpace(SegData, eval_result.AddrSpaceMask);
      if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && (address > SegLimits[SegData]))
        WrStrErrorPos(ErrNum_WOverRange, &ArgStr[1]);
      put_code(code | (address & 0x7f));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jmp(Word code)
 * \brief  handle instructions with absolute address argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jmp(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt11, &eval_result);

    if (eval_result.OK)
    {
      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      put_code(code | ((Hi(address) & 7)));
      BAsmCode[CodeLen++] = Lo(address);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jcp(Word code)
 * \brief  handle JCP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jcp(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt11, &eval_result);

    if (eval_result.OK)
    {
      /* PC[0..7] is not auto-incremented after fetching a jump or call
         instruction.  So JCP always remains in the current 64 byte page,
         even if it is located in the last byte of a 64 byte page: */

      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      if (!mFirstPassUnknownOrQuestionable(eval_result.Flags))
      {
        if ((EProgCounter() & 0x7c0) != (address & 0x7c0))
        {
          WrStrErrorPos(ErrNum_TargOnDiffPage, &ArgStr[1]);
          return;
        }
      }
      put_code(code | (address & 0x3f));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_czp(Word code)
 * \brief  handle CZP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_czp(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    Word address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt11, &eval_result);

    if (!eval_result.OK)
      return;

    /* Check whether the argument is an address (00h, 04h, 08h...3ch) or a vector (0..15): */

    /* If the argument is larger than 15, it must be an address (in code space)
       of the form 4*n with n <= 15: */

    if (address > 15)
    {
      ChkSpace(SegCode, eval_result.AddrSpaceMask);
      if (!mFirstPassUnknownOrQuestionable(eval_result.Flags))
      {
        if ((address & 3) || (address > 63))
        {
          WrStrErrorPos(ErrNum_NotFromThisAddress, &ArgStr[1]);
          return;
        }
      }
      address = (address >> 2) & 15;
    }

    /* Otherwise, if the argument is zero, the distinction is irrelevant.  If
       the argument is not a multiple of four, it must be a vector: */

    else if ((address & 3) || !address)
    {
    }

    /* So this leaves 4, 8, and 12 that might be a vector or an address.
       Distinguish by whether the symbol is in code space: */

    else if (eval_result.AddrSpaceMask & (1 << SegCode))
      address = (address >> 2) & 15;

    put_code(code | address);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ocd(Word code)
 * \brief  handle OCD instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ocd(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;
    Byte value = EvalStrIntExpression(&ArgStr[1], Int8, &ok);

    if (ok)
    {
      put_code(code);
      BAsmCode[CodeLen++] = value;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Code Table Handling */

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  set up instruction hash table
 * ------------------------------------------------------------------------ */

static void add_fixed(const char *p_name, Word code)
{
  AddInstTable(InstTable, p_name, code, decode_fixed);
}

static void add_fixed_uc43(const char *p_name, Word code)
{
  AddInstTable(InstTable, p_name, code, decode_fixed_uc43);
}

static void add_imm2(const char *p_name, Word code)
{
  AddInstTable(InstTable, p_name, code, decode_imm2);
}

static void add_imm2_uc43(const char *p_name, Word code)
{
  AddInstTable(InstTable, p_name, code, decode_imm2_uc43);
}

static void add_imm4(const char *p_name, Word code)
{
  AddInstTable(InstTable, p_name, code, decode_imm4);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(203);

  add_fixed("CLA", 0x90);
  add_fixed("CLC", 0x0b);
  add_fixed("CMA", 0x10);
  add_fixed("CIA", 0x11);
  add_fixed("INC", 0x0d);
  add_fixed("DEC", 0x0f);
  add_fixed("STC", 0x1b);
  add_fixed_uc43("XC",  0x1a);
  add_fixed_uc43("RAR", 0x30);
  add_fixed_uc43("INM", 0x1d);
  add_fixed_uc43("DEM", 0x1f);
  add_fixed("AD",  0x08);
  add_fixed("ADS", 0x09);
  add_fixed("ADC", 0x19);
  add_fixed("DAA", 0x06);
  add_fixed("DAS", 0x0a);
  add_fixed("EXL", 0x18);
  add_imm4 ("LI",  0x90);
  add_fixed("S",   0x02);
  add_fixed("L",   0x38); /* == LM 0 */
  add_imm2 ("LM",  0x38);
  add_fixed("X",   0x28); /* == XM 0 */
  add_imm2 ("XM",  0x28);
  add_fixed("XD",  0x2c); /* == XMD 0 */
  add_imm2 ("XMD", 0x2c);
  add_fixed("XI",  0x3c); /* == XMI 0 */
  add_imm2 ("XMI", 0x3c);
  AddInstTable(InstTable, "LDI", 0x1500, decode_ldi);
  add_imm4 ("LDZ", 0x80);
  add_fixed("DED", 0x13);
  add_fixed("IND", 0x33);
  add_fixed("TAL", 0x07);
  add_fixed("TLA", 0x12);
  add_fixed_uc43("XHX", 0x4f);
  add_fixed_uc43("XLY", 0x4e);
  add_fixed_uc43("THX", 0x47);
  add_fixed_uc43("TLY", 0x46);
  add_fixed_uc43("XAZ", 0x4a);
  add_fixed_uc43("XAW", 0x4b);
  add_fixed_uc43("TAZ", 0x42);
  add_fixed_uc43("TAW", 0x43);
  add_fixed_uc43("XHR", 0x4d);
  add_fixed_uc43("XLS", 0x4c);
  add_imm2 ("SMB", 0x78);
  add_imm2 ("RMB", 0x68);
  add_imm2 ("TMB", 0x58);
  add_imm2 ("TAB", 0x24);
  add_imm2 ("CMB", 0x34);
  add_imm2_uc43 ("SFB", 0x7c);
  add_imm2_uc43 ("RFB", 0x6c);
  add_imm2_uc43 ("FBT", 0x5c);
  add_imm2_uc43 ("FBF", 0x20);
  add_fixed("CM",  0x0c);
  add_imm4 ("CI",  0x17c0);
  add_imm4 ("CLI", 0x16e0);
  add_fixed("TC",  0x04);
  add_fixed_uc43("TTM", 0x05);
  add_fixed("TIT", 0x03);
  AddInstTable(InstTable, "JCP", 0xc0, decode_jcp);
  AddInstTable(InstTable, "JMP", 0xa0, decode_jmp);
  add_fixed("JPA", 0x41);
  add_fixed_uc43("EI",  0x31);
  add_fixed_uc43("DI",  0x01);
  AddInstTable(InstTable, "CZP", 0xb0, decode_czp);
  AddInstTable(InstTable, "CAL", 0xa8, decode_jmp);
  add_fixed("RT",  0x48);
  add_fixed("RTS", 0x49);
  add_imm2 ("SEB", 0x74);
  add_imm2 ("REB", 0x64);
  add_imm2 ("SPB", 0x70);
  add_imm2 ("RPB", 0x60);
  add_imm2 ("TPA", 0x54);
  add_imm2 ("TPB", 0x50);
  add_fixed("OE",  0x44);
  add_fixed("OP",  0x0e);
  AddInstTable(InstTable, "OCD", 0x1e, decode_ocd);
  add_fixed("IA",  0x40);
  add_fixed("IP",  0x32);
  AddInstTable(InstTable, "STM", 0x1480, decode_stm);
  add_fixed("NOP", NOPCode);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  dissolve instruction hash table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Callbacks */

/*!------------------------------------------------------------------------
 * \fn     make_code_uc43(void)
 * \brief  transform machine instruction to binary code
 * ------------------------------------------------------------------------ */

static void make_code_uc43(void)
{
  CodeLen = 0;

  DontPrint = False;

  /* Empty Instruction */

  if (!*OpPart.str.p_str)
    return;

  /* Pseudo Instructions */

  if (DecodeIntelPseudo(True))
    return;

  /* via table */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_uc43(void)
 * \brief  check whether instruction consumes label field
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_uc43(void)
{
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_uc43(void)
 * \brief  cleanups when switching away from uCOM-43 target
 * ------------------------------------------------------------------------ */

static void switch_from_uc43(void)
{
  deinit_fields();
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_uc43(void *p_user)
 * \brief  set up for uCOM-43 target
 * \param  points to selected properties
 * ------------------------------------------------------------------------ */

static void switch_to_uc43(void *p_user)
{
  const TFamilyDescr *p_descr;

  p_curr_cpu_props = (const cpu_props_t*)p_user;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  p_descr = FindFamilyByName("uCOM-43");
  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = p_curr_cpu_props->rom_end;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  SegLimits[SegData] = p_curr_cpu_props->ram_end;

  MakeCode = make_code_uc43; IsDef = is_def_uc43;
  SwitchFrom = switch_from_uc43; init_fields();
}

/*--------------------------------------------------------------------------*/
/* Initialisierung */

static const cpu_props_t cpu_props[] =
{
  { "uPD546", 1999, 95, e_ucom_43 },
  { "uPD547",  999, 63, e_ucom_44 },
  { "uPD550",  639, 31, e_ucom_45 },
  { "uPD552",  999, 63, e_ucom_44 },
  { "uPD553", 1999, 95, e_ucom_43 },
  { "uPD554",  999, 31, e_ucom_45 },
  { "uPD556", 1999, 95, e_ucom_43 }, /* ROM is external */
  { "uPD557", 1999, 95, e_ucom_43 },
  { "uPD650", 1999, 95, e_ucom_43 },
  { "uPD651",  999, 63, e_ucom_44 },
  { "uPD652",  999, 31, e_ucom_45 },
  { "",          0,  0, e_ucom_45 }
};

void codeuc43_init(void)
{
  const cpu_props_t *p_run;

  for (p_run = cpu_props; p_run->name[0]; p_run++)
    (void)AddCPUUser(p_run->name, switch_to_uc43, (void*)p_run, NULL);
}
