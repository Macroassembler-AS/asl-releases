#ifndef _AS_FLOAT_H
#define _AS_FLOAT_H
/* as_float.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Internal Floating Point Handling                                          */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include "datatypes.h"

typedef enum
{
  AS_FP_NORMAL,
  AS_FP_SUBNORMAL,
  AS_FP_NAN,
  AS_FP_INFINITE
} as_float_class_t;

#if 0
typedef as_uint16_t as_float_mant_word_t;
#define AS_FLOAT_MANT_WORD_BITS 16
#endif

#if 1
typedef as_uint32_t as_float_mant_word_t;
#define AS_FLOAT_MANT_WORD_BITS 32
#endif

#if 0
typedef as_uint64_t as_float_mant_word_t;
#define AS_FLOAT_MANT_WORD_BITS 64
#endif

#define AS_FLOAT_MANT_WORD_BITS_M1 (AS_FLOAT_MANT_WORD_BITS - 1)
#define AS_FLOAT_NUM_WORDS(num_bits) (((num_bits) + AS_FLOAT_MANT_WORD_BITS - 1) / AS_FLOAT_MANT_WORD_BITS)
typedef as_float_mant_word_t as_float_mant_t[192 / AS_FLOAT_MANT_WORD_BITS];


typedef enum
{
  e_round_none,  /* no bits lost by rounding */
  e_round_up,    /* rounded up, bits lost */
  e_round_down   /* rounded down, bits lost */
} as_float_round_t;

typedef struct as_float_dissect
{
  as_float_class_t fp_class;
  /* 1 (negative) or 0 (positive) */
  Boolean negative;
  /* power-of-2 of exponent */
  Integer exponent;
  /* left-aligned mantissa, range [1,2), explicit leading one: */
  as_float_mant_t mantissa;
  unsigned mantissa_bits;
} as_float_dissect_t;

extern void as_float_zero(as_float_dissect_t *p_dest);

extern void as_float_append_mantissa_bits(as_float_dissect_t *p_dest, LongWord new_bits, unsigned new_num_bits);

extern void as_float_remove_mantissa_bits(as_float_dissect_t *p_dest, unsigned remove_num_bits);

extern Boolean as_float_get_mantissa_bit(const as_float_mant_t p_mant, unsigned num_bits, unsigned bit_pos);

extern as_float_mant_word_t as_float_mantissa_add_bit(as_float_mant_t p_sum, const as_float_mant_t p_add, unsigned add_bit_pos, unsigned num_bits);

extern as_float_mant_word_t as_float_mantissa_shift_right(as_float_mant_t p_mantissa, as_float_mant_word_t shiftin_bit, unsigned num_bits);
extern as_float_mant_word_t as_float_mantissa_shift_left(as_float_mant_t p_mantissa, as_float_mant_word_t shiftin_bit, unsigned num_bits);

extern Boolean as_float_mantissa_is_zero_from(const as_float_dissect_t *p_num, unsigned start_bit);
#define as_float_mantissa_is_zero(p_num) as_float_mantissa_is_zero_from(p_num, 0)

extern Boolean as_float_mantissa_is_allones_from(const as_float_dissect_t *p_num, unsigned start_bit);
#define as_float_mantissa_is_allones(p_num) as_float_mantissa_is_allones_from(p_num, 0)

extern LongWord as_float_mantissa_extract(const as_float_dissect_t *p_num, unsigned start_bit, unsigned num_bits);

extern void as_float_mantissa_twos_complement(as_float_dissect_t *p_num);

extern void as_float_dissect(as_float_dissect_t *p_dest, as_float_t num);

extern as_float_round_t as_float_round(as_float_dissect_t *p_dest, unsigned target_bits);

extern void as_float_dump_mantissa(FILE *p_dest, const as_float_mant_t p_mantissa, unsigned mantissa_bits);
extern void as_float_dump(FILE *p_dest, const char *p_name, const as_float_dissect_t *p_float);

#endif /* _AS_FLOAT_H */
