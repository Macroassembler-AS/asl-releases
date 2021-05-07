#ifndef _ASMRELOCS_H
#define _ASMRELOCS_H
/* asmrelocs.h */
/****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                    */
/*                                                                          */
/* AS-Portierung                                                            */
/*                                                                          */
/* Verwaltung von Relokationslisten                                         */
/*                                                                          */
/* Historie: 25. 7.1999 Grundsteinlegung                                    */
/*            8. 8.1999 Reloc-Liste gespeichert                             */
/*           19. 1.2000 TransferRelocs begonnen                             */
/*           26. 6.2000 added exports                                       */
/*                                                                          */
/****************************************************************************/

struct sRelocEntry
{
  struct sRelocEntry *Next;
  char *Ref;
  Byte Add;
};
typedef struct sRelocEntry TRelocEntry, *PRelocEntry;


extern PRelocEntry LastRelocs;

extern PRelocEntry MergeRelocs(PRelocEntry *list1, PRelocEntry *list2,
                                Boolean Add);

extern void InvertRelocs(PRelocEntry *erg, PRelocEntry *src);

extern void FreeRelocs(PRelocEntry *list);

extern PRelocEntry DupRelocs(PRelocEntry src);

extern void SetRelocs(PRelocEntry List);

extern void TransferRelocs(LargeWord Addr, LongWord Type);

extern void TransferRelocs2(PRelocEntry RelocList, LargeWord Addr, LongWord Type);

extern void SubPCRefReloc(void);

extern void AddExport(char *Name, LargeInt Value, LongWord Flags);
#endif /* _ASMRELOCS_H */
