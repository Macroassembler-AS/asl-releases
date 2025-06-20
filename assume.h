#ifndef _ASSUME_H
#define _ASSUME_H
/* assume.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* ASSUME settings keeper                                                    */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>
#include "datatypes.h"

typedef struct as_assume_rec
{
  const char *p_name;
  LongInt *p_dest;
  LongInt min_value, max_value;
  LongInt nothing_value;
  void (*p_post_proc)(void);
} as_assume_rec_t;

extern void assume_set(const as_assume_rec_t *p_recs, size_t rec_cnt);

extern const as_assume_rec_t *assume_lookup(const char *p_name);

extern void assume_append(as_assume_rec_t **pp_recs, size_t *p_rec_cnt,
                          const char *p_name, LongInt *p_dest,
                          LongInt min_value, LongInt max_value, LongInt nothing_value,
                          void (*p_post_proc)(void));

extern void assume_dump(const as_assume_rec_t *p_recs, size_t rec_cnt);

#endif /* _ASSUME_H */
