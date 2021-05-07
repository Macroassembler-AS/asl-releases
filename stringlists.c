/* stringlists.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Implementation von String-Listen                                          */
/*                                                                           */
/* Historie:  5. 4.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "strutil.h"
#include "stringlists.h"

void InitStringList(StringList *List)
{
  *List = NULL;
}

void ClearStringEntry(StringRecPtr *Elem)
{
  if ((*Elem)->Content)
    free((*Elem)->Content);
  free(*Elem);
  *Elem = NULL;
}

void ClearStringList(StringList *List)
{
  StringRecPtr Hilf;

  while (*List)
  {
    Hilf = (*List);
    *List = (*List)->Next;
    ClearStringEntry(&Hilf);
  }
}

void AddStringListFirst(StringList *List, const char *NewStr)
{
  StringRecPtr Neu;

  Neu=(StringRecPtr) malloc(sizeof(StringRec));
  Neu->Content = NewStr ? as_strdup(NewStr) : NULL;
  Neu->Next = (*List);
  *List = Neu;
}

void AddStringListLast(StringList *List, const char *NewStr)
{
  StringRecPtr Neu, Lauf;

  Neu = (StringRecPtr) malloc(sizeof(StringRec));
  Neu->Content = NewStr ? as_strdup(NewStr) : NULL;
  Neu->Next = NULL;
  if (!*List)
    *List = Neu;
  else
  {
    Lauf = (*List);
    while (Lauf->Next)
      Lauf = Lauf->Next;
    Lauf->Next = Neu;
  }
}

void RemoveStringList(StringList *List, const char *OldStr)
{
  StringRecPtr Lauf, Hilf;

  if (!*List)
    return;
  if (!strcmp((*List)->Content,OldStr))
  {
    Hilf = *List;
    *List = (*List)->Next;
    ClearStringEntry(&Hilf);
  }
  else
  {
    Lauf = (*List);
    while ((Lauf->Next) && (strcmp(Lauf->Next->Content,OldStr)))
      Lauf = Lauf->Next;
    if (Lauf->Next)
    {
      Hilf = Lauf->Next;
      Lauf->Next = Hilf->Next;
      ClearStringEntry(&Hilf);
    }
  }
}

const char *GetStringListFirst(StringList List, StringRecPtr *Lauf)
{
  *Lauf = List;
  if (!*Lauf)
    return "";
  else
  {
    char *tmp = (*Lauf)->Content;
    *Lauf = (*Lauf)->Next;
    return tmp;
  }
}

const char *GetStringListNext(StringRecPtr *Lauf)
{
  if (!*Lauf)
    return "";
  else
  {
    char *tmp = (*Lauf)->Content;
    *Lauf = (*Lauf)->Next;
    return tmp;
  }
}

char *GetAndCutStringList(StringList *List)
{
  StringRecPtr Hilf;
  static String Result;

  if (!*List)
    Result[0] = '\0';
  else
  {
    Hilf = (*List);
    *List = (*List)->Next;
    strmaxcpy(Result, Hilf->Content, STRINGSIZE);
    free(Hilf->Content);
    free(Hilf);
  }
  return Result;
}

Boolean StringListEmpty(StringList List)
{
  return (!List);
}

StringList DuplicateStringList(StringList Src)
{
  StringRecPtr Lauf;
  StringList Dest;

  InitStringList(&Dest);
  if (Src)
  {
    AddStringListLast(&Dest, GetStringListFirst(Src, &Lauf));
    while (Lauf)
      AddStringListLast(&Dest, GetStringListNext(&Lauf));
  }
  return Dest;
}

Boolean StringListPresent(StringList List, char *Search)
{
  while ((List) && (strcmp(List->Content, Search)))
    List = List->Next;
  return (List != NULL);
}

void DumpStringList(StringList List)
{
  while (List)
  {
    printf("'%s' -> ", List->Content ? List->Content : "<NULL>");
    List = List->Next;
  }
  printf("\n");
}
