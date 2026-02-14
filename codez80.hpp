#ifndef _CODEZ80_HPP
#define _CODEZ80_HPP
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

/* Declare this in a header file so CLang++ does not complain
   about unused inline functions: */

# include "cppops.h"
DefCPPOps_Enum(tOpPrefix)

DefCPPOps_Mask(cpu_core_mask_t)
DefCPPOps_Mask(cpu_core_flags_t)

#endif /* _CODEZ80_HPP */
