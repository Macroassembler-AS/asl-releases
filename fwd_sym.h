#ifndef _FWD_SYM_H
#define _FWD_SYM_H
/* fwd_sysm.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Forward Symbol Definitions                                                */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef struct as_fwd_sym
{
  struct as_fwd_sym *p_next;
  char *p_name;
  LongInt dest_section;
  char *p_error_pos;
} as_fwd_sym_t;

extern as_fwd_sym_t *as_fwd_sym_create(void);
extern void as_fwd_sym_free(as_fwd_sym_t *p_symbol);
extern as_fwd_sym_t *as_fwd_sym_search(as_fwd_sym_t *p_root, const char *p_name);
extern Boolean as_fwd_sym_search_and_move_dest_section(as_fwd_sym_t **pp_root, const char *p_name, LongInt *p_dest_section);

#endif /* FWD_SYM_H */
