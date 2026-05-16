#!/usr/bin/env python3
"""
Regenerate FreeUSD schema token headers from published OpenUSD generatedSchema.usda files.

These USDA files are machine-generated schema *data* (property names, prim names), not C++
implementation. We extract literal identifiers and emit clean-room Token() accessors.

Source: Pixar OpenUSD repository `dev` branch (see OPENUSD_SCHEMA_REF below).
Run from repo root:  python3 scripts/gen_schema_tokens.py

Also emits **`include/freeusd/usd/schemaDataTokens.hpp`** + **`python/generated/usd_schema_data_tokens.inc`**
from **`pxr/usd/usd/generatedSchema.usda`** (core API schema strings), separate from hand-authored **`usd/tokens.hpp`**.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

OPENUSD_SCHEMA_REF = "https://raw.githubusercontent.com/PixarAnimationStudios/OpenUSD/dev/pxr/usd"

# (subdir under include/freeusd, OpenUSD pxr/usd subfolder)
LIBS = [
    ("usdGeom", "usdGeom"),
    ("usdShade", "usdShade"),
    ("usdLux", "usdLux"),
    ("usdPhysics", "usdPhysics"),
    ("usdVol", "usdVol"),
    ("usdSkel", "usdSkel"),
    ("usdMedia", "usdMedia"),
    ("usdRender", "usdRender"),
    ("usdRi", "usdRi"),
    ("usdHydra", "usdHydra"),
    ("usdMtlx", "usdMtlx"),
    ("usdProc", "usdProc"),
    ("usdSemantics", "usdSemantics"),
    ("usdUI", "usdUI"),
]

CXX_KEYWORDS = {
    "alignas",
    "alignof",
    "and",
    "and_eq",
    "asm",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "class",
    "compl",
    "concept",
    "const",
    "constexpr",
    "continue",
    "decltype",
    "default",
    "delete",
    "do",
    "double",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "template",
    "this",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",
}

DISCARD_NAMES = {"userDocBrief"}

PROP_RE = re.compile(
    r"^\s+(?:uniform\s+)?(?:rel|token|token\[\]|string|string\[\]|bool|int|int\[\]|int64|uint|uint64|float|float\[\]|"
    r"float2|float3|float4|double|double2|double3|double4|matrix4d|quatd|quatf|frame4d|"
    r"point3f|point3h|point3d|normal3f|normal3d|vector3f|vector3d|color3f|color3d|color4f|"
    r"texCoord2f|texCoord3f|matrix3d|matrix2d|half|half2|half3|half4|asset|opaque)\s*(?:\[\])?\s+([\w:]+)",
    re.M,
)


def parse_schema(text: str) -> list[str]:
    names: set[str] = set()
    for line in text.splitlines():
        m = re.match(r'class\s+"([^"]+)"\s*\(', line)
        if m:
            names.add(m.group(1))
            continue
        m = re.match(r'class\s+(\w+)\s+"([^"]+)"\s*\(', line)
        if m:
            names.add(m.group(2))
            continue
        m = re.match(r"class\s+(\w+)\s*\(", line)
        if m:
            names.add(m.group(1))
            continue
    for m in PROP_RE.finditer(text):
        names.add(m.group(1))
    names -= DISCARD_NAMES
    out = sorted(names)
    return out


def cpp_ident(name: str) -> str:
    # Namespaced USD names (e.g. ui:nodegraph:node:pos) are valid token strings but not C++ identifiers.
    name = name.replace(":", "_")
    if name in CXX_KEYWORDS:
        return name + "_"
    if name and name[0].isdigit():
        return "_" + name
    return name


def emit_header(ns_cpp: str, names: list[str]) -> str:
    lines = [
        "#pragma once",
        "",
        "// GENERATED FILE - do not edit by hand. Regenerate with: python3 scripts/gen_schema_tokens.py",
        f"// Token strings derived from OpenUSD published schema data ({OPENUSD_SCHEMA_REF}/.../generatedSchema.usda).",
        "",
        '#include "freeusd/tf/token.hpp"',
        "",
        f"namespace {ns_cpp} {{",
        "",
        "/// Schema tokens (OpenUSD-shaped names; clean-room).",
    ]
    for n in names:
        ident = cpp_ident(n)
        lines.append(f"inline freeusd::tf::Token {ident}() {{ return freeusd::tf::Token(\"{n}\"); }}")
    lines.append("")
    lines.append(f"}}  // namespace {ns_cpp}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    import urllib.request

    for inc_dir, usd_dir in LIBS:
        url = f"{OPENUSD_SCHEMA_REF}/{usd_dir}/generatedSchema.usda"
        print("fetch", url, file=sys.stderr)
        with urllib.request.urlopen(url, timeout=60) as r:
            text = r.read().decode("utf-8", errors="replace")
        names = parse_schema(text)
        if not names:
            print(f"warn: no names for {inc_dir}", file=sys.stderr)
            continue

        ns = f"freeusd::{inc_dir}::tokens"
        header_path = root / "include" / "freeusd" / inc_dir / "tokens.hpp"
        header_path.parent.mkdir(parents=True, exist_ok=True)
        header_path.write_text(emit_header(ns, names), encoding="utf-8")
        print("wrote", header_path, len(names), "tokens", file=sys.stderr)

        py_path = root / "python" / "generated" / f"{inc_dir}_tokens.inc"
        py_path.parent.mkdir(parents=True, exist_ok=True)
        py_path.write_text(
            emit_pyinc_for_bindings(inc_dir, names),
            encoding="utf-8",
        )
        print("wrote", py_path, file=sys.stderr)

    # `pxr/usd/usd/generatedSchema.usda`: core USD API schema names (distinct from Sdf composition
    # field tokens in `include/freeusd/usd/tokens.hpp`, which remain hand-authored).
    url = f"{OPENUSD_SCHEMA_REF}/usd/generatedSchema.usda"
    print("fetch", url, file=sys.stderr)
    with urllib.request.urlopen(url, timeout=60) as r:
        text = r.read().decode("utf-8", errors="replace")
    names = parse_schema(text)
    if not names:
        print("warn: no names for pxr/usd/usd generatedSchema", file=sys.stderr)
    else:
        ns = "freeusd::usd::schemaDataTokens"
        header_path = root / "include" / "freeusd" / "usd" / "schemaDataTokens.hpp"
        header_path.parent.mkdir(parents=True, exist_ok=True)
        header_path.write_text(emit_header(ns, names), encoding="utf-8")
        print("wrote", header_path, len(names), "tokens", file=sys.stderr)
        py_path = root / "python" / "generated" / "usd_schema_data_tokens.inc"
        py_path.parent.mkdir(parents=True, exist_ok=True)
        py_path.write_text(
            emit_pyinc_lines("usd_schema_data_tokens", ns, names, "usd.schema_data_tokens"),
            encoding="utf-8",
        )
        print("wrote", py_path, file=sys.stderr)
    return 0


def emit_pyinc_lines(pybind_var: str, cpp_ns: str, names: list[str], label: str) -> str:
    """C++ lines registering each token on a pybind submodule handle `pybind_var`."""
    lines = [f"// GENERATED - included from bindings.cpp for {label}"]
    for n in names:
        ident = cpp_ident(n)
        lines.append(f'{pybind_var}.def("{n}", []{{ return {cpp_ns}::{ident}(); }});')
    return "\n".join(lines) + "\n"


def emit_pyinc_for_bindings(inc_dir: str, names: list[str]) -> str:
    """C++ lines that register each token on the correct pybind submodule variable."""
    var_map = {
        "usdGeom": "geom_tokens",
        "usdShade": "shade_tokens",
        "usdLux": "lux_tokens",
        "usdPhysics": "physics_tokens",
        "usdVol": "vol_tokens",
        "usdSkel": "skel_tokens",
        "usdMedia": "media_tokens",
        "usdRender": "render_tokens",
        "usdRi": "ri_tokens",
        "usdHydra": "hydra_tokens",
        "usdMtlx": "mtlx_tokens",
        "usdProc": "proc_tokens",
        "usdSemantics": "semantics_tokens",
        "usdUI": "ui_tokens",
    }
    v = var_map[inc_dir]
    cpp_ns_base = f"freeusd::{inc_dir}::tokens"
    return emit_pyinc_lines(v, cpp_ns_base, names, inc_dir)


if __name__ == "__main__":
    raise SystemExit(main())
