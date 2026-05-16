"""USDA (ASCII) I/O - minimal subset."""

from __future__ import annotations

import importlib

_io = importlib.import_module("freeusd._native").io

ParseResult = _io.ParseResult
load_from_string = _io.load_from_string
save_to_string = _io.save_to_string
load_from_file = _io.load_from_file
save_to_file = _io.save_to_file

__all__ = [
    "ParseResult",
    "load_from_file",
    "load_from_string",
    "save_to_file",
    "save_to_string",
]
