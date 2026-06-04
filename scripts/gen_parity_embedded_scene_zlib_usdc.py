#!/usr/bin/env python3
"""Regenerate tests/fixtures/parity_embedded_scene_zlib.usdc (zlib-wrapped USDA section)."""

from __future__ import annotations

import zlib
from pathlib import Path

from gen_parity_embedded_scene_usdc import USDA_TEXT  # type: ignore
from gen_parity_tables_usdc import BOOT_SIZE, TOC_RECORD, le_u64, toc_name  # type: ignore

FUSDZC_MAGIC = b"FUSDZC\x01"


def wrap_fixture_zlib(uncompressed: bytes) -> bytes:
    return FUSDZC_MAGIC + le_u64(len(uncompressed)) + zlib.compress(uncompressed, level=9)


def main() -> None:
    usda_payload = wrap_fixture_zlib(USDA_TEXT)
    sections = [("USDA", usda_payload)]
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
    out = Path(__file__).resolve().parents[1] / "tests" / "fixtures" / "parity_embedded_scene_zlib.usdc"
    out.write_bytes(bytes(boot) + bytes(toc) + bytes(payloads))
    print(f"wrote {out} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
