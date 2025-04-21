#ifndef _ASMCODE_H
#define _ASMCODE_H
/* asmcode.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung der Code-Datei                                                 */
/*                                                                           */
/* Historie: 18. 5.1996 Grundsteinlegung                                     */
/*           19. 1.2000 Patchliste angelegt                                  */
/*           26. 6.2000 added exports                                        */
/*                                                                           */
/*****************************************************************************/

extern LongInt SectSymbolCounter;

extern PPatchEntry PatchList, PatchLast;
extern PExportEntry ExportList, ExportLast;

extern void DreheCodes(void);

extern void NewRecord(LargeWord NStart);

extern void OpenFile(void);

extern void CloseFile(void);

extern void WriteBytes(void);

extern void RetractWords(Word Cnt);

extern void InsertPadding(unsigned NumBytes, Boolean OnlyReserve);

extern void code_len_reset(void);

extern void set_basmcode_guessed(LongWord start, LongWord count, Byte value);
#define set_b_guessed(flags, offs, len, value) \
        do { if (mFirstPassUnknownOrQuestionable(flags)) set_basmcode_guessed(offs, len, value); } while (0)
extern void copy_basmcode_guessed(LongWord dest, LongWord src, size_t count);
extern void or_basmcode_guessed(LongWord start, LongWord count, Byte value);
#define or_b_guessed(flags, offs, len, value) \
        do { if (mFirstPassUnknownOrQuestionable(flags)) or_basmcode_guessed(offs, len, value); } while (0)
extern Byte get_basmcode_guessed(LongWord index);

extern void set_wasmcode_guessed(LongWord start, LongWord count, Word value);
#define set_w_guessed(flags, offs, len, value) \
        do { if (mFirstPassUnknownOrQuestionable(flags)) set_wasmcode_guessed(offs, len, value); } while (0)
extern void copy_wasmcode_guessed(LongWord dest, LongWord src, size_t count);
extern void or_wasmcode_guessed(LongWord start, LongWord count, Word value);
#define or_w_guessed(flags, offs, len, value) \
        do { if (mFirstPassUnknownOrQuestionable(flags)) or_wasmcode_guessed(offs, len, value); } while (0)
extern Word get_wasmcode_guessed(LongWord index);
extern void dump_wasmcode_guessed(const char *p_title);

extern void set_dasmcode_guessed(LongWord start, LongWord count, LongWord value);
#define set_d_guessed(flags, offs, len, value) \
        do { if (mFirstPassUnknownOrQuestionable(flags)) set_dasmcode_guessed(offs, len, value); } while (0)
extern void copy_dasmcode_guessed(LongWord dest, LongWord src, size_t count);
extern void or_dasmcode_guessed(LongWord start, LongWord count, LongWord value);
#define or_d_guessed(flags, offs, len, value) \
        do { if (mFirstPassUnknownOrQuestionable(flags)) or_dasmcode_guessed(offs, len, value); } while (0)
extern LongWord get_dasmcode_guessed(LongWord index);

extern void asmcode_init(void);

#endif /* _ASMCODE_H */
