#ifndef _STRINGLISTS_H
#define _STRINGLISTS_H
/* stringlists.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung von String-Listen                                              */
/*                                                                           */
/* Historie:  4. 5.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef struct sStringRec
{
  struct sStringRec *Next;
  char *Content;
} StringRec, *StringRecPtr;
typedef StringRecPtr StringList;

extern void InitStringList(StringList *List);

extern void ClearStringEntry(StringRecPtr *Elem);

extern void ClearStringList(StringList *List);

extern void AddStringListFirst(StringList *List, const char *NewStr);

extern void AddStringListLast(StringList *List, const char *NewStr);

extern void RemoveStringList(StringList *List, const char *OldStr);

extern const char *GetStringListFirst(StringList List, StringRecPtr *Lauf);

extern const char *GetStringListNext(StringRecPtr *Lauf);

extern char *MoveAndCutStringListFirst(StringList *p_list);

extern char *MoveAndCutStringListLast(StringList *List);

extern Boolean StringListEmpty(StringList List);

extern unsigned StringListCount(StringList List);

extern StringList DuplicateStringList(StringList Src);

extern Boolean StringListPresent(StringList List, char *Search);

extern void DumpStringList(StringList List);
#endif /* _STRINGLISTS_H */
