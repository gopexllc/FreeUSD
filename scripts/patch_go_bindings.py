#!/usr/bin/env python3
"""Patch bindings/go/freeusd.go for Go 1.22+ CGO and Vec4f parity (run from repo root)."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    dest = root / "bindings/go/freeusd.go"
    text = subprocess.check_output(["git", "show", "main:bindings/go/freeusd.go"], text=True, cwd=root)

    old_list = """func readUsdcStringList(path string, maxEntries uint64, maxTotalBytes uint64,
\tcall func(*C.char, C.uint64_t, C.uint64_t, ***C.char, *C.size_t) C.int) ([]string, int) {
\tcs := C.CString(path)
\tdefer C.free(unsafe.Pointer(cs))
\tvar raw **C.char
\tvar n C.size_t
\trc := int(call(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n))
\tif rc != 0 {
\t\treturn nil, rc
\t}
\tif raw == nil || n == 0 {
\t\treturn []string{}, 0
\t}
\tdefer C.freeusd_path_list_free(raw, n)
\tcount := int(n)
\tout := make([]string, count)
\tbase := unsafe.Pointer(raw)
\tstep := unsafe.Sizeof(raw)
\tfor i := 0; i < count; i++ {
\t\tptr := *(**C.char)(unsafe.Add(base, uintptr(i)*step))
\t\tout[i] = C.GoString(ptr)
\t}
\treturn out, 0
}

// ReadUsdcTokenTableFromPath reads the validated TOKENS table from a shared crate fixture.
func ReadUsdcTokenTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
\treturn readUsdcStringList(path, maxEntries, maxTotalBytes, C.freeusd_read_usdc_token_table_from_path_utf8)
}

// ReadUsdcStringTableFromPath reads the validated STRINGS table from a shared crate fixture.
func ReadUsdcStringTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
\treturn readUsdcStringList(path, maxEntries, maxTotalBytes, C.freeusd_read_usdc_string_table_from_path_utf8)
}

// ReadUsdcPathTableFromPath reads the validated PATHS table from a shared crate fixture.
func ReadUsdcPathTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
\treturn readUsdcStringList(path, maxEntries, maxTotalBytes, C.freeusd_read_usdc_path_table_from_path_utf8)
}"""

    new_list = """func readUsdcCStringTable(path string, maxEntries uint64, maxTotalBytes uint64, tableKind int) ([]string, int) {
\tcs := C.CString(path)
\tdefer C.free(unsafe.Pointer(cs))
\tvar raw **C.char
\tvar n C.size_t
\tvar rc C.int
\tswitch tableKind {
\tcase 0:
\t\trc = C.freeusd_read_usdc_token_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n)
\tcase 1:
\t\trc = C.freeusd_read_usdc_string_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n)
\tdefault:
\t\trc = C.freeusd_read_usdc_path_table_from_path_utf8(cs, C.uint64_t(maxEntries), C.uint64_t(maxTotalBytes), &raw, &n)
\t}
\tif int(rc) != 0 {
\t\treturn nil, int(rc)
\t}
\tif raw == nil || n == 0 {
\t\treturn []string{}, 0
\t}
\tdefer C.freeusd_path_list_free(raw, n)
\tcount := int(n)
\tout := make([]string, count)
\tbase := unsafe.Pointer(raw)
\tstep := unsafe.Sizeof(raw)
\tfor i := 0; i < count; i++ {
\t\tptr := *(**C.char)(unsafe.Add(base, uintptr(i)*step))
\t\tout[i] = C.GoString(ptr)
\t}
\treturn out, 0
}

// ReadUsdcTokenTableFromPath reads the validated TOKENS table from a shared crate fixture.
func ReadUsdcTokenTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
\treturn readUsdcCStringTable(path, maxEntries, maxTotalBytes, 0)
}

// ReadUsdcStringTableFromPath reads the validated STRINGS table from a shared crate fixture.
func ReadUsdcStringTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
\treturn readUsdcCStringTable(path, maxEntries, maxTotalBytes, 1)
}

// ReadUsdcPathTableFromPath reads the validated PATHS table from a shared crate fixture.
func ReadUsdcPathTableFromPath(path string, maxEntries uint64, maxTotalBytes uint64) ([]string, int) {
\treturn readUsdcCStringTable(path, maxEntries, maxTotalBytes, 2)
}"""

    if old_list not in text:
        print("readUsdcStringList block not found", file=sys.stderr)
        return 1
    text = text.replace(old_list, new_list)

    text = text.replace(
        "Vec3dValue: [3]float64{entry.vec3d_value[0], entry.vec3d_value[1], entry.vec3d_value[2]},",
        "Vec3dValue: [3]float64{float64(entry.vec3d_value[0]), float64(entry.vec3d_value[1]), float64(entry.vec3d_value[2])},",
    )
    text = text.replace(
        "\tFloatArray     []float32\n}",
        "\tFloatArray     []float32\n\tDoubleArray    []float64\n\tVec2fValue     [2]float32\n\tVec4fValue     [4]float32\n}",
    )

    marker = """\t\t\tFloatArray: func() []float32 {
\t\t\t\tn := int(entry.float_array_count)
\t\t\t\tif n == 0 || entry.float_array == nil {
\t\t\t\t\treturn nil
\t\t\t\t}
\t\t\t\tout := make([]float32, n)
\t\t\t\tfor j := 0; j < n; j++ {
\t\t\t\t\tv := *(*float32)(unsafe.Pointer(uintptr(unsafe.Pointer(entry.float_array)) + uintptr(j)*unsafe.Sizeof(float32(0))))
\t\t\t\t\tout[j] = v
\t\t\t\t}
\t\t\t\treturn out
\t\t\t}(),
\t\t}"""
    extra = """\t\t\tFloatArray: func() []float32 {
\t\t\t\tn := int(entry.float_array_count)
\t\t\t\tif n == 0 || entry.float_array == nil {
\t\t\t\t\treturn nil
\t\t\t\t}
\t\t\t\tout := make([]float32, n)
\t\t\t\tfor j := 0; j < n; j++ {
\t\t\t\t\tv := *(*float32)(unsafe.Pointer(uintptr(unsafe.Pointer(entry.float_array)) + uintptr(j)*unsafe.Sizeof(float32(0))))
\t\t\t\t\tout[j] = v
\t\t\t\t}
\t\t\t\treturn out
\t\t\t}(),
\t\t\tDoubleArray: func() []float64 {
\t\t\t\tn := int(entry.double_array_count)
\t\t\t\tif n == 0 || entry.double_array == nil {
\t\t\t\t\treturn nil
\t\t\t\t}
\t\t\t\tout := make([]float64, n)
\t\t\t\tfor j := 0; j < n; j++ {
\t\t\t\t\tv := *(*float64)(unsafe.Pointer(uintptr(unsafe.Pointer(entry.double_array)) + uintptr(j)*unsafe.Sizeof(float64(0))))
\t\t\t\t\tout[j] = v
\t\t\t\t}
\t\t\t\treturn out
\t\t\t}(),
\t\t\tVec2fValue: [2]float32{float32(entry.vec2f_value[0]), float32(entry.vec2f_value[1])},
\t\t\tVec4fValue: [4]float32{float32(entry.vec4f_value[0]), float32(entry.vec4f_value[1]), float32(entry.vec4f_value[2]), float32(entry.vec4f_value[3])},
\t\t}"""
    if marker not in text:
        print("FloatArray marker not found", file=sys.stderr)
        return 1
    text = text.replace(marker, extra, 1)

    m = re.search(
        r"func \(s \*Stage\) listPrimPathStrings\(primPath string, listFn func\(\*C\.FreeusdStage.*?\n\}\n\n// ListPrimReferences",
        text,
        re.S,
    )
    if not m:
        print("listPrimPathStrings not found", file=sys.stderr)
        return 1
    new_prim = """func (s *Stage) listPrimPathStrings(primPath string, kind primPathListKind) ([]string, int) {
\tif s == nil || s.ptr == nil {
\t\treturn nil, 1
\t}
\tpp := C.CString(primPath)
\tdefer C.free(unsafe.Pointer(pp))
\tvar arr **C.char
\tvar n C.size_t
\tvar rc C.int
\tswitch kind {
\tcase primPathListReferences:
\t\trc = C.freeusd_stage_list_prim_references(s.ptr, pp, &arr, &n)
\tcase primPathListInherits:
\t\trc = C.freeusd_stage_list_prim_inherits(s.ptr, pp, &arr, &n)
\tcase primPathListSpecializes:
\t\trc = C.freeusd_stage_list_prim_specializes(s.ptr, pp, &arr, &n)
\tdefault:
\t\trc = C.freeusd_stage_list_prim_payloads(s.ptr, pp, &arr, &n)
\t}
\tif int(rc) != 0 {
\t\treturn nil, int(rc)
\t}
\tif n == 0 || arr == nil {
\t\treturn []string{}, 0
\t}
\tdefer C.freeusd_path_list_free(arr, n)
\tout := make([]string, 0, int(n))
\tslice := unsafe.Slice(arr, int(n))
\tfor _, p := range slice {
\t\tif p != nil {
\t\t\tout = append(out, C.GoString(p))
\t\t}
\t}
\treturn out, 0
}

type primPathListKind int

const (
\tprimPathListReferences primPathListKind = iota
\tprimPathListInherits
\tprimPathListSpecializes
\tprimPathListPayloads
)

// ListPrimReferences"""
    text = text.replace(m.group(0), new_prim)
    for a, b in [
        ("return s.listPrimPathStrings(primPath, C.freeusd_stage_list_prim_references)", "return s.listPrimPathStrings(primPath, primPathListReferences)"),
        ("return s.listPrimPathStrings(primPath, C.freeusd_stage_list_prim_inherits)", "return s.listPrimPathStrings(primPath, primPathListInherits)"),
        ("return s.listPrimPathStrings(primPath, C.freeusd_stage_list_prim_specializes)", "return s.listPrimPathStrings(primPath, primPathListSpecializes)"),
        ("return s.listPrimPathStrings(primPath, C.freeusd_stage_list_prim_payloads)", "return s.listPrimPathStrings(primPath, primPathListPayloads)"),
    ]:
        text = text.replace(a, b)

    linux_ld = (
        "#cgo linux LDFLAGS: -L${SRCDIR}/../../build/src "
        "-lfreeusd_c -lfreeusd_base -lfreeusd_usdUtils -lfreeusd_usdSkel -lfreeusd_usdShade "
        "-lfreeusd_usdLux -lfreeusd_usdPhysics -lfreeusd_usdVol -lfreeusd_usdGeom -lfreeusd_usd "
        "-lfreeusd_ar -lfreeusd_io -lfreeusd_pcp -lfreeusd_sdf -lfreeusd_vt -lfreeusd_tf "
        "-lfreeusd_gf -lfreeusd_plug -lstdc++ -lm -lz -llz4"
    )
    darwin_ld = linux_ld.replace("linux", "darwin").replace("-lstdc++", "-lc++")

    text = re.sub(r"#cgo linux LDFLAGS:.*", linux_ld, text, count=1)
    text = re.sub(r"#cgo darwin LDFLAGS:.*", darwin_ld, text, count=1)

    dest.write_text(text)
    print(f"wrote {dest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
