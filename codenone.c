/* codenone.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Dummy Target                                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "asmdef.h"
#include "asmsub.h"
#include "errmsg.h"

#include "codenone.h"

LargeWord none_target_seglimit;

/*-------------------------------------------------------------------------*/

static void make_code_none(void)
{
  if (Memo("")) return;

  WrError(ErrNum_NoTarget);
}

static Boolean is_def_none(void)
{
  return False;
}

static void switch_from_none(void)
{
}

static void initpass_none(void)
{
  if (!PassNo)
    none_target_seglimit = 0xfffffffful;
}

static void switch_to_none(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "";
  HeaderID = 0x00;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = none_target_seglimit;

  MakeCode = make_code_none;
  SwitchFrom = switch_from_none;
  IsDef = is_def_none;
}

void codenone_init(void)
{
  (void)AddCPU("NONE", switch_to_none);
  AddInitPassProc(initpass_none);
}
