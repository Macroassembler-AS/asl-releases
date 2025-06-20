/* assume.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* ASSUME settings keeper                                                    */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "strutil.h"
#include "dyn_array.h"
#include "assume.h"

static size_t assume_rec_cnt;
static const as_assume_rec_t *p_assume_recs;

/*!------------------------------------------------------------------------
 * \fn     assume_set(const as_assume_rec_t *p_recs, size_t rec_cnt)
 * \brief  set new list of assume records
 * \param  p_recs list
 * \param  rec_cnt length of list
 * ------------------------------------------------------------------------ */

void assume_set(const as_assume_rec_t *p_recs, size_t rec_cnt)
{
  p_assume_recs = p_recs;
  assume_rec_cnt = rec_cnt;
}

/*!------------------------------------------------------------------------
 * \fn     assume_lookup(const char *p_name)
 * \brief  search for ASSUME variable/record
 * \param  p_name name of variable
 * \return * to record or NULL if not found
 * ------------------------------------------------------------------------ */

const as_assume_rec_t *assume_lookup(const char *p_name)
{
  size_t z;

  for (z = 0; z < assume_rec_cnt; z++)
    if (!as_strcasecmp(p_name, p_assume_recs[z].p_name))
      return &p_assume_recs[z];
  return NULL;
}

/*!------------------------------------------------------------------------
 * \fn     assume_append(as_assume_rec_t **pp_recs, size_t *p_rec_cnt,
                         const char *p_name, LongInt *p_dest,
                         LongInt min_value, LongInt max_value, LongInt nothing_value,
                         void (*p_post_proc)(void))
 * \brief  extend dynamic array of assume records
 * ------------------------------------------------------------------------ */

void assume_append(as_assume_rec_t **pp_recs, size_t *p_rec_cnt,
                   const char *p_name, LongInt *p_dest,
                   LongInt min_value, LongInt max_value, LongInt nothing_value,
                   void (*p_post_proc)(void))
{
  as_assume_rec_t *p_rec;

  dyn_array_rsv_end((*pp_recs), as_assume_rec_t, (*p_rec_cnt));
  p_rec = &(*pp_recs)[(*p_rec_cnt)++];
  p_rec->p_name = as_strdup(p_name);
  p_rec->p_dest = p_dest;
  p_rec->min_value = min_value;
  p_rec->max_value = max_value;
  p_rec->nothing_value = nothing_value;
  p_rec->p_post_proc = p_post_proc;
}

/*!------------------------------------------------------------------------
 * \fn     assume_dump(const as_assume_rec_t *p_recs, size_t rec_cnt)
 * \brief  dump list of records
 * \param  p_recs list
 * \param  rec_cnt length of list
 * ------------------------------------------------------------------------ */

void assume_dump(const as_assume_rec_t *p_recs, size_t rec_cnt)
{
  for (; rec_cnt; p_recs++, rec_cnt--)
    printf("%s\n", p_recs->p_name);
}
