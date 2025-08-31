#ifndef _TREES_H
#define _TREES_H
/* trees.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Tree management                                                           */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "datatypes.h"

extern Boolean BalanceTrees;

typedef struct _TTree
{
  struct _TTree *Left, *Right;
  ShortInt Balance;
  char *Name;
  LongInt Attribute;
} TTree, *PTree;

#define AS_TREE_STACK_MAX_PATH_LEN 64

typedef struct as_tree_path
{
  size_t length;
  TTree *path[AS_TREE_STACK_MAX_PATH_LEN];
  ShortInt state[AS_TREE_STACK_MAX_PATH_LEN];
} as_tree_path_t;

typedef void (*TTreeCallback)(PTree Node, void *pData);

typedef Boolean (*TTreeAdder)(PTree *PDest, PTree Neu, void *pData);

extern void IterTree(PTree Tree, TTreeCallback Callback, void *pData);

extern void GetTreeDepth(PTree Tree, LongInt *pMin, LongInt *pMax);

extern void DestroyTree(PTree *Tree, TTreeCallback Callback, void *pData);

extern void DumpTree(FILE *p_file, PTree Tree);

extern PTree SearchTree(PTree Tree, const char *Name, LongInt Attribute);

extern TTree *find_tree_first(TTree *p_tree, const char *p_name, as_tree_path_t *p_path);
extern TTree *find_tree_next(const char *p_name, as_tree_path_t *p_path);

extern Boolean EnterTree(PTree *PDest, PTree Neu, TTreeAdder Adder, void *pData);

#endif /* _TREES_H */
