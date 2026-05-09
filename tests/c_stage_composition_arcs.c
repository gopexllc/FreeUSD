#include <freeusd/c/freeusd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  const char usda[] =
      "#usda 1.0\n"
      "(\n"
      ")\n"
      "def Xform \"Root\"\n"
      "(\n"
      "    prepend references = @./a.usda@</World>\n"
      "    prepend inherits = </Base>\n"
      "    prepend specializes = </Spec>\n"
      "    prepend payload = @./p.usdc@\n"
      ")\n"
      "{\n"
      "}\n";

  FreeusdLayer* layer = freeusd_layer_new_anonymous("c_comp_arcs");
  if (!layer) {
    fprintf(stderr, "layer_new\n");
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

  if (freeusd_stage_has_prim_references(stage, "/Root") != 1 || freeusd_stage_has_prim_inherits(stage, "/Root") != 1 ||
      freeusd_stage_has_prim_specializes(stage, "/Root") != 1 || freeusd_stage_has_prim_payloads(stage, "/Root") != 1) {
    fprintf(stderr, "has_* flags\n");
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 4;
  }

  char** refs = NULL;
  size_t nrefs = 0;
  if (freeusd_stage_list_prim_references(stage, "/Root", &refs, &nrefs) != FREEUSD_OK || nrefs != 1u ||
      strcmp(refs[0], "@./a.usda@</World>") != 0) {
    fprintf(stderr, "references list\n");
    if (refs) {
      freeusd_path_list_free(refs, nrefs);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 5;
  }
  freeusd_path_list_free(refs, nrefs);

  char** inh = NULL;
  size_t ninh = 0;
  if (freeusd_stage_list_prim_inherits(stage, "/Root", &inh, &ninh) != FREEUSD_OK || ninh != 1u ||
      strcmp(inh[0], "/Base") != 0) {
    fprintf(stderr, "inherits list\n");
    if (inh) {
      freeusd_path_list_free(inh, ninh);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 6;
  }
  freeusd_path_list_free(inh, ninh);

  char** sp = NULL;
  size_t nsp = 0;
  if (freeusd_stage_list_prim_specializes(stage, "/Root", &sp, &nsp) != FREEUSD_OK || nsp != 1u ||
      strcmp(sp[0], "/Spec") != 0) {
    fprintf(stderr, "specializes list\n");
    if (sp) {
      freeusd_path_list_free(sp, nsp);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 7;
  }
  freeusd_path_list_free(sp, nsp);

  char** pay = NULL;
  size_t npay = 0;
  if (freeusd_stage_list_prim_payloads(stage, "/Root", &pay, &npay) != FREEUSD_OK || npay != 1u ||
      strcmp(pay[0], "@./p.usdc@") != 0) {
    fprintf(stderr, "payload list got %s\n", npay ? pay[0] : "(null)");
    if (pay) {
      freeusd_path_list_free(pay, npay);
    }
    freeusd_stage_free(stage);
    freeusd_layer_free(layer);
    return 8;
  }
  freeusd_path_list_free(pay, npay);

  freeusd_stage_free(stage);
  freeusd_layer_free(layer);
  return 0;
}
