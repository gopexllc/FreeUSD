#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_semantics_labels"
#endif

static int has_string(char** strings, size_t count, const char* want) {
  for (size_t i = 0; i < count; ++i) {
    if (strings[i] && strcmp(strings[i], want) == 0) {
      return 1;
    }
  }
  return 0;
}

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_semantics_labels.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  char** sets = NULL;
  size_t set_count = 0;
  if (freeusd_stage_list_semantic_label_sets(stage, "/World/Kitchen/CupBlue", &sets, &set_count) != FREEUSD_OK) {
    fprintf(stderr, "list label sets failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (set_count != 2 || !has_string(sets, set_count, "engine") || !has_string(sets, set_count, "somaHome")) {
    fprintf(stderr, "unexpected label sets\n");
    freeusd_path_list_free(sets, set_count);
    freeusd_stage_free(stage);
    return 4;
  }
  freeusd_path_list_free(sets, set_count);

  char** labels = NULL;
  size_t label_count = 0;
  if (freeusd_stage_read_semantic_labels(stage, "/World/Kitchen/CupBlue", "somaHome", &labels, &label_count) !=
      FREEUSD_OK) {
    fprintf(stderr, "read labels failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 5;
  }
  if (label_count != 2 || strcmp(labels[0], "Crockery") != 0 || strcmp(labels[1], "DesignedContainer") != 0) {
    fprintf(stderr, "unexpected somaHome labels\n");
    freeusd_path_list_free(labels, label_count);
    freeusd_stage_free(stage);
    return 6;
  }
  freeusd_path_list_free(labels, label_count);

  if (freeusd_stage_read_semantic_labels(stage, "/World/Kitchen/Stove", "somaHome", &labels, &label_count) !=
      FREEUSD_OK) {
    fprintf(stderr, "read stove labels failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 7;
  }
  if (label_count != 1 || strcmp(labels[0], "Appliance") != 0) {
    fprintf(stderr, "unexpected stove label\n");
    freeusd_path_list_free(labels, label_count);
    freeusd_stage_free(stage);
    return 8;
  }
  freeusd_path_list_free(labels, label_count);

  if (freeusd_stage_read_semantic_labels(stage, "/World/Kitchen/CupBlue", "missing", &labels, &label_count) !=
      FREEUSD_OK ||
      labels != NULL || label_count != 0) {
    fprintf(stderr, "missing label set should be empty\n");
    freeusd_path_list_free(labels, label_count);
    freeusd_stage_free(stage);
    return 9;
  }

  freeusd_stage_free(stage);
  return 0;
}
