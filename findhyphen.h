/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
#ifndef _FINDHYPHEN_H
#define _FINDHYPHEN_H

#define HYPHEN_CHR_ae "\344"
#define HYPHEN_CHR_oe "\366"
#define HYPHEN_CHR_ue "\374"
#define HYPHEN_CHR_AE "\304"
#define HYPHEN_CHR_OE "\326"
#define HYPHEN_CHR_UE "\334"
#define HYPHEN_CHR_sz "\337"
#define HYPHEN_CHR_mu "\265"

extern void BuildTree(char **Patterns);

extern void AddException(char *Name);

extern void DoHyphens(char *word, int **posis, int *posicnt);

extern void DestroyTree(void);
#endif /* _FINDHYPHEN_H */
