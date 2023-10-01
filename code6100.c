/* code6100.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Intersil IM6100                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "nls.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmitree.h"
#include "literals.h"
#include "codevars.h"
#include "codepseudo.h"
#include "headids.h"
#include "errmsg.h"
#include "onoff_common.h"

#include "code6100.h"

/*---------------------------------------------------------------------------*/

#define INSTINFO_COUNT 76

static CPUVar CPU6100;
static CPUVar CPU6120;


static Boolean BaseInst;

/* TODO: Not used anywhere? */

#if 0
static PInstTable InstTableOP[3]; /* Operate Instruction */
static const Word Inst6120 =  0x8000;
static const Word PanelOnly = 0x4000;
static const Word NormOnly =  0x2000;
#endif

#define F_6120  0x0001

#define F_Norm  0x0040	/* Normal mode only */
#define F_Panel 0x0080	/* Panel mode only */
#define F_Combi 0x1000
#define F_GP1   0x0800
#define F_GP2   0x0400
#define F_GP3   0x0200
#define F_GP(n) (0x0800>>(n))

static LongInt IBVal;

static Boolean PanelMode;
static unsigned int mregistered = 0;

static ASSUMERec ASSUME6100[] =
{
	{"IB", &IBVal, 0, 7, 0xff, NULL},
};

static void DecodeMR(Word Index);
static void DecodeOP(Word Index);
static void DecodeIOT(Word Index);
static void DecodeFix(Word Index);
static void DecodeMEI(Word Index);
static void DecodeRadix(Word Index);
static void DecodeDC(Word Index);
static void DecodeDS(Word Index);
static void DecodeLTORG(Word Index);

typedef struct
{
	char NName[8];
	InstProc Proc;
	Word NCode[3];
	Word Flags;
} tInstInfo;

static tInstInfo *InstInfo;

/*---------------------------------------------------------------------------*/

static Boolean ChkValidInst(Word Index)
{
	Word Flags = InstInfo[Index].Flags;

	if (!(Flags & F_Combi) && !BaseInst)
	{
		WrError(ErrNum_InvCombination);
		return False;
	}

	if (Flags & F_6120)
	{
		if (MomCPU < CPU6120)
		{
			WrError(ErrNum_InstructionNotSupported);
			return False;
		}
	}

	if (Flags & F_Panel)
	{
		if (!PanelMode)
		{
			WrStrErrorPos(ErrNum_NotInNormalMode, &OpPart);
		}
	}
	if (Flags & F_Norm)
	{
		if (PanelMode)
		{
			WrStrErrorPos(ErrNum_NotInPanelMode, &OpPart);
		}
	}

	return True;
}

/*---------------------------------------------------------------------------*/

static void DecodeMR(Word Index)
{
	Boolean Indirect = False;
	Boolean CurrPage = True;
	Boolean Literal = False;
	Word adrPos = 1;
	Integer Adr;
	Boolean OK;
	tSymbolFlags Flags;
	Word NCode = InstInfo[Index].NCode[0];

	if (!ChkValidInst(Index)) return;

	if (!ChkArgCnt(1, 2)) return;

	if (ArgCnt == 2)
	{
		size_t i;

		for (i = 0; i < strlen(ArgStr[1].str.p_str); i++)
		{
			switch (as_toupper(ArgStr[1].str.p_str[i]))
			{
				case 'I':
					Indirect = True;
					break;
				case 'Z':
					CurrPage = False;
					break;
				case 'L':
					Literal = True;
					break;
				default:
					WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
					return;
			}
		}
		if (!CurrPage && Literal)
		{
			WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
			return;
		}
		if (Literal && !Indirect && (NCode == 02000 || NCode == 03000))
		{
			WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
			return;
		}
		adrPos = 2;
	}

	Adr = EvalStrIntExpressionWithFlags(&ArgStr[adrPos], Int16, &OK, &Flags);
	if (!OK) return;

	if (Literal)
	{
		Boolean Critical = mFirstPassUnknown(Flags) || mUsesForwards(Flags);
		tStrComp LComp;
		String LStr;

		if (!mFirstPassUnknownOrQuestionable(Flags) && !ChkRange(Adr, -2048, 4095)) return;

		StrCompMkTemp(&LComp, LStr, sizeof(LStr));

    literal_make(&LComp, NULL, Adr, eSymbolSize12Bit, Critical);
		Adr = EvalStrIntExpressionWithFlags(&LComp, UInt12, &OK, &Flags);
		if (!OK) return;

		Adr |= EProgCounter() & 070000; /* Add current field to pass checks */
	}

	if (!mFirstPassUnknownOrQuestionable(Flags))
	{
		if (!ChkRange(Adr, 0, 32767)) return;

		if ((NCode == 04000 || NCode == 05000) && !Indirect)
		{
			if (IBVal == ASSUME6100[0].NothingVal)
			{
				if ((Adr & 070000) != (EProgCounter() & 070000))
				{
					/* Address not in current field */
					WrError(ErrNum_TargInDiffField);
					return;
				}
			}
			else
			{
				if ((Adr & 070000) != (IBVal << 12))
				{
					/* Address not in IB field */
					WrError(ErrNum_InAccFieldErr);
					return;
				}
			}
		}
		else
		{
			if ((Adr & 070000) != (EProgCounter() & 070000))
			{
				/* Address not in current field */
				WrError(ErrNum_TargInDiffField);
				return;
			}
		}

		if (!CurrPage && (Adr & 07600))
		{
			/* Not in ZERO page */
			WrError(ErrNum_InAccPageErr);
			return;
		}
		if (!(Adr & 07600))
		{
			/* Zero page */
			CurrPage = False;
		}
		else if ((Adr & 07600) != (EProgCounter() & 07600))
		{
			/* Not in current page */
			WrError(ErrNum_TargOnDiffPage);
			return;
		}
	}

	WAsmCode[0] = NCode | (Indirect ? 00400 : 0) | (CurrPage ? 00200 : 0) | (Adr & 00177);
	CodeLen = 1;
}

static Word CCode[3];

static void DecodeOP(Word Index)
{
	Word i;
	Word Flags = InstInfo[Index].Flags;

	if (!ChkValidInst(Index)) return;

	if (BaseInst)
	{
		for (i = 0; i < 3; i++) CCode[i] = 0;
		if (Flags & F_GP1) CCode[0] = InstInfo[Index].NCode[0];
		if (Flags & F_GP2) CCode[1] = InstInfo[Index].NCode[1];
		if (Flags & F_GP3) CCode[2] = InstInfo[Index].NCode[2];

		BaseInst = False;
		for (i = 1; i <= ArgCnt; i++)
		{
			NLS_UpString(ArgStr[i].str.p_str);
			if (!LookupInstTable(InstTable, ArgStr[i].str.p_str))
			{
				WrError(ErrNum_UnknownInstruction);
				return;
			}
		}
		for (i = 0; i < 3; i++)
		{
			if (CCode[i])
			{
				WAsmCode[0] = CCode[i];
				CodeLen = 1;
				return;
			}
		}
		WrError(ErrNum_InvCombination);
		return;
	}
	else
	{
		Word NCode;
		Word tmp;

		for (i = 0; i < 3; i++)
		{
			if (!CCode[i]) continue;

			if (!(Flags & F_GP(i)))
			{
				CCode[i] = 0;
				continue;
			}

			NCode = InstInfo[Index].NCode[i];

			tmp = CCode[i] | NCode;
      /* non-base instruction does not contribute any new bits (duplicate instruction?) */
			if (tmp == CCode[i] || tmp == NCode)
			{
				CCode[i] = 0;
				return;
			}

			switch (i)
			{
			case 0:
        /* Group 1: RAR, RAL, RTR, RTL, and BSW cannot be used in same instruction */
				if ((CCode[i] & 00016) && (NCode & 00016))
				{
					CCode[0] = 0;
					return;
				}
				break;
			case 1:
        /* Group 2: SMA|SZA|SNL and SPA|SNA|SZL cannot be used in same instruction */
				if ((CCode[i] & 00160) && (NCode & 00160) && ((CCode[i] & 00010) != (NCode & 00010)))
				{
					CCode[i] = 0;
					return;
				}
				break;
			}

			CCode[i] = tmp;
		}
	}
}

static void DecodeIOT(Word Index)
{
	Word dev;
	Word opr;
	Boolean OK;
	Word NCode = InstInfo[Index].NCode[0];

	if (!ChkValidInst(Index)) return;

	if (!ChkArgCnt(2, 2)) return;

	dev = EvalStrIntExpression(&ArgStr[1], UInt6, &OK);
	if (!OK) return;

	opr = EvalStrIntExpression(&ArgStr[2], UInt3, &OK);
	if (!OK) return;

	WAsmCode[0] = NCode | (dev << 3) | opr;
	CodeLen = 1;
}

static void DecodeFix(Word Index)
{
	Word NCode = InstInfo[Index].NCode[0];

	if (!ChkValidInst(Index)) return;

	if (!ChkArgCnt(0, 0)) return;

	WAsmCode[0] = NCode;
	CodeLen = 1;
}

static void DecodeMEI(Word Index)
{
	int fieldPos = 1;
	Word field;
	Boolean OK;
	Word NCode = InstInfo[Index].NCode[0];

	if (!ChkValidInst(Index)) return;

	if (!ChkArgCnt(1, 2)) return;

	if (ArgCnt == 2)
	{
		if ((!as_strcasecmp(OpPart.str.p_str, "CDF") && !as_strcasecmp(ArgStr[1].str.p_str, "CIF")) ||
			(!as_strcasecmp(OpPart.str.p_str, "CIF") && !as_strcasecmp(ArgStr[1].str.p_str, "CDF")))
		{
			NCode = 06203;
			fieldPos = 2;
		}
		else
		{
			WrError(ErrNum_UnknownInstruction);
			return;
		}
	}

	field = EvalStrIntExpression(&ArgStr[fieldPos], UInt3, &OK);
	if (!OK) return;

	WAsmCode[0] = NCode | (field << 3);
	CodeLen = 1;
}

static void DecodeRadix(Word Base)
{
	if (!ChkArgCnt(0, 0)) return;

	RadixBase = Base;
}

static Boolean DecodeConst(tStrComp *pStr)
{
	TempResult t;
	tSymbolFlags Flags;
	int c;
	Boolean ret = True;

	as_tempres_ini(&t);
	EvalStrExpression(pStr, &t);
	Flags = t.Flags;
	switch (t.Typ)
	{
		case TempInt:
			if (!mFirstPassUnknownOrQuestionable(Flags))
				if (!ChkRange(t.Contents.Int, -2048, 4095)) break;

			WAsmCode[CodeLen++] = t.Contents.Int & 07777;
			break;
		case TempString:
			as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pStr);
			for (c = 0; c < (int)t.Contents.str.len; c++)
			{
				WAsmCode[CodeLen++] = t.Contents.str.p_str[c] & 07777;
			}
			break;
		default:
			ret = False;
	}
	as_tempres_free(&t);

	return ret;
}

static void DecodeDC(Word Index)
{
	Boolean result = True;
	int i;

	UNUSED(Index);

	for (i = 1; i <= ArgCnt; i++)
	{
		if (result)
		{
			result = DecodeConst(&ArgStr[i]);
		}
	}

	if (!result) CodeLen = 0;
}

static void DecodeDS(Word Index)
{
	Boolean OK;
	tSymbolFlags Flags;
	Word Val;

	UNUSED(Index);

	if (!ChkArgCnt(1, 1)) return;

	Val = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt12, &OK, &Flags);
	if (!OK) return;
	if (mFirstPassUnknown(Flags))
	{
		WrStrErrorPos(ErrNum_FirstPassCalc, &ArgStr[1]);
		return;
	}

	DontPrint = True;
	CodeLen = Val;
	BookKeeping();
}

static LargeInt ltorg_12(const as_literal_t *p_lit, struct sStrComp *p_name)
{
  LargeInt ret;

  SetMaxCodeLen((CodeLen + 1) * 2);
	WAsmCode[CodeLen] = p_lit->value & 07777;
  ret = EProgCounter() + CodeLen;
	EnterIntSymbol(p_name, ret, ActPC, False);
	CodeLen++;
  return ret;
}

static void DecodeLTORG(Word Index)
{

	UNUSED(Index);

	if (!ChkArgCnt(0, 0)) return;

  literals_dump(ltorg_12, eSymbolSize12Bit, MomSectionHandle, False);
}

/* TODO: Not used anywhere? */

#if 0
static Boolean DecodeDefault(tStrComp *OpStr)
{
	Boolean result;
	int i;

	result = DecodeConst(OpStr);

	for (i = 1; i <= ArgCnt; i++)
	{
		if (result)
		{
			result = DecodeConst(&ArgStr[i]);
		}
	}

	if (!result) CodeLen = 0;

	return result;
}
#endif

/*---------------------------------------------------------------------------*/

static void AddInstInfo(const char *pName, InstProc Proc, 
                        Word Code0, Word Code1, Word Code2, Word Flags)
{
  assert(InstrZ < INSTINFO_COUNT);
  InstInfo[InstrZ].NCode[0] = Code0;
  InstInfo[InstrZ].NCode[1] = Code1;
  InstInfo[InstrZ].NCode[2] = Code2;
  InstInfo[InstrZ].Flags = Flags;
  AddInstTable(InstTable, pName, InstrZ++, Proc);
}

static void InitFields(void)
{
	InstTable = CreateInstTable(96);

  InstrZ = 0;
  InstInfo = (tInstInfo*)malloc(INSTINFO_COUNT * sizeof(*InstInfo));

	/* Memory Reference Instruction */
	AddInstInfo("AND", DecodeMR, 00000,     0,     0, 0);
	AddInstInfo("TAD", DecodeMR, 01000,     0,     0, 0);
	AddInstInfo("ISZ", DecodeMR, 02000,     0,     0, 0);
	AddInstInfo("DCA", DecodeMR, 03000,     0,     0, 0);
	AddInstInfo("JMS", DecodeMR, 04000,     0,     0, 0);
	AddInstInfo("JMP", DecodeMR, 05000,     0,     0, 0);

	/* Operate Instruction (Group 1) */
	AddInstInfo("NOP", DecodeOP, 07000, 07400, 07401, F_Combi | F_GP1 | F_GP2 | F_GP3);
	AddInstInfo("IAC", DecodeOP, 07001,     0,     0, F_Combi | F_GP1);
	AddInstInfo("RAL", DecodeOP, 07004,     0,     0, F_Combi | F_GP1);
	AddInstInfo("RTL", DecodeOP, 07006,     0,     0, F_Combi | F_GP1);
	AddInstInfo("R3L", DecodeOP, 07014,     0,     0, F_6120 | F_Combi | F_GP1);
	AddInstInfo("RAR", DecodeOP, 07010,     0,     0, F_Combi | F_GP1);
	AddInstInfo("RTR", DecodeOP, 07012,     0,     0, F_Combi | F_GP1);
	AddInstInfo("BSW", DecodeOP, 07002,     0,     0, F_Combi | F_GP1);
	AddInstInfo("CML", DecodeOP, 07020,     0,     0, F_Combi | F_GP1);
	AddInstInfo("CMA", DecodeOP, 07040,     0,     0, F_Combi | F_GP1);
	AddInstInfo("CIA", DecodeOP, 07041,     0,     0, F_Combi | F_GP1);
	AddInstInfo("CLL", DecodeOP, 07100,     0,     0, F_Combi | F_GP1);
	AddInstInfo("STL", DecodeOP, 07120,     0,     0, F_Combi | F_GP1);
	AddInstInfo("CLA", DecodeOP, 07200, 07600, 07601, F_Combi | F_GP1 | F_GP2 | F_GP3);
	AddInstInfo("GLK", DecodeOP, 07204,     0,     0, F_Combi | F_GP1);
	AddInstInfo("GLT", DecodeOP, 07204,     0,     0, F_Combi | F_GP1);	/* Some documents say "GLT" */
	AddInstInfo("STA", DecodeOP, 07240,     0,     0, F_Combi | F_GP1);

	/* Operate Instruction (Group 2) */
	AddInstInfo("HLT", DecodeOP,     0, 07402,     0, F_Combi | F_GP2);
	AddInstInfo("OSR", DecodeOP,     0, 07404,     0, F_Combi | F_GP2);
	AddInstInfo("SKP", DecodeOP,     0, 07410,     0, F_Combi | F_GP2);
	AddInstInfo("SNL", DecodeOP,     0, 07420,     0, F_Combi | F_GP2);
	AddInstInfo("SZL", DecodeOP,     0, 07430,     0, F_Combi | F_GP2);
	AddInstInfo("SZA", DecodeOP,     0, 07440,     0, F_Combi | F_GP2);
	AddInstInfo("SNA", DecodeOP,     0, 07450,     0, F_Combi | F_GP2);
	AddInstInfo("SMA", DecodeOP,     0, 07500,     0, F_Combi | F_GP2);
	AddInstInfo("SPA", DecodeOP,     0, 07510,     0, F_Combi | F_GP2);
	AddInstInfo("LAS", DecodeOP,     0, 07604,     0, F_Combi | F_GP2);

	/* Operate Instruction (Group 3) */
	AddInstInfo("MQL", DecodeOP,     0,     0, 07421, F_Combi | F_GP3);
	AddInstInfo("MQA", DecodeOP,     0,     0, 07501, F_Combi | F_GP3);
	AddInstInfo("SWP", DecodeOP,     0,     0, 07521, F_Combi | F_GP3);
	AddInstInfo("CAM", DecodeOP,     0,     0, 07621, F_Combi | F_GP3);
	AddInstInfo("ACL", DecodeOP,     0,     0, 07701, F_Combi | F_GP3);

	/* I/O Transfer Instruction */
	AddInstInfo("IOT" , DecodeIOT, 06000,     0,     0, 0);
	AddInstInfo("SKON", DecodeFix, 06000,     0,     0, F_Norm);
	AddInstInfo("ION" , DecodeFix, 06001,     0,     0, 0);
	AddInstInfo("IOF" , DecodeFix, 06002,     0,     0, 0);
	AddInstInfo("SRQ" , DecodeFix, 06003,     0,     0, F_Norm);
	AddInstInfo("GTF" , DecodeFix, 06004,     0,     0, F_Norm);
	AddInstInfo("RTF" , DecodeFix, 06005,     0,     0, 0);
	AddInstInfo("SGT" , DecodeFix, 06006,     0,     0, 0);
	AddInstInfo("CAF" , DecodeFix, 06007,     0,     0, 0);

	/* Stack Operation Instruction (6120 only) */
	AddInstInfo("PPC1", DecodeFix, 06205,    0,     0, F_6120);
	AddInstInfo("PPC2", DecodeFix, 06245,    0,     0, F_6120);
	AddInstInfo("PAC1", DecodeFix, 06215,    0,     0, F_6120);
	AddInstInfo("PAC2", DecodeFix, 06255,    0,     0, F_6120);
	AddInstInfo("RTN1", DecodeFix, 06225,    0,     0, F_6120);
	AddInstInfo("RTN2", DecodeFix, 06265,    0,     0, F_6120);
	AddInstInfo("POP1", DecodeFix, 06235,    0,     0, F_6120);
	AddInstInfo("POP2", DecodeFix, 06275,    0,     0, F_6120);
	AddInstInfo("RSP1", DecodeFix, 06207,    0,     0, F_6120);
	AddInstInfo("RSP2", DecodeFix, 06227,    0,     0, F_6120);
	AddInstInfo("LSP1", DecodeFix, 06217,    0,     0, F_6120);
	AddInstInfo("LSP2", DecodeFix, 06237,    0,     0, F_6120);

	/* Internal Control Instruction */
	AddInstInfo("WSR", DecodeFix, 06246,    0,     0, F_6120);
	AddInstInfo("GCF", DecodeFix, 06256,    0,     0, F_6120);

	/* Main Memory Control Instruction (6120 only) */
	AddInstInfo("PR0", DecodeFix, 06206,    0,     0, F_6120 | F_Norm);
	AddInstInfo("PR1", DecodeFix, 06216,    0,     0, F_6120 | F_Norm);
	AddInstInfo("PR2", DecodeFix, 06226,    0,     0, F_6120 | F_Norm);
	AddInstInfo("PR3", DecodeFix, 06236,    0,     0, F_6120 | F_Norm);

	/* Panel Mode Instruction */
	AddInstInfo("PRS", DecodeFix, 06000,    0,     0, F_Panel);
	AddInstInfo("PG0", DecodeFix, 06003,    0,     0, F_Panel);
	AddInstInfo("PEX", DecodeFix, 06004,    0,     0, F_Panel);
	AddInstInfo("CPD", DecodeFix, 06266,    0,     0, F_Panel);
	AddInstInfo("SPD", DecodeFix, 06276,    0,     0, F_Panel);

	/* Memory Extension Instruction */
	AddInstInfo("CDF", DecodeMEI, 06201,    0,     0, F_6120);
	AddInstInfo("CIF", DecodeMEI, 06202,    0,     0, F_6120);
	AddInstInfo("RDF", DecodeFix, 06214,    0,     0, F_6120);
	AddInstInfo("RIF", DecodeFix, 06224,    0,     0, F_6120);
	AddInstInfo("RIB", DecodeFix, 06234,    0,     0, F_6120);
	AddInstInfo("RMF", DecodeFix, 06244,    0,     0, F_6120);

	/* Pseudo Instruction */
	AddInstTable(InstTable, "DECIMAL", 10, DecodeRadix);
	AddInstTable(InstTable, "OCTAL",    8, DecodeRadix);
	AddInstTable(InstTable, "DC",       0, DecodeDC);
	AddInstTable(InstTable, "DS",       0, DecodeDS);
	AddInstTable(InstTable, "LTORG",    0, DecodeLTORG);
}

static void DeinitFields(void)
{
  free(InstInfo);
	DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_6100(void)
{
	CodeLen = 0;
	DontPrint = False;

	if (Memo("")) return;

	BaseInst = True;
	if (!LookupInstTable(InstTable, OpPart.str.p_str))
		WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_6100(void)
{
	mregistered = 0;

	IBVal = 0;
}

static Boolean IsDef_6100(void)
{
	return False;
}

static void SwitchFrom_6100(void)
{
	DeinitFields();
}

static void SwitchTo_6100(void)
{
	const TFamilyDescr *pDescr;

	TurnWords = True;
	SetIntConstMode(eIntConstModeC);

	pDescr = FindFamilyByName("IM6100");
	PCSymbol = ".";
	HeaderID = pDescr->Id;
	NOPCode = 0x7000;
	DivideChars = " \t";
	HasAttrs = False;

	ValidSegs = (1 << SegCode);
	Grans[SegCode] = 2;
	ListGrans[SegCode] = 2;
	SegInits[SegCode] = 0x0000;
	if (MomCPU >= CPU6120)
		SegLimits[SegCode] = 0x7fff;
	else
		SegLimits[SegCode] = 0x0fff;

	if (!(mregistered & (1 << 0)))
	{
		mregistered |= (1 << 0);
		SetFlag(&PanelMode, "INPANEL", False);
	}
	AddONOFF("PANEL", &PanelMode, "INPANEL", False);

	pASSUMERecs = ASSUME6100;
	ASSUMERecCnt = as_array_size(ASSUME6100);

	MakeCode = MakeCode_6100;
	IsDef = IsDef_6100;
	SwitchFrom = SwitchFrom_6100;
	InitFields();
}

void code6100_init(void)
{
	CPU6100 = AddCPU("6100", SwitchTo_6100);
	CPU6120 = AddCPU("6120", SwitchTo_6100);

	AddInitPassProc(InitCode_6100);

	AddCopyright("Intersil IM6100 Generator (C) 2022 Haruo Asano");
}
