#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      "    customLayerData = {\n"
      "        string layerTag = \"hello\",\n"
      "        string layerBuild = \"2025\",\n"
      "    }\n"
      ")\n"
      "def \"R\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_cld");
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

  if (freeusd_stage_custom_layer_data_key_in_any_layer(stage, "layerTag") != 1) {
    fprintf(stderr, "expected key layerTag\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }

  char* val = NULL;
  if (freeusd_stage_get_composed_custom_layer_data_utf8(stage, "layerTag", &val) != FREEUSD_OK || !val ||
      strcmp(val, "hello") != 0) {
    fprintf(stderr, "get layerTag\n");
    if (val) {
      freeusd_string_free(val);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 5;
  }
  freeusd_string_free(val);

  char** keys = NULL;
  size_t nk = 0;
  if (freeusd_stage_list_composed_custom_layer_data_keys(stage, &keys, &nk) != FREEUSD_OK || nk != 2u || !keys) {
    fprintf(stderr, "list keys\n");
    if (keys) {
      freeusd_path_list_free(keys, nk);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }
  if (strcmp(keys[0], "layerBuild") != 0 || strcmp(keys[1], "layerTag") != 0) {
    fprintf(stderr, "key order [%s] [%s]\n", keys[0], keys[1]);
    freeusd_path_list_free(keys, nk);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }
  freeusd_path_list_free(keys, nk);

  if (freeusd_stage_custom_layer_data_key_in_any_layer(stage, "nope") != 0) {
    fprintf(stderr, "expected 0 for missing key\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }

  if (freeusd_stage_get_composed_custom_layer_data_utf8(stage, "nope", &val) != FREEUSD_ERR_NOT_FOUND) {
    fprintf(stderr, "expected NOT_FOUND for nope\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
