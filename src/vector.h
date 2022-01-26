#pragma once

/* Implemented 3D vectors based on fixed point math, and some common math
 * operations for these vectors
 */

#include "fixedpoint.h"

struct Vec3 {
  Fixed24 x;
  Fixed24 y;
  Fixed24 z;

  // Constructors for various types
  Vec3() {
    x = Fixed24(0);
    y = Fixed24(0);
    z = Fixed24(0);
  }
  Vec3(int24_t _x, int24_t _y, int24_t _z) {
    x = Fixed24(_x);
    y = Fixed24(_y);
    z = Fixed24(_z);
  }
  Vec3(float _x, float _y, float _z) {
    x = Fixed24(_x);
    y = Fixed24(_y);
    z = Fixed24(_z);
  }
  Vec3(Fixed24 _x, Fixed24 _y, Fixed24 _z) {
    x = _x;
    y = _y;
    z = _z;
  }

  Vec3 operator+(Vec3 v) const {
    return Vec3(x + v.x, y + v.y, z + v.z);
  }
  Vec3 operator-(Vec3 v) const {
    return Vec3(x - v.x, y - v.y, z - v.z);
  }
  Vec3 operator*(Vec3 v) const {
    return Vec3(x * v.x, y * v.y, z * v.z);
  }


  void operator+=(Vec3 &v) {
    x += v.x;
    y += v.y;
    z += v.z;
  }


  Vec3 operator+(Fixed24 s) const {
    return Vec3(x + s, y + s, z + s);
  }
  Vec3 operator-(Fixed24 s) const {
    return Vec3(x - s, y - s, z - s);
  }
  Vec3 operator*(Fixed24 s) const {
    return Vec3(x * s, y * s, z * s);
  }

  /* Computes the squared L2 norm of this vector
   */
  Fixed24 norm_squared() {
    return sqr(x) + sqr(y) + sqr(z);
  }

  /* Computes the euclidean length of this vector
   */
  Fixed24 norm() {
    Fixed24 norm_sqr = norm_squared();
    return sqrt(norm_sqr);
  }
};

// 3D dot product
Fixed24 dot(Vec3 &l, Vec3 &r) {
  return (l.x * r.x) + (l.y * r.y) + (l.z * r.z);
}

// 3D cross product
inline Vec3 cross(Vec3 l, Vec3 r) {
  return Vec3(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
}

/* Prints the three components of this vector
 */
void print_vec(Vec3 vec) {
  print_fixed(vec.x);
  print_fixed(vec.y);
  print_fixed(vec.z);
  os_NewLine();
}