#!/usr/bin/env python3
"""Regenerate tests/fixtures/parity_tables.usdc (clean-room synthetic crate bytes)."""

from __future__ import annotations

import struct
from pathlib import Path

MAGIC = b"PXR-USDC"
BOOT_SIZE = 88
TOC_RECORD = 32


def le_u64(value: int) -> bytes:
    return struct.pack("<Q", value)


def toc_name(name: str) -> bytes:
    raw = name.encode("ascii")
    if len(raw) > 16:
        raise ValueError(f"section name too long: {name}")
    return raw + b"\0" * (16 - len(raw))


def string_table_payload(strings: list[str]) -> bytes:
    out = bytearray(le_u64(len(strings)))
    for s in strings:
        b = s.encode("utf-8")
        out += le_u64(len(b))
        out += b
    return bytes(out)


def fields_table_payload(pairs: list[tuple[int, int]]) -> bytes:
    out = bytearray(le_u64(len(pairs)))
    for token_index, value_type_index in pairs:
        out += le_u64(token_index)
        out += le_u64(value_type_index)
    return bytes(out)


def specs_table_payload(rows: list[tuple[int, int, int]]) -> bytes:
    out = bytearray(le_u64(len(rows)))
    for path_index, field_set_index, spec_type in rows:
        out += le_u64(path_index)
        out += le_u64(field_set_index)
        out += le_u64(spec_type)
    return bytes(out)


VALUE_KIND_OPAQUE = 0
VALUE_KIND_INT32 = 1
VALUE_KIND_FLOAT = 2
VALUE_KIND_TOKEN_INDEX = 3
VALUE_KIND_BOOL = 4
VALUE_KIND_DOUBLE = 5
VALUE_KIND_INT64 = 6
VALUE_KIND_STRING_UTF8 = 7
VALUE_KIND_VEC3F = 8
VALUE_KIND_STRING_INDEX = 9
VALUE_KIND_VEC3D = 10
VALUE_KIND_INT32_ARRAY = 11
VALUE_KIND_FLOAT_ARRAY = 12
VALUE_KIND_DOUBLE_ARRAY = 13
VALUE_KIND_VEC2F = 14
VALUE_KIND_VEC4F = 15
VALUE_KIND_VEC2D = 16
VALUE_KIND_QUATF = 17
VALUE_KIND_QUATD = 18


def typed_values_table_payload(entries: list[tuple[int, bytes]]) -> bytes:
    out = bytearray(le_u64(len(entries)))
    for kind, data in entries:
        out += le_u64(kind)
        out += le_u64(len(data))
        out += data
    return bytes(out)


def fieldsets_table_payload(sets: list[list[int]]) -> bytes:
    out = bytearray(le_u64(len(sets)))
    for indices in sets:
        out += le_u64(len(indices))
        for index in indices:
            out += le_u64(index)
    return bytes(out)


def parity_values_entries():
    import struct
    return [
        (VALUE_KIND_INT32, struct.pack("<i", 42)),
        (VALUE_KIND_FLOAT, struct.pack("<f", 1.5)),
        (VALUE_KIND_TOKEN_INDEX, le_u64(0)),
        (VALUE_KIND_BOOL, b"\x01"),
        (VALUE_KIND_DOUBLE, struct.pack("<d", 3.25)),
        (VALUE_KIND_INT64, struct.pack("<q", -9007199254740991)),
        (VALUE_KIND_STRING_UTF8, b"parity"),
        (VALUE_KIND_VEC3F, struct.pack("<fff", 1.0, 2.0, 3.0)),
        (VALUE_KIND_STRING_INDEX, le_u64(1)),
        (VALUE_KIND_VEC3D, struct.pack("<ddd", 4.0, 5.0, 6.0)),
        (VALUE_KIND_INT32_ARRAY, le_u64(3) + struct.pack("<iii", 7, 8, 9)),
        (VALUE_KIND_FLOAT_ARRAY, le_u64(2) + struct.pack("<ff", 0.25, 0.75)),
        (VALUE_KIND_DOUBLE_ARRAY, le_u64(2) + struct.pack("<dd", 1.0, 2.0)),
        (VALUE_KIND_VEC2F, struct.pack("<ff", 0.5, 1.25)),
        (VALUE_KIND_VEC4F, struct.pack("<ffff", 1.0, 2.0, 3.0, 4.0)),
        (VALUE_KIND_VEC2D, struct.pack("<dd", 0.5, 1.75)),
        (VALUE_KIND_QUATF, struct.pack("<ffff", 1.0, 0.5, 0.25, 0.125)),
        (VALUE_KIND_QUATD, struct.pack("<dddd", 1.0, 0.5, 0.25, 0.125)),
    ]


def build_crate() -> bytes:
    sections: list[tuple[str, bytes]] = [
        ("TOKENS", string_table_payload(["render", "invisible"])),
        ("STRINGS", string_table_payload(["hello", "world"])),
        ("PATHS", string_table_payload(["/World", "/World/Cube"])),
        ("FIELDS", fields_table_payload([(0, 1), (1, 0)])),
        ("FIELDSETS", fieldsets_table_payload([[0, 1], [1]])),
        ("SPECS", specs_table_payload([(0, 0, 1), (1, 1, 2)])),
        (
            "VALUES",
            typed_values_table_payload(parity_values_entries()),
        ),
    ]

    toc_offset = BOOT_SIZE
    payload_offset = toc_offset + 8 + len(sections) * TOC_RECORD

    boot = bytearray(BOOT_SIZE)
    boot[0:8] = MAGIC
    boot[8:11] = bytes([0, 8, 0])  # 0.8.0
    boot[16:24] = le_u64(toc_offset)

    toc = bytearray()
    toc += le_u64(len(sections))
    cursor = payload_offset
    payloads = bytearray()
    for name, payload in sections:
        toc += toc_name(name)
        toc += le_u64(cursor)
        toc += le_u64(len(payload))
        payloads += payload
        cursor += len(payload)

    return bytes(boot) + bytes(toc) + bytes(payloads)


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    out = repo / "tests" / "fixtures" / "parity_tables.usdc"
    out.write_bytes(build_crate())
    print(f"wrote {out} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
