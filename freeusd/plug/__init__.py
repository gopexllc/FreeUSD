"""Plug registry (minimal; clean-room)."""

from __future__ import annotations

from importlib import import_module

_plug = import_module("freeusd._native").plug


def load_all() -> None:
    _plug.load_all()


__all__ = ["load_all"]
