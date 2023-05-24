/* onoff_common.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* ON/OFF flags used by several targets                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "cmdarg.h"
#include "onoff_common.h"

Boolean FPUAvail,    /* floating point co processor instructions allowed? */
        PMMUAvail,   /* PMMU instructions allowed? */
        SupAllowed,  /* supervisor mode enabled */
        MaxMode,     /* CPU in maximum mode */
        CompMode,    /* Enable Compatibility Mode */
        TargetBigEndian, /* Data storage Big Endian? */
        DoPadding,   /* align to even address? */
        Packing;     /* packed data storage? */
unsigned registered;

#define SupAllowedCmdName "SUPMODE"   /* Privileged instructions allowed */
#define SupAllowedSymName "INSUPMODE"
static Boolean DefSupAllowed;

#define BigEndianCmdName  "BIGENDIAN"
#define BigEndianSymName  "BIGENDIAN"  /* Data storage MSB first */
static Boolean DefBigEndian;

#define CompModeCmdName   "COMPMODE"   /* Compatibility Mode */
#define CompModeSymName   "COMPMODE"
static Boolean DefCompMode;

#define PackingCmdName    "PACKING"
#define PackingSymName    "PACKING"
static Boolean DefPacking, DefPackingSet;

/*!------------------------------------------------------------------------
 * \fn     onoff_test_and_set(unsigned mask)
 * \brief  check whether on/off insn is registered first time during pass
 * \param  mask insn to test
 * \return mask if first time, otherwise 0
 * ------------------------------------------------------------------------ */

unsigned onoff_test_and_set(unsigned mask)
{
  unsigned curr = registered;
  registered |= mask;
  return curr & mask;
}

/*!------------------------------------------------------------------------
 * \fn     onoff_fpu_add(void)
 * \brief  register on/off command to enable/disable FPU instruction extensions
 * ------------------------------------------------------------------------ */

void onoff_fpu_add(void)
{
  if (!onoff_test_and_set(e_onoff_reg_fpu))
    SetFlag(&FPUAvail, FPUAvailName, False);
  AddONOFF("FPU", &FPUAvail, FPUAvailName, False);
}

/*!------------------------------------------------------------------------
 * \fn     onoff_pmmu_add(void)
 * \brief  register on/off command to enable/disable PMMU instruction extensions
 * ------------------------------------------------------------------------ */

void onoff_pmmu_add(void)
{
  if (!onoff_test_and_set(e_onoff_reg_pmmu))
    SetFlag(&PMMUAvail, PMMUAvailName, False);
  AddONOFF("PMMU", &PMMUAvail, PMMUAvailName, False);
}

/*!------------------------------------------------------------------------
 * \fn     onoff_supmode_add(void)
 * \brief  register on/off command to enable/disable supervisor mode-only instructions
 * ------------------------------------------------------------------------ */

void onoff_supmode_add(void)
{
  if (!onoff_test_and_set(e_onoff_reg_supmode))
    SetFlag(&SupAllowed, SupAllowedSymName, DefSupAllowed);
  AddONOFF(SupAllowedCmdName, &SupAllowed, SupAllowedSymName, False);
}

/*!------------------------------------------------------------------------
 * \fn     onoff_maxmode_add(void)
 * \brief  register on/off command to enable/disable maximum mode-only instructions
 * ------------------------------------------------------------------------ */

void onoff_maxmode_add(void)
{
  if (!onoff_test_and_set(e_onoff_reg_maxmode))
    SetFlag(&MaxMode, MaxModeSymName, False);
  AddONOFF(MaxModeCmdName, &MaxMode, MaxModeSymName, False);
}

/*!------------------------------------------------------------------------
 * \fn     onoff_compmode_add(void)
 * \brief  register on/off command to enable/disable compatibility mode
 * ------------------------------------------------------------------------ */

void onoff_compmode_add(void)
{
  if (!onoff_test_and_set(e_onoff_reg_compmode))
    SetFlag(&CompMode, CompModeSymName, DefCompMode);
  AddONOFF(CompModeCmdName, &CompMode, CompModeSymName, False);
}

/*!------------------------------------------------------------------------
 * \fn     onoff_bigendian_add(void)
 * \brief  register on/off command to set big/little endian data storage
 * ------------------------------------------------------------------------ */

void onoff_bigendian_add(void)
{
  if (!onoff_test_and_set(e_onoff_reg_bigendian))
    SetFlag(&TargetBigEndian, BigEndianSymName, DefBigEndian);
  AddONOFF(BigEndianCmdName, &TargetBigEndian, BigEndianSymName, False);
}

/*!------------------------------------------------------------------------
 * \fn     onoff_packing_add(Boolean def_value)
 * \brief  register on/off command to set packed data storage
 * \param  def_value to set as default
 * ------------------------------------------------------------------------ */

void onoff_packing_add(Boolean def_value)
{
  if (!onoff_test_and_set(e_onoff_reg_packing))
    SetFlag(&Packing, PackingSymName, DefPackingSet ? DefPacking : def_value);
  AddONOFF(PackingCmdName, &Packing, PackingSymName, False);
}

/*!------------------------------------------------------------------------
 * \fn     initpass_onoff(void)
 * \brief  per-pass initialization of module
 * ------------------------------------------------------------------------ */

static void initpass_onoff(void)
{
  registered = 0;
}

/*!------------------------------------------------------------------------
 * \fn     onoff_common_init(void)
 * \brief  module initialization
 * ------------------------------------------------------------------------ */

static as_cmd_result_t CMD_SupAllowed(Boolean Negate, const char *pArg)
{
  UNUSED(pArg);

  DefSupAllowed = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_BigEndian(Boolean Negate, const char *pArg)
{
  UNUSED(pArg);

  DefBigEndian = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_CompMode(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  DefCompMode = !Negate;
  return e_cmd_ok;
}

static as_cmd_result_t CMD_Packing(Boolean Negate, const char *Arg)
{
  UNUSED(Arg);

  DefPacking = !Negate;
  DefPackingSet = True;
  return e_cmd_ok;
}

static const as_cmd_rec_t onoff_params[] =
{
  { BigEndianCmdName  , CMD_BigEndian       },
  { CompModeCmdName   , CMD_CompMode        },
  { PackingCmdName    , CMD_Packing         },
  { SupAllowedCmdName , CMD_SupAllowed      }
};

extern void onoff_common_init(void)
{
  as_cmd_register(onoff_params, as_array_size(onoff_params));
  DefSupAllowed = False;
  DefBigEndian = False;
  DefCompMode = False;
  DefPacking = DefPackingSet = False;
  AddInitPassProc(initpass_onoff);
}
