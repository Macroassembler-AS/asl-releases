#ifndef _ASMPARS_H
#define _ASMPARS_H
/* asmpars.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Verwaltung von Symbolen und das ganze Drumherum...                        */
/*                                                                           */
/*****************************************************************************/

#include <stddef.h>

#include "symbolsize.h"
#include "symflags.h"
#include "tempresult.h"
#include "intformat.h"
#include "lstmacroexp.h"
#include "errmsg.h"
#include "addrspace.h"

typedef enum
{
  UInt0
 ,UInt1
 ,UInt2
 ,UInt3
 ,SInt4    , UInt4   , Int4
 ,SInt5    , UInt5   , Int5
 ,SInt6    , UInt6   , Int6
 ,SInt7    , UInt7
 ,SInt8    , UInt8   , Int8
 ,SInt9    , UInt9
 ,UInt10   , Int10
 ,UInt11
 ,UInt12   , Int12
 ,UInt13
 ,UInt14   , Int14
 ,SInt15   , UInt15  , Int15
 ,SInt16   , UInt16  , Int16
 ,UInt17
 ,UInt18
 ,UInt19
 ,SInt20   , UInt20  , Int20
 ,UInt21
 ,UInt22
 ,UInt23
 ,SInt24   , UInt24  , Int24
 ,SInt30   , UInt30  , Int30
 ,SInt32   , UInt32  , Int32
#ifdef HAS64
 ,SInt64   , UInt64  , Int64
#endif
 ,IntTypeCnt
} IntType;

#ifdef __cplusplus
# include "cppops.h"
DefCPPOps_Enum(IntType)
#endif

#ifdef HAS64
#define LargeUIntType UInt64
#define LargeSIntType SInt64
#define LargeIntType Int64
#else
#define LargeUIntType UInt32
#define LargeSIntType SInt32
#define LargeIntType Int32
#endif

typedef struct
{
  Word SignAndWidth;
  LargeWord Mask;
  LargeInt Min, Max;
} tIntTypeDef;

typedef enum
{
  e_symbol_source_none,
  e_symbol_source_label,
  e_symbol_source_define
} as_symbol_source_t;

typedef struct _TFunction
{
  struct _TFunction *Next;
  Byte ArguCnt;
  StringPtr Name, Definition;
} TFunction, *PFunction;

typedef struct sEvalResult
{
  Boolean OK;
  tSymbolFlags Flags;
  unsigned AddrSpaceMask; /* Welche Adressraeume genutzt ? */
  tSymbolSize DataSize;
} tEvalResult;

struct as_eval_cb_data;
typedef enum { e_eval_none, e_eval_fail, e_eval_ok } as_eval_cb_rtn_t;
typedef enum { e_operator, e_function } as_eval_cb_data_stack_elem_t;
#define DECLARE_AS_EVAL_CB(fnc) as_eval_cb_rtn_t fnc(struct as_eval_cb_data *p_data, struct sStrComp *p_arg, TempResult *p_res)
typedef DECLARE_AS_EVAL_CB((*as_eval_cb_t));
typedef struct as_eval_cb_data_stack
{
  struct as_eval_cb_data_stack *p_next;
  as_eval_cb_data_stack_elem_t type;
  const char *p_ident;
  int arg_index;
} as_eval_cb_data_stack_t;
struct as_operator;
typedef struct as_eval_cb_data
{
  as_eval_cb_t callback;
  const struct as_operator *p_operators;
  as_eval_cb_data_stack_t *p_stack;
  TempResult *p_other_arg;
} as_eval_cb_data_t;

struct sStrComp;
struct as_nonz_dynstr;
struct sRelocEntry;
struct sSymbolEntry;

extern tIntTypeDef IntTypeDefs[IntTypeCnt];
extern LongInt MomLocHandle;
extern LongInt TmpSymCounter,
               FwdSymCounter,
               BackSymCounter;
extern char TmpSymCounterVal[10];
extern LongInt LocHandleCnt;
extern LongInt MomLocHandle;


extern void AsmParsInit(void);

extern void InitTmpSymbols(void);

extern Boolean SingleBit(LargeInt Inp, LargeInt *Erg);


extern IntType GetSmallestUIntType(LargeWord MaxValue);

extern IntType GetUIntTypeByBits(unsigned Bits);

extern tErrorNum NonZString2Int(const struct as_nonz_dynstr *p_str, LargeInt *p_result);

extern Boolean Int2NonZString(struct as_nonz_dynstr *p_str, LargeInt Src);

extern int TempResultToInt(TempResult *pResult);

extern Boolean MultiCharToInt(TempResult *pResult, unsigned MaxLen);


extern Boolean RangeCheck(LargeInt Wert, IntType Typ);

extern Boolean ChkRangeByType(LargeInt value, IntType type, const struct sStrComp *p_comp);
extern Boolean ChkRangeWarnByType(LargeInt value, IntType type, const struct sStrComp *p_comp);


extern Boolean IdentifySection(const struct sStrComp *pName, LongInt *Erg);


extern struct sStrComp *ExpandStrSymbol(struct sStrComp *p_exp_comp, const struct sStrComp *pSrc, Boolean convert_upper);

extern void ChangeSymbol(struct sSymbolEntry *pEntry, LargeInt Value);

extern struct sSymbolEntry *EnterIntSymbolWithFlags(const struct sStrComp *pName, LargeInt Wert, as_addrspace_t addrspace, Boolean MayChange, tSymbolFlags Flags);

#define EnterIntSymbol(pName, Wert, addrspace, MayChange) EnterIntSymbolWithFlags(pName, Wert, addrspace, MayChange, eSymbolFlag_None)

extern void EnterExtSymbol(const struct sStrComp *pName, LargeInt Wert, as_addrspace_t addrspace, Boolean MayChange);

extern struct sSymbolEntry *EnterRelSymbol(const struct sStrComp *pName, LargeInt Wert, as_addrspace_t addrspace, Boolean MayChange);

extern void EnterFloatSymbol(const struct sStrComp *pName, as_float_t Wert, Boolean MayChange);

extern void EnterStringSymbol(const struct sStrComp *pName, const char *pValue, Boolean MayChange);

extern void EnterNonZStringSymbolWithFlags(const struct sStrComp *pName, const struct as_nonz_dynstr *p_value, Boolean MayChange, tSymbolFlags Flags);

extern void EnterRegSymbol(const struct sStrComp *pName, const tRegDescr *Value, tSymbolSize Size, Boolean MayChange, Boolean AddList);

#define EnterNonZStringSymbol(pName, pValue, MayChange) EnterNonZStringSymbolWithFlags(pName, pValue, MayChange, eSymbolFlag_None)

extern void LookupSymbol(const struct sStrComp *pName, TempResult *pValue, Boolean WantRelocs, TempType ReqType);

extern void PrintSymbolList(void);

extern void PrintDebSymbols(FILE *f);

extern void PrintNoISymbols(FILE *f);

extern void PrintSymbolTree(void);

extern void ClearSymbolList(void);

extern void ResetSymbolDefines(void);

extern void PrintSymbolDepth(void);


extern void EvalResultClear(tEvalResult *pResult);

extern void as_eval_cb_data_ini(struct as_eval_cb_data *p_data, as_eval_cb_t cb);
extern void as_dump_eval_cb_data_stack(const as_eval_cb_data_stack_t *p_stack);
extern unsigned as_eval_cb_data_stack_depth(const as_eval_cb_data_stack_t *p_stack);
extern Boolean as_eval_cb_data_stack_plain_add(const as_eval_cb_data_stack_t *p_stack);
extern Boolean as_eval_cb_data_stackelem_mul(const as_eval_cb_data_stack_t *p_stack);

extern void SetSymbolOrStructElemSize(const struct sStrComp *pName, tSymbolSize Size);

extern Boolean IsSymbolDefined(const struct sStrComp *pName);

extern Boolean IsSymbolUsed(const struct sStrComp *pName);

extern Boolean IsSymbolChangeable(const struct sStrComp *pName);

extern Integer GetSymbolType(const struct sStrComp *pName);

extern void EvalExpression(const char *pExpr, TempResult *Erg);

extern void EvalStrExpression(const struct sStrComp *pExpr, TempResult *pErg);
extern void EvalStrExpressionWithCallback(const struct sStrComp *pExpr, TempResult *pErg, as_eval_cb_data_t *p_callback_data);

extern void SetIntConstModeByMask(LongWord Mask);
extern void SetIntConstMode(tIntConstMode Mode);
extern void SetIntConstRelaxedMode(Boolean NewRelaxedMode);

extern LargeInt EvalStrIntExpression(const struct sStrComp *pExpr, IntType Type, Boolean *pResult);
extern LargeInt EvalStrIntExpressionWithFlags(const struct sStrComp *pExpr, IntType Type, Boolean *pResult, tSymbolFlags *pFlags);
extern LargeInt EvalStrIntExpressionWithResult(const struct sStrComp *pExpr, IntType Type, struct sEvalResult *pResult);
extern LargeInt EvalStrIntExprWithResultAndCallback(const struct sStrComp *pExpr, IntType Type, struct sEvalResult *pResult, as_eval_cb_data_t *p_callback_data);
extern LargeInt EvalStrIntExpressionOffs(const struct sStrComp *pExpr, int Offset, IntType Type, Boolean *pResult);
extern LargeInt EvalStrIntExpressionOffsWithFlags(const struct sStrComp *pExpr, int Offset, IntType Type, Boolean *pResult, tSymbolFlags *pFlags);
extern LargeInt EvalStrIntExpressionOffsWithResult(const struct sStrComp *pExpr, int Offset, IntType Type, struct sEvalResult *pResult);
extern LargeInt EvalStrIntExprOffsWithResultAndCallback(const struct sStrComp *pExpr, int Offset, IntType Type, struct sEvalResult *pResult, as_eval_cb_data_t *p_callback_data);

extern as_float_t EvalStrFloatExpressionWithResult(const struct sStrComp *pExpr, struct sEvalResult *pResult);
extern as_float_t EvalStrFloatExpression(const struct sStrComp *pExpr, Boolean *pResult);

extern void EvalStrStringExpressionWithResult(const struct sStrComp *pExpr, struct sEvalResult *pResult, char *pEvalResult);
extern void EvalStrStringExpression(const struct sStrComp *pExpr, Boolean *pResult, char *pEvalResult);

extern tErrorNum EvalStrRegExpressionWithResult(const struct sStrComp *pExpr, struct sRegDescr *pResult, struct sEvalResult *pEvalResult);
typedef enum { eIsNoReg, eIsReg, eRegAbort } tRegEvalResult;
extern tRegEvalResult EvalStrRegExpressionAsOperand(const struct sStrComp *pArg, struct sRegDescr *pResult, struct sEvalResult *pEvalResult, tSymbolSize ReqSize, Boolean MustBeReg);


extern Boolean PushSymbol(const struct sStrComp *pSymName, const struct sStrComp *pStackName);

extern Boolean PopSymbol(const struct sStrComp *pSymName, const struct sStrComp *pStackName);

extern void ClearStacks(void);


extern void EnterFunction(const struct sStrComp *pComp, char *FDefinition, Byte NewCnt);

extern PFunction FindFunction(const char *Name);

extern void PrintFunctionList(void);

extern void ClearFunctionList(void);


extern void AddDefSymbol(char *Name, TempResult *Value);

extern void RemoveDefSymbol(char *Name);

extern void CopyDefSymbols(void);

extern const TempResult *FindDefSymbol(const char *pName);

extern void PrintCrossList(void);

extern void ClearCrossList(void);


extern LongInt GetSectionHandle(const char *SName, Boolean AddEmpt, LongInt Parent);

extern const char *GetSectionName(LongInt Handle);

extern void SetMomSection(LongInt Handle);

extern void AddSectionUsage(LongInt Start, LongInt Length);

extern void ClearSectionUsage(void);

extern void PrintSectionList(void);

extern void PrintDebSections(FILE *f);

extern void ClearSectionList(void);


extern void SetFlag(Boolean *Flag, const char *Name, Boolean Wert);

extern LongInt GetLocHandle(void);

extern void PushLocHandle(LongInt NewLoc);

extern void PopLocHandle(void);

extern void ClearLocStack(void);


extern void PrintRegDefs(void);

extern PTransTable FindCodepage(const char *p_name, PTransTable p_source);

extern void ClearCodepages(void);

extern void PrintCodepages(void);


extern void asmpars_init(void);

#endif /* _ASMPARS_H */
