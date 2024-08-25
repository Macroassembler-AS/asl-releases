/* ibmfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* IBM Floating Point Format                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "stdinc.h"
#include <math.h>
#include <string.h>

#include "as_float.h"
#include "errmsg.h"
#include "asmerr.h"
#include "ibmfloat.h"

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ibm_float(Word *pDest, as_float_t Src, Boolean ToDouble)
 * \brief  convert floating point number to IBM single precision format
 * \param  pDest where to write result (2 words)
 * \param  Src floating point number to store
 * \param  ToDouble convert to double precision (64 instead of 32 bits)?
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int as_float_2_ibm_float(Word *pDest, as_float_t Src, Boolean ToDouble)
{
  as_float_dissect_t dissect;
  unsigned dest_num_bits;

  /* (1) Dissect IEEE number */

  as_float_dissect(&dissect, Src);

  /* NaN or infinity not representable: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  /* IBM format mantissa is in range [0.5,1) instead of [1,2): */

  dissect.exponent++;

  /* (2) Convert 2^n exponent to multiple of four since IBM float exponent is to the base of 16.
         Note that before shifting, we ad a zero bit at the end to avoid losing precision: */

  while (dissect.exponent & 3)
  {
    as_float_append_mantissa_bits(&dissect, 0, 1);
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
    dissect.exponent++;
  }

  /* (3) make base-16 exponent explicit */

  dissect.exponent /= 4;

  /* (4) Round to target precision.  We cannot use as_float_round() for rounding
         since the exponent is already base-16: */

  dest_num_bits = ToDouble ? 56 : 24;
  if (dest_num_bits > dissect.mantissa_bits)
    as_float_round(&dissect, dest_num_bits);
  else if (dest_num_bits < dissect.mantissa_bits)
  {
    Boolean round_up;

    if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dest_num_bits))
    {
      if (!as_float_mantissa_is_zero_from(&dissect, dest_num_bits + 1)) /* > 0.5 */
        round_up = True;
      else
        round_up = !!as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, dest_num_bits - 1);
    }
    else /* < 0.5 */
      round_up = False;

    if (round_up)
    {
      as_float_mant_word_t carry;
      as_float_mant_t sum;

      carry = as_float_mantissa_add_bit(sum, dissect.mantissa, dest_num_bits, dissect.mantissa_bits);
      if (carry)
      {
        as_float_mantissa_shift_right(sum, carry, dissect.mantissa_bits);
        as_float_mantissa_shift_right(sum, 0, dissect.mantissa_bits);
        as_float_mantissa_shift_right(sum, 0, dissect.mantissa_bits);
        as_float_mantissa_shift_right(sum, 0, dissect.mantissa_bits);
        dissect.exponent++;
      }
      memcpy(dissect.mantissa, sum, sizeof dissect.mantissa);
    }
    as_float_remove_mantissa_bits(&dissect, dissect.mantissa_bits - dest_num_bits);
  }

  /* Overrange? */

  if (dissect.exponent > 63)
    return -E2BIG;

  /* number that is too small may degenerate to 0: */

  while ((dissect.exponent < -64) && !as_float_mantissa_is_zero(&dissect))
  {
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
    dissect.exponent++;
  }

  /* Zero shall get zero (-64) as exponent: */

  if (as_float_mantissa_is_zero(&dissect))
    dissect.exponent = -64;

  if (dissect.exponent < -64)
    dissect.exponent = -64;

  /* add bias to exponent */

  dissect.exponent += 64;

  /* store result: */

  pDest[0] = (dissect.negative << 15)
           | ((dissect.exponent << 8) & 0x7f00)
           | as_float_mantissa_extract(&dissect, 0, 8);
  pDest[1] = as_float_mantissa_extract(&dissect, 8, 16);
  if (ToDouble)
  {
    pDest[2] = as_float_mantissa_extract(&dissect, 24, 16);
    pDest[3] = as_float_mantissa_extract(&dissect, 40, 16);
  }

  return 0;
}
