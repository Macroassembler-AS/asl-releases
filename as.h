#ifndef _AS_H
#define _AS_H
/* as.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Hauptmodul                                                                */
/*                                                                           */
/*****************************************************************************/

extern char *GetErrorPos(void);

extern void as_rebuild_main_inst_tables(void);

extern void WriteCode(void);

#endif /* _AS_H */
