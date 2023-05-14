#ifndef _CPULIST_H
#define _CPULIST_H
/* cpulist.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Port                                                                   */
/*                                                                           */
/* Manages CPU List                                                          */
/*                                                                           */
/*****************************************************************************/

typedef void (*tCPUSwitchProc)(
#ifdef __PROTOS__
void
#endif
);

typedef void (*tCPUSwitchUserProc)(
#ifdef __PROTOS__
void *pUserData
#endif
);

typedef void (*tPrintLineCPUProc)(
#ifdef __PROTOS__
const char *
#endif
);

typedef void (*tCPUFreeUserDataProc)(void *pUserData);

typedef unsigned CPUVar;
#define CPUNone ((CPUVar)-1)

typedef struct sCPUArg
{
  const char *pName;
  const LongInt Min, Max, DefValue;
  LongInt *pValue;
} tCPUArg;

typedef struct sCPUDef
{
  struct sCPUDef *Next;
  char *Name;
  CPUVar Number, Orig;
  tCPUSwitchUserProc SwitchProc;
  tCPUFreeUserDataProc FreeProc;
  void *pUserData;
  const tCPUArg *pArgs;
} tCPUDef, *tpCPUDef;

typedef void (*tCPUListIterator)(const tCPUDef *pRun, void *pUser);

extern CPUVar AddCPUWithArgs(const char *NewName, tCPUSwitchProc Switcher, const tCPUArg *pArgs);
#define AddCPU(NewName, Switcher) AddCPUWithArgs(NewName, Switcher, NULL)
extern CPUVar AddCPUUserWithArgs(const char *NewName, tCPUSwitchUserProc Switcher, void *pUserData, tCPUFreeUserDataProc Freeer, const tCPUArg *pArgs);
#define AddCPUUser(NewName, Switcher, pUserData, Freeer) AddCPUUserWithArgs(NewName, Switcher, pUserData, Freeer, NULL)

extern Boolean AddCPUAlias(char *OrigName, char *AliasName);

extern const tCPUDef *LookupCPUDefByVar(CPUVar Var);

extern const tCPUDef *LookupCPUDefByName(const char *pName);

extern void IterateCPUList(tCPUListIterator Iterator, void *pUser);

extern void PrintCPUList(tPrintLineCPUProc PrintProc);

extern void ClearCPUList(void);

extern void cpulist_init(void);

#endif /* _CPULIST_H */
