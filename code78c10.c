/* code78c10.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator NEC uPD78(C)1x                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code78c10.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  const char *Name;
  Byte Code;
} SReg;

typedef struct
{
  const char *pName;
  Byte Code;
  Byte MayIndirect;
} tAdrMode;

#define SRegCnt 28


static LongInt WorkArea;

static CPUVar CPU7810, CPU78C10;

static SReg *SRegs;

static ASSUMERec ASSUME78C10s[] =
{
  {"V" , &WorkArea, 0, 0xff, 0x100, NULL}
};

/*--------------------------------------------------------------------------------*/

static Boolean Decode_r(char *Asc, ShortInt *Erg)
{
  static const char Names[] = "VABCDEHL";
  const char *p;

  if (strlen(Asc) != 1) return False;
  p = strchr(Names, as_toupper(*Asc));
  if (!p) return False;
  *Erg = p - Names;
  return True;
}

static Boolean Decode_r1(char *Asc, ShortInt *Erg)
{
  if (!as_strcasecmp(Asc, "EAL")) *Erg = 1;
  else if (!as_strcasecmp(Asc, "EAH")) *Erg = 0;
  else
  {
    if (!Decode_r(Asc, Erg)) return False;
    return (*Erg > 1);
  }
  return True;
}

static Boolean Decode_r2(char *Asc, ShortInt *Erg)
{
  if (!Decode_r(Asc, Erg)) return False;
  return ((*Erg > 0) && (*Erg < 4));
}

static Boolean Decode_rp2(char *Asc, ShortInt *Erg)
{
  static const SReg Regs[] =
  {
    { "SP" , 0 },
    { "B"  , 1 },
    { "BC" , 1 },
    { "D"  , 2 },
    { "DE" , 2 },
    { "H"  , 3 },
    { "HL" , 3 },
    { "EA" , 4 },
    { NULL , 0 },
  };

  for (*Erg = 0; Regs[*Erg].Name; (*Erg)++)
    if (!as_strcasecmp(Asc, Regs[*Erg].Name))
    {
      *Erg = Regs[*Erg].Code;
      return True;
    }
  return False;
}

static Boolean Decode_rp(char *Asc, ShortInt *Erg)
{
  if (!Decode_rp2(Asc, Erg)) return False;
  return (*Erg < 4);
}

static Boolean Decode_rp1(char *Asc, ShortInt *Erg)
{
  if (!as_strcasecmp(Asc, "V")) *Erg = 0;
  else
  {
    if (!Decode_rp2(Asc, Erg)) return False;
    return (*Erg != 0);
  }
  return True;
}

static Boolean Decode_rp3(char *Asc, ShortInt *Erg)
{
  if (!Decode_rp2(Asc, Erg)) return False;
  return ((*Erg < 4) && (*Erg > 0));
}

static Boolean DecodeAdrMode(char *pAsc, const tAdrMode pModes[],
                             ShortInt *pErg, Boolean *pWasIndirect)
{
  int z;

  if (!*pWasIndirect && (IsIndirect(pAsc)))
  {
    strmov(pAsc, pAsc + 1);
    pAsc[strlen(pAsc) - 1] = '\0';
    *pWasIndirect = True;
  }

  for (z = 0; pModes[z].pName; z++)
  {
    if (*pWasIndirect && !pModes[z].MayIndirect)
      continue;
    if (!as_strcasecmp(pAsc, pModes[z].pName))
    {
      *pErg = pModes[z].Code;
      return True;
    }
  }
  return False;
}

static Boolean Decode_rpa2(const tStrComp *pArg, Boolean *pWasIndirect, ShortInt *Erg, ShortInt *Disp)
{
  static const tAdrMode AdrModes[] =
  {
    { "B"   ,  1  , True  },
    { "BC"  ,  1  , True  },
    { "D"   ,  2  , True  },
    { "DE"  ,  2  , True  },
    { "H"   ,  3  , True  },
    { "HL"  ,  3  , True  },
    { "D+"  ,  4  , True  },
    { "DE+" ,  4  , True  },
    { "(DE)+", 4  , False },
    { "H+"   , 5  , True  },
    { "HL+"  , 5  , True  },
    { "(HL)+", 5  , False },
    { "D-"   , 6  , True  },
    { "DE-"  , 6  , True  },
    { "(DE)-", 6  , False },
    { "H-"   , 7  , True  },
    { "HL-"  , 7  , True  },
    { "(HL)-", 7  , False },
    { "H+A"  , 12 , True  },
    { "HL+A" , 12 , True  },
    { "A+H"  , 12 , True  },
    { "A+HL" , 12 , True  },
    { "H+B"  , 13 , True  },
    { "HL+B" , 13 , True  },
    { "B+H"  , 13 , True  },
    { "B+HL" , 13 , True  },
    { "H+EA" , 14 , True  },
    { "HL+EA", 14 , True  },
    { "EA+H" , 14 , True  },
    { "EA+HL", 14 , True  },
    { NULL  , 0   , False },
  };

  char *p, Save;
  Boolean OK;
  ShortInt BaseReg;
  tStrComp Left, Right;

  if (DecodeAdrMode(pArg->str.p_str, AdrModes, Erg, pWasIndirect))
  {
    *Disp = 0;
    return True;
  }

  p = QuotMultPos(pArg->str.p_str, "+-");
  if (!p) return False;

  Save = StrCompSplitRef(&Left, &Right, pArg, p);
  OK = (Decode_rp2(Left.str.p_str, &BaseReg));
  *p = Save;
  if (!OK || ((BaseReg != 2) && (BaseReg != 3)))
    return False;
  *Erg = (BaseReg == 3) ? 15 : 11;
  *Disp = EvalStrIntExpressionOffs(pArg, p - pArg->str.p_str, SInt8, &OK);
  return OK;
}

static Boolean Decode_rpa(const tStrComp *pArg, ShortInt *Erg)
{
  ShortInt Dummy;
  Boolean WasIndirect = False;

  if (!Decode_rpa2(pArg, &WasIndirect, Erg, &Dummy)) return False;
  return (*Erg <= 7);
}

static Boolean Decode_rpa1(const tStrComp *pArg, ShortInt *Erg)
{
  ShortInt Dummy;
  Boolean WasIndirect = False;

  if (!Decode_rpa2(pArg, &WasIndirect, Erg, &Dummy)) return False;
  return (*Erg <= 3);
}

static Boolean Decode_rpa3(const tStrComp *pArg, ShortInt *Erg, ShortInt *Disp)
{
  static const tAdrMode AdrModes[] =
  {
    { "D++"   , 4 , True  },
    { "DE++"  , 4 , True  },
    { "(DE)++", 4 , False },
    { "H++"   , 5 , True  },
    { "HL++"  , 5 , True  },
    { "(HL)++", 5 , False },
    { NULL    , 0 , False },
  };
  Boolean WasIndirect = False;

  if (DecodeAdrMode(pArg->str.p_str, AdrModes, Erg, &WasIndirect))
  {
    *Disp = 0;
    return True;
  }

  if (!Decode_rpa2(pArg, &WasIndirect, Erg, Disp))
    return False;
  return ((*Erg == 2) || (*Erg == 3) || (*Erg >= 8));
}

static Boolean Decode_f(char *Asc, ShortInt *Erg)
{
#define FlagCnt 3
  static const char Flags[FlagCnt][3] = {"CY", "HC", "Z"};

  for (*Erg = 0; *Erg < FlagCnt; (*Erg)++)
   if (!as_strcasecmp(Flags[*Erg], Asc)) break;
  *Erg += 2; return (*Erg <= 4);
}

static Boolean Decode_sr0(char *Asc, ShortInt *Erg)
{
  int z;

  for (z = 0; z < SRegCnt; z++)
   if (!as_strcasecmp(Asc, SRegs[z].Name)) break;
  if ((z == SRegCnt-1) && (MomCPU == CPU7810))
  {
    WrError(ErrNum_InvCtrlReg); return False;
  }
  if (z < SRegCnt)
  {
    *Erg = SRegs[z].Code; return True;
  }
  else return False;
}

static Boolean Decode_sr1(char *Asc, ShortInt *Erg)
{
  if (!Decode_sr0(Asc, Erg)) return False;
  return (((*Erg >= 0) && (*Erg <= 9)) || (*Erg == 11) || (*Erg == 13) || (*Erg == 25) || ((*Erg >= 32) && (*Erg <= 35)));
}

static Boolean Decode_sr(char *Asc, ShortInt *Erg)
{
  if (!Decode_sr0(Asc, Erg)) return False;
  return (((*Erg >= 0) && (*Erg <= 24)) || (*Erg == 26) || (*Erg == 27) || (*Erg == 40));
}

static Boolean Decode_sr2(char *Asc, ShortInt *Erg)
{
  if (!Decode_sr0(Asc, Erg)) return False;
  return (((*Erg >= 0) && (*Erg <= 9)) || (*Erg == 11) || (*Erg == 13));
}

static Boolean Decode_sr3(char *Asc, ShortInt *Erg)
{
  if (!as_strcasecmp(Asc, "ETM0")) *Erg = 0;
  else if (!as_strcasecmp(Asc, "ETM1")) *Erg = 1;
  else return False;
  return True;
}

static Boolean Decode_sr4(char *Asc, ShortInt *Erg)
{
  if (!as_strcasecmp(Asc, "ECNT")) *Erg = 0;
  else if (!as_strcasecmp(Asc, "ECPT")) *Erg = 1;
  else return False;
  return True;
}

static Boolean Decode_irf(char *Asc, ShortInt *Erg)
{
#undef FlagCnt
#define FlagCnt 18
  static const char FlagNames[FlagCnt][5] =
           { "NMI" , "FT0" , "FT1" , "F1"  , "F2"  , "FE0" ,
             "FE1" , "FEIN", "FAD" , "FSR" , "FST" , "ER"  ,
             "OV"  , "AN4" , "AN5" , "AN6" , "AN7" , "SB"   };
  static const ShortInt FlagCodes[FlagCnt] =
            { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20 };

  for (*Erg = 0; *Erg < FlagCnt; (*Erg)++)
   if (!as_strcasecmp(FlagNames[*Erg], Asc)) break;
  if (*Erg >= FlagCnt) return False;
  *Erg = FlagCodes[*Erg];
  return True;
}

static Boolean Decode_wa(const tStrComp *pArg, Byte *Erg)
{
  Word Adr;
  Boolean OK;
  tSymbolFlags Flags;

  Adr = EvalStrIntExpressionWithFlags(pArg, Int16, &OK, &Flags);
  if (!OK) return False;
  if (!mFirstPassUnknown(Flags) && (Hi(Adr) != WorkArea)) WrError(ErrNum_InAccPage);
  *Erg = Lo(Adr);
  return True;
}

static Boolean HasDisp(ShortInt Mode)
{
  return ((Mode & 11) == 11);
}

/*--------------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 0;
    if (Hi(Code) != 0)
      BAsmCode[CodeLen++] = Hi(Code);
    BAsmCode[CodeLen++] = Lo(Code);
  }
}

static void DecodeMOV(Word Code)
{
  Boolean OK;
  ShortInt HReg;
  Integer AdrInt;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
  {
    if (Decode_sr1(ArgStr[2].str.p_str, &HReg))
    {
      CodeLen = 2;
      BAsmCode[0] = 0x4c;
      BAsmCode[1] = 0xc0 + HReg;
    }
    else if (Decode_r1(ArgStr[2].str.p_str, &HReg))
    {
      CodeLen = 1;
      BAsmCode[0] = 0x08 + HReg;
    }
    else
    {
      AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
      if (OK)
      {
        CodeLen = 4;
        BAsmCode[0] = 0x70;
        BAsmCode[1] = 0x69;
        BAsmCode[2] = Lo(AdrInt);
        BAsmCode[3] = Hi(AdrInt);
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
  {
    if (Decode_sr(ArgStr[1].str.p_str, &HReg))
    {
      CodeLen = 2;
      BAsmCode[0] = 0x4d;
      BAsmCode[1] = 0xc0 + HReg;
    }
    else if (Decode_r1(ArgStr[1].str.p_str, &HReg))
    {
      CodeLen = 1;
      BAsmCode[0] = 0x18 + HReg;
    }
    else
    {
      AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
      if (OK)
      {
        CodeLen = 4;
        BAsmCode[0] = 0x70;
        BAsmCode[1] = 0x79;
        BAsmCode[2] = Lo(AdrInt);
        BAsmCode[3] = Hi(AdrInt);
      }
    }
  }
  else if (Decode_r(ArgStr[1].str.p_str, &HReg))
  {
    AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      CodeLen = 4;
      BAsmCode[0] = 0x70;
      BAsmCode[1] = 0x68 + HReg;
      BAsmCode[2] = Lo(AdrInt);
      BAsmCode[3] = Hi(AdrInt);
    }
  }
  else if (Decode_r(ArgStr[2].str.p_str, &HReg))
  {
    AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      CodeLen = 4;
      BAsmCode[0] = 0x70;
      BAsmCode[1] = 0x78 + HReg;
      BAsmCode[2] = Lo(AdrInt);
      BAsmCode[3] = Hi(AdrInt);
    }
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeMVI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    ShortInt HReg;
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      if (Decode_r(ArgStr[1].str.p_str, &HReg))
      {
        CodeLen = 2;
        BAsmCode[0] = 0x68 + HReg;
      }
      else if (Decode_sr2(ArgStr[1].str.p_str, &HReg))
      {
        CodeLen = 3;
        BAsmCode[2] = BAsmCode[1];
        BAsmCode[0] = 0x64;
        BAsmCode[1] = (HReg & 7) + ((HReg & 8) << 4);
      }
      else WrError(ErrNum_InvAddrMode);
    }
  }
}

static void DecodeMVIW(Word Code)
{
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (Decode_wa(&ArgStr[1], BAsmCode + 1))
  {
    BAsmCode[2] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x71;
    }
  }
}

static void DecodeMVIX(Word Code)
{
  Boolean OK;
  ShortInt HReg;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!Decode_rpa1(&ArgStr[1], &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x48 + HReg;
      CodeLen = 2;
    }
  }
}

static void DecodeLDAX_STAX(Word Code)
{
  ShortInt HReg;
  Boolean WasIndirect = False;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_rpa2(&ArgStr[1], &WasIndirect, &HReg, (ShortInt *) BAsmCode + 1)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1 + Ord(HasDisp(HReg));
    BAsmCode[0] = Code + ((HReg & 8) << 4) + (HReg & 7);
  }
}

static void DecodeLDEAX_STEAX(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_rpa3(&ArgStr[1], &HReg, (ShortInt *) BAsmCode + 2)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2 + Ord(HasDisp(HReg));
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Code + HReg;
  }
}

static void DecodeLXI(Word Code)
{
  ShortInt HReg;
  Integer AdrInt;
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!Decode_rp2(ArgStr[1].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    AdrInt = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x04 + (HReg << 4);
      BAsmCode[1] = Lo(AdrInt);
      BAsmCode[2] = Hi(AdrInt);
    }
  }
}

static void DecodePUSH_POP(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_rp1(ArgStr[1].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = Code + HReg;
  }
}

static void DecodeDMOV(Word Code)
{
  ShortInt HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean Swap = as_strcasecmp(ArgStr[1].str.p_str, "EA") || False;
    char *pArg1 = Swap ? ArgStr[2].str.p_str : ArgStr[1].str.p_str,
         *pArg2 = Swap ? ArgStr[1].str.p_str : ArgStr[2].str.p_str;

    if (as_strcasecmp(pArg1, "EA")) WrError(ErrNum_InvAddrMode);
    else if (Decode_rp3(pArg2, &HReg))
    {
      CodeLen = 1;
      BAsmCode[0] = 0xa4 + HReg;
      if (Swap)
        BAsmCode[0] += 0x10;
    }
    else if (((Swap) && (Decode_sr3(pArg2, &HReg)))
          || ((!Swap) && (Decode_sr4(pArg2, &HReg))))
    {
      CodeLen = 2;
      BAsmCode[0] = 0x48;
      BAsmCode[1] = 0xc0 + HReg;
      if (Swap)
        BAsmCode[1] += 0x12;
    }
    else
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeALUImm(Word Code)
{
  ShortInt HVal8, HReg;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    HVal8 = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      {
        CodeLen = 2;
        BAsmCode[0] = 0x06 + ((Code & 14) << 3) + (Code & 1);
        BAsmCode[1] = HVal8;
      }
      else if (Decode_r(ArgStr[1].str.p_str, &HReg))
      {
        CodeLen = 3;
        BAsmCode[0] = 0x74;
        BAsmCode[2] = HVal8;
        BAsmCode[1] = HReg + (Code << 3);
      }
      else if (Decode_sr2(ArgStr[1].str.p_str, &HReg))
      {
        CodeLen = 3;
        BAsmCode[0] = 0x64;
        BAsmCode[2] = HVal8;
        BAsmCode[1] = (HReg & 7) + (Code << 3) + ((HReg & 8) << 4);
      }
      else WrError(ErrNum_InvAddrMode);
    }
  }
}

static void DecodeALUReg(Word Code)
{
  ShortInt HReg;

  if (ChkArgCnt(2, 2))
  {
    Boolean NoSwap = !as_strcasecmp(ArgStr[1].str.p_str, "A");
    char *pArg1 = NoSwap ? ArgStr[1].str.p_str : ArgStr[2].str.p_str,
         *pArg2 = NoSwap ? ArgStr[2].str.p_str : ArgStr[1].str.p_str;

    if (as_strcasecmp(pArg1, "A")) WrError(ErrNum_InvAddrMode);
    else if (!Decode_r(pArg2, &HReg)) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 2;
      BAsmCode[0] = 0x60;
      BAsmCode[1] = (Code << 3) + HReg;
      if ((NoSwap) || (Memo("ONA")) || (Memo("OFFA")))
        BAsmCode[1] += 0x80;
    }
  }
}

static void DecodeALURegW(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (Decode_wa(&ArgStr[1], BAsmCode + 2))
  {
    CodeLen = 3;
    BAsmCode[0] = 0x74;
    BAsmCode[1] = 0x80 + (Code << 3);
  }
}

static void DecodeALURegX(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_rpa(&ArgStr[1], &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x70;
    BAsmCode[1] = 0x80 + (Code << 3) + HReg;
  }
}

static void DecodeALUEA(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrError(ErrNum_InvAddrMode);
  else if (!Decode_rp3(ArgStr[2].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x74;
    BAsmCode[1] = 0x84 + (Code << 3) + HReg;
  }
}

static void DecodeALUImmW(Word Code)
{
  Boolean OK;

  if (!ChkArgCnt(2, 2));
  else if (Decode_wa(&ArgStr[1], BAsmCode + 1))
  {
    BAsmCode[2] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x05 + ((Code >> 1) << 4);
    }
  }
}

static void DecodeAbs(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else
  {
    Boolean OK;
    Integer AdrInt;

    AdrInt = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK)
    {
      CodeLen = 0;
      if (Hi(Code) != 0)
        BAsmCode[CodeLen++] = Hi(Code);
      BAsmCode[CodeLen++] = Lo(Code);
      BAsmCode[CodeLen++] = Lo(AdrInt);
      BAsmCode[CodeLen++] = Hi(AdrInt);
    }
  }
}

static void DecodeReg2(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_r2(ArgStr[1].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 0;
    if (Hi(Code) != 0)
      BAsmCode[CodeLen++] = Hi(Code);
    BAsmCode[CodeLen++] = Lo(Code) + HReg;
  }
}

static void DecodeWork(Word Code)
{
  if (ChkArgCnt(1, 1)
   && Decode_wa(&ArgStr[1], BAsmCode + 1))
  {
    CodeLen = 2;
    BAsmCode[0] = Code;
  }
}

static void DecodeEA(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
  }
  return;
}

static void DecodeDCX_INX(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "EA"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0xa8 + Code;
  }
  else if (Decode_rp(ArgStr[1].str.p_str, &HReg))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x02 + Code + (HReg << 4);
  }
  else
    WrError(ErrNum_InvAddrMode);
}

static void DecodeEADD_ESUB(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrError(ErrNum_InvAddrMode);
  else if (!Decode_r2(ArgStr[2].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x70;
    BAsmCode[1] = Code + HReg;
  }
}

static void DecodeJ_JR_JRE(Word Type)
{
  Boolean OK;
  Integer AdrInt;
  tSymbolFlags Flags;

  if (!ChkArgCnt(1, 1))
    return;

  AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags) - (EProgCounter() + 1);
  if (!OK)
    return;

  if (!Type) /* generic J */
    Type = RangeCheck(AdrInt, SInt6) ? 1 : 2;

  switch (Type)
  {
    case 1: /* JR */
      if (!mSymbolQuestionable(Flags) && !RangeCheck(AdrInt, SInt6)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 0xc0 + (AdrInt & 0x3f);
      }
      break;
    case 2:
      AdrInt--; /* JRE is 2 bytes long */
      if (!mSymbolQuestionable(Flags) && !RangeCheck(AdrInt, SInt9)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0x4e + (Hi(AdrInt) & 1);
        BAsmCode[1] = Lo(AdrInt);
      }
      break;
  }
}

static void DecodeCALF(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Integer AdrInt;
    tSymbolFlags Flags;

    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (OK)
    {
      if (!mFirstPassUnknown(Flags) && ((AdrInt >> 11) != 1)) WrError(ErrNum_NotFromThisAddress);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = Hi(AdrInt) + 0x70;
        BAsmCode[1] = Lo(AdrInt);
      }
    }
  }
}

static void DecodeCALT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Integer AdrInt;
    tSymbolFlags Flags;

    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (OK)
    {
      if (!mFirstPassUnknown(Flags) && ((AdrInt & 0xffc1) != 0x80)) WrError(ErrNum_NotFromThisAddress);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 0x80 + ((AdrInt & 0x3f) >> 1);
      }
    }
  }
}

static void DecodeBIT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    ShortInt HReg;

    HReg = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
    if (OK)
     if (Decode_wa(&ArgStr[2], BAsmCode + 1))
     {
       CodeLen = 2; BAsmCode[0] = 0x58 + HReg;
     }
  }
}

static void DecodeSK_SKN(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_f(ArgStr[1].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Code + HReg;
  }
}

static void DecodeSKIT_SKNIT(Word Code)
{
  ShortInt HReg;

  if (!ChkArgCnt(1, 1));
  else if (!Decode_irf(ArgStr[1].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Code + HReg;
  }
}

/*--------------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  if ((!strcmp(NName, "STOP")) && (MomCPU == CPU7810));
  else
    AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddSReg(const char *NName, Word NCode)
{
  if (InstrZ >= SRegCnt) exit(255);
  SRegs[InstrZ].Name = NName;
  SRegs[InstrZ++].Code = NCode;
}

static void AddALU(Byte NCode, const char *NNameI, const char *NNameReg, const char *NNameEA)
{
  char Name[20];

  AddInstTable(InstTable, NNameI, NCode, DecodeALUImm);
  AddInstTable(InstTable, NNameReg, NCode, DecodeALUReg);
  AddInstTable(InstTable, NNameEA, NCode, DecodeALUEA);
  as_snprintf(Name, sizeof(Name), "%sW", NNameReg);
  AddInstTable(InstTable, Name, NCode, DecodeALURegW);
  as_snprintf(Name, sizeof(Name), "%sX", NNameReg);
  AddInstTable(InstTable, Name, NCode, DecodeALURegX);
  as_snprintf(Name, sizeof(Name), "%sW", NNameI);
  AddInstTable(InstTable, Name, NCode, DecodeALUImmW);
}

static void AddAbs(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAbs);
}

static void AddReg2(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeReg2);
}

static void AddWork(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeWork);
}

static void AddEA(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeEA);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(301);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MVI", 0, DecodeMVI);
  AddInstTable(InstTable, "MVIW", 0, DecodeMVIW);
  AddInstTable(InstTable, "MVIX", 0, DecodeMVIX);
  AddInstTable(InstTable, "LDAX", 0x28, DecodeLDAX_STAX);
  AddInstTable(InstTable, "STAX", 0x38, DecodeLDAX_STAX);
  AddInstTable(InstTable, "LDEAX", 0x80, DecodeLDEAX_STEAX);
  AddInstTable(InstTable, "STEAX", 0x90, DecodeLDEAX_STEAX);
  AddInstTable(InstTable, "LXI", 0, DecodeLXI);
  AddInstTable(InstTable, "PUSH", 0xb0, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 0xa0, DecodePUSH_POP);
  AddInstTable(InstTable, "DMOV", 0, DecodeDMOV);
  AddInstTable(InstTable, "DCX", 1, DecodeDCX_INX);
  AddInstTable(InstTable, "INX", 0, DecodeDCX_INX);
  AddInstTable(InstTable, "EADD", 0x40, DecodeEADD_ESUB);
  AddInstTable(InstTable, "ESUB", 0x60, DecodeEADD_ESUB);
  AddInstTable(InstTable, "JR", 1, DecodeJ_JR_JRE);
  AddInstTable(InstTable, "JRE", 2, DecodeJ_JR_JRE);
  AddInstTable(InstTable, "J", 0, DecodeJ_JR_JRE);
  AddInstTable(InstTable, "CALF", 0, DecodeCALF);
  AddInstTable(InstTable, "CALT", 0, DecodeCALT);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
  AddInstTable(InstTable, "SK", 0x08, DecodeSK_SKN);
  AddInstTable(InstTable, "SKN", 0x18, DecodeSK_SKN);
  AddInstTable(InstTable, "SKIT", 0x40, DecodeSKIT_SKNIT);
  AddInstTable(InstTable, "SKNIT", 0x60, DecodeSKIT_SKNIT);

  AddFixed("EXX"  , 0x0011); AddFixed("EXA"  , 0x0010);
  AddFixed("EXH"  , 0x0050); AddFixed("BLOCK", 0x0031);
  AddFixed("TABLE", 0x48a8); AddFixed("DAA"  , 0x0061);
  AddFixed("STC"  , 0x482b); AddFixed("CLC"  , 0x482a);
  AddFixed("NEGA" , 0x483a); AddFixed("RLD"  , 0x4838);
  AddFixed("RRD"  , 0x4839); AddFixed("JB"   , 0x0021);
  AddFixed("JEA"  , 0x4828); AddFixed("CALB" , 0x4829);
  AddFixed("SOFTI", 0x0072); AddFixed("RET"  , 0x00b8);
  AddFixed("RETS" , 0x00b9); AddFixed("RETI" , 0x0062);
  AddFixed("NOP"  , 0x0000); AddFixed("EI"   , 0x00aa);
  AddFixed("DI"   , 0x00ba); AddFixed("HLT"  , 0x483b);
  AddFixed("STOP" , 0x48bb);

  SRegs = (SReg *) malloc(sizeof(SReg)*SRegCnt); InstrZ = 0;
  AddSReg("PA"  , 0x00); AddSReg("PB"  , 0x01);
  AddSReg("PC"  , 0x02); AddSReg("PD"  , 0x03);
  AddSReg("PF"  , 0x05); AddSReg("MKH" , 0x06);
  AddSReg("MKL" , 0x07); AddSReg("ANM" , 0x08);
  AddSReg("SMH" , 0x09); AddSReg("SML" , 0x0a);
  AddSReg("EOM" , 0x0b); AddSReg("ETMM", 0x0c);
  AddSReg("TMM" , 0x0d); AddSReg("MM"  , 0x10);
  AddSReg("MCC" , 0x11); AddSReg("MA"  , 0x12);
  AddSReg("MB"  , 0x13); AddSReg("MC"  , 0x14);
  AddSReg("MF"  , 0x17); AddSReg("TXB" , 0x18);
  AddSReg("RXB" , 0x19); AddSReg("TM0" , 0x1a);
  AddSReg("TM1" , 0x1b); AddSReg("CR0" , 0x20);
  AddSReg("CR1" , 0x21); AddSReg("CR2" , 0x22);
  AddSReg("CR3" , 0x23); AddSReg("ZCM" , 0x28);

  AddALU(10, "ACI"  , "ADC"  , "DADC"  );
  AddALU( 4, "ADINC", "ADDNC", "DADDNC");
  AddALU( 8, "ADI"  , "ADD"  , "DADD"  );
  AddALU( 1, "ANI"  , "ANA"  , "DAN"   );
  AddALU(15, "EQI"  , "EQA"  , "DEQ"   );
  AddALU( 5, "GTI"  , "GTA"  , "DGT"   );
  AddALU( 7, "LTI"  , "LTA"  , "DLT"   );
  AddALU(13, "NEI"  , "NEA"  , "DNE"   );
  AddALU(11, "OFFI" , "OFFA" , "DOFF"  );
  AddALU( 9, "ONI"  , "ONA"  , "DON"   );
  AddALU( 3, "ORI"  , "ORA"  , "DOR"   );
  AddALU(14, "SBI"  , "SBB"  , "DSBB"  );
  AddALU( 6, "SUINB", "SUBNB", "DSUBNB");
  AddALU(12, "SUI"  , "SUB"  , "DSUB"  );
  AddALU( 2, "XRI"  , "XRA"  , "DXR"   );

  AddAbs("CALL", 0x0040); AddAbs("JMP" , 0x0054);
  AddAbs("LBCD", 0x701f); AddAbs("LDED", 0x702f);
  AddAbs("LHLD", 0x703f); AddAbs("LSPD", 0x700f);
  AddAbs("SBCD", 0x701e); AddAbs("SDED", 0x702e);
  AddAbs("SHLD", 0x703e); AddAbs("SSPD", 0x700e);

  AddReg2("DCR" , 0x0050); AddReg2("DIV" , 0x483c);
  AddReg2("INR" , 0x0040); AddReg2("MUL" , 0x482c);
  AddReg2("RLL" , 0x4834); AddReg2("RLR" , 0x4830);
  AddReg2("SLL" , 0x4824); AddReg2("SLR" , 0x4820);
  AddReg2("SLLC", 0x4804); AddReg2("SLRC", 0x4800);

  AddWork("DCRW", 0x30); AddWork("INRW", 0x20);
  AddWork("LDAW", 0x01); AddWork("STAW", 0x63);

  AddEA("DRLL", 0x48b4); AddEA("DRLR", 0x48b0);
  AddEA("DSLL", 0x48a4); AddEA("DSLR", 0x48a0);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  free(SRegs);
}

/*--------------------------------------------------------------------------*/

static void MakeCode_78C10(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_78C10(void)
{
  WorkArea = 0x100;
}

static Boolean IsDef_78C10(void)
{
  return False;
}

static void SwitchFrom_78C10(void)
{
  DeinitFields();
}

static void SwitchTo_78C10(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = 0x7a; NOPCode = 0x00;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  pASSUMERecs = ASSUME78C10s;
  ASSUMERecCnt = sizeof(ASSUME78C10s) / sizeof(*ASSUME78C10s);

  MakeCode = MakeCode_78C10; IsDef = IsDef_78C10;
  SwitchFrom = SwitchFrom_78C10;
  InitFields();
}

void code78c10_init(void)
{
  CPU7810 = AddCPU("7810" , SwitchTo_78C10);
  CPU78C10 = AddCPU("78C10", SwitchTo_78C10);

  AddInitPassProc(InitCode_78C10);
}
