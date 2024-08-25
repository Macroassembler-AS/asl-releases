/* necfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* NEC<->IEEE Floating Point Conversion on host                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <math.h>
#include <errno.h>
#include <string.h>

#include "as_float.h"
#include "necfloat.h"

/*!------------------------------------------------------------------------
 * \fn     as_float_2_nec_4(as_float_t inp, LongWord *p_dest)
 * \brief  convert from host to NEC 4 byte float format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return >0 for number of words used (1) or <0 for error code
 * ------------------------------------------------------------------------ */

int as_float_2_nec_4(as_float_t inp, LongWord *p_dest)
{
  Boolean round_up;
  as_float_dissect_t dissect;
  LongWord mantissa;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* Inf/NaN cannot be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  /* For NEC float, Mantissa is in range 0.5...1.0, instead of 1.0...2.0: */

  dissect.exponent++;

  /* (3) Denormalize small numbers: */

  while ((dissect.exponent < -128) && !as_float_mantissa_is_zero(&dissect))
  {
    dissect.exponent++;
    as_float_mantissa_shift_right(dissect.mantissa, 0, dissect.mantissa_bits);
  }

  /* Build Two's complement.  Note the sign becomes part of the
     mantissa, which is afterwards one bit longer: */

  as_float_mantissa_twos_complement(&dissect);

  /* Normalize, so that topmost bits of mantissa are unequal.  This happens
     for powers of two, after negating: */

  switch (as_float_mantissa_extract(&dissect, 0, 2))
  {
    case 0:
    case 3:
      if (dissect.exponent > -128)
      {
        dissect.exponent--;
        as_float_mantissa_shift_left(dissect.mantissa, 0, dissect.mantissa_bits);
      }
      break;
  }

  /* (4) Round mantissa.  We will use 24 of them.  So the "half LSB" bit
         to look at is bit 24, seen from left: */

  if (as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 24))
  {
    if (!as_float_mantissa_is_zero_from(&dissect, 25)) /* > 0.5 */
      round_up = True;
    else /* == 0.5 */
      round_up = as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 23);
  }
  else /* < 0.5 */
    round_up = False;

  if (round_up)
  {
    as_float_mant_t round_sum;

    (void)as_float_mantissa_add_bit(round_sum, dissect.mantissa, 24, dissect.mantissa_bits);

    /* overflow during round-up? */

    if ((round_sum[0] ^ dissect.mantissa[0]) & 0x80000000ul)
    {
      dissect.exponent++;
      /* Arithmetic right shift of signed number to preserve sign: */
      as_float_mantissa_shift_right(round_sum, as_float_get_mantissa_bit(dissect.mantissa, dissect.mantissa_bits, 0), dissect.mantissa_bits);
    }

    memcpy(dissect.mantissa, round_sum, sizeof(dissect.mantissa));
  }

  /* After knowing final exponent, check for overflow: */

  if (dissect.exponent > 127)
    return -E2BIG;

  /* (5) Mantissa zero means exponent is minimum value */

  if (as_float_mantissa_is_zero(&dissect))
    dissect.exponent = -128;

  /* (6) NEC documentation does not use mantissa value 0x800000 (-1.0) unless
         absolutely necessary? */

  mantissa = as_float_mantissa_extract(&dissect, 0, 24);
  if ((mantissa == 0x800000) && (dissect.exponent < 127))
  {
    mantissa = 0xc00000;
    dissect.exponent++;
  }

  /* (7) Assemble: */

  *p_dest = dissect.exponent & 0xff;
  *p_dest = (*p_dest << 24) | mantissa;

  return 1;
}
