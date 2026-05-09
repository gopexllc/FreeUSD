#include <freeusd/c/freeusd.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
      "        bool flag = true\n"
      "        int n = 42\n"
      "        string label = \"hi\"\n"
      "        rel link = </W>\n"
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

  char** fnames = NULL;
  size_t nfn = 0;
  if (freeusd_stage_list_composed_field_names(stage, "/W/C", &fnames, &nfn) != FREEUSD_OK || nfn != 5 ||
      strcmp(fnames[0], "extent") != 0 || strcmp(fnames[1], "flag") != 0 || strcmp(fnames[2], "label") != 0 ||
      strcmp(fnames[3], "n") != 0 || strcmp(fnames[4], "x") != 0) {
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
