#pragma once

/* A representation of a 3D ray based on Vec3 and useful math operations.
 */

#include "vector.h"

struct Ray {
  Vec3 origin;
  Vec3 dir;

  Ray() {
    origin = Vec3(0, 0, 0);
    dir = Vec3(0, 0, 0);
  }

  Ray(Vec3 _origin, Vec3 _dir) {
    origin = _origin;
    dir = _dir;
  }

  /* Evaluates the ray position at parameter t
   */
  Vec3 at(Fixed24 &t) {
    Vec3 out;

    out.x = origin.x + (dir.x * t);
    out.y = origin.y + (dir.y * t);
    out.z = origin.z + (dir.z * t);
    
    return out;
  }
};