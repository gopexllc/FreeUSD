// Package freeusd wraps the stable FreeUSD C ABI (libfreeusd_c + static deps).
//
// Build: from the repository root, configure and build the C++ project first
// (so build/src/libfreeusd_*.a exist). Then, with CGO enabled:
//
//	CGO_CFLAGS="-I$PWD/include" \
//	CGO_LDFLAGS="-L$PWD/build/src -lfreeusd_c -lfreeusd_base -lfreeusd_io -lfreeusd_usd -lfreeusd_ar -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_plug -lstdc++ -lm" \
//	go test ./bindings/go/...
package freeusd

/*
#cgo CFLAGS: -I${SRCDIR}/../../include
#cgo linux LDFLAGS: -L${SRCDIR}/../../build/src -lfreeusd_c -lfreeusd_base -lfreeusd_io -lfreeusd_usd -lfreeusd_ar -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_plug -lstdc++ -lm
#cgo darwin LDFLAGS: -L${SRCDIR}/../../build/src -lfreeusd_c -lfreeusd_base -lfreeusd_io -lfreeusd_usd -lfreeusd_ar -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_plug -lc++ -lm
#include <stdlib.h>
#include <freeusd/c/freeusd.h>
*/
import "C"

import "unsafe"

// VersionString returns the FreeUSD library version (static storage in C; do not free).
func VersionString() string {
	return C.GoString(C.freeusd_version_string())
}

// LastErrorMessage returns the thread-local C API error string (valid until the next C call on this thread).
func LastErrorMessage() string {
	return C.GoString(C.freeusd_last_error_message())
}

// Layer wraps FreeusdLayer (USDA I/O and stage attachment via C API).
type Layer struct {
	ptr *C.FreeusdLayer
}

// NewAnonymousLayer creates an in-memory layer. identifier may be empty.
func NewAnonymousLayer(identifier string) *Layer {
	var cstr *C.char
	if identifier != "" {
		cstr = C.CString(identifier)
		defer C.free(unsafe.Pointer(cstr))
	}
	p := C.freeusd_layer_new_anonymous(cstr)
	if p == nil {
		return nil
	}
	return &Layer{ptr: p}
}

// Free releases the layer. Safe to call on nil.
func (l *Layer) Free() {
	if l == nil || l.ptr == nil {
		return
	}
	C.freeusd_layer_free(l.ptr)
	l.ptr = nil
}

// LoadUSDA replaces layer contents from a USDA string (0 = FREEUSD_OK).
func (l *Layer) LoadUSDA(text string) int {
	if l == nil || l.ptr == nil {
		return 1
	}
	cs := C.CString(text)
	defer C.free(unsafe.Pointer(cs))
	return int(C.freeusd_layer_load_usda_from_string(l.ptr, cs))
}

// Sublayer stacking policy for OpenStageFromRootFile (matches C++ RootLayerSublayersPolicy).
const (
	RootSubNone       = 0
	RootSubShallow    = 1
	RootSubDepthFirst = 2
)

// Stage wraps FreeusdStage (attach root layer).
type Stage struct {
	ptr *C.FreeusdStage
}

// OpenStageFromRootFile loads a USDA root from disk and optionally stacks subLayers (see RootSub*).
// Returns nil on failure (see LastErrorMessage). The stage owns loaded layers; do not free separate Layer handles.
func OpenStageFromRootFile(path string, sublayerPolicy int) *Stage {
	if path == "" {
		return nil
	}
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	p := C.freeusd_stage_open_from_root_file_utf8(cs, C.int(sublayerPolicy))
	if p == nil {
		return nil
	}
	return &Stage{ptr: p}
}

// AttachRootLayer builds a stage from a layer (keep the layer alive for the stage lifetime).
func AttachRootLayer(layer *Layer) *Stage {
	if layer == nil || layer.ptr == nil {
		return nil
	}
	p := C.freeusd_stage_attach_root_layer(layer.ptr)
	if p == nil {
		return nil
	}
	return &Stage{ptr: p}
}

// Free releases the stage.
func (s *Stage) Free() {
	if s == nil || s.ptr == nil {
		return
	}
	C.freeusd_stage_free(s.ptr)
	s.ptr = nil
}

// PrimSpecifier* match C FreeusdPrimSpecifierKind (composed def/class/over).
const (
	PrimSpecifierDefault = 0
	PrimSpecifierDef     = 1
	PrimSpecifierClass   = 2
	PrimSpecifierOver    = 3
)

// ResolvePrimSpecifierKind returns composed specifier kind, or negative C error code.
func (s *Stage) ResolvePrimSpecifierKind(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_resolve_prim_specifier_kind(s.ptr, pp))
}

// PrimPathInUse reports whether a prim path exists in the composed stage (1=yes via C ABI).
func (s *Stage) PrimPathInUse(primPath string) bool {
	if s == nil || s.ptr == nil {
		return false
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_prim_is_valid(s.ptr, pp)) == 1
}

// RelocatePairSep is U+001F, the separator between FROM and TO in each composed relocate or
// composed prefixSubstitution pair string from ListComposedRelocatePairs / ListComposedPrefixSubstitutionPairs.
const RelocatePairSep = "\x1f"

// RelocateSourceInAnyLayer returns 1 if any composed layer authors relocates for fromPrim, 0 if not, negative on error.
func (s *Stage) RelocateSourceInAnyLayer(fromPrim string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(fromPrim)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_relocate_source_in_any_layer(s.ptr, pp))
}

// ComposedRelocateTarget returns the composed relocate target for fromPrim.
// On rc==0, target is set; rc matches C FREEUSD_ERR_NOT_FOUND (3) when unmapped.
func (s *Stage) ComposedRelocateTarget(fromPrim string) (target string, rc int) {
	if s == nil || s.ptr == nil {
		return "", -1
	}
	pp := C.CString(fromPrim)
	defer C.free(unsafe.Pointer(pp))
	var out *C.char
	rc = int(C.freeusd_stage_get_composed_relocate_target_utf8(s.ptr, pp, &out))
	if rc == 0 && out != nil {
		target = C.GoString(out)
		C.freeusd_string_free(out)
	}
	return target, rc
}

// ListComposedRelocatePairs returns sorted pair strings "FROM"+RelocatePairSep+"TO"; rc is C result (0 ok).
func (s *Stage) ListComposedRelocatePairs() (pairs []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_composed_relocate_pairs_utf8(s.ptr, &arr, &n))
	if rc != 0 || n == 0 || arr == nil {
		return nil, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			pairs = append(pairs, C.GoString(p))
		}
	}
	return pairs, rc
}

// PrefixSubstitutionKeyInAnyLayer returns 1 if any composed layer authors prefixSubstitutions for fromPrefix, 0 if not.
func (s *Stage) PrefixSubstitutionKeyInAnyLayer(fromPrefix string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(fromPrefix)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_prefix_substitution_key_in_any_layer(s.ptr, pp))
}

// ComposedPrefixSubstitution returns the composed target prefix for fromPrefix. rc is 0 on success; 3 = not found (C NOT_FOUND).
func (s *Stage) ComposedPrefixSubstitution(fromPrefix string) (toPrefix string, rc int) {
	if s == nil || s.ptr == nil {
		return "", -1
	}
	pp := C.CString(fromPrefix)
	defer C.free(unsafe.Pointer(pp))
	var out *C.char
	rc = int(C.freeusd_stage_get_composed_prefix_substitution_utf8(s.ptr, pp, &out))
	if rc == 0 && out != nil {
		toPrefix = C.GoString(out)
		C.freeusd_string_free(out)
	}
	return toPrefix, rc
}

// ListComposedPrefixSubstitutionPairs returns sorted "FROM"+RelocatePairSep+"TO" strings; rc is C result (0 ok).
func (s *Stage) ListComposedPrefixSubstitutionPairs() (pairs []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_composed_prefix_substitution_pairs_utf8(s.ptr, &arr, &n))
	if rc != 0 || n == 0 || arr == nil {
		return nil, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			pairs = append(pairs, C.GoString(p))
		}
	}
	return pairs, rc
}

// CustomLayerDataKeyInAnyLayer returns 1 if any composed layer has customLayerData for key, 0 if not.
func (s *Stage) CustomLayerDataKeyInAnyLayer(key string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	k := C.CString(key)
	defer C.free(unsafe.Pointer(k))
	return int(C.freeusd_stage_custom_layer_data_key_in_any_layer(s.ptr, k))
}

// ComposedCustomLayerData returns the composed string/token customLayerData value for key (rc 0 ok; 3 = not found).
func (s *Stage) ComposedCustomLayerData(key string) (value string, rc int) {
	if s == nil || s.ptr == nil {
		return "", -1
	}
	k := C.CString(key)
	defer C.free(unsafe.Pointer(k))
	var out *C.char
	rc = int(C.freeusd_stage_get_composed_custom_layer_data_utf8(s.ptr, k, &out))
	if rc == 0 && out != nil {
		value = C.GoString(out)
		C.freeusd_string_free(out)
	}
	return value, rc
}

// ListComposedCustomLayerDataKeys returns sorted customLayerData keys across the stack (rc 0 ok).
func (s *Stage) ListComposedCustomLayerDataKeys() (keys []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_composed_custom_layer_data_keys(s.ptr, &arr, &n))
	if rc != 0 || n == 0 || arr == nil {
		return nil, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			keys = append(keys, C.GoString(p))
		}
	}
	return keys, rc
}

// ComposedPrimVariantSelection returns the composed variantSelection choice for variantSet on prim (rc 0 ok; 3 = not found).
func (s *Stage) ComposedPrimVariantSelection(primPath, variantSet string) (selected string, rc int) {
	if s == nil || s.ptr == nil {
		return "", -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	vs := C.CString(variantSet)
	defer C.free(unsafe.Pointer(vs))
	var out *C.char
	rc = int(C.freeusd_stage_get_composed_prim_variant_selection_utf8(s.ptr, pp, vs, &out))
	if rc == 0 && out != nil {
		selected = C.GoString(out)
		C.freeusd_string_free(out)
	}
	return selected, rc
}

// PrimVariantSelectionSetInAnyLayer returns 1 if any layer authors variantSelection for that set on prim.
func (s *Stage) PrimVariantSelectionSetInAnyLayer(primPath, variantSet string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	vs := C.CString(variantSet)
	defer C.free(unsafe.Pointer(vs))
	return int(C.freeusd_stage_prim_variant_selection_set_in_any_layer(s.ptr, pp, vs))
}

// ListComposedPrimVariantSelectionSets returns sorted variantSelection set names on prim.
func (s *Stage) ListComposedPrimVariantSelectionSets(primPath string) (items []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_composed_prim_variant_selection_sets_utf8(s.ptr, pp, &arr, &n))
	if rc != 0 || n == 0 || arr == nil {
		return nil, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			items = append(items, C.GoString(p))
		}
	}
	return items, rc
}

// ListComposedPrimVariantSetNames returns sorted variantSets keys on prim.
func (s *Stage) ListComposedPrimVariantSetNames(primPath string) (items []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_composed_prim_variant_set_names_utf8(s.ptr, pp, &arr, &n))
	if rc != 0 || n == 0 || arr == nil {
		return nil, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			items = append(items, C.GoString(p))
		}
	}
	return items, rc
}

// PrimVariantSetDeclaredInAnyLayer returns 1 if variantSets declares variantSetName on prim.
func (s *Stage) PrimVariantSetDeclaredInAnyLayer(primPath, variantSetName string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	vn := C.CString(variantSetName)
	defer C.free(unsafe.Pointer(vn))
	return int(C.freeusd_stage_prim_variant_set_declared_in_any_layer(s.ptr, pp, vn))
}

// ListComposedPrimVariantNames returns variant names for variantSetName (strongest declaring layer; rc 0 ok; 3 if set missing).
func (s *Stage) ListComposedPrimVariantNames(primPath, variantSetName string) ([]string, int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	vn := C.CString(variantSetName)
	defer C.free(unsafe.Pointer(vn))
	var arr **C.char
	var n C.size_t
	rc := int(C.freeusd_stage_list_composed_prim_variant_names_utf8(s.ptr, pp, vn, &arr, &n))
	if rc != 0 {
		return nil, rc
	}
	if n == 0 || arr == nil {
		return []string{}, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	var items []string
	for _, p := range slice {
		if p != nil {
			items = append(items, C.GoString(p))
		}
	}
	return items, rc
}

// ReadFieldDouble reads a composed double attribute at the given time.
func (s *Stage) ReadFieldDouble(primPath, attrName string, time float64) (float64, int) {
	if s == nil || s.ptr == nil {
		return 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out C.double
	rc := int(C.freeusd_stage_read_field_double(s.ptr, pp, an, C.double(time), &out))
	return float64(out), rc
}
