#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_lux"
#endif

static int flt_eq(float a, float b) { return fabsf(a - b) <= 1e-5f; }

static FreeusdStage* open_fixture(const char* name) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/%s", FREEUSD_TEST_FIXTURES_DIR, name) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return NULL;
  }
  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed for %s: %s\n", name, freeusd_last_error_message());
  }
  return stage;
}

int main(void) {
  FreeusdStage* stage = open_fixture("parity_lux_distant.usda");
  if (!stage) {
    return 2;
  }

  FreeusdLuxDistantLightSample sample = {0};
  if (freeusd_stage_read_lux_distant_light_sample(stage, "/World/Sun", 1.0, &sample) != FREEUSD_OK) {
    fprintf(stderr, "distant light sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }

  if (!flt_eq(sample.intensity, 1200.0f) || !flt_eq(sample.color[0], 1.0f) ||
      !flt_eq(sample.color[1], 0.95f) || !flt_eq(sample.color[2], 0.8f) || !flt_eq(sample.angle, 0.53f)) {
    fprintf(stderr, "unexpected distant light sample\n");
    freeusd_stage_free(stage);
    return 4;
  }

  freeusd_stage_free(stage);
  stage = open_fixture("parity_lux_sphere.usda");
  if (!stage) {
    return 5;
  }
  FreeusdLuxSphereLightSample sphere = {0};
  if (freeusd_stage_read_lux_sphere_light_sample(stage, "/World/Bulb", 1.0, &sphere) != FREEUSD_OK) {
    fprintf(stderr, "sphere light sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 6;
  }
  if (!flt_eq(sphere.intensity, 500.0f) || !flt_eq(sphere.color[1], 0.9f) ||
      !flt_eq(sphere.radius, 0.25f)) {
    fprintf(stderr, "unexpected sphere light sample\n");
    freeusd_stage_free(stage);
    return 7;
  }
  freeusd_stage_free(stage);

  stage = open_fixture("parity_lux_rect.usda");
  if (!stage) {
    return 8;
  }
  FreeusdLuxRectLightSample rect = {0};
  if (freeusd_stage_read_lux_rect_light_sample(stage, "/World/Panel", 1.0, &rect) != FREEUSD_OK) {
    fprintf(stderr, "rect light sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 9;
  }
  if (!flt_eq(rect.intensity, 1200.0f) || !flt_eq(rect.color[0], 0.95f) ||
      !flt_eq(rect.width, 2.0f) || !flt_eq(rect.height, 1.0f)) {
    fprintf(stderr, "unexpected rect light sample\n");
    freeusd_stage_free(stage);
    return 10;
  }
  freeusd_stage_free(stage);

  stage = open_fixture("parity_lux_disk.usda");
  if (!stage) {
    return 11;
  }
  FreeusdLuxDiskLightSample disk = {0};
  if (freeusd_stage_read_lux_disk_light_sample(stage, "/World/Softbox", 1.0, &disk) != FREEUSD_OK) {
    fprintf(stderr, "disk light sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 12;
  }
  if (!flt_eq(disk.intensity, 800.0f) || !flt_eq(disk.color[0], 1.0f) ||
      !flt_eq(disk.radius, 0.75f)) {
    fprintf(stderr, "unexpected disk light sample\n");
    freeusd_stage_free(stage);
    return 13;
  }
  freeusd_stage_free(stage);

  stage = open_fixture("parity_lux_cylinder.usda");
  if (!stage) {
    return 14;
  }
  FreeusdLuxCylinderLightSample cylinder = {0};
  if (freeusd_stage_read_lux_cylinder_light_sample(stage, "/World/Tube", 1.0, &cylinder) != FREEUSD_OK) {
    fprintf(stderr, "cylinder light sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 15;
  }
  if (!flt_eq(cylinder.length, 2.5f) || !flt_eq(cylinder.radius, 0.05f)) {
    fprintf(stderr, "unexpected cylinder light sample\n");
    freeusd_stage_free(stage);
    return 16;
  }
  freeusd_stage_free(stage);

  stage = open_fixture("parity_lux_dome.usda");
  if (!stage) {
    return 17;
  }
  FreeusdLuxDomeLightSample dome = {0};
  if (freeusd_stage_read_lux_dome_light_sample(stage, "/World/Sky", 1.0, &dome) != FREEUSD_OK) {
    fprintf(stderr, "dome light sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 18;
  }
  if (!flt_eq(dome.intensity, 1.0f) || strcmp(dome.texture_file_asset_path_utf8, "textures/sky.hdr") != 0 ||
      strcmp(dome.texture_format_utf8, "latlong") != 0) {
    fprintf(stderr, "unexpected dome light sample\n");
    freeusd_string_free(dome.texture_file_asset_path_utf8);
    freeusd_string_free(dome.texture_format_utf8);
    freeusd_stage_free(stage);
    return 19;
  }
  freeusd_string_free(dome.texture_file_asset_path_utf8);
  freeusd_string_free(dome.texture_format_utf8);
  freeusd_stage_free(stage);
  return 0;
}
