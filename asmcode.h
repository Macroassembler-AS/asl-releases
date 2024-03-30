#ifndef _ASMCODE_H
#define _ASMCODE_H
/* asmcode.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung der Code-Datei                                                 */
/*                                                                           */
/* Historie: 18. 5.1996 Grundsteinlegung                                     */
/*           19. 1.2000 Patchliste angelegt                                  */
/*           26. 6.2000 added exports                                        */
/*                                                                           */
/*****************************************************************************/

extern LongInt SectSymbolCounter;

extern PPatchEntry PatchList, PatchLast;
extern PExportEntry ExportList, ExportLast;

extern void DreheCodes(void);

extern void NewRecord(LargeWord NStart);

extern void OpenFile(void);

extern void CloseFile(void);

extern void WriteBytes(void);

extern void RetractWords(Word Cnt);

extern void InsertPadding(unsigned NumBytes, Boolean OnlyReserve);

extern void asmcode_init(void);

#endif /* _ASMCODE_H */
