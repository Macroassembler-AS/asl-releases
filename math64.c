/* bpemu.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* 64 bit arithmetic for platforms not having a 64 bit integer               */
/*                                                                           */
/*****************************************************************************/

#include <assert.h>

#include "stdinc.h"
#include "math64.h"

/*!------------------------------------------------------------------------
 * \fn     nlz16(Word n)
 * \brief  find # of tleading zeros
 * \param  n 16-bit word to examine
 * \return # of leading zeros
 * ------------------------------------------------------------------------ */

static int nlz16(Word n)
{
  unsigned z;
  for (z = 0; z < 16; z++)
  {
    if (n & 0x8000u)
      break;
    n = n << 1;
  }
  return z;
}

/*!------------------------------------------------------------------------
 * \fn     to16(Word *pOut, const t64 *pIn)
 * \brief  Convert t64 to array of uint16 needed by mul/div
 * \param  pOut destination array
 * \param  pIn source value
 * \return # of words written (1..4)
 * ------------------------------------------------------------------------ */

static int to16(Word *pOut, const t64 *pIn)
{
  int res = 0;

  pOut[res++] = pIn->low & 0xffff;
  pOut[res++] = (pIn->low >> 16) & 0xffff;
  pOut[res++] = pIn->high & 0xffff;
  pOut[res++] = (pIn->high >> 16) & 0xffff;
  while ((res > 1) && !pOut[res - 1])
    res--;
  return res;
}

/*!------------------------------------------------------------------------
 * \fn     from16(t64 *pOut, const Word *pIn)
 * \brief  convert uint16 array back to t64
 * \param  pOut value to reconstruct
 * \param  pIn uint16 array
 * ------------------------------------------------------------------------ */

static void from16(t64 *pOut, const Word *pIn)
{
  pOut->low = pIn[1];
  pOut->low = (pOut->low << 16) | pIn[0];
  pOut->high = pIn[3];
  pOut->high = (pOut->high << 16) | pIn[2];
}

/*!------------------------------------------------------------------------
 * \fn     mulmnu(Word w[], Word u[], Word v[], int m, int n)
 * \brief  unsigned multi-precision multiply (taken from Hacker's Delight)
 * \param  w result buffer
 * \param  u, v numbers to multiply
 * \param  m length of u in words
 * \param  n length of v in words
 * ------------------------------------------------------------------------ */

void mulmnu(Word w[], Word u[], Word v[], int m, int n)
{
  unsigned int k, t;
  int i, j;

  for (i = 0; i < m; i++)
    w[i] = 0;

  for (j = 0; j < n; j++)
  {
    k = 0;
    for (i = 0; i < m; i++)
    {
      t = u[i]*v[j] + w[i + j] + k;
      w[i + j] = t;          /* (I.e., t & 0xFFFF). */
      k = t >> 16;
    }
    w[j + m] = k;
  }
}

/*!------------------------------------------------------------------------
 * \fn     mulmnu(Word q[], Word r[], Word u[], Word v[], int m, int n)
 * \brief  unsigned multi-precision divide (taken from Hacker's Delight)
 * \param  q quotient buffer
 * \param  r remainder buffer (may be NULL)r
 * \param  u divident
 * \param  v divisor
 * \param  m length of u in words
 * \param  n length of v in words
 * \return 0 on success, 1 on error
 * ------------------------------------------------------------------------ */

static int divmnu(Word q[], Word r[], const Word u[], const Word v[], int m, int n)
{
  const unsigned b = 65536; /* Number base (16 bits). */
  Word un[16], vn[16]; /* Normalized form of u, v. */
  unsigned qhat;   /* Estimated quotient digit. */
  unsigned rhat;   /* A remainder. */
  unsigned p;      /* Product of two digits. */
  int s, i, j, t, k;

  /* Return if invalid param. */

  if (m < n || n <= 0 || v[n - 1] == 0)
    return 1;

  /* Take care of the case of a single-digit divisor here. */

  if (n == 1)
  {
    k = 0;
    for (j = m - 1; j >= 0; j--)
    {
      q[j] = (k*b + u[j]) / v[0];
      k = (k * b + u[j]) - q[j] * v[0];
    }
    if (r != NULL) r[0] = k;
    return 0;
  }

  /* Normalize by shifting v left just enough so that
     its high-order bit is on, and shift u left the
     same amount. We may have to append a high-order
     digit on the dividend; we do that unconditionally. */

  s = nlz16(v[n - 1]); /* 0 <= s <= 16. */
  assert(n < 16); /* vn = (Word *)alloca(2 * n); */
  for (i = n - 1; i > 0; i--)
    vn[i] = (v[i] << s) | (v[i - 1] >> (16 - s));
  vn[0] = v[0] << s;
  assert(m + 1 < 16); /* un = (Word *)alloca(2 * (m + 1)); */
  un[m] = u[m-1] >> (16 - s);
  for (i = m - 1; i > 0; i--)
    un[i] = (u[i] << s) | (u[i - 1] >> (16 - s));
  un[0] = u[0] << s;

  /* Main loop. */

  for (j = m - n; j >= 0; j--)
  {
    /* Compute estimate qhat of q[j]. */

    qhat = (un[j+n]*b + un[j+n-1])/vn[n-1];
    rhat = (un[j+n]*b + un[j+n-1]) - qhat*vn[n-1];
  again:
    if (qhat >= b || qhat*vn[n-2] > b*rhat + un[j+n-2])
    {
      qhat = qhat - 1;
      rhat = rhat + vn[n-1];
      if (rhat < b) goto again;
    }

    /* Multiply and subtract. */

    k = 0;
    for (i = 0; i < n; i++)
    {
      p = qhat*vn[i];
      t = un[i + j] - k - (p & 0xFFFF);
      un[i+j] = t;
      k = (p >> 16) - (t >> 16);
    }
    t = un[j+n] - k;
    un[j + n] = t;

    /* Store quotient digit. */

    q[j] = qhat;

    /* If we subtracted too much, add back. */

    if (t < 0)
    {
      q[j] = q[j] - 1;
      k = 0;
      for (i = 0; i < n; i++)
      {
        t = un[i + j] + vn[i] + k;
        un[i + j] = t;
        k = t >> 16;
      }
      un[j+n] = un[j+n] + k;
    }
  } /* End j. */

  /* If the caller wants the remainder, unnormalize
     it and pass it back. */

  if (r != NULL)
  {
    for (i = 0; i < n; i++)
      r[i] = (un[i] >> s) | (un[i + 1] << (16 - s));
  }
  return 0;
}

/*!------------------------------------------------------------------------
 * \fn     add64(t64 *pRes, const t64 *pA, const t64 *pB)
 * \brief  add two values
 * \param  pRes sum buffer
 * \param  pA, pB values to add
 * ------------------------------------------------------------------------ */

void add64(t64 *pRes, const t64 *pA, const t64 *pB)
{
  pRes->low = pA->low + pB->low;
  pRes->high = pA->high + pB->high + !!(pRes->low < pA->low);
}

/*!------------------------------------------------------------------------
 * \fn     sub64(t64 *pRes, const t64 *pA, const t64 *pB)
 * \brief  subtract two values
 * \param  pRes difference buffer
 * \param  pA, pB values to subtract
 * ------------------------------------------------------------------------ */

void sub64(t64 *pRes, const t64 *pA, const t64 *pB)
{
  pRes->low = pA->low - pB->low;
  pRes->high = pA->high - pB->high - !!(pRes->low > pA->low);
}

/*!------------------------------------------------------------------------
 * \fn     mul64(t64 *pRes, const t64 *pA, const t64 *pB)
 * \brief  multiply two values
 * \param  pRes product buffer
 * \param  pA, pB values to multiply
 * ------------------------------------------------------------------------ */

void mul64(t64 *pRes, const t64 *pA, const t64 *pB)
{
  Word u[4], v[4], w[8];
  int m = to16(u, pA),
      n = to16(v, pB);

  mulmnu(w, u, v, m, n);
  from16(pRes, w);
}

/*!------------------------------------------------------------------------
 * \fn     div64(t64 *pRes, const t64 *pA, const t64 *pB)
 * \brief  divide two values
 * \param  pRes quotient buffer
 * \param  pA, pB values to divide
 * ------------------------------------------------------------------------ */

void div64(t64 *pRes, const t64 *pA, const t64 *pB)
{
  Word u[4], v[4], q[4];
  int m = to16(u, pA),
      n = to16(v, pB);

  q[0] = q[1] = q[2] = q[3] = 0;
  divmnu(q, NULL, u, v, m, n);
  from16(pRes, q);
}

/*!------------------------------------------------------------------------
 * \fn     mod64(t64 *pRes, const t64 *pA, const t64 *pB)
 * \brief  modulo-divide two values
 * \param  pRes remainder buffer
 * \param  pA, pB values to divide
 * ------------------------------------------------------------------------ */

void mod64(t64 *pRes, const t64 *pA, const t64 *pB)
{
  Word u[4], v[4], q[4], r[4];
  int m = to16(u, pA),
      n = to16(v, pB);

  q[0] = q[1] = q[2] = q[3] = 0;
  divmnu(q, r, u, v, m, n);
  from16(pRes, r);
}

/*!------------------------------------------------------------------------
 * \fn     mod_div64(t64 *pQuot, t64 *pRem, const t64 *pA, const t64 *pB)
 * \brief  divide two values, and return also remainder
 * \param  pQuot quotient buffer
 * \param  pRem remainder buffer
 * \param  pA, pB values to divide
 * ------------------------------------------------------------------------ */

void mod_div64(t64 *pQuot, t64 *pRem, const t64 *pA, const t64 *pB)
{
  Word u[4], v[4], q[4], r[4];
  int m = to16(u, pA),
      n = to16(v, pB);

  q[0] = q[1] = q[2] = q[3] = 0;
  divmnu(q, r, u, v, m, n);
  from16(pQuot, q);
  from16(pRem, r);
}
