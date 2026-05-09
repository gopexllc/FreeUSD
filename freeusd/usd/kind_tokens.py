"""Model-hierarchy `kind` token stubs (UsdKindTokens-shaped)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").usd.kind_tokens

Assembly = _m.Assembly
Component = _m.Component
Group = _m.Group
Subcomponent = _m.Subcomponent

__all__ = ["Assembly", "Component", "Group", "Subcomponent"]
