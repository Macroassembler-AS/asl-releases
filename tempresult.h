#ifndef _TEMPRESULT_H
#define _TEMPRESULT_H
/* tempresult.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* internal holder for int/float/string                                      */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>

#include "datatypes.h"
#include "nonzstring.h"
#include "symflags.h"
#include "symbolsize.h"

typedef enum
{
  TempNone = 0,
  TempInt = 1,
  TempFloat = 2,
  TempString = 4,
  TempReg = 8,
  TempAll = 15
} TempType;

struct sRelocEntry;
struct as_dynstr;

typedef unsigned tRegInt;

typedef void (*DissectRegProc)(
#ifdef __PROTOS__
char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize
#endif
);

typedef struct sRegDescr
{
  DissectRegProc Dissect;
  tRegInt Reg;
} tRegDescr;

struct sTempResult
{
  TempType Typ;
  tSymbolFlags Flags;
  unsigned AddrSpaceMask;
  tSymbolSize DataSize;
  struct sRelocEntry *Relocs;
  union
  {
    LargeInt Int;
    Double Float;
    as_nonz_dynstr_t str;
    tRegDescr RegDescr;
  } Contents;
};
typedef struct sTempResult TempResult;

extern void as_tempres_ini(TempResult *p_res);

extern void as_tempres_free(TempResult *p_res);

extern void as_tempres_set_none(TempResult *p_res);

extern void as_tempres_set_int(TempResult *p_res, LargeInt value);

extern void as_tempres_set_float(TempResult *p_res, Double value);

extern void as_tempres_set_str(TempResult *p_res, const as_nonz_dynstr_t *p_value);

extern void as_tempres_set_str_raw(TempResult *p_res, const char *p_value, size_t len);

extern void as_tempres_set_c_str(TempResult *p_res, const char *p_value);

extern void as_tempres_set_reg(TempResult *p_res, const tRegDescr *p_value);

void as_tempres_copy_value(TempResult *p_dest, const TempResult *p_src);

extern void as_tempres_copy(TempResult *p_dest, const TempResult *p_src);

extern int as_tempres_cmp(const TempResult *p_res1, const TempResult *p_res2);

extern int TempResultToFloat(TempResult *pResult);

extern int as_tempres_append_dynstr(struct as_dynstr *p_dest, const TempResult *pResult);

#endif /* _TEMPRESULT_H */
