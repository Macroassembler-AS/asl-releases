#ifndef _CPU2PHYS_H
#define _CPU2PHYS_H
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* CPU->Physical Address Translation                                         */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include "datatypes.h"
#include "addrspace.h"

struct sTempResult;

extern void cpu_2_phys_area_clear(as_addrspace_t addr_space);
extern void cpu_2_phys_area_add(as_addrspace_t addr_space, LargeWord cpu_start, LargeWord phys_start, LargeWord len);
extern void cpu_2_phys_area_set_cpu_end(as_addrspace_t addr_space, LargeWord cpu_end);
extern void cpu_2_phys_area_fill(as_addrspace_t addr_space, LargeWord cpu_start, LargeWord cpu_end);
extern void cpu_2_phys_area_dump(as_addrspace_t addr_space, FILE *p_file);

extern Boolean def_phys_2_cpu(as_addrspace_t addr_space, LargeWord *p_address);
extern Boolean def_cpu_2_phys(as_addrspace_t addr_space, LargeWord *p_address);

extern Boolean fnc_phys_2_cpu(struct sTempResult *p_ret, const struct sTempResult *p_args, unsigned arg_cnt);
extern Boolean fnc_cpu_2_phys(struct sTempResult *p_ret, const struct sTempResult *p_args, unsigned arg_cnt);

#endif /* _CPU2PHYS_H */
