#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_vol"
#endif

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_vol_openvdb.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  char* file_path = NULL;
  char* field_name = NULL;
  if (freeusd_stage_read_openvdb_asset_info(stage, "/World/Smoke", 1.0, &file_path, &field_name) != FREEUSD_OK) {
    fprintf(stderr, "OpenVDBAsset read failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }

  if (strcmp(file_path, "volumes/smoke.vdb") != 0 || strcmp(field_name, "density") != 0) {
    fprintf(stderr, "unexpected OpenVDBAsset info: %s %s\n", file_path, field_name);
    freeusd_string_free(file_path);
    freeusd_string_free(field_name);
    freeusd_stage_free(stage);
    return 4;
  }

  freeusd_string_free(file_path);
  freeusd_string_free(field_name);
  freeusd_stage_free(stage);
  return 0;
}
