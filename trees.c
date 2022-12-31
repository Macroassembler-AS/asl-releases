/* trees.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Tree management                                                           */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "endian.h"
#include "nls.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"

#include "trees.h"

Boolean BalanceTrees;

static ShortInt StrCmp(const char *s1, const char *s2, LongInt Hand1, LongInt Hand2)
{
  int tmp;

  tmp = (*s1) - (*s2);
  if (!tmp)
    tmp = strcmp(s1, s2);
  if (!tmp)
    tmp = Hand1 - Hand2;
  if (tmp < 0)
    return -1;
  if (tmp > 0)
    return 1;
  return 0;
}

void IterTree(PTree Tree, TTreeCallback Callback, void *pData)
{
  ChkStack();
  if (Tree)
  {
    if (Tree->Left) IterTree(Tree->Left, Callback, pData);
    Callback(Tree, pData);
    if (Tree->Right) IterTree(Tree->Right, Callback, pData);
  }
}

static void TreeDepthIter(PTree Tree, LongInt Level, LongInt *pMin, LongInt *pMax)
{
  ChkStack();
  if (Tree)
  {
    TreeDepthIter(Tree->Left, Level + 1, pMin, pMax);
    TreeDepthIter(Tree->Right, Level + 1, pMin, pMax);
  }
  else
  {
    if (Level > *pMax) *pMax = Level;
    if (Level < *pMin) *pMin = Level;
  }
}

void GetTreeDepth(PTree Tree, LongInt *pMin, LongInt *pMax)
{
  *pMin = MaxLongInt; *pMax = 0;
  TreeDepthIter(Tree, 0, pMin, pMax);
}

void DestroyTree(PTree *Tree, TTreeCallback Callback, void *pData)
{
  ChkStack();
  if (*Tree)
  {
    if ((*Tree)->Left) DestroyTree(&((*Tree)->Left), Callback, pData);
    if ((*Tree)->Right) DestroyTree(&((*Tree)->Right), Callback, pData);
    Callback(*Tree, pData);
    if ((*Tree)->Name)
    {
      free((*Tree)->Name); (*Tree)->Name = NULL;
    }
    free(*Tree); *Tree = NULL;
  }
}

static void DumpTreeIter(PTree Tree, LongInt Level)
{
  ChkStack();
  if (Tree)
  {
    if (Tree->Left) DumpTreeIter(Tree->Left, Level + 1);
    fprintf(Debug,"%*s%s\n", 6 * Level, "", Tree->Name);
    if (Tree->Right) DumpTreeIter(Tree->Right, Level + 1);
  }
}

void DumpTree(PTree Tree)
{
  DumpTreeIter(Tree, 0);
}

PTree SearchTree(PTree Tree, const char *Name, LongInt Attribute)
{
  ShortInt SErg = -1;

  while ((Tree) && (SErg != 0))
  {
    SErg = StrCmp(Name, Tree->Name, Attribute, Tree->Attribute);
    if (SErg < 0) Tree = Tree->Left;
    else if (SErg > 0) Tree = Tree->Right;
  }
  return Tree;
}

Boolean EnterTree(PTree *PDest, PTree Neu, TTreeAdder Adder, void *pData)
{
  PTree Hilf, p1, p2;
  Boolean Grown, Result;
  ShortInt CompErg;

  /* check for stack overflow, nothing yet inserted */

  ChkStack(); Result = False;

  /* arrived at a leaf? --> simply add */

  if (*PDest == NULL)
  {
    (*PDest) = Neu;
    (*PDest)->Balance = 0; (*PDest)->Left = NULL; (*PDest)->Right = NULL;
    Adder(NULL, Neu, pData);
    return True;
  }

  /* go right, left, or straight? */

  CompErg = StrCmp(Neu->Name, (*PDest)->Name, Neu->Attribute, (*PDest)->Attribute);

  /* left ? */

  if (CompErg > 0)
  {
    Grown = EnterTree(&((*PDest)->Right), Neu, Adder, pData);
    if ((BalanceTrees) && (Grown))
     switch ((*PDest)->Balance)
     {
       case -1:
         (*PDest)->Balance = 0; break;
       case 0:
         (*PDest)->Balance = 1; Result = True; break;
       case 1:
        p1 = (*PDest)->Right;
        if (p1->Balance == 1)
        {
          (*PDest)->Right = p1->Left; p1->Left = *PDest;
          (*PDest)->Balance = 0; *PDest = p1;
        }
        else
        {
          p2 = p1->Left;
          p1->Left = p2->Right; p2->Right = p1;
          (*PDest)->Right = p2->Left; p2->Left = *PDest;
          if (p2->Balance ==  1) (*PDest)->Balance = (-1); else (*PDest)->Balance = 0;
          if (p2->Balance == -1) p1      ->Balance =    1; else p1      ->Balance = 0;
          *PDest = p2;
        }
        (*PDest)->Balance = 0;
        break;
     }
  }

  /* right ? */

   else if (CompErg < 0)
   {
     Grown = EnterTree(&((*PDest)->Left), Neu, Adder, pData);
     if ((BalanceTrees) && (Grown))
       switch ((*PDest)->Balance)
       {
         case 1:
           (*PDest)->Balance = 0;
           break;
         case 0:
           (*PDest)->Balance = -1;
           Result = True;
           break;
         case -1:
           p1 = (*PDest)->Left;
           if (p1->Balance == (-1))
           {
             (*PDest)->Left = p1->Right;
             p1->Right = *PDest;
             (*PDest)->Balance = 0;
             *PDest = p1;
           }
           else
           {
             p2 = p1->Right;
             p1->Right = p2->Left;
             p2->Left = p1;
             (*PDest)->Left = p2->Right;
             p2->Right = *PDest;
             if (p2->Balance == (-1)) (*PDest)->Balance =    1; else (*PDest)->Balance = 0;
             if (p2->Balance ==    1) p1      ->Balance = (-1); else p1      ->Balance = 0;
             *PDest = p2;
           }
           (*PDest)->Balance = 0;
           break;
       }
   }

  /* otherwise we might replace the node */

  else
  {
    if (Adder(PDest, Neu, pData))
    {
      Neu->Left = (*PDest)->Left; Neu->Right = (*PDest)->Right;
      Neu->Balance = (*PDest)->Balance;
      Hilf = *PDest; *PDest = Neu;
      if (Hilf->Name)
      {
        free(Hilf->Name); Hilf->Name = NULL;
      }
      free(Hilf);
    }
  }

  return Result;
}
