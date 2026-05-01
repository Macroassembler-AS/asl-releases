/* invaddress.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Disassembler                                                              */
/*                                                                           */
/* inverse symbol storage                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "strutil.h"
#include "trees.h"
#include "invaddress.h"

typedef struct
{
  TTree Tree;
  char *pSymbolName;
} tInvSymbol;

static tInvSymbol *pInvSymbolRoot;
static int MaxSymbolNameLen;

static void MakeInvSymbolName(char *pDest, unsigned DestSize, LargeWord Num)
{
  *pDest = 'I';
  SysString(pDest + 1, DestSize - 1, Num, 16, 8, False, HexStartCharacter, SplitByteCharacter);
}

static Boolean InvSymbolAdder(PTree *ppDest, PTree pNew, void *pData)
{
  /*tInvSymbol *pNewInvSymbol = (tInvSymbol*)pNew, *pNode;*/
  UNUSED(pData);
  UNUSED(pNew);

  /* added to an empty leaf ? */

  if (!ppDest)
  {
    return True;
  }
  return False;
}

void AddInvSymbol(const char *pSymbolName, LargeWord Value)
{
  String Name;
  tInvSymbol *pNew;
  PTree pTreeRoot;
  int ThisSymbolNameLen;

  if ((ThisSymbolNameLen = strlen(pSymbolName)) > MaxSymbolNameLen)
    MaxSymbolNameLen = ThisSymbolNameLen;

  MakeInvSymbolName(Name, sizeof(Name), Value);
  pNew = (tInvSymbol*)calloc(1, sizeof(*pNew));
  pNew->Tree.Name = as_strdup(Name);
  pNew->pSymbolName = as_strdup(pSymbolName);

  pTreeRoot = &(pInvSymbolRoot->Tree);
  EnterTree(&pTreeRoot, (PTree)pNew, InvSymbolAdder, NULL);
  pInvSymbolRoot = (tInvSymbol*)pTreeRoot;
}

const char *LookupInvSymbol(LargeWord Value)
{
  String Name;
  TTree *pTree;

  MakeInvSymbolName(Name, sizeof(Name), Value);
  pTree = SearchTree(&pInvSymbolRoot->Tree, Name, 0);
  if (pTree)
    return ((tInvSymbol*)pTree)->pSymbolName;
  else
    return NULL;
}

int GetMaxInvSymbolNameLen(void)
{
  return MaxSymbolNameLen;
}
