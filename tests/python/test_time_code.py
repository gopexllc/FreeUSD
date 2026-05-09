from __future__ import annotations

import freeusd


def test_time_code_sentinels() -> None:
    d = freeusd.usd.TimeCode.Default()
    e = freeusd.usd.TimeCode.EarliestTime()
    assert d.is_default() and not d.is_numeric()
    assert e.is_earliest_time()
    assert d == freeusd.usd.TimeCode.Default()


def test_time_code_numeric() -> None:
    t = freeusd.usd.TimeCode.FromDouble(2.25)
    assert t.is_numeric() and t.value() == 2.25


def test_usd_utils_flatten_options() -> None:
    opt = freeusd.usdUtils.FlattenOptions()
    assert opt.merge_authored_layer_metadata
    opt.merge_authored_layer_metadata = False
    assert not opt.merge_authored_layer_metadata
