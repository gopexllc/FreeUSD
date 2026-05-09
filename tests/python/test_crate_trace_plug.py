from __future__ import annotations

import tempfile
from pathlib import Path


def test_usd_crate_detect_ascii_and_magic() -> None:
    from freeusd.usd import crate

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
