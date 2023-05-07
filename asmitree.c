/* asmitree.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Opcode-Abfrage als Binaerbaum                                             */
/*                                                                           */
/* Historie: 30.10.1996 Grundsteinlegung                                     */
/*            8.10.1997 Hash-Tabelle                                         */
/*            6.12.1998 dynamisches Kopieren der Namen                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "chunks.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"

#include "asmitree.h"

/*---------------------------------------------------------------------------*/

static Boolean AddSingle(PInstTreeNode *Node, char *NName, InstProc NProc, Word NIndex)
{
  PInstTreeNode p1, p2;
  Boolean Result = False;

  ChkStack();

  if (!*Node)
  {
    *Node = (PInstTreeNode) malloc(sizeof(TInstTreeNode));
    (*Node)->Left = NULL;
    (*Node)->Right = NULL;
    (*Node)->Proc = NProc;
    (*Node)->Index = NIndex;
    (*Node)->Balance = 0;
    (*Node)->Name = as_strdup(NName);
    Result = True;
  }
  else if (strcmp(NName, (*Node)->Name) < 0)
  {
    if (AddSingle(&((*Node)->Left), NName, NProc, NIndex))
      switch ((*Node)->Balance)
      {
        case 1:
          (*Node)->Balance = 0;
          break;
        case 0:
          (*Node)->Balance = -1;
          Result = True;
          break;
        case -1:
          p1 = (*Node)->Left;
          if (p1->Balance == -1)
          {
            (*Node)->Left = p1->Right;
            p1->Right = (*Node);
            (*Node)->Balance = 0;
            *Node = p1;
          }
         else
         {
           p2 = p1->Right;
           p1->Right = p2->Left;
           p2->Left = p1;
           (*Node)->Left = p2->Right;
           p2->Right = (*Node);
           (*Node)->Balance = (p2->Balance == -1) ? 1 : 0;
           p1->Balance = (p2->Balance == 1) ? -1 : 0;
           *Node = p2;
         }
         (*Node)->Balance = 0;
         break;
     }
  }
  else
  {
    if (AddSingle(&((*Node)->Right), NName, NProc, NIndex))
      switch ((*Node)->Balance)
      {
        case -1:
          (*Node)->Balance = 0;
          break;
        case 0:
          (*Node)->Balance = 1;
          Result = True;
          break;
        case 1:
          p1 = (*Node)->Right;
          if (p1->Balance == 1)
          {
            (*Node)->Right = p1->Left;
            p1->Left = (*Node);
            (*Node)->Balance = 0;
            *Node = p1;
          }
          else
          {
            p2 = p1->Left;
            p1->Left = p2->Right;
            p2->Right = p1;
            (*Node)->Right = p2->Left;
            p2->Left = (*Node);
            (*Node)->Balance = (p2->Balance == 1) ? -1 : 0;
            p1->Balance = (p2->Balance == -1) ? 1 : 0;
            *Node = p2;
          }
          (*Node)->Balance = 0;
          break;
      }
  }
  return Result;
}

void AddInstTree(PInstTreeNode *Root, char *NName, InstProc NProc, Word NIndex)
{
  AddSingle(Root, NName, NProc, NIndex);
}

static void ClearSingle(PInstTreeNode *Node)
{
  ChkStack();

  if (*Node)
  {
    free((*Node)->Name);
    ClearSingle(&((*Node)->Left));
    ClearSingle(&((*Node)->Right));
    free(*Node);
    *Node = NULL;
  }
}

void ClearInstTree(PInstTreeNode *Root)
{
  ClearSingle(Root);
}

Boolean SearchInstTree(PInstTreeNode Root, char *OpPart)
{
  int z;

  z = 0;
  while ((Root) && (strcmp(Root->Name, OpPart)))
  {
    Root = (strcmp(OpPart, Root->Name) < 0) ? Root->Left : Root->Right;
    z++;
  }

  if (!Root)
    return False;
  else
  {
    Root->Proc(Root->Index);
    return True;
  }
}

static void PNode(PInstTreeNode Node, Word Lev)
{
  ChkStack();
  if (Node)
  {
    PNode(Node->Left, Lev + 1);
    printf("%*s %s %p %p %d\n", 5 * Lev, "", Node->Name, (void*)Node->Left, (void*)Node->Right, Node->Balance);
    PNode(Node->Right, Lev + 1);
  }
}

void PrintInstTree(PInstTreeNode Root)
{
  PNode(Root, 0);
}

/*----------------------------------------------------------------------------*/

static int GetKey(const char *Name, LongWord TableSize)
{
  register unsigned char *p;
  LongWord tmp = 0;

  for (p = (unsigned char *)Name; *p != '\0'; p++)
    tmp = (tmp << 2) + ((LongWord)*p);
  return tmp % TableSize;
}

PInstTable CreateInstTable(int TableSize)
{
  int z;
  PInstTableEntry tmp;
  PInstTable tab;

  tmp = (PInstTableEntry) malloc(sizeof(TInstTableEntry) * TableSize);
  for (z = 0; z < TableSize; z++)
    tmp[z].Name = NULL;
  tab = (PInstTable) malloc(sizeof(TInstTable));
  tab->Fill = 0;
  tab->Size = TableSize;
  tab->Entries = tmp;
  tab->Dynamic = FALSE;
  return tab;
}

/*!------------------------------------------------------------------------
 * \fn     inst_fnc_table_create(int table_size)
 * \brief  create new instance of function table
 * \param  table_size max. # of entries in table
 * \return * to table
 * ------------------------------------------------------------------------ */

inst_fnc_table_t *inst_fnc_table_create(int table_size)
{
  int z;
  inst_fnc_table_entry_t *p_entries = NULL;
  inst_fnc_table_t *p_ret = NULL;

  p_entries = (inst_fnc_table_entry_t*)calloc(table_size, sizeof(*p_entries));
  if (!p_entries)
    goto func_exit;
  for (z = 0; z < table_size; z++)
    p_entries[z].p_name = NULL;

  p_ret = (inst_fnc_table_t*) calloc(1, sizeof(*p_ret));
  if (!p_ret)
    goto func_exit;
  p_ret->fill = 0;
  p_ret->size = table_size;
  p_ret->p_entries = p_entries; p_entries = NULL;
  p_ret->dynamic = False;

func_exit:
  if (p_entries)
    free(p_entries);
  return p_ret;
}

void SetDynamicInstTable(PInstTable Table)
{
  Table->Dynamic = TRUE;
}

void DestroyInstTable(PInstTable tab)
{
  int z;

  if (tab->Dynamic)
    for (z = 0; z < tab->Size; z++)
      free(tab->Entries[z].Name);

  free(tab->Entries);
  free(tab);
}

void AddInstTable(PInstTable tab, const char *Name, Word Index, InstProc Proc)
{
  LongWord h0 = GetKey(Name, tab->Size), z = 0;

  /* mindestens ein freies Element lassen, damit der Sucher garantiert terminiert */

  if (tab->Size - 1 <= tab->Fill)
  {
    fprintf(stderr, "\nhash table overflow\n");
    exit(255);
  }
  while (1)
  {
    if (!tab->Entries[h0].Name)
    {
      tab->Entries[h0].Name = (tab->Dynamic) ? as_strdup(Name) : (char*)Name;
      tab->Entries[h0].Proc = Proc;
      tab->Entries[h0].Index = Index;
      tab->Entries[h0].Coll = z;
      tab->Fill++;
      return;
    }
    if (!strcmp(tab->Entries[h0].Name, Name))
    {
      printf("%s double in table\n", Name);
      exit(255);
    }
    z++;
    if ((LongInt)(++h0) == tab->Size)
      h0 = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     inst_fnc_table_add(inst_fnc_table_t *p_table, const char *p_name, Word index, inst_fnc_t fnc)
 * \brief  augment table by one entry
 * \param  p_table table to add to
 * \param  p_name name of instruction
 * \param  index callback argument
 * \param  fnc callback itself
 * ------------------------------------------------------------------------ */

void inst_fnc_table_add(inst_fnc_table_t *p_table, const char *p_name, Word index, inst_fnc_t fnc)
{
  LongWord tab_index = GetKey(p_name, p_table->size), num_coll = 0;

  /* mindestens ein freies Element lassen, damit der Sucher garantiert terminiert */

  if (p_table->size - 1 <= p_table->fill)
  {
    fprintf(stderr, "\nhash table overflow\n");
    exit(255);
  }
  while (1)
  {
    if (!p_table->p_entries[tab_index].p_name)
    {
      p_table->p_entries[tab_index].p_name = (p_table->dynamic) ? as_strdup(p_name) : (char*)p_name;
      p_table->p_entries[tab_index].fnc = fnc;
      p_table->p_entries[tab_index].index = index;
      p_table->p_entries[tab_index].coll = num_coll;
      p_table->fill++;
      return;
    }
    if (!strcmp(p_table->p_entries[tab_index].p_name, p_name))
    {
      printf("%s double in table\n", p_name);
      exit(255);
    }
    num_coll++;
    if ((LongInt)(++tab_index) == p_table->size)
      tab_index = 0;
  }
}

void RemoveInstTable(PInstTable tab, const char *Name)
{
  LongWord h0 = GetKey(Name, tab->Size);

  while (1)
  {
    if (!tab->Entries[h0].Name)
      return;
    else if (!strcmp(tab->Entries[h0].Name, Name))
    {
      tab->Entries[h0].Name = NULL;
      tab->Entries[h0].Proc = NULL;
      tab->Fill--;
      return;
    }
    if ((LongInt)(++h0) == tab->Size)
      h0 = 0;
  }
}

Boolean LookupInstTable(PInstTable tab, const char *Name)
{
  LongWord h0 = GetKey(Name, tab->Size);

  while (1)
  {
    if (!tab->Entries[h0].Name)
      return False;
    else if (!strcmp(tab->Entries[h0].Name, Name))
    {
      tab->Entries[h0].Proc(tab->Entries[h0].Index);
      return True;
    }
    if ((LongInt)(++h0) == tab->Size)
      h0 = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     inst_fnc_table_lookup(const inst_fnc_table_t *p_table, const char *p_name)
 * \brief  look up & execute instruction callback
 * \param  p_table table with functions
 * \param  p_name instruction name
 * \return True if callback was found and executed
 * ------------------------------------------------------------------------ */

Boolean inst_fnc_table_lookup(const inst_fnc_table_t *p_table, const char *p_name)
{
  LongWord index = GetKey(p_name, p_table->size);

  while (1)
  {
    if (!p_table->p_entries[index].p_name)
      return False;
    else if (!strcmp(p_table->p_entries[index].p_name, p_name))
      return p_table->p_entries[index].fnc(p_table->p_entries[index].index);
    if ((LongInt)(++index) == p_table->size)
      index = 0;
  }
}

void PrintInstTable(FILE *stream, PInstTable tab)
{
  int z;

  for (z = 0; z < tab->Size; z++)
    if (tab->Entries[z].Name)
      fprintf(stream, "[%3d]: %-10s Index %4d Coll %2d\n", z,
             tab->Entries[z].Name, tab->Entries[z].Index, tab->Entries[z].Coll);
}

/*----------------------------------------------------------------------------*/

void asmitree_init(void)
{
}
