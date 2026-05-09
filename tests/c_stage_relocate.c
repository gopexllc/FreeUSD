#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      "    relocates = {\n"
      "        </Src>: </Dst>,\n"
      "    }\n"
      ")\n"
      "def Xform \"Src\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n"
      "def Xform \"Dst\"\n"
      "(\n"
      ")\n"
      "{\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_reloc");
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

  int has = freeusd_stage_relocate_source_in_any_layer(stage, "/Src");
  if (has != 1) {
    fprintf(stderr, "relocate_source expected 1 got %d\n", has);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }

  char* tgt = NULL;
  if (freeusd_stage_get_composed_relocate_target_utf8(stage, "/Src", &tgt) != FREEUSD_OK || !tgt ||
      strcmp(tgt, "/Dst") != 0) {
    fprintf(stderr, "get relocate target\n");
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
  if (freeusd_stage_list_composed_relocate_pairs_utf8(stage, &pairs, &np) != FREEUSD_OK || np != 1u || !pairs) {
    fprintf(stderr, "list pairs\n");
    if (pairs) {
      freeusd_path_list_free(pairs, np);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }
  const char sep = '\x1f';
  char expect[32];
  if (snprintf(expect, sizeof expect, "/Src%c/Dst", sep) >= (int)sizeof expect) {
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

  if (freeusd_stage_relocate_source_in_any_layer(stage, "/Nope") != 0) {
    fprintf(stderr, "expected 0 for /Nope\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
