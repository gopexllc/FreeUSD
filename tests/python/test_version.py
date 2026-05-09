import freeusd


def test_version_string_nonempty() -> None:
    assert isinstance(freeusd.version(), str)
    assert freeusd.version()


def test_version_tuple() -> None:
    maj, min_, pat = freeusd.version_tuple()
    assert maj >= 0
    assert min_ >= 0
    assert pat >= 0
