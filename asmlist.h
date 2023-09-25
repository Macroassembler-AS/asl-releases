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

extern void MakeList(const char *pSrcLine);

extern void as_list_set_max_pc(LargeWord max_pc);

extern void asmlist_init(void);

#endif /* _ASMLIST_H */
