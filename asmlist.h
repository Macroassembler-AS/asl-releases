#ifndef _ASMLIST_H
#define _ASMLIST_H
/* asmlist.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS Port                                                                   */
/*                                                                           */
/* Generate Listing                                                          */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef enum
{
  e_liston_off = 0,
  e_liston_on = 1,
  e_liston_noskipped = 2,
  e_liston_purecode = 3
} as_liston_t;

extern as_liston_t ListOn;

extern void MakeList(const char *pSrcLine);

extern void as_list_set_max_pc(LargeWord max_pc);

extern Boolean as_list_liston_to_enum(const char *p_arg, as_liston_t *p_liston);

extern void asmlist_setup(void);

extern void asmlist_init(void);

#endif /* _ASMLIST_H */
