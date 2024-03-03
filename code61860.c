/* code61860.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Sharp SC61860                                               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "nls.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
/* #include "onoff_common.h" */
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "motpseudo.h"
#include "headids.h"
#include "errmsg.h"
#include "chartrans.h"

#include "code61860.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPUSC61860;

#define JR_PLUS 0x0100
#define JR_MINUS 0x0200

/*---------------------------------------------------------------------------*/

static void DecodeImm8(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
		if (!OK) return;

		BAsmCode[0] = Index;
		BAsmCode[1] = val;
		CodeLen = 2;
	}
}

static void DecodeImm16(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
		if (!OK) return;

		BAsmCode[0] = Index;
		BAsmCode[1] = val >> 8;
		BAsmCode[2] = val & 0xff;
		CodeLen = 3;
	}
}

static void DecodeLP(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], UInt6, &OK);
		if (!OK) return;

		BAsmCode[0] = Index | val;
		CodeLen = 1;
	}
}

static void DecodeFixed(Word Index)
{
	if (ChkArgCnt(0,0))
	{
		BAsmCode[0] = Index;
		CodeLen = 1;
	}
}

static void DecodeRel(Word Index)
{
	Word OpCode = Index & 0x00ff;
	LongInt disp;
	Boolean ValOK;
	tSymbolFlags flags;

	if (ChkArgCnt(1,1))
	{
		disp = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &ValOK, &flags) - (EProgCounter() + 1);

		if (!mFirstPassUnknownOrQuestionable(flags))
		{
			if (disp < -255 || disp > 255)
			{
				WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
				return;
			}

			if ((disp < 0 && (Index & JR_PLUS)) ||
				(disp > 0 && (Index & JR_MINUS)))
			{
				WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
				return;
			}

			if (disp < 0)
			{
				OpCode |= 0x01;
				disp = -disp;
			}
		}

		BAsmCode[0] = OpCode;
		BAsmCode[1] = disp;
		CodeLen = 2;
	}
}

static void DecodeCAL(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
		if (!OK) return;

		if (val > 0x1fff)
		{
			WrStrErrorPos(ErrNum_NoShortAddr, &ArgStr[1]);
			return;
		}

		BAsmCode[0] = Index | (val >> 8);
		BAsmCode[1] = val & 0xff;
		CodeLen = 2;
	}
}

/*---------------------------------------------------------------------------*/

static void AddImm8(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeImm8);
}

static void AddImm16(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeImm16);
}

static void AddFixed(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddRel(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRel);
}

static void InitFields(void)
{
	InstTable = CreateInstTable(163);

	/* [1] Data Transfer */

	AddImm8("LII",  0x00);
	AddImm8("LIJ",  0x01);
	AddImm8("LIA",  0x02);
	AddImm8("LIB",  0x03);
	AddImm16("LIDP", 0x10);
	AddImm8("LIDL", 0x11);
	AddImm8("LIP", 0x12);
	AddImm8("LIQ", 0x13);
	AddInstTable(InstTable, "LP", 0x80, DecodeLP);

	AddFixed("LDP", 0x20);
	AddFixed("LDQ", 0x21);
	AddFixed("LDR", 0x22);
	AddFixed("STP", 0x30);
	AddFixed("STQ", 0x31);
	AddFixed("STR", 0x32);

	AddFixed("STD", 0x52);
	AddFixed("LDD", 0x57);
	AddFixed("LDM", 0x59);

	AddFixed("MVDM", 0x53);
	AddFixed("MVMD", 0x55);

	AddFixed("EXAB", 0xda);
	AddFixed("EXAM", 0xdb);

	AddFixed("MVW", 0x08);
	AddFixed("MVB", 0x0a);
	AddFixed("MVWD", 0x18);
	AddFixed("MVBD", 0x1a);
	AddFixed("RST", 0x35);

	AddFixed("EXW", 0x09);
	AddFixed("EXB", 0x0b);
	AddFixed("EXWD", 0x19);
	AddFixed("EXBD", 0x1b);

	AddFixed("INCI", 0x40);	
	AddFixed("DECI", 0x41);	
	AddFixed("INCA", 0x42);	
	AddFixed("DECA", 0x43);	
	AddFixed("INCK", 0x48);	
	AddFixed("DECK", 0x49);	
	AddFixed("INCM", 0x4a);	
	AddFixed("DECM", 0x4b);	
	AddFixed("INCP", 0x50);	
	AddFixed("DECP", 0x51);	
	AddFixed("INCJ", 0xc0);	
	AddFixed("DECJ", 0xc1);	
	AddFixed("INCB", 0xc2);	
	AddFixed("DECB", 0xc3);	
	AddFixed("INCL", 0xc8);	
	AddFixed("DECL", 0xc9);	
	AddFixed("INCN", 0xca);	
	AddFixed("DECN", 0xcb);	

	AddFixed("IX", 0x04);	
	AddFixed("DX", 0x05);	
	AddFixed("IY", 0x06);	
	AddFixed("DY", 0x07);	

	AddFixed("IXL", 0x24);  	
	AddFixed("DXL", 0x25);  	
	AddFixed("IYS", 0x26);  	
	AddFixed("DYS", 0x27);  	

	AddFixed("FILM", 0x1e);  	
	AddFixed("FILD", 0x1f);  	

	/* [2] Arithmetic / Logic / Shift */
	
	AddImm8("ADIA", 0x74);
	AddImm8("SBIA", 0x75);

	AddImm8("ADIM", 0x70);
	AddImm8("SBIM", 0x71);

	AddFixed("ADM", 0x44);
	AddFixed("SBM", 0x45);
	
	AddFixed("ADCM", 0xc4);
	AddFixed("SBCM", 0xc5);
	
	AddFixed("ADB", 0x14);
	AddFixed("SBB", 0x15);
	
	AddFixed("ADN", 0x0c);
	AddFixed("SBN", 0x0d);
	AddFixed("ADW", 0x0e);
	AddFixed("SBW", 0x0f);
	
	AddFixed("SRW", 0x1c);
	AddFixed("SLW", 0x1d);
	
	AddFixed("ORMA", 0x47);
	AddImm8("ORIM", 0x61);
	AddImm8("ORIA", 0x65);
	AddImm8("ORID", 0xd5);

	AddFixed("ANMA", 0x46);
	AddImm8("ANIM", 0x60);
	AddImm8("ANIA", 0x64);
	AddImm8("ANID", 0xd4);

	AddImm8("TSIM", 0x62);
	AddImm8("TSIA", 0x66);
	AddImm8("TSID", 0xd6);
	AddFixed("TSMA", 0xc6);
		
	AddImm8("CPIM", 0x63);
	AddImm8("CPIA", 0x67);
	AddFixed("CPMA", 0xc7);

	AddFixed("SWP", 0x58);

	AddFixed("SL", 0x5a);
	AddFixed("SR", 0xd2);

	AddFixed("SC", 0xd0);
	AddFixed("RC", 0xd1);

	/* [3] Jump */

	AddRel("JRNZ", 0x28);
	AddRel("JRNZP", 0x28 | JR_PLUS);
	AddRel("JRNZM", 0x29 | JR_MINUS);
	AddRel("JRNC", 0x2a);
	AddRel("JRNCP", 0x2a | JR_PLUS);
	AddRel("JRNCM", 0x2b | JR_MINUS);
	AddRel("JR", 0x2c);
	AddRel("JRP", 0x2c | JR_PLUS);
	AddRel("JRM", 0x2d | JR_MINUS);
	AddRel("JRZ", 0x38);
	AddRel("JRZP", 0x38 | JR_PLUS);
	AddRel("JRZM", 0x39 | JR_MINUS);
	AddRel("JRC", 0x3a);
	AddRel("JRCP", 0x3a | JR_PLUS);
	AddRel("JRCM", 0x3b | JR_MINUS);

	AddImm16("JP", 0x79);
	AddImm16("JPNZ", 0x7c);
	AddImm16("JPNC", 0x7d);
	AddImm16("JPZ", 0x7e);
	AddImm16("JPC", 0x7f);

	/* [4] Other */

	AddRel("LOOP", 0x2f | JR_MINUS);
	AddFixed("PUSH", 0x34);
	AddFixed("RTN", 0x37);
	AddFixed("INA", 0x4c);
	AddFixed("NOPW", 0x4d);
	AddImm8("WAIT", 0x4e);
	AddFixed("POP", 0x5b);
	AddFixed("OUTA", 0x5d);
	AddFixed("OUTF", 0x5f);
	AddImm8("TEST", 0x6b);
	AddImm16("CALL", 0x78);
	AddFixed("INB", 0xcc);
	AddFixed("NOPT", 0xce);
	AddFixed("LEAVE", 0xd8);
	AddFixed("OUTB", 0xdd);
	AddFixed("OUTC", 0xdf);
	AddInstTable(InstTable, "CAL", 0xe0, DecodeCAL);

	AddFixed("PTC", 0x7a);
	AddFixed("DTC", 0x69);

	/* Undocumented Instruction described in 「ポケコン・マシン語ブック」 */
	AddFixed("MVWP", 0x35);
	AddFixed("IPXL", 0x4f);
	AddFixed("IPXH", 0x6f);
	AddFixed("CLRA", 0x23);
	AddFixed("MVMP", 0x54);
	AddFixed("LDPC", 0x56);

	/* Found in PC-1350 Machine Language Reference Manual */
	AddFixed("CASE1", 0x7a);
	AddFixed("CASE2", 0x69);
	AddFixed("CUP", 0x4f);
	AddFixed("CDN", 0x6f);
	
	init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
}

static void DeinitFields(void)
{
	DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_SC61860(void)
{
	CodeLen = 0;
	DontPrint = False;

	if (Memo("")) return;

/*	if (DecodeIntelPseudo(False)) return;	*/
	if (DecodeMoto16Pseudo(eSymbolSize8Bit, True)) return;

	if (!LookupInstTable(InstTable, OpPart.str.p_str))
		WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_SC61860(void)
{
	return False;
}

static void SwitchFrom_SC61860(void)
{
	DeinitFields();
}

static void SwitchTo_SC61860(void)
{
 	const TFamilyDescr *pDescr;
	TurnWords = False;
	SetIntConstMode(eIntConstModeMoto);

	pDescr = FindFamilyByName("SC61860");
	PCSymbol = "*";
	HeaderID = pDescr->Id;
	NOPCode = 0x4d;	/* NOPW */
	DivideChars = ",";
	HasAttrs = False;

	ValidSegs = (1 << SegCode) | (1 << SegReg);
	Grans[SegCode] = 1;
	ListGrans[SegCode] = 1;
	SegInits[SegCode] = 0x0000;
	SegLimits[SegCode] = 0xffff;
	Grans[SegReg] = 1;
	ListGrans[SegReg] = 1;
	SegInits[SegReg] = 0x00;
	SegLimits[SegReg] = 0x5f;

	MakeCode = MakeCode_SC61860;
	IsDef = IsDef_SC61860;
	SwitchFrom = SwitchFrom_SC61860;
	InitFields();
}

void code61860_init(void)
{
	CPUSC61860 = AddCPU("SC61860", SwitchTo_SC61860);
}
