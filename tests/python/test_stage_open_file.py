from __future__ import annotations

import importlib
import tempfile
from pathlib import Path

from freeusd.sdf import Path as SdfPath
from freeusd.usd import Stage

_RootLayerSublayersPolicy = importlib.import_module("freeusd._native.usd").RootLayerSublayersPolicy


def test_open_from_root_file_stacks_sublayers() -> None:
    with tempfile.TemporaryDirectory() as td:
        base = Path(td)
        (base / "sub.usda").write_text(
            '#usda 1.0\n(\n)\ndef Xform "FromSub"\n(\n)\n{\n}\n',
            encoding="utf-8",
        )
        (base / "root.usda").write_text(
            '#usda 1.0\n(\n    subLayers = [@./sub.usda@]\n)\ndef Xform "Root"\n(\n)\n{\n}\n',
            encoding="utf-8",
        )
        root_path = str(base / "root.usda")

        st_none = Stage.open_from_root_file(root_path, _RootLayerSublayersPolicy.none)
        assert st_none is not None
        assert not st_none.prim_path_in_use(SdfPath.from_string("/FromSub"))

        st = Stage.open_from_root_file(root_path, _RootLayerSublayersPolicy.depth_first)
        assert st is not None
        assert st.prim_path_in_use(SdfPath.from_string("/FromSub"))
        assert len(st.compose_layers()) >= 2
