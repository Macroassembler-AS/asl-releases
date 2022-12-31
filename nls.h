#ifndef _NLS_H
#define _NLS_H
/* nls.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Abhandlung landesspezifischer Unterschiede                                */
/*                                                                           */
/* Historie: 16. 5.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>

#include "chardefs.h"

typedef enum
{
  TimeFormatUSA,
  TimeFormatEurope,
  TimeFormatJapan
} TimeFormat;

typedef enum
{
  DateFormatMTY,
  DateFormatTMY,
  DateFormatYMT
} DateFormat;

typedef enum
{
  CurrFormatPreNoBlank,
  CurrFormatPostNoBlank,
  CurrFormatPreBlank,
  CurrFormatPostBlank
} CurrFormat;

typedef char CharTable[256];

extern CharTable UpCaseTable, LowCaseTable;


extern Boolean NLS_Initialize(int *argc, char **argv);

extern Word NLS_GetCountryCode(void);

extern tCodepage NLS_GetCodepage(void);

extern void NLS_DateString(Word Year, Word Month, Word Day, char *Dest, size_t DestSize);

extern void NLS_CurrDateString(char *Dest, size_t DestSize);

extern void NLS_TimeString(Word Hour, Word Minute, Word Second, Word Sec100, char *Dest, size_t DestSize);

extern void NLS_CurrTimeString(Boolean Use100, char *Dest, size_t DestSize);

extern void NLS_CurrencyString(double inp, char *erg, size_t DestSize);

extern char Upcase(char inp);

extern void NLS_UpString(char *s);
extern void NLS_UpString2(char *p_dest, size_t dest_size, const char *p_src, size_t src_len);

extern void NLS_LowString(char *s);

extern int NLS_StrCmp(const char *s1, const char *s2);


extern void nls_init(void);
#endif /* _NLS_H */
