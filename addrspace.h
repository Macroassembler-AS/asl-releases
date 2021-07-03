#ifndef _ADDRSPACE_H
#define _ADDRSPACE_H
/* addrspace.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Address Space enumeration                                                 */
/*                                                                           */
/*****************************************************************************/

/* NOTE: these constants are used in the .P files, so DO NOT CHANGE existing
   enums; only attach new enums at the end! */

typedef enum
{
  SegNone  = 0,
  SegCode  = 1,
  SegData  = 2,
  SegIData = 3,
  SegXData = 4,
  SegYData = 5,
  SegBData = 6,
  SegIO    = 7,
  SegReg   = 8,
  SegRData = 9,
  SegEEData = 10,
  SegCount = 11,
  StructSeg = SegCount,
  SegCountPlusStruct = 12
} as_addrspace_t;

#ifdef __cplusplus
#include "cppops.h"
DefCPPOps_Enum(as_addrspace_t)
#endif

extern const char *SegNames[SegCountPlusStruct];
extern char SegShorts[SegCountPlusStruct];

extern as_addrspace_t addrspace_lookup(const char *p_name);

#endif /* _ADDRSPACE_H */
