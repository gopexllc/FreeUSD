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

// Stage wraps FreeusdStage (attach root layer).
type Stage struct {
	ptr *C.FreeusdStage
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
