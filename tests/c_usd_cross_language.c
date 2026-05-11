#include <freeusd/c/freeusd.h>

#include <math.h>
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

static int dbl_eq(double a, double b) { return fabs(a - b) < 1e-9; }

static int flt_eq(float a, float b) { return fabsf(a - b) < 1e-5f; }

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

  if (freeusd_stage_has_field_opinion(stage, "/Scene/Child", "mass") != 1 ||
      freeusd_stage_has_field_opinion(stage, "/Scene/Child", "missingAttr") != 0) {
    fprintf(stderr, "field opinion contract\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 13;
  }

  float density = 0.0f;
  if (freeusd_stage_read_field_float(stage, "/Scene/Child", "density", 1.0, &density) != FREEUSD_OK ||
      !flt_eq(density, 1.25f)) {
    fprintf(stderr, "density float\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 14;
  }

  float mass_f = 0.0f;
  if (freeusd_stage_read_field_float(stage, "/Scene/Child", "mass", 1.0, &mass_f) != FREEUSD_OK ||
      !flt_eq(mass_f, 2.5f)) {
    fprintf(stderr, "mass float coercion\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 15;
  }

  int enabled = 0;
  if (freeusd_stage_read_field_bool(stage, "/Scene/Child", "enabled", 1.0, &enabled) != FREEUSD_OK || enabled != 1) {
    fprintf(stderr, "enabled bool\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 16;
  }

  int64_t count = 0;
  if (freeusd_stage_read_field_int64(stage, "/Scene/Child", "count", 1.0, &count) != FREEUSD_OK || count != -7) {
    fprintf(stderr, "count int64\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 17;
  }

  int64_t mass_i = 0;
  if (freeusd_stage_read_field_int64(stage, "/Scene/Child", "mass", 1.0, &mass_i) != FREEUSD_OK || mass_i != 2) {
    fprintf(stderr, "mass int64 coercion\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 18;
  }

  char* label = NULL;
  if (freeusd_stage_read_field_string(stage, "/Scene/Child", "label", 1.0, &label) != FREEUSD_OK || !label ||
      strcmp(label, "hello") != 0) {
    fprintf(stderr, "label string\n");
    if (label) {
      freeusd_string_free(label);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 19;
  }
  freeusd_string_free(label);

  char* kind = NULL;
  if (freeusd_stage_read_field_string(stage, "/Scene/Child", "kind", 1.0, &kind) != FREEUSD_OK || !kind ||
      strcmp(kind, "component") != 0) {
    fprintf(stderr, "kind string coercion\n");
    if (kind) {
      freeusd_string_free(kind);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 20;
  }
  freeusd_string_free(kind);

  double vx = 0.0, vy = 0.0, vz = 0.0;
  if (freeusd_stage_read_field_vec3d(stage, "/Scene/Child", "extent", 1.0, &vx, &vy, &vz) != FREEUSD_OK ||
      !dbl_eq(vx, 1.0) || !dbl_eq(vy, 2.0) || !dbl_eq(vz, 3.0)) {
    fprintf(stderr, "extent vec3d\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 21;
  }

  float fx = 0.0f, fy = 0.0f, fz = 0.0f;
  if (freeusd_stage_read_field_vec3f(stage, "/Scene/Child", "displayColor", 1.0, &fx, &fy, &fz) != FREEUSD_OK ||
      !flt_eq(fx, 0.25f) || !flt_eq(fy, 0.5f) || !flt_eq(fz, 0.75f)) {
    fprintf(stderr, "displayColor vec3f\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 22;
  }

  if (freeusd_stage_read_field_vec3f(stage, "/Scene/Child", "extent", 1.0, &fx, &fy, &fz) != FREEUSD_OK ||
      !flt_eq(fx, 1.0f) || !flt_eq(fy, 2.0f) || !flt_eq(fz, 3.0f)) {
    fprintf(stderr, "extent vec3f coercion\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 23;
  }

  double row_major[16];
  memset(row_major, 0, sizeof(row_major));
  if (freeusd_stage_read_field_matrix4d(stage, "/Scene/Child", "xf", 1.0, row_major) != FREEUSD_OK ||
      !dbl_eq(row_major[0], 1.0) || !dbl_eq(row_major[5], 1.0) || !dbl_eq(row_major[10], 1.0) ||
      !dbl_eq(row_major[15], 1.0)) {
    fprintf(stderr, "xf matrix4d\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 24;
  }

  double qdr = 0.0, qdi = 0.0, qdj = 0.0, qdk = 0.0;
  if (freeusd_stage_read_field_quatd(stage, "/Scene/Child", "qd", 1.0, &qdr, &qdi, &qdj, &qdk) != FREEUSD_OK ||
      !dbl_eq(qdr, 1.0) || !dbl_eq(qdi, 0.0) || !dbl_eq(qdj, 0.0) || !dbl_eq(qdk, 0.0)) {
    fprintf(stderr, "qd quatd\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 25;
  }

  float qfr = 0.0f, qfi = 0.0f, qfj = 0.0f, qfk = 0.0f;
  if (freeusd_stage_read_field_quatf(stage, "/Scene/Child", "qf", 1.0, &qfr, &qfi, &qfj, &qfk) != FREEUSD_OK ||
      !flt_eq(qfr, 0.70710677f) || !flt_eq(qfk, 0.70710677f)) {
    fprintf(stderr, "qf quatf\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 26;
  }

  if (freeusd_stage_read_field_quatf(stage, "/Scene/Child", "qd", 1.0, &qfr, &qfi, &qfj, &qfk) != FREEUSD_OK ||
      !flt_eq(qfr, 1.0f) || !flt_eq(qfi, 0.0f) || !flt_eq(qfj, 0.0f) || !flt_eq(qfk, 0.0f)) {
    fprintf(stderr, "qd quatf coercion\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 27;
  }

  char* token = NULL;
  if (freeusd_stage_read_field_token(stage, "/Scene/Child", "kind", 1.0, &token) != FREEUSD_OK || !token ||
      strcmp(token, "component") != 0) {
    fprintf(stderr, "kind token\n");
    if (token) {
      freeusd_string_free(token);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 28;
  }
  freeusd_string_free(token);

  char** tags = NULL;
  size_t tag_count = 0;
  if (freeusd_stage_read_field_token_array(stage, "/Scene/Child", "tags", 1.0, &tags, &tag_count) != FREEUSD_OK ||
      tag_count != 2 || strcmp(tags[0], "a") != 0 || strcmp(tags[1], "b") != 0) {
    fprintf(stderr, "tags token array\n");
    if (tags) {
      freeusd_path_list_free(tags, tag_count);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 29;
  }
  freeusd_path_list_free(tags, tag_count);

  if (freeusd_stage_prim_is_valid(stage, "/Scene/Missing") != 0) {
    fprintf(stderr, "missing prim unexpectedly valid\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 30;
  }

  {
    double sentinel = 123.0;
    if (freeusd_stage_read_field_double(stage, "/Scene/Missing", "mass", 1.0, &sentinel) != FREEUSD_ERR_NOT_FOUND ||
        !dbl_eq(sentinel, 123.0)) {
      fprintf(stderr, "missing prim double contract\n");
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 31;
    }
  }

  {
    double sentinel = 456.0;
    if (freeusd_stage_read_field_double(stage, "/Scene/Child", "missingAttr", 1.0, &sentinel) != FREEUSD_ERR_NOT_FOUND ||
        !dbl_eq(sentinel, 456.0)) {
      fprintf(stderr, "missing attr double contract\n");
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 32;
    }
  }

  {
    double sx = 11.0, sy = 12.0, sz = 13.0;
    if (freeusd_stage_read_field_vec3d(stage, "/Scene/Child", "displayColor", 1.0, &sx, &sy, &sz) !=
            FREEUSD_ERR_NOT_FOUND ||
        !dbl_eq(sx, 11.0) || !dbl_eq(sy, 12.0) || !dbl_eq(sz, 13.0)) {
      fprintf(stderr, "wrong-type vec3d contract\n");
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 33;
    }
  }

  {
    double sr = 21.0, si = 22.0, sj = 23.0, sk = 24.0;
    if (freeusd_stage_read_field_quatd(stage, "/Scene/Child", "qf", 1.0, &sr, &si, &sj, &sk) !=
            FREEUSD_ERR_NOT_FOUND ||
        !dbl_eq(sr, 21.0) || !dbl_eq(si, 22.0) || !dbl_eq(sj, 23.0) || !dbl_eq(sk, 24.0)) {
      fprintf(stderr, "wrong-type quatd contract\n");
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 34;
    }
  }

  {
    char* bad = NULL;
    if (freeusd_stage_read_field_token(stage, "/Scene/Child", "label", 1.0, &bad) != FREEUSD_ERR_NOT_FOUND ||
        bad != NULL) {
      fprintf(stderr, "wrong-type token contract\n");
      if (bad) {
        freeusd_string_free(bad);
      }
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 35;
    }
  }

  {
    char** bad = NULL;
    size_t bad_count = 999;
    if (freeusd_stage_read_field_token_array(stage, "/Scene/Child", "kind", 1.0, &bad, &bad_count) !=
            FREEUSD_ERR_NOT_FOUND ||
        bad != NULL || bad_count != 0) {
      fprintf(stderr, "wrong-type token array contract\n");
      if (bad) {
        freeusd_path_list_free(bad, bad_count);
      }
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 36;
    }
  }

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
