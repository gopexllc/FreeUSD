from __future__ import annotations

import freeusd


def test_vt_int64_float_token_accessors() -> None:
    i = freeusd.vt.Value.make_int64(-99)
    assert i.holds_int64() and i.as_int64() == -99
    assert not i.holds_double()
    # `GetDouble` promotes integral storage to double for reads (OpenUSD-like convenience).
    assert i.as_double() == -99.0

    f = freeusd.vt.Value.make_float(2.5)
    assert f.holds_float() and f.as_float() == 2.5

    tok = freeusd.tf.Token("scope")
    v = freeusd.vt.Value.make_token(tok)
    assert v.holds_token() and v.as_token().text() == "scope"


def test_vt_bool_and_int32() -> None:
    b = freeusd.vt.Value.make_bool(True)
    assert b.holds_bool() and b.as_bool() is True

    n = freeusd.vt.Value.make_int32(42)
    assert n.holds_int32() and n.as_int32() == 42
