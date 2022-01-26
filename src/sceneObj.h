#pragma once

/* A generic container for both types of scene objects (planes and spheres)
 *
 * This is used in main to reuse shading code between both object types
 */

#include "plane.h"
#include "sphere.h"

enum objType { PLANE, SPHERE };

struct SceneObj {
  // This should probably be done as a union type
  Plane *plane;
  Sphere *sphere;
  objType type;

  SceneObj(void) {
    plane = nullptr;
    sphere = nullptr;
    type = PLANE;
  }

  SceneObj(Plane* _plane) {
    plane = _plane;
    sphere = nullptr;
    type = PLANE;
  }

  SceneObj(Sphere* _sphere) {
    plane = nullptr;
    sphere = _sphere;
    type = SPHERE;
  }
};