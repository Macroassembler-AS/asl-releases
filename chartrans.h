#ifndef _CHARTRANS_H
#define _CHARTRANS_H
/* chartrans.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Character Translation                                                     */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>

struct as_nonz_dynstr;
typedef unsigned char *as_chartrans_table_t;

extern as_chartrans_table_t as_chartrans_table_new(void);

extern void as_chartrans_table_free(as_chartrans_table_t p_table);

extern as_chartrans_table_t as_chartrans_table_dup(const as_chartrans_table_t p_table);

extern void as_chartrans_table_reset(as_chartrans_table_t p_table);

extern int as_chartrans_table_set(as_chartrans_table_t p_table, unsigned input_code, unsigned output_code);

extern int as_chartrans_table_set_mult(as_chartrans_table_t p_table, unsigned input_code_from, unsigned input_code_to, unsigned output_code);

extern void as_chartrans_table_print(as_chartrans_table_t p_table, FILE *p_file);

extern int as_chartrans_xlate(const as_chartrans_table_t p_table, unsigned src);

extern int as_chartrans_xlate_rev(const as_chartrans_table_t p_table, unsigned src);

extern int as_chartrans_xlate_next(const as_chartrans_table_t p_table, unsigned *p_dest, const char **pp_src, size_t *p_src_len);

extern int as_chartrans_xlate_nonz_dynstr(const as_chartrans_table_t p_table, struct as_nonz_dynstr *p_str);

#define foreach_as_chartrans(p_table, str, dest) \
{ \
  const char *p_run = (str).p_str; \
  size_t run_len = (str).len; \
  while (!as_chartrans_xlate_next(p_table, &dest, &p_run, &run_len))
#define foreach_as_chartrans_end }

#endif /* _CHARTRANS_H */
