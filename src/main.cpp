#include <tice.h>
#include <graphx.h>
#include <math.h>
#include <stdlib.h>

#include "fixedpoint.h"
#include "vector.h"
#include "ray.h"
#include "plane.h"
#include "sphere.h"
#include "sceneObj.h"
#include "color.h"
#include "spectrum.h"
#include "texture.h"
#include "lightmap.h"
#include "scene.h"
#include "radiosity.h"

// The base pointer for VRAM
volatile Color* VRAM = (Color*)0xD40000;

Ray ray;
Vec3 hit_pos;

// Grain controls the granularity of the output render
// For full resolution, each pixel is a 1x1 square, although
// higher granularity can speed up render time
const uint8_t grain = 1;
// Supersampling allows for rendering of a scene at a resolution
// higher than the display hardware allows by rendering rectangular
// sections of pixels which fill the screen. This line will enable
// 2x2 supersampling for an output image with resolution 640 x 480
//const uint8_t supersample = 2;
const uint8_t supersample = 1;

const int24_t mid_x = LCD_WIDTH / 2;
const int24_t mid_y = LCD_HEIGHT / 2;

Vec3 cam_origin(0, 0, 0);
const Fixed24 cam_scl(1.0f / LCD_HEIGHT);

Fixed24 exposure(2.0f);

// Position to set the view ray along each axis when starting a new row or
// column of our raster scan
Fixed24 left;
Fixed24 top;

// The offset from the VRAM base of the pixel we are currently computing
uint24_t pixelOff = 0;

/* Initializes scene variables, and registers the camera with all scene objects
 */
void scene_init() {
  // A few altername scene parameters which can be substituted in for
  // different scene configurations
  //light.x = Fixed24( 0.5f);
  //light.z = Fixed24( 3.5f);
  //light.y = Fixed24(-0.9f);

  light.x = Fixed24( 0.0f);
  light.z = Fixed24( 3.0f);
  light.y = Fixed24( 0.5f);

  //cam_origin.x = Fixed24(-0.5f);
  //cam_origin.y = Fixed24(-0.5f);
  //cam_origin.z = Fixed24( 1.5f);


  spheres[0].point.x = Fixed24(-.33f);
  spheres[0].point.y = Fixed24(-.8f);
  spheres[0].point.z = Fixed24(3.052f);
  
  spheres[1].point.x = Fixed24(.43f);
  spheres[1].point.y = Fixed24(-.8f);
  spheres[1].point.z = Fixed24(2.43f);

  spheres[0].point.y = Fixed24(-.6f);
  spheres[1].point.y = Fixed24(-.6f);

  // Lighting initialization
  compute_illumination();
  compute_radiosity();

  left = Fixed24(-mid_x - grain) * cam_scl;
  top  = Fixed24( mid_y - grain) * cam_scl;

  // Register all scene objects with the camera
  for (Plane& obj : sceneObjs) {
    obj.register_camera(cam_origin);
  }

  for (Sphere& obj : spheres) {
    obj.register_camera(cam_origin);
  }
}

Spectrum compute_ray(Ray& ray, bool from_cam);

/* Given a ray and hit information, computes the shading of the provided
 * object at the hit point
 */
Spectrum compute_shading(Ray& ray, Vec3 &hit_pos, SceneObj &obj) {
  Vec3 normal;
  Vec3 shadowDir = light - hit_pos;
  Vec3 offset = light - hit_pos;
  Ray shadow_ray(hit_pos, offset);
  Vec3 sample_pos;
  Spectrum color;
  
  switch (obj.type) {
  case PLANE:
    normal = obj.plane->normal;

    // Otherwise sample the texture with respect to the plane origin
    sample_pos = hit_pos - obj.plane->point;
    color = obj.plane->light_map.sample(sample_pos, obj.plane->normal);

    if (!compute_shadow(shadow_ray)) {
      // Compute the lambertian attenuation of the lighting at this sample point
      Fixed24 attenuation = dot(normal, offset);

      attenuation = clamp01(div(attenuation, offset.norm() * offset.norm_squared()));

      Spectrum albedo = obj.plane->albedo;

      albedo.r = albedo.r * attenuation;
      albedo.g = albedo.g * attenuation;
      albedo.b = albedo.b * attenuation;

      color += albedo;
    }

    // If this object has a texture
    if (obj.plane->texture != nullptr) {
      // Remove the flat albedo
      color.r = div(color.r, obj.plane->albedo.r);
      color.g = div(color.g, obj.plane->albedo.g);
      color.b = div(color.b, obj.plane->albedo.b);

      // Sample the precise albedo from the texture, and apply that one
      Spectrum tex_color(obj.plane->texture->sample(sample_pos, obj.plane->normal));

      color.r = color.r * tex_color.r;
      color.g = color.g * tex_color.g;
      color.b = color.b * tex_color.b;
    }

    return color;

  case SPHERE:
    normal = hit_pos - obj.sphere->point;
    normal.x = div(normal.x, obj.sphere->radius);
    normal.y = div(normal.y, obj.sphere->radius);
    normal.z = div(normal.z, obj.sphere->radius);

    if (obj.sphere->reflective) {
      Vec3 reflection = ray.dir - (normal * (Fixed24(2) * dot(ray.dir, normal)));


      Ray reflect_ray(hit_pos + (reflection * Fixed24(0.01f)), reflection);

      return compute_ray(reflect_ray, false);
    }

    color = obj.sphere->light_map.sample(normal);

    // Compute the lambertian attenuation of the lighting at this sample point
    Fixed24 attenuation = dot(normal, shadowDir);

    attenuation = clamp01(div(attenuation, shadowDir.norm() * shadowDir.norm_squared()));

    color += Spectrum(attenuation);

    return color;
  }
  
}

/* Computes the scene color for a given ray
 */
Spectrum compute_ray(Ray &ray, bool from_cam) {
  Spectrum color(Fixed24(0));

  
  // Start the culling depth at 2000 units (practically infinity for FP24)
  Fixed24 min_t(2000);
  SceneObj closestObj;

  // Find the closest hit along our ray
  for (Plane& obj : sceneObjs) {
    Fixed24 t = from_cam ? obj.ray_intersect_fast(ray) : obj.ray_intersect(ray);

    // If we hit closer to the camera, keep this depth
    if (t.n > 1 && t < min_t) {
      closestObj = SceneObj(&obj);
      min_t = t;
    }
  }

  // Check if we hit any spheres
  for (Sphere& obj : spheres) {
    Fixed24 t = from_cam ? obj.ray_intersect_fast(ray) : obj.ray_intersect(ray);

    // If we hit closer to the camera, keep this depth
    if (t.n > 1 && t < min_t) {
      closestObj = SceneObj(&obj);
      min_t = t;
    }
  }

  // If we hit an object, compute the color of that object
  if (min_t < Fixed24(2000)) {
    hit_pos = ray.at(min_t);
    color = compute_shading(ray, hit_pos, closestObj);
  }

  color.r = color.r * exposure;
  color.g = color.g * exposure;
  color.b = color.b * exposure;

  return color;
}

/*  Renders the scene in segments, each of which will the screen
 */
void render_supersample() {
  ray.origin = cam_origin;

  for (int24_t sy = 0; sy < supersample * LCD_HEIGHT; sy += LCD_HEIGHT) {
    for (int24_t sx = 0; sx < supersample * LCD_WIDTH; sx += LCD_WIDTH) {
      //Compute the camera ray for every pixel
      ray.dir.z = Fixed24(1);
      for (int24_t y = 0; y < LCD_HEIGHT; y++) {
        // Accumulate error over each row
        Color24 error(0, 0, 0);

        for (int24_t x = 0; x < LCD_WIDTH; x++) {
          // Increment the ray direction for each pixel we consider
          ray.dir.y = top  - (Fixed24(sy + y) * cam_scl * Fixed24(1.0f / supersample));
          ray.dir.x = left + (Fixed24(sx + x) * cam_scl * Fixed24(1.0f / supersample));

          Color24 color24 = compute_ray(ray, true).toColor24();

          color24 += error;

          VRAM[pixelOff++] = color24.toColor16(error);
        }
      }

      // Reset offset to the top-left corner
      pixelOff = 0;
    }
  }
}

/* Renders the scene at standard size (no supersampling)
 */
void render() {
  ray.origin = cam_origin;

  //Compute the camera ray for every pixel
  ray.dir.z = Fixed24(1);
  ray.dir.y = top;
  for (int24_t y = 0; y < LCD_HEIGHT; y += grain) {
    // Increment the ray direction for each pixel we consider
    ray.dir.y -= (cam_scl * grain);
    ray.dir.x = left;

    // Accumulate error over each row
    Color24 error(0, 0, 0);

    for (int24_t x = 0; x < LCD_WIDTH; x += grain) {
      ray.dir.x += (cam_scl * grain);

      Color24 color24 = compute_ray(ray, true).toColor24();
      
      color24 += error;

      Color color = color24.toColor16(error);

      // If we are granularity 1, just place the pixel sequentially
      if (grain == 1) {
        VRAM[pixelOff++] = color;
      }
      // Otherwise, fill a box with the color
      else {
        for (uint8_t py = 0; py < grain; py++) {
          for (uint8_t px = 0; px < grain; px++) {
            VRAM[(x + px) + (LCD_WIDTH * (y + py))] = color;
          }
        }
      }
    }
  }
}

int main(void)
{
  /* Clear the homescreen */
  os_ClrHome();

  scene_init();
  
  if (supersample == 1) {
    render();
  }
  else
  {
    render_supersample();
  }
  
  /* Waits for a key */
  while (!os_GetCSC());

  return 0;
}