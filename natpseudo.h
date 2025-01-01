#ifndef _NATPSEUDO_H
#define _NATPSEUDO_H
/* natpseudo.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Pseudo Instructions commonly used on National targets                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

struct sInstTable;

extern void AddNatPseudo(struct sInstTable *InstTable);

#endif /* _NATPSEUDO_H */
