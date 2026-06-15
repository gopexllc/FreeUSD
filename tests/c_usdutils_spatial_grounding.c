#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usdutils_spatial_grounding"
#endif

static int nearly(double a, double b) { return fabs(a - b) <= 1e-9; }

static const FreeusdSpatialGroundingRecord* find_record(const FreeusdSpatialGroundingRecord* records, size_t count,
                                                        const char* path) {
  for (size_t i = 0; i < count; ++i) {
    if (records[i].path_utf8 && strcmp(records[i].path_utf8, path) == 0) {
      return &records[i];
    }
  }
  return NULL;
}

static int has_sibling(const FreeusdSpatialGroundingRecord* record, const char* name) {
  for (size_t i = 0; i < record->sibling_name_count; ++i) {
    if (record->sibling_names_utf8[i] && strcmp(record->sibling_names_utf8[i], name) == 0) {
      return 1;
    }
  }
  return 0;
}

static const FreeusdSemanticLabelSet* find_label_set(const FreeusdSpatialGroundingRecord* record, const char* name) {
  for (size_t i = 0; i < record->semantic_label_set_count; ++i) {
    if (record->semantic_label_sets[i].name_utf8 && strcmp(record->semantic_label_sets[i].name_utf8, name) == 0) {
      return &record->semantic_label_sets[i];
    }
  }
  return NULL;
}

int main(void) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/parity_spatial_grounding.usda", FREEUSD_TEST_FIXTURES_DIR) >=
      (int)sizeof path) {
    fprintf(stderr, "fixture path too long\n");
    return 1;
  }

  FreeusdStage* stage = freeusd_stage_open_from_root_file_utf8(path, 2);
  if (!stage) {
    fprintf(stderr, "stage open failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  FreeusdSpatialGroundingRecord* records = NULL;
  size_t count = 0;
  if (freeusd_usdutils_build_spatial_grounding_context(stage, 1.0, &records, &count) != FREEUSD_OK) {
    fprintf(stderr, "spatial context failed: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    return 3;
  }
  if (count != 5 || !records) {
    fprintf(stderr, "unexpected record count: %zu\n", count);
    freeusd_usdutils_spatial_grounding_records_free(records, count);
    freeusd_stage_free(stage);
    return 4;
  }

  const FreeusdSpatialGroundingRecord* cup = find_record(records, count, "/World/Kitchen/CupBlue");
  if (!cup || strcmp(cup->name_utf8, "CupBlue") != 0 || strcmp(cup->parent_path_utf8, "/World/Kitchen") != 0) {
    fprintf(stderr, "missing cup record\n");
    freeusd_usdutils_spatial_grounding_records_free(records, count);
    freeusd_stage_free(stage);
    return 5;
  }
  if (cup->sibling_name_count != 2 || !has_sibling(cup, "PlateGreen") || !has_sibling(cup, "Stove")) {
    fprintf(stderr, "unexpected sibling names\n");
    freeusd_usdutils_spatial_grounding_records_free(records, count);
    freeusd_stage_free(stage);
    return 6;
  }
  const FreeusdSemanticLabelSet* soma = find_label_set(cup, "somaHome");
  const FreeusdSemanticLabelSet* engine = find_label_set(cup, "engine");
  if (cup->semantic_label_set_count != 2 || !soma || !engine || soma->label_count != 2 || engine->label_count != 2 ||
      strcmp(soma->labels_utf8[0], "Crockery") != 0 || strcmp(soma->labels_utf8[1], "DesignedContainer") != 0 ||
      strcmp(engine->labels_utf8[0], "pickup") != 0 || strcmp(engine->labels_utf8[1], "container") != 0) {
    fprintf(stderr, "unexpected semantic label sets\n");
    freeusd_usdutils_spatial_grounding_records_free(records, count);
    freeusd_stage_free(stage);
    return 7;
  }
  if (!nearly(cup->world_position[0], 6.0) || !nearly(cup->world_position[1], 2.0) ||
      !nearly(cup->world_position[2], 3.0) || !cup->has_world_bound ||
      !nearly(cup->world_bound_dimensions[0], 0.5) || !nearly(cup->world_bound_dimensions[1], 1.5) ||
      !nearly(cup->world_bound_dimensions[2], 0.25) || !cup->has_mass_kg || fabs(cup->mass_kg - 0.35) > 1e-6) {
    fprintf(stderr, "unexpected cup spatial values\n");
    freeusd_usdutils_spatial_grounding_records_free(records, count);
    freeusd_stage_free(stage);
    return 8;
  }

  const FreeusdSpatialGroundingRecord* kitchen = find_record(records, count, "/World/Kitchen");
  if (!kitchen || strcmp(kitchen->parent_path_utf8, "/World") != 0 || kitchen->sibling_name_count != 0 ||
      kitchen->semantic_label_set_count != 0 || !nearly(kitchen->world_position[0], 10.0) || kitchen->has_world_bound ||
      kitchen->has_mass_kg) {
    fprintf(stderr, "unexpected kitchen record\n");
    freeusd_usdutils_spatial_grounding_records_free(records, count);
    freeusd_stage_free(stage);
    return 9;
  }

  freeusd_usdutils_spatial_grounding_records_free(records, count);
  freeusd_stage_free(stage);
  return 0;
}
