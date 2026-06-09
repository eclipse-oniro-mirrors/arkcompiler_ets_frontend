#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="002-arkts_driver_source_tree-task-2-add-logger-runtime-surface"
WORK_ROOT=${TMPDIR:-/tmp/opencode}
ARTIFACT_DIR="$WORK_ROOT/${TASK_ID}-$$"

ES2PANDA=${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}
ARK=${ARK:-$PANDA_ROOT/out/debug/bin/ark}
ARK_DISASM=${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}
ARK_VERIFIER=${ARK_VERIFIER:-$PANDA_ROOT/out/debug/bin/verifier}
ETSSTDLIB=${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}
LOGGER_ETS="$REPO_ROOT/driver/build_system/ets_src/logger.ets"
PROD_ARKTSCONFIG="$REPO_ROOT/driver/build_system/arktsconfig.json"
TIMEOUT_SEC=30

cleanup() {
  local status=$?
  if [ "$status" -eq 0 ]; then
    rm -rf "$ARTIFACT_DIR"
  else
    printf 'transient validation artifacts retained: %s\n' "$ARTIFACT_DIR" >&2
  fi
}
trap cleanup EXIT

fail() {
  printf 'FAIL: %s\n' "$1" >&2
  exit 1
}

blocker() {
  printf 'ENVIRONMENT BLOCKER: %s\n' "$1" >&2
  printf 'BLOCKED %s\n' "$TASK_ID"
  exit 0
}

info() {
  printf 'INFO: %s\n' "$1"
}

require_file() {
  [ -f "$1" ] || fail "$2: $1"
}

require_executable() {
  [ -x "$1" ] || fail "$2: $1"
}

normalize_assert_stdout() {
  python3 - "$1" "$2" "$3" "$4" "$5" <<'PY'
import pathlib
import sys
stdout = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
stderr = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8", errors="replace")
out = pathlib.Path(sys.argv[3])
status = int(sys.argv[4])
expected = sys.argv[5]
normalized = stdout.replace("\r\n", "\n")
if normalized.endswith("\n"):
    normalized = normalized[:-1]
if status == 0 and stderr == "" and normalized == expected:
    out.write_text("PASS stdout exactly matches expected visible diagnostic\n", encoding="utf-8")
    sys.exit(0)
out.write_text(
    f"status: {status}\nstdout repr: {stdout!r}\nstderr repr: {stderr!r}\nexpected stdout: {expected!r}\n",
    encoding="utf-8",
)
sys.exit(1)
PY
}

write_config() {
  python3 - "$1" "$2" "$3" "$4" <<'PY'
import json
import pathlib
import sys
workdir = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
package = sys.argv[3]
prod_cfg = json.loads(pathlib.Path(sys.argv[4]).read_text(encoding="utf-8"))
prod_options = prod_cfg.get("compilerOptions", {})
cfg = {
    "include": [str(path) for path in sorted(workdir.glob("*.ets"))],
    "compilerOptions": {
        "package": package,
        "baseUrl": str(workdir),
        "rootDir": str(workdir),
        "cacheDir": str(workdir / "__etscache"),
        "dependencies": prod_options.get("dependencies", {}),
    },
}
out.write_text(json.dumps(cfg, indent=2), encoding="utf-8")
PY
}

compile_package() {
  local name=$1
  local config=$2
  local output=$3
  local stdout_log=$4
  local stderr_log=$5
  local cmd_log=$6
  local cmd=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$config" --output "$output")
  printf '%q ' "${cmd[@]}" > "$cmd_log"
  printf '\n' >> "$cmd_log"
  "${cmd[@]}" > "$stdout_log" 2> "$stderr_log"
  local status=$?
  [ $status -eq 0 ] || fail "$name es2panda compilation failed with exit $status; see $stderr_log"
  [ -s "$output" ] || fail "$name compiled ABC missing or empty: $output"
  python3 - "$stdout_log" "$stderr_log" "$ARTIFACT_DIR/${name}_compile_assertion.txt" <<'PY'
import pathlib
import re
import sys
stdout = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
stderr = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8", errors="replace")
out = pathlib.Path(sys.argv[3])
combined = stdout + "\n" + stderr
if re.search(r"\b(Fatal error|Semantic error|Syntax error|error E|diagnostic)\b", combined, re.IGNORECASE):
    out.write_text("FAIL es2panda emitted diagnostic output\n" + combined, encoding="utf-8")
    sys.exit(1)
out.write_text("PASS es2panda emitted no diagnostic output\n", encoding="utf-8")
PY
  [ $? -eq 0 ] || fail "$name compiler diagnostic assertion failed"
}

derive_entrypoint() {
  local abc=$1
  local disasm=$2
  local package_fragment=$3
  "$ARK_DISASM" "$abc" "$disasm" > "$disasm.stdout.log" 2> "$disasm.stderr.log"
  local status=$?
  [ $status -eq 0 ] || fail "ark_disasm failed with exit $status; see $disasm.stderr.log"
  python3 - "$disasm" "$package_fragment" <<'PY'
import pathlib
import re
import sys
text = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
fragment = sys.argv[2]
for match in re.findall(r"^\.function\s+void\s+([^\s]+.ETSGLOBAL.main)\(\)", text, re.MULTILINE):
    if fragment in match:
        print(match.replace(".ETSGLOBAL.main", ".ETSGLOBAL::main"))
        sys.exit(0)
sys.exit(1)
PY
}

verify_abc() {
  local name=$1
  local abc=$2
  local cmd_log=$3
  local stdout_log=$4
  local stderr_log=$5
  local cmd=("$ARK_VERIFIER" --boot-panda-files "$ETSSTDLIB" --load-runtimes=ets "$abc")
  printf '%q ' "${cmd[@]}" > "$cmd_log"
  printf '\n' >> "$cmd_log"
  "${cmd[@]}" > "$stdout_log" 2> "$stderr_log"
  local status=$?
  [ $status -eq 0 ] || fail "$name verifier rejected ABC with exit $status; see $stderr_log"
}

run_abc() {
  local abc=$1
  local entrypoint=$2
  local cmd_log=$3
  local stdout_log=$4
  local stderr_log=$5
  local cmd=("$ARK" --boot-panda-files "$ETSSTDLIB" --load-runtimes=ets --compiler-ignore-failures=false --panda-files "$abc" "$abc" "$entrypoint")
  printf '%q ' "${cmd[@]}" > "$cmd_log"
  printf '\n' >> "$cmd_log"
  timeout "${TIMEOUT_SEC}s" "${cmd[@]}" > "$stdout_log" 2> "$stderr_log"
  return $?
}

mkdir -p "$ARTIFACT_DIR" || fail "failed to create artifact directory $ARTIFACT_DIR"
require_file "$LOGGER_ETS" "production logger not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"
require_executable "$ES2PANDA" "es2panda not executable"
require_executable "$ARK" "ark runtime not executable"
require_executable "$ARK_DISASM" "ark_disasm not executable"
require_executable "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

info "offline deterministic validation; network, external providers, and devices are disabled"
info "validating production logger surface at $LOGGER_ETS"

python3 - "$LOGGER_ETS" "$ARTIFACT_DIR/node_surface_assertion.txt" <<'PY'
import pathlib
import re
import sys
source = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
text = source.read_text(encoding="utf-8")
forbidden = [
    "process.exit",
    "process.stderr",
    "process.pid",
    "from 'child_process'",
    'from "child_process"',
    "from 'fs'",
    'from "fs"',
    "from 'path'",
    'from "path"',
    "from 'os'",
    'from "os"',
]
found = [item for item in forbidden if item in text]
node_import = re.search(r"^\s*import\s+.*\s+from\s+['\"](?:node:)?(?:process|fs|path|os|child_process)['\"]", text, re.MULTILINE)
if node_import:
    found.append(node_import.group(0).strip())
if found:
    out.write_text("FAIL forbidden Node.js logger surfaces found: " + ", ".join(found) + "\n", encoding="utf-8")
    sys.exit(1)
out.write_text("PASS no forbidden Node.js logger surfaces found\n", encoding="utf-8")
PY
[ $? -eq 0 ] || fail "forbidden Node.js logger surfaces found; see $ARTIFACT_DIR/node_surface_assertion.txt"

BASELINE_DIR="$ARTIFACT_DIR/baseline_pkg"
mkdir -p "$BASELINE_DIR" || fail "failed to create baseline package"
python3 - "$BASELINE_DIR/baseline_driver.ets" <<'PY'
import pathlib
import sys
pathlib.Path(sys.argv[1]).write_text(
    "function main(): void {\n"
    "  console.log('build started');\n"
    "}\n",
    encoding="utf-8",
)
PY
BASELINE_CONFIG="$ARTIFACT_DIR/baseline_arktsconfig.json"
BASELINE_ABC="$ARTIFACT_DIR/baseline_driver.abc"
write_config "$BASELINE_DIR" "$BASELINE_CONFIG" "@val_baseline" "$PROD_ARKTSCONFIG"
compile_package "baseline" "$BASELINE_CONFIG" "$BASELINE_ABC" "$ARTIFACT_DIR/baseline_compile.stdout.log" "$ARTIFACT_DIR/baseline_compile.stderr.log" "$ARTIFACT_DIR/baseline_compile_command.txt"
BASELINE_ENTRYPOINT=$(derive_entrypoint "$BASELINE_ABC" "$ARTIFACT_DIR/baseline_driver.pa" ".baseline_driver.")
[ -n "$BASELINE_ENTRYPOINT" ] || fail "could not derive baseline runtime entrypoint"
verify_abc "baseline" "$BASELINE_ABC" "$ARTIFACT_DIR/baseline_verify_command.txt" "$ARTIFACT_DIR/baseline_verify.stdout.log" "$ARTIFACT_DIR/baseline_verify.stderr.log"
run_abc "$BASELINE_ABC" "$BASELINE_ENTRYPOINT" "$ARTIFACT_DIR/baseline_runtime_command.txt" "$ARTIFACT_DIR/baseline_runtime.stdout.log" "$ARTIFACT_DIR/baseline_runtime.stderr.log"
baseline_status=$?
normalize_assert_stdout "$ARTIFACT_DIR/baseline_runtime.stdout.log" "$ARTIFACT_DIR/baseline_runtime.stderr.log" "$ARTIFACT_DIR/baseline_stdout_assertion.txt" "$baseline_status" "build started"
baseline_assert_status=$?
if [ $baseline_status -ne 0 ] || [ $baseline_assert_status -ne 0 ]; then
  cat "$ARTIFACT_DIR/baseline_stdout_assertion.txt" >&2
  blocker "same-toolchain console baseline failed; logger runtime stdout cannot be attributed to product code"
fi
info "same-toolchain console baseline passed"

WORKDIR="$ARTIFACT_DIR/logger_pkg"
mkdir -p "$WORKDIR" || fail "failed to create validation package"
cp "$LOGGER_ETS" "$WORKDIR/logger.ets" || fail "failed to copy production logger into validation package"
python3 - "$WORKDIR/val_driver.ets" <<'PY'
import pathlib
import sys
pathlib.Path(sys.argv[1]).write_text(
    "import { Logger } from './logger';\n"
    "\n"
    "function main(): void {\n"
    "  Logger.getInstance().printInfo('build started');\n"
    "}\n",
    encoding="utf-8",
)
PY
VALIDATION_CONFIG="$ARTIFACT_DIR/logger_arktsconfig.json"
DRIVER_ABC="$ARTIFACT_DIR/val_driver.abc"
write_config "$WORKDIR" "$VALIDATION_CONFIG" "@val_logger" "$PROD_ARKTSCONFIG"
compile_package "logger" "$VALIDATION_CONFIG" "$DRIVER_ABC" "$ARTIFACT_DIR/logger_compile.stdout.log" "$ARTIFACT_DIR/logger_compile.stderr.log" "$ARTIFACT_DIR/logger_compile_command.txt"
info "logger compilation succeeded, ABC size: $(stat -c '%s' "$DRIVER_ABC")"
ENTRYPOINT=$(derive_entrypoint "$DRIVER_ABC" "$ARTIFACT_DIR/val_driver.pa" ".val_driver.")
[ -n "$ENTRYPOINT" ] || fail "could not derive logger runtime entrypoint"
info "resolved entrypoint: $ENTRYPOINT"
verify_abc "logger" "$DRIVER_ABC" "$ARTIFACT_DIR/logger_verify_command.txt" "$ARTIFACT_DIR/logger_verify.stdout.log" "$ARTIFACT_DIR/logger_verify.stderr.log"
info "verifier accepted logger ABC"
run_abc "$DRIVER_ABC" "$ENTRYPOINT" "$ARTIFACT_DIR/logger_runtime_command.txt" "$ARTIFACT_DIR/logger_runtime.stdout.log" "$ARTIFACT_DIR/logger_runtime.stderr.log"
runtime_status=$?
normalize_assert_stdout "$ARTIFACT_DIR/logger_runtime.stdout.log" "$ARTIFACT_DIR/logger_runtime.stderr.log" "$ARTIFACT_DIR/logger_stdout_assertion.txt" "$runtime_status" "build started"
stdout_assert_status=$?
if [ $runtime_status -eq 0 ] && [ $stdout_assert_status -eq 0 ]; then
  printf 'PASS %s\n' "$TASK_ID"
  exit 0
fi
cat "$ARTIFACT_DIR/logger_stdout_assertion.txt" >&2
printf 'PRODUCT FAILURE: Logger runtime acceptance criteria not satisfied. Expected runtime exit status 0, stdout exactly build started, and empty stderr.\n' >&2
exit 1
