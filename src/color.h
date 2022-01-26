#pragma once

/* Defines types and functions for operating with onscreen colors
 * 
 * Generally types here should be interpreted as being in sRGB space
 */

#define COLOR_BITS 5
#define COLOR_MASK ((1 << COLOR_BITS) - 1)

#define LERP_BITS 5
#define LERP_MASK ((1 << LERP_BITS) - 1)
#define LERP_HALF (1 << (LERP_BITS - 1))

// Colors in VRAM are 16-bit integers
typedef uint16_t Color;

#define fromRGB(r, g, b) (((r) << 11) | ((g) << 6) | (b))

const Color BLACK = fromRGB( 0,  0,  0);
const Color GREY  = fromRGB(16, 16, 16);
const Color WHITE = fromRGB(31, 31, 31);

const Color RED   = fromRGB(31,  0,  0);
const Color GREEN = fromRGB( 0, 31,  0);
const Color BLUE  = fromRGB( 0,  0, 31);

/* Finds the midpoint between two colors
 */
Color lerp_half(Color a, Color b) {
  // Decompose the two colors
  uint8_t r1 = (a >> 11) & 0x1F;
  uint8_t g1 = (a >>  6) & 0x1F;
  uint8_t b1 =         a & 0x1F;

  uint8_t r2 = (b >> 11) & 0x1F;
  uint8_t g2 = (b >>  6) & 0x1F;
  uint8_t b2 =         b & 0x1F;

  return fromRGB((r1 + r2) / 2, (g1 + g2) / 2, (b1 + b2) / 2);
}

/* Linearly interpolates between two colors (t is 0 to 255)
 */
Color lerp(Color c1, Color c2, uint8_t t) {
  // Decompose the two colors
  int8_t r1 = (c1 >> 11) & COLOR_MASK;
  int8_t g1 = (c1 >>  6) & COLOR_MASK;
  int8_t b1 =  c1        & COLOR_MASK;

  int8_t r2 = (c2 >> 11) & COLOR_MASK;
  int8_t g2 = (c2 >>  6) & COLOR_MASK;
  int8_t b2 =  c2        & COLOR_MASK;
  
  uint8_t r = r1 + ((r2 - r1) * t / LERP_MASK);
  uint8_t g = g1 + ((g2 - g1) * t / LERP_MASK);
  uint8_t b = b1 + ((b2 - b1) * t / LERP_MASK);

  return fromRGB(r, g, b);
}

/* Performs bilinear interpolation between four colors
 */
Color bilinear(Color tex00, Color tex10, Color tex01, Color tex11, 
               uint8_t sub_x, uint8_t sub_y) {
  return lerp(lerp(tex00, tex10, sub_x), lerp(tex01, tex11, sub_x), sub_y);
}

// A 24-bit color struct to preserve some precision while dithering the output
struct Color24 {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  Color24(uint8_t _r, uint8_t _g, uint8_t _b) {
    r = _r;
    g = _g;
    b = _b;
  }

  /* Rounds down each channel to 5 bits and constructs a 16-bit color
   *
   * Updates error parameter to include rounding error, which can be
   * used for dithering
   */
  Color toColor16(Color24& error) {
    error.r = r & 0b111;
    error.g = g & 0b111;
    error.b = b & 0b111;

    return fromRGB(r >> 3, g >> 3, b >> 3);
  }

  void operator+=(Color24& c) {
    r += c.r;
    g += c.g;
    b += c.b;

    // Clamp this addition if it ever overflows
    if (r < c.r) r = 255;
    if (g < c.g) g = 255;
    if (b < c.b) b = 255;
  }
};