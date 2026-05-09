"""Tf-shaped foundation (tokens, hashing)."""

from __future__ import annotations

import importlib

_native = importlib.import_module("freeusd._native")
_tf = _native.tf

Token = _tf.Token

__all__ = ["Token"]
