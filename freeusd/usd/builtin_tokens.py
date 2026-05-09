"""Usd-shaped composition / prim list field tokens (clean-room)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usd.builtin_tokens

Inherits = _m.Inherits
Payload = _m.Payload
PrefixSubstitutions = _m.PrefixSubstitutions
References = _m.References
Relocates = _m.Relocates
Specializes = _m.Specializes
VariantSelection = _m.VariantSelection
VariantSets = _m.VariantSets

__all__ = [
    "Inherits",
    "Payload",
    "PrefixSubstitutions",
    "References",
    "Relocates",
    "Specializes",
    "VariantSelection",
    "VariantSets",
]
