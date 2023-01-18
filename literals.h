#ifndef _LITERALS_H
#define _LITERALS_H
/* literals.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Literal Handling Functions                                                */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>
#include "datatypes.h"
#include "symbolsize.h"

typedef struct as_literal
{
  struct as_literal *p_next;
  LongInt value, f_count;
  tSymbolSize size;
  Boolean is_forward;
  Integer pass_no;
  LongInt def_section;
} as_literal_t;

extern as_literal_t *p_first_literal;

extern LongInt literal_forward_count;

extern void literals_chk_alldone(void);

extern as_literal_t *literals_new(void);

extern void literals_free(as_literal_t* p_literal);

extern const char *literals_get_name(const as_literal_t *p_lit, char *p_result, size_t result_size);

struct sStrComp;
extern void literal_make(struct sStrComp *p_comp, Byte *p_data_offset, LongInt value, tSymbolSize value_size, Boolean force_create);

typedef LargeInt (*literal_dump_fnc_t)(const as_literal_t *p_lit, struct sStrComp *p_name);
extern void literals_dump(literal_dump_fnc_t dump, tSymbolSize value_size, LongInt section_handle, Boolean compress);

extern void literals_print(void);

extern void literals_init(void);

#endif /* _LITERALS_H */
