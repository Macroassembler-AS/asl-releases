/* dynstr_nls.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Handling of strings with dynamic allocation - NLS extensions              */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "dynstr.h"
#include "nls.h"
#include "dynstr_nls.h"

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_append_upr(as_dynstr_t *p_dest, const char *p_src, size_t src_len)
 * \brief  extend string and convert to upper case
 * \param  p_dest string to extend
 * \param  p_src what to append
 * \param  src_len how much to append
 * \return actual # of bytes transferred
 * ------------------------------------------------------------------------ */

size_t as_dynstr_append_upr(as_dynstr_t *p_dest, const char *p_src, size_t src_len)
{
  size_t dest_len = strlen(p_dest->p_str), dest_cap;

  if ((dest_len + src_len + 1 > p_dest->capacity) && p_dest->dynamic)
    as_dynstr_realloc(p_dest, as_dynstr_roundup_len(dest_len + src_len));
  dest_cap = p_dest->capacity - dest_len;
  if (src_len >= dest_cap)
    src_len = dest_cap - 1;
  NLS_UpString2(p_dest->p_str + dest_len, dest_cap, p_src, src_len);
  return src_len;
}

/*!------------------------------------------------------------------------
 * \fn     as_dynstr_append_c_str_upr(as_dynstr_t *p_dest, const char *p_src)
 * \brief  extend string and convert to upper case
 * \param  p_dest string to extend
 * \param  p_src what to append
 * \return actual # of bytes transferred
 * ------------------------------------------------------------------------ */

size_t as_dynstr_append_c_str_upr(as_dynstr_t *p_dest, const char *p_src)
{
  return as_dynstr_append_upr(p_dest, p_src, strlen(p_src));
}
