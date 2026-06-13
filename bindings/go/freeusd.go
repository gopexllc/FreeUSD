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
#cgo linux LDFLAGS: -L${SRCDIR}/../../build/src -lfreeusd_c -lfreeusd_base -lfreeusd_usdUtils -lfreeusd_usdSkel -lfreeusd_usdShade -lfreeusd_usdLux -lfreeusd_usdPhysics -lfreeusd_usdVol -lfreeusd_usdGeom -lfreeusd_usd -lfreeusd_ar -lfreeusd_io -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_gf -lfreeusd_plug -lstdc++ -lm -lz -llz4
#cgo darwin LDFLAGS: -L${SRCDIR}/../../build/src -lfreeusd_c -lfreeusd_base -lfreeusd_usdUtils -lfreeusd_usdSkel -lfreeusd_usdShade -lfreeusd_usdLux -lfreeusd_usdPhysics -lfreeusd_usdVol -lfreeusd_usdGeom -lfreeusd_usd -lfreeusd_ar -lfreeusd_io -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf -lfreeusd_gf -lfreeusd_plug -lc++ -lm -lz -llz4
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
	UsdFileKindUnknown   UsdFileKind = 3
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
	TocByteOffset    int64
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
		TocByteOffset:    int64(raw.toc_byte_offset),
	}, 0
}

// UsdcTocSection is one USDC TOC entry (matches C FreeusdUsdcTocSection).
type UsdcTocSection struct {
	Name            string
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
			Name:            string(name[:z]),
			StartByteOffset: int64(sec.start_byte_offset),
			SizeBytes:       int64(sec.size_bytes),
		}
	}
	C.freeusd_usdc_toc_sections_free(raw)
	return out, total, 0
}

// ReadUsdcSectionBytesFromPath reads one raw USDC section payload by name. On success rc is 0.
// The returned bytes are copied into Go memory before the C allocation is freed.
func ReadUsdcSectionBytesFromPath(path, sectionName string, maxBytes uint64) (payload []byte, rc int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	cname := C.CString(sectionName)
	defer C.free(unsafe.Pointer(cname))
	var raw *C.uint8_t
	var n C.uint64_t
	rc = int(C.freeusd_read_usdc_section_bytes_from_path_utf8(cs, cname, C.uint64_t(maxBytes), &raw, &n))
	if rc != 0 {
		return nil, rc
	}
	if raw == nil || n == 0 {
		return []byte{}, 0
	}
	defer C.freeusd_bytes_free(unsafe.Pointer(raw))
	return C.GoBytes(unsafe.Pointer(raw), C.int(n)), 0
}

func readUsdcCStringTable(path string, maxEntries uint64, maxTotalBytes uint64, tableKind int) ([]string, int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw **C.char
	var n C.size_t
	var rc C.int
	switch tableKind {
	case 0:
		rc = C.freeusd_read_usdc_token_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n)
	case 1:
		rc = C.freeusd_read_usdc_string_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n)
	default:
		rc = C.freeusd_read_usdc_path_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n)
	}
	if int(rc) != 0 {
		return nil, int(rc)
	}
	if raw == nil || n == 0 {
		return []string{}, 0
	}
	defer C.freeusd_path_list_free(raw, n)
	count := int(n)
	out := make([]string, count)
	base := unsafe.Pointer(raw)
	step := unsafe.Sizeof(raw)
	for i := 0; i < count; i++ {
		ptr := *(**C.char)(unsafe.Add(base, uintptr(i)*step))
		out[i] = C.GoString(ptr)
	}
	return out, 0
}

// ReadUsdcTokenTableFromPath reads the validated TOKENS table from a shared crate fixture.
func ReadUsdcTokenTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
	return readUsdcCStringTable(path, maxEntries, maxTotalBytes, 0)
}

// ReadUsdcStringTableFromPath reads the validated STRINGS table from a shared crate fixture.
func ReadUsdcStringTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
	return readUsdcCStringTable(path, maxEntries, maxTotalBytes, 1)
}

// ReadUsdcPathTableFromPath reads the validated PATHS table from a shared crate fixture.
func ReadUsdcPathTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
	return readUsdcCStringTable(path, maxEntries, maxTotalBytes, 2)
}

// UsdcFieldEntry is one row from the validated FIELDS table (matches C FreeusdUsdcFieldEntry).
type UsdcFieldEntry struct {
	TokenIndex          uint64
	ValueTypeTokenIndex uint64
}

// ReadUsdcFieldsTableFromPath reads the validated FIELDS table from a shared crate fixture.
func ReadUsdcFieldsTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]UsdcFieldEntry, int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw *C.FreeusdUsdcFieldEntry
	var count C.size_t
	rc := int(C.freeusd_read_usdc_fields_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &count))
	if rc != 0 {
		return nil, rc
	}
	defer C.freeusd_usdc_fields_entries_free(raw)
	if raw == nil || count == 0 {
		return nil, 0
	}
	step := int(unsafe.Sizeof(C.FreeusdUsdcFieldEntry{}))
	out := make([]UsdcFieldEntry, int(count))
	base := unsafe.Pointer(raw)
	for i := range out {
		entry := (*C.FreeusdUsdcFieldEntry)(unsafe.Add(base, uintptr(i)*uintptr(step)))
		out[i] = UsdcFieldEntry{
			TokenIndex:          uint64(entry.token_index),
			ValueTypeTokenIndex: uint64(entry.value_type_token_index),
		}
	}
	return out, 0
}

// UsdcSpecEntry is one row from the validated SPECS table (matches C FreeusdUsdcSpecEntry).
type UsdcSpecEntry struct {
	PathIndex     uint64
	FieldSetIndex uint64
	SpecType      uint64
}

// ReadUsdcSpecsTableFromPath reads the validated SPECS table from a shared crate fixture.
func ReadUsdcSpecsTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]UsdcSpecEntry, int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw *C.FreeusdUsdcSpecEntry
	var count C.size_t
	rc := int(C.freeusd_read_usdc_specs_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &count))
	if rc != 0 {
		return nil, rc
	}
	defer C.freeusd_usdc_specs_entries_free(raw)
	if raw == nil || count == 0 {
		return nil, 0
	}
	step := int(unsafe.Sizeof(C.FreeusdUsdcSpecEntry{}))
	out := make([]UsdcSpecEntry, int(count))
	base := unsafe.Pointer(raw)
	for i := range out {
		entry := (*C.FreeusdUsdcSpecEntry)(unsafe.Add(base, uintptr(i)*uintptr(step)))
		out[i] = UsdcSpecEntry{
			PathIndex:     uint64(entry.path_index),
			FieldSetIndex: uint64(entry.field_set_index),
			SpecType:      uint64(entry.spec_type),
		}
	}
	return out, 0
}

// UsdcFieldSet is one row from the validated FIELDSETS table.
type UsdcFieldSet struct {
	FieldIndices []uint64
}

// ReadUsdcFieldSetsTableFromPath reads the validated FIELDSETS table from a shared crate fixture.
func ReadUsdcFieldSetsTableFromPath(path string, maxFieldSets uint64, maxFieldsPerSet uint64, maxTotalBytes uint64) ([]UsdcFieldSet, int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw *C.FreeusdUsdcFieldSet
	var count C.size_t
	rc := int(C.freeusd_read_usdc_fieldsets_table_from_path_utf8(cs, C.uint64_t(maxFieldSets), C.uint64_t(maxFieldsPerSet), C.uint64_t(maxTotalBytes), &raw, &count))
	if rc != 0 {
		return nil, rc
	}
	defer C.freeusd_usdc_fieldsets_free(raw, count)
	if raw == nil || count == 0 {
		return nil, 0
	}
	out := make([]UsdcFieldSet, int(count))
	for i := 0; i < int(count); i++ {
		entry := (*C.FreeusdUsdcFieldSet)(unsafe.Pointer(uintptr(unsafe.Pointer(raw)) + uintptr(i)*unsafe.Sizeof(C.FreeusdUsdcFieldSet{})))
		n := int(entry.field_count)
		if n == 0 {
			continue
		}
		indices := make([]uint64, n)
		base := unsafe.Pointer(entry.field_indices)
		step := int(unsafe.Sizeof(C.uint64_t(0)))
		for j := 0; j < n; j++ {
			ptr := (*C.uint64_t)(unsafe.Add(base, uintptr(j)*uintptr(step)))
			indices[j] = uint64(*ptr)
		}
		out[i] = UsdcFieldSet{FieldIndices: indices}
	}
	return out, 0
}

// UsdcValueBlob is one raw payload from the validated VALUES table.
type UsdcValueBlob struct {
	Bytes []byte
}

// UsdcTypedValue is one typed entry from the validated VALUES table (fixture-oriented kinds).
type UsdcTypedValue struct {
	Kind        uint64
	Bytes       []byte
	Int32Value  int32
	FloatValue  float32
	TokenIndex  uint64
	BoolValue   bool
	DoubleValue float64
	Int64Value  int64
	StringUtf8  string
	Vec3fValue  [3]float32
	StringIndex uint64
	Vec3dValue  [3]float64
	Int32Array  []int32
	FloatArray  []float32
	DoubleArray []float64
	Vec2fValue  [2]float32
	Vec4fValue  [4]float32
}

// ReadUsdcTypedValuesTableFromPath reads the validated typed VALUES table from a shared crate fixture.
func ReadUsdcTypedValuesTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]UsdcTypedValue, int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw *C.FreeusdUsdcTypedValue
	var count C.size_t
	rc := int(C.freeusd_read_usdc_typed_values_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &count))
	if rc != 0 {
		return nil, rc
	}
	defer C.freeusd_usdc_typed_values_free(raw, count)
	if raw == nil || count == 0 {
		return nil, 0
	}
	out := make([]UsdcTypedValue, int(count))
	for i := 0; i < int(count); i++ {
		entry := (*C.FreeusdUsdcTypedValue)(unsafe.Pointer(uintptr(unsafe.Pointer(raw)) + uintptr(i)*unsafe.Sizeof(C.FreeusdUsdcTypedValue{})))
		n := int(entry.byte_count)
		var slice []byte
		if n > 0 {
			slice = make([]byte, n)
			copy(slice, C.GoBytes(unsafe.Pointer(entry.bytes), C.int(n)))
		}
		var stringUtf8 string
		if entry.string_utf8 != nil {
			stringUtf8 = C.GoString(entry.string_utf8)
		}
		out[i] = UsdcTypedValue{
			Kind:        uint64(entry.kind),
			Bytes:       slice,
			Int32Value:  int32(entry.int32_value),
			FloatValue:  float32(entry.float_value),
			TokenIndex:  uint64(entry.token_index),
			BoolValue:   entry.bool_value != 0,
			DoubleValue: float64(entry.double_value),
			Int64Value:  int64(entry.int64_value),
			StringUtf8:  stringUtf8,
			Vec3fValue:  [3]float32{float32(entry.vec3f_value[0]), float32(entry.vec3f_value[1]), float32(entry.vec3f_value[2])},
			StringIndex: uint64(entry.string_index),
			Vec3dValue:  [3]float64{float64(entry.vec3d_value[0]), float64(entry.vec3d_value[1]), float64(entry.vec3d_value[2])},
			Int32Array: func() []int32 {
				n := int(entry.int32_array_count)
				if n == 0 || entry.int32_array == nil {
					return nil
				}
				out := make([]int32, n)
				for j := 0; j < n; j++ {
					v := *(*int32)(unsafe.Pointer(uintptr(unsafe.Pointer(entry.int32_array)) + uintptr(j)*unsafe.Sizeof(int32(0))))
					out[j] = v
				}
				return out
			}(),
			FloatArray: func() []float32 {
				n := int(entry.float_array_count)
				if n == 0 || entry.float_array == nil {
					return nil
				}
				out := make([]float32, n)
				for j := 0; j < n; j++ {
					v := *(*float32)(unsafe.Pointer(uintptr(unsafe.Pointer(entry.float_array)) + uintptr(j)*unsafe.Sizeof(float32(0))))
					out[j] = v
				}
				return out
			}(),
			DoubleArray: func() []float64 {
				n := int(entry.double_array_count)
				if n == 0 || entry.double_array == nil {
					return nil
				}
				out := make([]float64, n)
				for j := 0; j < n; j++ {
					v := *(*float64)(unsafe.Pointer(uintptr(unsafe.Pointer(entry.double_array)) + uintptr(j)*unsafe.Sizeof(float64(0))))
					out[j] = v
				}
				return out
			}(),
			Vec2fValue: [2]float32{float32(entry.vec2f_value[0]), float32(entry.vec2f_value[1])},
			Vec4fValue: [4]float32{float32(entry.vec4f_value[0]), float32(entry.vec4f_value[1]), float32(entry.vec4f_value[2]), float32(entry.vec4f_value[3])},
		}
	}
	return out, 0
}

// ReadUsdcValuesTableFromPath reads the validated VALUES table from a shared crate fixture.
func ReadUsdcValuesTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]UsdcValueBlob, int) {
	cs := C.CString(path)
	defer C.free(unsafe.Pointer(cs))
	var raw *C.FreeusdUsdcValueBlob
	var count C.size_t
	rc := int(C.freeusd_read_usdc_values_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &count))
	if rc != 0 {
		return nil, rc
	}
	defer C.freeusd_usdc_values_blobs_free(raw, count)
	if raw == nil || count == 0 {
		return nil, 0
	}
	out := make([]UsdcValueBlob, int(count))
	for i := 0; i < int(count); i++ {
		entry := (*C.FreeusdUsdcValueBlob)(unsafe.Pointer(uintptr(unsafe.Pointer(raw)) + uintptr(i)*unsafe.Sizeof(C.FreeusdUsdcValueBlob{})))
		n := int(entry.byte_count)
		if n == 0 {
			continue
		}
		slice := make([]byte, n)
		copy(slice, C.GoBytes(unsafe.Pointer(entry.bytes), C.int(n)))
		out[i] = UsdcValueBlob{Bytes: slice}
	}
	return out, 0
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

// ResolvePrimActive returns the composed active flag (default true), with rc 0 on success.
func (s *Stage) ResolvePrimActive(primPath string) (active bool, rc int) {
	if s == nil || s.ptr == nil {
		return false, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var out C.int
	rc = int(C.freeusd_stage_resolve_prim_active(s.ptr, pp, &out))
	if rc != 0 {
		return false, rc
	}
	return out != 0, 0
}

// ResolveHasPrimActiveOpinion reports whether any composed layer authors active on primPath.
func (s *Stage) ResolveHasPrimActiveOpinion(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_resolve_has_prim_active_opinion(s.ptr, pp))
}

// ResolvePrimKind returns the composed schema kind token text for primPath.
func (s *Stage) ResolvePrimKind(primPath string) (kind string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	out := C.freeusd_stage_resolve_prim_kind(s.ptr, pp)
	if out == nil {
		return "", 3
	}
	defer C.freeusd_string_free(out)
	return C.GoString(out), 0
}

// ResolveHasPrimKind reports whether the composed prim has a schema kind opinion.
func (s *Stage) ResolveHasPrimKind(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_resolve_has_prim_kind(s.ptr, pp))
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

// ComputeLocalTransformMatrix4d returns the prim local transform (row-major 16 doubles) via usdGeom::Xformable.
func (s *Stage) ComputeLocalTransformMatrix4d(primPath string, time float64) (m [16]float64, rc int) {
	if s == nil || s.ptr == nil {
		return m, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	rc = int(C.freeusd_stage_compute_local_transform_matrix4d(s.ptr, pp, C.double(time), (*C.double)(unsafe.Pointer(&m[0]))))
	return m, rc
}

// ComputeLocalToWorldTransformMatrix4d returns the prim local-to-world transform (row-major 16 doubles).
func (s *Stage) ComputeLocalToWorldTransformMatrix4d(primPath string, time float64) (m [16]float64, rc int) {
	if s == nil || s.ptr == nil {
		return m, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	rc = int(C.freeusd_stage_compute_local_to_world_transform_matrix4d(s.ptr, pp, C.double(time), (*C.double)(unsafe.Pointer(&m[0]))))
	return m, rc
}

// ComputeImageableVisibility returns inherited visibility (true = visible) via usdGeom::Imageable.
func (s *Stage) ComputeImageableVisibility(primPath string, time float64) (visible bool, rc int) {
	if s == nil || s.ptr == nil {
		return false, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var out C.int
	rc = int(C.freeusd_stage_compute_imageable_visibility(s.ptr, pp, C.double(time), &out))
	return out != 0, rc
}

// ComputeImageablePurpose returns inherited purpose via usdGeom::Imageable. On success rc is 0.
func (s *Stage) ComputeImageablePurpose(primPath string, time float64) (purpose string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var out *C.char
	rc = int(C.freeusd_stage_compute_imageable_purpose_utf8(s.ptr, pp, C.double(time), &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	defer C.freeusd_string_free(out)
	return C.GoString(out), 0
}

// ComputeBoundableWorldBounds returns world-space axis-aligned bounds via usdGeom::Boundable.
func (s *Stage) ComputeBoundableWorldBounds(primPath string, time float64) (minX, minY, minZ, maxX, maxY, maxZ float64, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 0, 0, 0, 0, 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var mnX, mnY, mnZ, mxX, mxY, mxZ C.double
	rc = int(C.freeusd_stage_compute_boundable_world_bounds(s.ptr, pp, C.double(time), &mnX, &mnY, &mnZ, &mxX, &mxY, &mxZ))
	return float64(mnX), float64(mnY), float64(mnZ), float64(mxX), float64(mxY), float64(mxZ), rc
}

// ComputeBoundableLocalBounds returns local-space axis-aligned bounds via usdGeom::Boundable.
func (s *Stage) ComputeBoundableLocalBounds(primPath string, time float64) (minX, minY, minZ, maxX, maxY, maxZ float64, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 0, 0, 0, 0, 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var mnX, mnY, mnZ, mxX, mxY, mxZ C.double
	rc = int(C.freeusd_stage_compute_boundable_local_bounds(s.ptr, pp, C.double(time), &mnX, &mnY, &mnZ, &mxX, &mxY, &mxZ))
	return float64(mnX), float64(mnY), float64(mnZ), float64(mxX), float64(mxY), float64(mxZ), rc
}

// ReadMaterialSurfaceShaderPath resolves a Material outputs:surface connection to a shader prim path.
func (s *Stage) ReadMaterialSurfaceShaderPath(materialPath string) (shaderPath string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	mp := C.CString(materialPath)
	defer C.free(unsafe.Pointer(mp))
	var out *C.char
	rc = int(C.freeusd_stage_read_material_surface_shader_path(s.ptr, mp, &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	defer C.freeusd_string_free(out)
	return C.GoString(out), 0
}

// ReadPreviewSurfaceDiffuseColor reads UsdPreviewSurface inputs:diffuseColor at time.
func (s *Stage) ReadPreviewSurfaceDiffuseColor(shaderPath string, time float64) (rgb [3]float32, rc int) {
	if s == nil || s.ptr == nil {
		return rgb, 1
	}
	sp := C.CString(shaderPath)
	defer C.free(unsafe.Pointer(sp))
	var out [3]C.float
	rc = int(C.freeusd_stage_read_preview_surface_diffuse_color(s.ptr, sp, C.double(time), &out[0]))
	if rc != 0 {
		return rgb, rc
	}
	return [3]float32{float32(out[0]), float32(out[1]), float32(out[2])}, 0
}

// ReadPreviewSurfaceDiffuseTextureAssetPath reads a direct or connected diffuse texture asset path.
func (s *Stage) ReadPreviewSurfaceDiffuseTextureAssetPath(shaderPath string, time float64) (assetPath string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	sp := C.CString(shaderPath)
	defer C.free(unsafe.Pointer(sp))
	var out *C.char
	rc = int(C.freeusd_stage_read_preview_surface_diffuse_texture_asset_path(s.ptr, sp, C.double(time), &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	defer C.freeusd_string_free(out)
	return C.GoString(out), 0
}

// ListFieldSampleTimes returns sorted composed time-sample times for an attribute (rc 0 ok; empty slice valid).
func (s *Stage) ListFieldSampleTimes(primPath, attrName string) (times []float64, rc int) {
	if s == nil || s.ptr == nil {
		return nil, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var raw *C.double
	var n C.size_t
	rc = int(C.freeusd_stage_list_field_sample_times(s.ptr, pp, an, &raw, &n))
	if rc != 0 {
		return nil, rc
	}
	if n == 0 || raw == nil {
		return []float64{}, 0
	}
	defer C.freeusd_double_array_free(raw)
	slice := unsafe.Slice(raw, int(n))
	times = make([]float64, int(n))
	for i, t := range slice {
		times[i] = float64(t)
	}
	return times, 0
}

// HasFieldOpinion returns 1 if the attribute has a composed opinion, 0 if not, negative on error.
func (s *Stage) HasFieldOpinion(primPath, attrName string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	return int(C.freeusd_stage_has_field_opinion(s.ptr, pp, an))
}

// HasAttributeConnection returns 1 if the attribute has a composed .connect, 0 if not, negative on error.
func (s *Stage) HasAttributeConnection(primPath, attrName string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	return int(C.freeusd_stage_has_attribute_connection(s.ptr, pp, an))
}

// GetAttributeConnectionTarget returns the strongest composed connection target property path (rc 0 ok).
func (s *Stage) GetAttributeConnectionTarget(primPath, attrName string) (target string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	an := C.CString(attrName)
	defer C.free(unsafe.Pointer(an))
	var out *C.char
	rc = int(C.freeusd_stage_get_attribute_connection_target(s.ptr, pp, an, &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	defer C.freeusd_string_free(out)
	return C.GoString(out), 0
}

func (s *Stage) listPrimPathStrings(primPath string, kind primPathListKind) ([]string, int) {
	if s == nil || s.ptr == nil {
		return nil, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var arr **C.char
	var n C.size_t
	var rc C.int
	switch kind {
	case primPathListReferences:
		rc = C.freeusd_stage_list_prim_references(s.ptr, pp, &arr, &n)
	case primPathListInherits:
		rc = C.freeusd_stage_list_prim_inherits(s.ptr, pp, &arr, &n)
	case primPathListSpecializes:
		rc = C.freeusd_stage_list_prim_specializes(s.ptr, pp, &arr, &n)
	default:
		rc = C.freeusd_stage_list_prim_payloads(s.ptr, pp, &arr, &n)
	}
	if int(rc) != 0 {
		return nil, int(rc)
	}
	if n == 0 || arr == nil {
		return []string{}, 0
	}
	defer C.freeusd_path_list_free(arr, n)
	out := make([]string, 0, int(n))
	slice := unsafe.Slice(arr, int(n))
	for _, p := range slice {
		if p != nil {
			out = append(out, C.GoString(p))
		}
	}
	return out, 0
}

type primPathListKind int

const (
	primPathListReferences primPathListKind = iota
	primPathListInherits
	primPathListSpecializes
	primPathListPayloads
)

// ListPrimReferences returns composed reference entries (USDA-authored encoding).
func (s *Stage) ListPrimReferences(primPath string) ([]string, int) {
	return s.listPrimPathStrings(primPath, primPathListReferences)
}

// HasPrimReferences returns 1 if the prim has composed references, 0 if not, negative on error.
func (s *Stage) HasPrimReferences(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_has_prim_references(s.ptr, pp))
}

// ListPrimInherits returns composed inherit target prim paths.
func (s *Stage) ListPrimInherits(primPath string) ([]string, int) {
	return s.listPrimPathStrings(primPath, primPathListInherits)
}

// HasPrimInherits returns 1 if the prim has composed inherits, 0 if not, negative on error.
func (s *Stage) HasPrimInherits(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_has_prim_inherits(s.ptr, pp))
}

// ListPrimSpecializes returns composed specialize target prim paths.
func (s *Stage) ListPrimSpecializes(primPath string) ([]string, int) {
	return s.listPrimPathStrings(primPath, primPathListSpecializes)
}

// HasPrimSpecializes returns 1 if the prim has composed specializes, 0 if not, negative on error.
func (s *Stage) HasPrimSpecializes(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_has_prim_specializes(s.ptr, pp))
}

// ListPrimPayloads returns composed payload entries (USDA-authored encoding).
func (s *Stage) ListPrimPayloads(primPath string) ([]string, int) {
	return s.listPrimPathStrings(primPath, primPathListPayloads)
}

// HasPrimPayloads returns 1 if the prim has composed payloads, 0 if not, negative on error.
func (s *Stage) HasPrimPayloads(primPath string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	return int(C.freeusd_stage_has_prim_payloads(s.ptr, pp))
}

// ComposedPrimCustomData returns string/token customData for key (rc 0 ok; 3 = not found).
func (s *Stage) ComposedPrimCustomData(primPath, key string) (value string, rc int) {
	if s == nil || s.ptr == nil {
		return "", 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	k := C.CString(key)
	defer C.free(unsafe.Pointer(k))
	var out *C.char
	rc = int(C.freeusd_stage_get_composed_prim_custom_data(s.ptr, pp, k, &out))
	if rc != 0 {
		return "", rc
	}
	if out == nil {
		return "", 0
	}
	defer C.freeusd_string_free(out)
	return C.GoString(out), 0
}

// ComposedPrimCustomDataInt64 returns int/bool customData for key (rc 0 ok; 3 = not found).
func (s *Stage) ComposedPrimCustomDataInt64(primPath, key string) (value int64, rc int) {
	if s == nil || s.ptr == nil {
		return 0, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	k := C.CString(key)
	defer C.free(unsafe.Pointer(k))
	var out C.int64_t
	rc = int(C.freeusd_stage_get_composed_prim_custom_data_int64(s.ptr, pp, k, &out))
	return int64(out), rc
}

// PrimCustomDataKeyInAnyLayer returns 1 if any layer authors the customData key on the prim.
func (s *Stage) PrimCustomDataKeyInAnyLayer(primPath, key string) int {
	if s == nil || s.ptr == nil {
		return -1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	k := C.CString(key)
	defer C.free(unsafe.Pointer(k))
	return int(C.freeusd_stage_prim_custom_data_key_in_any_layer(s.ptr, pp, k))
}

// ListComposedPrimCustomDataKeys returns sorted customData keys for the prim (rc 0 ok).
func (s *Stage) ListComposedPrimCustomDataKeys(primPath string) (keys []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, 1
	}
	pp := C.CString(primPath)
	defer C.free(unsafe.Pointer(pp))
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_list_composed_prim_custom_data_keys(s.ptr, pp, &arr, &n))
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
			keys = append(keys, C.GoString(p))
		}
	}
	return keys, 0
}

// EngineRuntimeMode matches C FreeusdEngineRuntimeMode.
type EngineRuntimeMode int

const (
	EngineRuntimePreBaked         EngineRuntimeMode = 0
	EngineRuntimeHybrid           EngineRuntimeMode = 1
	EngineRuntimeExperimentalLive EngineRuntimeMode = 2
)

// EngineRuntimeSupport mirrors C FreeusdEngineRuntimeSupport.
type EngineRuntimeSupport struct {
	RecommendedMode            EngineRuntimeMode
	UsesComposedLayerStack     bool
	UsesReferences             bool
	UsesPayloads               bool
	UsesInherits               bool
	UsesSpecializes            bool
	UsesVariantSelection       bool
	UsesVariantSets            bool
	UsesRelocates              bool
	UsesPrefixSubstitutions    bool
	UsesTimeSamples            bool
	UsesRelationships          bool
	UsesCustomData             bool
	UsesAttributeConnections   bool
	UsesSkelBoundMeshes        bool
	UsesBlendShapes            bool
	UsesSkelAnimation          bool
	UsesMaterialBindings       bool
	UsesPreviewSurface         bool
	UsesPreviewSurfaceTextures bool
	UsesLuxLights              bool
	UsesComposedPrimKind       bool
	UsesPrimActiveOpinions     bool
	UsesKindActiveThroughArcs  bool
	UsesCustomDataThroughArcs  bool
	UsesPhysicsScenes          bool
	UsesRigidBodyAPI           bool
	UsesCollisionAPI           bool
	UsesPhysicsFixedJoints     bool
	UsesOpenVdbAssets          bool
	UsesVolumes                bool
}

// AssessEngineRuntimeSupport reports which engine integration mode fits the composed stage.
func (s *Stage) AssessEngineRuntimeSupport() (report EngineRuntimeSupport, rc int) {
	if s == nil || s.ptr == nil {
		return report, 1
	}
	var raw C.FreeusdEngineRuntimeSupport
	rc = int(C.freeusd_usdutils_assess_engine_runtime_support(s.ptr, &raw))
	if rc != 0 {
		return report, rc
	}
	report.RecommendedMode = EngineRuntimeMode(raw.recommended_mode)
	report.UsesComposedLayerStack = raw.uses_composed_layer_stack != 0
	report.UsesReferences = raw.uses_references != 0
	report.UsesPayloads = raw.uses_payloads != 0
	report.UsesInherits = raw.uses_inherits != 0
	report.UsesSpecializes = raw.uses_specializes != 0
	report.UsesVariantSelection = raw.uses_variant_selection != 0
	report.UsesVariantSets = raw.uses_variant_sets != 0
	report.UsesRelocates = raw.uses_relocates != 0
	report.UsesPrefixSubstitutions = raw.uses_prefix_substitutions != 0
	report.UsesTimeSamples = raw.uses_time_samples != 0
	report.UsesRelationships = raw.uses_relationships != 0
	report.UsesCustomData = raw.uses_custom_data != 0
	report.UsesAttributeConnections = raw.uses_attribute_connections != 0
	report.UsesSkelBoundMeshes = raw.uses_skel_bound_meshes != 0
	report.UsesBlendShapes = raw.uses_blend_shapes != 0
	report.UsesSkelAnimation = raw.uses_skel_animation != 0
	report.UsesMaterialBindings = raw.uses_material_bindings != 0
	report.UsesPreviewSurface = raw.uses_preview_surface != 0
	report.UsesPreviewSurfaceTextures = raw.uses_preview_surface_textures != 0
	report.UsesLuxLights = raw.uses_lux_lights != 0
	report.UsesComposedPrimKind = raw.uses_composed_prim_kind != 0
	report.UsesPrimActiveOpinions = raw.uses_prim_active_opinions != 0
	report.UsesKindActiveThroughArcs = raw.uses_kind_active_through_arcs != 0
	report.UsesCustomDataThroughArcs = raw.uses_custom_data_through_arcs != 0
	report.UsesPhysicsScenes = raw.uses_physics_scenes != 0
	report.UsesRigidBodyAPI = raw.uses_rigid_body_api != 0
	report.UsesCollisionAPI = raw.uses_collision_api != 0
	report.UsesPhysicsFixedJoints = raw.uses_physics_fixed_joints != 0
	report.UsesOpenVdbAssets = raw.uses_open_vdb_assets != 0
	report.UsesVolumes = raw.uses_volumes != 0
	return report, 0
}

// ReadSkelJointNames returns skeleton joint names (rc 0 ok).
func (s *Stage) ReadSkelJointNames(skeletonPath string) (names []string, rc int) {
	if s == nil || s.ptr == nil {
		return nil, 1
	}
	sp := C.CString(skeletonPath)
	defer C.free(unsafe.Pointer(sp))
	var arr **C.char
	var n C.size_t
	rc = int(C.freeusd_stage_read_skel_joint_names(s.ptr, sp, &arr, &n))
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
			names = append(names, C.GoString(p))
		}
	}
	return names, 0
}

// DeformPointsWithSkeleton runs CPU LBS using skeleton bind transforms and animation TRS at time.
func (s *Stage) DeformPointsWithSkeleton(skeletonPath, animationPath string, time float64, points [][3]float32, indices []int32, weights []float32, influencesPerPoint int) (out [][3]float32, rc int) {
	if s == nil || s.ptr == nil || len(points) == 0 {
		return nil, 1
	}
	sp := C.CString(skeletonPath)
	defer C.free(unsafe.Pointer(sp))
	ap := C.CString(animationPath)
	defer C.free(unsafe.Pointer(ap))
	inXYZ := make([]C.float, len(points)*3)
	outXYZ := make([]C.float, len(points)*3)
	for i, p := range points {
		inXYZ[i*3+0] = C.float(p[0])
		inXYZ[i*3+1] = C.float(p[1])
		inXYZ[i*3+2] = C.float(p[2])
	}
	idx := make([]C.int, len(indices))
	for i, v := range indices {
		idx[i] = C.int(v)
	}
	wts := make([]C.float, len(weights))
	for i, v := range weights {
		wts[i] = C.float(v)
	}
	rc = int(C.freeusd_stage_deform_points_with_skeleton(
		s.ptr, sp, ap, C.double(time), C.size_t(len(points)),
		(*C.float)(unsafe.Pointer(&inXYZ[0])),
		(*C.int)(unsafe.Pointer(&idx[0])),
		(*C.float)(unsafe.Pointer(&wts[0])),
		C.size_t(influencesPerPoint),
		(*C.float)(unsafe.Pointer(&outXYZ[0])),
	))
	if rc != 0 {
		return nil, rc
	}
	out = make([][3]float32, len(points))
	for i := range out {
		out[i] = [3]float32{float32(outXYZ[i*3+0]), float32(outXYZ[i*3+1]), float32(outXYZ[i*3+2])}
	}
	return out, 0
}

// ComputeSkinningMatrices builds joint palette matrices (row-major 16 doubles each).
func ComputeSkinningMatrices(jointWorld, inverseBind [][16]float64) (palette [][16]float64, rc int) {
	if len(jointWorld) == 0 || len(jointWorld) != len(inverseBind) {
		return nil, 1
	}
	n := len(jointWorld)
	worldFlat := make([]C.double, n*16)
	bindFlat := make([]C.double, n*16)
	outFlat := make([]C.double, n*16)
	for i := 0; i < n; i++ {
		for k := 0; k < 16; k++ {
			worldFlat[i*16+k] = C.double(jointWorld[i][k])
			bindFlat[i*16+k] = C.double(inverseBind[i][k])
		}
	}
	rc = int(C.freeusd_usdskel_compute_skinning_matrices(
		C.size_t(n),
		(*C.double)(unsafe.Pointer(&worldFlat[0])),
		(*C.double)(unsafe.Pointer(&bindFlat[0])),
		(*C.double)(unsafe.Pointer(&outFlat[0])),
	))
	if rc != 0 {
		return nil, rc
	}
	palette = make([][16]float64, n)
	for i := 0; i < n; i++ {
		for k := 0; k < 16; k++ {
			palette[i][k] = float64(outFlat[i*16+k])
		}
	}
	return palette, 0
}
