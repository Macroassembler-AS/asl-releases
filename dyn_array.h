#ifndef _DYN_ARRAY_H
#define _DYN_ARRAY_H
/* codevars.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Minimalistic Dynamic Array Handling                                       */
/*                                                                           */
/*****************************************************************************/

#define DYN_ARRAY_INCR 16

#define dyn_array_roundup(n) ((((n) + (DYN_ARRAY_INCR - 1)) / DYN_ARRAY_INCR) * DYN_ARRAY_INCR)

#define dyn_array_realloc(array, decl_type, old_cnt, new_cnt) \
  do { \
    size_t new_roundup_cnt = dyn_array_roundup(new_cnt); \
    size_t old_roundup_cnt = dyn_array_roundup(old_cnt); \
    if (new_roundup_cnt != old_roundup_cnt) { \
      size_t s = sizeof(*array) * new_roundup_cnt; \
      array = (decl_type*) (array ? realloc(array, s) : malloc(s)); \
      if (new_roundup_cnt > old_roundup_cnt) \
        memset(&array[old_roundup_cnt], 0, sizeof(*array) * (new_roundup_cnt - old_roundup_cnt)); \
      else if (new_roundup_cnt > (size_t)(new_cnt)) \
        memset(&array[new_cnt], 0, sizeof(*array) * (new_roundup_cnt - (new_cnt))); \
    } \
  } while (0)

#define dyn_array_rsv_end(array, decl_type, curr_cnt) \
        dyn_array_realloc(array, decl_type, curr_cnt, (curr_cnt) + 1)

#define order_array_rsv_end(orders, decl_type) \
        dyn_array_rsv_end(orders, decl_type, InstrZ)

#endif /* _DYN_ARRAY_H */
