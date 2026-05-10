from __future__ import annotations

import struct
import tempfile
from pathlib import Path


def test_usd_crate_detect_ascii_and_magic() -> None:
    from freeusd.usd import crate

    assert crate.usdc_crate_identifier() == "PXR-USDC"

    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "a.usda"
        p.write_text('#usda 1.0\n(\n)\ndef X "x" {}\n', encoding="utf-8")
        kind, detail = crate.detect_usd_file_kind_from_path(str(p))
        assert kind == crate.UsdFileKind.usda_ascii
        assert not detail

        q = Path(td) / "b.usdc"
        q.write_bytes(b"PXR-USDC\x00")
        kind2, _ = crate.detect_usd_file_kind_from_path(str(q))
        assert kind2 == crate.UsdFileKind.usdc_crate


def test_read_usdc_bootstrap_from_path() -> None:
    from freeusd.usd import crate

    with tempfile.TemporaryDirectory() as td:
        boot_path = Path(td) / "boot.usdc"
        buf = bytearray(128)
        buf[0:8] = b"PXR-USDC"
        buf[8] = 0
        buf[9] = 8
        buf[10] = 0
        buf[16:24] = struct.pack("<q", 88)
        boot_path.write_bytes(buf)

        ok, info, err = crate.read_usdc_bootstrap_from_path(str(boot_path))
        assert ok
        assert not err
        assert info is not None
        assert info["file_version_major"] == 0
        assert info["file_version_minor"] == 8
        assert info["file_version_patch"] == 0
        assert info["toc_byte_offset"] == 88

        ok2, _info2, err2 = crate.read_usdc_bootstrap_from_path(str(boot_path) + ".nope")
        assert not ok2
        assert err2

        short = Path(td) / "short.usdc"
        short.write_bytes(b"PXR-USDC\x00")
        ok3, _info3, err3 = crate.read_usdc_bootstrap_from_path(str(short))
        assert not ok3
        assert err3


def test_trace_stack_depth() -> None:
    import freeusd.trace as tr

    tr.reset()
    assert tr.stack_depth() == 0
    tr.push("a")
    assert tr.stack_depth() == 1
    tr.pop()
    assert tr.stack_depth() == 0


def test_plug_registered_paths() -> None:
    import freeusd.plug as plug

    plug.register_plugin_paths(["/tmp/freeusd_test_plugin_path"])
    assert "/tmp/freeusd_test_plugin_path" in plug.registered_plugin_paths()
