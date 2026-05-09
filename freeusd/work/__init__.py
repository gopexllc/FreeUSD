"""Work dispatcher (serial stub; clean-room)."""

from __future__ import annotations

from importlib import import_module

_work = import_module("freeusd._native").work


def run(job) -> None:
    _work.run(job)


__all__ = ["run"]
