#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  const char* ver = freeusd_version_string();
  if (!ver || ver[0] == '\0') {
    fprintf(stderr, "missing version string\n");
    return 1;
  }

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_smoke");
  if (!layer) {
    fprintf(stderr, "layer_new failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  const char usda[] =
      "#usda 1.0\n"
      "(\n)\n"
      "def Xform \"W\"\n"
      "{\n"
      "    def \"C\"\n"
      "    (\n"
      "        customData = { string tag = \"cd\" }\n"
      "    )\n"
      "    {\n"
      "        double x = 3.0\n"
      "        string label = \"hi\"\n"
      "    }\n"
      "}\n";

  if (freeusd_layer_load_usda_from_string(layer, usda) != FREEUSD_OK) {
    fprintf(stderr, "load failed: %s\n", freeusd_last_error_message());
    freeusd_layer_free(layer);
    return 3;
  }

  char* saved = freeusd_layer_save_usda_to_string(layer);
  if (!saved) {
    fprintf(stderr, "save failed: %s\n", freeusd_last_error_message());
    freeusd_layer_free(layer);
    return 4;
  }
  freeusd_string_free(saved);

  FreeusdStage* stage = freeusd_stage_attach_root_layer(layer);
  if (!stage) {
    fprintf(stderr, "stage failed: %s\n", freeusd_last_error_message());
    freeusd_layer_free(layer);
    return 5;
  }

  size_t nroot = 0;
  if (freeusd_stage_compose_layer_count(stage, &nroot) != FREEUSD_OK || nroot != 1) {
    fprintf(stderr, "compose_layer_count root expected 1\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 50;
  }

  char** kids = NULL;
  size_t nk = 0;
  if (freeusd_stage_list_child_paths(stage, "/", &kids, &nk) != FREEUSD_OK) {
    fprintf(stderr, "list_child_paths: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 51;
  }
  if (nk != 1 || strcmp(kids[0], "/W") != 0) {
    fprintf(stderr, "expected one child /W got count=%zu\n", nk);
    freeusd_path_list_free(kids, nk);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 52;
  }
  freeusd_path_list_free(kids, nk);

  if (freeusd_stage_prim_is_valid(stage, "/W/C") != 1) {
    fprintf(stderr, "prim invalid (err=%s)\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }

  double d = 0.0;
  if (freeusd_stage_read_field_double(stage, "/W/C", "x", 1.0, &d) != FREEUSD_OK) {
    fprintf(stderr, "read_field_double failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }
  if (fabs(d - 3.0) > 1e-9) {
    fprintf(stderr, "unexpected value %g\n", d);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }

  char* lab = NULL;
  if (freeusd_stage_read_field_string(stage, "/W/C", "label", 1.0, &lab) != FREEUSD_OK) {
    fprintf(stderr, "read_field_string: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 53;
  }
  if (strcmp(lab, "hi") != 0) {
    fprintf(stderr, "label mismatch\n");
    freeusd_string_free(lab);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 54;
  }
  freeusd_string_free(lab);

  char* cdt = NULL;
  if (freeusd_stage_get_composed_prim_custom_data(stage, "/W/C", "tag", &cdt) != FREEUSD_OK) {
    fprintf(stderr, "customData: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 55;
  }
  if (strcmp(cdt, "cd") != 0) {
    fprintf(stderr, "customData mismatch\n");
    freeusd_string_free(cdt);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 56;
  }
  freeusd_string_free(cdt);

  int active = 0;
  if (freeusd_stage_resolve_prim_active(stage, "/W/C", &active) != FREEUSD_OK || active != 1) {
    fprintf(stderr, "resolve active failed\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }

  int hask = freeusd_stage_resolve_has_prim_kind(stage, "/W");
  if (hask != 1) {
    fprintf(stderr, "expected has_prim_kind on /W\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 57;
  }

  char* kind = freeusd_stage_resolve_prim_kind(stage, "/W");
  if (!kind) {
    fprintf(stderr, "expected kind on /W\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 10;
  }
  if (strcmp(kind, "Xform") != 0) {
    fprintf(stderr, "unexpected kind %s\n", kind);
    freeusd_string_free(kind);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 11;
  }
  freeusd_string_free(kind);

  freeusd_stage_free(stage);

  /* Layer stack: strongest layer first */
  FreeusdLayer* strong = freeusd_layer_new_anonymous("s");
  FreeusdLayer* weak = freeusd_layer_new_anonymous("w");
  if (!strong || !weak) {
    fprintf(stderr, "layer_new stack layers\n");
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 12;
  }
  const char usda_s[] =
      "#usda 1.0\n"
      "(\n)\n"
      "def \"R\"\n"
      "{\n"
      "    def \"X\"\n"
      "    {\n"
      "        double v = 1.0\n"
      "    }\n"
      "}\n";
  const char usda_w[] =
      "#usda 1.0\n"
      "(\n)\n"
      "def \"R\"\n"
      "{\n"
      "    def \"X\"\n"
      "    {\n"
      "        double v = 99.0\n"
      "    }\n"
      "}\n";
  if (freeusd_layer_load_usda_from_string(strong, usda_s) != FREEUSD_OK ||
      freeusd_layer_load_usda_from_string(weak, usda_w) != FREEUSD_OK) {
    fprintf(stderr, "stack layer load: %s\n", freeusd_last_error_message());
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 13;
  }

  FreeusdLayerStack* stk = freeusd_layer_stack_new();
  if (!stk) {
    fprintf(stderr, "stack_new\n");
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 14;
  }
  freeusd_layer_stack_append(stk, strong);
  freeusd_layer_stack_append(stk, weak);

  FreeusdStage* st2 = freeusd_stage_attach_layer_stack(stk);
  freeusd_layer_stack_free(stk);
  if (!st2) {
    fprintf(stderr, "attach_layer_stack: %s\n", freeusd_last_error_message());
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 15;
  }

  size_t nlay = 0;
  if (freeusd_stage_compose_layer_count(st2, &nlay) != FREEUSD_OK || nlay != 2) {
    fprintf(stderr, "compose_layer_count expected 2 got %zu\n", nlay);
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 16;
  }

  d = 0.0;
  if (freeusd_stage_read_field_double(st2, "/R/X", "v", 1.0, &d) != FREEUSD_OK || fabs(d - 1.0) > 1e-9) {
    fprintf(stderr, "stack strongest double expected 1.0 got %g err=%s\n", d, freeusd_last_error_message());
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 17;
  }

  freeusd_stage_free(st2);
  freeusd_layer_free(strong);
  freeusd_layer_free(weak);

  freeusd_layer_free(layer);
  return 0;
}
