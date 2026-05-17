#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_skel"
#endif

static int nearly(double a, double b) { return fabs(a - b) <= 1e-5; }

static int flt_eq(float a, float b) { return fabsf(a - b) <= 1e-5f; }

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_skel_skinning.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  char** joint_names = NULL;
  size_t joint_count = 0;
  if (freeusd_stage_read_skel_joint_names(stage, "/World/SkelCharacter/Skeleton", &joint_names, &joint_count) !=
          FREEUSD_OK ||
      joint_count != 2) {
    fprintf(stderr, "read_skel_joint_names failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (strcmp(joint_names[0], "Root") != 0 || strcmp(joint_names[1], "Root/Hip") != 0) {
    fprintf(stderr, "unexpected joint names\n");
    freeusd_path_list_free(joint_names, joint_count);
    freeusd_stage_free(stage);
    return 4;
  }
  freeusd_path_list_free(joint_names, joint_count);

  size_t target_count = 0;
  if (freeusd_stage_read_geom_blend_shape_target_count(stage, "/World/SkelCharacter/Body", &target_count) ==
      FREEUSD_OK) {
    fprintf(stderr, "expected no blend shapes on skinning body\n");
    freeusd_stage_free(stage);
    return 5;
  }

  const float in_xyz[3] = {0.0f, 1.0f, 0.0f};
  const int indices[1] = {1};
  const float weights[1] = {1.0f};
  float out_xyz[3] = {0.0f, 0.0f, 0.0f};
  if (freeusd_stage_deform_points_with_skeleton(stage, "/World/SkelCharacter/Skeleton",
                                                "/World/SkelCharacter/Anim", 1.0, 1, in_xyz, indices, weights, 1,
                                                out_xyz) != FREEUSD_OK) {
    fprintf(stderr, "deform_points_with_skeleton failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 6;
  }
  if (out_xyz[1] <= in_xyz[1]) {
    fprintf(stderr, "deformed point did not move along +Y as expected\n");
    freeusd_stage_free(stage);
    return 7;
  }

  FreeusdEngineRuntimeSupport report;
  memset(&report, 0, sizeof report);
  if (freeusd_usdutils_assess_engine_runtime_support(stage, &report) != FREEUSD_OK) {
    fprintf(stderr, "assess_engine_runtime_support failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 8;
  }
  if (report.uses_skel_bound_meshes == 0 || report.uses_skel_animation == 0) {
    fprintf(stderr, "expected skel mesh and animation flags in runtime report\n");
    freeusd_stage_free(stage);
    return 9;
  }

  freeusd_stage_free(stage);

  double world[32];
  double bind[32];
  double palette[32];
  memset(world, 0, sizeof world);
  memset(bind, 0, sizeof bind);
  world[0] = world[5] = world[10] = world[15] = 1.0;
  bind[0] = bind[5] = bind[10] = bind[15] = 1.0;
  world[16 + 0] = world[16 + 5] = world[16 + 10] = world[16 + 15] = 1.0;
  bind[16 + 0] = bind[16 + 5] = bind[16 + 10] = bind[16 + 15] = 1.0;
  world[16 + 13] = 2.0;
  bind[16 + 13] = 1.0;

  if (freeusd_usdskel_compute_skinning_matrices(2, world, bind, palette) != FREEUSD_OK) {
    fprintf(stderr, "compute_skinning_matrices failed: %s\n", freeusd_last_error_message());
    return 10;
  }
  if (!nearly(palette[13], 0.0) || !nearly(palette[16 + 13], 3.0)) {
    fprintf(stderr, "unexpected skinning palette translation\n");
    return 11;
  }

  {
    double one_world[16];
    double one_bind[16];
    double one_palette[16];
    memset(one_world, 0, sizeof one_world);
    memset(one_bind, 0, sizeof one_bind);
    one_world[0] = one_world[5] = one_world[10] = one_world[15] = 1.0;
    one_world[13] = 2.0;
    one_bind[0] = one_bind[5] = one_bind[10] = one_bind[15] = 1.0;
    if (freeusd_usdskel_compute_skinning_matrices(1, one_world, one_bind, one_palette) != FREEUSD_OK) {
      fprintf(stderr, "single-joint skinning failed: %s\n", freeusd_last_error_message());
      return 12;
    }
    if (!nearly(one_palette[13], 2.0)) {
      fprintf(stderr, "single-joint palette translation mismatch\n");
      return 13;
    }
  }

  return 0;
}
