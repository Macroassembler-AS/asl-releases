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

#define ORDER_ARRAY_INCR 16

#define order_array_rsv_end(orders, decl_type) \
  do { \
    if (!(InstrZ % ORDER_ARRAY_INCR)) { \
      size_t s = sizeof(*orders) * (InstrZ + ORDER_ARRAY_INCR); \
      orders = (decl_type*) (orders ? realloc(orders, s) : malloc(s)); \
      memset(&orders[InstrZ], 0, sizeof(*orders) * ORDER_ARRAY_INCR); \
    } \
  } while (0)

#define order_array_free(orders) \
  do { \
    free(orders); \
    orders = NULL; \
  } while (0)

extern int InstrZ;
extern int AdrCnt;

extern PInstTable InstTable;

#endif /* _CODEVARS_H */
