/* milfloat.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* IEEE -> MIL STD 1750 Floating Point Conversion on host                    */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <errno.h>
#include "as_float.h"
#include "milfloat.h"

/*!------------------------------------------------------------------------
 * \fn     as_float_2_mil1750(as_float_t inp, Word *p_dest, Boolean extended)
 * \brief  convert host float to MIL STD 1750
 * \param  inp number to convert
 * \param  p_dest destination buffer
 * \param  extended convert to 32 or 48 bit format?
 * \return # of words written or < 0 if error
 * ------------------------------------------------------------------------ */

int as_float_2_mil1750(as_float_t inp, Word *p_dest, Boolean extended)
{
  as_float_dissect_t dissect;
  Word or_sum;
  unsigned req_mantissa_bits;

  /* Dissect number: */

  as_float_dissect(&dissect, inp);

  /* NaN and Infinity cannot be represented: */

  if ((dissect.fp_class == AS_FP_NAN)
   || (dissect.fp_class == AS_FP_INFINITE))
    return -EINVAL;

  /* Mantissa is in range [-1,+1) instead of [-2,+2): */

  dissect.exponent++;

  /* If exponent is too small to represent, shift down mantissa
     until exponent is large enough, or mantissa is all-zeroes: */

  while ((dissect.exponent < -128) && !as_float_mantissa_is_zero(&dissect))
  {
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
    dissect.exponent++;
  }

  /* exponent overflow? */

  if (dissect.exponent > 127)
    return -E2BIG;

  /* Form 2s complement of mantissa when sign is set: */

  as_float_mantissa_twos_complement(&dissect);

  /* Normalize, so that topmost bits of mantissa are unequal.  This happens
     for powers of two, after negating: */

  switch (as_float_mantissa_extract(&dissect, 0, 2))
  {
    case 0:
    case 3:
      if (dissect.exponent > -127) /* -128? */
      {
        dissect.exponent--;
        as_float_mantissa_shift_left(dissect.mantissa, 0, dissect.mantissa_bits);
      }
      break;
  }

  /* no rounding: */

  req_mantissa_bits = extended ? 40 : 24;
  if (req_mantissa_bits > dissect.mantissa_bits)
    as_float_append_mantissa_bits(&dissect, 0, req_mantissa_bits - dissect.mantissa_bits);

  /* Write out result: */

  or_sum  = (p_dest[0] = as_float_mantissa_extract(&dissect, 0, 16));
  or_sum |= (p_dest[1] = as_float_mantissa_extract(&dissect, 16, 8) << 8);
  if (extended)
    or_sum |= (p_dest[2] = as_float_mantissa_extract(&dissect, 24, 16));

  /* zero mantissa means zero exponent */

  if (or_sum)
    p_dest[1] |= dissect.exponent & 0xff;

  return extended ? 3 : 2;
}
