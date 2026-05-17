#!/usr/bin/env bash
# Mirror .github/workflows/ci.yml on a single machine (Linux-focused).
# Use when GitHub-hosted runners are unavailable (e.g. org billing lock).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

BUILD_DIR="${FREEUSD_CI_BUILD_DIR:-build}"
BUILD_PY_DIR="${FREEUSD_CI_BUILD_PY_DIR:-build-ci-py}"
JOBS="${FREEUSD_CI_JOBS:-$(nproc 2>/dev/null || echo 4)}"
FAILED=0

run_step() {
  local name="$1"
  shift
  echo ""
  echo "========== ${name} =========="
  if "$@"; then
    echo "OK: ${name}"
  else
    echo "FAIL: ${name}" >&2
    FAILED=1
  fi
}

run_step "tree-hygiene (tracked build-*)" bash -c '
  set -euo pipefail
  bad="$(git ls-files | grep -E "^build-" || true)"
  if [ -n "${bad}" ]; then
    echo "Tracked build-* paths:" >&2
    printf "%s\n" "${bad}" >&2
    exit 1
  fi
'

run_step "tree-hygiene (engine contract docs)" python3 scripts/check_engine_contract_docs.py

run_step "linux (Release, install integration)" bash -c "
  set -euo pipefail
  cmake -S . -B '${BUILD_DIR}' -DCMAKE_BUILD_TYPE=Release \
    -DFREEUSD_BUILD_PYTHON=OFF \
    -DFREEUSD_BUILD_TESTS=ON \
    -DFREEUSD_TEST_INSTALL_INTEGRATION=ON
  cmake --build '${BUILD_DIR}' --parallel '${JOBS}'
  ctest --test-dir '${BUILD_DIR}' --output-on-failure
"

if command -v python3 >/dev/null 2>&1; then
  run_step "linux-python" bash -c "
    set -euo pipefail
    if ! python3 -m pip install --upgrade --break-system-packages 'pytest>=9.0.3' 2>/dev/null; then
      python3 -m pip install --user --upgrade 'pytest>=9.0.3'
    fi
    cmake -S . -B '${BUILD_PY_DIR}' -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DFREEUSD_BUILD_PYTHON=ON \
      -DFREEUSD_BUILD_TESTS=ON \
      -DFREEUSD_TEST_INSTALL_INTEGRATION=OFF
    cmake --build '${BUILD_PY_DIR}' --parallel '${JOBS}'
    ctest --test-dir '${BUILD_PY_DIR}' --output-on-failure
    PYTHONPATH='${ROOT}' python3 -m pytest tests/python -q
  "
else
  echo "SKIP: linux-python (python3 not found)"
fi

if command -v cargo >/dev/null 2>&1 || command -v go >/dev/null 2>&1; then
  run_step "linux-bindings (C++ for CGO)" bash -c "
    set -euo pipefail
    if [ ! -f '${BUILD_DIR}/CMakeCache.txt' ]; then
      cmake -S . -B '${BUILD_DIR}' -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DFREEUSD_BUILD_PYTHON=OFF \
        -DFREEUSD_BUILD_TESTS=ON \
        -DFREEUSD_TEST_INSTALL_INTEGRATION=OFF
    fi
    cmake --build '${BUILD_DIR}' --parallel '${JOBS}'
  "
fi

if command -v cargo >/dev/null 2>&1; then
  run_step "linux-bindings (Rust)" bash -c "
    set -euo pipefail
    FREEUSD_REPO_ROOT='${ROOT}' cargo test --manifest-path bindings/rust/Cargo.toml
  "
else
  echo "SKIP: linux-bindings Rust (cargo not found)"
fi

if command -v go >/dev/null 2>&1; then
  run_step "linux-bindings (Go)" bash -c "
    set -euo pipefail
    (cd bindings/go && go test ./...)
  "
else
  echo "SKIP: linux-bindings Go (go not found)"
fi

echo ""
if [ "${FAILED}" -ne 0 ]; then
  echo "Local CI finished with failures." >&2
  exit 1
fi
echo "Local CI finished successfully."
