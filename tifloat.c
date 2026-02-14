/* tifloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* IEEE -> TI DSP Floating Point Conversion on host                          */
/*                                                                           */
/*****************************************************************************/

#include <errno.h>

#include "as_float.h"
#include "errmsg.h"
#include "tifloat.h"

/*!------------------------------------------------------------------------
 * \fn     split_exp(as_float_t inp, LongInt *p_exponent, LongWord *p_mantissa)
 * \brief  common number dissection
 * \param  inp float input (host format)
 * \param  p_exponent exponent (without bias)
 * \param  p_mantissa mantissa (two's complement)
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

static int split_exp(as_float_t inp, LongInt *p_exponent, LongWord *p_mantissa)
{
  as_float_dissect_t dissect;

  as_float_dissect(&dissect, inp);
  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  *p_exponent = dissect.exponent;
  if (dissect.mantissa_bits < 32)
    as_float_append_mantissa_bits(&dissect, 0, 32 - dissect.mantissa_bits);
  *p_mantissa = as_float_mantissa_extract(&dissect, 0, 32);
  if (dissect.negative)
    *p_mantissa = (0xffffffff - *p_mantissa) + 1;
  *p_mantissa = (*p_mantissa) ^ 0x80000000;

  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ti2(as_float_t inp, Word *p_dest)
 * \brief  convert host float to C3x/4x short (16 bits)
 * \param  inp float input (host format)
 * \param  result buffer
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int as_float_2_ti2(as_float_t inp, Word *p_dest)
{
  int ret;
  LongInt exponent;
  LongWord mantissa;

  if (inp == 0)
  {
    *p_dest = 0x8000;
    return 0;
  }

  if ((ret = split_exp(inp, &exponent, &mantissa)) < 0)
    return ret;
  if (!ChkRange(exponent, -7, 7))
    return -E2BIG;
  *p_dest = ((exponent << 12) & 0xf000) | ((mantissa >> 20) & 0xfff);
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ti4(as_float_t inp, LongWord *p_dest)
 * \brief  convert host float to C3x/4x single (32 bits)
 * \param  inp float input (host format)
 * \param  result buffer
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int as_float_2_ti4(as_float_t inp, LongWord *p_dest)
{
  int ret;
  LongInt exponent;
  LongWord mantissa;

  if (inp == 0)
  {
    *p_dest = 0x80000000;
    return 0;
  }

  if ((ret = split_exp(inp, &exponent, &mantissa)) < 0)
    return ret;
  if (!ChkRange(exponent, -127, 127))
    return -E2BIG;
  *p_dest = ((exponent << 24) & 0xff000000) + (mantissa >> 8);
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_ti5(as_float_t inp, LongWord *p_dest_l, LongWord *p_dest_h)
 * \brief  convert host float to C3x/4x extended (40 bits)
 * \param  inp float input (host format)
 * \param  p_dest_l result buffer (mantissa)
 * \param  p_dest_h result buffer (exponent)
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int as_float_2_ti5(as_float_t inp, LongWord *p_dest_l, LongWord *p_dest_h)
{
  int ret;
  LongInt exponent;

  if (inp == 0)
  {
    *p_dest_h = 0x80;
    *p_dest_l = 0x00000000;
    return 0;
  }

  if ((ret = split_exp(inp, &exponent, p_dest_l)) < 0)
    return ret;
  if (!ChkRange(exponent, -127, 127))
    return -E2BIG;
  *p_dest_h = exponent & 0xff;
 return 0;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_tms340_flt4(as_float_t inp, Byte *p_dest, Boolean needs_big)
 * \brief  convert float to TMS340 single (32 bit) float binary representation
 * \param  inp input number
 * \param  p_dest where to write binary data
 * \param  needs_big req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_tms340_flt4(as_float_t inp, Byte *p_dest, Boolean needs_big)
{
  as_float_dissect_t dissect;
  int index_mask = needs_big ? 3 : 0;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* No idea if Inf/NaN can be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  /* Format uses explicit leading one representing 1.0 portion in mantissa.
     as_float_dissect already delivers that: */

  as_float_round(&dissect, 23);

  /* Bias is 128: */

  if (dissect.exponent > 127)
    return -E2BIG;
  dissect.exponent += 128;

  p_dest[3 ^ index_mask] = ((dissect.negative & 1) << 7)
                         | ((dissect.exponent >> 1) & 0x7f);
  p_dest[2 ^ index_mask] = ((dissect.exponent & 1) << 7)
                         | as_float_mantissa_extract(&dissect, 0, 7);
  p_dest[1 ^ index_mask] = as_float_mantissa_extract(&dissect, 7, 8);
  p_dest[0 ^ index_mask] = as_float_mantissa_extract(&dissect, 15, 8);
  return 4;
}

/*!------------------------------------------------------------------------
 * \fn     as_float_2_tms340_flt8(as_float_t inp, Byte *p_dest, Boolean needs_big)
 * \brief  convert float to TMS340 double (64 bit) float binary representation
 * \param  inp input number
 * \param  p_dest where to write binary data
 * \param  needs_big req's big endian?
 * \return >=0 if conversion was successful, <0 for error
 * ------------------------------------------------------------------------ */

int as_float_2_tms340_flt8(as_float_t inp, Byte *p_dest, Boolean needs_big)
{
  as_float_dissect_t dissect;
  int index_mask = needs_big ? 7 : 0;

  /* Dissect */

  as_float_dissect(&dissect, inp);

  /* No idea if Inf/NaN can be represented in target format: */

  if ((dissect.fp_class != AS_FP_NORMAL)
   && (dissect.fp_class != AS_FP_SUBNORMAL))
    return -EINVAL;

  /* Format uses explicit leading one representing 1.0 portion in mantissa.
     as_float_dissect already delivers that: */

  as_float_round(&dissect, 52);

  /* Bias is 1024: */

  if (dissect.exponent > 1023)
    return -E2BIG;
  dissect.exponent += 1024;

  p_dest[7 ^ index_mask] = ((dissect.negative & 1) << 7)
                         | ((dissect.exponent >> 4) & 0x7f);
  p_dest[6 ^ index_mask] = ((dissect.exponent & 15) << 4)
                         | as_float_mantissa_extract(&dissect, 0, 4);
  p_dest[5 ^ index_mask] = as_float_mantissa_extract(&dissect, 4, 8);
  p_dest[4 ^ index_mask] = as_float_mantissa_extract(&dissect, 12, 8);
  p_dest[3 ^ index_mask] = as_float_mantissa_extract(&dissect, 20, 8);
  p_dest[2 ^ index_mask] = as_float_mantissa_extract(&dissect, 28, 8);
  p_dest[1 ^ index_mask] = as_float_mantissa_extract(&dissect, 36, 8);
  p_dest[0 ^ index_mask] = as_float_mantissa_extract(&dissect, 44, 8);
  return 8;
}
