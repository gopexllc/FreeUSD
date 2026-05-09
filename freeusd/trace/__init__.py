"""Tracing hooks (no-op collector; clean-room)."""

from __future__ import annotations

from importlib import import_module

_trace = import_module("freeusd._native").trace


def push(tag: str = "") -> None:
    _trace.push(tag)


def pop() -> None:
    _trace.pop()


__all__ = ["pop", "push"]
