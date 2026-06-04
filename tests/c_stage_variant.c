#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_stage_variant"
#endif

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      ")\n"
      "def Xform \"Root\"\n"
      "(\n"
      "    variantSelection = {\n"
      "        string modelVariant = \"HiPoly\",\n"
      "    }\n"
      "    variantSets = {\n"
      "        modelVariant = {\n"
      "            \"HiPoly\" = {},\n"
      "            LoPoly = {},\n"
      "        }\n"
      "    }\n"
      ")\n"
      "{\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_var");
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

  if (freeusd_stage_prim_variant_selection_set_in_any_layer(stage, "/Root", "modelVariant") != 1) {
    fprintf(stderr, "expected variantSelection modelVariant\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }

  char* sel = NULL;
  if (freeusd_stage_get_composed_prim_variant_selection_utf8(stage, "/Root", "modelVariant", &sel) != FREEUSD_OK ||
      !sel || strcmp(sel, "HiPoly") != 0) {
    fprintf(stderr, "composed selection\n");
    if (sel) {
      freeusd_string_free(sel);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 5;
  }
  freeusd_string_free(sel);

  char** sets = NULL;
  size_t ns = 0;
  if (freeusd_stage_list_composed_prim_variant_selection_sets_utf8(stage, "/Root", &sets, &ns) != FREEUSD_OK ||
      ns != 1u || !sets || strcmp(sets[0], "modelVariant") != 0) {
    fprintf(stderr, "list selection sets\n");
    if (sets) {
      freeusd_path_list_free(sets, ns);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }
  freeusd_path_list_free(sets, ns);

  if (freeusd_stage_prim_variant_set_declared_in_any_layer(stage, "/Root", "modelVariant") != 1) {
    fprintf(stderr, "expected variantSet modelVariant\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }

  char** vsets = NULL;
  size_t nv = 0;
  if (freeusd_stage_list_composed_prim_variant_set_names_utf8(stage, "/Root", &vsets, &nv) != FREEUSD_OK || nv != 1u ||
      !vsets || strcmp(vsets[0], "modelVariant") != 0) {
    fprintf(stderr, "list variant set names\n");
    if (vsets) {
      freeusd_path_list_free(vsets, nv);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }
  freeusd_path_list_free(vsets, nv);

  char** names = NULL;
  size_t nn = 0;
  if (freeusd_stage_list_composed_prim_variant_names_utf8(stage, "/Root", "modelVariant", &names, &nn) != FREEUSD_OK ||
      nn != 2u || !names) {
    fprintf(stderr, "list variant names rc\n");
    if (names) {
      freeusd_path_list_free(names, nn);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }
  if (strcmp(names[0], "HiPoly") != 0 || strcmp(names[1], "LoPoly") != 0) {
    fprintf(stderr, "variant name order [%s] [%s]\n", names[0], names[1]);
    freeusd_path_list_free(names, nn);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 10;
  }
  freeusd_path_list_free(names, nn);

  if (freeusd_stage_get_composed_prim_variant_selection_utf8(stage, "/Root", "missing", &sel) !=
      FREEUSD_ERR_NOT_FOUND) {
    fprintf(stderr, "expected NOT_FOUND selection\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 11;
  }

  if (freeusd_stage_list_composed_prim_variant_names_utf8(stage, "/Root", "missing", &names, &nn) !=
      FREEUSD_ERR_NOT_FOUND) {
    fprintf(stderr, "expected NOT_FOUND variant names\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 12;
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
