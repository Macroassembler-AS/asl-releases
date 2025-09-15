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
#include <assert.h>

#include "be_le.h"
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

void DumpTreeNode(FILE *p_file, TTree *p_tree, int indent)
{
  if (indent > 0)
    fprintf(p_file, "%*s", indent, "");
  fprintf(p_file, "%s", p_tree->Name);
  if (p_tree->Attribute >= 0)
    fprintf(p_file, "(%d)", (int)p_tree->Attribute);
}

static void DumpTreeIter(FILE *p_file, PTree Tree, LongInt Level)
{
  ChkStack();
  if (Tree)
  {
    if (Tree->Left) DumpTreeIter(p_file, Tree->Left, Level + 1);
    DumpTreeNode(p_file, Tree, 6 * Level);
    fprintf(p_file, "\n");
    if (Tree->Right) DumpTreeIter(p_file, Tree->Right, Level + 1);
  }
}

void DumpTree(FILE *p_file, PTree Tree)
{
  DumpTreeIter(p_file, Tree, 0);
}

void DumpTreePath(FILE *p_file, const as_tree_path_t *p_path)
{
  size_t z;
  for (z = 0; z < p_path->length; z++)
  {
    fprintf(p_file, "->");
    DumpTreeNode(p_file, p_path->path[z], 0);
    fprintf(p_file, "[%d]", p_path->state[z]);
  }
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

#define DBG_FIND_TREE 0

#if DBG_FIND_TREE

#include <stdarg.h>

static int tree_dbg_printf(const char *p_fmt, ...)
{
  va_list ap;
  int ret;
  va_start(ap, p_fmt);
  ret = vfprintf(stderr, p_fmt, ap);
  va_end(ap);
  return ret;
}

#define tree_dbg_node(p_node) DumpTreeNode(stderr, p_node, 0)
#define tree_dbg_path(p_path) DumpTreePath(stderr, p_path)
#define tree_dbg_tree(p_tree) DumpTree(stderr, p_tree)

#else /* !DBG_FIND_TREE */

static int tree_dbg_printf(const char *p_fmt, ...)
{
  UNUSED(p_fmt);
  return 0;
}

#define tree_dbg_node(p_node) do { (void)p_node; } while (0)
#define tree_dbg_path(p_path) do { (void)p_path; } while (0)
#define tree_dbg_tree(p_tree) do { (void)p_tree; } while (0)

#endif /* DBG_FIND_TREE */

/*!------------------------------------------------------------------------
 * \fn     find_tree_first(TTree *p_tree, const char *p_name, as_tree_path_t *p_path)
 * \brief  find first occurence of symbol with given name
 * \param  p_tree tree to search
 * \param  p_name symbol name to search
 * \param  p_path tracks path in tree for find_tree_next()
 * \return * to first entry or NULL if not found
 * ------------------------------------------------------------------------ */

static void add_path(as_tree_path_t *p_path, TTree *p_tree, int state)
{
  assert(p_path->length < as_array_size(p_path->path));
  p_path->state[p_path->length] = state;
  p_path->path[p_path->length++] = p_tree;
}

static TTree *find_min_for_recursive(TTree *p_tree, const char *p_name, as_tree_path_t *p_path)
{
  int this_cmp;

  if (!p_tree)
    return NULL;
  tree_dbg_printf(" cmp to '");
  tree_dbg_node(p_tree);
  this_cmp = StrCmp(p_name, p_tree->Name, -1, p_tree->Attribute);
  tree_dbg_printf("' -> this_cmp %d\n", this_cmp);

  if (this_cmp == 0)
  {
    add_path(p_path, p_tree, 0);
    return p_tree;
  }
  else if (this_cmp < 0)
  {
    TTree *p_candidate;
    size_t save_path_length;

    add_path(p_path, p_tree, -1);
    save_path_length = p_path->length;
    p_candidate = find_min_for_recursive(p_tree->Left, p_name, p_path);
    if (!p_candidate)
    {
      p_path->length = save_path_length;
      p_path->state[p_path->length - 1] = 0;
      return p_tree;
    }
    else
      return p_candidate;
  }
  else
  {
    add_path(p_path, p_tree, 1);
    return find_min_for_recursive(p_tree->Right, p_name, p_path);
  }
}

static TTree *find_min_for_iterative(TTree *p_tree, const char *p_name, as_tree_path_t *p_path)
{
  TTree *p_ret;

  for (p_ret = p_tree; p_ret;)
  {
    int cmp_res;

    tree_dbg_printf(" cmp to '");
    tree_dbg_node(p_ret);
    tree_dbg_printf("'\n");

    assert(p_path->length < as_array_size(p_path->path));
    p_path->path[p_path->length] = p_ret;
    p_path->state[p_path->length] =
    cmp_res = StrCmp(p_name, p_ret->Name, -1, p_ret->Attribute);
    p_path->length++;
    if (cmp_res < 0)
      p_ret = p_ret->Left;
    else if (cmp_res > 0)
      p_ret = p_ret->Right;
    else
      break;
  }
  return p_ret;
}

TTree *find_tree_first(TTree *p_tree, const char *p_name, as_tree_path_t *p_path)
{
  TTree *p_ret;

  tree_dbg_printf("find_tree_first '%s'\n", p_name);
  tree_dbg_tree(p_tree);

  p_path->length = 0;
  p_ret = p_tree;
  p_ret = find_min_for_recursive(p_tree, p_name, p_path);
  (void)find_min_for_iterative;
  (void)find_min_for_recursive;
  if (p_ret && strcmp(p_ret->Name, p_name))
  {
    tree_dbg_printf("->element '%s' does not match, ignore\n", p_ret->Name);
    p_ret = NULL;
  }
  /* We return the end of p_path.  If it is no exact match, the associated
     state might be !=0.  Force to 0 so find_tree_next() does not return the
     same node again: */
  if (p_ret && p_path->length && (p_path->path[p_path->length - 1] == p_ret))
    p_path->state[p_path->length - 1] = 0;

  if (p_ret)
  {
    tree_dbg_printf("->found '");
    tree_dbg_node(p_ret);
    tree_dbg_printf("', path (length %u): ", (unsigned)p_path->length);
    tree_dbg_path(p_path);
    tree_dbg_printf("\n");
  }

  return p_ret;
}

/*!------------------------------------------------------------------------
 * \fn     find_tree_next(const char *p_name, as_tree_path_t *p_path)
 * \brief  retrieve successor element in tree
 * \param  p_name selector name
 * \param  p_path current position in tree
 * \return next element or NULL if end of tree or no more symbols of given name
 * ------------------------------------------------------------------------ */

static TTree *find_tree_next_sub(TTree *p_sub, as_tree_path_t *p_path)
{
  if (!p_sub)
    return NULL;
  if (p_sub->Left)
  {
    add_path(p_path, p_sub, -1);
    return find_tree_next_sub(p_sub->Left, p_path);
  }
  else
  {
    add_path(p_path, p_sub, 0);
    return p_sub;
  }
}

TTree *find_tree_next(const char *p_name, as_tree_path_t *p_path)
{
  TTree *p_ret;
  size_t node_index;

  tree_dbg_printf("find_tree_next: ");
  tree_dbg_path(p_path);
  tree_dbg_printf("\n");

again:
  if (!p_path->length)
  {
    tree_dbg_printf("-> no path left, return NULL\n");
    return NULL;
  }
  node_index = p_path->length - 1;

  /* Iterate in tree, using path: */

  if (p_path->state[node_index] == 0)
  {
    p_ret = find_tree_next_sub(p_path->path[node_index]->Right, p_path);
    if (!p_ret)
    {
      p_path->length--;
      tree_dbg_printf("-> was on node self, and no right subtree, reduce path length to %u\n",
                      (unsigned)p_path->length);
      goto again;
    }
    else
    {
      tree_dbg_printf("-> was on node self, and right subtree exists, stepped down\n");
      p_path->state[node_index] = 1;
    }
  }
  else if (p_path->state[node_index] < 0)
  {
    tree_dbg_printf("-> was on left subtree, return self\n");
    p_path->state[node_index] = 0;
    p_ret = p_path->path[node_index];
  }
  else /* if (p_path->state[node_index] > 0) */
  {
    p_path->length--;
    tree_dbg_printf("-> was on right subtree, node exhausted, reduce path length to %u\n",
                    (unsigned)p_path->length);
    goto again;;
  }

  /* Successor in tree, but carrying different name: end of list of symbols
     with same name reached: */

  if (p_ret && strcmp(p_ret->Name, p_name))
    p_ret = NULL;

  tree_dbg_printf("-> return ");
  if (p_ret)
    tree_dbg_node(p_ret);
  else
    tree_dbg_printf("NULL");
  tree_dbg_printf("\n");

  return p_ret;
}
