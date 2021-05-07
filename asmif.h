#ifndef _ASMIF_H
#define _ASMIF_H
/* asmif.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Befehle zur bedingten Assemblierung                                       */
/*                                                                           */
/* Historie: 15. 5.1996 Grundsteinlegung                                     */
/*                                                                           */
/*****************************************************************************/

typedef enum
{
  IfState_IFIF, IfState_IFELSE,
  IfState_CASESWITCH, IfState_CASECASE, IfState_CASEELSE
} tIfState;

typedef struct _TIfSave
{
  struct _TIfSave *Next;
  Integer NestLevel;
  Boolean SaveIfAsm;
  TempResult SaveExpr;
  tIfState State;
  Boolean CaseFound;
  LongInt StartLine;
} TIfSave, *PIfSave;

extern Boolean IfAsm;
extern PIfSave FirstIfSave;


extern Boolean CodeIFs(void);

extern void AsmIFInit(void);

extern Integer SaveIFs(void);

extern void RestoreIFs(Integer Level);

extern Boolean IFListMask(void);

extern void asmif_init(void);
#endif /* _ASMIF_H */
