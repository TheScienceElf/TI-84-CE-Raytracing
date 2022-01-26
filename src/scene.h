#pragma once

/* Initialization of scene parameters to create a Cornell Box lookalike
 *
 * Right now, scenes are composed of plane segments (in a 2x2x2 cube) and 
 * spheres. While sceneObj permits the possibility of storing all objects
 * in one array, we can make some assumptions about the geometry of the
 * scene (no interior planes, only spheres can cast shadows) due to how it
 * has been set up that allow for useful simplifications in other parts of
 * the code.
 * 
 * Basically, this could (and should) be generalized, but it was easier
 * to keep this more scene specific in the name of speed.
 */

#include "vector.h"
#include "plane.h"
#include "sphere.h"

Plane sceneObjs[5] = {
  Plane(Vec3(-1, -1,  2), Vec3( 0,  1,  0), fromRGB(29, 24, 18), &wood_tex),
  Plane(Vec3(-1,  1,  2), Vec3( 0, -1,  0), fromRGB(24, 24, 24),   nullptr),
  Plane(Vec3(-1, -1,  2), Vec3( 1,  0,  0), fromRGB(24,  9,  9),   nullptr),
  Plane(Vec3( 1, -1,  2), Vec3(-1,  0,  0), fromRGB( 9,  9, 26),   nullptr),
  Plane(Vec3(-1, -1,  4), Vec3( 0,  0, -1), fromRGB(24, 24, 24),   nullptr)
};

Sphere spheres[2] = {
  Sphere(Vec3(0, 0, 3), .4f, nullptr, true),
  Sphere(Vec3(0, 0, 3), .4f, nullptr, false)
};

// Scenes currently only support a single point light source, maybe that 
// should change?
Vec3 light(0, 1, 3);

/* Returns true if the ray intersects any sphere. False otherwise.
 * 
 * We are assuming here that spheres are the only scene objects to cast
 * shadows
 */
bool compute_shadow(Ray& ray) {
  for (Sphere& sphere : spheres) {
    if (sphere.shadow_intersect(ray)) return true;
  }

  return false;
}