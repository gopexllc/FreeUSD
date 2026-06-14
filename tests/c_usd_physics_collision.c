#include <freeusd/c/freeusd.h>

#include <stdio.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_physics_collision"
#endif

static int open_path(char* out, size_t out_size, const char* fixture_name) {
  if (snprintf(out, out_size, "%s/%s", FREEUSD_TEST_FIXTURES_DIR, fixture_name) >= (int)out_size) {
    fprintf(stderr, "fixture path too long\n");
    return 0;
  }
  return 1;
}

static int check_fixture(const char* fixture_name) {
  char path[4096];
  if (!open_path(path, sizeof path, fixture_name)) {
    return 1;
  }
  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }
  FreeusdPhysicsCollisionSample sample = {0};
  if (freeusd_stage_read_physics_collision_sample(stage, "/World/Collider", 1.0, &sample) != FREEUSD_OK) {
    fprintf(stderr, "CollisionAPI sample failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (sample.collision_enabled != 0) {
    fprintf(stderr, "unexpected CollisionAPI sample\n");
    freeusd_stage_free(stage);
    return 4;
  }
  freeusd_stage_free(stage);
  return 0;
}

int main(void) {
  int rc = check_fixture("parity_physics_collision.usda");
  if (rc != 0) {
    return rc;
  }
  rc = check_fixture("parity_physics_collision_inherit.usda");
  if (rc != 0) {
    return 10 + rc;
  }
  return 0;
}
