/* codechunks.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Maintain address ranges & included code                                   */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include "codechunks.h"

static void SetCodePtr(tCodeChunk *pChunk, void *pCode, unsigned Granularity)
{
  switch ((pChunk->Granularity = Granularity))
  {
    case 4:
      pChunk->pLongCode = (LongWord*)pCode;
      pChunk->pWordCode = NULL;
      pChunk->pCode = NULL;
      break;
    case 2:
      pChunk->pLongCode = (LongWord*)pCode;
      pChunk->pWordCode = (Word*)pCode;
      pChunk->pCode = NULL;
      break;
    case 1:
      pChunk->pLongCode = (LongWord*)pCode;
      pChunk->pWordCode = (Word*)pCode;
      pChunk->pCode = (Byte*)pCode;
      break;
  }
}

static void CheckOverlap(const tCodeChunk *pChunk1, const tCodeChunk *pChunk2)
{
  LargeWord OverlapStart, OverlapEnd;

  OverlapStart = max(pChunk1->Start, pChunk2->Start);
  OverlapEnd = min(pChunk1->Start + pChunk1->Length - 1, pChunk2->Start + pChunk2->Length - 1);
  if (OverlapStart <= OverlapEnd)
    fprintf(stderr, "code chunk overlap\n");
}

static int SetCodeChunkListAllocLen(tCodeChunkList *pCodeChunkList, unsigned NewAllocLen)
{
  int Result;

  if (NewAllocLen < pCodeChunkList->RealLen)
  {
    unsigned z;

    for (z = NewAllocLen; z < pCodeChunkList->RealLen; z++)
      FreeCodeChunk(pCodeChunkList->Chunks + z);
    pCodeChunkList->RealLen = NewAllocLen;
  }

  if (!pCodeChunkList->AllocLen)
  {
    pCodeChunkList->Chunks = (tCodeChunk*)malloc(NewAllocLen * sizeof(tCodeChunk));
    Result = pCodeChunkList->Chunks ? 0 : ENOMEM;
  }
  else
  {
    tCodeChunk *pNewChunks = (tCodeChunk*)realloc(pCodeChunkList->Chunks, NewAllocLen * sizeof(tCodeChunk));
    Result = pNewChunks ? 0 : ENOMEM;
    if (!Result)
      pCodeChunkList->Chunks = pNewChunks;
  }

  if (!Result)
  {
    pCodeChunkList->AllocLen = NewAllocLen;
    if (!pCodeChunkList->AllocLen)
      pCodeChunkList->Chunks = NULL;
  }
  return Result;
}

void InitCodeChunk(tCodeChunk *pChunk)
{
  pChunk->Start = pChunk->Length = 0;
  pChunk->Granularity = 1;
  SetCodePtr(pChunk, NULL, 1);
}

int ReadCodeChunk(tCodeChunk *pChunk, const char *pFileName, LargeWord Start, LargeWord Length, unsigned Granularity)
{
  FILE *pFile = NULL;
  Byte *pCode = NULL;
  int Result;

  UNUSED(Granularity);
  FreeCodeChunk(pChunk);

  pFile = fopen(pFileName, OPENRDMODE);
  if (!pFile)
  {
    Result = EIO;
    goto func_exit;
  }

  if (!Length)
  {
    fseek(pFile, 0, SEEK_END);
    Length = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
  }

  pCode = (Byte*)malloc(Length);
  if (!pCode)
  {
    Result = ENOMEM;
    goto func_exit;
  }

  if (fread(pCode, 1, Length, pFile) != Length)
  {
    Result = EIO;
    goto func_exit;
  }

  pChunk->Start = Start;
  pChunk->Length = Length;
  SetCodePtr(pChunk, pCode, 1);
  pCode = NULL;

  Result = 0;

func_exit:
  if (pCode)
    free(pCode);
  if (pFile)
    fclose(pFile);
  return Result;
}

void FreeCodeChunk(tCodeChunk *pChunk)
{
  if (pChunk->pCode)
    free(pChunk->pCode);
  InitCodeChunk(pChunk);
}

void InitCodeChunkList(tCodeChunkList *pCodeChunkList)
{
  pCodeChunkList->RealLen =
  pCodeChunkList->AllocLen = 0;
  pCodeChunkList->Chunks = NULL;
}

int MoveCodeChunkToList(tCodeChunkList *pCodeChunkList, tCodeChunk *pNewCodeChunk, Boolean WarnOverlap)
{
  unsigned Index;

  /* increase size? */

  if (pCodeChunkList->RealLen >= pCodeChunkList->AllocLen)
  {
    int Res = SetCodeChunkListAllocLen(pCodeChunkList, pCodeChunkList->RealLen + 16);

    if (Res)
      return Res;
  }

  /* find sort in position */

  for (Index = 0; Index < pCodeChunkList->RealLen; Index++)
    if (pCodeChunkList->Chunks[Index].Start > pNewCodeChunk->Start)
      break;

  /* sort in */

  memmove(pCodeChunkList->Chunks + Index + 1,
          pCodeChunkList->Chunks + Index,
          sizeof(*pCodeChunkList->Chunks) * (pCodeChunkList->RealLen - Index));
  pCodeChunkList->Chunks[Index] = *pNewCodeChunk;
  pCodeChunkList->RealLen++;

  /* data was moved out of chunk */

  InitCodeChunk(pNewCodeChunk);

  /* check overlap? */

  if (WarnOverlap)
  {
    if (Index > 0)
      CheckOverlap(pCodeChunkList->Chunks + Index - 1,
                   pCodeChunkList->Chunks + Index);
    if (Index < pCodeChunkList->RealLen - 1U)
      CheckOverlap(pCodeChunkList->Chunks + Index,
                   pCodeChunkList->Chunks + Index + 1);
  }

  return 0;
}

extern void FreeCodeChunkList(tCodeChunkList *pCodeChunkList)
{
  SetCodeChunkListAllocLen(pCodeChunkList, 0);
}

Boolean RetrieveCodeFromChunkList(const tCodeChunkList *pCodeChunkList, LargeWord Start, Byte *pData, unsigned Count)
{
  const tCodeChunk *pChunk;
  LargeWord OverlapStart, OverlapEnd;
  Boolean Found;

  while (Count > 0)
  {
    Found = False;
    for (pChunk = pCodeChunkList->Chunks; pChunk < pCodeChunkList->Chunks + pCodeChunkList->RealLen; pChunk++)
    {
      OverlapStart = max(pChunk->Start, Start);
      OverlapEnd = min(pChunk->Start + pChunk->Length - 1, Start + Count - 1);
      if (OverlapStart <= OverlapEnd)
      {
        unsigned PartLength = OverlapEnd - OverlapStart + 1;

        memcpy(pData, pChunk->pCode + (OverlapStart - pChunk->Start), PartLength);
        pData += PartLength;
        Count -= PartLength;
        Found = True;
        break;
      }
    }
    if (!Found)
      return False;
  }
  return True;
}

LargeWord GetCodeChunksStored(const tCodeChunkList *pCodeChunkList)
{
  LargeWord Sum = 0;
  const tCodeChunk *pChunk;

  for (pChunk = pCodeChunkList->Chunks; pChunk < pCodeChunkList->Chunks + pCodeChunkList->RealLen; pChunk++)
    Sum += pChunk->Length;

  return Sum;
}
