#ifndef _STRUTIL_H
#define _STRUTIL_H
/* strutil.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* haeufig benoetigte String-Funktionen                                      */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

#include "datatypes.h"

struct as_dynstr;

extern char HexStartCharacter;
extern char SplitByteCharacter;

extern const char *Blanks(int cnt);

#define HexString(pDest, DestSize, i, Stellen) SysString(pDest, DestSize, i, 16, Stellen, False, HexStartCharacter, SplitByteCharacter)
#define DecString(pDest, DestSize, i, Digits) SysString(pDest, DestSize, i, 10, Digits, False, HexStartCharacter, '\0')

extern int SysString(char *pDest, size_t DestSize, LargeWord i, int System, int Stellen, Boolean ForceLeadZero, char StartCharacter, char SplitCharacter);

extern char *as_strdup(const char *s);

extern int as_vsnprcatf(char *pDest, size_t DestSize, const char *pFormat, va_list ap);
extern int as_snprcatf(char *pDest, size_t DestSize, const char *pFormat, ...);
extern int as_vsnprintf(char *pDest, size_t DestSize, const char *pFormat, va_list ap);
extern int as_snprintf(char *pDest, size_t DestSize, const char *pFormat, ...);

extern int as_vsdprcatf(struct as_dynstr *p_dest, const char *pFormat, va_list ap);
extern int as_sdprcatf(struct as_dynstr *p_dest, const char *pFormat, ...);
extern int as_vsdprintf(struct as_dynstr *p_dest, const char *pFormat, va_list ap);
extern int as_sdprintf(struct as_dynstr *p_dest, const char *pFormat, ...);

extern int as_strcasecmp(const char *src1, const char *src2);
extern int as_strncasecmp(const char *src1, const char *src2, size_t maxlen);

#ifdef NEEDS_STRSTR
extern char *strstr(const char *haystack, const char *needle);
#endif

extern char *strrmultchr(const char *haystack, const char *needles);

extern size_t strmaxcpy(char *dest, const char *src, size_t Max);
extern size_t strmaxcat(char *Dest, const char *Src, size_t MaxLen);
extern void strprep(char *Dest, const char *Src);
extern void strmaxprep(char *p_dest, const char *p_src, size_t max_len);
extern void strmaxprep2(char *p_dest, const char *p_src, size_t max_len);
extern void strins(char *Dest, const char *Src, int Pos);
extern void strmaxins(char *Dest, const char *Src, int Pos, size_t MaxLen);

extern size_t as_strnlen(const char *pStr, size_t MaxLen);

extern int strreplace(char *pHaystack, const char *pFrom, const char *pTo, size_t ToMaxLen, size_t HaystackSize);

extern int strlencmp(const char *pStr1, unsigned Str1Len,
                     const char *pStr2, unsigned Str2Len);

extern unsigned fstrlenprint(FILE *pFile, const char *pStr, unsigned StrLen);

extern void ReadLn(FILE *Datei, char *Zeile);

extern size_t ReadLnCont(FILE *Datei, struct as_dynstr *p_line);

extern int DigitVal(char ch, int Base);

extern LargeInt ConstLongInt(const char *inp, Boolean *pErr, LongInt Base);

extern char *ParenthPos(char *pHaystack, char Needle);

extern void KillBlanks(char *s);

extern int CopyNoBlanks(char *pDest, const char *pSrc, size_t MaxLen);

extern int KillPrefBlanks(char *s);

extern int KillPostBlanks(char *s);

extern char TabCompressed(char in);

extern int strqcmp(const char *s1, const char *s2);

extern char *strmov(char *pDest, const char *pSrc);

extern int strmemcpy(char *pDest, size_t DestSize, const char *pSrc, size_t SrcLen);

extern void strutil_init(void);

/* avoid nasty "subscript has type char..." messages on some platforms */

#include <ctype.h>

#define __chartouint(c) (((unsigned int)(c)) & 0xff)
#define as_toupper(c) (toupper(__chartouint(c)))
#define as_tolower(c) (tolower(__chartouint(c)))
#define as_isspace(c) (!!isspace(__chartouint(c)))
#define as_isdigit(c) (!!isdigit(__chartouint(c)))
#define as_isxdigit(c) (!!isxdigit(__chartouint(c)))
#define as_isprint(c) (!!isprint(__chartouint(c)))
#define as_isalpha(c) (!!isalpha(__chartouint(c)))
#define as_isupper(c) (!!isupper(__chartouint(c)))
#define as_islower(c) (!!islower(__chartouint(c)))
#define as_isalnum(c) (!!isalnum(__chartouint(c)))

#endif /* _STRUTIL_H */
