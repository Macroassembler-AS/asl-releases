/* codenano.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator HP Nanoprocessor                                            */
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
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "headids.h"
#include "errmsg.h"
#include "onoff_common.h"

#include "codenano.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPUNANO;

/*---------------------------------------------------------------------------*/

static void DecodeBit(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
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

static void DecodeImm(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
		if (!OK) return;

		BAsmCode[0] = Index;
		BAsmCode[1] = val;
		CodeLen = 2;
	}
}

static void DecodeReg(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word reg;
		Boolean OK;

		reg = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
		if (!OK) return;

		BAsmCode[0] = Index | reg;
		CodeLen = 1;
	}
}

static void DecodeRegImm(Word Index)
{
	if (ChkArgCnt(2,2))
	{
		Word reg;
		Word val;
		Boolean OK;

		reg = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
		if (!OK) return;

		val = EvalStrIntExpression(&ArgStr[2], UInt8, &OK);
		if (!OK) return;

		BAsmCode[0] = Index | reg;
		BAsmCode[1] = val;
		CodeLen = 2;
	}
}

static void DecodeDev(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word dev;
		Boolean OK;

		dev = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
		if (!OK) return;
		if (Index == 0x50 && dev == 15)	/* OTA 15 not allowed */
		{
			WrError(ErrNum_OverRange);
			return;
		}

		BAsmCode[0] = Index | dev;
		CodeLen = 1;
	}
}

static void DecodeDevImm(Word Index)
{
	if (ChkArgCnt(2,2))
	{
		Word dev;
		Word val;
		Boolean OK;

		dev = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
		if (!OK) return;

		val = EvalStrIntExpression(&ArgStr[2], UInt8, &OK);
		if (!OK) return;

		if (dev == 15)	/* OTR 15,xx not allowed */
		{
			WrError(ErrNum_OverRange);
			return;
		}

		BAsmCode[0] = Index | dev;
		BAsmCode[1] = val;
		CodeLen = 2;
	}
}

static void DecodeDirect(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
		if (!OK) return;
		if (val == 7)
		{
			WrError(ErrNum_OverRange);
			return;
		}

		BAsmCode[0] = Index | val;
		CodeLen = 1;
	}
}

static void DecodeAddress(Word Index)
{
	if (ChkArgCnt(1,1))
	{
		Word val;
		Boolean OK;

		val = EvalStrIntExpression(&ArgStr[1], UInt11, &OK);
		if (!OK) return;

		BAsmCode[0] = Index | (val >> 8);
		BAsmCode[1] = val & 0xff;
		CodeLen = 2;
	}
}


/*---------------------------------------------------------------------------*/

static void AddBit(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void AddFixed(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddImm(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeImm);
}

static void AddReg(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeReg);
}

static void AddRegImm(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRegImm);
}

static void AddDev(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDev);
}

static void AddDevImm(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDevImm);
}

static void AddDirect(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDirect);
}

static void AddAddress(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeAddress);
}

static void InitFields(void)
{
	InstTable = CreateInstTable(49);

	/* A. Accumulator Group */
	AddBit("SBS", 0x10);
	AddBit("SBZ", 0x30);
	AddBit("SBN", 0x20);
	AddBit("CBN", 0xA0);
	AddFixed("INB", 0x00);
	AddFixed("IND", 0x02);
	AddFixed("DEB", 0x01);
	AddFixed("DED", 0x03);
	AddFixed("CLA", 0x04);
	AddFixed("CMA", 0x05);
	AddFixed("LSA", 0x07);
	AddFixed("RSA", 0x06);
	AddFixed("SES", 0x1f);
	AddFixed("SEZ", 0x3f);
	AddImm("LDR", 0xcf);

	/* B. Register Transfer Group */
	AddReg("LDA", 0x60);
	AddReg("STA", 0x70);
	AddReg("LDI", 0xe0);
	AddReg("STI", 0xf0);
	AddRegImm("STR", 0xd0);

	/* C. Extend Register Group */
	AddFixed("STE", 0xb4);
	AddFixed("CLE", 0xb5);

	/* D. Interrupt Group */
	AddFixed("DSI", 0xaf);
	AddFixed("ENI", 0x2f);

	/* E. Comparator Group */
	AddFixed("SLT", 0x09);
	AddFixed("SEQ", 0x0a);
	AddFixed("SAZ", 0x0b);
	AddFixed("SLE", 0x0c);
	AddFixed("SGE", 0x0d);
	AddFixed("SNE", 0x0e);
	AddFixed("SAN", 0x0f);
	AddFixed("SGT", 0x08);

	/* F. Input/Output Group */
	AddDev("INA", 0x40);
	AddDev("OTA", 0x50);
	AddDevImm("OTR", 0xc0);
	AddDirect("STC", 0x28);
	AddDirect("CLC", 0xa8);
	AddDirect("SFS", 0x18);
	AddDirect("SFZ", 0x38);
	AddFixed("RTI", 0x90);
	AddFixed("RTE", 0xb1);
	AddFixed("NOP", 0x5f);
	AddReg("JAI", 0x90);
	AddReg("JAS", 0x98);

	/* G. Program Control Group */
	AddAddress("JMP", 0x80);
	AddAddress("JSB", 0x88);
	AddFixed("RTS", 0xb8);
	AddFixed("RSE", 0xb9);
}

static void DeinitFields(void)
{
	DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_NANO(void)
{
	CodeLen = 0;
	DontPrint = False;

	if (Memo("")) return;

	if (!LookupInstTable(InstTable, OpPart.str.p_str))
		WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_NANO(void)
{
	return False;
}

static void SwitchFrom_NANO(void)
{
	DeinitFields();
}

static void SwitchTo_NANO(void)
{
	const TFamilyDescr *pDescr;

	TurnWords = False;
	SetIntConstMode(eIntConstModeC);

	pDescr = FindFamilyByName("NANO");
	PCSymbol = "*";
	HeaderID = pDescr->Id;
	NOPCode = 0x5f;
	DivideChars = ",";
	HasAttrs = False;

	ValidSegs = (1 << SegCode);
	Grans[SegCode] = 1;
	ListGrans[SegCode] = 1;
	SegInits[SegCode] = 0x0000;
	SegLimits[SegCode] = 0x07ff;

	MakeCode = MakeCode_NANO;
	IsDef = IsDef_NANO;
	SwitchFrom = SwitchFrom_NANO;
	InitFields();
}

void codenano_init(void)
{
	CPUNANO = AddCPU("NANO", SwitchTo_NANO);

	AddCopyright("HP Nanoprocessor Generator (C) 2022 Haruo Asano");
}
