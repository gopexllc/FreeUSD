#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_shade"
#endif

static int flt_eq(float a, float b) { return fabsf(a - b) <= 1e-5f; }

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_shade_preview.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  char* shader_path = NULL;
  if (freeusd_stage_read_material_surface_shader_path(stage, "/World/Looks/Material", &shader_path) != FREEUSD_OK) {
    fprintf(stderr, "surface shader path failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (strcmp(shader_path, "/World/Looks/Material/PreviewSurface") != 0) {
    fprintf(stderr, "unexpected shader path: %s\n", shader_path);
    freeusd_string_free(shader_path);
    freeusd_stage_free(stage);
    return 4;
  }

  float rgb[3] = {0.0f, 0.0f, 0.0f};
  if (freeusd_stage_read_preview_surface_diffuse_color(stage, shader_path, 1.0, rgb) != FREEUSD_OK) {
    fprintf(stderr, "diffuse read failed: %s\n", freeusd_last_error_message());
    freeusd_string_free(shader_path);
    freeusd_stage_free(stage);
    return 5;
  }
  freeusd_string_free(shader_path);

  if (!flt_eq(rgb[0], 0.8f) || !flt_eq(rgb[1], 0.2f) || !flt_eq(rgb[2], 0.1f)) {
    fprintf(stderr, "unexpected diffuseColor\n");
    freeusd_stage_free(stage);
    return 6;
  }

  if (snprintf(path, sizeof path, "%s/parity_shade_texture.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }
  stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "texture stage open failed: %s\n", freeusd_last_error_message());
    return 7;
  }
  shader_path = NULL;
  if (freeusd_stage_read_material_surface_shader_path(stage, "/World/Looks/Material", &shader_path) != FREEUSD_OK) {
    fprintf(stderr, "texture surface shader path failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 8;
  }
  char* texture_path = NULL;
  if (freeusd_stage_read_preview_surface_diffuse_texture_asset_path(stage, shader_path, 1.0, &texture_path) !=
      FREEUSD_OK) {
    fprintf(stderr, "texture path read failed: %s\n", freeusd_last_error_message());
    freeusd_string_free(shader_path);
    freeusd_stage_free(stage);
    return 9;
  }
  if (strcmp(texture_path, "textures/albedo.png") != 0) {
    fprintf(stderr, "unexpected texture path: %s\n", texture_path);
    freeusd_string_free(texture_path);
    freeusd_string_free(shader_path);
    freeusd_stage_free(stage);
    return 10;
  }
  freeusd_string_free(texture_path);
  freeusd_string_free(shader_path);

  freeusd_stage_free(stage);
  return 0;
}
