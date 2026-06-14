#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FREEUSD_TEST_FIXTURES_DIR
#error "FREEUSD_TEST_FIXTURES_DIR must be set by CMake for c_abi_smoke"
#endif

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

static long c_smoke_pid(void) {
#ifdef _WIN32
  return (long)_getpid();
#else
  return (long)getpid();
#endif
}

static int list_has(const char* needle, char** arr, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (strcmp(arr[i], needle) == 0) {
      return 1;
    }
  }
  return 0;
}

int main(void) {
  const char* ver = freeusd_version_string();
  if (!ver || ver[0] == '\0') {
    fprintf(stderr, "missing version string\n");
    return 1;
  }

  const char* usdc_id = freeusd_usdc_crate_identifier_utf8();
  if (!usdc_id || strcmp(usdc_id, "PXR-USDC") != 0) {
    fprintf(stderr, "unexpected usdc crate identifier\n");
    return 1;
  }

  {
    const char* fixture_usda = FREEUSD_TEST_FIXTURES_DIR "/stage_open_root.usda";
    int kind = -1;
    char* detail = NULL;
    if (freeusd_detect_usd_file_kind_from_path_utf8(fixture_usda, &kind, &detail) != FREEUSD_OK) {
      fprintf(stderr, "detect_usd_file_kind fixture usda failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (detail) {
      freeusd_string_free(detail);
      detail = NULL;
    }
    if (kind != FREEUSD_USD_FILE_USDA_ASCII) {
      fprintf(stderr, "expected USDA kind for fixture, got %d\n", kind);
      return 1;
    }
  }

  {
    char crate_path[256];
    snprintf(crate_path, sizeof crate_path, "fusd_c_smoke_usdc_%ld.bin", c_smoke_pid());
    FILE* wf = fopen(crate_path, "wb");
    if (!wf) {
      fprintf(stderr, "fopen crate temp failed\n");
      return 1;
    }
    if (fwrite(usdc_id, 1, strlen(usdc_id), wf) != strlen(usdc_id) || fputc(0, wf) != 0) {
      fprintf(stderr, "write crate temp failed\n");
      fclose(wf);
      remove(crate_path);
      return 1;
    }
    fclose(wf);
    int kind = -1;
    char* detail = NULL;
    if (freeusd_detect_usd_file_kind_from_path_utf8(crate_path, &kind, &detail) != FREEUSD_OK) {
      fprintf(stderr, "detect_usd_file_kind crate temp failed: %s\n", freeusd_last_error_message());
      remove(crate_path);
      return 1;
    }
    if (detail) {
      freeusd_string_free(detail);
    }
    remove(crate_path);
    if (kind != FREEUSD_USD_FILE_USDC_CRATE) {
      fprintf(stderr, "expected USDC crate kind, got %d\n", kind);
      return 1;
    }
  }

  {
    int kind = 0;
    if (freeusd_detect_usd_file_kind_from_path_utf8(NULL, &kind, NULL) != FREEUSD_ERR_INVALID_ARGUMENT) {
      fprintf(stderr, "expected INVALID_ARGUMENT for null path\n");
      return 1;
    }
  }

  {
    char boot_path[256];
    snprintf(boot_path, sizeof boot_path, "fusd_c_smoke_boot_%ld.usdc", c_smoke_pid());
    FILE* wf = fopen(boot_path, "wb");
    if (!wf) {
      fprintf(stderr, "fopen bootstrap temp failed\n");
      return 1;
    }
    unsigned char buf[128];
    memset(buf, 0, sizeof buf);
    memcpy(buf, usdc_id, strlen(usdc_id));
    buf[8] = 0;
    buf[9] = 8;
    buf[10] = 0;
    buf[16] = 88;
    buf[17] = 0;
    buf[18] = 0;
    buf[19] = 0;
    buf[20] = 0;
    buf[21] = 0;
    buf[22] = 0;
    buf[23] = 0;
    if (fwrite(buf, 1, sizeof buf, wf) != sizeof buf) {
      fprintf(stderr, "write bootstrap temp failed\n");
      fclose(wf);
      remove(boot_path);
      return 1;
    }
    fclose(wf);
    FreeusdUsdcBootstrap boot;
    memset(&boot, 0, sizeof boot);
    if (freeusd_read_usdc_bootstrap_from_path_utf8(boot_path, &boot) != FREEUSD_OK) {
      fprintf(stderr, "read_usdc_bootstrap failed: %s\n", freeusd_last_error_message());
      remove(boot_path);
      return 1;
    }
    remove(boot_path);
    if (boot.file_version_major != 0 || boot.file_version_minor != 8 || boot.file_version_patch != 0 ||
        boot.toc_byte_offset != 88) {
      fprintf(stderr, "unexpected bootstrap fields\n");
      return 1;
    }
  }

  {
    char toc_path[256];
    snprintf(toc_path, sizeof toc_path, "fusd_c_smoke_toc_%ld.usdc", c_smoke_pid());
    FILE* wf = fopen(toc_path, "wb");
    if (!wf) {
      fprintf(stderr, "fopen toc temp failed\n");
      return 1;
    }
    unsigned char buf[160];
    memset(buf, 0, sizeof buf);
    memcpy(buf, usdc_id, strlen(usdc_id));
    buf[8] = 0;
    buf[9] = 8;
    buf[10] = 0;
    buf[16] = 88;
    buf[17] = 0;
    buf[18] = 0;
    buf[19] = 0;
    buf[20] = 0;
    buf[21] = 0;
    buf[22] = 0;
    buf[23] = 0;
    buf[88] = 2;
    memcpy(buf + 96, "TOKENS", 7);
    memcpy(buf + 128, "PATHS", 6);
    buf[128 + 16] = 120;
    memset(buf + 128 + 17, 0, 7);
    buf[128 + 24] = 40;
    memset(buf + 128 + 25, 0, 7);
    if (fwrite(buf, 1, sizeof buf, wf) != sizeof buf) {
      fprintf(stderr, "write toc temp failed\n");
      fclose(wf);
      remove(toc_path);
      return 1;
    }
    fclose(wf);
    uint64_t total = 0;
    uint64_t ret = 0;
    FreeusdUsdcTocSection* secs = NULL;
    if (freeusd_read_usdc_toc_from_path_utf8(toc_path, 16, &total, &secs, &ret) != FREEUSD_OK) {
      fprintf(stderr, "read_usdc_toc failed: %s\n", freeusd_last_error_message());
      remove(toc_path);
      return 1;
    }
    remove(toc_path);
    if (total != 2 || ret != 2 || !secs) {
      fprintf(stderr, "unexpected toc counts\n");
      freeusd_usdc_toc_sections_free(secs);
      return 1;
    }
    if (strncmp(secs[0].name, "TOKENS", 16) != 0 || secs[0].start_byte_offset != 0 || secs[0].size_bytes != 0) {
      fprintf(stderr, "unexpected toc section 0\n");
      freeusd_usdc_toc_sections_free(secs);
      return 1;
    }
    if (strncmp(secs[1].name, "PATHS", 16) != 0 || secs[1].start_byte_offset != 120 || secs[1].size_bytes != 40) {
      fprintf(stderr, "unexpected toc section 1\n");
      freeusd_usdc_toc_sections_free(secs);
      return 1;
    }
    freeusd_usdc_toc_sections_free(secs);
  }

  {
    char sec_path[256];
    snprintf(sec_path, sizeof sec_path, "fusd_c_smoke_sec_%ld.usdc", c_smoke_pid());
    FILE* wf = fopen(sec_path, "wb");
    if (!wf) {
      fprintf(stderr, "fopen section temp failed\n");
      return 1;
    }
    unsigned char buf[160];
    memset(buf, 0, sizeof buf);
    memcpy(buf, usdc_id, strlen(usdc_id));
    buf[8] = 0;
    buf[9] = 8;
    buf[10] = 0;
    buf[16] = 88;
    buf[88] = 1;
    memcpy(buf + 96, "TOKENS", 7);
    buf[96 + 16] = 128;
    buf[96 + 24] = 5;
    memcpy(buf + 128, "alpha", 5);
    if (fwrite(buf, 1, sizeof buf, wf) != sizeof buf) {
      fprintf(stderr, "write section temp failed\n");
      fclose(wf);
      remove(sec_path);
      return 1;
    }
    fclose(wf);
    uint8_t* bytes = NULL;
    uint64_t nbytes = 0;
    if (freeusd_read_usdc_section_bytes_from_path_utf8(sec_path, "TOKENS", 64, &bytes, &nbytes) != FREEUSD_OK) {
      fprintf(stderr, "read_usdc_section_bytes failed: %s\n", freeusd_last_error_message());
      remove(sec_path);
      return 1;
    }
    if (!bytes || nbytes != 5 || bytes[0] != 'a' || bytes[4] != 'a') {
      fprintf(stderr, "unexpected section payload\n");
      freeusd_bytes_free(bytes);
      remove(sec_path);
      return 1;
    }
    freeusd_bytes_free(bytes);
    if (freeusd_read_usdc_section_bytes_from_path_utf8(sec_path, "MISSING", 64, &bytes, &nbytes) != FREEUSD_ERR_NOT_FOUND) {
      fprintf(stderr, "expected NOT_FOUND for missing section\n");
      remove(sec_path);
      return 1;
    }
    remove(sec_path);
  }

  {
    char tables_path[4096];
    if (snprintf(tables_path, sizeof tables_path, "%s/parity_tables.usdc", FREEUSD_TEST_FIXTURES_DIR) >=
        (int)sizeof tables_path) {
      fprintf(stderr, "tables fixture path too long\n");
      return 1;
    }
    char** items = NULL;
    size_t count = 0;
    if (freeusd_read_usdc_token_table_from_path_utf8(tables_path, 16, 1024, &items, &count) != FREEUSD_OK) {
      fprintf(stderr, "read token table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!items || count != 2u || strcmp(items[0], "render") != 0 || strcmp(items[1], "invisible") != 0) {
      fprintf(stderr, "unexpected token table\n");
      freeusd_path_list_free(items, count);
      return 1;
    }
    freeusd_path_list_free(items, count);
    items = NULL;
    count = 0;
    if (freeusd_read_usdc_string_table_from_path_utf8(tables_path, 16, 1024, &items, &count) != FREEUSD_OK) {
      fprintf(stderr, "read string table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!items || count != 2u || strcmp(items[0], "hello") != 0 || strcmp(items[1], "world") != 0) {
      fprintf(stderr, "unexpected string table\n");
      freeusd_path_list_free(items, count);
      return 1;
    }
    freeusd_path_list_free(items, count);
    items = NULL;
    count = 0;
    if (freeusd_read_usdc_path_table_from_path_utf8(tables_path, 16, 1024, &items, &count) != FREEUSD_OK) {
      fprintf(stderr, "read path table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!items || count != 2u || strcmp(items[0], "/World") != 0 || strcmp(items[1], "/World/Cube") != 0) {
      fprintf(stderr, "unexpected path table\n");
      freeusd_path_list_free(items, count);
      return 1;
    }
    freeusd_path_list_free(items, count);
    FreeusdUsdcFieldEntry* field_entries = NULL;
    count = 0;
    if (freeusd_read_usdc_fields_table_from_path_utf8(tables_path, 16, 1024, &field_entries, &count) != FREEUSD_OK) {
      fprintf(stderr, "read fields table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!field_entries || count != 2u || field_entries[0].token_index != 0u ||
        field_entries[0].value_type_token_index != 1u || field_entries[1].token_index != 1u ||
        field_entries[1].value_type_token_index != 0u) {
      fprintf(stderr, "unexpected fields table\n");
      freeusd_usdc_fields_entries_free(field_entries);
      return 1;
    }
    freeusd_usdc_fields_entries_free(field_entries);
    FreeusdUsdcSpecEntry* spec_entries = NULL;
    count = 0;
    if (freeusd_read_usdc_specs_table_from_path_utf8(tables_path, 16, 1024, &spec_entries, &count) != FREEUSD_OK) {
      fprintf(stderr, "read specs table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!spec_entries || count != 2u || spec_entries[0].path_index != 0u || spec_entries[0].field_set_index != 0u ||
        spec_entries[0].spec_type != 1u || spec_entries[1].path_index != 1u || spec_entries[1].field_set_index != 1u ||
        spec_entries[1].spec_type != 2u) {
      fprintf(stderr, "unexpected specs table\n");
      freeusd_usdc_specs_entries_free(spec_entries);
      return 1;
    }
    freeusd_usdc_specs_entries_free(spec_entries);
    FreeusdUsdcFieldSet* fieldsets = NULL;
    count = 0;
    if (freeusd_read_usdc_fieldsets_table_from_path_utf8(tables_path, 8, 16, 1024, &fieldsets, &count) != FREEUSD_OK) {
      fprintf(stderr, "read fieldsets table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!fieldsets || count != 2u || fieldsets[0].field_count != 2u || fieldsets[0].field_indices[0] != 0u ||
        fieldsets[0].field_indices[1] != 1u || fieldsets[1].field_count != 1u || fieldsets[1].field_indices[0] != 1u) {
      fprintf(stderr, "unexpected fieldsets table\n");
      freeusd_usdc_fieldsets_free(fieldsets, count);
      return 1;
    }
    freeusd_usdc_fieldsets_free(fieldsets, count);
    FreeusdUsdcValueBlob* value_blobs = NULL;
    count = 0;
    if (freeusd_read_usdc_values_table_from_path_utf8(tables_path, 19, 1024, &value_blobs, &count) != FREEUSD_OK) {
      fprintf(stderr, "read values table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!value_blobs || count != 19u || value_blobs[0].byte_count != 4u) {
      fprintf(stderr, "unexpected values table\n");
      freeusd_usdc_values_blobs_free(value_blobs, count);
      return 1;
    }
    freeusd_usdc_values_blobs_free(value_blobs, count);
    FreeusdUsdcTypedValue* typed_values = NULL;
    count = 0;
    if (freeusd_read_usdc_typed_values_table_from_path_utf8(tables_path, 19, 1024, &typed_values, &count) !=
        FREEUSD_OK) {
      fprintf(stderr, "read typed values table failed: %s\n", freeusd_last_error_message());
      return 1;
    }
    if (!typed_values || count != 19u || typed_values[0].kind != FREEUSD_USDC_VALUE_INT32 ||
        typed_values[0].int32_value != 42 || typed_values[1].kind != FREEUSD_USDC_VALUE_FLOAT ||
        typed_values[2].kind != FREEUSD_USDC_VALUE_TOKEN_INDEX || typed_values[2].token_index != 0u ||
        typed_values[3].kind != FREEUSD_USDC_VALUE_BOOL || !typed_values[3].bool_value ||
        typed_values[4].kind != FREEUSD_USDC_VALUE_DOUBLE || typed_values[4].double_value < 3.24 ||
        typed_values[4].double_value > 3.26 || typed_values[5].kind != FREEUSD_USDC_VALUE_INT64 ||
        typed_values[5].int64_value != -9007199254740991LL || typed_values[6].kind != FREEUSD_USDC_VALUE_STRING_UTF8 ||
        !typed_values[6].string_utf8 || strcmp(typed_values[6].string_utf8, "parity") != 0 ||
        typed_values[7].kind != FREEUSD_USDC_VALUE_VEC3F ||
        typed_values[7].vec3f_value[0] < 0.99f || typed_values[7].vec3f_value[2] > 3.01f ||
        typed_values[8].kind != FREEUSD_USDC_VALUE_STRING_INDEX || typed_values[8].string_index != 1u ||
        typed_values[9].kind != FREEUSD_USDC_VALUE_VEC3D || typed_values[9].vec3d_value[0] < 3.99 ||
        typed_values[9].vec3d_value[2] > 6.01 || typed_values[10].kind != FREEUSD_USDC_VALUE_INT32_ARRAY ||
        typed_values[10].int32_array_count != 3u || typed_values[10].int32_array[0] != 7 ||
        typed_values[10].int32_array[2] != 9 ||
        typed_values[11].kind != FREEUSD_USDC_VALUE_FLOAT_ARRAY || typed_values[11].float_array_count != 2u ||
        typed_values[11].float_array[0] < 0.24f || typed_values[11].float_array[1] > 0.76f ||
        typed_values[12].kind != FREEUSD_USDC_VALUE_DOUBLE_ARRAY || typed_values[12].double_array_count != 2u ||
        typed_values[13].kind != FREEUSD_USDC_VALUE_VEC2F || typed_values[13].vec2f_value[0] < 0.49f ||
        typed_values[14].kind != FREEUSD_USDC_VALUE_VEC4F || typed_values[14].vec4f_value[0] < 0.99f ||
        typed_values[14].vec4f_value[3] > 4.01f || typed_values[15].kind != FREEUSD_USDC_VALUE_VEC2D ||
        typed_values[15].vec2d_value[0] < 0.49 || typed_values[15].vec2d_value[1] < 1.74 ||
        typed_values[16].kind != FREEUSD_USDC_VALUE_QUATF || typed_values[16].quatf_value[0] < 0.99f ||
        typed_values[16].quatf_value[1] < 0.49f || typed_values[16].quatf_value[3] < 0.12f ||
        typed_values[17].kind != FREEUSD_USDC_VALUE_QUATD || typed_values[17].quatd_value[0] < 0.99 ||
        typed_values[17].quatd_value[3] < 0.12 ||
        typed_values[18].kind != FREEUSD_USDC_VALUE_TOKEN_INDEX_ARRAY ||
        typed_values[18].token_index_array_count != 2u || typed_values[18].token_index_array[0] != 0u ||
        typed_values[18].token_index_array[1] != 1u) {
      fprintf(stderr, "unexpected typed values table\n");
      freeusd_usdc_typed_values_free(typed_values, count);
      return 1;
    }
    freeusd_usdc_typed_values_free(typed_values, count);
  }

  {
    char embedded_path[512];
    if (snprintf(embedded_path, sizeof embedded_path, "%s/parity_embedded_scene.usdc",
                 FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof embedded_path) {
      fprintf(stderr, "embedded path too long\n");
      return 1;
    }
    char* usda_text = NULL;
    if (freeusd_read_usdc_usda_section_from_path_utf8(embedded_path, 1024 * 1024, &usda_text) != FREEUSD_OK ||
        !usda_text || strstr(usda_text, "defaultPrim = \"World\"") == NULL) {
      fprintf(stderr, "read usda section failed: %s\n", freeusd_last_error_message());
      free(usda_text);
      return 1;
    }
    free(usda_text);
  }

  {
    char embedded_lz4_path[512];
    if (snprintf(embedded_lz4_path, sizeof embedded_lz4_path, "%s/parity_embedded_scene_lz4.usdc",
                 FREEUSD_TEST_FIXTURES_DIR) >= (int)sizeof embedded_lz4_path) {
      fprintf(stderr, "embedded lz4 path too long\n");
      return 1;
    }
    char* usda_lz4 = NULL;
    if (freeusd_read_usdc_usda_section_from_path_utf8(embedded_lz4_path, 1024 * 1024, &usda_lz4) != FREEUSD_OK ||
        !usda_lz4 || strstr(usda_lz4, "defaultPrim = \"World\"") == NULL) {
      fprintf(stderr, "read lz4 usda section failed: %s\n", freeusd_last_error_message());
      free(usda_lz4);
      return 1;
    }
    free(usda_lz4);
  }

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_smoke");
  if (!layer) {
    fprintf(stderr, "layer_new failed: %s\n", freeusd_last_error_message());
    return 2;
  }

  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      "    doc = \"c_abi_smoke_layer\"\n"
      "    defaultPrim = \"W\"\n"
      ")\n"
      "def Xform \"W\"\n"
      "{\n"
      "    def \"Pipe2\"\n"
      "    {\n"
      "        double outv = 9.0\n"
      "    }\n"
      "    def \"ConnTest\"\n"
      "    {\n"
      "        double q.connect = </W/Pipe2.outv>\n"
      "    }\n"
      "    def \"Hidden\"\n"
      "    (\n"
      "        active = false\n"
      "    )\n"
      "    {\n"
      "    }\n"
      "    def \"C\"\n"
      "    (\n"
      "        customData = {\n"
      "            string tag = \"cd\",\n"
      "            int rank = 7,\n"
      "        }\n"
      "    )\n"
      "    {\n"
      "        double x = 3.0\n"
      "        double x.timeSamples = {\n"
      "            1: 3.0,\n"
      "            7: 3.5,\n"
      "        }\n"
      "        double3 extent = (1, 2, 3)\n"
      "        color3f displayColor = (0.25, 0.5, 0.75)\n"
      "        float radius = 2.5\n"
      "        bool flag = true\n"
      "        int n = 42\n"
      "        string label = \"hi\"\n"
      "        rel link = </W>\n"
      "    }\n"
      "    def \"GFTypes\"\n"
      "    {\n"
      "        matrix4d m = ((1,0,0,0), (0,1,0,0), (0,0,1,0), (0,0,0,1))\n"
      "        quatd qd = (1, 0, 0, 0)\n"
      "        quatf qf = (0.70710677, 0, 0, 0.70710677)\n"
      "        token kind = component\n"
      "        token[] tags = [@a@, @b@]\n"
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

  char* lid = freeusd_layer_get_identifier_utf8(layer);
  if (!lid || strstr(lid, "c_smoke") == NULL) {
    fprintf(stderr, "layer_get_identifier\n");
    if (lid) {
      freeusd_string_free(lid);
    }
    freeusd_layer_free(layer);
    return 72;
  }
  freeusd_string_free(lid);

  char** lpaths = NULL;
  size_t nlp = 0;
  if (freeusd_layer_list_prim_paths(layer, &lpaths, &nlp) != FREEUSD_OK || !list_has("/W/C", lpaths, nlp)) {
    fprintf(stderr, "layer_list_prim_paths\n");
    if (lpaths) {
      freeusd_path_list_free(lpaths, nlp);
    }
    freeusd_layer_free(layer);
    return 73;
  }
  const size_t n_layer_prim_paths = nlp;
  freeusd_path_list_free(lpaths, nlp);

  char* ldoc = freeusd_layer_get_documentation_utf8(layer);
  if (!ldoc || strstr(ldoc, "c_abi_smoke_layer") == NULL) {
    fprintf(stderr, "layer_get_documentation\n");
    if (ldoc) {
      freeusd_string_free(ldoc);
    }
    freeusd_layer_free(layer);
    return 78;
  }
  freeusd_string_free(ldoc);

  char** subs = NULL;
  size_t nsub = 0;
  if (freeusd_layer_list_sub_layers(layer, &subs, &nsub) != FREEUSD_OK || nsub != 0) {
    fprintf(stderr, "layer_list_sub_layers\n");
    if (subs) {
      freeusd_path_list_free(subs, nsub);
    }
    freeusd_layer_free(layer);
    return 86;
  }
  freeusd_path_list_free(subs, nsub);

  if (freeusd_layer_has_default_prim(layer) != 1) {
    fprintf(stderr, "layer_has_default_prim\n");
    freeusd_layer_free(layer);
    return 79;
  }
  char* dpl = NULL;
  if (freeusd_layer_get_default_prim_utf8(layer, &dpl) != FREEUSD_OK || strcmp(dpl, "W") != 0) {
    fprintf(stderr, "layer_get_default_prim\n");
    if (dpl) {
      freeusd_string_free(dpl);
    }
    freeusd_layer_free(layer);
    return 80;
  }
  freeusd_string_free(dpl);

  FreeusdStage* stage = freeusd_stage_attach_root_layer(layer);
  if (!stage) {
    fprintf(stderr, "stage failed: %s\n", freeusd_last_error_message());
    freeusd_layer_free(layer);
    return 5;
  }

  char* proot = freeusd_stage_get_pseudo_root_path_utf8(stage);
  if (!proot || strcmp(proot, "/") != 0) {
    fprintf(stderr, "pseudo_root_path\n");
    if (proot) {
      freeusd_string_free(proot);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 66;
  }
  freeusd_string_free(proot);

  if (freeusd_stage_has_default_prim(stage) != 1) {
    fprintf(stderr, "stage_has_default_prim\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 81;
  }
  char* dps = NULL;
  if (freeusd_stage_get_default_prim_utf8(stage, &dps) != FREEUSD_OK || strcmp(dps, "W") != 0) {
    fprintf(stderr, "stage_get_default_prim\n");
    if (dps) {
      freeusd_string_free(dps);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 82;
  }
  freeusd_string_free(dps);

  char** allp = NULL;
  size_t nap = 0;
  if (freeusd_stage_list_composed_prim_paths(stage, &allp, &nap) != FREEUSD_OK || nap != n_layer_prim_paths) {
    fprintf(stderr, "stage_list_composed_prim_paths count %zu vs layer %zu\n", nap, n_layer_prim_paths);
    if (allp) {
      freeusd_path_list_free(allp, nap);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 83;
  }
  freeusd_path_list_free(allp, nap);

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

  float rf = 0.0f;
  if (freeusd_stage_read_field_float(stage, "/W/C", "radius", 1.0, &rf) != FREEUSD_OK ||
      fabsf(rf - 2.5f) > 1e-5f) {
    fprintf(stderr, "read_field_float radius: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 93;
  }
  rf = 0.0f;
  if (freeusd_stage_read_field_float(stage, "/W/C", "x", 1.0, &rf) != FREEUSD_OK || fabsf(rf - 3.0f) > 1e-5f) {
    fprintf(stderr, "read_field_float from double x\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 94;
  }

  double* stimes = NULL;
  size_t nst = 0;
  if (freeusd_stage_list_field_sample_times(stage, "/W/C", "x", &stimes, &nst) != FREEUSD_OK || nst != 2 ||
      fabs(stimes[0] - 1.0) > 1e-9 || fabs(stimes[1] - 7.0) > 1e-9) {
    fprintf(stderr, "list_field_sample_times x\n");
    if (stimes) {
      freeusd_double_array_free(stimes);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 87;
  }
  freeusd_double_array_free(stimes);

  d = 0.0;
  if (freeusd_stage_read_field_double(stage, "/W/C", "x", 7.0, &d) != FREEUSD_OK || fabs(d - 3.5) > 1e-9) {
    fprintf(stderr, "read_field_double at sample 7\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 88;
  }

  if (freeusd_stage_has_field_opinion(stage, "/W/C", "x") != 1 ||
      freeusd_stage_has_field_opinion(stage, "/W/C", "nosuchattr") != 0) {
    fprintf(stderr, "has_field_opinion mismatch\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 58;
  }

  if (freeusd_stage_has_attribute_connection(stage, "/W/ConnTest", "q") != 1) {
    fprintf(stderr, "has_attribute_connection\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 67;
  }
  char* ctgt = NULL;
  if (freeusd_stage_get_attribute_connection_target(stage, "/W/ConnTest", "q", &ctgt) != FREEUSD_OK ||
      strcmp(ctgt, "/W/Pipe2.outv") != 0) {
    fprintf(stderr, "get_attribute_connection_target: %s\n", freeusd_last_error_message());
    if (ctgt) {
      freeusd_string_free(ctgt);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 68;
  }
  freeusd_string_free(ctgt);

  d = 0.0;
  if (freeusd_stage_read_field_double(stage, "/W/ConnTest", "q", 1.0, &d) != FREEUSD_OK || fabs(d - 9.0) > 1e-9) {
    fprintf(stderr, "read_field_double via connection\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 69;
  }

  if (freeusd_stage_resolve_has_prim_active_opinion(stage, "/W/C") != 0 ||
      freeusd_stage_resolve_has_prim_active_opinion(stage, "/W/Hidden") != 1) {
    fprintf(stderr, "resolve_has_prim_active_opinion\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 70;
  }
  int hid = 1;
  if (freeusd_stage_resolve_prim_active(stage, "/W/Hidden", &hid) != FREEUSD_OK || hid != 0) {
    fprintf(stderr, "resolve_prim_active Hidden\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 71;
  }

  int bflag = 0;
  if (freeusd_stage_read_field_bool(stage, "/W/C", "flag", 1.0, &bflag) != FREEUSD_OK || bflag != 1) {
    fprintf(stderr, "read_field_bool: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 59;
  }

  int64_t ni = 0;
  if (freeusd_stage_read_field_int64(stage, "/W/C", "n", 1.0, &ni) != FREEUSD_OK || ni != 42) {
    fprintf(stderr, "read_field_int64: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 60;
  }

  if (freeusd_stage_has_relationship(stage, "/W/C", "link") != 1) {
    fprintf(stderr, "has_relationship expected 1\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 61;
  }

  char** rels = NULL;
  size_t nrel = 0;
  if (freeusd_stage_list_relationship_targets(stage, "/W/C", "link", &rels, &nrel) != FREEUSD_OK || nrel != 1 ||
      strcmp(rels[0], "/W") != 0) {
    fprintf(stderr, "list_relationship_targets\n");
    if (rels) {
      freeusd_path_list_free(rels, nrel);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 62;
  }
  freeusd_path_list_free(rels, nrel);

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

  if (freeusd_stage_prim_custom_data_key_in_any_layer(stage, "/W/C", "tag") != 1 ||
      freeusd_stage_prim_custom_data_key_in_any_layer(stage, "/W/C", "nosuchkey") != 0) {
    fprintf(stderr, "prim_custom_data_key_in_any_layer\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 63;
  }

  char** ckeys = NULL;
  size_t nck = 0;
  if (freeusd_stage_list_composed_prim_custom_data_keys(stage, "/W/C", &ckeys, &nck) != FREEUSD_OK || nck != 2 ||
      strcmp(ckeys[0], "rank") != 0 || strcmp(ckeys[1], "tag") != 0) {
    fprintf(stderr, "list_composed_prim_custom_data_keys\n");
    if (ckeys) {
      freeusd_path_list_free(ckeys, nck);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 64;
  }
  freeusd_path_list_free(ckeys, nck);

  double vx = 0, vy = 0, vz = 0;
  if (freeusd_stage_read_field_vec3d(stage, "/W/C", "extent", 1.0, &vx, &vy, &vz) != FREEUSD_OK ||
      fabs(vx - 1.0) > 1e-9 || fabs(vy - 2.0) > 1e-9 || fabs(vz - 3.0) > 1e-9) {
    fprintf(stderr, "read_field_vec3d: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 65;
  }

  float fx = 0, fy = 0, fz = 0;
  if (freeusd_stage_read_field_vec3f(stage, "/W/C", "displayColor", 1.0, &fx, &fy, &fz) != FREEUSD_OK ||
      fabsf(fx - 0.25f) > 1e-5f || fabsf(fy - 0.5f) > 1e-5f || fabsf(fz - 0.75f) > 1e-5f) {
    fprintf(stderr, "read_field_vec3f: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 91;
  }
  if (freeusd_stage_read_field_vec3f(stage, "/W/C", "extent", 1.0, &fx, &fy, &fz) != FREEUSD_OK ||
      fabsf(fx - 1.0f) > 1e-5f || fabsf(fy - 2.0f) > 1e-5f || fabsf(fz - 3.0f) > 1e-5f) {
    fprintf(stderr, "read_field_vec3f extent from double3: %s\n", freeusd_last_error_message());
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 92;
  }

  {
    double m16[16];
    if (freeusd_stage_read_field_matrix4d(stage, "/W/GFTypes", "m", 1.0, m16) != FREEUSD_OK) {
      fprintf(stderr, "read_field_matrix4d: %s\n", freeusd_last_error_message());
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 95;
    }
    for (int i = 0; i < 16; ++i) {
      const double want = (i == 0 || i == 5 || i == 10 || i == 15) ? 1.0 : 0.0;
      if (fabs(m16[i] - want) > 1e-9) {
        fprintf(stderr, "read_field_matrix4d[%d] got %g want %g\n", i, m16[i], want);
        freeusd_stage_free(stage);
        freeusd_layer_free(layer);
        return 96;
      }
    }
    double qr = 0, qi = 0, qj = 0, qk = 0;
    if (freeusd_stage_read_field_quatd(stage, "/W/GFTypes", "qd", 1.0, &qr, &qi, &qj, &qk) != FREEUSD_OK ||
        fabs(qr - 1.0) > 1e-9 || fabs(qi) > 1e-9 || fabs(qj) > 1e-9 || fabs(qk) > 1e-9) {
      fprintf(stderr, "read_field_quatd: %s\n", freeusd_last_error_message());
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 97;
    }
    float fr = 0, fi = 0, fj = 0, fk = 0;
    if (freeusd_stage_read_field_quatf(stage, "/W/GFTypes", "qf", 1.0, &fr, &fi, &fj, &fk) != FREEUSD_OK ||
        fabsf(fr - 0.70710677f) > 1e-5f || fabsf(fi) > 1e-5f || fabsf(fj) > 1e-5f ||
        fabsf(fk - 0.70710677f) > 1e-5f) {
      fprintf(stderr, "read_field_quatf: %s\n", freeusd_last_error_message());
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 98;
    }
    char* tok = NULL;
    if (freeusd_stage_read_field_token(stage, "/W/GFTypes", "kind", 1.0, &tok) != FREEUSD_OK ||
        strcmp(tok, "component") != 0) {
      fprintf(stderr, "read_field_token: %s\n", freeusd_last_error_message());
      if (tok) {
        freeusd_string_free(tok);
      }
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 99;
    }
    freeusd_string_free(tok);
    char** tags = NULL;
    size_t nt = 0;
    if (freeusd_stage_read_field_token_array(stage, "/W/GFTypes", "tags", 1.0, &tags, &nt) != FREEUSD_OK ||
        nt != 2 || strcmp(tags[0], "a") != 0 || strcmp(tags[1], "b") != 0) {
      fprintf(stderr, "read_field_token_array: %s\n", freeusd_last_error_message());
      if (tags) {
        freeusd_path_list_free(tags, nt);
      }
      freeusd_stage_free(stage);
      freeusd_layer_free(layer);
      return 100;
    }
    freeusd_path_list_free(tags, nt);
  }

  char** fnames = NULL;
  size_t nfn = 0;
  if (freeusd_stage_list_composed_field_names(stage, "/W/C", &fnames, &nfn) != FREEUSD_OK || nfn != 7 ||
      strcmp(fnames[0], "displayColor") != 0 || strcmp(fnames[1], "extent") != 0 || strcmp(fnames[2], "flag") != 0 ||
      strcmp(fnames[3], "label") != 0 || strcmp(fnames[4], "n") != 0 || strcmp(fnames[5], "radius") != 0 ||
      strcmp(fnames[6], "x") != 0) {
    fprintf(stderr, "list_composed_field_names\n");
    if (fnames) {
      freeusd_path_list_free(fnames, nfn);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 74;
  }
  freeusd_path_list_free(fnames, nfn);

  char** rnames = NULL;
  size_t nrn = 0;
  if (freeusd_stage_list_composed_relationship_names(stage, "/W/C", &rnames, &nrn) != FREEUSD_OK || nrn != 1 ||
      strcmp(rnames[0], "link") != 0) {
    fprintf(stderr, "list_composed_relationship_names\n");
    if (rnames) {
      freeusd_path_list_free(rnames, nrn);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 75;
  }
  freeusd_path_list_free(rnames, nrn);

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
      "        double motion.timeSamples = {\n"
      "            0: 1.0,\n"
      "        }\n"
      "        double onlyS = 11.0\n"
      "        rel r = </S1>\n"
      "    }\n"
      "}\n"
      "def \"S1\" {}\n";
  const char usda_w[] =
      "#usda 1.0\n"
      "(\n)\n"
      "def \"R\"\n"
      "{\n"
      "    def \"X\"\n"
      "    {\n"
      "        double v = 99.0\n"
      "        double motion.timeSamples = {\n"
      "            3: 2.0,\n"
      "        }\n"
      "        double onlyW = 22.0\n"
      "        rel r = </S2>\n"
      "    }\n"
      "}\n"
      "def \"S2\" {}\n";
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

  char** r2 = NULL;
  size_t nr2 = 0;
  if (freeusd_stage_list_relationship_targets(st2, "/R/X", "r", &r2, &nr2) != FREEUSD_OK || nr2 != 2 ||
      strcmp(r2[0], "/S1") != 0 || strcmp(r2[1], "/S2") != 0) {
    fprintf(stderr, "stack rel targets expected /S1 /S2\n");
    if (r2) {
      freeusd_path_list_free(r2, nr2);
    }
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 18;
  }
  freeusd_path_list_free(r2, nr2);

  char** fstack = NULL;
  size_t nfs = 0;
  if (freeusd_stage_list_composed_field_names(st2, "/R/X", &fstack, &nfs) != FREEUSD_OK || nfs != 4 ||
      strcmp(fstack[0], "motion") != 0 || strcmp(fstack[1], "onlyS") != 0 || strcmp(fstack[2], "onlyW") != 0 ||
      strcmp(fstack[3], "v") != 0) {
    fprintf(stderr, "stack composed field names\n");
    if (fstack) {
      freeusd_path_list_free(fstack, nfs);
    }
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 76;
  }
  freeusd_path_list_free(fstack, nfs);

  double* vt = NULL;
  size_t nvt = 0;
  if (freeusd_stage_list_field_sample_times(st2, "/R/X", "motion", &vt, &nvt) != FREEUSD_OK || nvt != 2 ||
      fabs(vt[0] - 0.0) > 1e-9 || fabs(vt[1] - 3.0) > 1e-9) {
    fprintf(stderr, "stack list_field_sample_times motion\n");
    if (vt) {
      freeusd_double_array_free(vt);
    }
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 89;
  }
  freeusd_double_array_free(vt);

  char** rstack = NULL;
  size_t nrs = 0;
  if (freeusd_stage_list_composed_relationship_names(st2, "/R/X", &rstack, &nrs) != FREEUSD_OK || nrs != 1 ||
      strcmp(rstack[0], "r") != 0) {
    fprintf(stderr, "stack composed relationship names\n");
    if (rstack) {
      freeusd_path_list_free(rstack, nrs);
    }
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 77;
  }
  freeusd_path_list_free(rstack, nrs);

  if (freeusd_stage_has_default_prim(st2) != 0) {
    fprintf(stderr, "stack stage should have no defaultPrim\n");
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 84;
  }
  char** ap2 = NULL;
  size_t na2 = 0;
  if (freeusd_stage_list_composed_prim_paths(st2, &ap2, &na2) != FREEUSD_OK || na2 < 4 ||
      !list_has("/R", ap2, na2) || !list_has("/R/X", ap2, na2) || !list_has("/S1", ap2, na2) ||
      !list_has("/S2", ap2, na2)) {
    fprintf(stderr, "stack composed prim paths (count=%zu)\n", na2);
    if (ap2) {
      freeusd_path_list_free(ap2, na2);
    }
    freeusd_stage_free(st2);
    freeusd_layer_free(strong);
    freeusd_layer_free(weak);
    freeusd_layer_free(layer);
    return 85;
  }
  freeusd_path_list_free(ap2, na2);

  freeusd_stage_free(st2);
  freeusd_layer_free(strong);
  freeusd_layer_free(weak);

  freeusd_layer_free(layer);
  return 0;
}
