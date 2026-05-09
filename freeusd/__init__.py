"""FreeUSD: independent OpenUSD-shaped runtime (clean-room)."""

from __future__ import annotations

import importlib

from freeusd import ar as ar
from freeusd import io as io
from freeusd import pcp as pcp
from freeusd import sdf as sdf
from freeusd import tf as tf
from freeusd import usd as usd
from freeusd import usdGeom as usdGeom
from freeusd import vt as vt

_native = importlib.import_module("freeusd._native")

__all__ = [
    "ar",
    "io",
    "pcp",
    "sdf",
    "tf",
    "usd",
    "usdGeom",
    "vt",
    "version",
    "version_tuple",
]


def version() -> str:
    return _native.version_string()


def version_tuple() -> tuple[int, int, int]:
    maj, min_, pat = _native.version_parts()
    return int(maj), int(min_), int(pat)
