"""UsdMedia-shaped schema tokens (full set from `_native`)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usdMedia.tokens

__all__ = sorted(n for n in dir(_m) if not n.startswith("_") and callable(getattr(_m, n)))
for _n in __all__:
    globals()[_n] = getattr(_m, _n)
