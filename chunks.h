#ifndef _CHUNKS_H
#define _CHUNKS_H
/* chunks.h */
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

typedef struct
{
  LargeWord Start, Length;
} OneChunk;

typedef struct
{
  Word RealLen,AllocLen;
  OneChunk *Chunks;
} ChunkList;


extern Boolean AddChunk(ChunkList *NChunk, LargeWord NewStart, LargeWord NewLen, Boolean Warn);

extern void DeleteChunk(ChunkList *NChunk, LargeWord DelStart, LargeWord DelLen);

extern Boolean AddressInChunk(ChunkList *NChunk, LargeWord Address);

extern void SortChunks(ChunkList *NChunk);

extern void InitChunk(ChunkList *NChunk);

extern void ClearChunk(ChunkList *NChunk);

extern LargeWord ChunkMin(ChunkList *NChunk);

extern LargeWord ChunkMax(ChunkList *NChunk);

extern LargeWord ChunkSum(ChunkList *NChunk);


extern void chunks_init(void);
#endif /* _CHUNKS_H */
