/* code62015.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Sharp SC62015                                               */
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
#include "headids.h"
#include "errmsg.h"
#include "chartrans.h"
#include "codepseudo.h"
#include "intpseudo.h"

#include "code62015.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPUSC62015;

/*---------------------------------------------------------------------------*/

#define RegA  0
#define RegIL 1
#define RegBA 2
#define RegI  3
#define RegX  4
#define RegY  5
#define RegU  6
#define RegS  7
#define RegB  8
#define RegIH 9
#define RegPS 10
#define RegPC 11
#define RegF  12
#define RegIMR 13

#define IsR1(reg) ((reg) == RegA || (reg) == RegIL)
#define IsR2(reg) ((reg) == RegBA || (reg) == RegI)
#define IsR3(reg) ((reg) >= RegX && (reg) <= RegS)
#define IsR4(reg) ((reg) >= RegX && (reg) <= RegU)

#define IsWP(index) ((index) == 0 || (index) == 1)
#define IsWPL(index) ((index) == 0 || (index) == 1 || (index) == 2)

typedef struct
{
	const char name[4];
	Byte addr;
} tIntReg;

static const tIntReg IntRegs[] =
{
	{ "BP" , 0xec },
	{ "PX" , 0xed },
	{ "PY" , 0xee },
	{ "AMC", 0xef },
	{ "KOL", 0xf0 },
	{ "KOH", 0xf1 },
	{ "KI" , 0xf2 },
	{ "EOL", 0xf3 },
	{ "EOH", 0xf4 },
	{ "EIL", 0xf5 },
	{ "EIH", 0xf6 },
	{ "UCR", 0xf7 },
	{ "USR", 0xf8 },
	{ "RXD", 0xf9 },
	{ "TXD", 0xfa },
	{ "IMR", 0xfb },
	{ "ISR", 0xfc },
	{ "SCR", 0xfd },
	{ "LCC", 0xfe },
	{ "SSR", 0xff },
};

static Boolean DecodeReg(const tStrComp *arg, Byte *pReg)
{
	const char rtable[][4] = { "A", "IL", "BA", "I", "X", "Y", "U", "S", "B", "IH", "PS", "PC", "F", "IMR" };
	const char *str = arg->str.p_str;
	size_t i;

	for (i = 0; i < as_array_size(rtable); i++)
	{
		if (as_strcasecmp(str, rtable[i]) == 0)
		{
			*pReg = i;
			return True;
		}
	}

	return False;
}

static int RegSize(Byte Reg)
{
	int size;

	switch (Reg)
	{
	case RegPS:
		size = 0;
		break;
	case 0:	/* A */
	case 1:	/* IL */
	case 8:	/* B */
	case 9:	/* IH */
		size = 1;
		break;
	case 2:	/* BA */
	case 3:	/* I */
	case 11:	/* PC */
		size = 2;
		break;
	default:
		size = 3;
	}

	return size;
}

static Boolean IsCoreReg(Byte reg)
{
	return (reg < 8) ? True : False;
}

static Boolean DecodeInternal(const tStrComp *pArg, Byte *pMode, Byte *pDisp, const char *pPReg)
{
	size_t i;
	String Temp;
	tStrComp temp;
	Byte mode = 0;
	Boolean OK;
	int val;
	int sign = 1;
	tSymbolFlags flags;

	for (i = 0; i < as_array_size(IntRegs); i++)
	{
		if (!as_strcasecmp(pArg->str.p_str, IntRegs[i].name))
		{
			*pMode = 0;
			*pDisp = IntRegs[i].addr;
			return True;
		}
	}

	if (!IsIndirect(pArg->str.p_str)) return False;

#if 0
	StrCompRefRight(&temp, pArg, 1);	/* strip off left '(' */
#endif
	StrCompMkTemp(&temp, Temp, sizeof(Temp));
	StrCompCopy(&temp, pArg);
	StrCompCutLeft(&temp, 1);
	StrCompShorten(&temp, 1);	/* strip off right ')' */
	KillPrefBlanksStrComp(&temp);
	KillPostBlanksStrComp(&temp);

	if (!as_strncasecmp(temp.str.p_str, "BP", 2))
	{
		mode = 1;
	}
	else if (!as_strncasecmp(temp.str.p_str, pPReg, 2))
	{
		mode = 2;
	}
	else
	{
		val = EvalStrIntExpression(&temp, UInt8, &OK);
		if (OK)
		{
			*pMode = 0;
			*pDisp = val;
			return OK;
		}

		return False;
	}

	StrCompCutLeft(&temp, 2);
	KillPrefBlanksStrComp(&temp);

	if (temp.Pos.Len == 0)
	{
		/* Displacement omitted */
		*pMode = mode;
		*pDisp = 0;
		return True;
	}

	if (temp.str.p_str[0] == '+')
	{
		sign = 1;
		StrCompCutLeft(&temp, 1);
	}
	else if (temp.str.p_str[0] == '-')
	{
		sign = -1;
		StrCompCutLeft(&temp, 1);
	}
	KillPrefBlanksStrComp(&temp);

	if (mode == 1 && !as_strcasecmp(temp.str.p_str, pPReg))
	{
		*pMode = 3;
		*pDisp = 0;
		return True;
	}

	val = EvalStrIntExpressionWithFlags(&temp, UInt8, &OK, &flags) * sign;
	if (OK)
	{
		if (!mFirstPassUnknownOrQuestionable(flags) && (val > 127 || val < -128))
		{
			WrStrErrorPos(ErrNum_DistTooBig, pArg);
			return False;
		}
		*pMode = mode;
		*pDisp = val;
		return True;
	}

	return False;
}

static void PreByte(Byte mode1, Byte mode2)
{
	static const Byte ptable[] = {
		0x32, 0x30, 0x33, 0x31,
		0x22, 0x00, 0x23, 0x21,
		0x36, 0x34, 0x37, 0x35,
		0x26, 0x24, 0x27, 0x25 };

	if (mode1 == 1 && mode2 == 1) return;

	if (mode1 > 3 || mode2 > 3)
	{
		WrError(ErrNum_InternalError);
		return;
	}

	BAsmCode[CodeLen++] = ptable[mode1 * 4 + mode2];
}

static Boolean DecodeDirect(const tStrComp *pArg, LongInt *pAddr)
{
	String Temp;
	tStrComp temp;
	LongInt val;
	Boolean OK;

	if (!IsIndirectGen(pArg->str.p_str, "[]")) return False;

	StrCompMkTemp(&temp, Temp, sizeof(Temp));
	StrCompCopy(&temp, pArg);
	StrCompCutLeft(&temp, 1);
	StrCompShorten(&temp, 1);
	KillPrefBlanksStrComp(&temp);
	KillPostBlanksStrComp(&temp);

	val = EvalStrIntExpression(&temp, UInt20, &OK);
	if (OK)
	{
		*pAddr = val;
		return OK;
	}

	return False;
}

static Boolean DecodeRegIndirect(const tStrComp *pArg, Byte *pReg, Byte *pMode)
{
	String Temp;
	tStrComp temp;
	Byte reg;
	int l;
	Byte mode = 0x00;

	if (!IsIndirectGen(pArg->str.p_str, "[]")) return False;
	StrCompMkTemp(&temp, Temp, sizeof(Temp));
	StrCompCopy(&temp, pArg);
	StrCompCutLeft(&temp, 1);
	StrCompShorten(&temp, 1);
	KillPrefBlanksStrComp(&temp);
	KillPostBlanksStrComp(&temp);

	l = strlen(temp.str.p_str);
	if ((l >= 2) && !strncmp(temp.str.p_str + l - 2, "++", 2))
	{
		mode = 0x20;
		StrCompShorten(&temp, 2);
	}
	else if (!strncmp(temp.str.p_str, "--", 2))
	{
		mode = 0x30;
		StrCompCutLeft(&temp, 2);
	}

	if (DecodeReg(&temp, &reg))
	{
		*pReg = reg;
		*pMode = mode;
		return True;
	}

	return False;
}

static Boolean DecodeRegBase(const tStrComp *pArg, Byte *pReg, Byte *pMode, Byte *pDisp)
{
	String Temp;
	tStrComp temp;
	String Temp2;
	tStrComp temp2;
	int i;
	char ch;
	Byte reg;
	Byte mode;
	Word val;
	Boolean OK;

	if (!IsIndirectGen(pArg->str.p_str, "[]")) return False;
	StrCompMkTemp(&temp, Temp, sizeof(Temp));
	StrCompCopy(&temp, pArg);
	StrCompCutLeft(&temp, 1);
	StrCompShorten(&temp, 1);
	KillPrefBlanksStrComp(&temp);
	KillPostBlanksStrComp(&temp);

	for (i = 0; temp.str.p_str[i]; i++)
	{
		ch = temp.str.p_str[i];
		if (isalpha(ch)) continue;
		if (isspace(ch) || ch == '+' || ch == '-') break;
		return False;
	}

	if (i <= 2)
	{
		StrCompMkTemp(&temp2, Temp2, sizeof(Temp2));
		StrCompCopy(&temp2, &temp);
		StrCompShorten(&temp, strlen(temp.str.p_str) - i);
		StrCompCutLeft(&temp2, i);
		KillPrefBlanksStrComp(&temp2);

		if (!DecodeReg(&temp, &reg) || !IsR3(reg)) return False;

		if (temp2.str.p_str[0] == '+')
		{
			mode = 0x80;
		}
		else if (temp2.str.p_str[0] == '-')
		{
			mode = 0xc0;
		}
		else return False;

		StrCompCutLeft(&temp2, i);
		KillPrefBlanksStrComp(&temp2);

		val = EvalStrIntExpression(&temp2, UInt8, &OK);
		if (!OK) return False;

		*pReg = reg;
		*pMode = mode;
		*pDisp = val;
		return True;
	}

	return False;
}

static Boolean DecodeImm(const tStrComp *pArg, LongInt *pVal, int size)
{
	LongInt val;
	Boolean OK;

	switch (size)
	{
	case 1:	/* 8 bit */
		val = EvalStrIntExpression(pArg, Int8, &OK);
		break;
	case 2:	/* 16 bit */
		val = EvalStrIntExpression(pArg, Int16, &OK);
		break;
	case 3:	/* 20 bit */
		val = EvalStrIntExpression(pArg, Int20, &OK);
		break;
	default:
		WrError(ErrNum_InternalError);
		return False;
	}

	if (OK)
	{
		*pVal = val;
		return True;
	}

	return False;
}

static Boolean DecodeIntIndirect(const tStrComp *pArg, Byte *pDir, Byte *pDisp2, Byte *pMode, Byte *pDisp, const char *pPReg)
{
	String Temp;
	String Temp2;
	tStrComp temp;
	tStrComp temp2;
	char *pEnd;
	int pos;
	Byte dir;
	Byte mode;
	Byte disp;
	Byte disp2;
	Boolean OK;

	if (!IsIndirectGen(pArg->str.p_str, "[]")) return False;
	StrCompMkTemp(&temp, Temp, sizeof(Temp));
	StrCompCopy(&temp, pArg);
	StrCompCutLeft(&temp, 1);
	StrCompShorten(&temp, 1);
	KillPrefBlanksStrComp(&temp);
	KillPostBlanksStrComp(&temp);

	if (temp.str.p_str[0] != '(') return False;
	pEnd = FindClosingParenthese(temp.str.p_str + 1);
	pos = pEnd - temp.str.p_str + 1;
	if (!pEnd) return False;

	StrCompMkTemp(&temp2, Temp2, sizeof(Temp2));
	StrCompCopySub(&temp2, &temp, pos, strlen(temp.str.p_str) - pos);
	StrCompShorten(&temp, strlen(temp.str.p_str) - pos);
	KillPrefBlanksStrComp(&temp2);

	if (!DecodeInternal(&temp, &mode, &disp, pPReg)) return False;

	dir = 0x00;
	if (strlen(temp2.str.p_str) > 0)
	{
		switch (temp2.str.p_str[0])
		{
		case '+':
			dir = 0x80;
			break;
		case '-':
			dir = 0xc0;
			break;
		default:
			return False;
		}

		StrCompCutLeft(&temp2, 1);
		disp2 = EvalStrIntExpression(&temp2, UInt8, &OK);
		if (!OK) return False;
	}
  else
    disp2 = 0;

	*pDir = dir;
	*pMode = mode;
	*pDisp2 = disp2;
	*pDisp = disp;
	return True;
}

static void AddrOut20(LongInt addr)
{
	BAsmCode[CodeLen++] = addr;
	BAsmCode[CodeLen++] = addr >> 8;
	BAsmCode[CodeLen++] = addr >> 16;
}

/*---------------------------------------------------------------------------*/

static void DecodeMV(Word Index)
{
	int size = Index;
	int i;
	Byte reg1;
	Byte reg2;
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;
	Byte dir;
	Byte mdisp;
	LongInt addr;
	LongInt val;

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg1))
		{
			if (!size) size = RegSize(reg1);

			if (DecodeReg(&ArgStr[2], &reg2))
			{
				if (size == 1)
				{
					if (reg1 == RegA && reg2 == RegB)
					{
						BAsmCode[0] = 0x74;
						CodeLen = 1;
						return;
					}
					if (reg1 == RegB && reg2 == RegA)
					{
						BAsmCode[0] = 0x75;
						CodeLen = 1;
						return;
					}
				}

				if (!Index && reg1 < 8 && reg2 < 8 && RegSize(reg1) > 1)
				{
					if (RegSize(reg1) != RegSize(reg2))
					{
						WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);	/* Register size conflict */
						return;
					}

					BAsmCode[0] = 0xFD;
					BAsmCode[1] = (reg1 << 4) | reg2;
					CodeLen = 2;
					return;
				}

				WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0x80 | reg1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (reg1 != RegS && DecodeRegIndirect(&ArgStr[2], &reg2, &mode2))
			{
				BAsmCode[0] = 0x90 | reg1;
				BAsmCode[1] = mode2 | reg2;
				CodeLen = 2;
				return;
			}

			if (DecodeRegBase(&ArgStr[2], &reg2, &mode2, &disp2))
			{
				BAsmCode[0] = 0x90 | reg1;
				BAsmCode[1] = mode2 | reg2;
				BAsmCode[2] = disp2;
				CodeLen = 3;
				return;
			}

			if (DecodeIntIndirect(&ArgStr[2], &dir, &mdisp, &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0x98 | reg1;
				BAsmCode[CodeLen++] = dir;
				BAsmCode[CodeLen++] = disp2;
				if (dir) BAsmCode[CodeLen++] = mdisp;
				return;
			}

			if (DecodeDirect(&ArgStr[2], &addr))
			{
				BAsmCode[CodeLen++] = 0x88 | reg1;
				AddrOut20(addr);
				return;
			}

			if (DecodeImm(&ArgStr[2], &val, size))
			{
				BAsmCode[CodeLen++] = 0x08 | reg1;
				for (i = 0; i < size; i++)
				{
					BAsmCode[CodeLen++] = val >> (i * 8);
				}
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg2) && IsCoreReg(reg2))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xa0 | reg2;
				BAsmCode[CodeLen++] = disp1;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xc8;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (DecodeRegIndirect(&ArgStr[2], &reg2, &mode2))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xe0;
				BAsmCode[CodeLen++] = mode2 | reg2;
				BAsmCode[CodeLen++] = disp1;
				return;
			}

			if (DecodeRegBase(&ArgStr[2], &reg2, &mode2, &disp2))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xe0;
				BAsmCode[CodeLen++] = mode2 | reg2;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (DecodeIntIndirect(&ArgStr[2], &dir, &mdisp, &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xf0;
				BAsmCode[CodeLen++] = dir;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				if (dir) BAsmCode[CodeLen++] = mdisp;
				return;
			}

			if (DecodeDirect(&ArgStr[2], &addr))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xd0;
				BAsmCode[CodeLen++] = disp1;
				AddrOut20(addr);
				return;
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xcc;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeRegIndirect(&ArgStr[1], &reg1, &mode1))
		{
			if (DecodeReg(&ArgStr[2], &reg2) && reg2 != RegS)
			{
				BAsmCode[0] = 0xb0 | reg2;
				BAsmCode[1] = mode1 | reg1;
				CodeLen = 2;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0xe8;
				BAsmCode[CodeLen++] = mode1 | reg1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeRegBase(&ArgStr[1], &reg1, &mode1, &disp1))
		{
			if (DecodeReg(&ArgStr[2], &reg2))
			{
				BAsmCode[0] = 0xb0 | reg2;
				BAsmCode[1] = mode1 | reg1;
				BAsmCode[2] = disp1;
				CodeLen = 3;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0xe8;
				BAsmCode[CodeLen++] = mode1 | reg1;
				BAsmCode[CodeLen++] = disp2;
				BAsmCode[CodeLen++] = disp1;
				return;				
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeIntIndirect(&ArgStr[1], &dir, &mdisp, &mode1, &disp1, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg2))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xb8 | reg2;
				BAsmCode[CodeLen++] = dir;
				BAsmCode[CodeLen++] = disp1;
				if (dir) BAsmCode[CodeLen++] = mdisp;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xf8;
				BAsmCode[CodeLen++] = dir;
				BAsmCode[CodeLen++] = disp1;
				if (dir) BAsmCode[CodeLen++] = mdisp;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeDirect(&ArgStr[1], &addr))
		{
			if (DecodeReg(&ArgStr[2], &reg2) && IsCoreReg(reg2))
			{
				BAsmCode[CodeLen++] = 0xa8 | reg2;
				AddrOut20(addr);
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0xd8;
				AddrOut20(addr);
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
		return;
	}
}

static void DecodeMVW(Word Index)
{
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;
	Byte reg;
	Byte dir;
	Byte mdisp;
	LongInt val;
	LongInt addr;
	int i;

	if (ChkArgCnt(2, 2))
	{
		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = (Index == 3) ? 0xcf : 0xc9 + Index;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (IsWPL(Index) &&	DecodeRegIndirect(&ArgStr[2], &reg, &mode2))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xe1 + Index;
				BAsmCode[CodeLen++] = mode2 | reg;
				BAsmCode[CodeLen++] = disp1;
				return;
			}

			if (IsWPL(Index) &&	DecodeRegBase(&ArgStr[2], &reg, &mode2, &disp2))
			{
				const Byte optab[] = { 0xe1, 0xe2, 0x56, 0x00 };

				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = optab[Index];
				BAsmCode[CodeLen++] = mode2 | reg;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (IsWPL(Index) &&	DecodeIntIndirect(&ArgStr[2], &dir, &mdisp, &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xf1 + Index;
				BAsmCode[CodeLen++] = dir;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				if (dir) BAsmCode[CodeLen++] = mdisp;
				return;
			}

			if (IsWPL(Index) &&	DecodeDirect(&ArgStr[2], &addr))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0xd1 + Index;
				BAsmCode[CodeLen++] = disp1;
				AddrOut20(addr);
				return;
			}

			if (IsWP(Index) && DecodeImm(&ArgStr[2], &val, Index + 2))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = (Index == 0) ? 0xcd : 0xdc;
				BAsmCode[CodeLen++] = disp1;
				for (i = 0; i < Index + 2; i++)
				{
					BAsmCode[CodeLen++] = val >> (i * 8);
				}
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeRegIndirect(&ArgStr[1], &reg, &mode1))
		{
			if (IsWPL(Index) &&	DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0xe9 + Index;
				BAsmCode[CodeLen++] = mode1 | reg;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeRegBase(&ArgStr[1], &reg, &mode1, &disp1))
		{
			if (IsWPL(Index) &&	DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				const Byte optab[] = { 0xe9, 0xea, 0x5e, 0x00 };

				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = optab[Index];
				BAsmCode[CodeLen++] = mode1 | reg;
				BAsmCode[CodeLen++] = disp2;
				BAsmCode[CodeLen++] = disp1;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeIntIndirect(&ArgStr[1], &dir, &mdisp, &mode1, &disp1, "PX"))
		{
			if (IsWPL(Index) &&	DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xf9 + Index;
				BAsmCode[CodeLen++] = dir;
				BAsmCode[CodeLen++] = disp1;
				if (dir) BAsmCode[CodeLen++] = mdisp;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeDirect(&ArgStr[1], &addr))
		{
			if (IsWPL(Index) &&	DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = 0xd9 + Index;
				AddrOut20(addr);
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
		return;
	}
}

static void DecodeEX(Word Index)
{
	Byte reg1;
	Byte reg2;
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;

  UNUSED(Index);

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg1))
		{
			if (DecodeReg(&ArgStr[2], &reg2))
			{
				if (RegSize(reg1) != RegSize(reg2))
				{
					WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);	/* Register size conflict */
					return;
				}

				if (RegSize(reg1) == 1)
				{
					if (reg1 != RegA || reg2 != RegB)
					{
						WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);	/* Register size conflict */
						return;
					}

					BAsmCode[0] = 0xdd;
					CodeLen =1;
					return;
				}
				else
				{
					if ((IsR2(reg1) && IsR2(reg2)) || (IsR3(reg1) && IsR3(reg2)))
					{
						BAsmCode[0] = 0xed;
						BAsmCode[1] = (reg1 << 4) | reg2;
						CodeLen = 2;
						return;
					}
				}
				
				WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xc0;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
		return;
	}
}

static void DecodeEXW(Word Index)
{
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;

	if (ChkArgCnt(2, 2))
	{
		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = Index;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
		return;
	}
}

static void DecodeSWAP(Word Index)
{
	Byte reg;

	if (ChkArgCnt(1, 1))
	{
		if (DecodeReg(&ArgStr[1], &reg))
		{
			if (reg == RegA)
			{
				BAsmCode[0] = Index;
				CodeLen = 1;
				return;
			}
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
		return;
	}
}

static void DecodeADD(Word Index)
{
	Byte reg1;
	Byte reg2;
	Byte mode;
	Byte disp;
	LongInt val;

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg1))
		{
			if (DecodeReg(&ArgStr[2], &reg2))
			{
				if (IsR1(reg1) && IsR1(reg2))
				{
					BAsmCode[0] = Index | 0x06;
				}
				else if (IsR2(reg1) && (IsR1(reg2) || IsR2(reg2)))
				{
					BAsmCode[0] = Index | 0x04;
				}
				else if (IsR3(reg1) && (IsR1(reg2) || IsR2(reg2) || IsR3(reg2)))
				{
					BAsmCode[0] = Index | 0x05;
				}
				else
				{
					WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
					return;
				}
				BAsmCode[1] = (reg1 << 4) | reg2;
				CodeLen = 2;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode, &disp, "PX"))
			{
				if (reg1 == RegA)
				{
					PreByte(mode, 1);
					BAsmCode[CodeLen++] = Index | 0x02;
					BAsmCode[CodeLen++] = disp;
					return;
				}
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				if (reg1 == RegA)
				{
					BAsmCode[0] = Index;
					BAsmCode[1] = val;
					CodeLen = 2;
					return;
				}
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg2))
			{
				if (reg2 == RegA)
				{
					PreByte(mode, 1);
					BAsmCode[CodeLen++] = Index | 0x03;
					BAsmCode[CodeLen++] = disp;
					return;
				}
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode, 1);
				BAsmCode[CodeLen++] = Index | 0x01;
				BAsmCode[CodeLen++] = disp;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeADC(Word Index)
{
	Byte reg;
	LongInt val;
	Byte mode;
	Byte disp;

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg))
		{
			if (DecodeInternal(&ArgStr[2], &mode, &disp, "PX"))
			{
				if (reg == RegA)
				{
					PreByte(mode, 1);
					BAsmCode[CodeLen++] = Index | 0x02;
					BAsmCode[CodeLen++] = disp;
					return;
				}
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				if (reg == RegA)
				{
					BAsmCode[0] = Index;
					BAsmCode[1] = val;
					CodeLen = 2;
					return;
				}

				WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg))
			{
				if (reg == RegA)
				{
					PreByte(mode, 1);
					BAsmCode[CodeLen++] = Index | 0x03;
					BAsmCode[CodeLen++] = disp;
					return;
				}
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode, 1);
				BAsmCode[CodeLen++] = Index | 0x01;
				BAsmCode[CodeLen++] = disp;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeADCL(Word Index)
{
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;
	Byte reg;

  UNUSED(Index);

	if (ChkArgCnt(2, 2))
	{
		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg))
			{
				if (reg == RegA)
				{
					PreByte(mode1, 1);
					BAsmCode[CodeLen++] = Index | 0x01;
					BAsmCode[CodeLen++] = disp1;
					return;
				}
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = Index;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}		
		
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodePMDF(Word Index)
{
	Byte mode;
	Byte disp;
	Byte reg;
	LongInt val;

  UNUSED(Index);

	if (ChkArgCnt(2, 2))
	{
		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg))
			{
				if (reg == RegA)
				{
					PreByte(mode, 1);
					BAsmCode[CodeLen++] = 0x57;
					BAsmCode[CodeLen++] = disp;
					return;
				}
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode, 1);
				BAsmCode[CodeLen++] = 0x47;
				BAsmCode[CodeLen++] = disp;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}		
		
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeAND(Word Index)
{
	Byte reg;
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;
	LongInt addr;
	LongInt val;

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg) && reg == RegA)
		{
			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PX"))
			{
				PreByte(mode2, 1);
				BAsmCode[CodeLen++] = Index | 0x07;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				BAsmCode[0] = Index;
				BAsmCode[1] = val;
				CodeLen = 2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg) && reg == RegA)
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = Index | 0x03;
				BAsmCode[CodeLen++] = disp1;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = Index | 0x06;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = Index | 0x01;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeDirect(&ArgStr[1], &addr))
		{
			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				BAsmCode[CodeLen++] = Index | 0x02;
				AddrOut20(addr);
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeINC(Word Index)
{
	Byte reg;
	Byte mode;
	Byte disp;

	if (ChkArgCnt(1, 1))
	{
		if (DecodeReg(&ArgStr[1], &reg) && IsCoreReg(reg))
		{
			BAsmCode[0] = Index;
			BAsmCode[1] = reg;
			CodeLen = 2;
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			PreByte(mode, 1);
			BAsmCode[CodeLen++] = Index | 0x01;
			BAsmCode[CodeLen++] = disp;
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeROR(Word Index)
{
	Byte reg;
	Byte mode;
	Byte disp;

	if (ChkArgCnt(1, 1))
	{
		if (DecodeReg(&ArgStr[1], &reg) && reg == RegA)
		{
			BAsmCode[0] = Index;
			CodeLen = 1;
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			PreByte(mode, 1);
			BAsmCode[CodeLen++] = Index | 0x01;
			BAsmCode[CodeLen++] = disp;
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeDSRL(Word Index)
{
	Byte mode;
	Byte disp;

  UNUSED(Index);

	if (ChkArgCnt(1, 1))
	{
		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			PreByte(mode, 1);
			BAsmCode[CodeLen++] = Index;
			BAsmCode[CodeLen++] = disp;
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeCMP(Word Index)
{
	Byte reg;
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;
	LongInt val;
	LongInt addr;

  UNUSED(Index);

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg) && reg == RegA)
		{
			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				BAsmCode[0] = 0x60;
				BAsmCode[1] = val;
				CodeLen = 2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg) && reg == RegA)
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0x63;
				BAsmCode[CodeLen++] = disp1;
				return;
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xb7;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode1, 1);
				BAsmCode[CodeLen++] = 0x61;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeDirect(&ArgStr[1], &addr))
		{
			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				BAsmCode[CodeLen++] = 0x62;
				AddrOut20(addr);
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeCMPW(Word Index)
{
	Byte reg;
	Byte mode1;
	Byte mode2;
	Byte disp1;
	Byte disp2;

  UNUSED(Index);

	if (ChkArgCnt(2, 2))
	{
		if (DecodeInternal(&ArgStr[1], &mode1, &disp1, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg))
			{
				if (IsCoreReg(reg) && RegSize(reg) == Index)
				{
					PreByte(mode1, 1);
					BAsmCode[CodeLen++] = 0xd4 | Index;
					BAsmCode[CodeLen++] = reg;
					BAsmCode[CodeLen++] = disp1;
					return;
				}
			}

			if (DecodeInternal(&ArgStr[2], &mode2, &disp2, "PY"))
			{
				PreByte(mode1, mode2);
				BAsmCode[CodeLen++] = 0xc4 | Index;
				BAsmCode[CodeLen++] = disp1;
				BAsmCode[CodeLen++] = disp2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeTEST(Word Index)
{
	Byte reg;
	Byte mode;
	Byte disp;
	LongInt val;
	LongInt addr;

  UNUSED(Index);

	if (ChkArgCnt(2, 2))
	{
		if (DecodeReg(&ArgStr[1], &reg) && reg == RegA)
		{
			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				BAsmCode[0] = 0x64;
				BAsmCode[1] = val;
				CodeLen = 2;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeDirect(&ArgStr[1], &addr))
		{
			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				BAsmCode[CodeLen++] = 0x66;
				AddrOut20(addr);
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		if (DecodeInternal(&ArgStr[1], &mode, &disp, "PX"))
		{
			if (DecodeReg(&ArgStr[2], &reg) && reg == RegA)
			{
				PreByte(mode, 1);
				BAsmCode[CodeLen++] = 0x67;
				BAsmCode[CodeLen++] = disp;
				return;
			}

			if (DecodeImm(&ArgStr[2], &val, 1))
			{
				PreByte(mode, 1);
				BAsmCode[CodeLen++] = 0x65;
				BAsmCode[CodeLen++] = disp;
				BAsmCode[CodeLen++] = val;
				return;
			}

			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeJP(Word Index)
{
	const Byte OP = Index & 0xff;
	const int alen = (Index & 0x0100) ? 3 : 2;
	int i;
	Boolean ValOK;
	tSymbolFlags flags;
	LongInt addr;
	Byte mode;
	Byte disp;
	Byte reg;

	if (ChkArgCnt(1, 1))
	{
		if (Index & 0x0200)
		{
			if (DecodeReg(&ArgStr[1], &reg) && IsR3(reg))
			{
				BAsmCode[0] = 0x11;
				BAsmCode[1] = reg;
				CodeLen = 2;
				return;
			}

			if (DecodeInternal(&ArgStr[1], &mode, &disp, "PY"))
			{
				PreByte(mode, 1);
				BAsmCode[CodeLen++] = 0x10;
				BAsmCode[CodeLen++] = disp;
				return;
			}
		}

		addr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int20, &ValOK, &flags);
		if (ValOK)
		{
			if (!(Index & 0x100) &&
				!mFirstPassUnknownOrQuestionable(flags) &&
				(addr & 0xf0000) != (EProgCounter() & 0xf0000))
			{
				WrStrErrorPos(ErrNum_TargOnDiffPage, &ArgStr[1]);
				return;
			}

			BAsmCode[CodeLen++] = OP;
			for (i = 0; i < alen; i++)
			{
				BAsmCode[CodeLen++] = addr >> (i * 8);
			}
			return;
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodeJR(Word Index)
{
	Byte OpCode = Index;
	LongInt addr;
	int disp;
	Boolean ValOK;
	tSymbolFlags flags;

	if (ChkArgCnt(1, 1))
	{
		addr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int20, &ValOK, &flags);
		if (!mFirstPassUnknownOrQuestionable(flags) &&
			(addr & 0xf0000) != (EProgCounter() & 0xf0000))
		{
			WrStrErrorPos(ErrNum_TargOnDiffPage, &ArgStr[1]);
			return;
		}

		disp = addr - (EProgCounter() + 2);

		if (!mFirstPassUnknownOrQuestionable(flags))
		{
			if (disp < -255 || disp > 255)
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

static void DecodeFixed(Word Index)
{
	if (ChkArgCnt(0, 0))
	{
		BAsmCode[0] = Index;
		CodeLen = 1;
	}
}

static void DecodePUSHS(Word Index)
{
	Byte reg;

	if (ChkArgCnt(1, 1))
	{
		if (DecodeReg(&ArgStr[1], &reg))
		{
			if (IsR1(reg) || IsR2(reg) || reg == RegX || reg == RegY)
			{
				BAsmCode[0] = (Index ? 0x90 : 0xb0) | reg;
				BAsmCode[1] = 0x37;
				CodeLen = 2;
				return;
			}

			if (reg == RegF)
			{
				BAsmCode[0] = Index ? 0x5f : 0x4f;
				CodeLen = 1;
				return;
			}

			if (reg == RegIMR)
			{
				BAsmCode[0] = 0x30;
				BAsmCode[1] = Index ? 0xe0 : 0xe8;
				BAsmCode[2] = Index ? 0x27 : 0x37;
				BAsmCode[3] = 0xfb;
				CodeLen = 4;
				return;
			}
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

static void DecodePUSHU(Word Index)
{
	const Byte OpBase = Index ? 0x38 : 0x28;
	Byte reg;

	if (ChkArgCnt(1, 1))
	{
		if (DecodeReg(&ArgStr[1], &reg))
		{
			if (IsR1(reg) || IsR2(reg) || reg == RegX || reg == RegY)
			{
				BAsmCode[0] = OpBase | reg;
				CodeLen = 1;
				return;
			}

			if (reg == RegF)
			{
				BAsmCode[0] = OpBase | 0x06;
				CodeLen = 1;
				return;
			}

			if (reg == RegIMR)
			{
				BAsmCode[0] = OpBase | 0x07;
				CodeLen = 1;
				return;
			}
		}

		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
	}
}

/*---------------------------------------------------------------------------*/

static void AddMVW(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVW);
}

static void AddEXW(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeEXW);
}

static void AddADD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeADD);
}

static void AddADC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeADC);
}

static void AddADCL(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeADCL);
}

static void AddAND(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeAND);
}

static void AddINC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeINC);
}

static void AddROR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeROR);
}

static void AddDSRL(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDSRL);
}

static void AddCMPW(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeCMPW);
}

static void AddJP(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeJP);
}

static void AddJR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeJR);
}

static void AddFixed(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddPUSHS(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodePUSHS);
}

static void AddPUSHU(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodePUSHU);
}

static void InitFields(void)
{
	InstTable = CreateInstTable(64);

	/* a. MV */
	AddInstTable(InstTable, "MV", 0, DecodeMV);
	AddMVW("MVW", 0);
	AddMVW("MVP", 1);
	AddMVW("MVL", 2);
	AddMVW("MVLD", 3);

	/* b. EX/SWAP */
	AddInstTable(InstTable, "EX", 0, DecodeEX);
	AddEXW("EXW", 0xc1);
	AddEXW("EXP", 0xc2);
	AddEXW("EXL", 0xc3);
	AddInstTable(InstTable, "SWAP", 0xee, DecodeSWAP);

	/* c. ADD/ADC/SUB/SBC/DADL/DSBL/PMDF */
	AddADD("ADD", 0x40);
	AddADC("ADC", 0x50);
	AddADD("SUB", 0x48);
	AddADC("SBC", 0x58);
	AddADCL("ADCL", 0x54);
	AddADCL("SBCL", 0x5c);
	AddADCL("DADL", 0xc4);
	AddADCL("DSBL", 0xd4);
	AddInstTable(InstTable, "PMDF", 0, DecodePMDF);

	/* d. AND/OR/XOR */
	AddAND("AND", 0x70);
	AddAND("OR", 0x78);
	AddAND("XOR", 0x68);

	/* e. INC/DEC */
	AddINC("INC", 0x6c);
	AddINC("DEC", 0x7c);

	/* f. ROR */
	AddROR("ROR", 0xe4);
	AddROR("ROL", 0xe6);
	AddROR("SHR", 0xf4);
	AddROR("SHL", 0xf6);
	AddDSRL("DSRL", 0xfc);
	AddDSRL("DSLL", 0xec);

	/* g. CMP/TEST */
	AddInstTable(InstTable, "CMP", 0, DecodeCMP);
	AddCMPW("CMPW", 2);
	AddCMPW("CMPP", 3);
	AddInstTable(InstTable, "TEST", 0, DecodeTEST);

	/* h. JP/JPF/JR/CALL */
	AddJP("JP", 0x02 | 0x0200);
	AddJP("JPF", 0x03 | 0x0100);
	AddJR("JR", 0x12);
	AddJP("JPZ", 0x14);
	AddJP("JPNZ", 0x15);
	AddJP("JPC", 0x16);
	AddJP("JPNC", 0x17);
	AddJR("JRZ", 0x18);
	AddJR("JRNZ", 0x1a);
	AddJR("JRC", 0x1c);
	AddJR("JRNC", 0x1e);

	/* i. CALL/CALLF/RET */
	AddJP("CALL", 0x04);
	AddJP("CALLF", 0x05 | 0x0100);
	AddFixed("RET", 0x06);
	AddFixed("RETF", 0x07);

	/* j. PUSH/POP */
	AddPUSHS("PUSHS", 0);
	AddPUSHU("PUSHU", 0);
	AddPUSHS("POPS", 1);
	AddPUSHU("POPU", 1);

	/* k. Other */
	AddFixed("NOP", 0x00);
	AddFixed("WAIT", 0xef);
	AddFixed("SC", 0x97);
	AddFixed("RC", 0x9f);
	AddFixed("RETI", 0x01);
	AddFixed("HALT", 0xde);
	AddFixed("OFF", 0xdf);
	AddFixed("TCL", 0xce);
	AddFixed("IR", 0xfe);
	AddFixed("RESET", 0xff);
}

static void DeinitFields(void)
{
	DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_SC62015(void)
{
	CodeLen = 0;
	DontPrint = False;

	if (Memo("")) return;

	if (DecodeIntelPseudo(False)) return;

	if (!LookupInstTable(InstTable, OpPart.str.p_str))
		WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);

	if ((EProgCounter() & 0xf0000) != ((EProgCounter() + CodeLen) & 0xf0000))
		WrError(ErrNum_PageCrossing);
}

static Boolean IsDef_SC62015(void)
{
	return False;
}

static void SwitchFrom_SC62015(void)
{
	DeinitFields();
}

static void SwitchTo_SC62015(void)
{
 	const TFamilyDescr *pDescr;
	TurnWords = False;
	SetIntConstMode(eIntConstModeIntel);

	pDescr = FindFamilyByName("SC62015");
	PCSymbol = "*";
	HeaderID = pDescr->Id;
	NOPCode = 0x00;	/* NOP */
	DivideChars = ",";
	HasAttrs = False;

	ValidSegs = (1 << SegCode) | (1 << SegReg);
	Grans[SegCode] = 1;
	ListGrans[SegCode] = 1;
	SegInits[SegCode] = 0x00000;
	SegLimits[SegCode] = 0xfffff;
	Grans[SegReg] = 1;
	ListGrans[SegReg] = 1;
	SegInits[SegReg] = 0x00;
	SegLimits[SegReg] = 0xff;

	MakeCode = MakeCode_SC62015;
	IsDef = IsDef_SC62015;
	SwitchFrom = SwitchFrom_SC62015;
	InitFields();
}

void code62015_init(void)
{
	CPUSC62015 = AddCPU("SC62015", SwitchTo_SC62015);
}
