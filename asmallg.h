#ifndef _ASMALLG_H
#define _ASMALLG_H
/* codeallg.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* von allen Codegeneratoren benutzte Pseudobefehle                          */
/*                                                                           */
/* Historie:  10. 5.1996 Grundsteinlegung                                    */
/*                                                                           */
/*****************************************************************************/

struct sStrComp;

extern void SetCPUByType(CPUVar NewCPU, const struct sStrComp *pCPUArgs);

extern Boolean SetCPUByName(const struct sStrComp *pName);

extern void UnsetCPU(void);

extern Boolean CheckONOFFArg(const tStrComp *pArg, Boolean *pResult);

extern void AddONOFF(const char *InstName, Boolean *Flag, const char *FlagName, Boolean Persist);

extern void ClearONOFF(void);

extern Boolean CodeGlobalPseudo(void);

extern void CodeREG(Word Index);
extern void CodeNAMEREG(Word Index);

extern void INCLUDE_SearchCore(struct sStrComp *pDest, const struct sStrComp *pArg, Boolean SearchPath);

extern void codeallg_init(void);
#endif /* _ASMALLG_H */
