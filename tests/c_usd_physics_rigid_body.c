#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_physics_rigid_body"
#endif

static int flt_eq(float a, float b) { return fabsf(a - b) <= 1e-5f; }

static int open_path(char* out, size_t out_size, const char* fixture_name) {
  if (snprintf(out, out_size, "%s/%s", FREEUSD_TEST_FIXTURES_DIR, fixture_name) >= (int)out_size) {
    fprintf(stderr, "fixture path too long\n");
    return 0;
  }
  return 1;
}

int main(void) {
  char path[4096];
  if (!open_path(path, sizeof path, "parity_physics_rigid_body.usda")) {
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  FreeusdPhysicsRigidBodySample sample = {0};
  if (freeusd_stage_read_physics_rigid_body_sample(stage, "/World/Body", 1.0, &sample) != FREEUSD_OK) {
    fprintf(stderr, "RigidBodyAPI sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (!flt_eq(sample.mass, 2.5f) || sample.has_kinematic_enabled != 0 || sample.kinematic_enabled != 0) {
    fprintf(stderr, "unexpected RigidBodyAPI sample\n");
    freeusd_stage_free(stage);
    return 4;
  }
  freeusd_stage_free(stage);

  if (!open_path(path, sizeof path, "parity_physics_rigid_body_kinematic.usda")) {
    return 5;
  }
  stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "kinematic stage open failed: %s\n", freeusd_last_error_message());
    return 6;
  }
  sample = (FreeusdPhysicsRigidBodySample){0};
  if (freeusd_stage_read_physics_rigid_body_sample(stage, "/World/Body", 1.0, &sample) != FREEUSD_OK) {
    fprintf(stderr, "kinematic RigidBodyAPI sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 7;
  }
  if (!flt_eq(sample.mass, 1.0f) || sample.has_kinematic_enabled != 1 || sample.kinematic_enabled != 1) {
    fprintf(stderr, "unexpected kinematic RigidBodyAPI sample\n");
    freeusd_stage_free(stage);
    return 8;
  }
  freeusd_stage_free(stage);
  return 0;
}
