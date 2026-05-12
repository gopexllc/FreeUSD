#!/usr/bin/env python3
"""Lightweight guardrail for engine contract and clean-room docs."""

from __future__ import annotations

from pathlib import Path
import sys


REQUIRED_FILES = {
    "CONTRIBUTING.md": ["clean-room-policy", "engine-supported-subset"],
    "docs/engine-supported-subset.md": ["pre_baked_assets_only", "tests/fixtures/parity_namespace.usda"],
    "docs/clean-room-policy.md": ["Prohibited Inputs", "Contribution Rules"],
    "docs/fixture-policy.md": ["Binary Fixture Rules", "parity_*.usdc"],
    "docs/compatibility-claims.md": ["docs/openusd-parity-matrix.md", "pre_baked_assets_only"],
    "docs/engine-integration.md": ["engine-supported-subset.md", "freeusd_stage_compute_local_to_world_transform_matrix4d"],
    "docs/openusd-parity-matrix.md": ["tests/fixtures/parity_embedded_scene.usdc", "engineScene"],
}


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    missing: list[str] = []
    for rel_path, needles in REQUIRED_FILES.items():
        path = repo / rel_path
        if not path.is_file():
            missing.append(f"missing file: {rel_path}")
            continue
        text = path.read_text(encoding="utf-8")
        for needle in needles:
            if needle not in text:
                missing.append(f"{rel_path} missing required text: {needle}")
    if missing:
        for item in missing:
            print(item, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
