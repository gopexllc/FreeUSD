#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_usd_cross_language"
#endif

static int path_in_list(const char* needle, char** arr, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (strcmp(arr[i], needle) == 0) {
      return 1;
    }
  }
  return 0;
}

static char* read_fixture_usda(size_t* out_len) {
  char path[4096];
  if (snprintf(path, sizeof path, "%s/usd_cross_language.usda", FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof path) {
    return NULL;
  }
  FILE* f = fopen(path, "rb");
  if (!f) {
    return NULL;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long sz = ftell(f);
  if (sz < 0) {
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }
  char* buf = (char*)malloc((size_t)sz + 1u);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  const size_t nread = fread(buf, 1u, (size_t)sz, f);
  fclose(f);
  if (nread != (size_t)sz) {
    free(buf);
    return NULL;
  }
  buf[sz] = '\0';
  *out_len = (size_t)sz;
  return buf;
}

int main(void) {
  size_t len = 0;
  char* usda = read_fixture_usda(&len);
  if (!usda || len == 0) {
    fprintf(stderr, "read_fixture_usda failed (path under FREEUSD_TEST_FIXTURES_DIR)\n");
    return 1;
  }

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_cross_lang");
  if (!layer) {
    fprintf(stderr, "layer_new: %s\n", freeusd_last_error_message());
    free(usda);
    return 2;
  }

  if (freeusd_layer_load_usda_from_string(layer, usda) != FREEUSD_OK) {
    fprintf(stderr, "load: %s\n", freeusd_last_error_message());
    free(usda);
    freeusd_layer_free(layer);
    return 3;
  }
  free(usda);

  char* doc = freeusd_layer_get_documentation_utf8(layer);
  if (!doc || strcmp(doc, "cross_lang_fixture") != 0) {
    fprintf(stderr, "layer documentation mismatch\n");
    if (doc) {
      freeusd_string_free(doc);
    }
    freeusd_layer_free(layer);
    return 4;
  }
  freeusd_string_free(doc);

  char* dp = NULL;
  if (freeusd_layer_get_default_prim_utf8(layer, &dp) != FREEUSD_OK || !dp || strcmp(dp, "Scene") != 0) {
    fprintf(stderr, "layer defaultPrim\n");
    if (dp) {
      freeusd_string_free(dp);
    }
    freeusd_layer_free(layer);
    return 5;
  }
  freeusd_string_free(dp);

  FreeusdStage* stage = freeusd_stage_attach_root_layer(layer);
  if (!stage) {
    fprintf(stderr, "stage_attach: %s\n", freeusd_last_error_message());
    freeusd_layer_free(layer);
    return 6;
  }

  int has_dp = freeusd_stage_has_default_prim(stage);
  if (has_dp != 1) {
    fprintf(stderr, "stage_has_default_prim %d\n", has_dp);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }

  char* sdp = NULL;
  if (freeusd_stage_get_default_prim_utf8(stage, &sdp) != FREEUSD_OK || !sdp || strcmp(sdp, "Scene") != 0) {
    fprintf(stderr, "stage default prim name\n");
    if (sdp) {
      freeusd_string_free(sdp);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }
  freeusd_string_free(sdp);

  char** children = NULL;
  size_t nc = 0;
  if (freeusd_stage_list_child_paths(stage, "/Scene", &children, &nc) != FREEUSD_OK ||
      !path_in_list("/Scene/Child", children, nc)) {
    fprintf(stderr, "child paths under /Scene\n");
    if (children) {
      freeusd_path_list_free(children, nc);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 9;
  }
  freeusd_path_list_free(children, nc);

  int pk = freeusd_stage_prim_custom_data_key_in_any_layer(stage, "/Scene/Child", "tag");
  if (pk != 1) {
    fprintf(stderr, "prim_custom_data_key tag expected 1 got %d\n", pk);
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 10;
  }

  double mass = 0;
  if (freeusd_stage_read_field_double(stage, "/Scene/Child", "mass", 1.0, &mass) != FREEUSD_OK || mass != 2.5) {
    fprintf(stderr, "mass @1.0\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 11;
  }
  if (freeusd_stage_read_field_double(stage, "/Scene/Child", "mass", 2.0, &mass) != FREEUSD_OK || mass != 4.0) {
    fprintf(stderr, "mass @2.0\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 12;
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
