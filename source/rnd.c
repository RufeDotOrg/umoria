/* source/rnd.c: random number generator

   Copyright (C) 1989-2008 James E. Wilson, Robert A. Koeneke,
                           David J. Grabiner

   This file is part of Umoria.

   Umoria is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Umoria is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Umoria.  If not, see <http://www.gnu.org/licenses/>. */
/* Define this to compile as a standalone test */
/* #define TEST_RNG */
/* This alg uses a prime modulus multiplicative congruential generator
   (PMMLCG), also known as a Lehmer Grammer, which satisfies the following
   properties

   (i)   modulus: m - a large prime integer
   (ii)   multiplier: a - an integer in the range 2, 3, ..., m - 1
   (iii) z[n+1] = f(z[n]), for n = 1, 2, ...
   (iv)   f(z) = az mod m
   (v)   u[n] = z[n] / m, for n = 1, 2, ...

   The sequence of z's must be initialized by choosing an initial seed
   z[1] from the range 1, 2, ..., m - 1.  The sequence of z's is a pseudo-
   random sequence drawn without replacement from the set 1, 2, ..., m - 1.
   The u's form a psuedo-random sequence of real numbers between (but not
   including) 0 and 1.

   Schrage's method is used to compute the sequence of z's.
   Let m = aq + r, where q = m div a, and r = m mod a.
   Then f(z) = az mod m = az - m * (az div m) =
             = gamma(z) + m * delta(z)
   Where gamma(z) = a(z mod q) - r(z div q)
   and   delta(z) = (z div q) - (az div m)

   If r < q, then for all z in 1, 2, ..., m - 1:
   (1) delta(z) is either 0 or 1
   (2) both a(z mod q) and r(z div q) are in 0, 1, ..., m - 1
   (3) absolute value of gamma(z) <= m - 1
   (4) delta(z) = 1 iff gamma(z) < 0

   Hence each value of z can be computed exactly without overflow as long
   as m can be represented as an integer.
 */
/* a good random number generator, correct on any machine with 32 bit
   integers, this algorithm is from:

Stephen K. Park and Keith W. Miller, "Random Number Generators:
        Good ones are hard to find", Communications of the ACM, October 1988,
        vol 31, number 10, pp. 1192-1201.

   If this algorithm is implemented correctly, then if z[1] = 1, then
   z[10001] will equal 1043618065

   Has a full period of 2^31 - 1.
   Returns integers in the range 1 to 2^31-1.
 */
/* 32 bit seed */
static int32u rnd_seed;
int32u
get_rnd_seed()
{
  return rnd_seed;
}
void set_rnd_seed(seedval) int32u seedval;
{
  /* set seed to value between 1 and m-1 */
  rnd_seed = (seedval % (2147483647L - 1)) + 1;
}
/* returns a pseudo-random number from set 1, 2, ..., RNG_M - 1 */
int32
rnd()
{
  register long low, high, test;
  high = rnd_seed / 127773L;
  low = rnd_seed % 127773L;
  test = 16807L * low - 2836L * high;
  if (test > 0)
    rnd_seed = test;
  else
    rnd_seed = test + 2147483647L;
  return rnd_seed;
}
