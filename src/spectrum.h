#pragma once

/* A high precision (24 bit per channel) fixed point representation of
 * light and color. This type is generally interpreted as being in linear
 * color space.
 * 
 * Conversion functions to and from sRGB colors are provided
 */

#include "color.h"
#include "fixedpoint.h"

uint16_t gamma_LUT[256]{ 
  0,
  0,
  0,
  0,
  0,
  1,
  1,
  2,
  2,
  3,
  4,
  5,
  6,
  7,
  9,
  10,
  12,
  13,
  15,
  17,
  19,
  21,
  23,
  26,
  28,
  31,
  33,
  36,
  39,
  42,
  45,
  48,
  52,
  55,
  59,
  63,
  67,
  71,
  75,
  79,
  83,
  88,
  93,
  97,
  102,
  107,
  112,
  118,
  123,
  129,
  134,
  140,
  146,
  152,
  159,
  165,
  171,
  178,
  185,
  192,
  199,
  206,
  213,
  221,
  229,
  236,
  244,
  253,
  261,
  269,
  278,
  286,
  295,
  304,
  314,
  323,
  332,
  342,
  352,
  362,
  372,
  382,
  393,
  403,
  414,
  425,
  436,
  448,
  459,
  471,
  483,
  495,
  507,
  520,
  532,
  545,
  558,
  571,
  585,
  598,
  612,
  626,
  640,
  655,
  669,
  684,
  699,
  714,
  730,
  745,
  761,
  777,
  794,
  810,
  827,
  844,
  861,
  879,
  896,
  914,
  932,
  951,
  970,
  989,
  1008,
  1027,
  1047,
  1067,
  1087,
  1108,
  1129,
  1150,
  1171,
  1193,
  1215,
  1238,
  1260,
  1283,
  1306,
  1330,
  1354,
  1378,
  1403,
  1428,
  1453,
  1479,
  1505,
  1531,
  1558,
  1585,
  1612,
  1640,
  1668,
  1697,
  1726,
  1756,
  1785,
  1816,
  1847,
  1878,
  1910,
  1942,
  1974,
  2007,
  2041,
  2075,
  2110,
  2145,
  2181,
  2217,
  2254,
  2291,
  2329,
  2368,
  2407,
  2447,
  2488,
  2529,
  2571,
  2613,
  2657,
  2701,
  2745,
  2791,
  2837,
  2884,
  2932,
  2981,
  3031,
  3081,
  3133,
  3185,
  3239,
  3293,
  3349,
  3406,
  3463,
  3522,
  3582,
  3644,
  3706,
  3770,
  3836,
  3903,
  3971,
  4041,
  4112,
  4185,
  4260,
  4336,
  4415,
  4495,
  4578,
  4662,
  4749,
  4838,
  4930,
  5024,
  5121,
  5221,
  5324,
  5429,
  5539,
  5652,
  5768,
  5889,
  6014,
  6144,
  6278,
  6418,
  6563,
  6714,
  6872,
  7038,
  7210,
  7392,
  7582,
  7783,
  7996,
  8221,
  8460,
  8715,
  8989,
  9283,
  9602,
  9950,
  10331,
  10754,
  11227,
  11765,
  12388,
  13126,
  14031,
  15200,
  16852
};

int24_t degamma_LUT[32]{ 
  0,
  2,
  12,
  28,
  52,
  83,
  123,
  171,
  229,
  295,
  372,
  459,
  558,
  669,
  794,
  932,
  1087,
  1260,
  1453,
  1668,
  1910,
  2181,
  2488,
  2837,
  3239,
  3706,
  4260,
  4930,
  5768,
  6872,
  8460,
  11227 };

/* Invert the gamma_LUT to compute an 0-255 sRGB value for this illumination
 * This is done via binary search
 */
uint8_t gamma(Fixed24 x) {
  uint24_t low = 0;
  uint24_t high = 255;

  while (low < high) {
    int24_t mid = (low + high) >> 1;

    if (gamma_LUT[mid] == x.n) return mid;

    if (gamma_LUT[mid] > x.n) {
      high = mid - 1;
    }
    else {
      low = mid + 1;
    }
  }

  return low;
}

/* A struct to represent light of various intensities. Each component is a
 * separate Fixed24 to maintain high precision.
 */
struct Spectrum {
  Fixed24 r;
  Fixed24 g;
  Fixed24 b;

  Spectrum() {
    r = Fixed24(0);
    g = Fixed24(0);
    b = Fixed24(0);
  }

  /* Creates a spectral representation of the provided color
   * These are easier to do various forms of arithmetic on
   */
  Spectrum(Color c) {
    // Decompose the color into components
    int8_t _r = (c >> 11) & COLOR_MASK;
    int8_t _g = (c >>  6) & COLOR_MASK;
    int8_t _b =  c        & COLOR_MASK;

    r.n = degamma_LUT[_r];
    g.n = degamma_LUT[_g];
    b.n = degamma_LUT[_b];
  }

  Spectrum(Fixed24 f) {
    r = f;
    g = f;
    b = f;
  }

  /* Converts this spectrum back into a 24-bit color that can be dithered to 
   * be displayed accurately onscreen
   * the screen
   */
  Color24 toColor24() {
    uint8_t _r = gamma(r);
    uint8_t _g = gamma(g);
    uint8_t _b = gamma(b);

    return Color24(_r, _g, _b);
  }

  Spectrum operator+(Spectrum s) const {
    Spectrum out;

    out.r = r + s.r;
    out.g = g + s.g;
    out.b = b + s.b;

    return out;
  }

  void operator+=(Spectrum s) {
    r += s.r;
    g += s.g;
    b += s.b;
  }

  Spectrum operator*(Fixed24 s) const {
    Spectrum out;
    
    out.r = r * s;
    out.g = g * s;
    out.b = b * s;

    return out;
  }
};

/* Linearly interpolates between two colors (t is 0 to 255)
 */
Spectrum lerp(Spectrum& c1, Spectrum& c2, uint8_t t) {
  // Decompose the two colors
  int24_t r1 = c1.r.n;
  int24_t g1 = c1.g.n;
  int24_t b1 = c1.b.n;

  int24_t r2 = c2.r.n;
  int24_t g2 = c2.g.n;
  int24_t b2 = c2.b.n;

  Spectrum out;
  out.r.n = r1 + ((r2 - r1) * t / LERP_MASK);
  out.g.n = g1 + ((g2 - g1) * t / LERP_MASK);
  out.b.n = b1 + ((b2 - b1) * t / LERP_MASK);

  return out;
}

/* Performs bilinear interpolation between four colors
 *
 * sub_x and sub_y are interpreted as being in [0, 255]
 */
Spectrum bilinear(Spectrum& tex00, Spectrum& tex10, Spectrum& tex01, Spectrum& tex11,
  uint8_t sub_x, uint8_t sub_y) {

  Spectrum c1 = lerp(tex00, tex10, sub_x);
  Spectrum c2 = lerp(tex01, tex11, sub_x);

  return lerp(c1, c2, sub_y);
}