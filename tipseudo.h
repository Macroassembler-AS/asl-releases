#ifndef _TIPSEUDO_H
#define _TIPSEUDO_H
/* tipseudo.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Haeufiger benutzte Texas Instruments Pseudo-Befehle                       */
/*                                                                           */
/*****************************************************************************/

#include "bpemu.h"

/*****************************************************************************
 * Global Functions
 *****************************************************************************/

struct sInstTable;

extern Boolean DecodeTIPseudo(void);

extern Boolean IsTIDef(void);

extern void AddTI34xPseudo(struct sInstTable *pInstTable);

#endif /* _TIPSEUDO_H */
