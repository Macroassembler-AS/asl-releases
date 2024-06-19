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

extern PInstTable InstTable;

#endif /* _CODEVARS_H */
