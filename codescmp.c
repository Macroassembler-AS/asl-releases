/* codescmp.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator National SC/MP                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "intpseudo.h"
#include "codevars.h"
#include "codepseudo.h"
#include "errmsg.h"
#include "headids.h"

#include "codescmp.h"

/*---------------------------------------------------------------------------*/

#define REG_PC 0
#define REG_E 0x80

static CPUVar CPUSCMP;

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *p_arg, tRegInt *p_result, tSymbolSize *p_size)
 * \brief  check whether argument is a CPU register
 * \param  p_arg source argument
 * \param  p_result resulting register #
 * \param  p_size register's size
 * \return True if argument is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *p_arg, tRegInt *p_result, tSymbolSize *p_size)
{
  int l = strlen(p_arg);

  switch (l)
  {
    case 2:
      if (as_toupper(*p_arg) != 'P')
        return False;
      p_arg++;
      switch (as_toupper(*p_arg))
      {
        case '0':
        case '1':
        case '2':
        case '3':
          *p_result = *p_arg - '0';
          *p_size = eSymbolSize16Bit;
          return True;
        case 'C':
          *p_result = REG_PC | REGSYM_FLAG_ALIAS;
          *p_size = eSymbolSize16Bit;
          return True;
        default:
          break;
      }
      break;
    case 1:
      if (as_toupper(*p_arg) == 'E')
      {
        *p_result = REG_E;
        *p_size = eSymbolSize8Bit;
        return True;
      }
      break;
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *p_arg, Byte *p_value, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
 * \brief  check whether argument is a CPU register
 * \param  p_arg source argument
 * \param  p_value resulting register # if yes
 * \param  p_size resulting register's size
 * \param  req_size size of requested register
 * \param  must_be_reg is a register argument expected?
 * \return eval result
 * ------------------------------------------------------------------------ */

static Boolean ChkRegSize(tSymbolSize req_size, tSymbolSize act_size)
{
  return (req_size == eSymbolSizeUnknown)
      || (req_size == act_size);
}

static tRegEvalResult DecodeReg(const tStrComp *p_arg, Byte *p_value, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
{
  tRegDescr reg_descr;
  tEvalResult eval_result;
  tRegEvalResult reg_eval_result;

  if (DecodeRegCore(p_arg->str.p_str, &reg_descr.Reg, &eval_result.DataSize))
    reg_eval_result = eIsReg;
  else
    reg_eval_result = EvalStrRegExpressionAsOperand(p_arg, &reg_descr, &eval_result, eSymbolSizeUnknown, must_be_reg);

  if (reg_eval_result == eIsReg)
  {
    if (!ChkRegSize(req_size, eval_result.DataSize))
    {
      WrStrErrorPos(ErrNum_InvOpSize, p_arg);
      reg_eval_result = must_be_reg ? eIsNoReg : eRegAbort;
    }
  }

  *p_value = reg_descr.Reg;
  if (eval_result.DataSize == eSymbolSize16Bit)
    *p_value &= ~REGSYM_FLAG_ALIAS;
  if (p_size) *p_size = eval_result.DataSize;
  return reg_eval_result;
}

/*!------------------------------------------------------------------------
 * \fn     decode_ptr_reg(const tStrComp *p_arg, Byte *p_result)
 * \brief  parse pointer register expression
 * \param  p_arg source argument
 * \param  p_result result buffer
 * \return eIsReg/eRegAbort
 * ------------------------------------------------------------------------ */

static tRegEvalResult decode_ptr_reg(const tStrComp *p_arg, Byte *p_result)
{
  tRegEvalResult result = DecodeReg(p_arg, p_result, NULL, eSymbolSize16Bit, False);

  /* Pointer register may be named or plain number from 0..3: */

  if (eIsNoReg == result)
  {
    Boolean ok;

    *p_result = EvalStrIntExpression(p_arg, UInt2, &ok);
    result = ok ? eIsReg : eRegAbort;
  }

  return result;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_SCMP(char *p_dest, size_t dest_dize, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - SC/MP variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void DissectReg_SCMP(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize8Bit:
      if (value == REG_E)
      {
        strmaxcpy(p_dest, "E", dest_size);
        break;
      }
      else
        goto unknown;
    case eSymbolSize16Bit:
      if (value == (REG_PC | REGSYM_FLAG_ALIAS))
        strmaxcpy(p_dest, "PC", dest_size);
      else
        as_snprintf(p_dest, dest_size, "P%u", (unsigned)value);
      break;
    unknown:
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, Boolean MayInc, Byte PCDisp, Byte *Arg)
 * \brief  decode address expression
 * \param  pArg source argument
 * \param  MayInc allow auto-increment?
 * \param  PCDisp additional offset to take into account for PC-relative addressing
 * \param  Arg returns m|ptr for opcode byte
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr(const tStrComp *pArg, Boolean MayInc, Byte PCDisp, Byte *Arg)
{
  Word Target;
  Boolean OK;
  int l, SplitPos;
  tSymbolFlags Flags;
  String ArgStr;
  tStrComp ArgCopy;

  StrCompMkTemp(&ArgCopy, ArgStr, sizeof(ArgStr));
  StrCompCopy(&ArgCopy, pArg);
  if (((SplitPos = FindDispBaseSplit(ArgCopy.str.p_str, &l)) >= 0) && (l >= 4))
  {
    tStrComp Left, Right;

    StrCompSplitRef(&Left, &Right, &ArgCopy, ArgCopy.str.p_str + SplitPos);
    StrCompShorten(&Right, 1);

    if (decode_ptr_reg(&Right, Arg) != eIsReg)
      return False;

    if (*Left.str.p_str == '@')
    {
      if (!MayInc)
      {
        WrError(ErrNum_InvAddrMode);
        return False;
      }
      StrCompIncRefLeft(&Left, 1);
      *Arg += 4;
    }
    /* Programmer's manual says that 'Auto-indexing requires ..., and a pointer register (other than PC)...' : */
    if (*Arg == (4 | REG_PC))
    {
      WrStrErrorPos(ErrNum_InvReg, &Right);
      return False;
    }
    switch (DecodeReg(&Left, &BAsmCode[1], NULL, eSymbolSize8Bit, False))
    {
      case eIsReg:
        /* 0x80 -> use E register only applies if pointer register is not P0(PC): */
        if (*Arg == REG_PC)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &Left);
          return False;
        }
        break;
      case eRegAbort:
        return False;
      default:
      {
        tEvalResult result;
        BAsmCode[1] = EvalStrIntExpressionWithResult(&Left, SInt8, &result);
        if (!result.OK)
          return False;
        /* Depending on pointer register, valid range is -128...+127 or -127...+127: */
        if ((*Arg != REG_PC) && (BAsmCode[1] == 0x80) && !mFirstPassUnknownOrQuestionable(result.Flags))
          WrStrErrorPos(ErrNum_MeansE, &Left);
      }
    }
    return True;
  }

  /* no carry in PC from bit 11 to 12; additionally handle preincrement */

  Target = EvalStrIntExpressionWithFlags(pArg, UInt16, &OK, &Flags);
  if (OK)
  {
    Word PCVal = (EProgCounter() & 0xf000) + ((EProgCounter() + 1 + PCDisp) & 0xfff);
    Word Disp = (Target - PCVal) & 0xfff;

    if (mSymbolQuestionable(Flags))
      Target = PCVal;

    if (!ChkSamePage(Target, PCVal, 12, Flags));

    /* Since the pointer register is P0(PC) in this case, a displacement of 0x80
       (-128) does not signify usage of E register, and can be used at
       this place: */

    else if ((Disp > 0x7f) && (Disp < 0xf80)) WrError(ErrNum_DistTooBig);
    else
    {
      BAsmCode[1] = Disp & 0xff;
      *Arg = REG_PC;
      return True;
    }
  }
  return False;
}

static void ChkPage(void)
{
  if (((EProgCounter()) & 0xf000) != ((EProgCounter() + CodeLen) & 0xf000))
    WrError(ErrNum_PageCrossing);
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Index; CodeLen = 1;
  }
}

static void DecodeImm(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = Index; CodeLen = 2; ChkPage();
    }
  }
}

static void DecodeRegOrder(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (decode_ptr_reg(&ArgStr[1], BAsmCode + 0) != eIsReg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] |= Index; CodeLen = 1;
  }
}

static void DecodeMem(Word Index)
{
  if (ChkArgCnt(1, 1))
  if (DecodeAdr(&ArgStr[1], True, 0, BAsmCode + 0))
  {
    BAsmCode[0] |= Index; CodeLen = 2; ChkPage();
  }
}

static void DecodeJmp(Word Index)
{
  if (ChkArgCnt(1, 1))
  if (DecodeAdr(&ArgStr[1], False, 1, BAsmCode + 0))
  {
    BAsmCode[0] |= Index; CodeLen = 2; ChkPage();
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_js(Word Index)
 * \brief  decode JS (macro) instruction
 * \return 
 * ------------------------------------------------------------------------ */

static void decode_js(Word Index)
{
  Byte reg;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (decode_ptr_reg(&ArgStr[1], &reg) != eIsReg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    Boolean ok;
    Word address = EvalStrIntExpression(&ArgStr[2], UInt16, &ok);

    if (ok)
    {
      address--;
      /* LDI H(address) */
      BAsmCode[CodeLen++] = 0xc4;
      BAsmCode[CodeLen++] = Hi(address);
      /* XPAH Pn */
      BAsmCode[CodeLen++] = 0x34 + reg;
      /* LDI L(address) */
      BAsmCode[CodeLen++] = 0xc4;
      BAsmCode[CodeLen++] = Lo(address);
      /* XPAL Pn */
      BAsmCode[CodeLen++] = 0x30 + reg;
      /* XPPC Pn */
      BAsmCode[CodeLen++] = 0x3c + reg;
    }
  }
}

static void DecodeLD(Word Index)
{
  if (ChkArgCnt(1, 1))
  if (DecodeAdr(&ArgStr[1], False, 0, BAsmCode + 0))
  {
    BAsmCode[0] |= Index; CodeLen = 2; ChkPage();
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddImm(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm);
}

static void AddReg(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRegOrder);
}

static void AddMem(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeMem);
}

static void AddJmp(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  add_null_pseudo(InstTable);

  AddFixed("LDE" ,0x40); AddFixed("XAE" ,0x01); AddFixed("ANE" ,0x50);
  AddFixed("ORE" ,0x58); AddFixed("XRE" ,0x60); AddFixed("DAE" ,0x68);
  AddFixed("ADE" ,0x70); AddFixed("CAE" ,0x78); AddFixed("SIO" ,0x19);
  AddFixed("SR"  ,0x1c); AddFixed("SRL" ,0x1d); AddFixed("RR"  ,0x1e);
  AddFixed("RRL" ,0x1f); AddFixed("HALT",0x00); AddFixed("CCL" ,0x02);
  AddFixed("SCL" ,0x03); AddFixed("DINT",0x04); AddFixed("IEN" ,0x05);
  AddFixed("CSA" ,0x06); AddFixed("CAS" ,0x07); AddFixed("NOP" ,0x08);

  AddImm("LDI" , 0xc4); AddImm("ANI" , 0xd4); AddImm("ORI" , 0xdc);
  AddImm("XRI" , 0xe4); AddImm("DAI" , 0xec); AddImm("ADI" , 0xf4);
  AddImm("CAI" , 0xfc); AddImm("DLY" , 0x8f);

  AddReg("XPAL", 0x30); AddReg("XPAH", 0x34); AddReg("XPPC", 0x3c);

  AddMem("LD"  , 0xc0); AddMem("ST"  , 0xc8); AddMem("AND" , 0xd0);
  AddMem("OR"  , 0xd8); AddMem("XOR" , 0xe0); AddMem("DAD" , 0xe8);
  AddMem("ADD" , 0xf0); AddMem("CAD" , 0xf8);

  AddJmp("JMP" , 0x90); AddJmp("JP"  , 0x94); AddJmp("JZ"  , 0x98);
  AddJmp("JNZ" , 0x9c);
  AddInstTable(InstTable, "JS", 0x00, decode_js);

  AddInstTable(InstTable, "ILD", 0xa8, DecodeLD);
  AddInstTable(InstTable, "DLD", 0xb8, DecodeLD);
  AddInstTable(InstTable, "REG" , 0, CodeREG);

  AddIntelPseudo(InstTable, eIntPseudoFlag_DynEndian);
  AddInstTable(InstTable, "BYTE", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt, DecodeIntelDB);
  AddInstTable(InstTable, "DBYTE", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt, DecodeIntelDW);
  AddInstTable(InstTable, ".BYTE", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt, DecodeIntelDB);
  AddInstTable(InstTable, ".DBYTE", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt, DecodeIntelDW);
  AddInstTable(InstTable, "ASCII", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowString, DecodeIntelDB);
  AddInstTable(InstTable, ".ASCII", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowString, DecodeIntelDB);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_SCMP(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for SC/MP
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_SCMP(char *pArg, TempResult *pResult)
{
  if (DecodeRegCore(pArg, &pResult->Contents.RegDescr.Reg, &pResult->DataSize))
  {
    pResult->Typ = TempReg;
    pResult->Contents.RegDescr.Dissect = DissectReg_SCMP;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void MakeCode_SCMP(void)
{
  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_SCMP(void)
{
  return Memo("REG");
}

static void SwitchTo_SCMP(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("SC/MP");

  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x08;
  DivideChars = ",";
  HasAttrs = False;
  AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_SCMP; IsDef = IsDef_SCMP;
  SwitchFrom = DeinitFields; InitFields();

  onoff_bigendian_add();

  QualifyQuote = QualifyQuote_SingleQuoteConstant;
  DissectReg = DissectReg_SCMP;
  InternSymbol = InternSymbol_SCMP;

  IntConstModeIBMNoTerm = True;
}

void codescmp_init(void)
{
  CPUSCMP = AddCPU("SC/MP", SwitchTo_SCMP);
}
