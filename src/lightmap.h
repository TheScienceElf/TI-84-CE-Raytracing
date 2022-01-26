#pragma once

/* A data structure used to store radiance emitted by a scene surface at a 
 * few select sample points. These maps are primarily used for radiosity
 * calculations
 */

#include <stdlib.h>
#include "color.h"
#include "texture.h"
#include "spectrum.h"

// Lightmap resolution must be a power of 2. In this case, lightmaps are 8x8
#define MAP_BITS 3
#define MAP_MASK ((1 << MAP_BITS) - 1)
#define MAP_HALF (1 << (MAP_BITS - 1))
#define MAP_SIZE (1 <<  MAP_BITS)

// Space between sample steps on a plane
const Fixed24 halfstep_size(1.0f / MAP_SIZE);
const Fixed24 step_size(2.0f / MAP_SIZE);

// Lookup tables for sphere sample positions
int24_t sphere_x[8] {3784, 1567, -1567, -3784, -3784, -1567,  1567,  3784};
int24_t sphere_y[8] {1567, 3784,  3784,  1567, -1567, -3784, -3784, -1567};

int24_t sphere_elev[8]{-4017, -3405, -2275, -799,  799, 2275, 3405, 4017};
int24_t  sphere_rad[8]{  799,  2275,  3405, 4017, 4017, 3405, 2275,  799};

struct LightMap {
  // Map of total emission for rendering
  Spectrum bitmap[MAP_SIZE][MAP_SIZE];

  // Bitmaps that hold temporary emission for radiosity calculation
  Spectrum emissive[MAP_SIZE][MAP_SIZE];  // Outgoing radiance
  Spectrum emissive2[MAP_SIZE][MAP_SIZE]; // Incoming radiance

  /* Convert from spectral color to bitmap color
   */
  void toTexture(Texture &texture) {
    for (uint8_t y = 0; y < MAP_SIZE; y++) {
      for (uint8_t x = 0; x < MAP_SIZE; x++) {
        Color24 error(0, 0, 0);
        texture.bitmap[x][y] = bitmap[x][y].toColor24().toColor16(error);
      }
    }
  }

  /* Sets all entries in the bitmap lightmap to zero
   */
  void clear() {
    for (uint8_t y = 0; y < MAP_SIZE; y++) {
      for (uint8_t x = 0; x < MAP_SIZE; x++) {
        bitmap[x][y] = Spectrum();
      }
    }
  }

  /* Adds collected emission from emissive2 to the bitmap, and replaces
   * emissive with the reflected light
   */
  void copy() {
    for (uint8_t y = 0; y < MAP_SIZE; y++) {
      for (uint8_t x = 0; x < MAP_SIZE; x++) {
        // Add this round's illumination to the total illumination
        bitmap[x][y] += emissive2[x][y];
        emissive[x][y] = emissive2[x][y];
      }
    }
  }

  /* Copy the bitmap illumination into the emissive to compute sphere radiosity
   */
  void from_bitmap() {
    for (uint8_t y = 0; y < MAP_SIZE; y++) {
      for (uint8_t x = 0; x < MAP_SIZE; x++) {
        emissive[x][y] = bitmap[x][y];
      }
    }
  }


  /* Samples the texture on a plane with origin at (0, 0, 0)
   */
  Spectrum sample(Vec3& hit_pos, Vec3& normal) {
    // Compute the texture color at this floor position
    int24_t tile_x = hit_pos.x.floor(LERP_BITS + MAP_BITS - 1) - LERP_HALF;
    int24_t tile_y = hit_pos.y.floor(LERP_BITS + MAP_BITS - 1) - LERP_HALF;
    int24_t tile_z = hit_pos.z.floor(LERP_BITS + MAP_BITS - 1) - LERP_HALF;

    int24_t tex_x = 0;
    int24_t tex_y = 0;
    int8_t sub_x = 0;
    int8_t sub_y = 0;

    if (normal.x.n) {
      tex_x = (tile_z >> LERP_BITS);
      tex_y = (tile_y >> LERP_BITS);

      sub_x = tile_z & LERP_MASK;
      sub_y = tile_y & LERP_MASK;
    }

    if (normal.y.n) {
      tex_x = (tile_x >> LERP_BITS);
      tex_y = (tile_z >> LERP_BITS);

      sub_x = tile_x & LERP_MASK;
      sub_y = tile_z & LERP_MASK;
    }

    if (normal.z.n) {
      tex_x = (tile_x >> LERP_BITS);
      tex_y = (tile_y >> LERP_BITS);

      sub_x = tile_x & LERP_MASK;
      sub_y = tile_y & LERP_MASK;
    }

    int16_t tex_x0 = tex_x;
    int16_t tex_x1 = (tex_x + 1);
    int16_t tex_y0 = tex_y;
    int16_t tex_y1 = (tex_y + 1);

    tex_x0 = tex_x0 < 0 ? 0 : tex_x0;
    tex_x1 = tex_x1 > MAP_MASK ? MAP_MASK : tex_x1;
    tex_y0 = tex_y0 < 0 ? 0 : tex_y0;
    tex_y1 = tex_y1 > MAP_MASK ? MAP_MASK : tex_y1;

    Spectrum tex00 = bitmap[tex_x0][tex_y0];
    Spectrum tex10 = bitmap[tex_x1][tex_y0];
    Spectrum tex01 = bitmap[tex_x0][tex_y1];
    Spectrum tex11 = bitmap[tex_x1][tex_y1];

    return bilinear(tex00, tex10, tex01, tex11, sub_x, sub_y);
  }

  /* Samples the texture on a sphere with origin (0, 0, 0)
   */
  Spectrum sample(Vec3& normal) {
    // Compute the spherical coordinates of this normal
    Fixed24 theta = atan2(normal.x, normal.z);
    Fixed24   phi = (Fixed24(1) + asin(normal.y)); \

    // Compute the texture index of this point
    int24_t tile_x = theta.floor(LERP_BITS + MAP_BITS - 1) - LERP_HALF;
    int24_t tile_y = phi.floor(LERP_BITS + MAP_BITS - 1) - LERP_HALF;

    // Compute the texel coordinates
    int8_t tex_x = tile_x >> LERP_BITS;
    int8_t tex_y = tile_y >> LERP_BITS;

    // Get the subpixel index
    int8_t sub_x = tile_x & LERP_MASK;
    int8_t sub_y = tile_y & LERP_MASK;

    int16_t tex_x0 = tex_x;
    int16_t tex_x1 = (tex_x + 1);
    int16_t tex_y0 = tex_y;
    int16_t tex_y1 = (tex_y + 1);

    tex_x0 = tex_x0 < 0 ? 0 : tex_x0;
    tex_x1 = tex_x1 > MAP_MASK ? MAP_MASK : tex_x1;
    tex_y0 = tex_y0 < 0 ? 0 : tex_y0;
    tex_y1 = tex_y1 > MAP_MASK ? MAP_MASK : tex_y1;

    Spectrum tex00 = bitmap[tex_x0][tex_y0];
    Spectrum tex10 = bitmap[tex_x1][tex_y0];
    Spectrum tex01 = bitmap[tex_x0][tex_y1];
    Spectrum tex11 = bitmap[tex_x1][tex_y1];

    return bilinear(tex00, tex10, tex01, tex11, sub_x, sub_y);
  }
};

/* Given a plane origin and normal, computes the world space coordinates
 * for the given texture pixel coordinates.
 *
 * This implementation only works for axis-aligned planes
 */
Vec3 get_sample_pos(Vec3 &point, Vec3 &normal, uint8_t x, uint8_t y) {
  Vec3 step_x;
  Vec3 step_y;
  
  // Determine which direction each sample point moves as we increment the x 
  // and y coordinates in the texture
  if (normal.x.n != 0) {
    step_x = Vec3(Fixed24(0), Fixed24(0), step_size);
    step_y = Vec3(Fixed24(0), step_size, Fixed24(0));
  }

  if (normal.y.n != 0) {
    step_x = Vec3(step_size, Fixed24(0), Fixed24(0));
    step_y = Vec3(Fixed24(0), Fixed24(0), step_size);
  }

  if (normal.z.n != 0) {
    step_x = Vec3(step_size, Fixed24(0), Fixed24(0));
    step_y = Vec3(Fixed24(0), step_size, Fixed24(0));
  }


  // Move the sample origin half a sample step from the real origin
  Vec3 out = point + (step_x + step_y) * Fixed24(.5f);

  for (int8_t i = 0; i < x; i++) {
    out += step_x;
  }

  for (int8_t i = 0; i < y; i++) {
    out += step_y;
  }

  return out;


}

/* Given a unit sphere centered about the origin, computes the world space 
 * coordinates for the given texture pixel coordinates.
 */
Vec3 get_sample_pos(uint8_t x, uint8_t y) {
  Vec3 out;

  // Get the sphere elevation and radius at this elevation
  Fixed24 scl;
  out.y.n = sphere_elev[y];
  scl.n = sphere_rad[y];

  // Compute the point in the horizontal plane
  out.x.n = sphere_x[x];
  out.z.n = sphere_y[x];

  // Scale this point to the unit sphere
  out.x = out.x * scl;
  out.z = out.z * scl;

  return out;
}