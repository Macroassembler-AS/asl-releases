#ifndef _TEXFONTS_H
#define _TEXFONTS_H
/* texfonts.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX-->ASCII/HTML Converter: Font Stuff                                    */
/*                                                                           */
/*****************************************************************************/

#define MFontEmphasized (1 << FontEmphasized)
#define MFontBold (1 << FontBold)
#define MFontTeletype (1 << FontTeletype)
#define MFontItalic (1 << FontItalic)

/*--------------------------------------------------------------------------*/

typedef enum
{
  FontTiny, FontSmall, FontNormalSize, FontLarge, FontHuge
} tFontSize;

typedef enum
{
  FontStandard, FontEmphasized, FontBold, FontTeletype, FontItalic, FontSuper, FontCnt
} tFontType;

/*--------------------------------------------------------------------------*/

extern int CurrFontFlags;
extern tFontSize CurrFontSize;
extern tFontType CurrFontType;

/*--------------------------------------------------------------------------*/

extern void InitFont(void);

extern void SaveFont(void);

extern void RestoreFont(void);

extern void FreeFontStack(void);

#endif /* _TEXFONTS_H */
