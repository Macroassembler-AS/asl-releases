#ifndef _DYNSTRING_H
#define _DYNSTRING_H
/* dynstring.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Dynamic String Handling                                                   */
/*                                                                           */
/*****************************************************************************/


typedef struct sDynString
{
  unsigned Length /*, AllocLength*/;
  char Contents[256];
} tDynString;

extern unsigned DynString2CString(char *pDest, const tDynString *pSrc, unsigned DestLen);

extern unsigned DynString2DynString(tDynString *pDest, const tDynString *pSrc);

extern unsigned DynStringAppend(tDynString *pDest, const char *pSrc, int SrcLen); /* -1 -> strlen */

extern unsigned CString2DynString(tDynString *pDest, const char *pSrc);

extern unsigned DynStringAppendDynString(tDynString *pDest, const tDynString *pSrc);

extern int DynStringCmp(const tDynString *pStr1, const tDynString *pStr2);

extern int DynStringFind(const tDynString *pHaystack, const tDynString *pNeedle);

#endif /* _DYNSTRING_H */
