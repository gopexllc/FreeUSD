"""Tracing hooks (stack-depth collector; clean-room)."""

from __future__ import annotations

from importlib import import_module

_trace = import_module("freeusd._native").trace


def push(tag: str = "") -> None:
    _trace.push(tag)


def pop() -> None:
    _trace.pop()


def stack_depth() -> int:
    return int(_trace.stack_depth())


def reset() -> None:
    _trace.reset()


__all__ = ["pop", "push", "reset", "stack_depth"]
