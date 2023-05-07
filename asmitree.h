#ifndef _ASMITREE_H
#define _ASMITREE_H
/* asmitree.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Opcode-Abfrage als Binaerbaum                                             */
/*                                                                           */
/* Historie: 30.10.1996 Grundsteinlegung                                     */
/*            6.12.1998 dynamische Variante                                  */
/*                                                                           */
/*****************************************************************************/

#include "datatypes.h"

typedef void (*InstProc)(
#ifdef __PROTOS__
Word Index
#endif
);

typedef Boolean (*inst_fnc_t)(
#ifdef __PROTOS__
Word index
#endif
);

typedef struct _TInstTreeNode
{
  struct _TInstTreeNode *Left,*Right;
  InstProc Proc;
  char *Name;
  Word Index;
  ShortInt Balance;
} TInstTreeNode,*PInstTreeNode;

typedef struct _TInstTableEntry
{
  InstProc Proc;
  char *Name;
  Word Index;
  int Coll;
}
TInstTableEntry, *PInstTableEntry;

typedef struct inst_fnc_table_entry
{
  inst_fnc_t fnc;
  char *p_name;
  Word index;
  int coll;
} inst_fnc_table_entry_t;

struct sInstTable
{
  int Fill,Size;
  Boolean Dynamic;
  PInstTableEntry Entries;
};
typedef struct sInstTable TInstTable;
typedef struct sInstTable *PInstTable;

typedef struct inst_fnc_table
{
  int fill, size;
  Boolean dynamic;
  inst_fnc_table_entry_t *p_entries;
} inst_fnc_table_t;

extern void AddInstTree(PInstTreeNode *Root, char *NName, InstProc NProc, Word NIndex);

extern void ClearInstTree(PInstTreeNode *Root);

extern Boolean SearchInstTree(PInstTreeNode Root, char *OpPart);

extern void PrintInstTree(PInstTreeNode Root);


extern PInstTable CreateInstTable(int TableSize);
extern inst_fnc_table_t *inst_fnc_table_create(int table_size);

extern void SetDynamicInstTable(PInstTable Table);

extern void DestroyInstTable(PInstTable tab);
extern void inst_fnc_table_destroy(inst_fnc_table_t *p_table);

extern void AddInstTable(PInstTable tab, const char *Name, Word Index, InstProc Proc);
extern void inst_fnc_table_add(inst_fnc_table_t *p_table, const char *p_name, Word index, inst_fnc_t fnc);

extern void RemoveInstTable(PInstTable tab, const char *Name);

extern Boolean LookupInstTable(PInstTable tab, const char *Name);
extern Boolean inst_fnc_table_lookup(const inst_fnc_table_t *p_table, const char *p_name);

extern void PrintInstTable(FILE *stream, PInstTable tab);

extern void asmitree_init(void);

#endif /* _ASMITREE_H */
