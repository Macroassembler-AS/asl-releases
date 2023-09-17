#ifndef _IOERRS_H
#define _IOERRS_H
/* ioerrs.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Abliefern der I/O-Fehlermeldungen                                         */
/*                                                                           */
/* Historie: 11.10.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

extern const char *GetErrorMsg(int number);

extern const char *GetReadErrorMsg(void);

extern void ioerrs_init(char *ProgPath);
#endif /* _IOERRS_H */
