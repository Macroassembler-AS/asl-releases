/* aplfloat.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* APPLE<->IEEE Floating Point Conversion on host                            */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <errno.h>

#include "errmsg.h"
#include "asmerr.h"
#include "strcomp.h"
#include "ieeefloat.h"
#include "aplfloat.h"

/*!------------------------------------------------------------------------
 * \fn     Double_2_apl4(Double inp, Word *p_dest)
 * \brief  convert from host to Apple II 4 byte float format
 * \param  inp value to dispose
 * \param  p_dest where to dispose
 * \return 0 or error code
 * ------------------------------------------------------------------------ */

int Double_2_apl4(Double inp, Word *p_dest)
{
  Word sign;
  Integer exponent;
  LongWord mantissa, fraction;
  Boolean round_up;

  /* Dissect IEEE number.  (Absolute of) mantissa is already in range 2.0 > M >= 1.0,
     unless denormal. NaN and Infinity cannot be represented: */

  ieee8_dissect(&sign, &exponent, &mantissa, &fraction, inp);
  if (exponent == 2047)
    return EINVAL;

  /* (3) Denormalize small numbers: */

#if 0
  printf("0x%08x 0x%08x\n", (unsigned)mantissa, (unsigned)fraction);
#endif
  while ((exponent < -128) && (mantissa || fraction))
  {
    mantissa++;
    fraction = ((fraction >> 1) & 0x7ffffful) | ((mantissa & 1) ? 0x800000ul : 0x000000ul);
    mantissa = (mantissa >> 1) & 0x1fffffful;
  }

  /* Two's Complement of mantissa. Note mantissa afterwards has 30 bits, including sign: */

  if (sign)
  {
    fraction = fraction ^ 0x00fffffful;
    mantissa = mantissa ^ 0x7ffffffful;
    if (++fraction >= 0x01000000ul)
    {
      fraction = 0;
      mantissa = (mantissa + 1) & 0x7ffffffful;
      
    }
  }

  /* Normalize, so that topmost bits of mantissa are unequal.  This happens
     for powers of two, after negating:  */

  switch ((mantissa >> 28) & 3)
  {
    case 0:
    case 3:
      exponent--;
      mantissa = ((mantissa << 1) & 0x7ffffffeul) | ((fraction >> 23) & 1);
      fraction = (fraction << 1) & 0xfffffful;
      break;
  }

  /* (4) Round mantissa.  The mantissa currently has 30 bits, and - including sign - we
         will use 24 of them.  So the "half LSB" bit to look at is bit 5: */

  if (mantissa & 0x20) /* >= 0.5 */
  {
    if ((mantissa & 0x1f) || fraction) /* > 0.5 */
      round_up = True;
    else /* == 0.5 */
      round_up = !!(mantissa & 0x40); /* round towards even */
  }
  else /* < 0.5 */
    round_up = False;

  if (round_up)
  {
    LongWord new_mantissa = mantissa + 0x40;

    /* overflow during round-up? */

    if ((new_mantissa ^ mantissa) & 0x40000000ul)
    {
      /* arithmetic right shift, preserving sign */
      mantissa = (mantissa & 0x40000000ul) | ((mantissa >> 1) & 0x3ffffffful);
      exponent++;
    }
    mantissa += 0x40;
  }

  /* After knowing final exponent, check for overflow: */

  if (exponent > 127)
    return E2BIG;

  /* (5) mantissa zero means exponent is also zero */

  if (!mantissa)
    exponent = 0;

  /* (7) Assemble: */

  p_dest[0] = (((exponent + 128) << 8) & 0xff00ul) | ((mantissa >> 22) & 0x00fful);
  p_dest[1] = (mantissa >> 6) & 0xfffful;
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     check_apl_fp_dispose_result(int ret, const struct sStrComp *p_arg)
 * \brief  check the result of Double_2... and throw associated error messages
 * \param  ret return code
 * \param  p_arg associated source argument
 * ------------------------------------------------------------------------ */

Boolean check_apl_fp_dispose_result(int ret, const struct sStrComp *p_arg)
{
  switch (ret)
  {
    case 0:
      return True;
    case E2BIG:
      WrStrErrorPos(ErrNum_OverRange, p_arg);
      return False;
    default:
      WrXErrorPos(ErrNum_InvArg, "INF/NaN", &p_arg->Pos);
      return False;
  }
}
