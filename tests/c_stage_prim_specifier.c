#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      ")\n"
      "class Xform \"P\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n"
      "over Xform \"O\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n"
      "def Xform \"Q\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_spec");
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

  if (freeusd_stage_resolve_prim_specifier_kind(stage, "/P") != FREEUSD_PRIM_SPECIFIER_CLASS) {
    fprintf(stderr, "/P expected CLASS\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }
  if (freeusd_stage_resolve_prim_specifier_kind(stage, "/O") != FREEUSD_PRIM_SPECIFIER_OVER) {
    fprintf(stderr, "/O expected OVER\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 5;
  }
  if (freeusd_stage_resolve_prim_specifier_kind(stage, "/Q") != FREEUSD_PRIM_SPECIFIER_DEFAULT) {
    fprintf(stderr, "/Q expected DEFAULT\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }

  if (freeusd_stage_resolve_prim_specifier_kind(stage, "not_a_path") >= 0) {
    fprintf(stderr, "expected negative error for bad path\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
