/* intformat.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* enums regarding integer constant notations                                */
/*                                                                           */
/*****************************************************************************/

#ifndef _INTFORMAT_H
#define _INTFORMAT_H

typedef enum
{
  eIntFormatNone,
  eIntFormatDefRadix, /* ... */
  eIntFormatMotBin,   /* %... */
  eIntFormatMotOct,   /* @... */
  eIntFormatMotHex,   /* $... */
  eIntFormatIntBin,   /* ...b */
  eIntFormatIntOOct,  /* ...o */
  eIntFormatIntQOct,  /* ...q */
  eIntFormatIntHex,   /* ...h */
  eIntFormatIBMBin,   /* b'...' */
  eIntFormatIBMOct,   /* o'...' */
  eIntFormatIBMXHex,  /* x'...' */
  eIntFormatIBMHHex,  /* h'...' */
  eIntFormatIBMAsc,   /* a'...' */
  eIntFormatCBin,     /* 0b... */
  eIntFormatCOct,     /* 0... */
  eIntFormatCHex,     /* 0x... */
  eIntFormatNatHex    /* 0..., incompatible with eIntFormatCOct */
} tIntFormatId;

#define eIntFormatMaskC     ((1ul << eIntFormatCHex)   | (1ul << eIntFormatCBin)   | (1ul << eIntFormatCOct))
#define eIntFormatMaskIntel ((1ul << eIntFormatIntHex) | (1ul << eIntFormatIntBin) | (1ul << eIntFormatIntOOct) | (1ul << eIntFormatIntQOct))
#define eIntFormatMaskMoto  ((1ul << eIntFormatMotHex) | (1ul << eIntFormatMotBin) | (1ul << eIntFormatMotOct))
#define eIntFormatMaskIBM   ((1ul << eIntFormatIBMXHex) | (1ul << eIntFormatIBMHHex) | (1ul << eIntFormatIBMBin) | (1ul << eIntFormatIBMOct))

typedef enum eIntConstMode
{
  eIntConstModeIntel,     /* Hex xxxxh, Oct xxxxo, Bin xxxxb */
  eIntConstModeMoto,      /* Hex $xxxx, Oct @xxxx, Bin %xxxx */
  eIntConstModeC,         /* Hex 0x..., Oct 0...., Bin 0b... */
  eIntConstModeIBM        /* Hex 'xxxx['], Oct o'xxxx['], Bin b'xxxx['] */
} tIntConstMode;

typedef struct
{
  const char *pExpr;
  size_t ExprLen;
  int Base;
} tIntCheckCtx;

typedef Boolean (*tIntFormatCheck)(tIntCheckCtx *pCtx, char Ch);

typedef struct
{
  tIntFormatCheck Check;
  Byte Id;
  Integer Base;
  char Ch;
  char Ident[7];
} tIntFormatList;

extern LongWord NativeIntConstModeMask, OtherIntConstModeMask;
extern tIntFormatList *IntFormatList;
extern Boolean RelaxedMode;
extern int RadixBase;

extern const char *GetIntConstMotoPrefix(unsigned Radix);
extern const char *GetIntConstIntelSuffix(unsigned Radix);
extern const char *GetIntConstIBMPrefix(unsigned Radix);
extern const char *GetIntConstIBMSuffix(unsigned Radix);
extern const char *GetIntConstCPrefix(unsigned Radix);

extern void SetIntConstModeByMask(LongWord Mask);
extern Boolean ModifyIntConstModeByMask(LongWord ANDMask, LongWord ORMask);

extern void SetIntConstMode(tIntConstMode Mode);

extern void SetIntConstRelaxedMode(Boolean NewRelaxedMode);

extern tIntFormatId GetIntFormatId(const char *pIdent);

extern void intformat_init(void);

#endif /* _INTFORMAT_H */
