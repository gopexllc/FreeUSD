"""USD container sniffing (ASCII vs crate magic; no binary decode)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usd.crate

UsdFileKind = _m.UsdFileKind
detect_usd_file_kind_from_path = _m.detect_usd_file_kind_from_path

__all__ = ["UsdFileKind", "detect_usd_file_kind_from_path"]
