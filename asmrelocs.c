/* asmrelocs.c */
/****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                    */
/*                                                                          */
/* AS-Portierung                                                            */
/*                                                                          */
/* Verwaltung von Relokationslisten                                         */
/*                                                                          */
/* Historie: 25. 7.1999 Grundsteinlegung                                    */
/*            1. 8.1999 Merge-Funktion implementiert                        */
/*            8. 8.1999 Reloc-Liste gespeichert                             */
/*           15. 9.1999 fehlende Includes                                   */
/*                      Add in Merge                                        */
/*           19. 1.2000 TransferRelocs begonnen                             */
/*            8. 3.2000 'ambigious else'-Warnungen beseitigt                */
/*           30.10.2000 added transfer of arbitrary lists                   */
/*                                                                          */
/****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmcode.h"
#include "asmrelocs.h"
#include "errmsg.h"
#include "fileformat.h"

/*---------------------------------------------------------------------------*/

PRelocEntry LastRelocs = NULL;

/*---------------------------------------------------------------------------*/

PRelocEntry MergeRelocs(PRelocEntry *list1, PRelocEntry *list2, Boolean Add)
{
  PRelocEntry PRun1, PRun2, PPrev, PNext, PLast, PRes;

  PRun1 = *list1;
  PLast = PRes = NULL;

  /* ueber alle in Liste 1 */

  while (PRun1)
  {
    /* Uebereinstimmung suchen, die sich aufhebt */

    PNext = PRun1->Next;
    PRun2 = *list2;
    PPrev = NULL;
    while (PRun2)
      if ((!as_strcasecmp(PRun1->Ref, PRun2->Ref)) && ((PRun1->Add != PRun2->Add) != Add))
      {
        /* gefunden -> beide weg */

        free(PRun1->Ref);
        free(PRun2->Ref);
        if (!PPrev)
          *list2 = PRun2->Next;
        else
          PPrev->Next = PRun2->Next;
        free(PRun2);
        free(PRun1);
        PRun1 = NULL;
        break;
      }
      else
      {
        PPrev = PRun2;
        PRun2 = PRun2->Next;
      }

    /* ansonsten an Ergebnisliste anhaengen */

    if (PRun1)
    {
      if (!PLast)
        PRes = PLast = PRun1;
      else
      {
        PLast->Next = PRun1;
        PLast = PRun1;
      }
    }
    PRun1 = PNext;
  }

  /* Reste aus Liste 2 nicht vergessen */

  if (!PLast)
    PRes = *list2;
  else
    PLast->Next = *list2;

  /* Quellisten jetzt leer */

  *list1 = *list2 = NULL;

  /* fertich */

  return PRes;
}

void InvertRelocs(PRelocEntry *erg, PRelocEntry *src)
{
  PRelocEntry SRun;

  for (SRun = *src; SRun; SRun = SRun->Next)
    SRun->Add = !(SRun->Add);

  *erg = *src;
}

void FreeRelocs(PRelocEntry *list)
{
  PRelocEntry Run;

  while (*list)
  {
    Run = *list;
    *list = (*list)->Next;
    free(Run->Ref);
    free(Run);
  }
}

PRelocEntry DupRelocs(PRelocEntry src)
{
  PRelocEntry First, Run, SRun, Neu;

  First = Run = NULL;
  for (SRun = src; SRun; SRun = SRun->Next)
  {
    Neu = (PRelocEntry) malloc(sizeof(TRelocEntry));
    Neu->Next = NULL;
    Neu->Ref = as_strdup(SRun->Ref);
    Neu->Add = SRun->Add;
    if (!First)
      First = Neu;
    else
      Run->Next = Neu;
    Run = Neu;
  }

  return First;
}

void SetRelocs(PRelocEntry List)
{
  if (LastRelocs)
  {
    WrError(ErrNum_UnresRelocs);
    FreeRelocs(&LastRelocs);
  }
  LastRelocs = List;
}

void TransferRelocs2(PRelocEntry RelocList, LargeWord Addr, LongWord Type)
{
  PPatchEntry NewPatch;
  PRelocEntry Curr;

  while (RelocList)
  {
    Curr = RelocList;
    NewPatch = (PPatchEntry) malloc(sizeof(TPatchEntry));
    NewPatch->Next = NULL;
    NewPatch->Address = Addr;
    NewPatch->Ref = Curr->Ref;
    NewPatch->RelocType = Type;
    if (!Curr->Add)
      NewPatch->RelocType |= RelocFlagSUB;
    if (PatchLast == NULL)
      PatchList = NewPatch;
    else
      PatchLast->Next = NewPatch;
    PatchLast = NewPatch;
    RelocList = Curr->Next;
    free(Curr);
  }
}

void TransferRelocs(LargeWord Addr, LongWord Type)
{
  TransferRelocs2(LastRelocs, Addr, Type);
  LastRelocs = NULL;
}

void SubPCRefReloc(void)
{
  PRelocEntry Run, Prev, New;

  if (!RelSegs)
    return;

  /* search if PC subtraction evens out against an addition */

  for (Prev = NULL, Run = LastRelocs; Run; Prev = Run, Run = Run->Next)
    if ((Run->Add) && (!strcmp(Run->Ref, RelName_SegStart)))
    {
      free(Run->Ref);
      if (Prev)
        Prev->Next = Run->Next;
      else
        LastRelocs = Run->Next;
      free(Run);
      return;
    }

  /* in case we did not find one, add a new one */

  New = (PRelocEntry) malloc(sizeof(TRelocEntry));
  New->Ref = as_strdup(RelName_SegStart);
  New->Add = FALSE;
  New->Next = NULL;
  if (Prev)
    Prev->Next = New;
  else
    LastRelocs = New;
}

void AddExport(char *Name, LargeInt Value, LongWord Flags)
{
  PExportEntry PNew;

  PNew = (PExportEntry) malloc(sizeof(TExportEntry));
  PNew->Next = NULL;
  PNew->Name = as_strdup(Name);
  PNew->Value = Value;
  PNew->Flags = Flags;
  if (!ExportList)
    ExportList = PNew;
  else
    ExportLast->Next = PNew;
  ExportLast = PNew;
}
