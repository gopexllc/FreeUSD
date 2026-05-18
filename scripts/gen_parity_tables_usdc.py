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


def values_table_payload(blobs: list[bytes]) -> bytes:
    out = bytearray(le_u64(len(blobs)))
    for blob in blobs:
        out += le_u64(len(blob))
        out += blob
    return bytes(out)


def fieldsets_table_payload(sets: list[list[int]]) -> bytes:
    out = bytearray(le_u64(len(sets)))
    for indices in sets:
        out += le_u64(len(indices))
        for index in indices:
            out += le_u64(index)
    return bytes(out)


def build_crate() -> bytes:
    sections: list[tuple[str, bytes]] = [
        ("TOKENS", string_table_payload(["render", "invisible"])),
        ("STRINGS", string_table_payload(["hello", "world"])),
        ("PATHS", string_table_payload(["/World", "/World/Cube"])),
        ("FIELDS", fields_table_payload([(0, 1), (1, 0)])),
        ("FIELDSETS", fieldsets_table_payload([[0, 1], [1]])),
        ("SPECS", specs_table_payload([(0, 0, 1), (1, 1, 2)])),
        ("VALUES", values_table_payload([b"v0", b"v1-payload"])),
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
