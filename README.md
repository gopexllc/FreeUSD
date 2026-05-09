# FreeUSD
A GPL-3.0 licensed, independent implementation of the OpenUSD (Universal Scene Description) file formats and core data model.

---

FreeUSD is an independent project and is not endorsed by or affiliated with Pixar, the Alliance for OpenUSD, or the official OpenUSD project. It aims for compliance with the published OpenUSD Core Specification.

## Layout

C++ libraries follow OpenUSD-style layering (`tf`, `gf`, `vt`, `ar`, `sdf`, `pcp`, `usd`, `usdGeom`, plus `plug` / `trace` / `work` stubs). See [docs/openusd-map.md](docs/openusd-map.md) for a side-by-side map. **USDA:** minimal ASCII load/save (`freeusd::io::usda` / `freeusd.io`), including `attr.timeSamples = { t: v, ... }` blocks. **`.usdc`:** not implemented yet (header placeholder only).

## Building

**C++ (library, tests, extension in the build tree)**

```bash
cmake -S . -B build -DFREEUSD_BUILD_PYTHON=ON -DFREEUSD_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The native Python module is built as `_native*.so` under `build/`; use an editable install to import it as `freeusd`.

**Python package (recommended for development)**

```bash
python3 -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -e ".[dev]"
pytest tests/python -q
```
