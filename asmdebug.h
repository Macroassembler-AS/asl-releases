#ifndef _ASMDEBUG_H
#define _ASMDEBUG_H
/* asmdebug.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung der Debug-Informationen zur Assemblierzeit                     */
/*                                                                           */
/* Historie: 16. 5.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

extern void AddLineInfo(Boolean InMacro, LongInt LineNum, char *FileName,
                        ShortInt Space, LargeInt Address, LargeInt Len);

extern void InitLineInfo(void);

extern void ClearLineInfo(void);

extern void DumpDebugInfo(void);

extern void asmdebug_init(void);
#endif /* _ASMDEBUG_H */
