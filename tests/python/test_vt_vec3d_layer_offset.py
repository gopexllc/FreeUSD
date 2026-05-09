from __future__ import annotations

import freeusd


def test_vt_make_vec3d_roundtrip() -> None:
    v = freeusd.vt.Value.make_vec3d(1.0, 2.0, 3.0)
    assert v.holds_vec3d()
    out = v.as_vec3d()
    assert out is not None
    assert out.x() == 1.0 and out.y() == 2.0 and out.z() == 3.0


def test_vt_make_vec3d_from_vec() -> None:
    vec = freeusd.gf.Vec3d(4.0, 5.0, 6.0)
    v = freeusd.vt.Value.make_vec3d_vec(vec)
    assert v.as_vec3d() == vec


def test_sdf_layer_offset_identity() -> None:
    lo = freeusd.sdf.LayerOffset()
    assert lo.is_identity()
    lo.offset = 1.0
    assert not lo.is_identity()
