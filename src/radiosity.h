#pragma once

/* Implements various functions to model light bounces between various objects
 * in the scene. Here, we use lightmaps, which represet a scene object like a
 * plane as a set of small reflective patches.
 *
 * Radiosity only needs to be computed once to initalize all
 * lightmaps, and can then be reused, even if the camera is in a different 
 * position.
 */

#include <math.h>
#include "scene.h"

/* Initializes the lightmap of a scene plane based on direct illumination
 * from the light source, occluded by scene objects light spheres
 */
void compute_shadows(Plane& plane) {
  Spectrum spectrum = plane.albedo;

  // Compute shadows on the floor plane
  for (uint8_t y = 0; y < MAP_SIZE; y++) {
    for (uint8_t x = 0; x < MAP_SIZE; x++) {
      Vec3 shadowOrigin = get_sample_pos(plane.point, plane.normal, x, y);

      Ray shadow_ray(shadowOrigin, light - shadowOrigin);

      // Compute the lambertian attenuation of the lighting at this sample point
      Fixed24 attenuation = div(dot(plane.normal, shadow_ray.dir),
        shadow_ray.dir.norm() * shadow_ray.dir.norm_squared());

      attenuation = clamp0(attenuation);

      // Set the texture color to the albedo color
      plane.light_map.emissive[x][y] = spectrum * attenuation;

      // Shade this location if a sphere obstructs it
      for (Sphere& obj : spheres) {
        obj.register_camera(shadow_ray.origin);
        if (obj.ray_intersect(shadow_ray).n > 0) {
          plane.light_map.emissive[x][y] = Spectrum(BLACK);
          break;
        }
      }
    }
  }
}

/* Given a patch location, normal, and emissive plane, computes the contribution
 * of each plane patch to the illumination of that point.
 *
 * Shadows cast by scene objects are ignored here for efficiency, though this
 * does make the results somewhat inaccurate
 */
Spectrum compute_incident_radiosity(Plane& plane, Vec3& in_pos, Vec3& normal) {
  Spectrum incident;


  for (uint8_t y = 0; y < MAP_SIZE; y++) {
    for (uint8_t x = 0; x < MAP_SIZE; x++) {
      Vec3 out_pos = get_sample_pos(plane.point, plane.normal, x, y);
      Vec3 dir = out_pos - in_pos;
      
      // Compute the lambertian attenuation of the lighting at this sample point
      Fixed24 attenuation = dot(normal, dir) * -dot(plane.normal, dir);

      attenuation = clamp0(div(attenuation, sqr(dir.norm_squared())));

      // Contributed reflectance is the amount emitted by the plane scaled
      // by the attenuation based on distance and angle
      Spectrum s = plane.light_map.emissive[x][y];

      s.r = s.r * attenuation;
      s.g = s.g * attenuation;
      s.b = s.b * attenuation;

      incident += s;
    }
  }

  // After adding up all contributions from the other plane, we scale
  // with respect to the surface area of each emitting patch
  incident.r = incident.r * Fixed24(1.0f / (MAP_SIZE * MAP_SIZE));
  incident.g = incident.g * Fixed24(1.0f / (MAP_SIZE * MAP_SIZE));
  incident.b = incident.b * Fixed24(1.0f / (MAP_SIZE * MAP_SIZE));

  return incident;
}

/* Given a plane, computes the incident illumination for all patches in the 
 * lightmap. (ie, one light bounce from the scene)
 */
void compute_plane_radiosity(Plane& plane) {
  Spectrum albedo = plane.albedo;

  for (uint8_t y0 = 0; y0 < MAP_SIZE; y0++) {
    for (uint8_t x0 = 0; x0 < MAP_SIZE; x0++) {

      Vec3 in_pos = get_sample_pos(plane.point, plane.normal, x0, y0);

      Spectrum incident;

      for (Plane& out_plane : sceneObjs) {
        // Ignore self-illumination
        if (&plane == &out_plane) continue;
        incident += compute_incident_radiosity(out_plane, in_pos, plane.normal);
      }

      // Divide by pi because calculus
      incident.r = incident.r * albedo.r * Fixed24(1.0f / (float)M_PI);
      incident.g = incident.g * albedo.g * Fixed24(1.0f / (float)M_PI);
      incident.b = incident.b * albedo.b * Fixed24(1.0f / (float)M_PI);

      plane.light_map.emissive2[x0][y0] = incident;
    }
  }

}

/* Given a sphere, computes the incident illumination for all patches in the 
 * lightmap. (ie, one light bounce from the scene)
 */
void compute_sphere_radiosity(Sphere& sphere) {
  for (uint8_t y0 = 0; y0 < MAP_SIZE; y0++) {
    for (uint8_t x0 = 0; x0 < MAP_SIZE; x0++) {

      Vec3 normal = get_sample_pos(x0, y0);
      Vec3 point = (normal * sphere.radius) + sphere.point;

      Spectrum incident;

      for (Plane& out_plane : sceneObjs) {
        incident += compute_incident_radiosity(out_plane, point, normal);
      }

      // This is not physically accurate, but it does help to emphasize the
      // effects of color bleeding, so global illumination is stronger on 
      // spheres
      incident.r = incident.r * Fixed24((float)M_PI);
      incident.g = incident.g * Fixed24((float)M_PI);
      incident.b = incident.b * Fixed24((float)M_PI);

      sphere.light_map.bitmap[x0][y0] = incident;
    }
  }

}

/* Computes direct illumination lightmaps for all objects in the scene
 * This must be performed before computing radiosity, as this is the
 * light which will be bounced around
 */
void compute_illumination() {
  os_PutStrFull("Computing Shadows");
  os_NewLine();

  // Compute shading for all planes in the scene
  for (Plane& plane : sceneObjs) {
    compute_shadows(plane);
    plane.light_map.clear();
  }

  for (Sphere& sphere : spheres) {
    sphere.light_map.clear();
  }
}

/* Computes radiosity for all lightmaps in the scene. This involves computing
 * a few rounds of light bounces to approximate global illumination. 
 *
 * Generally results converge decently after 2 bounces for the provided scene
 */
void compute_radiosity() {
  os_PutStrFull("Computing Plane Radiosity");
  os_NewLine();
  
  // Compute radiosity light bounces
  for (uint8_t i = 0; i < 2; i++) {
    // Print the round number
    char str[2];
    str[0] = digits[i + 1];
    str[1] = '\0';
    os_PutStrFull(str);

    // As a progress indicator, each dot represents one plane's lightmap
    // being updated
    for (Plane& plane : sceneObjs) {
      os_PutStrFull(".");
      compute_plane_radiosity(plane);
    }

    // After all lightmaps have been updated, replace incident illumination
    // with outgoing illumination
    for (Plane& plane : sceneObjs) {
      plane.light_map.copy();
    }
  }
  os_NewLine();

  // Because spheres contribute less to global illumination compared to the
  // colored walls, their illumination is only included after radiosity for
  // planes has been fully computed
  os_PutStrFull("Computing Sphere Radiosity");

  for (Plane& plane : sceneObjs) {
    plane.light_map.from_bitmap();
  }

  for (Sphere& sphere : spheres) {
    os_PutStrFull(".");
    if (!sphere.reflective) {
      compute_sphere_radiosity(sphere);
    }
  }
  os_NewLine();
}