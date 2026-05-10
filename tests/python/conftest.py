"""Pytest hooks so the in-repo ``freeusd/`` package wins over stale editable snapshots."""

from __future__ import annotations

import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[2]


def _is_skbuild_freeusd_redirector(finder: object) -> bool:
    t = type(finder)
    return t.__name__ == "ScikitBuildRedirectingFinder" and getattr(t, "__module__", "") == "_freeusd_editable"


def pytest_configure() -> None:
    # scikit-build-core editable installs register a MetaPathFinder that redirects
    # ``freeusd`` to a copy under site-packages. CTest sets ``PYTHONPATH`` to the
    # repository root, but that is ignored while the finder is active.
    sys.meta_path[:] = [f for f in sys.meta_path if not _is_skbuild_freeusd_redirector(f)]
    root = str(_REPO_ROOT)
    if root not in sys.path:
        sys.path.insert(0, root)
    for key in list(sys.modules):
        if key == "freeusd" or key.startswith("freeusd."):
            del sys.modules[key]
