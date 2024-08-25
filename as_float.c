/* as_float.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Internal Floating Point Handling                                          */
/*                                                                           */
/*****************************************************************************/

#include <string.h>
#include "strutil.h"
#include "as_float.h"

#define DBG_MANT_APPEND 0
#define DBG_MANT_ADD 0

static as_float_mant_word_t MASK(unsigned n)
{
  if (n < AS_FLOAT_MANT_WORD_BITS)
    return ((as_float_mant_word_t)((1ul << (n)) - 1));
  else
    return ((as_float_mant_word_t)0) - 1;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_get_mantissa_bit(const as_float_mant_t p_mant, unsigned num_bits, unsigned bit_pos)
 * \brief  retrieve single bit of mantissa
 * \param  p_mant mantissa to extract from
 * \param  num_bits mantissa length
 * \param  bit_pos bit position (0=MSB)
 * \return 0 or 1
 * ------------------------------------------------------------------------ */

Boolean as_float_get_mantissa_bit(const as_float_mant_t p_mant, unsigned num_bits, unsigned bit_pos)
{
  if (bit_pos >= num_bits)
    return 0;
  else
  {
    return ((p_mant[bit_pos / AS_FLOAT_MANT_WORD_BITS])
         >> (AS_FLOAT_MANT_WORD_BITS_M1 - (bit_pos % AS_FLOAT_MANT_WORD_BITS))) & 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     mantissa_used_bit_mask(unsigned mantissa_bits, unsigned word_index)
 * \brief  retrieve bit mask of used mantissa bits in word #n
 * \param  mantissa_bits total mantissa length in bits
 * \param  word_index word index in mantissa
 * \return bit mask
 * ------------------------------------------------------------------------ */

static as_float_mant_word_t mantissa_used_bit_mask(unsigned mantissa_bits, unsigned word_index)
{
  unsigned num_words = AS_FLOAT_NUM_WORDS(mantissa_bits),
           last_num_bits = mantissa_bits % AS_FLOAT_MANT_WORD_BITS;

  if (!num_words)
    return 0;
  else if ((word_index < num_words - 1) || !last_num_bits)
    return ~((as_float_mant_word_t)0);
  else
    return ~MASK(AS_FLOAT_MANT_WORD_BITS - last_num_bits);
}

/*!------------------------------------------------------------------------
 * \fn     mantissa_add(as_float_mant_t p_sum, const as_float_mant_t p_add, const as_float_mant_t p_addend, unsigned num_bits)
 * \brief  add two mantissas
 * \param  p_sum sum buffer
 * \param  p_add operand
 * \param  p_addend addend
 * \param  num_bits mantissa length
 * \return carry of MSB add
 * ------------------------------------------------------------------------ */

static as_float_mant_word_t mantissa_add(as_float_mant_t p_sum, const as_float_mant_t p_add, const as_float_mant_t p_addend, unsigned num_bits)
{
  as_float_mant_word_t carry = 0, carry1, carry2, tmp1, tmp2;
  int num_words = AS_FLOAT_NUM_WORDS(num_bits);

  for (num_words--; num_words >= 0; num_words--)
  {
    tmp1 = p_add[num_words] + p_addend[num_words];
    carry1 = tmp1 < p_add[num_words];
    tmp2 = tmp1 + carry;
    carry2 = tmp2 < tmp1;
    p_sum[num_words] = tmp2;
#if DBG_MANT_ADD
    fprintf(stderr, "mantissa_add[%u] %08x %08x %u -> %08x %u\n", num_words,
            (unsigned)p_add[num_words], (unsigned)p_addend[num_words], carry,
            (unsigned)p_sum[num_words], carry1 | carry2);
#endif
    carry = carry1 | carry2;
  }
  return carry;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_add_bit(as_float_mant_t p_sum, const as_float_mant_t p_add, unsigned add_bit_pos, unsigned num_bits)
 * \brief  add one bit to mantissa
 * \param  p_sum sum buffer
 * \param  p_add operand
 * \param  add_bit_pos position of bit to add (0 = MSB)
 * \param  num_bits mantissa length
 * \return carry of MSB add
 * ------------------------------------------------------------------------ */

as_float_mant_word_t as_float_mantissa_add_bit(as_float_mant_t p_sum, const as_float_mant_t p_add, unsigned add_bit_pos, unsigned num_bits)
{
  as_float_mant_t addend;

  memset(addend, 0, sizeof addend);
  addend[add_bit_pos / AS_FLOAT_MANT_WORD_BITS] |= 1ul << (AS_FLOAT_MANT_WORD_BITS_M1 - (add_bit_pos % AS_FLOAT_MANT_WORD_BITS));
  return mantissa_add(p_sum, p_add, addend, num_bits);
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_shift_right(as_float_mant_t p_mantissa, as_float_mant_word_t shiftin_bit, unsigned num_bits)
 * \brief  shift right mantissa one bit
 * \param  p_mantissa mantissa to shift
 * \param  shiftin_bit MSB to shift in from left
 * \param  num_bits bit length of mantissa
 * \return LSB of mantissa shifted out
 * ------------------------------------------------------------------------ */

as_float_mant_word_t as_float_mantissa_shift_right(as_float_mant_t p_mantissa, as_float_mant_word_t shiftin_bit, unsigned num_bits)
{
  int num_words = AS_FLOAT_NUM_WORDS(num_bits), z;
  unsigned last_num_bits;
  as_float_mant_word_t shiftout_bit = 0;

  if (!(last_num_bits = num_bits % AS_FLOAT_MANT_WORD_BITS))
    last_num_bits = AS_FLOAT_MANT_WORD_BITS;
  for (z = 0; z < num_words; z++)
  {
    shiftout_bit = (p_mantissa[z] >> ((z == num_words - 1) ? (AS_FLOAT_MANT_WORD_BITS - last_num_bits) : 0)) & 1;
    p_mantissa[z] = ((p_mantissa[z] >> 1) & MASK(AS_FLOAT_MANT_WORD_BITS_M1));
    if ((z == num_words - 1) && (last_num_bits < AS_FLOAT_MANT_WORD_BITS))
      p_mantissa[z] &= ~MASK(AS_FLOAT_MANT_WORD_BITS - last_num_bits);
    p_mantissa[z] |= (shiftin_bit << AS_FLOAT_MANT_WORD_BITS_M1);
    shiftin_bit = shiftout_bit;
  }
  return shiftout_bit;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_shift_left(as_float_mant_t p_mantissa, as_float_mant_word_t shiftin_bit, unsigned num_bits)
 * \brief  shift left mantissa one bit
 * \param  p_mantissa mantissa to shift
 * \param  shiftin_bit MSB to shift in from right
 * \param  num_bits bit length of mantissa
 * \return MSB of mantissa shifted out
 * ------------------------------------------------------------------------ */

as_float_mant_word_t as_float_mantissa_shift_left(as_float_mant_t p_mantissa, as_float_mant_word_t shiftin_bit, unsigned num_bits)
{
  int num_words = AS_FLOAT_NUM_WORDS(num_bits), z;
  as_float_mant_word_t shiftout_bit = 0;

  for (z = num_words - 1; z >= 0; z--)
  {
    shiftout_bit = (p_mantissa[z] >> AS_FLOAT_MANT_WORD_BITS_M1) & 1;
    p_mantissa[z] = ((p_mantissa[z] << 1) & (MASK(AS_FLOAT_MANT_WORD_BITS_M1) << 1));
    p_mantissa[z] &= mantissa_used_bit_mask(num_bits, z);
    p_mantissa[z] |= shiftin_bit;
    shiftin_bit = shiftout_bit;
  }
  return shiftout_bit;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_is_zero_from(const as_float_dissect_t *p_num, unsigned start_bit)
 * \brief  is mantissa all-zeros, starting at given bit?
 * \param  p_num number to check
 * \param  start_bit starting bit position
 * \return True if zero
 * ------------------------------------------------------------------------ */

Boolean as_float_mantissa_is_zero_from(const as_float_dissect_t *p_num, unsigned start_bit)
{
  if (start_bit < p_num->mantissa_bits)
  {
    as_float_mant_word_t sum = 0, mask;
    unsigned num_words = AS_FLOAT_NUM_WORDS(p_num->mantissa_bits), z,
             start_word = start_bit / AS_FLOAT_MANT_WORD_BITS,
             start_bit_in_word = start_bit % AS_FLOAT_MANT_WORD_BITS;

    for (z = start_word; z < num_words; z++)
    {
      mask = mantissa_used_bit_mask(p_num->mantissa_bits, z);
      if ((z == start_word) && start_bit_in_word)
        mask &= MASK(AS_FLOAT_MANT_WORD_BITS - start_bit_in_word);
      sum |= p_num->mantissa[z] & mask;
    }
    return !sum;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_is_allones_from(const as_float_dissect_t *p_num, unsigned start_bit)
 * \brief  is mantissa all-ones, starting at given bit?
 * \param  p_num number to check
 * \param  start_bit starting bit position
 * \return True if zero
 * ------------------------------------------------------------------------ */

Boolean as_float_mantissa_is_allones_from(const as_float_dissect_t *p_num, unsigned start_bit)
{
  if (start_bit < p_num->mantissa_bits)
  {
    unsigned num_words = AS_FLOAT_NUM_WORDS(p_num->mantissa_bits), z,
             start_word = start_bit / AS_FLOAT_MANT_WORD_BITS,
             start_bit_in_word = start_bit % AS_FLOAT_MANT_WORD_BITS;
    as_float_mant_word_t mask;

    for (z = start_word; z < num_words; z++)
    {
      mask = mantissa_used_bit_mask(p_num->mantissa_bits, z);
      if ((z == start_word) && start_bit_in_word)
        mask &= MASK(AS_FLOAT_MANT_WORD_BITS - start_bit_in_word);
      if ((p_num->mantissa[z] & mask) != mask)
        return False;
    }
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_extract(const as_float_dissect_t *p_num, unsigned start_bit, unsigned num_bits)
 * \brief  extract bit field from mantissa
 * \param  p_num mantissa source
 * \param  start_bit start (highest, leftmost) position
 * \param  num_bits bit field length
 * \return extracted field
 * ------------------------------------------------------------------------ */

LongWord as_float_mantissa_extract(const as_float_dissect_t *p_num, unsigned start_bit, unsigned num_bits)
{
  LongWord ret = 0;
  unsigned this_num_bits, this_word, end_bit, max_end_bit;

  if ((start_bit >= p_num->mantissa_bits)
   || (start_bit + num_bits > p_num->mantissa_bits))
    return ret;

  while (num_bits > 0)
  {
    this_word = start_bit / AS_FLOAT_MANT_WORD_BITS;
    end_bit = start_bit + num_bits;
    max_end_bit = start_bit + (AS_FLOAT_MANT_WORD_BITS - (start_bit % AS_FLOAT_MANT_WORD_BITS));
    if (end_bit > max_end_bit)
      end_bit = max_end_bit;
    this_num_bits = end_bit - start_bit;
    ret = (ret << this_num_bits) | ((p_num->mantissa[this_word] >> (max_end_bit - end_bit)) & MASK(this_num_bits));
    start_bit += this_num_bits;
    num_bits -= this_num_bits;
  }
  return ret;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_zero(as_float_dissect_t *p_dest)
 * \brief  initialize / zero dissected fload
 * \param  p_dest what to init
 * ------------------------------------------------------------------------ */

void as_float_zero(as_float_dissect_t *p_dest)
{
  p_dest->fp_class = AS_FP_NORMAL;
  p_dest->negative = False;
  p_dest->exponent = 0;
  p_dest->mantissa_bits = 0;
  memset (p_dest->mantissa, 0, sizeof p_dest->mantissa);
}

/*!------------------------------------------------------------------------
 * \fn     as_float_mantissa_twos_complement(as_float_dissect_t *p_num)
 * \brief  transform mantissa into two's complement
 * \param  p_num number to transform
 * ------------------------------------------------------------------------ */

void as_float_mantissa_twos_complement(as_float_dissect_t *p_num)
{
  /* mantissa becomes one bit longer, due to the leading sign bit */

  as_float_append_mantissa_bits(p_num, 0, 1);
  as_float_mantissa_shift_right(p_num->mantissa, 0, p_num->mantissa_bits);
  if (p_num->negative)
  {
    unsigned num_words = AS_FLOAT_NUM_WORDS(p_num->mantissa_bits), z;

    for (z = 0; z < num_words; z++)
    {
      as_float_mant_word_t mask = mantissa_used_bit_mask(p_num->mantissa_bits, z);
      p_num->mantissa[z] = (p_num->mantissa[z] ^ mask) & mask;
    }
    as_float_mantissa_add_bit(p_num->mantissa, p_num->mantissa, p_num->mantissa_bits - 1, p_num->mantissa_bits);
    p_num->negative = False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_float_append_mantissa_bits(as_float_dissect_t *p_dest, LongWord new_bits, unsigned new_num_bits)
 * \brief  extend mantissa of dissected float
 * \param  p_dest what to extend
 * \param  new_bits bits to append
 * \param  new_num_bits how many bits to append from new_bits?
 * ------------------------------------------------------------------------ */

extern void as_float_append_mantissa_bits(as_float_dissect_t *p_dest, LongWord new_bits, unsigned new_num_bits)
{
#if DBG_MANT_APPEND
  fprintf(stderr, "append_mantissa_bits %u curr %u\n", new_num_bits, p_dest->mantissa_bits);
#endif
  while (new_num_bits > 0)
  {
    unsigned rem_num_bits = AS_FLOAT_MANT_WORD_BITS - (p_dest->mantissa_bits & AS_FLOAT_MANT_WORD_BITS_M1),
             this_num_bits = (rem_num_bits < new_num_bits) ? rem_num_bits : new_num_bits,
             dest_pos = rem_num_bits - this_num_bits,
             src_pos = new_num_bits - this_num_bits,
             lowest_mantissa = p_dest->mantissa_bits / AS_FLOAT_MANT_WORD_BITS;
    if (AS_FLOAT_MANT_WORD_BITS == rem_num_bits)
      p_dest->mantissa[lowest_mantissa] = (as_float_mant_word_t)0;

#if DBG_MANT_APPEND
    fprintf(stderr, " append %u from %u to %u\n", this_num_bits, src_pos, dest_pos);
#endif
    p_dest->mantissa[lowest_mantissa] |= ((new_bits >> src_pos) & MASK(this_num_bits)) << dest_pos;

    new_num_bits -= this_num_bits;
    p_dest->mantissa_bits += this_num_bits;
  }
}

/*!------------------------------------------------------------------------
 * \fn     as_float_remove_mantissa_bits(as_float_dissect_t *p_num, unsigned remove_num_bits)
 * \brief  remove given # of bits from mantissa at end
 * \param  p_num number to modify
 * \param  remove_num_bits # of bits to remove
 * ------------------------------------------------------------------------ */

void as_float_remove_mantissa_bits(as_float_dissect_t *p_num, unsigned remove_num_bits)
{
  unsigned new_num_bits, new_num_words, this_num_words, last_num_bits;

  if (!p_num->mantissa_bits)
    return;
  if (remove_num_bits >= p_num->mantissa_bits)
    remove_num_bits = p_num->mantissa_bits - 1;

  new_num_bits = p_num->mantissa_bits - remove_num_bits;
  new_num_words = AS_FLOAT_NUM_WORDS(new_num_bits);
  this_num_words = AS_FLOAT_NUM_WORDS(p_num->mantissa_bits);
  if (new_num_words < this_num_words)
    memset(&p_num->mantissa[new_num_words], 0, (this_num_words - new_num_words) * sizeof(*p_num->mantissa));
  last_num_bits = new_num_bits % AS_FLOAT_MANT_WORD_BITS;
  if (last_num_bits)
    p_num->mantissa[new_num_words - 1] &= ~MASK(AS_FLOAT_MANT_WORD_BITS - last_num_bits);
  p_num->mantissa_bits = new_num_bits;
}

/*!------------------------------------------------------------------------
 * \fn     void as_float_round(as_float_dissect_t *p_num, unsigned target_bits)
 * \brief  round/extend float to the given number of mantissa bits
 * \param  p_num number to round
 * \param  target_bits # of mantissa bits to achieve
 * \return type of rounding performed
 * ------------------------------------------------------------------------ */

as_float_round_t as_float_round(as_float_dissect_t *p_num, unsigned target_bits)
{
  as_float_round_t round_type = e_round_none;

  /* if the current mantissa is shorter, append zeros: */

  while (p_num->mantissa_bits < target_bits)
  {
    unsigned delta = target_bits - p_num->mantissa_bits;

    if (delta > AS_FLOAT_MANT_WORD_BITS)
      delta = AS_FLOAT_MANT_WORD_BITS;
    as_float_append_mantissa_bits(p_num, 0, delta);
  }

  /* Rounding: round-to-the nearest: */

  if (p_num->mantissa_bits > target_bits)
  {
    /* The bit representing 0.5 digits is zero: fractional part is < 0.5, round down */

    if (!as_float_get_mantissa_bit(p_num->mantissa, p_num->mantissa_bits, target_bits));
    else
    {
      /* any lower bit in fraction is also set: fractional part is > 0.5, round up */

      if (!as_float_mantissa_is_zero_from(p_num, target_bits + 1))
        round_type = e_round_up;

      /* fractional part is exactly 0.5: round towards even, i.e if mantissa part to be kept is odd: */

      else if (as_float_get_mantissa_bit(p_num->mantissa, p_num->mantissa_bits, target_bits - 1))
        round_type = e_round_up;
    }

    if (round_type == e_round_up)
    {
      as_float_mant_t sum;
      as_float_mant_word_t carry;

      /* round up: add 0.5 LSB: */

      carry = as_float_mantissa_add_bit(sum, p_num->mantissa, target_bits, p_num->mantissa_bits);

      /* shift mantissa if carry: */

      if (carry)
      {
        as_float_mantissa_shift_right(sum, carry, p_num->mantissa_bits);
        p_num->exponent++;
      }
      memcpy(p_num->mantissa, sum, sizeof p_num->mantissa);
    }

    /* Store whether we are actually rounding down of just truncating zeros: */

    else if (!as_float_mantissa_is_zero_from(p_num, target_bits + 1))
      round_type = e_round_down;

    /* Truncate mantissa after possible rounding */

    as_float_remove_mantissa_bits(p_num, p_num->mantissa_bits - target_bits);
  }

  return round_type;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_dump_mantissa(FILE *p_dest, const as_float_mant_t p_mantissa, unsigned mantissa_bits)
 * \brief  dump mantissa of as_float_dissect_t
 * \param  p_mantissa mantissa bits
 * \param  mantissa_bits mantissa length
 * ------------------------------------------------------------------------ */

void as_float_dump_mantissa(FILE *p_dest, const as_float_mant_t p_mantissa, unsigned mantissa_bits)
{
  unsigned mant_idx, num_mant_words = AS_FLOAT_NUM_WORDS(mantissa_bits),
           num_last_bits = mantissa_bits % AS_FLOAT_MANT_WORD_BITS;
  char str[20];

  for (mant_idx = 0; mant_idx < num_mant_words; mant_idx++)
  {
    as_snprintf(str, sizeof str, "%0*lx", AS_FLOAT_MANT_WORD_BITS / 4, (unsigned long)p_mantissa[mant_idx]);
    if ((mant_idx == num_mant_words - 1) && num_last_bits)
      str[(num_last_bits + 3) / 4] = '\0';
    fprintf(p_dest, " %s", str);
  }
  fprintf(p_dest, " (%u)", mantissa_bits);
}

/*!------------------------------------------------------------------------
 * \fn     as_float_dump(FILE *p_dest, const char *p_name, const as_float_dissect_t *p_float)
 * \brief  dump contents of dissected float
 * \param  p_dest where to dump
 * \param  p_name optional title
 * \param  p_float what to dump
 * ------------------------------------------------------------------------ */

void as_float_dump(FILE *p_dest, const char *p_name, const as_float_dissect_t *p_float)
{
  if (p_name)
    fprintf(p_dest, "%s: ", p_name);
  fprintf(p_dest, "negative %u exp %d mant", p_float->negative, p_float->exponent);
  as_float_dump_mantissa(p_dest, p_float->mantissa, p_float->mantissa_bits);
  fprintf(p_dest, "\n");
}
