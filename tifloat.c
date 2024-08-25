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
