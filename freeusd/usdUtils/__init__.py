"""UsdUtils-shaped helpers (clean-room; grows with flatten/cache/engine APIs)."""

from __future__ import annotations

import importlib

_utils = importlib.import_module("freeusd._native").usdUtils

EnginePrimEditorView = _utils.EnginePrimEditorView
EngineRuntimeMode = _utils.EngineRuntimeMode
EngineRuntimeSupportReport = _utils.EngineRuntimeSupportReport
EngineSceneNode = _utils.EngineSceneNode
EngineSceneSnapshot = _utils.EngineSceneSnapshot
FlattenOptions = _utils.FlattenOptions
assess_engine_runtime_support = _utils.assess_engine_runtime_support
build_engine_prim_editor_view = _utils.build_engine_prim_editor_view
build_engine_scene_snapshot = _utils.build_engine_scene_snapshot
engine_runtime_mode_name = _utils.engine_runtime_mode_name
flatten_stage_at_time = _utils.flatten_stage_at_time

__all__ = [
    "EnginePrimEditorView",
    "EngineRuntimeMode",
    "EngineRuntimeSupportReport",
    "EngineSceneNode",
    "EngineSceneSnapshot",
    "FlattenOptions",
    "assess_engine_runtime_support",
    "build_engine_prim_editor_view",
    "build_engine_scene_snapshot",
    "engine_runtime_mode_name",
    "flatten_stage_at_time",
]
