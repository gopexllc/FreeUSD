"""FreeUSD: independent OpenUSD-shaped runtime (clean-room)."""

from __future__ import annotations

import importlib

from freeusd import ar as ar
from freeusd import gf as gf
from freeusd import io as io
from freeusd import pcp as pcp
from freeusd import plug as plug
from freeusd import sdf as sdf
from freeusd import tf as tf
from freeusd import trace as trace
from freeusd import usd as usd
from freeusd import usdGeom as usdGeom
from freeusd import usdLux as usdLux
from freeusd import usdMedia as usdMedia
from freeusd import usdPhysics as usdPhysics
from freeusd import usdRender as usdRender
from freeusd import usdRi as usdRi
from freeusd import usdShade as usdShade
from freeusd import usdSkel as usdSkel
from freeusd import usdUtils as usdUtils
from freeusd import usdVol as usdVol
from freeusd import vt as vt
from freeusd import work as work

_native = importlib.import_module("freeusd._native")

__all__ = [
    "ar",
    "gf",
    "io",
    "pcp",
    "plug",
    "sdf",
    "tf",
    "trace",
    "usd",
    "usdGeom",
    "usdLux",
    "usdMedia",
    "usdPhysics",
    "usdRender",
    "usdRi",
    "usdShade",
    "usdSkel",
    "usdUtils",
    "usdVol",
    "vt",
    "work",
    "version",
    "version_tuple",
]


def version() -> str:
    return _native.version_string()


def version_tuple() -> tuple[int, int, int]:
    maj, min_, pat = _native.version_parts()
    return int(maj), int(min_), int(pat)
