#ifndef _BPEMU_H
#define _BPEMU_H
/* bpemu.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Emulation einiger Borland-Pascal-Funktionen                               */
/*                                                                           */
/* Historie: 20. 5.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

typedef void (*charcallback)(
#ifdef __PROTOS__
char *Name
#endif
);

extern char *FExpand(char *Src);

extern int FSearch(char *pDest, size_t DestSize, const char *FileToSearch, const char *pCurrFileName, const char *SearchPath);

extern long FileSize(FILE *file);

extern Byte Lo(Word inp);

extern Byte Hi(Word inp);

extern unsigned LoWord(LongWord Src);

extern unsigned HiWord(LongWord Src);

extern unsigned long LoDWord(LargeWord Src);

extern Boolean Odd (int inp);

extern Boolean DirScan(const char *Mask, charcallback callback);

extern LongInt MyGetFileTime(char *Name);

#ifdef __CYGWIN32__
extern char *DeCygWinDirList(char *pStr);

extern char *DeCygwinPath(char *pStr);
#endif

extern void bpemu_init(void);
#endif /* _BPEMU_H */
