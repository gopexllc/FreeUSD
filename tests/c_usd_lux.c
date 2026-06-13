#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_lux"
#endif

static int flt_eq(float a, float b) { return fabsf(a - b) <= 1e-5f; }

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_lux_distant.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
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
  return 0;
}
