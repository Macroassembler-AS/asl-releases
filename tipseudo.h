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

extern Boolean decode_ti_qxx(void);

extern Boolean DecodeTIPseudo(void);

extern Boolean IsTIDef(void);

extern void add_ti_pseudo(struct sInstTable *p_inst_table);

extern void AddTI34xPseudo(struct sInstTable *pInstTable);

#endif /* _TIPSEUDO_H */
