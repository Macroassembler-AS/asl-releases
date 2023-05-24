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

/*!------------------------------------------------------------------------
 * \fn     free_rec(StringRecPtr p_rec)
 * \brief  free/destroy string list record
 * \param  p_rec record to free
 * ------------------------------------------------------------------------ */

void InitStringList(StringList *List)
{
  *List = NULL;
}

void ClearStringEntry(StringRecPtr *Elem)
{
  if (*Elem)
  {
    if ((*Elem)->Content)
      free((*Elem)->Content);
    free(*Elem);
    *Elem = NULL;
  }
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

/*!------------------------------------------------------------------------
 * \fn     GetStringListFirst(StringList List, StringRecPtr *Lauf)
 * \brief  retrieve first item of string list & set run pointer
 * \param  List list to iterate
 * \param  Lauf run pointer
 * \return content of head or NULL if list empty
 * ------------------------------------------------------------------------ */

const char *GetStringListFirst(StringList List, StringRecPtr *Lauf)
{
  *Lauf = List;
  if (!*Lauf)
    return NULL;
  else
  {
    char *tmp = (*Lauf)->Content;
    *Lauf = (*Lauf)->Next;
    return tmp;
  }
}

/*!------------------------------------------------------------------------
 * \fn     GetStringListNext(StringRecPtr *Lauf)
 * \brief  retrieve nextitem of string list & update run pointer
 * \param  Lauf run pointer
 * \return content of next item or NULL if end of list reached
 * ------------------------------------------------------------------------ */

const char *GetStringListNext(StringRecPtr *Lauf)
{
  if (!*Lauf)
    return NULL;
  else
  {
    char *tmp = (*Lauf)->Content;
    *Lauf = (*Lauf)->Next;
    return tmp;
  }
}

/*!------------------------------------------------------------------------
 * \fn     MoveAndCutStringListFirst(StringList *p_list)
 * \brief  cut off the head of the string list and return its content, which
           must be freed afer use
 * \param  p_list list to cut from
 * \return * to former head's content or NULL
 * ------------------------------------------------------------------------ */

char *MoveAndCutStringListFirst(StringList *p_list)
{
  if (!*p_list)
    return NULL;
  else
  {
    StringRecPtr p_head;
    char *p_ret;

    p_head = *p_list;
    *p_list = (*p_list)->Next;
    p_ret = p_head->Content; p_head->Content = NULL;
    ClearStringEntry(&p_head);
    return p_ret;
  }
}

/*!------------------------------------------------------------------------
 * \fn     MoveAndCutStringListLast(StringList *p_list)
 * \brief  cut off the tail of the string list and return its content, which
           must be freed afer use
 * \param  p_list list to cut from
 * \return * to former head's content or NULL
 * ------------------------------------------------------------------------ */

char *MoveAndCutStringListLast(StringList *p_list)
{
  if (!*p_list)
    return NULL;
  else
  {
    StringRecPtr p_run, p_prev;
    char *p_ret;

    for (p_prev = NULL, p_run = *p_list; p_run->Next; p_prev = p_run, p_run = p_run->Next);
    if (p_prev)
      p_prev->Next = p_run->Next;
    else
      *p_list = p_run->Next;
    p_ret = p_run->Content; p_run->Content = NULL;
    ClearStringEntry(&p_run);
    return p_ret;
  }
}

Boolean StringListEmpty(StringList List)
{
  return (!List);
}

unsigned StringListCount(StringList List)
{
  unsigned count = 0;

  while (List)
  {
    List = List->Next;
    count++;
  }
  return count;
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
