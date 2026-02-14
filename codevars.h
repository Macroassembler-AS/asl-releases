#ifndef _CODEVARS_H
#define _CODEVARS_H
/* codevars.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Gemeinsame Variablen aller Codegeneratoren                                */
/*                                                                           */
/* Historie: 26.5.1997 Grundsteinlegung                                      */
/*                                                                           */
/*****************************************************************************/

#include "dyn_array.h"

#define order_array_rsv_end(orders, decl_type) \
        dyn_array_rsv_end(orders, decl_type, InstrZ)

#define order_array_free(orders) \
  do { \
    free(orders); \
    orders = NULL; \
  } while (0)

extern int InstrZ;
extern int AdrCnt;

struct sInstTable;
extern struct sInstTable *InstTable,
                         *main_inst_table_no_leading_dot,
                         *main_inst_table_leading_dot;

extern void as_augment_main_inst_tables(const char *p_name, Word arg, InstProc proc, Boolean nodot_occupied);

#endif /* _CODEVARS_H */
