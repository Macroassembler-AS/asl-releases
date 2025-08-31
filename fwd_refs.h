#ifndef _FWD_REFS_H
#define _FWD_REFS_H
/* fwd_refs.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Store and resolve symbol forward references for the purpose of symbol     */
/* usage tracking                                                            */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

extern void as_forward_ref_add(const char *p_expanded_name);

extern Boolean as_forward_ref_search_and_mark(const char *p_name, LongInt attribute);

extern void as_forward_ref_init(void);

#endif /* _FWD_REFS_H */
