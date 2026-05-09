from __future__ import annotations

import freeusd


def test_vec3d_roundtrip() -> None:
    v = freeusd.gf.Vec3d(1.0, 2.0, 3.0)
    assert v.x() == 1.0 and v.y() == 2.0 and v.z() == 3.0
    assert list(v.as_array()) == [1.0, 2.0, 3.0]
    z = freeusd.gf.Vec3d.Zero()
    assert z.x() == 0.0


def test_matrix4d_identity_list() -> None:
    m = freeusd.gf.Matrix4d.Identity()
    lst = m.as_list()
    assert len(lst) == 16
    assert lst[0] == 1.0 and lst[5] == 1.0 and lst[10] == 1.0 and lst[15] == 1.0


def test_bbox3d_empty_and_from_min_max() -> None:
    e = freeusd.gf.BBox3d.empty()
    assert e.is_empty()

    a = freeusd.gf.Vec3d(1.0, 2.0, 3.0)
    b = freeusd.gf.Vec3d(-1.0, 4.0, 0.0)
    box = freeusd.gf.BBox3d.from_min_max(a, b)
    assert not box.is_empty()
    assert box.min.x() == -1.0 and box.max.y() == 4.0


def test_quatd_identity() -> None:
    q = freeusd.gf.Quatd.Identity()
    assert q.real == 1.0 and q.i == 0.0 and q.j == 0.0 and q.k == 0.0
    r = freeusd.gf.Quatd(0.5, 0.5, 0.5, 0.5)
    assert r.real == 0.5


def test_range1d_empty_and_unit() -> None:
    e = freeusd.gf.Range1d()
    assert e.is_empty()

    u = freeusd.gf.Range1d.unit_interval()
    assert not u.is_empty() and u.min == 0.0 and u.max == 1.0

    r = freeusd.gf.Range1d(3.0, 3.0)
    assert not r.is_empty()
