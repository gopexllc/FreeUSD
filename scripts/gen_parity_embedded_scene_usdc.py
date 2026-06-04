#!/usr/bin/env python3
"""Regenerate tests/fixtures/parity_embedded_scene.usdc (embedded USDA section, clean-room)."""

from __future__ import annotations

from pathlib import Path

from gen_parity_tables_usdc import BOOT_SIZE, TOC_RECORD, le_u64, toc_name  # type: ignore

USDA_TEXT = b"""#usda 1.0
(
    defaultPrim = "World"
)

def Xform "World"
{
    def Cube "Cube"
    {
        double size = 2.0
    }
}
"""


def main() -> None:
    usda_payload = USDA_TEXT
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
    out = Path(__file__).resolve().parents[1] / "tests" / "fixtures" / "parity_embedded_scene.usdc"
    out.write_bytes(bytes(boot) + bytes(toc) + bytes(payloads))
    print(f"wrote {out} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
