/* codemn1610.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Panafacom MN1610 / MN1613                                   */
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
#include "onoff_common.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "errmsg.h"
#include "ibmfloat.h"
#include "chartrans.h"

#include "codemn1610.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPUMN1610;
static CPUVar CPUMN1613;

static tStrComp Inner;
static tStrComp InnerZ;

/*---------------------------------------------------------------------------*/

static Boolean DecReg(const char *pAsc, Word *pErg, Boolean errMsg)
{
	if (!as_strcasecmp(pAsc, "SP"))
	{
		*pErg = 5;
		return True;
	}

	if (!as_strcasecmp(pAsc, "STR"))
	{
		*pErg = 6;
		return True;
	}

	if (!as_strcasecmp(pAsc, "IC"))
	{
		*pErg = 7;
		return True;
	}

	if (strlen(pAsc) != 2)
	{
		WrError(ErrNum_InvRegName);
		return False;
	}

	if (toupper(pAsc[0]) == 'R' && pAsc[1] >= '0' && pAsc[1] <= '4')
	{
		*pErg = pAsc[1] - '0';
		return True;
	}

	if (toupper(pAsc[0]) == 'X' && pAsc[1] >= '0' && pAsc[1] <= '1')
	{
		*pErg = pAsc[1] - '0' + 3;
		return True;
	}

	if (errMsg) WrError(ErrNum_InvRegName);
	return False;
}

static Boolean DecSkip(const char *pAsc, Word *pErg, Boolean errMsg)
{
	static struct {
		const char *str;
		Word skip;
	} SkipTable[] = {
		{ "SKP", 0x1 },
		{ "M",   0x2 },
		{ "PZ",  0x3 },
		{ "Z",   0x4 }, { "E",   0x4 },
		{ "NZ",  0x5 }, { "NE",  0x5 },
		{ "MZ",  0x6 },
		{ "P",   0x7 },
		{ "EZ",  0x8 },
		{ "ENZ", 0x9 },
		{ "OZ",  0xa },
		{ "ONZ", 0xb },
		{ "LMZ", 0xc },
		{ "LP",  0xd },
		{ "LPZ", 0xe },
		{ "LM",  0xf },
		{ NULL,  0x0 } };
	int i;

	for (i = 0; SkipTable[i].str; i++)
	{
		if (!as_strcasecmp(pAsc, SkipTable[i].str))
		{
			*pErg = SkipTable[i].skip;
			return True;
		}
	}

	if (errMsg) WrError(ErrNum_UndefCond);
	return False;
}

static Boolean DecEM(const char *pAsc, Word *pErg, Boolean errMsg)
{
	static struct {
		const char *str;
		Word em;
	} EMTable[] = {
		{ "RE", 0x1 },
		{ "SE", 0x2 },
		{ "CE", 0x3 },
		{ NULL, 0x0 } };
	int i;

	for (i = 0; EMTable[i].str; i++)
	{
		if (!as_strcasecmp(pAsc, EMTable[i].str))
		{
			*pErg = EMTable[i].em;
			return True;
		}
	}

	if (errMsg) WrError(ErrNum_InvShiftArg);
	return False;
}

static Boolean DecBB(const char *pAsc, Word *pErg)
{
	static struct {
		const char *str;
		Word bb;
	} BBTable[] = {
		{ "CSBR", 0x0 },
		{ "SSBR", 0x1 },
		{ "TSR0", 0x2 },
		{ "TSR1", 0x3 },
		{ NULL,   0x0 } };
	int i;

	for (i = 0; BBTable[i].str; i++)
	{
		if (!as_strcasecmp(pAsc, BBTable[i].str))
		{
			*pErg = BBTable[i].bb;
			return True;
		}
	}

	WrError(ErrNum_UnknownSegReg);
	return False;
}

static Boolean DecRIndirect(const char *pAsc, Word *mm, Word *ii, Boolean bAuto)
{
	int l = strlen(pAsc);
	int rp;
	char MinReg, MaxReg;
	Word RegOffs;

	if (l == 4 && pAsc[0] == '(' && pAsc[3] == ')')
	{
		*mm = 0x1;
		rp = 1;
	}
	else if (l == 5 && pAsc[0] == '-' && pAsc[1] == '(' && pAsc[4] == ')')
	{
		*mm = 0x2;
		rp = 2;
	}
	else if (l == 5 && pAsc[0] == '(' && pAsc[3] == ')' && pAsc[4] == '+')
	{
		*mm = 0x3;
		rp = 1;
	}
	else
	{
		WrError(ErrNum_InvArg);
		return False;
	}

	if (!bAuto && *mm != 0x1)
	{
		WrError(ErrNum_InvArg);
		return False;
	}

	switch (toupper(pAsc[rp]))
	{
		case 'R':
			MinReg = '1'; MaxReg = '4';
			RegOffs = 0;
			break;
		case 'X':
			MinReg = '0'; MaxReg = '1';
			RegOffs = 2;
			break;
		default:
			WrError(ErrNum_InvReg);
			return False;
	}

	if (pAsc[rp + 1] < MinReg || pAsc[rp + 1] > MaxReg)
	{
		WrError(ErrNum_InvReg);
		return False;
	}

	*ii = pAsc[rp + 1] - MinReg + RegOffs;

	return True;
}

static Boolean DecC(const char *pAsc, Word *pErg, Boolean errMsg)
{
	if (strlen(pAsc) == 1)
	{
		if (pAsc[0] == '0')
		{
			*pErg = 0;
			return True;
		}
		if (pAsc[0] == '1' || toupper(pAsc[0]) == 'C')
		{
			*pErg = 1;
			return True;
		}
	}

	if (errMsg) WrError(ErrNum_InvArg);
	return False;
}

static Boolean DecDReg(const char *pAsc, Boolean errMsg)
{
	if (as_strcasecmp(pAsc, "DR0") == 0) return True;

	if (errMsg) WrError(ErrNum_InvReg);
	return False;
}

static Boolean DecBR(const char *pAsc, Word *pErg)
{
	static struct {
		const char *str;
		Word bbb;
	} BRTable[] = {
		{ "CSBR", 0x0 },
		{ "SSBR", 0x1 },
		{ "TSR0", 0x2 },
		{ "TSR1", 0x3 },
		{ "OSR0", 0x4 },
		{ "OSR1", 0x5 },
		{ "OSR2", 0x6 },
		{ "OSR3", 0x7 },
		{ NULL,   0x0 } };
	int i;

	for (i = 0; BRTable[i].str; i++)
	{
		if (!as_strcasecmp(pAsc, BRTable[i].str))
		{
			*pErg = BRTable[i].bbb;
			return True;
		}
	}

	WrError(ErrNum_UnknownSegReg);
	return False;
}

static Boolean DecSR(const char *pAsc, Word *pErg)
{
	static struct {
		const char *str;
		Word ppp;
	} SRTable[] = {
		{ "SBRB", 0x0 },
		{ "ICB",  0x1 },
		{ "NPP",  0x2 },
		{ NULL,   0x0 } };
	int i;

	for (i = 0; SRTable[i].str; i++)
	{
		if (!as_strcasecmp(pAsc, SRTable[i].str))
		{
			*pErg = SRTable[i].ppp;
			return True;
		}
	}

	WrError(ErrNum_InvRegName);
	return False;
}

static Boolean DecHR(const char *pAsc, Word *pErg)
{
	static struct {
		const char *str;
		Word hhh;
	} HRTable[] = {
		{ "TCR",  0x0 },
		{ "TIR",  0x1 },
		{ "TSR",  0x2 },
		{ "SCR",  0x3 },
		{ "SSR",  0x4 },
		{ "SOR",  0x5 },
		{ "SIR",  0x5 },
		{ "IISR", 0x6 },
		{ NULL,   0x0 } };
	int i;

	for (i = 0; HRTable[i].str; i++)
	{
		if (!as_strcasecmp(pAsc, HRTable[i].str))
		{
			*pErg = HRTable[i].hhh;
			return True;
		}
	}

	WrError(ErrNum_InvRegName);
	return False;
}

/* MN1610 */

/* XXXX Rn, Adr  or  XXXX Adr */
static void DecodeAdr(Word Index)
{
	int idx;
	Boolean indirect;
	tStrComp Left, Right;
	Word preg;
	Boolean index;
	tStrComp *sc;
	Word reg = 0;
	Word mode = 0;
	LongInt adr;
	Boolean OK;
	int disp = 0;
	int l;
	
	if ((Index & 0x0700) == 0x0600)	/* IMS/DMS */
	{
		if (!ChkArgCnt(1,1)) return;
		idx = 1;
	}
	else if ((Index & 0x0700) == 0x0700)	/* B/BAL */
	{
		if (!ChkArgCnt(1,1)) return;
		idx = 1;
	}
	else
	{
		if (!ChkArgCnt(2,2)) return;
		if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
		if (reg == 6 || reg == 7)
		{
			WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
			return;
		}
		idx = 2;
	}

	sc = &ArgStr[idx];
	indirect = IsIndirect(ArgStr[idx].str.p_str);
	if (indirect)
	{
		l = strlen(ArgStr[idx].str.p_str);
		StrCompCopySub(&Inner, &ArgStr[idx], 1, l - 2);
		sc = &Inner;
	}

	index = False;
	l = strlen(sc->str.p_str);
	if (l > 4 && sc->str.p_str[l - 4] == '(' && sc->str.p_str[l - 1] == ')')
	{
		StrCompSplitRef(&Left, &Right, sc, sc->str.p_str + l - 4);
		StrCompShorten(&Right, 1);

		if (DecReg(Right.str.p_str, &preg, False))
		{
			switch (preg) {
			case 3:
				index = True;
				break;
			case 4:
				index = True;
				break;
			case 7:
				index = True;
				break;
			default:
				break;
			}
		}
	}

	if (as_strcasecmp(ArgStr[idx].str.p_str, "(X0)") == 0)
	{
		mode = 4;
		disp = 0;
	}
	else if (as_strcasecmp(ArgStr[idx].str.p_str, "(X1)") == 0)
	{
		mode = 5;
		disp = 0;
	}
	else if (index)
	{
		if (IsIndirect(Left.str.p_str))
		{	/* (zadr)(X0) , (zadr)(X1) */
			tEvalResult EvalResult;

			l = strlen(Left.str.p_str);
			StrCompCopySub(&InnerZ, &Left, 1, l - 2);
			disp = EvalStrIntExpressionWithResult(&InnerZ, Int16, &EvalResult);
			if (!EvalResult.OK) return;
			ChkSpace(SegCode, EvalResult.AddrSpaceMask);
			if (disp < 0 || disp > 0xff)
			{
				WrError(ErrNum_OverRange);
				return;
			}

			switch (preg)
			{
			case 3:
				mode = 6;
				break;
			case 4:
				mode = 7;
				break;
			default:
				WrError(ErrNum_InvRegisterPointer);
				return;
			}
		}
		else
		{
			disp = EvalStrIntExpression(&Left, Int8, &OK);
			if (!OK) return;

			if (preg == 7)
			{	/* disp(IC) , (disp(IC)) */
				if( disp > 0x7f || disp < -0x80)
				{
					WrError(ErrNum_OverRange);
					return;
				}

				mode = indirect ? 0x3 : 0x1;
			}
			else
			{	/* disp(X0) , disp(X1) */
				if( disp > 0xff || disp < -0x00)
				{
					WrError(ErrNum_OverRange);
					return;
				}
				switch (preg)
				{
				case 3:
					mode = 4;
					break;
				case 4:
					mode = 5;
					break;
				default:
					WrError(ErrNum_InvRegisterPointer);
					return;
				}
			}
		}
	}
	else
	{	/* !index */
		tEvalResult EvalResult;

		adr = EvalStrIntExpressionWithResult(sc, UInt16, &EvalResult);
		if (!EvalResult.OK) return;
		ChkSpace(SegCode, EvalResult.AddrSpaceMask);
		
		if (adr < 0x100)
		{
			mode = indirect ? 0x2 : 0x0;
			disp = adr;
		}
		else
		{
			adr -= EProgCounter();
			if( adr <= 0x7fL && adr >= -0x80L)
			{
				mode = indirect ? 0x3 : 0x1;
				disp = adr & 0xff;
			}
			else WrError(ErrNum_JmpDistTooBig);
		}
	}
	
	WAsmCode[0] = Index | (mode << 11 ) | (reg << 8) | (disp & 0xff);
	CodeLen = 1;
}

/* XXXX */
static void DecodeFixed(Word Index)
{
	if (ChkArgCnt(0, 0))
	{
		WAsmCode[0] = Index;
		CodeLen = 1;
	}
}

/* XXXX Rn,Imm8 */
static void DecodeRegImm8(Word Index)
{
	Word reg;
	Byte imm8;
	
	if (ChkArgCnt(2, 2))
	{
		if (DecReg(ArgStr[1].str.p_str, &reg, True))
		{
			tEvalResult EvalResult;

			if (reg == 7)
			{
				WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
				return;				
			}
			
			imm8 = EvalStrIntExpressionWithResult(&ArgStr[2], Int8, &EvalResult);
			if (EvalResult.OK)
			{
				if (Index & 0x1000) ChkSpace(SegIO, EvalResult.AddrSpaceMask);	/* RD/WR */

				WAsmCode[0] = Index | ((Word)reg << 8) | imm8;
				CodeLen = 1;
			}
		}
	}
}

/* XXXX Rn */
static void DecodeReg(Word Index)
{
	Word reg;
	
	if (!ChkArgCnt(1,1)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}
	
	WAsmCode[0] = Index | (reg << 8);
	CodeLen = 1;
}

/* XXXX Rd, Rs[, Skip] */
static void DecodeRegRegSkip(Word Index)
{
	Word reg1;
	Word reg2;
	Word skip = 0;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg1, True)) return;
	if (reg1 == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}
	if (!DecReg(ArgStr[2].str.p_str, &reg2, True)) return;
	if (reg2 == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}
	
	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (reg1 << 8) | reg2 | (skip << 4);
	CodeLen = 1;
}

static void DecodeShift(Word Index)
{
	Word reg;
	Word em;
	Word skip;

	if (!ChkArgCnt(1,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}
	
	switch (ArgCnt)
	{
	case 1:
		em = 0;
		skip = 0;
		break;
	case 2:
		if (DecEM(ArgStr[2].str.p_str, &em, False))
		{
			skip = 0;
		}
		else if (DecSkip(ArgStr[2].str.p_str, &skip, False))
		{
			em = 0;
		}
		else
		{
			WrStrErrorPos(ErrNum_InvFormat, &ArgStr[2]);
			return;
		}
		break;
	case 3:
		if (!DecEM(ArgStr[2].str.p_str, &em, True)) return;
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
		break;
	default:
		return;
	}
	
	WAsmCode[0] = Index | (reg << 8) | (skip << 4) | em;
	CodeLen = 1;
}

static void DecodeBit(Word Index)
{
	Word reg;
	Word skip = 0;
	Byte imm4;
	Boolean OK;
	
	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}
	
	imm4 = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
	if (!OK) return;
	if (/*imm4 < 0 ||*/ imm4 > 0x0f)
	{
		WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
		return;
	}

	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}
	
	WAsmCode[0] = Index | (reg << 8) | (skip << 4) | imm4;
	CodeLen = 1;
}

static void DecodeLevel(Word Index)
{
	Word l;
	Boolean OK;
	
	if (!ChkArgCnt(1,1)) return;

	l = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
	if (!OK) return;
	
	WAsmCode[0] = Index | l;
	CodeLen = 1;
}

/* MN1613 */

static void DecodeLD(Word Index)
{
	Word reg;
	Word br = 0x0;	/* CSBR when not specified */
	Word exp;
	tEvalResult EvalResult;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 6 || reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (ArgCnt == 3)
	{
		if (!DecBB(ArgStr[2].str.p_str, &br)) return;
	}

	exp = EvalStrIntExpressionWithResult(&ArgStr[ArgCnt], Int16, &EvalResult);
	if (!EvalResult.OK) return;
	ChkSpace(SegCode, EvalResult.AddrSpaceMask);
	
	WAsmCode[0] = Index | (br << 4) | reg;
	WAsmCode[1] = exp;
	CodeLen = 2;
}

/* */
static void DecodeLR(Word Index)
{
	Word reg;
	Word br = 0x0;	/* CSBR when not specified */
	Word mm;
	Word ii;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 6 || reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (ArgCnt == 3)
	{
		if (!DecBB(ArgStr[2].str.p_str, &br)) return;
	}

	if (!DecRIndirect(ArgStr[ArgCnt].str.p_str, &mm, &ii, True)) return;

	WAsmCode[0] = Index | (reg << 8) | (mm << 6) | (br << 4) | ii;
	CodeLen = 1;
}

/* XXXX R0,(Ri)[,kkkk] */
static void DecodeMVWR(Word Index)
{
	Word reg;
	Word mm;
	Word ii;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg != 0)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
		return;
	}

	if (!DecRIndirect(ArgStr[2].str.p_str, &mm, &ii, False)) return;
	
	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}


	WAsmCode[0] = Index | (skip << 4) | ii;
	CodeLen = 1;
}

/* XXXX Rd, Exp[, kkkk] */
static void DecodeMVWI(Word Index)
{
	Word reg;
	Word exp;
	Boolean OK;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}
	
	exp = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
	if (!OK) return;
	
	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}


	WAsmCode[0] = Index | (reg << 8) | (skip << 4);
	WAsmCode[1] = exp;
	CodeLen = 2;
}

/* XXXX */
static void DecodeFixed1613(Word Index)
{
	if (!ChkMinCPU(CPUMN1613)) return;

	if (ChkArgCnt(0, 0))
	{
		WAsmCode[0] = Index;
		CodeLen = 1;
	}
}

/* XXXX Rd[, C][, kkkk] */
static void DecodeNEG(Word Index)
{
	Word reg;
	Word c = 0;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(1,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (ArgCnt == 2)
	{
		if (!DecC(ArgStr[2].str.p_str, &c, False))
		{
			if (!DecSkip(ArgStr[2].str.p_str, &skip, True)) return;
		}
	}

	if (ArgCnt == 3)
	{
		if (!DecC(ArgStr[2].str.p_str, &c, True)) return;

		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4) | (c << 3) | reg;
	CodeLen = 1;
}

/* XXXX DR0, (Ri)[, C][, kkkk] */
static void DecodeAD(Word Index)
{
	Word mm;
	Word ii;
	Word c = 0;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,4)) return;

	if (!DecDReg(ArgStr[1].str.p_str, True)) return;

	if (!DecRIndirect(ArgStr[2].str.p_str, &mm, &ii, False)) return;
	
	if (ArgCnt == 3)
	{
		if (!DecC(ArgStr[3].str.p_str, &c, False))
		{
			if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
		}
	}

	if (ArgCnt == 4)
	{
		if (!DecC(ArgStr[3].str.p_str, &c, True)) return;

		if (!DecSkip(ArgStr[4].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4) | (c << 3) | ii;
	CodeLen = 1;
}

/* XXXX DR0, (Ri)[, kkkk] */
static void DecodeM(Word Index)
{
	Word mm;
	Word ii;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecDReg(ArgStr[1].str.p_str, True)) return;

	if (!DecRIndirect(ArgStr[2].str.p_str, &mm, &ii, False)) return;
	
	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4) | ii;
	CodeLen = 1;
}

/* XXXX R0, (Ri)[, C][, kkkk] */
static void DecodeDAA(Word Index)
{
	Word reg;
	Word mm;
	Word ii;
	Word c = 0;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,4)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg != 0)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
		return;
	}

	if (!DecRIndirect(ArgStr[2].str.p_str, &mm, &ii, False)) return;
	
	if (ArgCnt == 3)
	{
		if (!DecC(ArgStr[3].str.p_str, &c, False))
		{
			if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
		}
	}

	if (ArgCnt == 4)
	{
		if (!DecC(ArgStr[3].str.p_str, &c, True)) return;

		if (!DecSkip(ArgStr[4].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4) | (c << 3) | ii;
	CodeLen = 1;
}

/* XXXX R0, DR0[, kkkk] */
static void DecodeFIX(Word Index)
{
	Word reg;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg != 0)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
		return;
	}

	if (!DecDReg(ArgStr[2].str.p_str, True)) return;
	
	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4);
	CodeLen = 1;
}

/* XXXX DR0, R0[, kkkk] */
static void DecodeFLT(Word Index)
{
	Word reg;
	Word skip = 0;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecDReg(ArgStr[1].str.p_str, True)) return;
	
	if (!DecReg(ArgStr[2].str.p_str, &reg, True)) return;
	if (reg != 0)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
		return;
	}

	if (ArgCnt == 3)
	{
		if (!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4);
	CodeLen = 1;
}

/* XXXX Exp */
static void DecodeBD(Word Index)
{
	Word exp;
	tEvalResult EvalResult;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(1,1)) return;

	exp = EvalStrIntExpressionWithResult(&ArgStr[1], Int16, &EvalResult);
	if (!EvalResult.OK) return;
	ChkSpace(SegCode, EvalResult.AddrSpaceMask);

	WAsmCode[0] = Index;
	WAsmCode[1] = exp;
	CodeLen = 2;
}

/* XXXX (Exp) */
static void DecodeBL(Word Index)
{
	int l;
	Word exp;
	tEvalResult EvalResult;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(1,1)) return;

	l = strlen(ArgStr[1].str.p_str);

	if (l < 3 || ArgStr[1].str.p_str[0] != '(' || ArgStr[1].str.p_str[l - 1] != ')')
	{
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
		return;
	}

	StrCompCopySub(&Inner, &ArgStr[1], 1, l - 2);
	exp = EvalStrIntExpressionWithResult(&Inner, Int16, &EvalResult);
	if (!EvalResult.OK) return;
	ChkSpace(SegCode, EvalResult.AddrSpaceMask);

	WAsmCode[0] = Index;
	WAsmCode[1] = exp;
	CodeLen = 2;
}

/* XXXX (Ri) */
static void DecodeBR(Word Index)
{
	Word mm;
	Word ii;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(1,1)) return;

	if (!DecRIndirect(ArgStr[1].str.p_str, &mm, &ii, False)) return;

	WAsmCode[0] = Index | ii;
	CodeLen = 1;
}

/* XXXX Rs, Exp[, Skip] */
static void DecodeTSET(Word Index)
{
	Word reg;
	Word exp;
	tEvalResult EvalResult;
	Word skip = 0;

	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,3)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	exp = EvalStrIntExpressionWithResult(&ArgStr[2], Int16, &EvalResult);
	if (!EvalResult.OK) return;
	ChkSpace(SegCode, EvalResult.AddrSpaceMask);

	if (ArgCnt == 3)
	{
		if(!DecSkip(ArgStr[3].str.p_str, &skip, True)) return;
	}

	WAsmCode[0] = Index | (skip << 4) | reg;
	WAsmCode[1] = exp;
	CodeLen = 2;
}

/* XXXX Rd, Rs */
static void DecodeSRBT(Word Index)
{
	Word reg1;
	Word reg2;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,2)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg1, True)) return;
	if (reg1 == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (!DecReg(ArgStr[2].str.p_str, &reg2, True)) return;
	if (reg2 == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	switch (Index)
	{
	case 0x3f70:	/* SRBT */
		if (reg1 != 0)
		{
			WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
			return;
		}
		break;
	case 0x3ff0:	/* DEBP */
		if (reg2 != 0)
		{
			WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
			return;
		}
		break;
	default:
		WrError(ErrNum_InternalError);
		return;
	}

	WAsmCode[0] = Index | reg1 | reg2;
	CodeLen = 1;
}

/* XXXX (R2), (R1), R0 */
static void DecodeBLK(Word Index)
{
	Word mm;
	Word ii1;
	Word ii2;
	Word reg;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(3,3)) return;

	if (!DecRIndirect(ArgStr[1].str.p_str, &mm, &ii1, False)) return;
	if (ii1 != 1)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
		return;
	}
	
	if (!DecRIndirect(ArgStr[2].str.p_str, &mm, &ii2, False)) return;
	if (ii2 != 0)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
		return;
	}

	if (!DecReg(ArgStr[3].str.p_str, &reg, True)) return;
	if (reg != 0)
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[3]);
		return;
	}

	WAsmCode[0] = Index;
	CodeLen = 1;
}

/* XXXX R, (Ri) */
static void DecodeRDR(Word Index)
{
	Word reg;
	Word mm;
	Word ii;

	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,2)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 6 || reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (!DecRIndirect(ArgStr[2].str.p_str, &mm, &ii, False)) return;

	WAsmCode[0] = Index | (reg << 8) | ii;
	CodeLen = 1;
}

/* XXXX BRn/SRn, Exp */
static void DecodeLB(Word Index)
{
	Word reg;
	Word exp;
	tEvalResult EvalResult;

	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,2)) return;

	if (Index & 0x0008)
	{	/* SR */
		if (!DecSR(ArgStr[1].str.p_str, &reg)) return;
	}
	else
	{	/* BR */
		if (!DecBR(ArgStr[1].str.p_str, &reg)) return;
		if (reg == 0 && !(Index & 0x0080))
		{
			WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
			return;
		}
	}

	exp = EvalStrIntExpressionWithResult(&ArgStr[2], Int16, &EvalResult);
	if (!EvalResult.OK) return;
	ChkSpace(SegCode, EvalResult.AddrSpaceMask);

	WAsmCode[0] = Index | (reg << 4);
	WAsmCode[1] = exp;
	CodeLen = 2;
}

/* XXXX Rn, BRn/SRn/HRn */
static void DecodeCPYB(Word Index)
{
	Word reg1;
	Word reg2;
	
	if (!ChkMinCPU(CPUMN1613)) return;

	if (!ChkArgCnt(2,2)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg1, True)) return;
	if (reg1 == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	switch (Index & 0x3008)
	{
	case 0x0000:	/* BR */
		if (!DecBR(ArgStr[2].str.p_str, &reg2)) return;
		if (reg2 == 0 && !(Index & 0x0080))
		{
			WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
			return;
		}
		break;
	case 0x0008:	/* SR */
		if (!DecSR(ArgStr[2].str.p_str, &reg2)) return;
		break;
	case 0x3000:	/* HR */
		if (!DecHR(ArgStr[2].str.p_str, &reg2)) return;
		break;
	default:
		WrError(ErrNum_InternalError);
		return;
	}

	WAsmCode[0] = Index | (reg2 << 4) | reg1;
	CodeLen = 1;
}

/* Alias */

static void DecodeCLR(Word Index)
{
	Word reg;
	
	if (!ChkArgCnt(1,1)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	WAsmCode[0] = Index | (reg << 8) | reg;
	CodeLen = 1;
}

static void DecodeSKIP(Word Index)
{
	Word reg;
	Word skip;

	if (!ChkArgCnt(2,2)) return;

	if (!DecReg(ArgStr[1].str.p_str, &reg, True)) return;
	if (reg == 7)
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (!DecSkip(ArgStr[2].str.p_str, &skip, True)) return;

	WAsmCode[0] = Index | (reg << 8) | (skip << 4) | reg;
	CodeLen = 1;
}

/* Pseudo Instruction */

static void PutByte(Byte data, Boolean *p_lower_byte)
{
	if (*p_lower_byte)
	{
		WAsmCode[CodeLen - 1] |= data;
		*p_lower_byte = False;
	}
	else
	{
		SetMaxCodeLen((CodeLen + 1) * 2);
		WAsmCode[CodeLen++] = data << 8;
		*p_lower_byte = True;
	}
}

static void PutWord(Word data, Boolean *p_lower_byte)
{
	SetMaxCodeLen((CodeLen + 1) * 2);	
	WAsmCode[CodeLen++] = data;
  *p_lower_byte = False;
}

static void DecodeDC(Word Index)
{
	int c;
	Boolean LowerByte = False;
	TempResult t;
	
	UNUSED(Index);

	as_tempres_ini(&t);
	if (ChkArgCnt(1, ArgCntMax))
	{
		Boolean OK = True;
		tStrComp *pArg;

		forallargs(pArg, OK)
		{
			EvalStrExpression(pArg, &t);
			if (mFirstPassUnknown(t.Flags) && t.Typ == TempInt) t.Contents.Int &= 65535;
			switch (t.Typ)
			{
			case TempInt:
				if (Packing)
				{
					if (ChkRange(t.Contents.Int, -128, 255))
					{
						PutByte(t.Contents.Int, &LowerByte);
					}
				}
				else
				{
				ToInt:
					if (ChkRange(t.Contents.Int, -32768, 65535))
					{
						PutWord(t.Contents.Int, &LowerByte);
					}
				}
				break;
			case TempFloat:
				SetMaxCodeLen((CodeLen + 2) * 2);
				if (Double2IBMFloat(&WAsmCode[CodeLen], t.Contents.Float, False))
				{
					CodeLen += 2;
				}
				else
				{
					OK = False;
				}
				LowerByte = False;
				break;
			case TempString:
				if (MultiCharToInt(&t, 2))
					goto ToInt;
				if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
					OK = False;
				else
					for (c = 0; c < (int)t.Contents.str.len; c++)
					{
						PutByte(((usint)t.Contents.str.p_str[c]) & 0xff, &LowerByte);
					}
				break;
			default:
				OK = False;
			}
		}
		if (!OK) CodeLen = 0;
	}
  as_tempres_free(&t);
}

static void DecodeDS(Word Index)
{
	LongInt Size;
	Boolean OK;
	tSymbolFlags Flags;
	
	UNUSED(Index);

	if (!ChkArgCnt(1, 1)) return;

	Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);
	if (!OK) return;
	if (mFirstPassUnknown(Flags))
	{
		WrStrErrorPos(ErrNum_FirstPassCalc, &ArgStr[1]);
		return;
	}

	DontPrint = True;
	CodeLen = Size;
	BookKeeping();
}

/*---------------------------------------------------------------------------*/

/* MN1610 */

static void AddAdr(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeAdr);
}

static void AddFixed(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddRegImm8(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRegImm8);
}

static void AddReg(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeReg);
}

static void AddRegRegSkip(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRegRegSkip);
}

static void AddShift(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeShift);
}
	
static void AddBit(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBit);
}
	
static void AddLevel(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeLevel);
}

/* MN1613 */

static void AddLD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeLD);
}

static void AddLR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeLR);
}

static void AddMVWR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVWR);
}

static void AddMVWI(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVWI);
}

static void AddFixed1613(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFixed1613);
}

static void AddNEG(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeNEG);
}

static void AddAD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeAD);
}

static void AddM(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeM);
}

static void AddDAA(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDAA);
}

static void AddFIX(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFIX);
}

static void AddFLT(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFLT);
}

static void AddBD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBD);
}

static void AddBL(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBL);
}

static void AddBR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBR);
}

static void AddTSET(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeTSET);
}

static void AddSRBT(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeSRBT);
}

static void AddBLK(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBLK);
}

static void AddRDR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRDR);
}

static void AddLB(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeLB);
}

static void AddCPYB(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeCPYB);
}

static void InitFields(void)
{
	InstTable = CreateInstTable(128);

	/* Basic MN1610 Instructions */
	
	AddAdr("L",   0xc000);
	AddAdr("ST",  0x8000);
	AddAdr("B",   0xc700);
	AddAdr("BAL", 0x8700);
	AddAdr("IMS", 0xc600);
	AddAdr("DMS", 0x8600);
	
	AddRegRegSkip("A",    0x5808);
	AddRegRegSkip("S",    0x5800);
	AddRegRegSkip("AND",  0x6808);
	AddRegRegSkip("OR",   0x6008);
	AddRegRegSkip("EOR",  0x6000);
	AddRegRegSkip("C",    0x5008);
	AddRegRegSkip("CB",   0x5000);
	AddRegRegSkip("MV",   0x7808);
	AddRegRegSkip("MVB",  0x7800);
	AddRegRegSkip("BSWP", 0x7008);
	AddRegRegSkip("DSWP", 0x7000);
	AddRegRegSkip("LAD",  0x6800);

	AddShift("SL", 0x200c);
	AddShift("SR", 0x2008);

	AddBit("SBIT", 0x3800);
	AddBit("RBIT", 0x3000);
	AddBit("TBIT", 0x2800);
	AddBit("AI",   0x4800);
	AddBit("SI",   0x4000);
	
	AddRegImm8("RD",  0x1800);
	AddRegImm8("WR",  0x1000);
	AddRegImm8("WT",  0x1000);
	AddRegImm8("MVI", 0x0800);

	AddLevel("LPSW", 0x2004);
	AddFixed("H",   0x2000);
	AddReg("PUSH", 0x2001);
	AddReg("POP",  0x2002);
	AddFixed("RET", 0x2003);

	/* MN1613 Instructions */

	AddLD("LD",  0x2708);
	AddLD("STD", 0x2748);

	AddLR("LR",  0x2000);
	AddLR("STR", 0x2004);

	AddMVWR("MVWR", 0x7f08);
	AddMVWR("MVBR", 0x7f00);
	AddMVWR("BSWR", 0x7708);
	AddMVWR("DSWR", 0x7700);
	AddMVWR("AWR",  0x5f08);
	AddMVWR("SWR",  0x5f00);
	AddMVWR("CWR",  0x5708);
	AddMVWR("CBR",  0x5700);
	AddMVWR("LADR", 0x6f00);
	AddMVWR("ANDR", 0x6f08);
	AddMVWR("ORR",  0x6708);
	AddMVWR("EORR", 0x6700);

	AddMVWI("MVWI", 0x780f);
	AddMVWI("AWI",  0x580f);
	AddMVWI("SWI",  0x5807);
	AddMVWI("CWI",  0x500f);
	AddMVWI("CBI",  0x5007);
	AddMVWI("LADI", 0x6807);
	AddMVWI("ANDI", 0x680f);
	AddMVWI("ORI",  0x600f);
	AddMVWI("EORI", 0x6007);

	AddFixed1613("PSHM", 0x170f);
	AddFixed1613("POPM", 0x1707);
	AddFixed1613("RETL", 0x3f07);

	AddNEG("NEG", 0x1f00);

	AddAD("AD", 0x4f04);
	AddAD("SD", 0x4704);

	AddM("M",  0x7f0c);
	AddM("D",  0x770c);
	AddM("FA", 0x6f0c);
	AddM("FS", 0x6f04);
	AddM("FM", 0x670c);
	AddM("FD", 0x6704);

	AddDAA("DAA", 0x5f04);
	AddDAA("DAS", 0x5704);

	AddFIX("FIX", 0x1f0f);
	
	AddFLT("FLT", 0x1f07);

	AddBD("BD",   0x2607);
	AddBD("BALD", 0x2617);
	
	AddBL("BL",   0x270f);
	AddBL("BALL", 0x271f);
	
	AddBR("BR",   0x2704);
	AddBR("BALR", 0x2714);

	AddTSET("TSET", 0x1708);
	AddTSET("TRST", 0x1700);

	AddSRBT("SRBT", 0x3f70);
	AddSRBT("DEBP", 0x3ff0);

	AddBLK("BLK", 0x3f17);

	AddRDR("RDR", 0x2014);
	AddRDR("WTR", 0x2010);

	AddLB("LB",  0x0f07);
	AddLB("LS",  0x0f0f);
	AddLB("STB", 0x0f87);
	AddLB("STS", 0x0f8f);

	AddCPYB("CPYB", 0x0f80);
	AddCPYB("CPYS", 0x0f88);
	AddCPYB("CPYH", 0x3f80);
	AddCPYB("SETB", 0x0f00);
	AddCPYB("SETS", 0x0f08);
	AddCPYB("SETH", 0x3f00);

	/* Alias */

	AddFixed("NOP", 0x7808);	/* MOV R0,R0*/
	AddInstTable(InstTable, "CLR",   0x6000, DecodeCLR);	/* EOR Rn,Rn */
	AddInstTable(InstTable, "CLEAR", 0x6000, DecodeCLR);	/* EOR Rn,Rn */
	AddInstTable(InstTable, "SKIP",  0x7808, DecodeSKIP);	/* MV  Rn,Rn,Skip */
	
	/* Pseudo Instructions */

	AddInstTable(InstTable, "DC", 0, DecodeDC);
	AddInstTable(InstTable, "DS", 0, DecodeDS);
	
	StrCompAlloc(&Inner, STRINGSIZE);
	StrCompAlloc(&InnerZ, STRINGSIZE);
}

static void DeinitFields(void)
{
	DestroyInstTable(InstTable);

	StrCompFree(&InnerZ);
	StrCompFree(&Inner);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_MN1610(void)
{
	CodeLen = 0;
	DontPrint = False;

	if (Memo("")) return;

	/* if (DecodeIntelPseudo(False)) return; */
	/* if (DecodeMoto16Pseudo(eSymbolSize16Bit, True)) return; */
	
	if (!LookupInstTable(InstTable, OpPart.str.p_str))
		WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_MN1610(void)
{
	return False;
}

static void SwitchFrom_MN1610(void)
{
	DeinitFields();
}

static void SwitchTo_MN1610(void)
{
	TurnWords = True;
	SetIntConstMode(eIntConstModeIBM); /*ConstModeC;*/

	PCSymbol = "*";
	HeaderID = 0x36;
	NOPCode = 0x7808;	/* MV R0,R0 */
	DivideChars = ",";
	HasAttrs = False;

	ValidSegs = (1 << SegCode) | (1 << SegIO);
	Grans[SegCode] = 2;
	ListGrans[SegCode] = 2;
	SegInits[SegCode] = 0;
	SegLimits[SegCode] = 0xffff;
	Grans[SegIO] = 2;
	ListGrans[SegIO] = 2;
	SegInits[SegIO] = 0;
	SegLimits[SegIO] = 0xffff;

	onoff_packing_add(False);

	MakeCode = MakeCode_MN1610;
	IsDef = IsDef_MN1610;
	SwitchFrom = SwitchFrom_MN1610;
	InitFields();
}

void codemn1610_init(void)
{
	CPUMN1610 = AddCPU("MN1610", SwitchTo_MN1610);
	CPUMN1613 = AddCPU("MN1613", SwitchTo_MN1610);

	AddCopyright("Panafacom MN1610/1613-Generator (C) 2019 Haruo Asano");
}
