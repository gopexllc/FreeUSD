"""Sdf-shaped builtin field name tokens (clean-room)."""

from __future__ import annotations

from importlib import import_module

_m = import_module("freeusd._native").sdf.builtin_tokens

Active = _m.Active
Comment = _m.Comment
CustomData = _m.CustomData
CustomLayerData = _m.CustomLayerData
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
    "Comment",
    "CustomData",
    "CustomLayerData",
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
    "SubLayerOffsets",
    "TimeCodesPerSecond",
    "UpAxis",
    "VariantSetNames",
]
