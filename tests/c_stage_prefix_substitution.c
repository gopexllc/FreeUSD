#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      "    prefixSubstitutions = {\n"
      "        \"/Models\": \"/ModelsV2\",\n"
      "    }\n"
      ")\n"
      "def Xform \"Root\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_psub");
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

  int has = freeusd_stage_prefix_substitution_key_in_any_layer(stage, "/Models");
  if (has != 1) {
    fprintf(stderr, "key_in_any_layer expected 1 got %d\n", has);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }

  char* tgt = NULL;
  if (freeusd_stage_get_composed_prefix_substitution_utf8(stage, "/Models", &tgt) != FREEUSD_OK || !tgt ||
      strcmp(tgt, "/ModelsV2") != 0) {
    fprintf(stderr, "get composed prefix\n");
    if (tgt) {
      freeusd_string_free(tgt);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 5;
  }
  freeusd_string_free(tgt);

  char** pairs = NULL;
  size_t np = 0;
  if (freeusd_stage_list_composed_prefix_substitution_pairs_utf8(stage, &pairs, &np) != FREEUSD_OK || np != 1u ||
      !pairs) {
    fprintf(stderr, "list pairs\n");
    if (pairs) {
      freeusd_path_list_free(pairs, np);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }
  const char sep = '\x1f';
  char expect[64];
  if (snprintf(expect, sizeof expect, "/Models%c/ModelsV2", sep) >= (int)sizeof expect) {
    fprintf(stderr, "expect buf\n");
    freeusd_path_list_free(pairs, np);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }
  if (strcmp(pairs[0], expect) != 0) {
    fprintf(stderr, "pair string got [%s] want [%s]\n", pairs[0], expect);
    freeusd_path_list_free(pairs, np);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }
  freeusd_path_list_free(pairs, np);

  if (freeusd_stage_prefix_substitution_key_in_any_layer(stage, "/Nope") != 0) {
    fprintf(stderr, "expected 0 for /Nope key\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }

  if (freeusd_stage_get_composed_prefix_substitution_utf8(stage, "/Nope", &tgt) != FREEUSD_ERR_NOT_FOUND) {
    fprintf(stderr, "expected NOT_FOUND for /Nope\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 10;
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
