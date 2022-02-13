#pragma once

/* Operators for 24-bit (12.12) fixed point arithmetic on the ez80.
 * 
 * Fixed point is preferred for this application because arithmetic is generally
 * faster, and the limited dynamic range is not necessary for the scene being
 * rendered.
 */

#define POINT 12

#include <tice.h>
#include "asmmath.h"
#include <math.h>

const char* digits = "0123456789ABCDEF";

struct Fixed24 {
  int24_t n;

  Fixed24() {
    n = 0;
  }

  // Convert this integer to a fixed point representation
  Fixed24(int24_t _n) {
    n = _n << POINT;
  }

  // Approximates a given float as a fixed point number
  Fixed24(float _n) {
    n = (int24_t)(_n * (1 << POINT));
  }

  int24_t floor() {
    return n >> POINT;
  }

  /* Rounds down the provided Fixed24 while preserving the last requested number
   * of digits. This is equivalent to
   *
   * floor(n * (2 ^ digits))
   * 
   * This is generally useful for sampling discrete values between 0 and 1
   * so long as the discrete space is 2 ^ digits in length
   */
  int24_t floor(uint8_t digits) {
    return n >> (POINT - digits);
  }

  Fixed24 operator+(Fixed24 v) const {
    Fixed24 out;

    out.n = n + v.n;

    return out;
  }

  Fixed24 operator-(Fixed24 v) const {
    Fixed24 out;

    out.n = n - v.n;

    return out;
  }

  Fixed24 operator*(Fixed24 v) const {
    Fixed24 out;

    // Invokes a specialized asm routine implemented in asmtest.asm
    out.n = fp_mul(n, v.n);

    return out;
  }

  Fixed24 operator-() const {
    Fixed24 out;
    
    out.n = -n;

    return out;
  }

  void operator+=(Fixed24 v) {
    n += v.n;
  }

  void operator-=(Fixed24 v) {
    n -= v.n;
  }

  bool operator<(Fixed24 x) const {
    return n < x.n;
  }

  bool operator<=(Fixed24 x) const {
    return n <= x.n;
  }

  bool operator>(Fixed24 x) const {
    return n > x.n;
  }

  bool operator>=(Fixed24 x) const {
    return n >= x.n;
  }

  bool operator!=(Fixed24 x) const {
    return n != x.n;
  }
};

/* Prints the hex digits of a fixed point number for debug usage
 */
void print_fixed(Fixed24 &x) {
  char str[8];
  str[7] = '\0';

  uint24_t n = x.n;
  for (int8_t i = 6; i >= 0; i--) {
    // Skip the fourth character, which holds the decimal point
    if (i == 3) {
      str[i] = '.';
      continue;
    }

    // Extract the least significant digit and shift down the next
    str[i] = digits[n & 0xF];
    n = n >> 4;
  }

  os_PutStrFull(str);
  os_NewLine();
}

/* Computes the square root of a fixed point number and returns
 * the result as a fixed point number
 */
Fixed24 sqrt(Fixed24 &x) {

  // Convert to a float, take sqrt, then convert back
  float f = x.n / (float)(1 << POINT);

  return Fixed24(sqrtf(f));
}

/* Computes the square of a FP24 number.
 *
 * This uses a specialized multiplication implementation and is preferable
 * over multiplying the number with itself
 */
Fixed24 sqr(Fixed24 x) {

  Fixed24 out;
  out.n = fp_sqr(x.n);

  return out;
}

/* Computes division of larger numbers by multiplying a by the reciprocal of b
 * This is slow and imprecise for certain values of b, so it should be replaced 
 * by an ASM routine
 */
Fixed24 div(Fixed24 a, Fixed24 b) {
  Fixed24 reciprocal;

  reciprocal.n = (((int32_t)(1 << POINT)) << POINT) / (int32_t)b.n;

  return a * reciprocal;
}

/* Clamps the Fixed24 to be within the range 0 and 1 inclusively
 */
Fixed24 clamp01(Fixed24 x) {
  if (x < Fixed24(0)) return Fixed24(0);
  if (x > Fixed24(1)) return Fixed24(1);
  return x;
}

/* Clamps the Fixed24 to be no less than zero
 */
Fixed24 clamp0(Fixed24 x) {
  if (x < Fixed24(0)) return Fixed24(0);
  return x;
}

// A table of values of arcsin(x) / (pi / 2) used as a LUT in the asin function
int24_t asin_table[65]{ -4096,
-3442,
-3169,
-2957,
-2778,
-2618,
-2473,
-2338,
-2211,
-2091,
-1976,
-1866,
-1760,
-1657,
-1557,
-1460,
-1365,
-1272,
-1180,
-1090,
-1002,
-915,
-828,
-743,
-658,
-575,
-491,
-409,
-326,
-244,
-163,
-81,
0,
81,
163,
244,
326,
409,
491,
575,
658,
743,
828,
915,
1002,
1090,
1180,
1272,
1365,
1460,
1557,
1657,
1760,
1866,
1976,
2091,
2211,
2338,
2473,
2618,
2778,
2957,
3169,
3442,
4096 };

/* Computes the arcsine of x, and returns a value in the range -1 to 1
 * (the actual arcsine divided by pi/2 since this value is more useful to us)
 */
Fixed24 asin(Fixed24 x) {
  Fixed24 out;

  out.n = asin_table[(x + Fixed24(1)).floor(5)];

  return out;
}

/* Computes arctangent of the angle created between the x axis
 * and the line from the origin to (x, y)
 *
 * The result is also divided by pi/2 for simplicity in texture
 * sampling
 */
Fixed24 atan2(Fixed24 &x, Fixed24 &y) {
  Fixed24 rad2 = sqr(x) + sqr(y);
  Fixed24 rad = sqrt(rad2);

  Fixed24 norm_x = div(x, rad);

  // Clamp this value into the unit range
  norm_x = norm_x > Fixed24(-1) ? norm_x : Fixed24(-1);
  norm_x = norm_x < Fixed24( 1) ? norm_x : Fixed24( 1);
  //
  Fixed24 a = Fixed24(1) + asin(-norm_x);
  a.n >>= 1;

  return y > Fixed24(0) ? a : Fixed24(2) - a;\
}
