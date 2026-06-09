#!/usr/bin/env bash
set -uo pipefail

SCENARIO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCENARIO_DIR}/../../.." && pwd)"
ARTIFACT_DIR="${SCENARIO_DIR}/artifacts"
mkdir -p "${ARTIFACT_DIR}"

LOG_FILE="${ARTIFACT_DIR}/es2panda_build_system.log"
COMMAND_FILE="${ARTIFACT_DIR}/command.txt"
ABC_FILE="${REPO_ROOT}/driver/build_system/dist/build_system.abc"
CONFIG_FILE="${REPO_ROOT}/driver/build_system/arktsconfig.json"
ETS_SRC_DIR="${REPO_ROOT}/driver/build_system/ets_src"
BINDINGS_ENTRY="${REPO_ROOT}/arkts_bindings/src/index.ets"

fail() {
  printf 'FAIL: %s\n' "$1" >&2
  exit 1
}

require_file() {
  [ -f "$1" ] || fail "missing required file: ${1#${REPO_ROOT}/}"
}

require_dir() {
  [ -d "$1" ] || fail "missing required directory: ${1#${REPO_ROOT}/}"
}

resolve_es2panda() {
  if command -v es2panda >/dev/null 2>&1; then
    command -v es2panda
    return 0
  fi

  local candidate
  for candidate in \
    "${REPO_ROOT}/../../../out/debug/bin/es2panda" \
    "${REPO_ROOT}/../../../out/release/bin/es2panda" \
    "${REPO_ROOT}/../../../out/bin/es2panda" \
    "${REPO_ROOT}/../../out/debug/bin/es2panda" \
    "${REPO_ROOT}/../../out/release/bin/es2panda" \
    "${REPO_ROOT}/../../out/bin/es2panda" \
    "${REPO_ROOT}/../out/debug/bin/es2panda" \
    "${REPO_ROOT}/../out/release/bin/es2panda" \
    "${REPO_ROOT}/../out/bin/es2panda" \
    "${REPO_ROOT}/out/debug/bin/es2panda" \
    "${REPO_ROOT}/out/release/bin/es2panda" \
    "${REPO_ROOT}/out/bin/es2panda" \
    "${REPO_ROOT}/build/bin/es2panda"; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

require_file "${CONFIG_FILE}"
require_dir "${ETS_SRC_DIR}"
require_file "${BINDINGS_ENTRY}"
require_file "${ETS_SRC_DIR}/entry.ets"
require_file "${ETS_SRC_DIR}/index.ets"
require_file "${ETS_SRC_DIR}/logger.ets"
require_file "${ETS_SRC_DIR}/types.ets"
require_file "${ETS_SRC_DIR}/pre_define.ets"
require_file "${ETS_SRC_DIR}/init/process_build_config.ets"
require_file "${ETS_SRC_DIR}/build/base_mode.ets"
require_file "${ETS_SRC_DIR}/build/build_mode.ets"
require_file "${ETS_SRC_DIR}/build/generate_arktsconfig.ets"
require_file "${ETS_SRC_DIR}/util/error.ets"
require_file "${ETS_SRC_DIR}/util/ets2panda.ets"
require_file "${ETS_SRC_DIR}/util/graph.ets"
require_file "${ETS_SRC_DIR}/util/utils.ets"
require_file "${ETS_SRC_DIR}/obfuscation/obfuscation_config.ets"
require_file "${ETS_SRC_DIR}/plugins/plugins_driver.ets"

python3 - "${CONFIG_FILE}" <<'PY' || fail "arktsconfig.json failed scaffold/decommission checks"
import json
import sys
from pathlib import Path

config_path = Path(sys.argv[1])
config = json.loads(config_path.read_text(encoding="utf-8"))
text = json.dumps(config, sort_keys=True)
for forbidden in ("driver/build_system/src", "package.json", "npm run build", "\"build\"", "build/src"):
    if forbidden in text:
        raise SystemExit(f"forbidden TypeScript build surface referenced by arktsconfig.json: {forbidden}")
for forbidden_pattern in (" tsc", "tsc ", "tsc --", "typescript"):
    if forbidden_pattern in text.lower():
        raise SystemExit(f"forbidden TypeScript build surface referenced by arktsconfig.json: {forbidden_pattern.strip()}")

paths = config.get("compilerOptions", {}).get("paths", {})
if "@arkts-bindings" not in paths:
    raise SystemExit("arktsconfig.json does not map @arkts-bindings")

include = config.get("include", [])
files = config.get("files", [])
if not any("ets_src" in str(item) for item in include + files):
    raise SystemExit("arktsconfig.json does not include ets_src sources")
PY

ES2PANDA_BIN="$(resolve_es2panda)" || fail "es2panda executable not found in PATH or common local build output locations"
mkdir -p "${REPO_ROOT}/driver/build_system/dist" || fail "failed to create driver/build_system/dist"
rm -f "${ABC_FILE}"

printf '%q --ets-module --arktsconfig %q --output %q %q\n' \
  "${ES2PANDA_BIN}" \
  "driver/build_system/arktsconfig.json" \
  "driver/build_system/dist/build_system.abc" \
  "driver/build_system/ets_src/entry.ets" > "${COMMAND_FILE}"

(
  cd "${REPO_ROOT}" && \
  "${ES2PANDA_BIN}" --ets-module --arktsconfig "driver/build_system/arktsconfig.json" --output "driver/build_system/dist/build_system.abc" "driver/build_system/ets_src/entry.ets"
) > "${LOG_FILE}" 2>&1

status=$?
if [ ${status} -ne 0 ]; then
  printf 'Compiler output saved to %s\n' "${LOG_FILE}" >&2
  exit ${status}
fi

python3 - "${LOG_FILE}" <<'PY' || fail "compiler emitted diagnostics; see ${LOG_FILE}"
import re
import sys
from pathlib import Path

log = Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
patterns = [
    r"(^|\n).*\berror\b",
    r"(^|\n).*\bfatal\b",
    r"(^|\n).*diagnostic",
]
for pattern in patterns:
    if re.search(pattern, log, re.IGNORECASE):
        raise SystemExit("compiler emitted diagnostic-looking output")
PY

[ -f "${ABC_FILE}" ] || fail "build_system.abc was not produced"
[ -s "${ABC_FILE}" ] || fail "build_system.abc is empty"

stat -c '%n %s bytes' "${ABC_FILE}" > "${ARTIFACT_DIR}/build_system_abc.stat"
printf 'PASS: ArkTS build-system scaffold compiled successfully.\n'
