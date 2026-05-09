#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_stage_open_from_file"
#endif

int main(void) {
  char root_path[4096];
  if (snprintf(root_path, sizeof root_path, "%s/stage_open_root.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof root_path) {
    fprintf(stderr, "path buffer\n");
    return 1;
  }

  FreeusdStage* st0 = freeusd_stage_open_from_root_file_utf8(root_path, 0);
  if (!st0) {
    fprintf(stderr, "open none: %s\n", freeusd_last_error_message());
    return 2;
  }
  if (freeusd_stage_prim_is_valid(st0, "/FromSub") != 0) {
    fprintf(stderr, "expected /FromSub absent with policy 0\n");
    freeusd_stage_free(st0);
    return 3;
  }
  freeusd_stage_free(st0);

  FreeusdStage* st2 = freeusd_stage_open_from_root_file_utf8(root_path, 2);
  if (!st2) {
    fprintf(stderr, "open df: %s\n", freeusd_last_error_message());
    return 4;
  }
  if (freeusd_stage_prim_is_valid(st2, "/FromSub") != 1) {
    fprintf(stderr, "expected /FromSub with depth_first\n");
    freeusd_stage_free(st2);
    return 5;
  }
  size_t nly = 0;
  if (freeusd_stage_compose_layer_count(st2, &nly) != FREEUSD_OK || nly < 2u) {
    fprintf(stderr, "expected >=2 compose layers\n");
    freeusd_stage_free(st2);
    return 6;
  }
  freeusd_stage_free(st2);

  if (freeusd_stage_open_from_root_file_utf8(root_path, 99) != NULL) {
    fprintf(stderr, "expected NULL for bad policy\n");
    return 7;
  }

  return 0;
}
