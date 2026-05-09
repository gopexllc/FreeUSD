from __future__ import annotations

import freeusd


def test_resolved_path_construct() -> None:
    r = freeusd.ar.ResolvedPath("/abs/a.usda")
    assert r.path() == "/abs/a.usda"
    assert not r.is_empty()


def test_default_resolver_resolve_path() -> None:
    res = freeusd.ar.DefaultResolver("/tmp")
    rp = res.resolve_path("./scene.usda")
    assert isinstance(rp, freeusd.ar.ResolvedPath)
    assert not rp.is_empty()
