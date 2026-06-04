#!/usr/bin/env python3
"""Regenerate tests/fixtures/parity_tables_zlib.usdc (VALUES zlib-wrapped, clean-room)."""

from __future__ import annotations

import struct
import zlib
from pathlib import Path

from gen_parity_tables_usdc import (  # type: ignore
    BOOT_SIZE,
    TOC_RECORD,
    fieldsets_table_payload,
    fields_table_payload,
    le_u64,
    specs_table_payload,
    string_table_payload,
    toc_name,
    typed_values_table_payload,
    VALUE_KIND_BOOL,
    VALUE_KIND_DOUBLE,
    VALUE_KIND_FLOAT,
    VALUE_KIND_FLOAT_ARRAY,
    VALUE_KIND_INT32,
    VALUE_KIND_INT32_ARRAY,
    VALUE_KIND_INT64,
    VALUE_KIND_STRING_INDEX,
    VALUE_KIND_STRING_UTF8,
    VALUE_KIND_TOKEN_INDEX,
    VALUE_KIND_VEC3D,
    VALUE_KIND_VEC3F,
)

FUSDZC_MAGIC = b"FUSDZC\x01"


def wrap_fixture_zlib(uncompressed: bytes) -> bytes:
    return FUSDZC_MAGIC + le_u64(len(uncompressed)) + zlib.compress(uncompressed, level=9)


def values_entries() -> list[tuple[int, bytes]]:
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
    ]


def build_crate(values_payload: bytes) -> bytes:
    sections: list[tuple[str, bytes]] = [
        ("TOKENS", string_table_payload(["render", "invisible"])),
        ("STRINGS", string_table_payload(["hello", "world"])),
        ("PATHS", string_table_payload(["/World", "/World/Cube"])),
        ("FIELDS", fields_table_payload([(0, 1), (1, 0)])),
        ("FIELDSETS", fieldsets_table_payload([[0, 1], [1]])),
        ("SPECS", specs_table_payload([(0, 0, 1), (1, 1, 2)])),
        ("VALUES", values_payload),
    ]
    toc_offset = BOOT_SIZE
    payload_offset = toc_offset + 8 + len(sections) * TOC_RECORD
    boot = bytearray(BOOT_SIZE)
    boot[0:8] = b"PXR-USDC"
    boot[8:11] = bytes([0, 8, 0])
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
    plain = typed_values_table_payload(values_entries())
    wrapped = wrap_fixture_zlib(plain)
    out = Path(__file__).resolve().parents[1] / "tests" / "fixtures" / "parity_tables_zlib.usdc"
    out.write_bytes(build_crate(wrapped))
    print(f"wrote {out} ({out.stat().st_size} bytes, VALUES zlib {len(wrapped)} bytes)")


if __name__ == "__main__":
    main()
