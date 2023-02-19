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
struct sStrComp;

typedef struct as_chartrans_table
{
  unsigned char mapped[256 / 8];
  unsigned char mapping[256];
} as_chartrans_table_t;

extern as_chartrans_table_t *as_chartrans_table_new(void);

extern void as_chartrans_table_free(as_chartrans_table_t *p_table);

extern as_chartrans_table_t *as_chartrans_table_dup(const as_chartrans_table_t *p_table);

extern void as_chartrans_table_reset(as_chartrans_table_t *p_table);

extern int as_chartrans_table_unset(as_chartrans_table_t *p_table, unsigned input_code);
extern int as_chartrans_table_set(as_chartrans_table_t *p_table, unsigned input_code, unsigned output_code);

extern int as_chartrans_table_unset_mult(as_chartrans_table_t *p_table, unsigned input_code_from, unsigned input_code_to);
extern int as_chartrans_table_set_mult(as_chartrans_table_t *p_table, unsigned input_code_from, unsigned input_code_to, unsigned output_code);

extern void as_chartrans_table_print(const as_chartrans_table_t *p_table, FILE *p_file);

extern int as_chartrans_xlate(const as_chartrans_table_t *p_table, unsigned src) /*__attribute__((warn_unused_result))*/;

extern int as_chartrans_xlate_rev(const as_chartrans_table_t *p_table, unsigned src) /*__attribute__((warn_unused_result))*/;

extern int as_chartrans_xlate_next(const as_chartrans_table_t *p_table, unsigned *p_dest, const char **pp_src, size_t *p_src_len) /*__attribute__((warn_unused_result))*/;

extern int as_chartrans_xlate_nonz_dynstr(const as_chartrans_table_t *p_table, struct as_nonz_dynstr *p_str, const struct sStrComp *p_src_arg) /*__attribute__((warn_unused_result))*/;

#endif /* _CHARTRANS_H */
