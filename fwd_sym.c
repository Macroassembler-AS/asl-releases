/* fwd_sysm.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Forward Symbol Definitions                                                */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "fwd_sym.h"

/*!------------------------------------------------------------------------
 * \fn     as_fwd_sym_create(void)
 * \brief  create forward symbol entry
 * \return * to created entry
 * ------------------------------------------------------------------------ */

as_fwd_sym_t *as_fwd_sym_create(void)
{
  as_fwd_sym_t *p_ret;

  p_ret = (as_fwd_sym_t*)malloc(sizeof(*p_ret));
  if (p_ret)
  {
    p_ret->p_next = NULL;
    p_ret->p_name = NULL;
    p_ret->dest_section = 0;
    p_ret->p_error_pos = NULL;
  }
  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_fwd_sym_free(as_fwd_sym_t *p_symbol)
 * \brief  free entry from forward symbol list
 * \param  p_symbol entry to free
 * ------------------------------------------------------------------------ */

void as_fwd_sym_free(as_fwd_sym_t *p_symbol)
{
  free(p_symbol->p_name); p_symbol->p_name = NULL;
  free(p_symbol->p_error_pos); p_symbol->p_error_pos = NULL;
  free(p_symbol);
}

/*!------------------------------------------------------------------------
 * \fn     as_fwd_sym_search(as_fwd_sym_t *p_root, const char *p_name)
 * \brief  search forward symbol in list
 * \param  p_root list to search
 * \param  p_name symbol name to look up
 * \return * to found entry in list or NULL
 * ------------------------------------------------------------------------ */

as_fwd_sym_t *as_fwd_sym_search(as_fwd_sym_t *p_root, const char *p_name)
{
  while (p_root && strcmp(p_root->p_name, p_name))
    p_root = p_root->p_next;
  return p_root;
}

/*!------------------------------------------------------------------------
 * \fn     as_fwd_sym_search_and_move_dest_section(as_fwd_sym_t **pp_root, const char *p_name, LongInt *p_dest_section)
 * \brief  search forward symbol in list, and if found, delete and return its dest section
 * \param  pp_root list to search
 * \param  p_name symbol name to look up
 * \param  p_dest_section extracted section if found
 * \return True if found entry
 * ------------------------------------------------------------------------ */

Boolean as_fwd_sym_search_and_move_dest_section(as_fwd_sym_t **pp_root, const char *p_name, LongInt *p_dest_section)
{
  as_fwd_sym_t *p_run, *p_prev;

  for (p_run = *pp_root, p_prev = NULL;
       p_run;
       p_prev = p_run, p_run = p_run->p_next)
    if (!strcmp(p_run->p_name, p_name))
    {
      *p_dest_section = p_run->dest_section;
      if (!p_prev)
        *pp_root = p_run->p_next;
      else
        p_prev->p_next = p_run->p_next;
      as_fwd_sym_free(p_run);
      return True;
    }
  return False;
}

