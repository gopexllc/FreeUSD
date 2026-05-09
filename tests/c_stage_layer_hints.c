#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int approx_eq(double a, double b) { return fabs(a - b) < 1e-9; }

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      "    startTimeCode = 0\n"
      "    endTimeCode = 100\n"
      "    timeCodesPerSecond = 30\n"
      "    framesPerSecond = 30\n"
      "    framePrecision = 2\n"
      "    metersPerUnit = 0.01\n"
      "    upAxis = \"Z\"\n"
      "    primOrder = [</Root/A>, </Root>]\n"
      ")\n"
      "def Xform \"Root\"\n"
      "{\n"
      "    def \"A\"\n"
      "    {\n"
      "    }\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_hints");
  if (!layer) {
    return 1;
  }
  if (freeusd_layer_load_usda_from_string(layer, usda) != FREEUSD_OK) {
    fprintf(stderr, "load: %s\n", freeusd_last_error_message());
    freeusd_layer_free(layer);
    return 2;
  }

  FreeusdStage* stage = freeusd_stage_attach_root_layer(layer);
  if (!stage) {
    fprintf(stderr, "stage\n");
    freeusd_layer_free(layer);
    return 3;
  }

  double dv = 0.0;
  int has = 0;
  if (freeusd_stage_get_start_time_code(stage, &dv, &has) != FREEUSD_OK || has != 1 || !approx_eq(dv, 0.0)) {
    fprintf(stderr, "startTimeCode\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }
  if (freeusd_stage_get_end_time_code(stage, &dv, &has) != FREEUSD_OK || has != 1 || !approx_eq(dv, 100.0)) {
    fprintf(stderr, "endTimeCode\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 5;
  }
  if (freeusd_stage_get_time_codes_per_second(stage, &dv, &has) != FREEUSD_OK || has != 1 || !approx_eq(dv, 30.0)) {
    fprintf(stderr, "timeCodesPerSecond\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }
  if (freeusd_stage_get_frames_per_second(stage, &dv, &has) != FREEUSD_OK || has != 1 || !approx_eq(dv, 30.0)) {
    fprintf(stderr, "framesPerSecond\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }
  int64_t iv = 0;
  if (freeusd_stage_get_frame_precision(stage, &iv, &has) != FREEUSD_OK || has != 1 || iv != 2) {
    fprintf(stderr, "framePrecision\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }
  if (freeusd_stage_get_meters_per_unit(stage, &dv, &has) != FREEUSD_OK || has != 1 || !approx_eq(dv, 0.01)) {
    fprintf(stderr, "metersPerUnit\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }

  char* axis = NULL;
  if (freeusd_stage_get_up_axis_utf8(stage, &axis) != FREEUSD_OK || !axis || strcmp(axis, "Z") != 0) {
    fprintf(stderr, "upAxis\n");
    if (axis) {
      freeusd_string_free(axis);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 10;
  }
  freeusd_string_free(axis);

  char** po = NULL;
  size_t np = 0;
  if (freeusd_stage_list_prim_order_paths_utf8(stage, &po, &np) != FREEUSD_OK || np != 2u || !po) {
    fprintf(stderr, "primOrder list\n");
    if (po) {
      freeusd_path_list_free(po, np);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 11;
  }
  if (strcmp(po[0], "/Root/A") != 0 || strcmp(po[1], "/Root") != 0) {
    fprintf(stderr, "primOrder order\n");
    freeusd_path_list_free(po, np);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 12;
  }
  freeusd_path_list_free(po, np);

  FreeusdLayer* bare = freeusd_layer_new_anonymous("c_bare");
  if (!bare) {
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 13;
  }
  const char bare_usda[] =
      "#usda 1.0\n"
      "(\n"
      ")\n"
      "def Xform \"X\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n";
  if (freeusd_layer_load_usda_from_string(bare, bare_usda) != FREEUSD_OK) {
    freeusd_layer_free(bare);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 14;
  }
  FreeusdStage* st2 = freeusd_stage_attach_root_layer(bare);
  if (!st2) {
    freeusd_layer_free(bare);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 15;
  }
  if (freeusd_stage_get_start_time_code(st2, &dv, &has) != FREEUSD_OK || has != 0) {
    fprintf(stderr, "expected no startTimeCode\n");
    freeusd_stage_free(st2);
    freeusd_layer_free(bare);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 16;
  }
  if (freeusd_stage_get_up_axis_utf8(st2, &axis) != FREEUSD_ERR_NOT_FOUND) {
    fprintf(stderr, "expected NOT_FOUND upAxis\n");
    freeusd_stage_free(st2);
    freeusd_layer_free(bare);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 17;
  }
  if (freeusd_stage_list_prim_order_paths_utf8(st2, &po, &np) != FREEUSD_OK || np != 0u || po != NULL) {
    fprintf(stderr, "expected empty primOrder\n");
    if (po) {
      freeusd_path_list_free(po, np);
    }
    freeusd_stage_free(st2);
    freeusd_layer_free(bare);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 18;
  }

  freeusd_stage_free(st2);
  freeusd_layer_free(bare);
  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
