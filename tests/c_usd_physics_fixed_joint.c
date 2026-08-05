#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_physics_fixed_joint"
#endif

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_physics_fixed_joint.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  FreeusdPhysicsFixedJointSample sample = {0};
  if (freeusd_stage_read_physics_fixed_joint_sample(stage, "/World/Anchor", 1.0, &sample) != FREEUSD_OK) {
    fprintf(stderr, "PhysicsFixedJoint sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }

  if (!sample.body0_path_utf8 || strcmp(sample.body0_path_utf8, "/World/BodyA") != 0 ||
      !sample.body1_path_utf8 || strcmp(sample.body1_path_utf8, "/World/BodyB") != 0 ||
      sample.joint_enabled != 1) {
    fprintf(stderr, "unexpected PhysicsFixedJoint sample\n");
    freeusd_string_free(sample.body0_path_utf8);
    freeusd_string_free(sample.body1_path_utf8);
    freeusd_stage_free(stage);
    return 4;
  }

  freeusd_string_free(sample.body0_path_utf8);
  freeusd_string_free(sample.body1_path_utf8);
  freeusd_stage_free(stage);
  return 0;
}
