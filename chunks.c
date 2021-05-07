/* chunks.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung von Adressbereichslisten                                       */
/*                                                                           */
/* Historie: 16. 5.1996 Grundsteinlegung                                     */
/*           16. 8.1998 Min/Max-Ausgabe                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include "strutil.h"

#include "chunks.h"

/*--------------------------------------------------------------------------*/
/* eine Chunkliste initialisieren */

void InitChunk(ChunkList *NChunk)
{
  NChunk->RealLen = 0;
  NChunk->AllocLen = 0;
  NChunk->Chunks = NULL;
}

void ClearChunk(ChunkList *NChunk)
{
  if (NChunk->AllocLen > 0)
    free(NChunk->Chunks);
  InitChunk(NChunk);
}

/*--------------------------------------------------------------------------*/
/* eine Chunkliste um einen Eintrag erweitern */

static Boolean Overlap(LargeWord Start1, LargeWord Len1, LargeWord Start2, LargeWord Len2)
{
  return ((Start1 == Start2)
       || ((Start2 > Start1) && (Start1 + Len1 >= Start2))
       || ((Start1 > Start2) && (Start2 + Len2 >= Start1)));
}

static void SetChunk(OneChunk *NChunk,
                     LargeWord Start1, LargeWord Len1,
                     LargeWord Start2, LargeWord Len2)
{
  NChunk->Start  = min(Start1, Start2);
  NChunk->Length = max(Start1 + Len1 - 1, Start2 + Len2 - 1) - NChunk->Start + 1;
}

static void IncChunk(ChunkList *NChunk)
{
  if (NChunk->RealLen + 1 > NChunk->AllocLen)
  {
    if (NChunk->RealLen == 0)
      NChunk->Chunks = (OneChunk *) malloc(sizeof(OneChunk));
    else
      NChunk->Chunks = (OneChunk *) realloc(NChunk->Chunks, sizeof(OneChunk) * (NChunk->RealLen + 1));
    NChunk->AllocLen = NChunk->RealLen + 1;
  }
}

Boolean AddChunk(ChunkList *NChunk, LargeWord NewStart, LargeWord NewLen, Boolean Warn)
{
  Word z, f1 = 0, f2 = 0;
  Boolean Found;
  LongWord PartSum;
  Boolean Result;

  Result = False;

  if (NewLen == 0)
    return Result;

  /* herausfinden, ob sich das neue Teil irgendwo mitanhaengen laesst */

  Found = False;
  for (z = 0; z < NChunk->RealLen; z++)
    if (Overlap(NewStart, NewLen, NChunk->Chunks[z].Start, NChunk->Chunks[z].Length))
    {
      Found = True;
      f1 = z;
      break;
    }

  /* Fall 1: etwas gefunden : */

  if (Found)
  {
    /* gefundene Chunk erweitern */

    PartSum = NChunk->Chunks[f1].Length + NewLen;
    SetChunk(NChunk->Chunks + f1, NewStart, NewLen, NChunk->Chunks[f1].Start, NChunk->Chunks[f1].Length);
    if (Warn)
      if (PartSum != NChunk->Chunks[f1].Length) Result = True;

    /* schauen, ob sukzessiv neue Chunks angebunden werden koennen */

    do
    {
      Found = False;
      for (z = 1; z < NChunk->RealLen; z++)
        if (z != f1)
         if (Overlap(NChunk->Chunks[z].Start, NChunk->Chunks[z].Length, NChunk->Chunks[f1].Start, NChunk->Chunks[f1].Length))
         {
           Found = True;
           f2 = z; break;
         }
      if (Found)
      {
        SetChunk(NChunk->Chunks + f1, NChunk->Chunks[f1].Start, NChunk->Chunks[f1].Length, NChunk->Chunks[f2].Start, NChunk->Chunks[f2].Length);
        NChunk->Chunks[f2] = NChunk->Chunks[--NChunk->RealLen];
      }
    }
    while (Found);
  }

  /* ansonsten Feld erweitern und einschreiben */

  else
  {
    IncChunk(NChunk);

    NChunk->Chunks[NChunk->RealLen].Length = NewLen;
    NChunk->Chunks[NChunk->RealLen].Start = NewStart;
    NChunk->RealLen++;
  }

  return Result;
}

/*--------------------------------------------------------------------------*/
/* Ein Stueck wieder austragen */

void DeleteChunk(ChunkList *NChunk, LargeWord DelStart, LargeWord DelLen)
{
  Word z;
  LargeWord OStart;

  if (DelLen == 0)
    return;

  z = 0;
  while (z <= NChunk->RealLen)
  {
    if (Overlap(DelStart, DelLen, NChunk->Chunks[z].Start, NChunk->Chunks[z].Length))
    {
      if (NChunk->Chunks[z].Start >= DelStart)
      {
        if (DelStart + DelLen >= NChunk->Chunks[z].Start + NChunk->Chunks[z].Length)
        {
          /* ganz loeschen */
          NChunk->Chunks[z] = NChunk->Chunks[--NChunk->RealLen];
        }
        else
        {
          /* unten abschneiden */
          OStart = NChunk->Chunks[z].Start;
          NChunk->Chunks[z].Start = DelStart + DelLen;
          NChunk->Chunks[z].Start -= NChunk->Chunks[z].Start - OStart;
        }
      }
      else
       if (DelStart + DelLen >= NChunk->Chunks[z].Start + NChunk->Chunks[z].Length)
       {
         /* oben abschneiden */
         NChunk->Chunks[z].Length = DelStart - NChunk->Chunks[z].Start;
         /* wenn Laenge 0, ganz loeschen */
         if (NChunk->Chunks[z].Length == 0)
         {
           NChunk->Chunks[z] = NChunk->Chunks[--NChunk->RealLen];
         }
       }
       else
       {
         /* teilen */
         IncChunk(NChunk);
         NChunk->Chunks[NChunk->RealLen].Start = DelStart + DelLen;
         NChunk->Chunks[NChunk->RealLen].Length = NChunk->Chunks[z].Start + NChunk->Chunks[z].Length-NChunk->Chunks[NChunk->RealLen].Start;
         NChunk->Chunks[z].Length = DelStart - NChunk->Chunks[z].Start;
       }
    }
    z++;
  }
}

/*--------------------------------------------------------------------------*/
/* check whether address is in chunk */

Boolean AddressInChunk(ChunkList *NChunk, LargeWord Address)
{
  Word z;

  for (z = 0; z < NChunk->RealLen; z++)
  {
    if ((NChunk->Chunks[z].Start <= Address)
     && (NChunk->Chunks[z].Start + NChunk->Chunks[z].Length - 1 >= Address))
      return True;
  }
  return False;
}

/*--------------------------------------------------------------------------*/
/* Minimaladresse holen */

LargeWord ChunkMin(ChunkList *NChunk)
{
  LongInt z;
  LargeWord t = (LargeWord) -1;

  if (NChunk->RealLen == 0)
    return 0;

  for (z = 0; z < NChunk->RealLen; z++)
    if (NChunk->Chunks[z].Start < t)
      t = NChunk->Chunks[z].Start;

  return t;
}

/*--------------------------------------------------------------------------*/
/* Maximaladresse holen */

LargeWord ChunkMax(ChunkList *NChunk)
{
  LongInt z;
  LargeWord t = (LargeWord) 0;

  if (NChunk->RealLen == 0) return 0;

  for (z = 0; z < NChunk->RealLen; z++)
    if (NChunk->Chunks[z].Start + NChunk->Chunks[z].Length - 1 > t)
      t = NChunk->Chunks[z].Start + NChunk->Chunks[z].Length - 1;

  return t;
}

/*--------------------------------------------------------------------------*/
/* Menge holen */

LargeWord ChunkSum(ChunkList *NChunk)
{
  LongInt z;
  LargeWord Sum = 0;

  for (z = 0; z < NChunk->RealLen; z++)
    Sum += NChunk->Chunks[z].Length;

  return Sum;
}

/*--------------------------------------------------------------------------*/
/* sort by address */

static int CompareChunk(const void *c1, const void *c2)
{
  const OneChunk *pChunk1 = (const OneChunk*)c1,
                 *pChunk2 = (const OneChunk*)c2;

  if (pChunk1->Start < pChunk2->Start)
    return -1;
  else if (pChunk1->Start > pChunk2->Start)
    return 1;
  else
    return 0;
}

void SortChunks(ChunkList *NChunk)
{
  qsort(NChunk->Chunks, NChunk->RealLen, sizeof(NChunk->Chunks[0]), CompareChunk);
}

void chunks_init(void)
{
}
