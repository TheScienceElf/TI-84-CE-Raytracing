#pragma once

/* Implements a data structure and functions to represent planes
 * and compute intersections with them from view rays
 *
 * Planes can register the ray origin (camera position) before
 * computing intersections to avoid redundant computations when
 * the camera position is constant
 */

#include "vector.h"
#include "ray.h"
#include "color.h"
#include "texture.h"
#include "lightmap.h"

struct Plane {
  Vec3 point;
  Vec3 normal;
  Spectrum albedo;
  LightMap light_map;
  Texture* texture;

  // Precomputed value to speed up the math
  Fixed24 numerator;

  Plane(Vec3 _point, Vec3 _normal, Color _color, Texture *_texture) {
    point   = _point;
    normal  = _normal;
    albedo = Spectrum(_color);
    texture = _texture;
  }

  /* Precompute some of the ray intersection math which is not dependent on
   * ray direction. This only needs updated whenever the camera position
   * is changed
   */
  void register_camera(Vec3 &origin) {
    Vec3 offset = point - origin;

    numerator = dot(offset, normal);
  }

  /* Compute the t parameter where this ray intersects with the plane.
   * t < 0 implies no intersection
   * 
   * Requires register_camera to have been called earlier with the correct
   * camera position.
   */
  Fixed24 ray_intersect_fast(Ray &r) {
    Fixed24 t = div(numerator, dot(r.dir, normal));

    Vec3 hit_pos = r.at(t);
    hit_pos = hit_pos - point;

    // Restrict intersections to the 2x2x2 scene region
    if (hit_pos.x > Fixed24(2.01f)) return Fixed24(-1);
    if (hit_pos.y > Fixed24(2.01f)) return Fixed24(-1);
    if (hit_pos.z > Fixed24(2.01f)) return Fixed24(-1);
    if (hit_pos.x < Fixed24(-.01f)) return Fixed24(-1);
    if (hit_pos.y < Fixed24(-.01f)) return Fixed24(-1);
    if (hit_pos.z < Fixed24(-.01f)) return Fixed24(-1);

    return t;
  }

  /* Compute the t parameter where this ray intersects with the plane.
   * t < 0 implies no intersection
   *
   * This implementation does not rely on precomputed values, thus it
   * can be used for any ray r, although it will be somewhat slower
   */
  Fixed24 ray_intersect(Ray& r) {
    Vec3 offset = point - r.origin;

    Fixed24 t = div(dot(offset, normal), dot(r.dir, normal));

    Vec3 hit_pos = r.at(t);
    hit_pos = hit_pos - point;

    // Restrict intersections to the 2x2x2 scene region
    if (hit_pos.x > Fixed24(2.01f)) return Fixed24(-1);
    if (hit_pos.y > Fixed24(2.01f)) return Fixed24(-1);
    if (hit_pos.z > Fixed24(2.01f)) return Fixed24(-1);
    if (hit_pos.x < Fixed24(-.01f)) return Fixed24(-1);
    if (hit_pos.y < Fixed24(-.01f)) return Fixed24(-1);
    if (hit_pos.z < Fixed24(-.01f)) return Fixed24(-1);

    return t;
  }
};