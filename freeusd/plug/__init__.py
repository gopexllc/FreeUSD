"""Plug registry (minimal; clean-room)."""

from __future__ import annotations

from importlib import import_module

_plug = import_module("freeusd._native").plug


def load_all() -> None:
    _plug.load_all()


def register_plugin_paths(paths: list[str]) -> None:
    _plug.register_plugin_paths(paths)


def registered_plugin_paths() -> list[str]:
    return list(_plug.registered_plugin_paths())


__all__ = ["load_all", "register_plugin_paths", "registered_plugin_paths"]
