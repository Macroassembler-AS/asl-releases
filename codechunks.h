#ifndef _CODECHUNKS_H
#define _CODECHUNKS_H
/* codechunks.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Maintain address ranges & included code                                   */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef struct
{
  LargeWord Start, Length;
  unsigned Granularity;
  LongWord *pLongCode;
  Word *pWordCode;
  Byte *pCode;
} tCodeChunk;

typedef struct
{
  Word RealLen, AllocLen;
  tCodeChunk *Chunks;
} tCodeChunkList;

extern void InitCodeChunk(tCodeChunk *pChunk);
extern int ReadCodeChunk(tCodeChunk *pChunk, const char *pFileName, LargeWord Start, LargeWord Length, unsigned Granularity);
extern void FreeCodeChunk(tCodeChunk *pChunk);

extern void InitCodeChunkList(tCodeChunkList *pCodeChunkList);
extern int MoveCodeChunkToList(tCodeChunkList *pCodeChunkList, tCodeChunk *pNewCodeChunk, Boolean WarnOverlap);
extern void FreeCodeChunkList(tCodeChunkList *pCodeChunkList);
extern Boolean RetrieveCodeFromChunkList(const tCodeChunkList *pCodeChunkList, LargeWord Start, Byte *pData, unsigned Count);

extern LargeWord GetCodeChunksStored(const tCodeChunkList *pCodeChunkList);

#endif /* _CODECHUNKS_H */
