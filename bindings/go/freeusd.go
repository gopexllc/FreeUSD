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

// UsdcCrateIdentifier returns the USDC crate magic prefix "PXR-USDC" (static storage in C; do not free).
func UsdcCrateIdentifier() string {
	return C.GoString(C.freeusd_usdc_crate_identifier_utf8())
}

// UsdFileKind is a sniff-only file classification (matches C FreeusdUsdFileKind / C++ UsdFileKind).
type UsdFileKind int

const (
	UsdFileKindIoOrEmpty UsdFileKind = 0
	UsdFileKindUsdaAscii UsdFileKind = 1
	UsdFileKindUsdcCrate UsdFileKind = 2
	UsdFileKindUnknown UsdFileKind = 3
)

// DetectUsdFileKindFromPath reads the first bytes of path (no full USDC decode). On success rc is 0 and kind is set.
// detail is non-empty only when the C API returned optional diagnostics (typically for IoOrEmpty).
func DetectUsdFileKindFromPath(path string) (kind UsdFileKind, detail string, rc int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var out C.int
	var dptr *C.char
	cret := C.freeusd_detect_usd_file_kind_from_path_utf8(cs, &out, &dptr)
	rc = int(cret)
	if rc != 0 {
		return 0, "", rc
	}
	kind = UsdFileKind(out)
	if dptr != nil {
		detail = C.GoString(dptr)
		C.freeusd_string_free(dptr)
	}
	return kind, detail, 0
}

// UsdcBootstrap is the decoded USDC file bootstrap (matches C FreeusdUsdcBootstrap).
type UsdcBootstrap struct {
	FileVersionMajor byte
	FileVersionMinor byte
	FileVersionPatch byte
	TocByteOffset      int64
}

// ReadUsdcBootstrapFromPath reads the 88-byte USDC bootstrap (little-endian TOC offset). rc is 0 on success.
func ReadUsdcBootstrapFromPath(path string) (boot UsdcBootstrap, rc int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw C.FreeusdUsdcBootstrap
	rc = int(C.freeusd_read_usdc_bootstrap_from_path_utf8(cs, &raw))
	if rc != 0 {
		return UsdcBootstrap{}, rc
	}
	return UsdcBootstrap{
		FileVersionMajor: byte(raw.file_version_major),
		FileVersionMinor: byte(raw.file_version_minor),
		FileVersionPatch: byte(raw.file_version_patch),
		TocByteOffset:      int64(raw.toc_byte_offset),
	}, 0
}

// UsdcTocSection is one USDC TOC entry (matches C FreeusdUsdcTocSection).
type UsdcTocSection struct {
	Name             string
	StartByteOffset int64
	SizeBytes       int64
}

// ReadUsdcTocFromPath reads the crate TOC (section names and byte ranges). maxSections caps the allowed section count.
// On success rc is 0; total matches len(sections). Frees the C allocation before returning.
func ReadUsdcTocFromPath(path string, maxSections uint64) (sections []UsdcTocSection, total uint64, rc int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var totalC C.uint64_t
	var ret C.uint64_t
	var raw *C.FreeusdUsdcTocSection
	rc = int(C.freeusd_read_usdc_toc_from_path_utf8(cs, C.uint64_t(maxSections), &totalC, &raw, &ret))
	if rc != 0 {
		return nil, 0, rc
	}
	total = uint64(totalC)
	if total == 0 {
		return []UsdcTocSection{}, 0, 0
	}
	if raw == nil || ret == 0 || uint64(ret) != total {
		if raw != nil {
			C.freeusd_usdc_toc_sections_free(raw)
		}
		return nil, 0, 4
	}
	out := make([]UsdcTocSection, int(ret))
	p := unsafe.Pointer(raw)
	step := int(unsafe.Sizeof(C.FreeusdUsdcTocSection{}))
	for i := range out {
		sec := (*C.FreeusdUsdcTocSection)(unsafe.Add(p, step*i))
		name := C.GoBytes(unsafe.Pointer(&sec.name[0]), 16)
		z := 0
		for z < len(name) && name[z] != 0 {
			z++
		}
		out[i] = UsdcTocSection{
			Name:             string(name[:z]),
			StartByteOffset: int64(sec.start_byte_offset),
			SizeBytes:       int64(sec.size_bytes),
		}
	}
	C.freeusd_usdc_toc_sections_free(raw)
	return out, total, 0
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

func stageLayerDouble(s *Stage, get func(*C.FreeusdStage, *C.double, *C.int) C.int) (value float64, has bool, rc int) {
	if s == nil || s.ptr == nil {
		return 0, false, -1
	}
	var v C.double
	var h C.int
	rc = int(get(s.ptr, &v, &h))
	if rc != 0 {
		return 0, false, rc
	}
	return float64(v), h != 0, rc
}

// StartTimeCode returns (value, has, rc) from composed layer metadata (rc 0 on success).
func (s *Stage) StartTimeCode() (float64, bool, int) {
	return stageLayerDouble(s, func(st *C.FreeusdStage, v *C.double, h *C.int) C.int {
		return C.freeusd_stage_get_start_time_code(st, v, h)
	})
}

// EndTimeCode returns (value, has, rc).
func (s *Stage) EndTimeCode() (float64, bool, int) {
	return stageLayerDouble(s, func(st *C.FreeusdStage, v *C.double, h *C.int) C.int {
		return C.freeusd_stage_get_end_time_code(st, v, h)
	})
}

// TimeCodesPerSecond returns (value, has, rc).
func (s *Stage) TimeCodesPerSecond() (float64, bool, int) {
	return stageLayerDouble(s, func(st *C.FreeusdStage, v *C.double, h *C.int) C.int {
		return C.freeusd_stage_get_time_codes_per_second(st, v, h)
	})
}

// FramesPerSecond returns (value, has, rc).
func (s *Stage) FramesPerSecond() (float64, bool, int) {
	return stageLayerDouble(s, func(st *C.FreeusdStage, v *C.double, h *C.int) C.int {
		return C.freeusd_stage_get_frames_per_second(st, v, h)
	})
}

// MetersPerUnit returns (value, has, rc).
func (s *Stage) MetersPerUnit() (float64, bool, int) {
	return stageLayerDouble(s, func(st *C.FreeusdStage, v *C.double, h *C.int) C.int {
		return C.freeusd_stage_get_meters_per_unit(st, v, h)
	})
}

// FramePrecision returns (value, has, rc); value is only valid when has is true.
func (s *Stage) FramePrecision() (int64, bool, int) {
	if s == nil || s.ptr == nil {
		return 0, false, -1
	}
	var v C.int64_t
	var h C.int
	rc := int(C.freeusd_stage_get_frame_precision(s.ptr, &v, &h))
	if rc != 0 {
		return 0, false, rc
	}
	return int64(v), h != 0, rc
}

// UpAxis returns composed upAxis string and rc (0 ok; 3 = C NOT_FOUND if unset).
func (s *Stage) UpAxis() (axis string, rc int) {
	if s == nil || s.ptr == nil {
		return "", -1
	}
	var out *C.char
	rc = int(C.freeusd_stage_get_up_axis_utf8(s.ptr, &out))
	if rc == 0 && out != nil {
		axis = C.GoString(out)
		C.freeusd_string_free(out)
	}
	return axis, rc
}

// PrimOrderPaths returns ordered prim paths from composed primOrder (rc 0 ok; empty slice if none).
func (s *Stage) PrimOrderPaths() (paths []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, -1
	}
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_prim_order_paths_utf8(s.ptr, &arr, &n))
	if rc != 0 {
		return nil, rc
	}
	if n == 0 || arr == nil {
		return []string{}, rc
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			paths = append(paths, C.GoString(p))
		}
	}
	return paths, rc
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
// On non-zero rc the returned float64 is always zero (C out is not read).
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
	if rc != 0 {
		return 0, rc
	}
	return float64(out), 0
}

// ReadFieldFloat reads a composed float attribute (or double / int / bool coerced to float32) at the given time.
// On non-zero rc the returned float32 is always zero.
func (s *Stage) ReadFieldFloat(primPath, attrName string, time float64) (float32, int) {
	if s == nil || s.ptr == nil {
		return 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out C.float
	rc := int(C.freeusd_stage_read_field_float(s.ptr, pp, an, C.double(time), &out))
	if rc != 0 {
		return 0, rc
	}
	return float32(out), 0
}

// ReadFieldBool reads a composed bool attribute at the given time.
// On non-zero rc, v is always false.
func (s *Stage) ReadFieldBool(primPath, attrName string, time float64) (v bool, rc int) {
	if s == nil || s.ptr == nil {
		return false, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out C.int
	rc = int(C.freeusd_stage_read_field_bool(s.ptr, pp, an, C.double(time), &out))
	if rc != 0 {
		return false, rc
	}
	return out != 0, 0
}

// ReadFieldInt64 reads a composed int64 attribute (or coerced int/bool/float) at the given time.
// On non-zero rc, n is always zero.
func (s *Stage) ReadFieldInt64(primPath, attrName string, time float64) (n int64, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out C.int64_t
	rc = int(C.freeusd_stage_read_field_int64(s.ptr, pp, an, C.double(time), &out))
	if rc != 0 {
		return 0, rc
	}
	return int64(out), 0
}

// ReadFieldString reads a composed string attribute (or token text) at the given time.
// On non-zero rc, str is always empty.
func (s *Stage) ReadFieldString(primPath, attrName string, time float64) (str string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out *C.char
	rc = int(C.freeusd_stage_read_field_string(s.ptr, pp, an, C.double(time), &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	str = C.GoString(out)
	C.freeusd_string_free(out)
	return str, 0
}

// ReadFieldVec3d reads a composed double3 / Vec3d attribute at the given time.
// On non-zero rc, x/y/z are always zero.
func (s *Stage) ReadFieldVec3d(primPath, attrName string, time float64) (x, y, z float64, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 0, 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var ox, oy, oz C.double
	rc = int(C.freeusd_stage_read_field_vec3d(s.ptr, pp, an, C.double(time), &ox, &oy, &oz))
	if rc != 0 {
		return 0, 0, 0, rc
	}
	return float64(ox), float64(oy), float64(oz), 0
}

// ReadFieldVec3f reads a composed float3 / Vec3f attribute (or Vec3d narrowed to float) at the given time.
// On non-zero rc, x/y/z are always zero.
func (s *Stage) ReadFieldVec3f(primPath, attrName string, time float64) (x, y, z float32, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 0, 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var ox, oy, oz C.float
	rc = int(C.freeusd_stage_read_field_vec3f(s.ptr, pp, an, C.double(time), &ox, &oy, &oz))
	if rc != 0 {
		return 0, 0, 0, rc
	}
	return float32(ox), float32(oy), float32(oz), 0
}

// ReadFieldMatrix4d reads a composed matrix4d (row-major 16 doubles) at the given time.
func (s *Stage) ReadFieldMatrix4d(primPath, attrName string, time float64) (m [16]float64, rc int) {
	if s == nil || s.ptr == nil {
		return m, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var buf [16]C.double
	rc = int(C.freeusd_stage_read_field_matrix4d(s.ptr, pp, an, C.double(time), &buf[0]))
	if rc != 0 {
		return m, rc
	}
	for i := 0; i < 16; i++ {
		m[i] = float64(buf[i])
	}
	return m, 0
}

// ReadFieldQuatd reads a composed quatd (real, i, j, k) at the given time.
// On non-zero rc, all quaternion components are zero.
func (s *Stage) ReadFieldQuatd(primPath, attrName string, time float64) (real, i, j, k float64, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 0, 0, 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var wr, wi, wj, wk C.double
	rc = int(C.freeusd_stage_read_field_quatd(s.ptr, pp, an, C.double(time), &wr, &wi, &wj, &wk))
	if rc != 0 {
		return 0, 0, 0, 0, rc
	}
	return float64(wr), float64(wi), float64(wj), float64(wk), 0
}

// ReadFieldQuatf reads a composed quatf (or quatd narrowed to float32) at the given time.
// On non-zero rc, all quaternion components are zero.
func (s *Stage) ReadFieldQuatf(primPath, attrName string, time float64) (real, i, j, k float32, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 0, 0, 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var wr, wi, wj, wk C.float
	rc = int(C.freeusd_stage_read_field_quatf(s.ptr, pp, an, C.double(time), &wr, &wi, &wj, &wk))
	if rc != 0 {
		return 0, 0, 0, 0, rc
	}
	return float32(wr), float32(wi), float32(wj), float32(wk), 0
}

// ReadFieldToken reads a composed token (not a plain string) at the given time. On success rc is 0.
func (s *Stage) ReadFieldToken(primPath, attrName string, time float64) (token string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out *C.char
	rc = int(C.freeusd_stage_read_field_token(s.ptr, pp, an, C.double(time), &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	token = C.GoString(out)
	C.freeusd_string_free(out)
	return token, 0
}

// ReadFieldTokenArray reads composed token[] at the given time. On success rc is 0 (empty slice is valid).
func (s *Stage) ReadFieldTokenArray(primPath, attrName string, time float64) (tokens []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_read_field_token_array(s.ptr, pp, an, C.double(time), &arr, &n))
	if rc != 0 {
		return nil, rc
	}
	if n == 0 || arr == nil {
		return []string{}, 0
	}
	defer C.freeusd_path_list_free(arr, n)
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			tokens = append(tokens, C.GoString(p))
		}
	}
	return tokens, 0
}
