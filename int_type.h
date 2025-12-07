#ifndef _INT_TYPE_H
#define _INT_TYPE_H
/* int_type.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Integer Types of various lengths and signedness                           */
/*                                                                           */
/*****************************************************************************/

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
 ,SInt64   , UInt64  , Int64
 ,SInt128   , UInt128  , Int128
 ,IntTypeCnt
} IntType;

#ifdef __cplusplus
# include "cppops.h"
DefCPPOps_Enum(IntType)
#endif

#ifdef HAS128
#define LargeUIntType UInt128
#define LargeSIntType SInt128
#define LargeIntType Int128
#else
#ifdef HAS64
#define LargeUIntType UInt64
#define LargeSIntType SInt64
#define LargeIntType Int64
#else
#define LargeUIntType UInt32
#define LargeSIntType SInt32
#define LargeIntType Int32
#endif
#endif

#endif /* _INT_TYPE_H */
