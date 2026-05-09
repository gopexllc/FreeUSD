"""Copy built ``_native`` into ``site-packages/freeusd`` when that package exists (editable / venv).

Checks: ``<repo>/.venv`` (this file lives in ``cmake/``), then ``$VIRTUAL_ENV``, then
``sysconfig.get_path('purelib')`` for the interpreter running this script.
"""
from __future__ import annotations

import os
import shutil
import sys
import sysconfig
from pathlib import Path


def _venv_roots() -> list[Path]:
    repo = Path(__file__).resolve().parents[1]
    roots = [repo / ".venv"]
    ve = os.environ.get("VIRTUAL_ENV", "").strip()
    if ve:
        roots.append(Path(ve))
    return roots


def _candidate_site_roots() -> list[Path]:
    out: list[Path] = []
    for venv in _venv_roots():
        if not venv.is_dir():
            continue
        lib = venv / "lib"
        if not lib.is_dir():
            continue
        for site in sorted(lib.glob("python*/site-packages")):
            if site.is_dir() and site not in out:
                out.append(site)
    pure = Path(sysconfig.get_path("purelib"))
    if pure.is_dir() and pure not in out:
        out.append(pure)
    return out


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} <path-to-built-_native.so>")
    so = Path(sys.argv[1]).resolve()
    if not so.is_file():
        raise SystemExit(f"missing extension: {so}")
    for site in _candidate_site_roots():
        dest_dir = site / "freeusd"
        if dest_dir.is_dir():
            shutil.copy2(so, dest_dir / so.name)


if __name__ == "__main__":
    main()
