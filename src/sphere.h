#pragma once

/* Implements a data structure and functions to represent spheres
 * and compute intersections with them from view rays
 *
 * Spheres can register the ray origin (camera position) before
 * computing intersections to avoid redundant computations when
 * the camera position is constant
 */

#include <math.h>
#include "fixedpoint.h"
#include "vector.h"
#include "ray.h"

struct Sphere {
  Vec3 point;
  Fixed24 radius;
  bool reflective;

  Texture* texture;
  LightMap light_map;

  // Precomputed values to speed up the math
  Vec3 offset;
  Fixed24 c;

  Sphere(Vec3 _point, float _radius, Texture* _texture, bool _reflective) {
    point  = _point;
    radius = Fixed24(_radius);
    reflective = _reflective;

    texture = _texture;
  }

  /* Precompute some of the ray intersection math which is not dependent on
   * ray direction. This only needs updated whenever the camera position
   * is changed
   */
  void register_camera(Vec3& origin) {
    offset = origin - point;
    c = offset.norm_squared() - sqr(radius);
  }

  /* Compute the t parameter where this ray intersects with the sphere.
   * t < 0 implies no intersection
   *
   * Requires register_camera to have been called earlier with the correct
   * camera position.
   */
  Fixed24 ray_intersect_fast(Ray& ray) {
    // Compute the sphere intersection
    Fixed24 a = ray.dir.norm_squared();
    Fixed24 b2 = dot(offset, ray.dir);

    // Compute the discriminant
    Fixed24 d = sqr(b2) - (a * c);

    // If we don't intersect, just return any negative number
    if (d.n < 0) return Fixed24(-1);

    // Compute the quadratic formula result for t
    return div(-b2 - sqrt(d), a);
  }

  /* Compute the t parameter where this ray intersects with the sphere.
   * t < 0 implies no intersection
   *
   * This implementation does not rely on precomputed values, thus it
   * can be used for any ray r, although it will be somewhat slower
   */
  Fixed24 ray_intersect(Ray& ray) {
    Vec3 _offset = ray.origin - point;
    // Compute the sphere intersection
    Fixed24 a = ray.dir.norm_squared();
    Fixed24 b2 = dot(_offset, ray.dir);
    Fixed24 _c = _offset.norm_squared() - sqr(radius);

    // Compute the discriminant
    Fixed24 d = sqr(b2) - (a * _c);

    // If we don't intersect, just return any negative number
    if (d < Fixed24(0)) return Fixed24(-1);

    // Compute the quadratic formula result for t
    return div(-b2 - sqrt(d), a);
  }

  /* Tests if the ray intersects the sphere at any point. This fails on cases
   * with intersections before the ray origin, but it is a lot faster to compute
   */
  bool shadow_intersect(Ray& ray) {
    Vec3 _offset = ray.origin - point;
    // Compute the sphere intersection
    Fixed24 a = ray.dir.norm_squared();
    Fixed24 b2 = dot(_offset, ray.dir);
    Fixed24 _c = _offset.norm_squared() - sqr(radius);

    // Compute the discriminant
    Fixed24 d = sqr(b2) - (a * _c);

    if (d < Fixed24(0)) return false;
    
    // Check that the intersection point has t < 1 (simplified)
    return -b2 - sqrt(d) < a;
  }
};