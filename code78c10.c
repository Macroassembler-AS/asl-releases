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
#include "headids.h"

#include "code78c10.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  const char *p_name;
  Byte code;
} intflag_t;

typedef struct
{
  const char *p_name;
  Byte code, flags;
} sreg_t;

typedef struct
{
  const char *pName;
  Byte Code;
  Byte MayIndirect;
} tAdrMode;

typedef enum
{
  eCoreNone,
  eCore7800Low,
  eCore7800High,
  eCore7810
} tCore;

#define core_mask_no_low ((1 << eCore7800High) | (1 << eCore7810))
#define core_mask_7800 ((1 << eCore7800Low) | (1 << eCore7800High))
#define core_mask_7810 (1 << eCore7810)
#define core_mask_all ((1 << eCore7800Low) | (1 << eCore7800High) | (1 << eCore7810))

enum
{
  eFlagHasV = 1 << 0,
  eFlagCMOS = 1 << 1,
  eFlagSR   = 1 << 2, /* sr  -> may be written from A */
  eFlagSR1  = 1 << 3, /* sr1 -> may be read to A */
  eFlagSR2  = 1 << 4  /* sr2 -> load or read/modify/write with immediate */
};

typedef struct
{
  char Name[6];
  Byte Core;
  Byte Flags;
} tCPUProps;

typedef enum { e_decode_reg_unknown, e_decode_reg_ok, e_decode_reg_error } decode_reg_res_t;

typedef struct
{
  Word code;
  unsigned core_mask;
} order_t;

#define FixedOrderCnt 38
#define Reg2OrderCnt 10
#define SRegCnt 28
#define IntFlagCnt 18

static LongInt WorkArea;

static const tCPUProps *pCurrCPUProps;

static order_t *fixed_orders, *reg2_orders;
static intflag_t *int_flags;
static sreg_t *s_regs;

/*--------------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     decode_r(const tStrComp *p_arg, ShortInt *p_res)
 * \brief  decode name of 8 bit register
 * \param  p_arg source argument
 * \param  p_res return buffer
 * \return e_decode_reg_ok -> register known & OK
 * ------------------------------------------------------------------------ */

static decode_reg_res_t decode_r(const tStrComp *p_arg, ShortInt *p_res)
{
  static const char Names[] = "VABCDEHL";
  const char *p;

  if (strlen(p_arg->str.p_str) != 1)
    return e_decode_reg_unknown;

  p = strchr(Names, as_toupper(p_arg->str.p_str[0]));
  if (!p)
    return e_decode_reg_unknown;

  *p_res = p - Names;
  if ((0 == *p_res) && !(pCurrCPUProps->Flags & eFlagHasV))
  {
    WrStrErrorPos(ErrNum_InvReg, p_arg);
    return e_decode_reg_error;
  }
  return e_decode_reg_ok;
}

/*!------------------------------------------------------------------------
 * \fn     decode_r1(const tStrComp *p_arg, ShortInt *p_res)
 * \brief  decode name of 8 bit register, plus EAL/H on 78C1x
 * \param  p_arg source argument
 * \param  p_res return buffer
 * \return e_decode_reg_ok -> register known & OK
 * ------------------------------------------------------------------------ */

static decode_reg_res_t decode_r1(const tStrComp *p_arg, ShortInt *p_res)
{
  if ((pCurrCPUProps->Core == eCore7810) && !as_strcasecmp(p_arg->str.p_str, "EAL")) *p_res = 1;
  else if ((pCurrCPUProps->Core == eCore7810) && !as_strcasecmp(p_arg->str.p_str, "EAH")) *p_res = 0;
  else
  {
    decode_reg_res_t ret = decode_r(p_arg, p_res);

    if ((e_decode_reg_ok == ret) && (*p_res < 2))
    {
      WrStrErrorPos(ErrNum_InvReg, p_arg);
      return e_decode_reg_error;
    }
    return ret;
  }
  return e_decode_reg_ok;
}

static decode_reg_res_t decode_r2(const tStrComp *p_arg, ShortInt *p_res)
{
  decode_reg_res_t ret = decode_r(p_arg, p_res);

  if ((e_decode_reg_ok == ret) && ((*p_res == 0) || (*p_res >= 4)))
  {
    WrStrErrorPos(ErrNum_InvReg, p_arg);
    return e_decode_reg_error;
  }
  return ret;
}

static Boolean Decode_rp2(char *p_arg, ShortInt *p_res)
{
  static const sreg_t regs[] =
  {
    { "SP" , 0, 0 },
    { "B"  , 1, 0 },
    { "BC" , 1, 0 },
    { "D"  , 2, 0 },
    { "DE" , 2, 0 },
    { "H"  , 3, 0 },
    { "HL" , 3, 0 },
    { "EA" , 4, 0 },
    { NULL , 0, 0 },
  };

  for (*p_res = 0; regs[*p_res].p_name; (*p_res)++)
    if (!as_strcasecmp(p_arg, regs[*p_res].p_name))
    {
      *p_res = regs[*p_res].code;
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

static const sreg_t *decode_sr_core(const tStrComp *p_arg)
{
  int z;

  for (z = 0; (z < SRegCnt) && s_regs[z].p_name; z++)
    if (!as_strcasecmp(p_arg->str.p_str, s_regs[z].p_name))
      return &s_regs[z];
  return NULL;
}

static decode_reg_res_t decode_sr_flag(const tStrComp *p_arg, ShortInt *p_res, Byte flag)
{
  const sreg_t *p_reg = decode_sr_core(p_arg);

  if (p_reg)
  {
    if (p_reg->flags & flag)
    {
      *p_res = p_reg->code;
      return e_decode_reg_ok;
    }
    else
    {
      WrStrErrorPos(p_reg->flags ? ErrNum_InvOpOnReg : ErrNum_InvReg, p_arg);
      return e_decode_reg_error;
    }
  }
  else
    return e_decode_reg_unknown;
}

#define decode_sr1(p_arg, p_res) decode_sr_flag(p_arg, p_res, eFlagSR1)
#define decode_sr(p_arg, p_res) decode_sr_flag(p_arg, p_res, eFlagSR)
#define decode_sr2(p_arg, p_res) decode_sr_flag(p_arg, p_res, eFlagSR2)

static Boolean Decode_sr3(const tStrComp *p_arg, ShortInt *p_res)
{
  if (!as_strcasecmp(p_arg->str.p_str, "ETM0")) *p_res = 0;
  else if (!as_strcasecmp(p_arg->str.p_str, "ETM1")) *p_res = 1;
  else
  {
    WrStrErrorPos(ErrNum_InvCtrlReg, p_arg);
    return False;
  }
  return True;
}

static Boolean Decode_sr4(const tStrComp *p_arg, ShortInt *p_res)
{
  if (!as_strcasecmp(p_arg->str.p_str, "ECNT")) *p_res = 0;
  else if (!as_strcasecmp(p_arg->str.p_str, "ECPT")) *p_res = 1;
  else
  {
    WrStrErrorPos(ErrNum_InvCtrlReg, p_arg);
    return False;
  }
  return True;
}

static Boolean Decode_irf(const tStrComp *p_arg, ShortInt *p_res)
{
  for (*p_res = 0; (*p_res < IntFlagCnt) && int_flags[*p_res].p_name; (*p_res)++)
    if (!as_strcasecmp(int_flags[*p_res].p_name, p_arg->str.p_str))
    {
      *p_res = int_flags[*p_res].code;
      return True;
    }
  WrStrErrorPos(ErrNum_UnknownInt, p_arg);
  return False;
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

static Boolean check_core(unsigned core_mask)
{
  if ((core_mask >> pCurrCPUProps->Core) & 1)
    return True;
  WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  return False;
}

static void PutCode(Word Code)
{
  if (Hi(Code) != 0)
    BAsmCode[CodeLen++] = Hi(Code);
  BAsmCode[CodeLen++] = Lo(Code);
}

static void DecodeFixed(Word index)
{
  const order_t *p_order = &fixed_orders[index];

  if (ChkArgCnt(0, 0) && check_core(p_order->core_mask))
    PutCode(p_order->code);
}

static void DecodeMOV(Word Code)
{
  Boolean OK;
  ShortInt HReg;
  Integer AdrInt;
  decode_reg_res_t res1;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
  {
    decode_reg_res_t res2;

    if ((res2 = decode_sr1(&ArgStr[2], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 2;
        BAsmCode[0] = 0x4c;
        BAsmCode[1] = 0xc0 + HReg;
      }
    }
    else if ((res2 = decode_r1(&ArgStr[2], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 1;
        BAsmCode[0] = 0x08 + HReg;
      }
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
    decode_reg_res_t res2;

    if ((res2 = decode_sr(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 2;
        BAsmCode[0] = 0x4d;
        BAsmCode[1] = 0xc0 + HReg;
      }
    }
    else if ((res2 = decode_r1(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
    {
      if (res2 == e_decode_reg_ok)
      {
        CodeLen = 1;
        BAsmCode[0] = 0x18 + HReg;
      }
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
  else if ((res1 = decode_r(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
  {
    if (res1 == e_decode_reg_ok)
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
  }
  else if ((res1 = decode_r(&ArgStr[2], &HReg)) != e_decode_reg_unknown)
  {
    if (res1 == e_decode_reg_ok)
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
      decode_reg_res_t res;

      if ((res = decode_r(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
      {
        if (res == e_decode_reg_ok)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x68 + HReg;
        }
      }
      else if ((res = decode_sr2(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
      {
        if (res == e_decode_reg_ok)
        {
          if (pCurrCPUProps->Core != eCore7810) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
          else
          {
            CodeLen = 3;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[0] = 0x64;
            BAsmCode[1] = (HReg & 7) + ((HReg & 8) << 4);
          }
        }
      }
      else WrError(ErrNum_InvAddrMode);
    }
  }
}

static void DecodeMVIW(Word Code)
{
  Boolean OK;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2) || !check_core(core_mask_no_low));
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

  if (!ChkArgCnt(2, 2) || !check_core(core_mask_no_low));
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

  if (!ChkArgCnt(1, 1) || !check_core(core_mask_7810));
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
    PutCode(Code | (HReg << (Hi(Code) ? 4 : 0)));
}

static void DecodeDMOV(Word Code)
{
  ShortInt HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2) && check_core(core_mask_7810))
  {
    Boolean Swap = as_strcasecmp(ArgStr[1].str.p_str, "EA") || False;
    const tStrComp *pArg1 = Swap ? &ArgStr[2] : &ArgStr[1],
                   *pArg2 = Swap ? &ArgStr[1] : &ArgStr[2];

    if (as_strcasecmp(pArg1->str.p_str, "EA")) WrError(ErrNum_InvAddrMode);
    else if (Decode_rp3(pArg2->str.p_str, &HReg))
    {
      CodeLen = 1;
      BAsmCode[0] = 0xa4 + HReg;
      if (Swap)
        BAsmCode[0] += 0x10;
    }
    else if ((Swap && Decode_sr3(pArg2, &HReg))
          || (!Swap && Decode_sr4(pArg2, &HReg)))
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

#define ALUImm_SR (1 << 0)
#define ALUReg_Src (1 << 1)
#define ALUReg_Dest (1 << 2)

static void DecodeALUImm(Word Code)
{
  ShortInt HVal8, HReg;
  Boolean OK;
  Byte Flags;

  Flags = Hi(Code);
  Code = Lo(Code);

  if (ChkArgCnt(2, 2))
  {
    HVal8 = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      tStrComp Arg1;
      decode_reg_res_t res;

      /* allow >A to enforce long addressing */

      StrCompRefRight(&Arg1, &ArgStr[1], as_strcasecmp(ArgStr[1].str.p_str, ">A") ? 0 : 1);
      if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      {
        CodeLen = 2;
        BAsmCode[0] = 0x06 + ((Code & 14) << 3) + (Code & 1);
        BAsmCode[1] = HVal8;
      }
      else if ((res = decode_r(&Arg1, &HReg)) != e_decode_reg_unknown)
      {
        if (res != e_decode_reg_ok);
        else if (pCurrCPUProps->Core == eCore7800Low) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          CodeLen = 3;
          BAsmCode[0] = (pCurrCPUProps->Core == eCore7810) ? 0x74 : 0x64;
          BAsmCode[2] = HVal8;
          BAsmCode[1] = HReg + (Code << 3);
        }
      }
      else if ((res = decode_sr2(&ArgStr[1], &HReg)) != e_decode_reg_unknown)
      {
        if (res != e_decode_reg_ok);
        else if (!(Flags & ALUImm_SR)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          CodeLen = 3;
          BAsmCode[0] = 0x64;
          BAsmCode[1] = (HReg & 7) | (Code << 3)
                      | ((pCurrCPUProps->Core == eCore7810) ? ((HReg & 8) << 4) : 0x80);
          BAsmCode[2] = HVal8;
        }
      }
      else WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
}

static void DecodeALUReg(Word Code)
{
  ShortInt HReg;
  Byte Flags;

  Flags = Hi(Code);
  Code = Lo(Code);

  if (ChkArgCnt(2, 2))
  {
    Boolean NoSwap = !as_strcasecmp(ArgStr[1].str.p_str, "A");
    tStrComp *pArg1 = NoSwap ? &ArgStr[1] : &ArgStr[2],
             *pArg2 = NoSwap ? &ArgStr[2] : &ArgStr[1];
    decode_reg_res_t res;

    if (as_strcasecmp(pArg1->str.p_str, "A")) WrStrErrorPos(ErrNum_InvAddrMode, pArg1);
    else if ((res = decode_r(pArg2, &HReg)) == e_decode_reg_unknown) WrStrErrorPos(ErrNum_InvAddrMode, pArg2);
    else if (res == e_decode_reg_ok)
    {
      if (NoSwap && !(Flags & ALUReg_Src)) WrStrErrorPos(ErrNum_InvAddrMode, pArg2);
      else if (!NoSwap && !(Flags & ALUReg_Dest)) WrStrErrorPos(ErrNum_InvAddrMode, pArg2);
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
}

static void DecodeALURegW(Word Code)
{
  if (ChkArgCnt(1, 1)
   && check_core(core_mask_no_low)
   && Decode_wa(&ArgStr[1], BAsmCode + 2))
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
      PutCode(Code);
      BAsmCode[CodeLen++] = Lo(AdrInt);
      BAsmCode[CodeLen++] = Hi(AdrInt);
    }
  }
}

static void DecodeReg2(Word index)
{
  const order_t *p_order = &reg2_orders[index];
  ShortInt HReg;
  decode_reg_res_t res;

  if (!ChkArgCnt(1, 1) || !check_core(p_order->core_mask));
  else if ((res = decode_r2(&ArgStr[1], &HReg)) == e_decode_reg_unknown) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else if (res == e_decode_reg_ok)
    PutCode(p_order->code + HReg);
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

static void DecodeA(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
    PutCode(Code);
}

static void DecodeEA(Word Code)
{
  if (!ChkArgCnt(1, 1) || !check_core(core_mask_7810));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
  }
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
  decode_reg_res_t res;

  if (!ChkArgCnt(2, 2) || !check_core(core_mask_7810));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "EA")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else if ((res = decode_r2(&ArgStr[2], &HReg)) == e_decode_reg_unknown) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
  else if (res == e_decode_reg_ok)
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
      Word AdrMask = (pCurrCPUProps->Core == eCore7810) ? 0xffc1 : 0xff81;

      if (!mFirstPassUnknown(Flags) && ((AdrInt & AdrMask) != 0x80)) WrError(ErrNum_NotFromThisAddress);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 0x80 + ((AdrInt & ~AdrMask) >> 1);
      }
    }
  }
}

static void DecodeBIT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2) && check_core(core_mask_no_low))
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

  if (!ChkArgCnt(1, 1) || !check_core(Hi(Code)));
  else if (!Decode_f(ArgStr[1].str.p_str, &HReg)) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 2;
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Lo(Code) + HReg;
  }
}

static void DecodeSKIT_SKNIT(Word Code)
{
  ShortInt HReg;

  if (ChkArgCnt(1, 1)
   && check_core(Hi(Code))
   && Decode_irf(&ArgStr[1], &HReg))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x48;
    BAsmCode[1] = Lo(Code) + HReg;
  }
}

static void DecodeIN_OUT(Word Code)
{
  const tStrComp *p_port_arg;

  switch (ArgCnt)
  {
    case 2:
    {
      const tStrComp *p_acc_arg = &ArgStr[(Code & 1) + 1];

      if (as_strcasecmp(p_acc_arg->str.p_str, "A"))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_acc_arg);
        return;
      }
      p_port_arg = &ArgStr[2 - (Code & 1)];
      goto common;
    }
    case 1:
      p_port_arg = &ArgStr[1];
      /* fall-thru */
    common:
    {
      Boolean OK;

      BAsmCode[1] = EvalStrIntExpression(p_port_arg, UInt8, &OK);
      if (OK)
      {
        BAsmCode[0] = Code;
        CodeLen = 2;
      }
      break;
    }
    default:
      (void)ChkArgCnt(1, 2);
  }
}

/*--------------------------------------------------------------------------------*/

static void AddFixed(const char *p_name, Word code, unsigned core_mask)
{
  if (InstrZ >= FixedOrderCnt) exit(255);
  fixed_orders[InstrZ].code = code;
  fixed_orders[InstrZ].core_mask = core_mask;
  AddInstTable(InstTable, p_name, InstrZ++, DecodeFixed);
}

static void AddSReg(const char *p_name, Word code, Byte flags)
{
  if (InstrZ >= SRegCnt) exit(255);
  s_regs[InstrZ].p_name = p_name;
  s_regs[InstrZ].code = code;
  s_regs[InstrZ++].flags = flags;
}

static void AddIntFlag(const char *p_name, Byte code)
{
  if (InstrZ >= IntFlagCnt) exit(255);
  int_flags[InstrZ].p_name = p_name;
  int_flags[InstrZ++].code = code;
}

static void AddALU(Byte NCode, Word Flags, const char *NNameI, const char *NNameReg, const char *NNameEA)
{
  char Name[20];

  AddInstTable(InstTable, NNameI, ((Flags & ALUImm_SR) << 8) | NCode, DecodeALUImm);
  AddInstTable(InstTable, NNameReg, ((Flags & (ALUReg_Src | ALUReg_Dest)) << 8) | NCode, DecodeALUReg);
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

static void AddReg2(const char *p_name, Word code, unsigned core_mask)
{
  if (InstrZ >= Reg2OrderCnt) exit(255);
  reg2_orders[InstrZ].code = code;
  reg2_orders[InstrZ].core_mask = core_mask;
  AddInstTable(InstTable, p_name, InstrZ++, DecodeReg2);
}

static void AddWork(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeWork);
}

static void AddA(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeA);
}

static void AddEA(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeEA);
}

static void InitFields(void)
{
  Boolean IsLow = pCurrCPUProps->Core == eCore7800Low,
          IsHigh = pCurrCPUProps->Core == eCore7800High,
          Is781x = pCurrCPUProps->Core == eCore7810;

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
  AddInstTable(InstTable, "PUSH", Is781x ? 0xb0 : 0x480e, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", Is781x ? 0xa0 : 0x480f, DecodePUSH_POP);
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
  AddInstTable(InstTable, "SK" , (core_mask_no_low << 8) | 0x08, DecodeSK_SKN);
  AddInstTable(InstTable, "SKN", (core_mask_all    << 8) | 0x18, DecodeSK_SKN);
  AddInstTable(InstTable, "SKIT" , (core_mask_no_low << 8) | (Is781x ? 0x40 : 0x00), DecodeSKIT_SKNIT);
  AddInstTable(InstTable, "SKNIT", (core_mask_all    << 8) | (Is781x ? 0x60 : 0x10), DecodeSKIT_SKNIT);

  fixed_orders = (order_t*) calloc(sizeof(*fixed_orders), FixedOrderCnt); InstrZ = 0;
  AddFixed("EX"   , 0x0010                   , (1 << eCore7800High));
  AddFixed("PEN"  , 0x482c                   , (1 << eCore7800High));
  AddFixed("RCL"  , 0x4832                   , (1 << eCore7800High));
  AddFixed("RCR"  , 0x4833                   , (1 << eCore7800High));
  AddFixed("SHAL" , 0x4834                   , (1 << eCore7800High));
  AddFixed("SHAR" , 0x4835                   , (1 << eCore7800High));
  AddFixed("SHCL" , 0x4836                   , (1 << eCore7800High));
  AddFixed("SHCR" , 0x4837                   , (1 << eCore7800High));
  AddFixed("EXX"  , 0x0011                   , core_mask_no_low);
  AddFixed("EXA"  , 0x0010                   , core_mask_all);
  AddFixed("EXH"  , 0x0050                   , core_mask_all);
  AddFixed("BLOCK", 0x0031                   , core_mask_no_low);
  AddFixed("TABLE", Is781x ? 0x48a8 : 0x0021 , core_mask_no_low);
  AddFixed("DAA"  , 0x0061                   , core_mask_all);
  AddFixed("STC"  , 0x482b                   , core_mask_all);
  AddFixed("CLC"  , 0x482a                   , core_mask_all);
  AddFixed("NEGA" , 0x483a                   , core_mask_all);
  AddFixed("RLD"  , 0x4838                   , core_mask_all);
  AddFixed("RRD"  , 0x4839                   , core_mask_all);
  AddFixed("JB"   , Is781x ? 0x0021 : 0x0073 , core_mask_all);
  AddFixed("JEA"  , 0x4828                   , core_mask_all);
  AddFixed("CALB" , Is781x ? 0x4829 : 0x0063 , core_mask_no_low);
  AddFixed("SOFTI", 0x0072                   , core_mask_no_low);
  AddFixed("RET"  , Is781x ? 0x00b8 : 0x0008 , core_mask_all);
  AddFixed("RETS" , Is781x ? 0x00b9 : 0x0018 , core_mask_all);
  AddFixed("RETI" , 0x0062                   , core_mask_all);
  AddFixed("NOP"  , 0x0000                   , core_mask_all);
  AddFixed("EI"   , Is781x ? 0x00aa : 0x4820 , core_mask_all);
  AddFixed("DI"   , Is781x ? 0x00ba : 0x4824 , core_mask_all);
  AddFixed("HLT"  , Is781x ? 0x483b : 0x0001 , core_mask_no_low);
  AddFixed("SIO"  , 0x0009                   , core_mask_7800);
  AddFixed("STM"  , 0x0019                   , core_mask_7800);
  AddFixed("PEX"  , 0x482d                   , core_mask_7800);
  AddFixed("RAL"  , Is781x ? 0x4835 : 0x4830 , core_mask_all);
  AddFixed("RAR"  , 0x4831                   , core_mask_all);
  AddFixed("PER"  , 0x483c                   , core_mask_7800);
  if (pCurrCPUProps->Flags & eFlagCMOS)
    AddFixed("STOP" , 0x48bb, core_mask_all);

  s_regs = (sreg_t*) calloc(SRegCnt, sizeof(*s_regs)); InstrZ = 0;
  AddSReg("PA"  , 0x00, eFlagSR | eFlagSR1 | eFlagSR2);
  AddSReg("PB"  , 0x01, eFlagSR | eFlagSR1 | eFlagSR2);
  if (Is781x)
  {
    AddSReg("PC"  , 0x02, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("PD"  , 0x03, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("PF"  , 0x05, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("MKH" , 0x06, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("MKL" , 0x07, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("ANM" , 0x08, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("SMH" , 0x09, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("SML" , 0x0a, eFlagSR);
    AddSReg("EOM" , 0x0b, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("ETMM", 0x0c, eFlagSR);
    AddSReg("TMM" , 0x0d, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("MM"  , 0x10, eFlagSR);
    AddSReg("MCC" , 0x11, eFlagSR);
    AddSReg("MA"  , 0x12, eFlagSR);
    AddSReg("MB"  , 0x13, eFlagSR);
    AddSReg("MC"  , 0x14, eFlagSR);
    AddSReg("MF"  , 0x17, eFlagSR);
    AddSReg("TXB" , 0x18, eFlagSR);
    AddSReg("RXB" , 0x19, eFlagSR1);
    AddSReg("TM0" , 0x1a, eFlagSR);
    AddSReg("TM1" , 0x1b, eFlagSR);
    AddSReg("CR0" , 0x20, eFlagSR1);
    AddSReg("CR1" , 0x21, eFlagSR1);
    AddSReg("CR2" , 0x22, eFlagSR1);
    AddSReg("CR3" , 0x23, eFlagSR1);
    AddSReg("ZCM" , 0x28, (pCurrCPUProps->Flags & eFlagCMOS) ? eFlagSR : 0);
    /* 0x28 eFlagSR ? */
  }
  else
  {
    AddSReg("MK"  , 0x03, eFlagSR | eFlagSR1 | eFlagSR2);
    AddSReg("MB"  , 0x04, eFlagSR);
    AddSReg("PC"  , 0x02, eFlagSR1 | (IsHigh ? eFlagSR | eFlagSR2 : eFlagSR2));
    AddSReg("MC"  , 0x05, IsHigh ? eFlagSR : 0);
    AddSReg("TM0" , 0x06, IsHigh ? eFlagSR : 0);
    AddSReg("TM1" , 0x07, IsHigh ? eFlagSR : 0);
    AddSReg("TM"  , 0x06, IsLow ? eFlagSR : 0);
    AddSReg("SM"  , 0x0a, IsLow ? eFlagSR | eFlagSR1 : 0);
    AddSReg("SC"  , 0x0b, IsLow ? eFlagSR | eFlagSR1 : 0);
    AddSReg("S"   , 0x08, eFlagSR | eFlagSR1);
    AddSReg("TMM" , 0x09, eFlagSR | eFlagSR1);
  }

  int_flags = (intflag_t*) calloc(IntFlagCnt, sizeof(*int_flags)); InstrZ = 0;
  if (Is781x)
  {
    AddIntFlag("NMI" , 0);
    AddIntFlag("FT0" , 1);
    AddIntFlag("FT1" , 2);
    AddIntFlag("F1"  , 3);
    AddIntFlag("F2"  , 4);
    AddIntFlag("FE0" , 5);
    AddIntFlag("FE1" , 6);
    AddIntFlag("FEIN", 7);
    AddIntFlag("FAD" , 8);
    AddIntFlag("FSR" , 9);
    AddIntFlag("FST" ,10);
    AddIntFlag("ER"  ,11);
    AddIntFlag("OV"  ,12);
    AddIntFlag("AN4" ,16);
    AddIntFlag("AN5" ,17);
    AddIntFlag("AN6" ,18);
    AddIntFlag("AN7" ,19);
    AddIntFlag("SB"  ,20);
  }
  else
  {
    AddIntFlag("F0"  , 0);
    AddIntFlag("FT"  , 1);
    AddIntFlag("F1"  , 2);
    if (IsHigh)
      AddIntFlag("F2"  , 3);
    AddIntFlag("FS"  , 4);
  }

  AddALU(10, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "ACI"  , "ADC"  , "DADC"  );
  AddALU( 4, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "ADINC", "ADDNC", "DADDNC");
  AddALU( 8, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "ADI"  , "ADD"  , "DADD"  );
  AddALU( 1, IsLow ? ALUImm_SR | ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "ANI"  , "ANA"  , "DAN"   );
  AddALU(15, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "EQI"  , "EQA"  , "DEQ"   );
  AddALU( 5, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "GTI"  , "GTA"  , "DGT"   );
  AddALU( 7, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "LTI"  , "LTA"  , "DLT"   );
  AddALU(13, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "NEI"  , "NEA"  , "DNE"   );
  AddALU(11, IsLow ? ALUImm_SR              : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "OFFI" , "OFFA" , "DOFF"  );
  AddALU( 9, IsLow ? ALUImm_SR              : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "ONI"  , "ONA"  , "DON"   );
  AddALU( 3, IsLow ? ALUImm_SR | ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "ORI"  , "ORA"  , "DOR"   );
  AddALU(14, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "SBI"  , "SBB"  , "DSBB"  );
  AddALU( 6, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "SUINB", "SUBNB", "DSUBNB");
  AddALU(12, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "SUI"  , "SUB"  , "DSUB"  );
  AddALU( 2, IsLow ?             ALUReg_Src : ALUImm_SR | ALUReg_Src | ALUReg_Dest, "XRI"  , "XRA"  , "DXR"   );

  AddAbs("CALL", Is781x ? 0x0040 : 0x0044);
  AddAbs("JMP" , 0x0054);
  AddAbs("LBCD", 0x701f);
  AddAbs("LDED", 0x702f);
  AddAbs("LHLD", 0x703f);
  AddAbs("LSPD", 0x700f);
  AddAbs("SBCD", 0x701e);
  AddAbs("SDED", 0x702e);
  AddAbs("SHLD", 0x703e);
  AddAbs("SSPD", 0x700e);

  reg2_orders = (order_t*) calloc(sizeof(*reg2_orders), Reg2OrderCnt); InstrZ = 0;
  AddReg2("DCR" , 0x0050, core_mask_all);
  AddReg2("DIV" , 0x483c, core_mask_7810);
  AddReg2("INR" , 0x0040, core_mask_all);
  AddReg2("MUL" , 0x482c, core_mask_7810);
  if (Is781x)
  {
    AddReg2("RLL" , 0x4834, core_mask_7810);
    AddReg2("RLR" , 0x4830, core_mask_7810);
  }
  else
  {
    AddA("RLL", 0x4830);
    AddA("RLR", 0x4831);
  }
  AddReg2("SLL" , 0x4824, core_mask_7810);
  AddReg2("SLR" , 0x4820, core_mask_7810);
  AddReg2("SLLC", 0x4804, core_mask_7810);
  AddReg2("SLRC", 0x4800, core_mask_7810);

  AddWork("DCRW", 0x30);
  AddWork("INRW", 0x20);
  AddWork("LDAW", Is781x ? 0x01 : 0x28);
  AddWork("STAW", Is781x ? 0x63 : 0x38);

  AddEA("DRLL", 0x48b4); AddEA("DRLR", 0x48b0);
  AddEA("DSLL", 0x48a4); AddEA("DSLR", 0x48a0);

  if (!Is781x)
  {
    AddInstTable(InstTable, "IN" , 0x4c, DecodeIN_OUT);
    AddInstTable(InstTable, "OUT", 0x4d, DecodeIN_OUT);
  }
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  free(s_regs);
  free(int_flags);
  free(fixed_orders);
  free(reg2_orders);
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

static void SwitchTo_78C10(void *pUser)
{
  PFamilyDescr pDescr = FindFamilyByName("78(C)xx");
  pCurrCPUProps = (const tCPUProps*)pUser;
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = pDescr->Id; NOPCode = 0x00;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  if (pCurrCPUProps->Flags & eFlagHasV)
  {
    static ASSUMERec ASSUME78C10s[] =
    {
      {"V" , &WorkArea, 0, 0xff, 0x100, NULL}
    };

    pASSUMERecs = ASSUME78C10s;
    ASSUMERecCnt = sizeof(ASSUME78C10s) / sizeof(*ASSUME78C10s);
  }
  else
    WorkArea = 0xff;

  MakeCode = MakeCode_78C10; IsDef = IsDef_78C10;
  SwitchFrom = SwitchFrom_78C10;
  InitFields();
}

static const tCPUProps CPUProps[] =
{
  { "7800" , eCore7800High, eFlagHasV             }, /* ROMless , 128B RAM */
  { "7801" , eCore7800High, eFlagHasV             }, /* 4KB ROM , 128B RAM */
  { "7802" , eCore7800High, eFlagHasV             }, /* 6KB ROM , 128B RAM */
  { "78C05", eCore7800Low,  0                     }, /* ROMless , 128B RAM */
  { "78C06", eCore7800Low,  0                     }, /* 4KB ROM , 128B RAM */
  { "7810" , eCore7810,     eFlagHasV             }, /* ROMless , 256B RAM */
  { "78C10", eCore7810,     eFlagHasV | eFlagCMOS }, /* ROMless , 256B RAM */
  { "78C11", eCore7810,     eFlagHasV | eFlagCMOS }, /* 4KB ROM , 256B RAM */
  { "78C12", eCore7810,     eFlagHasV | eFlagCMOS }, /* 8KB ROM , 256B RAM */
  { "78C14", eCore7810,     eFlagHasV | eFlagCMOS }, /* 16KB ROM, 256B RAM */
  { "78C17", eCore7810,     eFlagHasV | eFlagCMOS }, /* ROMless ,  1KB RAM */
  { "78C18", eCore7810,     eFlagHasV | eFlagCMOS }, /* 32KB ROM,  1KB RAM */
  { ""     , eCoreNone,     0                     },
};

void code78c10_init(void)
{
  const tCPUProps *pProp;

  for (pProp = CPUProps; pProp->Name[0]; pProp++)
    (void)AddCPUUserWithArgs(pProp->Name, SwitchTo_78C10, (void*)pProp, NULL, NULL);

  AddInitPassProc(InitCode_78C10);
}
