#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_engine_integration"
#endif

static int nearly(double a, double b) { return fabs(a - b) <= 1e-9; }

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_imageable.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  double l2w[16];
  if (freeusd_stage_compute_local_to_world_transform_matrix4d(stage, "/World/Cube", 1.0, l2w) != FREEUSD_OK) {
    fprintf(stderr, "compute_local_to_world failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (!nearly(l2w[12], 1.0) || !nearly(l2w[13], 2.0) || !nearly(l2w[14], 3.0) || !nearly(l2w[15], 1.0)) {
    fprintf(stderr, "unexpected local_to_world translation row\n");
    freeusd_stage_free(stage);
    return 4;
  }

  int visible = 1;
  if (freeusd_stage_compute_imageable_visibility(stage, "/World/Cube", 1.0, &visible) != FREEUSD_OK || visible != 0) {
    fprintf(stderr, "compute_imageable_visibility failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 5;
  }

  char* purpose = NULL;
  if (freeusd_stage_compute_imageable_purpose_utf8(stage, "/World/Cube", 1.0, &purpose) != FREEUSD_OK || !purpose ||
      strcmp(purpose, "render") != 0) {
    fprintf(stderr, "compute_imageable_purpose failed: %s\n", freeusd_last_error_message());
    if (purpose) {
      freeusd_string_free(purpose);
    }
    freeusd_stage_free(stage);
    return 6;
  }
  freeusd_string_free(purpose);

  double min_x = 0, min_y = 0, min_z = 0, max_x = 0, max_y = 0, max_z = 0;
  if (freeusd_stage_compute_boundable_world_bounds(stage, "/World/Cube", 1.0, &min_x, &min_y, &min_z, &max_x, &max_y,
                                                   &max_z) != FREEUSD_OK) {
    fprintf(stderr, "compute_boundable_world_bounds failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 7;
  }
  if (!nearly(min_x, 0.0) || !nearly(min_y, 1.0) || !nearly(min_z, 2.0) || !nearly(max_x, 2.0) ||
      !nearly(max_y, 3.0) || !nearly(max_z, 4.0)) {
    fprintf(stderr, "unexpected world bounds\n");
    freeusd_stage_free(stage);
    return 8;
  }

  if (freeusd_stage_compute_boundable_world_bounds(stage, "/World", 1.0, &min_x, &min_y, &min_z, &max_x, &max_y,
                                                   &max_z) != FREEUSD_ERR_NOT_FOUND) {
    fprintf(stderr, "expected NOT_FOUND for non-boundable world prim\n");
    freeusd_stage_free(stage);
    return 9;
  }

  freeusd_stage_free(stage);

  if (snprintf(path, sizeof path, "%s/parity_embedded_scene.usdc", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "crate fixture path too long\n");
    return 10;
  }
  stage = freeusd_stage_open_from_root_file_utf8(path, 0);
  if (!stage) {
    fprintf(stderr, "crate stage open failed: %s\n", freeusd_last_error_message());
    return 11;
  }
  if (freeusd_stage_has_default_prim(stage) != 1) {
    fprintf(stderr, "crate stage missing default prim\n");
    freeusd_stage_free(stage);
    return 12;
  }
  char* default_prim = NULL;
  if (freeusd_stage_get_default_prim_utf8(stage, &default_prim) != FREEUSD_OK || !default_prim ||
      strcmp(default_prim, "World") != 0) {
    fprintf(stderr, "crate default prim mismatch: %s\n", freeusd_last_error_message());
    if (default_prim) {
      freeusd_string_free(default_prim);
    }
    freeusd_stage_free(stage);
    return 13;
  }
  freeusd_string_free(default_prim);
  if (freeusd_stage_prim_is_valid(stage, "/World/Cube") != 1) {
    fprintf(stderr, "crate stage missing /World/Cube\n");
    freeusd_stage_free(stage);
    return 14;
  }
  freeusd_stage_free(stage);
  return 0;
}
