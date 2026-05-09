"""Sdf-shaped builtin field name tokens (clean-room)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").sdf.builtin_tokens

Active = _m.Active
CustomData = _m.CustomData
DefaultPrim = _m.DefaultPrim
Documentation = _m.Documentation
EndTimeCode = _m.EndTimeCode
FramePrecision = _m.FramePrecision
FramesPerSecond = _m.FramesPerSecond
Kind = _m.Kind
MetersPerUnit = _m.MetersPerUnit
PrimOrder = _m.PrimOrder
StartTimeCode = _m.StartTimeCode
SubLayers = _m.SubLayers
TimeCodesPerSecond = _m.TimeCodesPerSecond
UpAxis = _m.UpAxis
VariantSetNames = _m.VariantSetNames

__all__ = [
    "Active",
    "CustomData",
    "DefaultPrim",
    "Documentation",
    "EndTimeCode",
    "FramePrecision",
    "FramesPerSecond",
    "Kind",
    "MetersPerUnit",
    "PrimOrder",
    "StartTimeCode",
    "SubLayers",
    "TimeCodesPerSecond",
    "UpAxis",
    "VariantSetNames",
]
