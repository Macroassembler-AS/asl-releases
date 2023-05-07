/* coderx.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Renesas RX                                                  */
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
#include "ieeefloat.h"
#include "onoff_common.h"
#include "intpseudo.h"
#include "endian.h"

#include "coderx.h"

/*---------------------------------------------------------------------------*/

/* Define this to use Renesas Assembler like pseudo instructions */
#define COMPAT

/*---------------------------------------------------------------------------*/

typedef enum
{
	eRn,
	eDRn,
	eDRHn,
	eDRLn,
	eDCRn
} tRegType;

typedef enum
{
	eRXv1,
	eRXv2,
	eRXv3
} tInstSet;

typedef struct
{
	char Name[7];
	tInstSet InstSet;
	Boolean hasFloat;
	Boolean hasDouble;
	Word RegBank;
	Boolean hasMVTIPL;
} tCPUProps;

/*---------------------------------------------------------------------------*/

static const tCPUProps *pCurrCPUProps;

static tStrComp Temp1;
static tStrComp Temp2;

/*---------------------------------------------------------------------------*/

static Boolean ChkNoAttr(void)
{
	if (AttrPart.str.p_str[0])
	{
		WrError(ErrNum_UseLessAttr);
		return False;
	}

	return True;
}

static Boolean CheckSup(void)
{
	if (SupAllowed) return True;
	
	WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
	return False;
}

static Boolean CheckV2(void)
{
	if (pCurrCPUProps->InstSet >= eRXv2) return True;

	WrError(ErrNum_InstructionNotSupported);
	return False;
}

static Boolean CheckV3(void)
{
	if (pCurrCPUProps->InstSet >= eRXv3) return True;

	WrError(ErrNum_InstructionNotSupported);
	return False;
}

static Boolean CheckFloat(void)
{
	if (pCurrCPUProps->hasFloat) return True;

	WrError(ErrNum_FPUNotEnabled);
	return False;
}

static Boolean CheckDouble(void)
{
	if (pCurrCPUProps->hasDouble) return True;

	WrError(ErrNum_FPUNotEnabled);
	return False;
}

static const char *DCReg[] = {
	"DPSW",
	"DCMR",
	"DECNT",
	"DEPC"
};

static Boolean DecodeReg(const tStrComp *pArg, Byte *pResult, tRegType type)
{
	const char *str = pArg->str.p_str;
	const int len = strlen(str);
	int i;
	int num = 16;

	switch (type)
	{
	case eRn:
		if (as_strncasecmp(str, "R", 1)) return False;
		i = 1;
		break;
	case eDRn:
		if (as_strncasecmp(str, "DR", 2)) return False;
		i = 2;
		break;
	case eDRHn:
		if (as_strncasecmp(str, "DRH", 3)) return False;
		i = 3;
		break;
	case eDRLn:
		if (as_strncasecmp(str, "DRL", 3)) return False;
		i = 3;
		break;
	case eDCRn:
		for (i = 0; i < 4; i++)
		{
			if (!as_strcasecmp(str, DCReg[i]))
			{
				*pResult = i;
				return True;
			}
		}
		if (as_strncasecmp(str, "DCR", 3)) return False;
		i = 3;
		num = 4;
		break;
	}
	*pResult = 0;
	for (; i < len; i++)
	{
		if (!isdigit(str[i])) return False;
		*pResult = *pResult * 10 + (str[i] - '0');
	}

	return *pResult < num;
}

static Boolean DecodeImm(const tStrComp *pArg, LongInt *pResult, tSymbolFlags *pFlags)
{
	const char *str = pArg->str.p_str;
	tStrComp ImmArg;
	Boolean ValOK;
	tSymbolFlags flags;

	if (str[0] != '#') return False;

	StrCompRefRight(&ImmArg, pArg, 1);
	*pResult = EvalStrIntExpressionWithFlags(&ImmArg, Int32, &ValOK, &flags);
	if (pFlags) *pFlags = flags;

	return ValOK;
}

static Byte ImmSize32(LongInt value, tSymbolFlags flags)
{
	if (mFirstPassUnknown(flags)) return 0x00;	/* Temporarily return maximum size */

	if ((value & 0xFFFFFF80) == 0xFFFFFF80 ||
		(value & 0xFFFFFF80) == 0x00000000) return 0x01;	/* SIMM:8 */

	if ((value & 0xFFFF8000) == 0xFFFF8000 ||
		(value & 0xFFFF8000) == 0x00000000) return 0x02;	/* SIMM:16 */

	if ((value & 0xFF800000) == 0xFF800000 ||
		(value & 0xFF800000) == 0x00000000) return 0x03;	/* SIMM:24 */

	return 0x00;	/* IMM:32 */
}

static Byte ImmSize16(LongInt value, tSymbolFlags flags)
{
	if (mFirstPassUnknown(flags)) return 0x02;	/* Temporarily return maximum size */

	if ((value & 0xFFFFFF80) == 0xFFFFFF80 ||
		(value & 0xFFFFFF80) == 0x00000000) return 0x01;	/* SIMM:8 */

	return 0x02;	/* IMM:16 */
}

static int ImmOut(int pos, Byte size, LongInt imm)
{
	int i;

	if (size == 0x00) size = 4;
	for (i = 0; i < size; i++)
	{
		BAsmCode[pos+i] = imm & 0xFF;
		imm >>= 8;
	}
	return pos + size;
}

static Boolean DecodeIndirectADC(const tStrComp *pArg, Byte *reg, LongInt *disp, tSymbolFlags *flags)
{
	const char *str = pArg->str.p_str;
	const int len = strlen(str);
	int pos;
	Boolean ValOK;

	if (str[len-1] != ']') return False;
	for (pos = len - 2; pos >= 0; pos--)
	{
		if (str[pos] == '[') break;
	}
	if (pos < 0) return False;

	StrCompCopySub(&Temp1, pArg, pos + 1, len - pos - 2);
	if (!DecodeReg(&Temp1, reg, eRn)) return False;

	if (pos == 0)
	{
		*flags = eSymbolFlag_None;
		*disp = 0;
		return True;
	}

	StrCompCopySub(&Temp1, pArg, 0, pos);
	*disp = EvalStrIntExpressionWithFlags(&Temp1, UInt20, &ValOK, flags);

	return ValOK;
}

static Byte DispSize(LongInt disp, tSymbolFlags flags, Byte scale)
{
	Byte size = 0;
	Byte mask = scale - 1;

  UNUSED(flags);

	if (disp & mask)
	{
		WrStrErrorPos(ErrNum_NotAligned, &ArgStr[1]);
		return 0;
	}
	disp /= scale;

	if (disp == 0) size = 0x00;
	else if (disp < 0) size = 0xFF;
	else if (disp < 256) size = 0x01;
	else if (disp < 65536) size = 0x02;
	else size = 0xFF;

	return size;
}

static int DispOut(int pos, Byte size, LongInt disp, Byte scale)
{
	int i;

	disp /= scale;

	if (size > 0x02) size = 0x00;
	for (i = 0; i < size; i++ )
	{
		BAsmCode[pos+i] = disp & 0xFF;
		disp >>= 8;
	}
	return  pos + size;
}

static Boolean DecodeIndirectADD(const tStrComp *pArg, Byte *reg, LongInt *disp, tSymbolFlags *flags, Byte *memex, Byte *scale)
{
	const char *str = pArg->str.p_str;
	const int len = strlen(str);

	if (len > 2 && str[len-2] == '.')
	{
		switch (as_toupper(str[len-1]))
		{
		case 'B':
			*memex = 0x00;
			*scale = 1;
			break;
		case 'W':
			*memex = 0x01;
			*scale = 2;
			break;
		case 'L':
			*memex = 0x02;
			*scale = 4;
			break;
		default:
			return False;
		}
		StrCompCopySub(&Temp2, pArg, 0, len - 2);
		pArg = &Temp2;
	}
	else if (len > 3 && str[len-3] == '.' && as_toupper(str[len-2]) == 'U')
	{
		switch (as_toupper(str[len-1]))
		{
		case 'B':
			*memex = 0x80;
			*scale = 1;
			break;
		case 'W':
			*memex = 0x03;
			*scale = 2;
			break;
		default:
			return False;
		}
		StrCompCopySub(&Temp2, pArg, 0, len - 3);
		pArg = &Temp2;
	}

	return DecodeIndirectADC(pArg, reg, disp, flags);
}

static Boolean DecodeFloat(const tStrComp *pArg, LongInt *pResult)
{
	const char *str = pArg->str.p_str;
	tStrComp ImmArg;
	TempResult temp;
  Boolean Result = True;

	if (str[0] != '#') return False;

	StrCompRefRight(&ImmArg, pArg, 1);
  as_tempres_ini(&temp);
	EvalStrExpression(&ImmArg, &temp);
	switch (temp.Typ)
	{
	case TempInt:
		*pResult = temp.Contents.Int;
		break;
	case TempFloat:
		Double_2_ieee4(temp.Contents.Float, (unsigned char *)pResult, False);
    /* TODO: rework this - we should better pass in a byte array as pResult */
    if (HostBigEndian)
      DSwap(pResult, 4);
		break;
	default:
		Result = False;
	}
  as_tempres_free(&temp);

	return Result;
}

static Boolean DecodeIndirectL(const tStrComp *pArg, Byte *reg, LongInt *disp, tSymbolFlags *flags)
{
	Boolean result;
	Byte memex = 0x02;	/* L */
	Byte scale;

	result = DecodeIndirectADD(pArg, reg, disp, flags, &memex, &scale);

	if (result && memex != 0x02) result = False;

	return result;
}

static Boolean DecodeAttrSize(Byte *size)
{
	const int len = strlen(AttrPart.str.p_str);

	if (len > 1)
	{
		WrStrErrorPos(ErrNum_TooLongAttr, &AttrPart);
		return False;
	}

	if (len)
	{
		switch (AttrPart.str.p_str[0])
		{
		case 'B':
			*size = 0x00;
			break;
		case 'W':
			*size = 0x01;
			break;
		case 'L':
			*size = 0x02;
			break;
		default:
			WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
			return False;
		}

		return True;
	}

	WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
	return False;
}

static Boolean DecodeRelative(const tStrComp *pArg, Byte *reg, LongInt *disp, tSymbolFlags *flags)
{	/* dsp:5[Rn] (R0-R7) */
	const char *str = pArg->str.p_str;
	const int len = strlen(str);
	int pos;
	Boolean ValOK;

	if (str[len-1] != ']') return False;
	for (pos = len - 2; pos > 0; pos--)
	{
		if (str[pos] == '[') break;
	}
	if (pos < 1) return False;

	StrCompCopySub(&Temp1, pArg, pos + 1, len - pos - 2);
	if (!DecodeReg(&Temp1, reg, eRn)) return False;
	if (*reg > 7) return False;

	StrCompCopySub(&Temp1, pArg, 0, pos);
	*disp = EvalStrIntExpressionWithFlags(&Temp1, UInt20, &ValOK, flags);

	return ValOK;
}

static int Size2Scale(Byte size)
{
	Byte scale;

	switch (size)
	{
	case 0x00:	/* B */
		scale = 1;
		break;
	case 0x01:	/* W */
		scale = 2;
		break;
	case 0x02:	/* L */
		scale = 4;
		break;
	default:
		return -1;
	}

	return scale;
}

static Boolean ChkDisp5(Byte size, LongInt disp, tSymbolFlags flags)
{
	const int scale = Size2Scale(size);

	if (scale < 0) return False;
	
	if (!mFirstPassUnknown(flags))
	{
		if (disp & (scale - 1))
		{
			WrError(ErrNum_AddrMustBeAligned);
			return False;
		}

		if (disp / scale > 31 || disp < 0) return False;
	}

	return True;
}

static Byte DispSize5(Byte size, LongInt disp)
{
	const Byte scale = Size2Scale(size);

	return disp / scale;
}

static Boolean DecodeIndexed(tStrComp *pArg, Byte *regi, Byte *regb)
{	/* [Ri,Rb] */
	const char *str = pArg->str.p_str;
	const int len = strlen(str);
	int pos;

	if (str[0] != '[' || str[len-1] != ']') return False;

	for (pos = 2; pos < len - 2; pos++)
	{
		if (str[pos] == ',') break;
	}

	StrCompCopySub(&Temp1, pArg, 1, pos - 1);
	if (!DecodeReg(&Temp1, regi, eRn)) return False;

	StrCompCopySub(&Temp1, pArg, pos + 1, len - pos - 2);
	if (!DecodeReg(&Temp1, regb, eRn)) return False;

	return True;
}

static Boolean DecodeIncDec(tStrComp *pArg, Byte *reg, Byte *id)
{	/* [Rn+] / [-Rn]] */
	const char *str = pArg->str.p_str;
	const int len = strlen(str);

	if (len < 5) return False;
	if (str[0] != '[' || str[len-1] != ']') return False;

	if (str[len-2] == '+')
	{
		*id = 0x00;
		StrCompCopySub(&Temp1, pArg, 1, len - 3);
	}
	else if (str[1] == '-')
	{
		*id = 0x01;
		StrCompCopySub(&Temp1, pArg, 2, len - 3);
	}
	else return False;

	return DecodeReg(&Temp1, reg, eRn);
}

static Boolean DecodeRegRange(tStrComp *pArg, Byte *range)
{
	const char *str = pArg->str.p_str;
	const int len = strlen(str);
	Byte reg1;
	Byte reg2;
	int pos;

	if (len < 5) return False;
	for (pos = 2; pos < len - 2; pos++)
	{
		if (str[pos] == '-') break;
	}
	if (pos >= len - 2) return False;

	StrCompCopySub(&Temp1, pArg, 0, pos);
	if (!DecodeReg(&Temp1, &reg1, eRn)) return False;

	StrCompCopySub(&Temp1, pArg, pos + 1, len - pos - 1);
	if (!DecodeReg(&Temp1, &reg2, eRn)) return False;

	if (reg1 == 0 || reg1 >= reg2) return False;

	*range = (reg1 << 4) | reg2;
	return True;
}

static Boolean DecodeIndirect(tStrComp *pArg, Byte *reg)
{
	const char *str = pArg->str.p_str;
	const int len = strlen(str);

	if (str[0] != '[') return False;
	if (str[len-1] != ']') return False;

	StrCompCopySub(&Temp1, pArg, 1, len - 2);
	if (!DecodeReg(&Temp1, reg, eRn)) return False;

	return True;
}

static Boolean DecodeAcc(tStrComp *pArg, Byte *acc)
{
	const char *str = pArg->str.p_str;
	const int len = strlen(str);

	if (len != 2) return False;
	if (as_toupper(str[0]) != 'A') return False;
	if (str[1] != '0' && str[1] != '1') return False;

	*acc = str[1] - '0';
	return True;
}

static Boolean DecodeAttrDouble(Byte *size)
{
	if (strlen(AttrPart.str.p_str) != 1)
	{
		WrStrErrorPos(ErrNum_TooLongAttr, &AttrPart);
		return False;
	}
	switch (as_toupper(AttrPart.str.p_str[0]))
	{
	case 'L':
		*size = 0;
		break;
	case 'D':
		*size = 1;
		break;
	default:
		WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
		return False;
	}

	return True;
}

/*---------------------------------------------------------------------------*/

static void DecodeABS(Word Index)
{
	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,2)) return;

	if (ArgCnt == 1)
	{
		Byte reg;

		if (!DecodeReg(&ArgStr[1], &reg, eRn))
		{
			WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
			return;
		}
		
		BAsmCode[0] = 0x7E;
		BAsmCode[1] = (Index >> 8) | reg;
		CodeLen = 2;
	}
	else
	{
		Byte regs;
		Byte regd;

		if (!DecodeReg(&ArgStr[1], &regs, eRn))
		{
			WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
			return;
		}
		if (!DecodeReg(&ArgStr[2], &regd, eRn))
		{
			WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
			return;
		}

		BAsmCode[0] = 0xFC;
		BAsmCode[1] = Index & 0xFF;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
	}
}

static void DecodeADC(Word Index)
{
	Byte regs;
	Byte regd;
	Byte size;
	LongInt imm;
	tSymbolFlags flags;
	LongInt disp;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	if ((Index & 0x0800) && DecodeImm(&ArgStr[1], &imm, &flags))	/* ADC only */
	{
		size = ImmSize32(imm, flags);

		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x70 | (size << 2);
		BAsmCode[2] = 0x20 | regd;
		CodeLen = ImmOut(3, size, imm);

		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		BAsmCode[0] = 0xFC;
		BAsmCode[1] = 0x03 | (Index >> 8);
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;

		return;
	}

	if (DecodeIndirectL(&ArgStr[1], &regs, &disp, &flags))
	{
		if (mFirstPassUnknown(flags)) size = 0x02;
		else size = DispSize(disp, flags, 4);

		BAsmCode[0] = 0x06;
		BAsmCode[1] = 0xA0 | size;
		BAsmCode[2] = Index & 0xFF;
		BAsmCode[3] = (regs << 4) | regd;
		CodeLen = DispOut(4, size, disp, 4);
	}
}

static const struct {
	Byte OpcImm4;	/* #IMM:4,Rd */
	Byte OpcImm1;	/* #IMM:*(,Rs),Rd 1st byte */
	Byte OpcImm2;	/* 2nd byte */
	Byte Opc2;	/* Rs,Rd / [Rs].UB,Rd */
	Byte Opc3;	/* Rs,Rs2,Rd */
	Byte Opc4;	/* [Rs],Rd */
	Byte flags;	/* 1<<0:, 1<<1:, 1<<2:#UIMM8 1<<3:Rs,Rs,Rd */
} OpTabADD[] = {
	{ 0x62, 0x70, 0x00, 0x48, 0x20, 0x08, 0x0B },	/* ADD */
	{ 0x64, 0x74, 0x20, 0x50, 0x40, 0x10, 0x09 },	/* AND */
	{ 0x61, 0x74, 0x00, 0x44, 0,    0x04, 0x05 },	/* CMP */
	{ 0x63, 0x74, 0x10, 0x4C, 0x30, 0x0C, 0x09 },	/* MUL */
	{ 0x65, 0x74, 0x30, 0x54, 0x50, 0x14, 0x09 },	/* OR */
	{ 0x60, 0x00, 0x00, 0x40, 0x00, 0x00, 0x08 },	/* SUB */
};

static void DecodeADD(Word Index)
{
	Byte regs1;
	Byte regs2;
	Byte regd;
	Byte size;
	LongInt imm;
	tSymbolFlags flags;
	LongInt disp;
	Byte memex;
	Byte scale;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,3)) return;

	if (!DecodeReg(&ArgStr[2], &regs2, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}
	if (ArgCnt == 2)
		regd = regs2;
	else
	{
		if (!DecodeReg(&ArgStr[3], &regd, eRn))
		{
			WrStrErrorPos(ErrNum_InvRegName, &ArgStr[3]);
			return;
		}
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (ArgCnt == 2 && !mFirstPassUnknown(flags) && imm >= 0 && imm < 16 )
		{	/* #UIMM:4 */
			BAsmCode[0] = OpTabADD[Index].OpcImm4;
			BAsmCode[1] = (imm << 4) | regd;
			CodeLen = 2;
			return;
		}

		if ((OpTabADD[Index].flags & 0x04) && ArgCnt == 2 &&
			!mFirstPassUnknown(flags) && imm >= 0 && imm < 256)
		{	/* #UIMM:8 */
			BAsmCode[0] = 0x75;
			BAsmCode[1] = 0x50 | regd;
			BAsmCode[2] = imm;
			CodeLen = 3;
			return;
		}

		if (((OpTabADD[Index].flags & 0x01) && ArgCnt == 2) || (OpTabADD[Index].flags & 0x02))
		{	/* #SIMM:* */
			size = ImmSize32(imm, flags);

			BAsmCode[0] = OpTabADD[Index].OpcImm1 | size;
			if (OpTabADD[Index].flags & 0x02)
				if (ArgCnt == 2)
					BAsmCode[1] = (regd << 4) | regd;	/* #Imm,Rd */
				else
					BAsmCode[1] = (regs2 << 4) | regd;	/* #imm,Rs,Rd */
			else
				BAsmCode[1] = OpTabADD[Index].OpcImm2 | regd;
			CodeLen = ImmOut(2, size, imm);
			return;
		}
	}

	if (DecodeReg(&ArgStr[1], &regs1, eRn))
	{	/* Rs */
		if (ArgCnt == 2)
		{
			BAsmCode[0] = OpTabADD[Index].Opc2 | 0x03;
			BAsmCode[1] = (regs1 << 4) | regd;
			CodeLen = 2;
		}
		else if (OpTabADD[Index].flags & 0x08)
		{
			BAsmCode[0] = 0xFF;
			BAsmCode[1] = OpTabADD[Index].Opc3 | regd;
			BAsmCode[2] = (regs1 << 4) | regs2;
			CodeLen = 3;
		}
		else WrStrErrorPos(ErrNum_TooManyArgs, &ArgStr[3]);
		return;
	}

	if (ArgCnt == 2 && DecodeIndirectADD(&ArgStr[1], &regs1, &disp, &flags, &memex, &scale))
	{
		if (memex == 0x80)
		{
			size = DispSize(disp, flags, 1);

			BAsmCode[0] = OpTabADD[Index].Opc2 | size;
			BAsmCode[1] = (regs1 << 4) | regd;
			CodeLen = DispOut(2, size, disp, 1);
			return;
		}

		size = DispSize(disp, flags, scale);

		BAsmCode[0] = 0x06;
		BAsmCode[1] = OpTabADD[Index].Opc4 | (memex << 6) | size;
		BAsmCode[2] = (regs1 << 4) | regd;
		CodeLen = DispOut(3, size, disp, scale);
		return;
	}

	WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);
}

static const struct {
	Byte OpcIM1;	/* Imm3,[Rd] */
	Byte OpcIM2;
	Byte OpcRM;		/* Rs,[Rd] / Rs,Rd */
	Byte OpcIR;	/* #Imm5,Rd */
	Byte flags;
} OpTabBCLR[] = {
	{ 0xF0, 0x08, 0x64, 0x7A, 0x00 },	/* BCLR */
	{ 0xF0, 0x00, 0x6C, 0,    0x01 },	/* BNOT */
	{ 0xF0, 0x00, 0x60, 0x78, 0x00 },	/* BSET */
	{ 0xF4, 0x00, 0x68, 0x7C, 0x00 },	/* BTST */
};

static void DecodeBCLR(Word Index)
{
	Byte regs;
	Byte regd;
	Byte size;
	Byte memex;
	Byte scale;
	LongInt imm;
	LongInt disp;
	tSymbolFlags flags;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	memex = 0;
	if (DecodeIndirectADD(&ArgStr[2], &regd, &disp, &flags, &memex, &scale))
	{
		if (memex)
		{
			WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[2]);
			return;
		}
		size = DispSize(disp, flags, 1);
		if (size > 0x02){
			WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
			return;
		}

		if (DecodeImm(&ArgStr[1], &imm, &flags))
		{
			if (!mSymbolQuestionable(flags) && (imm < 0 || imm > 7))
			{
				WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
				return;
			}

			if (OpTabBCLR[Index].flags & 0x01)
			{	/* BNOT */
				BAsmCode[0] = 0xFC;
				BAsmCode[1] = 0xE0 | (imm << 2) | size;
				BAsmCode[2] = 0x0F | (regd << 4);
				CodeLen = DispOut(3, size, disp, 1);
			}
			else
			{	/* BCLR, BSET, BTST */
				BAsmCode[0] = OpTabBCLR[Index].OpcIM1 | size;
				BAsmCode[1] = OpTabBCLR[Index].OpcIM2 | (regd << 4) | imm;
				CodeLen = DispOut(2, size, disp, 1);
			}
			return;
		}
		if (DecodeReg(&ArgStr[1], &regs, eRn))
		{
			BAsmCode[0] = 0xFC;
			BAsmCode[1] = OpTabBCLR[Index].OpcRM | size;
			BAsmCode[2] = regs | (regd << 4);
			CodeLen = DispOut(3, size, disp, 1);
			return;
		}

		WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);		
	}
	else if (DecodeReg(&ArgStr[2], &regd, eRn))
	{
		if (DecodeImm(&ArgStr[1], &imm, &flags))
		{
			if (!mSymbolQuestionable(flags) && (imm < 0 || imm > 31))
			{
				WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
				return;
			}

			if (OpTabBCLR[Index].flags & 0x01)
			{
				BAsmCode[0] = 0xFD;
				BAsmCode[1] = 0xE0 | imm;
				BAsmCode[2] = 0xF0 | regd;
				CodeLen = 3;
			}
			else
			{
				BAsmCode[0] = OpTabBCLR[Index].OpcIR | (imm >> 4);
				BAsmCode[1] = ((imm & 0x0F) << 4) | regd;
				CodeLen = 2;
			}
			return;
		}
		if (DecodeReg(&ArgStr[1], &regs, eRn))
		{
			BAsmCode[0] = 0xFC;
			BAsmCode[1] = OpTabBCLR[Index].OpcRM | 0x03;
			BAsmCode[2] = regs | (regd << 4);
			CodeLen = 3;
			return;
		}

		WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
}

static void DecodeBCnd(Word Index)
{
	const char *str = AttrPart.str.p_str;
	const int len = strlen(str);
	Byte attr = 0;
	Byte reg;
	LongInt addr;
	tSymbolFlags flags;
	Boolean ValOK;
	LongInt disp;

	if (len > 1)
	{
		WrStrErrorPos(ErrNum_TooLongAttr, &AttrPart);
		return;
	}
	if (len == 1)
	{
		attr = as_toupper(str[0]);
		switch (attr)
		{
		case 'S':
			break;
		case 'B':
			break;
		case 'W':
			break;
		case 'A':
		case 'L':
			if (Index != 14)
			{
				WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
				return;
			}
			break;
		default:
			WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
			return;
		}
	}

	if (!ChkArgCnt(1,1)) return;

	if ((attr == 0 || attr == 'L') && DecodeReg(&ArgStr[1], &reg, eRn))
	{
		BAsmCode[0] = 0x7F;
		BAsmCode[1] = 0x40 | reg;
		CodeLen = 2;
		return;
	}

	addr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ValOK, &flags);
	if (ValOK)
	{
		if (attr == 'L')
		{
			WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
			return;
		}

		disp = addr - EProgCounter();

		/* Try 3 bit */
		if ((attr == 0 || attr == 'S') && disp >= 3 && disp <= 10)
		{
			if (disp > 7) disp -= 8;

			if (Index == 0 || Index == 1)
			{

				BAsmCode[0] = 0x10 | (Index << 3) | disp;
				CodeLen = 1;
				return;
			}

			if (Index == 14)
			{
				BAsmCode[0] = 0x08 | disp;
				CodeLen = 1;
				return;
			}

			WrError(ErrNum_InternalError);
		}

		if (attr == 'S' && !mFirstPassUnknownOrQuestionable(flags))
		{
			WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
			return;
		}

		/* Try 8 bit */
		if ((attr == 0 || attr == 'B') &&
			((disp & 0xFFFFFF80) == 0xFFFFFF80 ||
			 (disp & 0xFFFFFF80) == 0x00000000))
		{
			BAsmCode[0] = 0x20 | Index;
			BAsmCode[1] = disp & 0xFF;
			CodeLen = 2;
			return;
		}

		if (attr == 'B' && !mFirstPassUnknownOrQuestionable(flags))
		{
			WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
			return;
		}

		/* Try 16 bit */
		if ((attr == 0 || attr == 'W') &&
			((disp & 0xFFFF8000) == 0xFFFF8000 ||
			 (disp & 0xFFFF8000) == 0x00000000))
		{
			if (Index == 0 || Index == 1)
			{
				BAsmCode[0] = 0x3A | Index;
				BAsmCode[1] = disp & 0xFF;
				BAsmCode[2] = (disp >> 8) & 0xFF;
				CodeLen = 3;
				return;
			}

			if (Index == 14)
			{
				BAsmCode[0] = 0x38;
				BAsmCode[1] = disp & 0xFF;
				BAsmCode[2] = (disp >> 8) & 0xFF;
				CodeLen = 3;
				return;
			}

			WrError(ErrNum_JmpDistTooBig);
			return;
		}
		
		if (attr == 'W' && !mFirstPassUnknownOrQuestionable(flags))
		{
			WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
			return;
		}
		
		/* Try 24 bit */
		if ((disp & 0xFF800000) == 0xFF800000 ||
			(disp & 0xFF800000) == 0x00000000)
		{
			if (Index == 14)
			{
				BAsmCode[0] = 0x04;
				BAsmCode[1] = disp & 0xFF;
				BAsmCode[2] = (disp >> 8) & 0xFF;
				BAsmCode[3] = (disp >> 16) & 0xFF;
				CodeLen = 4;
				return;
			}
		}

		WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
	}
}

static void DecodeBMCnd(Word Index)
{
	Byte regd;
	Byte size;
	Byte memex;
	Byte scale;
	LongInt imm;
	LongInt disp;
	tSymbolFlags flags;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	memex = 0;
	if (DecodeIndirectADD(&ArgStr[2], &regd, &disp, &flags, &memex, &scale))
	{
		if (memex)
		{
			WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[2]);
			return;
		}
		size = DispSize(disp, flags, 1);
		if (size > 0x02){
			WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
			return;
		}
	}
	else if (DecodeReg(&ArgStr[2], &regd, eRn))
	{
		size = 0x03;
	}
	else
	{
		WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (size < 0x03)
		{
			/* #imm,disp[Rd] */
			if (!mSymbolQuestionable(flags) && (imm < 0 || imm > 7))
			{
				WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
				return;
			}

			BAsmCode[0] = 0xFC;
			BAsmCode[1] = 0xE0 | (imm << 2) | size;
			BAsmCode[2] = (regd << 4) | Index;
			CodeLen = DispOut(3, size, disp, 1);
			return;
		}
		else
		{
			/* #imm,Rd */
			if (!mSymbolQuestionable(flags) && (imm < 0 || imm > 31))
			{
				WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
				return;
			}
			
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0xE0 | imm;
			BAsmCode[2] = (Index << 4) | regd;
			CodeLen = 3;
			return;
		}
	}

	WrError(ErrNum_OverRange);
}

static void DecodeBRK(Word Index)
{
	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(0,0)) return;

	BAsmCode[0] = Index;
	CodeLen = 1;
}

static void DecodeBSR(Word Index)
{
	const char *str = AttrPart.str.p_str;
	const int len = strlen(str);
	Byte attr = 0;
	Byte reg;
	LongInt addr;
	tSymbolFlags flags;
	Boolean ValOK;
	LongInt disp;

  UNUSED(Index);

	if (len > 1)
	{
		WrStrErrorPos(ErrNum_TooLongAttr, &AttrPart);
		return;
	}
	if (len == 1)
	{
		attr = as_toupper(str[0]);
		switch (attr)
		{
		case 'W':
		case 'A':
		case 'L':
			break;
		default:
			WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
			return;
		}
	}

	if (!ChkArgCnt(1,1)) return;

	if ((attr == 0 || attr == 'L') && DecodeReg(&ArgStr[1], &reg, eRn))
	{
		BAsmCode[0] = 0x7F;
		BAsmCode[1] = 0x50 | reg;
		CodeLen = 2;
		return;
	}

	addr = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ValOK, &flags);
	if (ValOK)
	{
		if (attr == 'L')
		{
			WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
			return;
		}

		disp = addr - EProgCounter();

		/* Try 16 bit */
		if ((attr == 0 || attr == 'W') &&
			((disp & 0xFFFF8000) == 0xFFFF8000 ||
			 (disp & 0xFFFF8000) == 0x00000000))
		{
			BAsmCode[0] = 0x39;
			BAsmCode[1] = disp & 0xFF;
			BAsmCode[2] = (disp >> 8) & 0xFF;
			CodeLen = 3;
			return;
		}

		if (attr == 'W' && !mFirstPassUnknownOrQuestionable(flags))
		{
			WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
			return;
		}
		
		/* Try 24 bit */
		if ((disp & 0xFF800000) == 0xFF800000 ||
			(disp & 0xFF800000) == 0x00000000)
		{
			BAsmCode[0] = 0x05;
			BAsmCode[1] = disp & 0xFF;
			BAsmCode[2] = (disp >> 8) & 0xFF;
			BAsmCode[3] = (disp >> 16) & 0xFF;
			CodeLen = 4;
			return;
		}

		WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
	}		
}

static const char *BitPSW[] = {
	"C",  "Z",  "S",  "O",  NULL, NULL, NULL, NULL,
	"I",  "U",  NULL, NULL, NULL, NULL, NULL, NULL };

static void DecodeCLRPSW(Word Index)
{
	int i;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	for (i = 0; i < 16; i++)
	{
		if (BitPSW[i] && !as_strcasecmp(ArgStr[1].str.p_str, BitPSW[i])) break;
	}

	if (i < 16)
	{
		BAsmCode[0] = 0x7F;
		BAsmCode[1] = Index | i;
		CodeLen = 2;
	}
	else WrStrErrorPos(ErrNum_UnknownFlag, &ArgStr[1]);
}

static const struct {
	Byte OpcI;
	Byte Opc2;
	Byte OpcM;
} OpTabDIV[] = {
	{ 0x80, 0x20, 0x08 },	/* DIV */
	{ 0x90, 0x24, 0x09 },	/* DIVU */
	{ 0x60, 0x18, 0x06 },	/* EMUL */
	{ 0x70, 0x1C, 0x07 },	/* EMULU */
	{ 0x40, 0x10, 0x04 },	/* MAX */
	{ 0x50, 0x14, 0x05 },	/* MIN */
};

static void DecodeDIV(Word Index)
{
	Byte regs;
	Byte regd;
	Byte size;
	LongInt imm;
	tSymbolFlags flags;
	LongInt disp;
	Byte memex;
	Byte scale;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		size = ImmSize32(imm, flags);

		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x70 | (size << 2);
		BAsmCode[2] = OpTabDIV[Index].OpcI | regd;
		CodeLen = ImmOut(3, size, imm);
		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		BAsmCode[0] = 0xFC;
		BAsmCode[1] = OpTabDIV[Index].Opc2 | 0x03;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
		return;
	}

	if (DecodeIndirectADD(&ArgStr[1], &regs, &disp, &flags, &memex, &scale))
	{
		if (memex == 0x80)
		{
			size = DispSize(disp, flags, 1);

			BAsmCode[0] = 0xFC;
			BAsmCode[1] = OpTabDIV[Index].Opc2 | size;
			BAsmCode[2] = (regs << 4) | regd;
			CodeLen = DispOut(3, size, disp, 1);
			return;
		}

		size = DispSize(disp, flags, scale);

		BAsmCode[0] = 0x06;
		BAsmCode[1] = 0x20 | (memex << 6) | size;
		BAsmCode[2] = OpTabDIV[Index].OpcM;
		BAsmCode[3] = (regs << 4) | regd;
		CodeLen = DispOut(4, size, disp, scale);
		return;
	}

	WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);	
}

static const struct {
	Byte OpcI;
	Byte OpcM;
	Byte Opc3;
	Byte flags;
} OpTabFADD[] = {
	{ 0x20, 0x88, 0xA0, 0x03 },	/* FADD */
	{ 0x10, 0x84, 0,    0x01 },	/* FCMP */
	{ 0x40, 0x90, 0,    0x01 },	/* FDIV */
	{ 0x30, 0x8C, 0xB0, 0x03 },	/* FMUL */
	{ 0x00, 0x80, 0x80, 0x03 },	/* FSUB */
	{ 0,    0x94, 0,    0x00 },	/* FTOI */
	{ 0,    0x98, 0,    0x00 },	/* ROUND */
	{ 0,    0xA0, 0,    0x10 },	/* FSQRT */
	{ 0,    0xA4, 0,    0x10 },	/* FTOU */
};

static void DecodeFADD(Word Index)
{
	Byte regs;
	Byte regd;
	Byte size;
	LongInt imm;
	LongInt disp;
	tSymbolFlags flags;

	if (!ChkNoAttr()) return;
	if (!CheckFloat()) return;

	if ((OpTabFADD[Index].flags & 0x02) && ArgCnt == 3)
	{
		Byte regs2;

		if (!DecodeReg(&ArgStr[1], &regs, eRn))
		{
			WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
			return;
		}
		if (!DecodeReg(&ArgStr[2], &regs2, eRn))
		{
			WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[2]);
			return;
		}
		if (!DecodeReg(&ArgStr[3], &regd, eRn))
		{
			WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[3]);
			return;
		}

		BAsmCode[0] = 0xFF;
		BAsmCode[1] = OpTabFADD[Index].Opc3 | regd;
		BAsmCode[2] = (regs << 4) | regs2;
		CodeLen = 3;
		return;
	}

	if (!ChkArgCnt(2,2)) return;

	if (OpTabFADD[Index].flags & 0x10)
	{
		if (!CheckV2()) return;
	}

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	if ((OpTabFADD[Index].flags & 0x01) &&
		DecodeFloat(&ArgStr[1], &imm))
	{
		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x72;
		BAsmCode[2] = OpTabFADD[Index].OpcI | regd;
		CodeLen = ImmOut(3, 0, imm);	/* size==0 means 4 byte */
		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		size = 0x03;
		disp = 0;
	}
	else if (DecodeIndirectL(&ArgStr[1], &regs, &disp, &flags))
	{
		if (mFirstPassUnknown(flags)) size = 0x02;
		else size = DispSize(disp, flags, 4);
	}
	else
	{
		WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);	
		return;
	}

	BAsmCode[0] = 0xFC;
	BAsmCode[1] = OpTabFADD[Index].OpcM | size;
	BAsmCode[2] = (regs << 4) | regd;
	CodeLen = DispOut(3, size, disp, 4);
}

static void DecodeINT(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (!mFirstPassUnknown(flags) && (imm < -128 || imm > 255))
		{
			WrStrErrorPos(ErrNum_ArgOutOfRange, &ArgStr[1]);
			return;
		}

		BAsmCode[0] = 0x75;
		BAsmCode[1] = 0x60;
		BAsmCode[2] = imm & 0xFF;
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);	
}

static const struct {
	Byte Opc1;
	Byte Opc2;
	Byte flags;
} OpTabITOF[] = {
	{ 0x44, 0x11, 0x00 },	/* ITOF */
	{ 0x54, 0x15, 0x10 },	/* UTOF */
};

static void DecodeITOF(Word Index)
{
	Byte regs;
	Byte regd;
	Byte size;
	Byte memex;
	Byte scale;
	LongInt disp;
	tSymbolFlags flags;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;
	if (!CheckFloat()) return;

	if ((OpTabITOF[Index].flags & 0x10) && !CheckV2()) return;

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		size = 0x03;
		disp = 0;
		memex = 0;
	}
	else if (DecodeIndirectADD(&ArgStr[1], &regs, &disp, &flags, &memex, &scale))
	{
		if (mFirstPassUnknown(flags)) size = 0x02;
		else size = DispSize(disp, flags, scale);
	}
	else
	{
		WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);	
		return;
	}

	if (memex == 0x80 || size == 0x03)
	{
		BAsmCode[0] = 0xFC;
		BAsmCode[1] = OpTabITOF[Index].Opc1 | size;
		BAsmCode[2] = (regs << 4) | regd;
		if (size < 0x03) CodeLen = DispOut(3, size, disp, scale);
		else CodeLen = 3;
		return;
	}

	BAsmCode[0] = 0x06;
	BAsmCode[1] = 0x20 | (memex << 6) | size;
	BAsmCode[2] = OpTabITOF[Index].Opc2;
	BAsmCode[3] = (regs << 4) | regd;
	CodeLen = DispOut(4, size, disp, scale);
}

static const struct {
	Byte Opc1;
	Byte Opc2;
} OpTabJMP[] = {
	{ 0x7F, 0x00 },	/* JMP */
	{ 0x7F, 0x10 },	/* JSR */
	{ 0x7E, 0xB0 },	/* POP */
	{ 0x7E, 0x50 },	/* ROLC */
	{ 0x7E, 0x40 },	/* RORC */
	{ 0x7E, 0x30 },	/* SAT */
};

static void DecodeJMP(Word Index)
{
	Byte reg;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	if (DecodeReg(&ArgStr[1], &reg, eRn))
	{
		BAsmCode[0] = OpTabJMP[Index].Opc1;
		BAsmCode[1] = OpTabJMP[Index].Opc2 | reg;
		CodeLen = 2;
		return;
	}
	
	WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);	
}

static const struct {
	Byte Opc;
	Byte flags;
} OpTabMACHI[] = {
	{ 0x04, 0x01 },	/* MACHI */
	{ 0x05, 0x01 },	/* MACLO */
	{ 0x00, 0x01 },	/* MULHI */
	{ 0x01, 0x01 },	/* MULLO */
	{ 0x67, 0x00 },	/* REVL */
	
	{ 0x65, 0x00 },	/* REVW */
	{ 0x07, 0x11 },	/* EMACA */
	{ 0x47, 0x11 },	/* EMSBA */
	{ 0x03, 0x11 },	/* EMULA */
	{ 0x06, 0x11 },	/* MACLH */

	{ 0x44, 0x11 },	/* MSBHI */
	{ 0x46, 0x11 },	/* MSBLH */
	{ 0x45, 0x11 },	/* MSBLO */
	{ 0x02, 0x11 },	/* MULLH */
};

static void DecodeMACHI(Word Index)
{
	Byte reg1;
	Byte reg2;
	Byte acc = 0;

	if ((OpTabMACHI[Index].flags & 0x10) && !CheckV2()) return;
	if (!ChkNoAttr()) return;
	if (pCurrCPUProps->InstSet == eRXv1)
	{
		if (!ChkArgCnt(2,2)) return;
	}
	else
	{
		if (OpTabMACHI[Index].flags & 0x01)
		{
			if (!ChkArgCnt(3,3)) return;
			if (!DecodeAcc(&ArgStr[3], &acc))
			{
				WrStrErrorPos(ErrNum_InvArg, &ArgStr[3]);
				return;
			}
		}
		else
		{
			if (!ChkArgCnt(2,2)) return;
		}
	}

	if (!DecodeReg(&ArgStr[1], &reg1, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
		return;
	}

	if (!DecodeReg(&ArgStr[2], &reg2, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = OpTabMACHI[Index].Opc | (acc << 3);
	BAsmCode[2] = (reg1 << 4) | reg2;
	CodeLen = 3;
}

static void DecodeMOV(Word Index)
{
	Byte size;
	Byte regs;
	Byte regd;
	LongInt imm;
	LongInt disp;
	tSymbolFlags flags;
	tSymbolFlags flags2;
	Byte disp2;
	Byte isize;
	Byte dsize;
	Byte scale;
	Byte regi;

	UNUSED(Index);

	if (!DecodeAttrSize(&size)) return;
	scale = Size2Scale(size);
	
	if (DecodeReg(&ArgStr[1], &regs, eRn) && regs < 8 &&
		DecodeRelative(&ArgStr[2], &regd, &disp, &flags))
	{	/* (1) */
		if (ChkDisp5(size, disp, flags))
		{
			disp2 = DispSize5(size, disp);
			
			BAsmCode[0] = 0x80 | (size << 4) | ((disp2 >> 2) & 0x07);
			BAsmCode[1] = ((disp2 << 6) & 0x80) | (regd << 4) | ((disp2 << 3) & 0x08) | regs;
			CodeLen = 2;
			return;
		}
	}

	if (DecodeRelative(&ArgStr[1], &regs, &disp, &flags) &&
		DecodeReg(&ArgStr[2], &regd, eRn) && regd < 8)
	{	/* (2) */
		if (ChkDisp5(size, disp, flags))
		{
			disp2 = DispSize5(size, disp);
			
			BAsmCode[0] = 0x88 | (size << 4) | ((disp2 >> 2) & 0x07);
			BAsmCode[1] = ((disp2 << 6) & 0x80) | (regs << 4) | ((disp2 << 3) & 0x08) | regd;
			CodeLen = 2;
			return;
		}
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (size == 0x02 && DecodeReg(&ArgStr[2], &regd, eRn) &&
			!mFirstPassUnknown(flags) && imm >= 0 && imm < 16)
		{	/* (3) */
			BAsmCode[0] = 0x66;
			BAsmCode[1] = (imm << 4) | regd;
			CodeLen = 2;
			return;
		}

		if (DecodeRelative(&ArgStr[2], &regd, &disp, &flags))
		{	/* (4) */
			if ((size == 0x00 && imm >= -128 && imm < 256) ||
				(size != 0x00 && imm >= 0 && imm < 256))
			{
				disp2 = DispSize5(size, disp);

				BAsmCode[0] = 0x3C | size;
				BAsmCode[1] = ((disp2 << 3) & 0x80) | (regd << 4) | (disp2 & 0x0F);
				BAsmCode[2] = imm;
				CodeLen = 3;
				return;
			}
		}

		if (DecodeReg(&ArgStr[2], &regd, eRn))
		{
			if (size == 0x02 && !mFirstPassUnknown(flags) &&
				imm >= 0 && imm < 256)
			{	/* (5) */
				BAsmCode[0] = 0x75;
				BAsmCode[1] = 0x40 | regd;
				BAsmCode[2] = imm;
				CodeLen = 3;
				return;
			}

			/* (6) */
			isize = ImmSize32(imm, flags);

			BAsmCode[0] = 0xFB;
			BAsmCode[1] = (regd << 4) | (isize << 2) | 0x02;

			CodeLen = ImmOut(2, isize, imm);
			return;
		}

		if (DecodeIndirectADC(&ArgStr[2], &regd, &disp, &flags2))
		{	/* (8) */
			switch (size)
			{
			case 0x00:	/* B */
				if (imm < -128 || imm > 255)
				{
					WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
					return;
				}
				isize = 0x01;
				break;
			case 0x01:	/* W */
				if (imm < -32768 || imm > 65535)
				{
					WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
					return;
				}
				isize = ImmSize16(imm, flags);
				break;
			case 0x02:	/* L */
				isize = ImmSize32(imm, flags);
				break;
			}

			dsize = DispSize(disp, flags2, scale);

			BAsmCode[0] = 0xF8 | dsize;
			BAsmCode[1] = (regd << 4) | (isize << 2) | size;
			CodeLen = DispOut(2, dsize, disp, scale);
			CodeLen = ImmOut(CodeLen, isize, imm);
			return;
		}

		WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);			
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		if (DecodeReg(&ArgStr[2], &regd, eRn))
		{	/* (7) */
			BAsmCode[0] = 0xCF | (size << 4);
			BAsmCode[1] = (regs << 4) | regd;
			CodeLen = 2;
			return;
		}

		if (DecodeIndirectADC(&ArgStr[2], &regd, &disp, &flags))
		{	/* (11) */
			dsize = DispSize(disp, flags, scale);

			BAsmCode[0] = 0xC3 | (size << 4) | (dsize << 2);
			BAsmCode[1] = (regd << 4) | regs;
			CodeLen = DispOut(2, dsize, disp, scale);
			return;
		}

		if (DecodeIndexed(&ArgStr[2], &regi, &regd))
		{	/* (12) */
			BAsmCode[0] = 0xFE;
			BAsmCode[1] = (size << 4) | regi;
			BAsmCode[2] = (regd << 4) | regs;
			CodeLen = 3;
			return;
		}

		if (DecodeIncDec(&ArgStr[2], &regd, &regi))
		{	/* (14) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x20 | (regi << 2) | size;
			BAsmCode[2] = (regd << 4) | regs;
			CodeLen = 3;
			return;
		}
	}

	if (DecodeIndirectADC(&ArgStr[1], &regs, &disp, &flags))
	{
		if (DecodeReg(&ArgStr[2], &regd, eRn))
		{	/* (9) */
			dsize = DispSize(disp, flags, scale);

			BAsmCode[0] = 0xCC | (size << 4) | dsize;
			BAsmCode[1] = (regs << 4) | regd;
			CodeLen = DispOut(2, dsize, disp, scale);
			return;
		}

		if (DecodeIndirectADC(&ArgStr[2], &regd, &imm, &flags2))
		{	/* (13) */
			dsize = DispSize(disp, flags, scale);	/* src */
			isize = DispSize(imm, flags2, scale);	/* dst */

			BAsmCode[0] = 0xC0 | (size << 4) | (isize << 2) | dsize;
			BAsmCode[1] = (regs << 4) | regd;
			CodeLen = DispOut(2, dsize, disp, scale);
			CodeLen = DispOut(CodeLen, isize, imm, scale);
			return;
		}
	}

	if (DecodeIndexed(&ArgStr[1], &regi, &regs) &&
		DecodeReg(&ArgStr[2], &regd, eRn))
	{	/* (10) */
		BAsmCode[0] = 0xFE;
		BAsmCode[1] = 0x40 | (size << 4) | regi;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
		return;
	}

	if (DecodeIncDec(&ArgStr[1], &regs, &regi) &&
		DecodeReg(&ArgStr[2], &regd, eRn))
	{
		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x28 | (regi << 2) | size;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);	
}

static void DecodeMOVU(Word Index)
{
	Byte size;
	Byte regs;
	Byte regd;
	Byte scale;
	LongInt disp;
	tSymbolFlags flags;
	Byte disp2;
	Byte dsize;
	Byte regi;

	UNUSED(Index);

	if (!DecodeAttrSize(&size) || size == 0x02) return;
	scale = Size2Scale(size);

	if (DecodeReg(&ArgStr[2], &regd, eRn))
	{
		if (DecodeRelative(&ArgStr[1], &regs, &disp, &flags) &&
			ChkDisp5(size, disp, flags) && regd < 8)
		{	/* (1) */
			disp2 = DispSize5(size, disp);

			BAsmCode[0] = 0xB0 | (size << 3) | ((disp2 >> 2) & 0x07);
			BAsmCode[1] = ((disp2 << 6) & 0x80) | (regs << 4) | ((disp2 << 3) & 0x08) | regd;
			CodeLen = 2;
			return;
		}

		if (DecodeReg(&ArgStr[1], &regs, eRn))
		{	/* (2)-1 */
			BAsmCode[0] = 0x5B | (size << 2);
			BAsmCode[1] = (regs << 4) | regd;
			CodeLen = 2;
			return;
		}

		if (DecodeIndirectADC(&ArgStr[1], &regs, &disp, &flags))
		{	/* (2)-2 */
			dsize = DispSize(disp, flags, scale);

			BAsmCode[0] = 0x58 | (size << 2) | dsize;
			BAsmCode[1] = (regs << 4) | regd;
			CodeLen = DispOut(2, dsize, disp, scale);
			return;
		}

		if (DecodeIndexed(&ArgStr[1], &regi, &regs))
		{	/* (3) */
			BAsmCode[0] = 0xFE;
			BAsmCode[1] = 0xC0 | (size << 4) | regi;
			BAsmCode[2] = (regs << 4) | regd;
			CodeLen = 3;
			return;
		}

		if (DecodeIncDec(&ArgStr[1], &regs, &regi))
		{	/* (4) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x38 | (regi << 2) | size;
			BAsmCode[2] = (regs << 4) | regd;
			CodeLen = 3;
			return;
		}
	}

	WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);	
}

static const struct {
	Byte Opc2;
	Byte Opc3;
	Byte flags;
} OpTabMVFACHI[] = {
	{ 0x1E, 0x00, 0x00 },	/* MVFACHI */
	{ 0x1E, 0x20, 0x00 },	/* MVFACMI */
	{ 0x1E, 0x30, 0x10 },	/* MVFACGU */
	{ 0x1E, 0x10, 0x10 },	/* MVFACLO */
};

static void DecodeMVFACHI(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;
	Byte acc;
	Byte reg;
	
	if ((OpTabMVFACHI[Index].flags & 0x10) && !CheckV2()) return;
	if (!ChkNoAttr()) return;

	if (pCurrCPUProps->InstSet == eRXv1)
	{
		if (!ChkArgCnt(1,1)) return;
		imm = 0x02;
		acc = 0x00;
	}
	else
	{
		if (!ChkArgCnt(3,3)) return;
		if (!DecodeImm(&ArgStr[1], &imm, &flags))
		{
			WrStrErrorPos(ErrNum_OnlyImmAddr, &ArgStr[1]);	
			return;
		}
		switch (imm)
		{
		case 0:
			imm = 0x02;
			break;
		case 1:
			imm = 0x03;
			break;
		case 2:
			imm = 0x00;
			break;
		default:
			if (mSymbolQuestionable(flags)) break;
			WrStrErrorPos(ErrNum_ArgOutOfRange, &ArgStr[1]);
			return;
		}

		if (!DecodeAcc(&ArgStr[2], &acc))
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}
	}
	
	if (!DecodeReg(&ArgStr[ArgCnt], &reg, eRn))
	{
		WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[ArgCnt]);	
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = OpTabMVFACHI[Index].Opc2 | ((imm >> 1) & 0x01);
	BAsmCode[2] = OpTabMVFACHI[Index].Opc3 | (acc << 7) | ((imm << 6) & 0x40) | reg;
	CodeLen = 3;
}

static const char *SPReg[] = {
	"PSW",   "PC",    "USP",   "FPSW",   NULL,    NULL,    NULL,    NULL,
	"BPSW",  "BPC",   "ISP",   "FINTV",  "INTB",  "EXTB",  NULL,    NULL,
};

static void DecodeMVFC(Word Index)
{
	Byte reg;
	size_t i;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (DecodeReg(&ArgStr[2], &reg, eRn))
	{
		for (i = 0; i < as_array_size(SPReg); i++)
		{
			if (i == 13 && pCurrCPUProps->InstSet == eRXv1)
			{
				WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[2]);
			}

			if (SPReg[i] && !as_strcasecmp(SPReg[i], ArgStr[1].str.p_str))
			{
				BAsmCode[0] = 0xFD;
				BAsmCode[1] = 0x6A;
				BAsmCode[2] = (i << 4) | reg;
				CodeLen = 3;
				return;
			}
		}
		
		WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);			
}

static const struct {
	Byte Opc2;
	Byte Opc3;
	Byte flags;
} OpTabMVTACHI[] = {
	{ 0x17, 0x00, 0x00 },	/* MVTACHI */
	{ 0x17, 0x10, 0x00 },	/* MVTACLO */
	{ 0x17, 0x30, 0x10 },	/* MVTACGU */
};

static void DecodeMVTACHI(Word Index)
{
	Byte acc;
	Byte reg;
	
	if ((OpTabMVTACHI[Index].flags & 0x10) && !CheckV2()) return;
	if (!ChkNoAttr()) return;

	if (pCurrCPUProps->InstSet == eRXv1)
	{
		if (!ChkArgCnt(1,1)) return;
		acc = 0x00;
	}
	else
	{
		if (!ChkArgCnt(2,2)) return;

		if (!DecodeAcc(&ArgStr[2], &acc))
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}
	}
	
	if (!DecodeReg(&ArgStr[1], &reg, eRn))
	{
		WrStrErrorPos(ErrNum_AddrModeNotSupported, &ArgStr[1]);
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = OpTabMVTACHI[Index].Opc2;
	BAsmCode[2] = OpTabMVTACHI[Index].Opc3 | (acc << 7) | reg;
	CodeLen = 3;
}

static void DecodeMVTC(Word Index)
{
	Byte reg;
	LongInt imm;
	Byte size;
	tSymbolFlags flags;
	size_t i;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	for (i = 0; i < as_array_size(SPReg); i++)
	{
		if (SPReg[i] && !as_strcasecmp(SPReg[i], ArgStr[2].str.p_str))
		{
			if (i == 1)
			{
				WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[2]);
				return;
			}

			if (i == 13 && pCurrCPUProps->InstSet == eRXv1)
			{
				WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[2]);
			}

			if (DecodeImm(&ArgStr[1], &imm, &flags))
			{
				size = ImmSize32(imm, flags);

				BAsmCode[0] = 0xFD;
				BAsmCode[1] = 0x73 | (size << 2);
				BAsmCode[2] = i;
				CodeLen = ImmOut(3, size, imm);
				return;
			}

			if (DecodeReg(&ArgStr[1], &reg, eRn))
			{
				BAsmCode[0] = 0xFD;
				BAsmCode[1] = 0x68;
				BAsmCode[2] = (reg << 4) | i;
				CodeLen = 3;
				return;
			}
		
			WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
		}
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);			
}

static void DecodeMVTIPL(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;

	UNUSED(Index);

	if (!CheckSup()) return;
	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	if (!pCurrCPUProps->hasMVTIPL)
	{
		WrError(ErrNum_InstructionNotSupported);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (!mFirstPassUnknown(flags) && (imm < 0 || imm > 15))
		{
			WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
			return;
		}

		BAsmCode[0] = 0x75;
		BAsmCode[1] = 0x70;
		BAsmCode[2] = imm;
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);			
}

static const Byte OpTabPOPC[] = {
	0xE0,	/* POPC */
	0xC0,	/* PUSHC */
};

static void DecodePOPC(Word Index)
{
	size_t i;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	for (i = 0; i < as_array_size(SPReg); i++)
	{
		if (SPReg[i] && !as_strcasecmp(SPReg[i], ArgStr[1].str.p_str))
		{
			if (Index == 0 && i == 1)
			{
				WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
				return;
			}

			if (i == 13 && pCurrCPUProps->InstSet == eRXv1)
			{
				WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[2]);
			}

			BAsmCode[0] = 0x7E;
			BAsmCode[1] = OpTabPOPC[Index] | i;
			CodeLen = 2;
			return;
		}
	}

	WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[1]);
}

static const Byte OpTabPOPM[] = {
	0x6F,	/* POPM */
	0x6E,	/* PUSHM */
};

static void DecodePOPM(Word Index)
{
	Byte range;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	if (DecodeRegRange(&ArgStr[1], &range))
	{
		BAsmCode[0] = OpTabPOPM[Index];
		BAsmCode[1] = range;
		CodeLen = 2;
		return;
	}

	WrStrErrorPos(ErrNum_InvRegList, &ArgStr[1]);
}

static void DecodePUSH(Word Index)
{
	Byte size;
	Byte reg;
	LongInt disp;
	tSymbolFlags flags;
	Byte dsize;
	Byte scale;

	UNUSED(Index);

	if (!DecodeAttrSize(&size)) return;
	if (!ChkArgCnt(1,1)) return;

	scale = Size2Scale(size);

	if (DecodeReg(&ArgStr[1], &reg, eRn))
	{
		BAsmCode[0] = 0x7E;
		BAsmCode[1] = 0x80 | (size << 4) | reg;
		CodeLen = 2;
		return;
	}

	if (DecodeIndirectADC(&ArgStr[1], &reg, &disp, &flags))
	{
		dsize = DispSize(disp, flags, scale);

		BAsmCode[0] = 0xF4 | dsize;
		BAsmCode[1] = 0x08 | (reg << 4) | size;
		CodeLen = DispOut(2, dsize, disp, scale);
		return;
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

static const struct {
	Byte Opc2;
	Byte Opc3;
	Byte flags;
} OpTabRACW[] = {
	{ 0x18, 0x00, 0x00 },	/* RACW */
	{ 0x19, 0x00, 0x10 },	/* RACL */
	{ 0x19, 0x40, 0x10 },	/* RDACL */
	{ 0x18, 0x40, 0x10 },	/* RDACW */
};

static void DecodeRACW(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;
	Byte acc = 0;

	if ((OpTabRACW[Index].flags & 0x10) && !CheckV2()) return;
	if (!ChkNoAttr()) return;

	if (pCurrCPUProps->InstSet == eRXv1)
	{
		if (!ChkArgCnt(1,1)) return;
	}
	else
	{
		if (!ChkArgCnt(2,2)) return;

		if (!DecodeAcc(&ArgStr[2], &acc))
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
			return;
		}
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (mFirstPassUnknown(flags) || imm == 1 || imm == 2)
		{
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = OpTabRACW[Index].Opc2;
			BAsmCode[2] = OpTabRACW[Index].Opc3 | (acc << 7) | ((imm - 1) << 4);
			CodeLen = 3;
			return;
		}
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

static void DecodeRMPA(Word Index)
{
	Byte size;

	if (!DecodeAttrSize(&size)) return;
	if (!ChkArgCnt(0,0)) return;

	BAsmCode[0] = 0x7F;
	BAsmCode[1] = Index | size;
	CodeLen = 2;
}

static const struct {
	Byte Opc1;
	Byte Opc2;
} OpTabROTL[] = {
	{ 0x6E, 0x66 },	/* ROTL */
	{ 0x6C, 0x64 },	/* ROTR */
};

static void DecodeROTL(Word Index)
{
	Byte regs;
	Byte regd;
	LongInt imm;
	tSymbolFlags flags;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (!mFirstPassUnknown(flags) && (imm < 0 || imm > 31))
		{
			WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
			return;
		}

		BAsmCode[0] = 0xFD;
		BAsmCode[1] = OpTabROTL[Index].Opc1 | ((imm >> 4) & 0x01);
		BAsmCode[2] = ((imm << 4) & 0xF0) | regd;
		CodeLen = 3;
		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		BAsmCode[0] = 0xFD;
		BAsmCode[1] = OpTabROTL[Index].Opc2;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
}

static void DecodeRTE(Word Index)
{
	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(0,0)) return;

	switch (Index)
	{
	case 0x7F95:	/* RTE */
	case 0x7F94:	/* RTFI */
	case 0x7F96:	/* WAIT */
		if (!CheckSup()) return;
	}

	BAsmCode[0] = Index >> 8;
	BAsmCode[1] = Index;
	CodeLen = 2;
}

static void DecodeRTSD(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;
	Byte range;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,2)) return;

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (mSymbolQuestionable(flags) && (imm < 0 || imm > 255))
		{
			WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
			return;
		}

		if (ArgCnt == 2 && DecodeRegRange(&ArgStr[2], &range))
		{
			BAsmCode[0] = 0x3F;
			BAsmCode[1] = range;
			BAsmCode[2] = imm;
			CodeLen = 3;
			return;
		}

		if (ArgCnt > 1)
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;
		}

		BAsmCode[0] = 0x67;
		BAsmCode[1] = imm;
		CodeLen = 2;
		return;
	}

	WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
}

static void DecodeSCCnd(Word Index)
{
	Byte size;
	Byte reg;
	LongInt disp;
	tSymbolFlags flags;
	Byte dsize;
	Byte scale;

	if (!DecodeAttrSize(&size)) return;
	if (!ChkArgCnt(1,1)) return;

	scale = Size2Scale(size);

	if (DecodeReg(&ArgStr[1], &reg, eRn))
	{
		if (size != 0x02)
		{
			WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
			return;
		}

		BAsmCode[0] = 0xFC;
		BAsmCode[1] = 0xD3 | (size << 2);
		BAsmCode[2] = (reg << 4) | Index;
		CodeLen = 3;
		return;
	}

	if (DecodeIndirectADC(&ArgStr[1], &reg, &disp, &flags))
	{
		dsize = DispSize(disp, flags, scale);

		BAsmCode[0] = 0xFC;
		BAsmCode[1] = 0xD0 | (size << 2) | dsize;
		BAsmCode[2] = (reg << 4) | Index;
		CodeLen = DispOut(3, dsize, disp, scale);
		return;
	}

	WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
}

static const struct {
	Byte Opc1;
	Byte Opc2;
	Byte Opc3;
} OpTabSHAR[] = {
	{ 0x6A, 0x61, 0xA0 },	/* SHAR */
	{ 0x6C, 0x62, 0xC0 },	/* SHLL */
	{ 0x68, 0x60, 0x80 },	/* SHLR */
};

static void DecodeSHAR(Word Index)
{
	Byte regs;
	Byte regd;
	LongInt imm;
	tSymbolFlags flags;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,3)) return;

	if (!DecodeReg(&ArgStr[ArgCnt], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (!mFirstPassUnknown(flags) && (imm < 0 || imm > 31))
		{
			WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
			return;
		}

		if (ArgCnt == 3)
		{
			if (DecodeReg(&ArgStr[2], &regs, eRn))
			{	/* (3) */
				BAsmCode[0] = 0xFD;
				BAsmCode[1] = OpTabSHAR[Index].Opc3 | imm;
				BAsmCode[2] = (regs << 4) | regd;
				CodeLen = 3;
				return;
			}

			WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
			return;
		}

		/* (1) */
		BAsmCode[0] = OpTabSHAR[Index].Opc1 | (imm >> 4);
		BAsmCode[1] = ((imm << 4) & 0xF0) | regd;
		CodeLen = 2;
		return;
	}

	if (ArgCnt > 2)
	{
		WrStrErrorPos(ErrNum_TooManyArgs, &ArgStr[2]);
		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{	/* (2) */
		BAsmCode[0] = 0xFD;
		BAsmCode[1] = OpTabSHAR[Index].Opc2;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
}

static const struct {
	Byte OpcIR;
	Byte OpcRR;
} OpTabSTNZ[] = {
	{ 0xF0, 0x4F },	/* STNZ */
	{ 0xE0, 0x4B },	/* STZ */
};

static void DecodeSTNZ(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;
	Byte regd;
	Byte regs;
	Byte size;
	
	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		size = ImmSize32(imm, flags);

		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x70 | (size << 2);
		BAsmCode[2] = OpTabSTNZ[Index].OpcIR | regd;
		CodeLen = ImmOut(3, size, imm);
		return;
	}

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		if (!CheckV2()) return;

		BAsmCode[0] = 0xFC;
		BAsmCode[1] = OpTabSTNZ[Index].OpcRR;
		BAsmCode[2] = (regs << 4) | regd;
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
}

static const struct {
	Byte Opc1;
	Byte Opc2;
	Byte Opc3;
	Byte flags;
} OpTabTST[] = {
	{ 0xC0, 0x30, 0x0C, 0x01 },	/* TST */
	{ 0,    0x40, 0x10, 0x00 },	/* XCHG */
	{ 0xD0, 0x34, 0x0D, 0x03 },	/* XOR */
};

static void DecodeTST(Word Index)
{
	Byte reg1;
	Byte reg2;
	LongInt imm;
	LongInt disp;
	tSymbolFlags flags;
	Byte size;
	Byte memex;
	Byte scale;

	if (!ChkNoAttr()) return;
	if ((OpTabTST[Index].flags & 0x02) && ArgCnt == 3)
	{
		Byte reg3;

		if (!CheckV3()) return;

		if (!DecodeReg(&ArgStr[1], &reg1, eRn))
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
			return;		
		}

		if (!DecodeReg(&ArgStr[2], &reg2, eRn))
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
			return;		
		}

		if (!DecodeReg(&ArgStr[3], &reg3, eRn))
		{
			WrStrErrorPos(ErrNum_InvArg, &ArgStr[3]);
			return;		
		}

		BAsmCode[0] = 0xFF;
		BAsmCode[1] = 0x60 | reg3;
		BAsmCode[2] = (reg1 << 4) | reg2;
		CodeLen = 3;
		return;
	}
		
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeReg(&ArgStr[2], &reg2, eRn))
	{
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
		return;		
	}

	if ((OpTabTST[Index].flags & 0x01) &&
		DecodeImm(&ArgStr[1], &imm, &flags))
	{
		size = ImmSize32(imm, flags);

		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x70 | (size << 2);
		BAsmCode[2] = OpTabTST[Index].Opc1 | reg2;
		CodeLen = ImmOut(3, size, imm);
		return;
	}

	if (DecodeReg(&ArgStr[1], &reg1, eRn))
	{
		BAsmCode[0] = 0xFC;
		BAsmCode[1] = OpTabTST[Index].Opc2 | 0x03;
		BAsmCode[2] = (reg1 << 4) | reg2;
		CodeLen = 3;
		return;
	}

	if (DecodeIndirectADD(&ArgStr[1], &reg1, &disp, &flags, &memex, &scale))
	{
		if (memex == 0x80)
		{
			size = DispSize(disp, flags, 1);
			
			BAsmCode[0] = 0xFC;
			BAsmCode[1] = OpTabTST[Index].Opc2 | size;
			BAsmCode[2] = (reg1 << 4) | reg2;
			CodeLen = DispOut(3, size, disp, 1);
			return;
		}

		size = DispSize(disp, flags, scale);

		BAsmCode[0] = 0x06;
		BAsmCode[1] = 0x20 | (memex << 6) | size;
		BAsmCode[2] = OpTabTST[Index].Opc3;
		BAsmCode[3] = (reg1 << 4) | reg2;
		CodeLen = DispOut(4, size, disp, scale);
		return;
	}
			
}

static void DecodeMOVCO(Word Index)
{
	Byte regs;
	Byte regd;

	UNUSED(Index);

	if (!CheckV2()) return;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeReg(&ArgStr[1], &regs, eRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
		return;
	}

	if (!DecodeIndirect(&ArgStr[2], &regd))
	{
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = 0x27;
	BAsmCode[2] = (regd << 4) | regs;
	CodeLen = 3;
}

static void DecodeMOVLI(Word Index)
{
	Byte regs;
	Byte regd;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;

	if (!DecodeIndirect(&ArgStr[1], &regs))
	{
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
		return;
	}

	if (!DecodeReg(&ArgStr[2], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[2]);
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = 0x2F;
	BAsmCode[2] = (regs << 4) | regd;
	CodeLen = 3;
}

static const Byte OpTabBFMOV[] = {
	0x5E,	/* BFMOV */
	0x5A,	/* BFMOVZ */
};

static void DecodeBFMOV(Word Index)
{
	LongInt slsb;
	LongInt dlsb;
	LongInt width;
	tSymbolFlags flags;
	Byte regs;
	Byte regd;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(5,5)) return;
	if (!CheckV3()) return;

	if (!DecodeImm(&ArgStr[1], &slsb, &flags))
	{
		WrStrErrorPos(ErrNum_ExpectInt, &ArgStr[1]);
		return;
	}
	if (!mFirstPassUnknownOrQuestionable(flags) && !ChkRange(slsb, 0, 31)) return;

	if (!DecodeImm(&ArgStr[2], &dlsb, &flags))
	{
		WrStrErrorPos(ErrNum_ExpectInt, &ArgStr[2]);
		return;
	}
	if (!mFirstPassUnknownOrQuestionable(flags) && !ChkRange(dlsb, 0, 31)) return;

	if (!DecodeImm(&ArgStr[3], &width, &flags))
	{
		WrStrErrorPos(ErrNum_ExpectInt, &ArgStr[3]);
		return;
	}
	if (!mFirstPassUnknownOrQuestionable(flags) && !ChkRange(width, 1, 31)) return;

	if (!DecodeReg(&ArgStr[4], &regs, eRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[4]);
		return;
	}

	if (!DecodeReg(&ArgStr[5], &regd, eRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[5]);
		return;
	}

	if (slsb + width > 32 || dlsb + width > 32)
	{
		WrError(ErrNum_OverRange);
		return;
	}

	BAsmCode[0] = 0xFC;
	BAsmCode[1] = OpTabBFMOV[Index];
	BAsmCode[2] = (regs << 4) | regd;
	BAsmCode[3] = ((dlsb - slsb) & 0x1f) | ((dlsb << 5) & 0xE0);
	BAsmCode[4] = ((dlsb >> 3) & 0x03) | (((dlsb + width) << 2) & 0x7C);
	CodeLen = 5;
}

static const struct {
	Byte OpcI;
	Byte OpcR;
} OpTabRSTR[] = {
	{ 0xF0, 0xD0 },	/* RSTR */
	{ 0xE0, 0xC0 },	/* SAVE */
};

static void DecodeRSTR(Word Index)
{
	LongInt imm;
	tSymbolFlags flags;
	Byte reg;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;
	if (!CheckV3()) return;

	if (!pCurrCPUProps->RegBank)
	{
		WrError(ErrNum_InstructionNotSupported);
		return;
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (!ChkRange(imm, 0, pCurrCPUProps->RegBank - 1)) return;

		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x76;
		BAsmCode[2] = OpTabRSTR[Index].OpcI;
		BAsmCode[3] = imm;
		CodeLen = 4;
		return;
	}

	if (DecodeReg(&ArgStr[1], &reg, eRn))
	{
		BAsmCode[0] = 0xFD;
		BAsmCode[1] = 0x76;
		BAsmCode[2] = OpTabRSTR[Index].OpcR | reg;
		BAsmCode[3] = 0x00;
		CodeLen = 4;
		return;
	}

	WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
}

static const struct {
	Byte Opc1;
	Byte Opc2;
} OpTabDABS[] = {
	{ 0x0C, 0x01 },	/* DABS */
	{ 0x0C, 0x02 },	/* DNEG */
	{ 0x0D, 0x0D },	/* DROUND */
	{ 0x0D, 0x00 },	/* DSQRT */
	{ 0x0D, 0x0C },	/* DTOF */
	{ 0x0D, 0x08 },	/* DTOI */
	{ 0x0D, 0x09 },	/* DTOU */
};

static void DecodeDABS(Word Index)
{
	Byte regs;
	Byte regd;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	if (!DecodeReg(&ArgStr[1], &regs, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
		return;
	}

	if (!DecodeReg(&ArgStr[2], &regd, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[2]);
		return;
	}

	BAsmCode[0] = 0x76;
	BAsmCode[1] = 0x90;
	BAsmCode[2] = OpTabDABS[Index].Opc1 | (regs << 4);
	BAsmCode[3] = OpTabDABS[Index].Opc2 | (regd << 4);
	CodeLen = 4;
}

static const struct {
	Byte Opc;
} OpTabDADD[] = {
	{ 0x00 },	/* DADD */
	{ 0x05 },	/* DDIV */
	{ 0x02 },	/* DMUL */
	{ 0x01 },	/* DSUB */
};

static void DecodeDADD(Word Index)
{
	Byte regs1;
	Byte regs2;
	Byte regd;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(3,3)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	if (!DecodeReg(&ArgStr[1], &regs1, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
		return;
	}

	if (!DecodeReg(&ArgStr[2], &regs2, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[2]);
		return;
	}

	if (!DecodeReg(&ArgStr[3], &regd, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[3]);
		return;
	}

	BAsmCode[0] = 0x76;
	BAsmCode[1] = 0x90;
	BAsmCode[2] = OpTabDADD[Index].Opc | (regs2 << 4);
	BAsmCode[3] = (regd << 4) | regs1;
	CodeLen = 4;
}

static void DecodeDCMPcm(Word Index)
{
	Byte regs1;
	Byte regs2;

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	if (!DecodeReg(&ArgStr[1], &regs1, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
		return;
	}

	if (!DecodeReg(&ArgStr[2], &regs2, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[2]);
		return;
	}

	BAsmCode[0] = 0x76;
	BAsmCode[1] = 0x90;
	BAsmCode[2] = 0x08 | (regs2 << 4);
	BAsmCode[3] = ((Index << 4) & 0xF0) | regs1;
	CodeLen = 4;
}

static void DecodeDMOV(Word Index)
{
	Byte size;
	Byte regs;
	Byte regd;
	LongInt disp;
	LongInt imm;
	tSymbolFlags flags;
	Byte dsize;

	UNUSED(Index);

	if (!ChkArgCnt(2,2)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	if (!DecodeAttrDouble(&size)) return;

	if (DecodeReg(&ArgStr[1], &regs, eRn))
	{
		if (DecodeReg(&ArgStr[2], &regd, eDRHn))
		{	/* (1) / (2) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x77;
			BAsmCode[2] = 0x80 | regs;
			BAsmCode[3] = 0x02 | (regd << 4) | size;
			CodeLen = 4;
			return;
		}

		if (DecodeReg(&ArgStr[2], &regd, eDRLn) && !size)
		{	/* (3) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x77;
			BAsmCode[2] = 0x80 | regs;
			BAsmCode[3] = 0x00 | (regd << 4);
			CodeLen = 4;
			return;
		}
	}

	if (DecodeReg(&ArgStr[2], &regd, eRn))
	{
		if (DecodeReg(&ArgStr[1], &regs, eDRHn) && !size)
		{	/* (4) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x75;
			BAsmCode[2] = 0x80 | regd;
			BAsmCode[3] = 0x02 | (regs << 4);
			CodeLen = 4;
			return;
		}

		if (DecodeReg(&ArgStr[1], &regs, eDRLn) && !size)
		{	/* (5) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x75;
			BAsmCode[2] = 0x80 | regd;
			BAsmCode[3] = 0x00 | (regs << 4);
			CodeLen = 4;
			return;
		}
	}

	if (DecodeReg(&ArgStr[1], &regs, eDRn) && size)
	{
		if (DecodeReg(&ArgStr[2], &regd, eDRn))
		{	/* (6) */
			BAsmCode[0] = 0xFD;
			BAsmCode[1] = 0x90;
			BAsmCode[2] = 0x0C | (regs << 4);
			BAsmCode[3] = 0x00 | (regd << 4);
			CodeLen = 4;
			return;
		}

		if (DecodeIndirectADC(&ArgStr[2], &regd, &disp, &flags) && size)
		{	/* (7) */
			dsize = DispSize(disp, flags, 8);

			BAsmCode[0] = 0xFC;
			BAsmCode[1] = 0x78 | dsize;
			BAsmCode[2] = 0x08 | (regd << 4);
			CodeLen = DispOut(3, dsize, disp, 8);
			BAsmCode[CodeLen++] = 0x00 | (regs << 4);
			return;
		}
	}

	if (DecodeIndirectADC(&ArgStr[1], &regs, &disp, &flags) && size)
	{
		if (DecodeReg(&ArgStr[2], &regd, eDRn))
		{	/* (8) */
			dsize = DispSize(disp, flags, 8);

			BAsmCode[0] = 0xFC;
			BAsmCode[1] = 0xC8 | dsize;
			BAsmCode[2] = 0x08 | (regs << 4);
			CodeLen = DispOut(3, dsize, disp, 8);
			BAsmCode[CodeLen++] = 0x00 | (regd << 4);
			return;
		}
	}

	if (DecodeImm(&ArgStr[1], &imm, &flags))
	{
		if (DecodeReg(&ArgStr[2], &regd, eDRHn))
		{	/* (9) / (10) */
			BAsmCode[0] = 0xF9;
			BAsmCode[1] = 0x03;
			BAsmCode[2] = 0x02 | (regd << 4) | size;
			CodeLen = ImmOut(3, 0, imm);
			return;
		}

		if (DecodeReg(&ArgStr[2], &regd, eDRLn) && !size)
		{	/* (11) */
			BAsmCode[0] = 0xF9;
			BAsmCode[1] = 0x03;
			BAsmCode[2] = 0x00 | (regd << 4);
			CodeLen = ImmOut(3, 0, imm);
			return;
		}
	}

	WrError(ErrNum_InvArg);
}

static const Byte OpTabDPOPM[] = {
	0xA8,	/* DPOPM */
	0xA0	/* DPUSHM */
};

static void DecodeDPOPM(Word Index)
{
	const int len = strlen(ArgStr[1].str.p_str);
	Byte size;
	Byte reg1;
	Byte reg2;
	int i;

	if (!DecodeAttrDouble(&size)) return;
	if (!ChkArgCnt(1,1)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	for (i = 0; i < len; i++)
	{
		if (ArgStr[1].str.p_str[i] == '-') break;
	}
	if (i >= len)
	{
		WrStrErrorPos(ErrNum_InvRegList, &ArgStr[1]);
		return;
	}

	StrCompCopySub(&Temp1, &ArgStr[1], 0, i);
	if (!DecodeReg(&Temp1, &reg1, size ? eDRn : eDCRn))
	{
		WrStrErrorPos(ErrNum_InvRegList, &ArgStr[1]);
		return;
	}
		
	StrCompCopySub(&Temp1, &ArgStr[1], i + 1, len - i - 1);
	if (!DecodeReg(&Temp1, &reg2, size ? eDRn : eDCRn))
	{
		WrStrErrorPos(ErrNum_InvRegList, &ArgStr[1]);
		return;
	}

	if (reg2 >= reg1)
	{
		BAsmCode[0] = 0x75;
		BAsmCode[1] = OpTabDPOPM[Index] | (size << 4);
		BAsmCode[2] = (reg1 << 4) | (reg2 - reg1);
		CodeLen = 3;
		return;
	}

	WrStrErrorPos(ErrNum_InvRegList, &ArgStr[1]);
}

static const struct {
	Byte Opc1;
	Byte Opc2;
} OpTabFTOD[] = {
	{ 0x80, 0x0A },	/* FTOD */
	{ 0x80, 0x09 },	/* ITOD */
	{ 0x80, 0x0D },	/* UTOD */
};

static void DecodeFTOD(Word Index)
{
	Byte regs;
	Byte regd;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	if (!DecodeReg(&ArgStr[1], &regs, eRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[1]);
		return;
	}

	if (!DecodeReg(&ArgStr[2], &regd, eDRn))
	{
		WrStrErrorPos(ErrNum_ExpectReg, &ArgStr[2]);
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = 0x77;
	BAsmCode[2] = OpTabFTOD[Index].Opc1 | regs;
	BAsmCode[3] = OpTabFTOD[Index].Opc2 | (regd << 4);
	CodeLen = 4;
}

static void DecodeMVFDC(Word Index)
{
	Byte regc;
	Byte reg;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(2,2)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	if (!DecodeReg(&ArgStr[Index + 1], &regc, eDCRn))
	{
		WrStrErrorPos(ErrNum_InvCtrlReg, &ArgStr[Index + 1]);
		return;
	}
	
	if (!DecodeReg(&ArgStr[2 - Index], &reg, eRn))
	{
		WrStrErrorPos(ErrNum_InvReg, &ArgStr[2 - Index]);
		return;
	}

	BAsmCode[0] = 0xFD;
	BAsmCode[1] = 0x75 | (Index << 1);
	BAsmCode[2] = 0x80 | reg;
	BAsmCode[3] = 0x04 | (regc << 4);
	CodeLen = 4;
}

static void DecodeMVFDR(Word Index)
{
	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(0,0)) return;
	if (!CheckV3()) return;
	if (!CheckDouble()) return;

	BAsmCode[0] = 0x75;
	BAsmCode[1] = 0x90;
	BAsmCode[2] = 0x1B;
	CodeLen = 3;
}

#ifdef COMPAT

#define BigEndianSymName "BIGENDIAN" /* T.B.D. */
static void DecodeENDIAN(Word Index)
{
	const char *str;

	UNUSED(Index);

	if (!ChkNoAttr()) return;
	if (!ChkArgCnt(1,1)) return;

	str = ArgStr[1].str.p_str;
	if (!as_strcasecmp(str, "BIG"))
		SetFlag(&TargetBigEndian, BigEndianSymName, True);
	else if (!as_strcasecmp(str, "LITTLE"))
		SetFlag(&TargetBigEndian, BigEndianSymName, False);
	else
		WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
}

static void DecodeWORD(Word flags)
{
	DecodeIntelDW(flags | (TargetBigEndian ? eIntPseudoFlag_BigEndian : 0));
}

static void DecodeLWORD(Word flags)
{
	DecodeIntelDD(flags | (TargetBigEndian ? eIntPseudoFlag_BigEndian : 0));
}

static void DecodeFLOAT(Word flags)
{
	DecodeIntelDD(flags | (TargetBigEndian ? eIntPseudoFlag_BigEndian : 0));
}

static void DecodeDOUBLE(Word flags)
{
	DecodeIntelDQ(flags | (TargetBigEndian ? eIntPseudoFlag_BigEndian : 0));
}

#endif /* COMPAT */

/*---------------------------------------------------------------------------*/

static void AddABS(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeABS);
}

static void AddADC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeADC);
}

static void AddADD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeADD);
}

static void AddBCLR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBCLR);
}

static void AddBCnd(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBCnd);
}

static void AddBMCnd(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBMCnd);
}

static void AddBRK(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBRK);
}

static void AddBSR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBSR);
}

static void AddCLRPSW(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeCLRPSW);
}

static void AddDIV(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDIV);
}

static void AddFADD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFADD);
}

static void AddINT(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeINT);
}

static void AddITOF(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeITOF);
}

static void AddJMP(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeJMP);
}

static void AddMACHI(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMACHI);
}

static void AddMOV(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMOV);
}

static void AddMOVU(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMOVU);
}

static void AddMVFACHI(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVFACHI);
}

static void AddMVFC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVFC);
}

static void AddMVTACHI(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVTACHI);
}

static void AddMVTC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVTC);
}

static void AddMVTIPL(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVTIPL);
}

static void AddPOPC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodePOPC);
}

static void AddPOPM(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodePOPM);
}

static void AddPUSH(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodePUSH);
}

static void AddRACW(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRACW);
}

static void AddRMPA(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRMPA);
}

static void AddROTL(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeROTL);
}

static void AddRTE(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRTE);
}

static void AddRTSD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRTSD);
}

static void AddSCCnd(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeSCCnd);
}

static void AddSHAR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeSHAR);
}

static void AddSTNZ(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeSTNZ);
}

static void AddTST(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeTST);
}

static void AddMOVCO(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMOVCO);
}

static void AddMOVLI(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMOVLI);
}

static void AddBFMOV(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeBFMOV);
}

static void AddRSTR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeRSTR);
}

static void AddDABS(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDABS);
}

static void AddDADD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDADD);
}

static void AddDCMPcm(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDCMPcm);
}

static void AddDMOV(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDMOV);
}

static void AddDPOPM(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeDPOPM);
}

static void AddFTOD(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeFTOD);
}

static void AddMVFDC(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVFDC);
}

static void AddMVFDR(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeMVFDR);
}

#ifdef COMPAT

static void AddBLK(const char *NName, Word NCode)
{
	AddInstTable(InstTable, NName, NCode, DecodeIntelDS);
}

#endif /* COMPAT */

static Boolean TrueFnc(void)
{
  return True;
}

static void InitFields(void)
{
	InstTable = CreateInstTable(201);

	/* RXv1 */
	AddABS("ABS", 0x200F);
	AddADC("ADC", 0x0802);
	AddADD("ADD", 0);
	AddADD("AND", 1);
	AddBCLR("BCLR", 0);
	AddBCnd("BEQ",  0);
	AddBCnd("BZ",   0);
	AddBCnd("BNE",  1);
	AddBCnd("BNZ",  1);
	AddBCnd("BGEU", 2);
	AddBCnd("BC",   2);
	AddBCnd("BLTU", 3);
	AddBCnd("BNC",  3);
	AddBCnd("BGTU", 4);
	AddBCnd("BLEU", 5);
	AddBCnd("BPZ",  6);
	AddBCnd("BN",   7);
	AddBCnd("BGE",  8);
	AddBCnd("BLT",  9);
	AddBCnd("BGT",  10);
	AddBCnd("BLE",  11);
	AddBCnd("BO",   12);
	AddBCnd("BNO",  13);
	AddBMCnd("BMEQ",  0);
	AddBMCnd("BMZ", 0);
	AddBMCnd("BMNE", 1);
	AddBMCnd("BMNZ", 1);
	AddBMCnd("BMGEU", 2);
	AddBMCnd("BMC", 2);
	AddBMCnd("BMLTU", 3);
	AddBMCnd("BMNC", 3);
	AddBMCnd("BMGTU", 4);
	AddBMCnd("BMLEU", 5);
	AddBMCnd("BMPZ", 6);
	AddBMCnd("BMN", 7);
	AddBMCnd("BMGE", 8);
	AddBMCnd("BMLT", 9);
	AddBMCnd("BMGT", 10);
	AddBMCnd("BMLE", 11);
	AddBMCnd("BMO", 12);
	AddBMCnd("BMNO", 13);
	AddBCLR("BNOT", 1);
	AddBCnd("BRA", 14);
	AddBRK("BRK", 0x0000);
	AddBCLR("BSET", 2);
	AddBSR("BSR", 0);
	AddBCLR("BTST", 3);
	AddCLRPSW("CLRPSW", 0xB0);
	AddADD("CMP", 2);
	AddDIV("DIV", 0);
	AddDIV("DIVU", 1);
	AddDIV("EMUL", 2);
	AddDIV("EMULU", 3);
	AddFADD("FADD", 0);
	AddFADD("FCMP", 1);
	AddFADD("FDIV", 2);
	AddFADD("FMUL", 3);
	AddFADD("FSUB", 4);
	AddFADD("FTOI", 5);
	AddINT("INT", 0);
	AddITOF("ITOF", 0);
	AddJMP("JMP", 0);
	AddJMP("JSR", 1);
	AddMACHI("MACHI", 0);
	AddMACHI("MACLO", 1);
	AddDIV("MAX", 4);
	AddDIV("MIN", 5);
	AddMOV("MOV", 0);
	AddMOVU("MOVU", 0);
	AddADD("MUL", 3);
	AddMACHI("MULHI", 2);
	AddMACHI("MULLO", 3);
	AddMVFACHI("MVFACHI", 0);
	AddMVFACHI("MVFACMI", 1);
	AddMVFC("MVFC", 0);
	AddMVTACHI("MVTACHI", 0);
	AddMVTACHI("MVTACLO", 1);
	AddMVTC("MVTC", 0);
	AddMVTIPL("MVTIPL", 0);
	AddABS("NEG", 0x1007);
	AddBRK("NOP", 0x0003);
	AddABS("NOT", 0x003B);
	AddADD("OR", 4);
	AddJMP("POP", 2);
	AddPOPC("POPC", 0);
	AddPOPM("POPM", 0);
	AddPUSH("PUSH", 0);
	AddPOPC("PUSHC", 1);
	AddPOPM("PUSHM", 1);
	AddRACW("RACW", 0);
	AddMACHI("REVL", 4);
	AddMACHI("REVW", 5);
	AddRMPA("RMPA", 0x8C);
	AddJMP("ROLC", 3);
	AddJMP("RORC", 4);
	AddROTL("ROTL", 0);
	AddROTL("ROTR", 1);
	AddFADD("ROUND", 6);
	AddRTE("RTE", 0x7F95);
	AddRTE("RTFI", 0x7F94);
	AddBRK("RTS", 0x0002);
	AddRTSD("RTSD", 0);
	AddJMP("SAT", 5);
	AddRTE("SATR", 0x7F93);
	AddADC("SBB", 0x0000);
	AddSCCnd("SCEQ", 0);
	AddSCCnd("SCZ", 0);
	AddSCCnd("SCNE", 1);
	AddSCCnd("SCNZ", 1);
	AddSCCnd("SCGEU", 2);
	AddSCCnd("SCC", 2);
	AddSCCnd("SCLTU", 3);
	AddSCCnd("SCNC", 3);
	AddSCCnd("SCGTU", 4);
	AddSCCnd("SCLEU", 5);
	AddSCCnd("SCPZ", 6);
	AddSCCnd("SCN", 7);
	AddSCCnd("SCGE", 8);
	AddSCCnd("SCLT", 9);
	AddSCCnd("SCGT", 10);
	AddSCCnd("SCLE", 11);
	AddSCCnd("SCO", 12);
	AddSCCnd("SCNO", 13);
	AddRTE("SCMPU", 0x7F83);
	AddCLRPSW("SETPSW", 0xA0);
	AddSHAR("SHAR", 0);
	AddSHAR("SHLL", 1);
	AddSHAR("SHLR", 2);
	AddRTE("SMOVB", 0x7F8B);
	AddRTE("SMOVF", 0x7F8F);
	AddRTE("SMOVU", 0x7F87);
	AddRMPA("SSTR", 0x88);
	AddSTNZ("STNZ", 0);
	AddSTNZ("STZ", 1);
	AddADD("SUB", 5);
	AddRMPA("SUNTIL", 0x80);
	AddRMPA("SWHILE", 0x84);
	AddTST("TST", 0);
	AddRTE("WAIT", 0x7F96);
	AddTST("XCHG", 1);
	AddTST("XOR", 2);

	/* RXv2 */
	AddFADD("FSQRT", 7);
	AddFADD("FTOU", 8);
	AddITOF("UTOF", 1);
	AddMOVCO("MOVCO", 0);
	AddMOVLI("MOVLI", 0);
	AddMACHI("EMACA", 6);
	AddMACHI("EMSBA", 7);
	AddMACHI("EMULA", 8);
	AddMACHI("MACLH", 9);
	AddMACHI("MSBHI", 10);
	AddMACHI("MSBLH", 11);
	AddMACHI("MSBLO", 12);
	AddMACHI("MULLH", 13);
	AddMVFACHI("MVFACGU", 2);
	AddMVFACHI("MVFACLO", 3);
	AddMVTACHI("MVTACGU", 2);
	AddRACW("RACL", 1);
	AddRACW("RDACL", 2);
	AddRACW("RDACW", 3);

	/* RXv3 */
	AddBFMOV("BFMOV", 0);
	AddBFMOV("BFMOVZ", 1);
	AddRSTR("RSTR", 0);
	AddRSTR("SAVE", 1); SaveIsOccupiedFnc = TrueFnc;
	AddDABS("DABS", 0);
	AddDADD("DADD", 0);
	AddDCMPcm("DCMPUN", 0x01);
	AddDCMPcm("DCMPEQ", 0x02);
	AddDCMPcm("DCMPLT", 0x04);
	AddDCMPcm("DCMPLE", 0x06);
	AddDADD("DDIV", 1);
	AddDMOV("DMOV", 0);
	AddDADD("DMUL", 2);
	AddDABS("DNEG", 1);
	AddDPOPM("DPOPM", 0);
	AddDPOPM("DPUSHM", 1);
	AddDABS("DROUND", 2);
	AddDABS("DSQRT", 3);
	AddDADD("DSUB", 3);
	AddDABS("DTOF", 4);
	AddDABS("DTOI", 5);
	AddDABS("DTOU", 6);
	AddFTOD("FTOD", 0);
	AddFTOD("ITOD", 1);
	AddMVFDC("MVFDC", 0);
	AddMVFDR("MVFDR", 0);
	AddMVFDC("MVTDC", 1);
	AddFTOD("UTOD", 2);

	/* Pseudo Instructions */
#ifdef COMPAT
	AddInstTable(InstTable, "ENDIAN", 0, DecodeENDIAN);
	AddBLK("BLKB", 1);
	AddBLK("BLKW", 2);
	AddBLK("BLKL", 4);
	AddBLK("BLKD", 8);
	AddInstTable(InstTable, "BYTE", eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDB);
	AddInstTable(InstTable, "WORD", eIntPseudoFlag_AllowInt, DecodeWORD);
	AddInstTable(InstTable, "LWORD", eIntPseudoFlag_AllowInt, DecodeLWORD);
	AddInstTable(InstTable, "FLOAT", eIntPseudoFlag_AllowFloat, DecodeFLOAT);
	AddInstTable(InstTable, "DOUBLE", eIntPseudoFlag_AllowFloat, DecodeDOUBLE);
#endif

	StrCompAlloc(&Temp1, STRINGSIZE);
	StrCompAlloc(&Temp2, STRINGSIZE);
}

static void DeinitFields(void)
{
	StrCompFree(&Temp2);
	StrCompFree(&Temp1);

	DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_RX(void)
{
	CodeLen = 0;
	DontPrint = False;

	if (Memo("")) return;

	if (!LookupInstTable(InstTable, OpPart.str.p_str))
		WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_RX(void)
{
	return False;
}

static void SwitchFrom_RX(void)
{
	DeinitFields();
}

static void SwitchTo_RX(void *pUser)
{
	const TFamilyDescr *pDescr;

	TurnWords = False;
	SetIntConstMode(eIntConstModeIntel);

	pDescr = FindFamilyByName("RX");
	PCSymbol = "$";
	HeaderID = pDescr->Id;
	NOPCode = 0x03;
	DivideChars = ",";
	HasAttrs = True;
	AttrChars = ".";

	ValidSegs = (1 << SegCode);
	Grans[SegCode] = 1;
	ListGrans[SegCode] = 1;
	SegInits[SegCode] = 0x0000;
	SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

	MakeCode = MakeCode_RX;
	IsDef = IsDef_RX;
	SwitchFrom = SwitchFrom_RX;
	InitFields();
	onoff_supmode_add();
	onoff_bigendian_add();

	pCurrCPUProps = (const tCPUProps *)pUser;
}

static const tCPUProps CPUProps[] =
{
	/* Group   InsSet FLT    DBL    RegB MVTIPL */
	{ "RXV1",  eRXv1, True,  False, 0,   True  },	/* Generic RXv1 (Full option) */
	{ "RX110", eRXv1, False, False, 0,   True  },
	{ "RX111", eRXv1, False, False, 0,   True  },
	{ "RX113", eRXv1, False, False, 0,   True  },
	{ "RX130", eRXv1, False, False, 0,   True  },
	{ "RX210", eRXv1, False, False, 0,   True  },
	{ "RX21A", eRXv1, False, False, 0,   True  },
	{ "RX220", eRXv1, False, False, 0,   True  },
	{ "RX610", eRXv1, True,  False, 0,   False },
	{ "RX621", eRXv1, True,  False, 0,   True  },
	{ "RX62N", eRXv1, True,  False, 0,   True  },
	{ "RX630", eRXv1, True,  False, 0,   True  },
	{ "RX631", eRXv1, True,  False, 0,   True  },

	{ "RXV2",  eRXv2, True,  False, 0,   True  },	/* Generic RXv2 */
	{ "RX140", eRXv2, True,  False, 0,   True  },
	{ "RX230", eRXv2, True,  False, 0,   True  },
	{ "RX231", eRXv2, True,  False, 0,   True  },
	{ "RX64M", eRXv2, True,  False, 0,   True  },
	{ "RX651", eRXv2, True,  False, 0,   True  },
	
	{ "RXV3",  eRXv3, True,  True,  256, True  },	/* Generic RXv3 (Full option) */
	{ "RX660", eRXv3, True,  False, 16,  True  },
	{ "RX671", eRXv3, True,  True,  16,  True  },
	{ "RX72M", eRXv3, True,  True,  16,  True  },
	{ "RX72N", eRXv3, True,  True,  16,  True  },

	{ ""     ,    eRXv1, False, False, 0,   False }
};

void coderx_init(void)
{
	const tCPUProps *pProp;

	for (pProp = CPUProps; pProp->Name[0]; pProp++)
		(void)AddCPUUser(pProp->Name, SwitchTo_RX, (void *)pProp, NULL);

	AddCopyright("Renesas RX Generator (C) 2023 Haruo Asano");
}
