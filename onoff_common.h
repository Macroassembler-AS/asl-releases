#ifndef _ONOFF_COMMON_H
#define _ONOFF_COMMON_H
/* onoff_common.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* ON/OFF flags used by several targets                                      */
/*                                                                           */
/*****************************************************************************/

#define FPUAvailName     "HASFPU"     /* FPU-Befehle erlaubt */
#define PMMUAvailName    "HASPMMU"    /* PMMU-Befehle erlaubt */
#define FullPMMUName     "FULLPMMU"
#define MaxModeSymName    "INMAXMODE"  /* CPU in maximum mode */
#define MaxModeCmdName    "MAXMODE"
#define SrcModeCmdName    "SRCMODE"
#define SrcModeSymName    "INSRCMODE"  /* CPU im Quellcode-kompatiblen Modus */
#define ExtModeCmdName    "EXTMODE"
#define ExtModeSymName    "INEXTMODE"
#define LWordModeCmdName  "LWORDMODE"
#define LWordModeSymName  "INLWORDMODE"
#define DSPCmdName        "DSP"
#define DSPSymName        "HASDSP"
#define CustomAvailCmdName "CUSTOM"
#define CustomAvailSymName "CUSTOM"
#define BranchExtCmdName  "BRANCHEXT"
#define BranchExtSymName  "BRANCHEXT"

extern Boolean FPUAvail,
               PMMUAvail,
               SupAllowed,
               MaxMode,
               CompMode,
               TargetBigEndian,
               DoPadding,
               Packing;

/* NOTE: will have to switch this to #define as soon as
   everything up to 2**15 is used up - 16 bit compilers
   silently limit enums to int range */

enum
{
  e_onoff_reg_fpu = 1 << 0,
  e_onoff_reg_pmmu = 1 << 1,
  e_onoff_reg_custom = 1 << 2,
  e_onoff_reg_supmode = 1 << 3,
  e_onoff_reg_maxmode = 1 << 4,
  e_onoff_reg_extmode = 1 << 5,
  e_onoff_reg_srcmode = 1 << 6,
  e_onoff_reg_compmode = 1 << 7,
  e_onoff_reg_lwordmode = 1 << 8,
  e_onoff_reg_fullpmmu = 1 << 9,
  e_onoff_reg_dsp = 1 << 10,
  e_onoff_reg_packing = 1 << 11,
  e_onoff_reg_bigendian = 1 << 12,
  e_onoff_reg_branchext = 1 << 13,
  e_onoff_reg_z80syntax = 1 << 14
};

extern unsigned onoff_test_and_set(unsigned mask);

extern void onoff_fpu_add(void);
extern void onoff_pmmu_add(void);
extern void onoff_supmode_add(void);
extern void onoff_maxmode_add(void);
extern void onoff_compmode_add(void);
extern void onoff_bigendian_add(void);
extern void onoff_packing_add(Boolean def_value);

extern void onoff_common_init(void);

#endif /* _ONOFF_COMMON_H */
