#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="008-arkts_driver_interop_runtime-task-1-add-interop-helper-boundary"
WORK_ROOT=${TMPDIR:-/tmp/opencode}
ARTIFACT_DIR="$WORK_ROOT/${TASK_ID}-$$"
BLOCKED=0

ES2PANDA=${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}
ARK=${ARK:-$PANDA_ROOT/out/debug/bin/ark}
ARK_DISASM=${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}
ARK_VERIFIER=${ARK_VERIFIER:-$PANDA_ROOT/out/debug/bin/verifier}
ETSSTDLIB=${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}
PROD_ARKTSCONFIG="$REPO_ROOT/driver/build_system/arktsconfig.json"
PROD_SRC="$REPO_ROOT/driver/build_system/ets_src"
INTEROP_HELPER="$PROD_SRC/util/interop_helper.ets"
PROCESS_CONFIG="$PROD_SRC/init/process_build_config.ets"
DEMO_CONFIG="$REPO_ROOT/driver/build_system/test/demo_hap/build_config.json"
PROCESS_TEST="$REPO_ROOT/driver/build_system/test/ets_ut/process_build_config_test.ets"
MISSING_MODULE_CONFIG="$REPO_ROOT/driver/build_system/test/ets_ut/missing_module_build_config.json"
MISSING_SDK_CONFIG="$REPO_ROOT/driver/build_system/test/ets_ut/missing_sdk_build_config.json"
EMPTY_OUTPUT="$REPO_ROOT/driver/build_system/test/ets_ut/empty_output.abc"
TIMEOUT_SEC=45

cleanup() {
  local status=$?
  if [ "$status" -eq 0 ] && [ "$BLOCKED" -eq 0 ]; then
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
  BLOCKED=1
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

require_executable_or_blocker() {
  [ -x "$1" ] || blocker "$2: $1"
}

compile_arkts() {
  local name=$1
  local output=$2
  local stdout_log=$3
  local stderr_log=$4
  local cmd_log=$5
  shift 5
  local cmd=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$PROD_ARKTSCONFIG" --output "$output" "$@")
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
if re.search(r"\b(Fatal error|Semantic error|Syntax error|error E)\b", combined, re.IGNORECASE):
    out.write_text("FAIL es2panda emitted error diagnostics\n" + combined, encoding="utf-8")
    sys.exit(1)
out.write_text("PASS es2panda emitted no error diagnostics\n", encoding="utf-8")
PY
  [ $? -eq 0 ] || fail "$name compiler diagnostic assertion failed"
}

write_baseline_package() {
  local pkg_dir=$1
  local config=$2
  mkdir -p "$pkg_dir" || fail "failed to create baseline package"
  cat > "$pkg_dir/baseline.ets" <<'EOF_BASELINE'
function main(): void {
  console.log('console-baseline-ok');
}
EOF_BASELINE
  python3 - "$pkg_dir" "$config" "$PROD_ARKTSCONFIG" <<'PY'
import json
import pathlib
import sys
pkg_dir = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
prod = json.loads(pathlib.Path(sys.argv[3]).read_text(encoding="utf-8"))
options = prod.get("compilerOptions", {})
cfg = {
    "include": [str(pkg_dir / "baseline.ets")],
    "compilerOptions": {
        "package": "@val_interop_helper_baseline",
        "baseUrl": str(pkg_dir),
        "rootDir": str(pkg_dir),
        "cacheDir": str(pkg_dir / "__etscache"),
        "paths": options.get("paths", {}),
        "dependencies": options.get("dependencies", {}),
    },
    "references": prod.get("references", []),
}
out.write_text(json.dumps(cfg, indent=2), encoding="utf-8")
PY
}

compile_baseline() {
  local config=$1
  local source=$2
  local output=$3
  local stdout_log=$4
  local stderr_log=$5
  local cmd_log=$6
  local cmd=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$config" --output "$output" "$source")
  printf '%q ' "${cmd[@]}" > "$cmd_log"
  printf '\n' >> "$cmd_log"
  "${cmd[@]}" > "$stdout_log" 2> "$stderr_log"
  local status=$?
  [ $status -eq 0 ] || blocker "console baseline compilation failed with exit $status; see $stderr_log"
  [ -s "$output" ] || blocker "console baseline ABC missing or empty: $output"
}

derive_entrypoint() {
  local abc=$1
  local disasm=$2
  "$ARK_DISASM" "$abc" "$disasm" > "$disasm.stdout.log" 2> "$disasm.stderr.log"
  local status=$?
  [ $status -eq 0 ] || blocker "ark_disasm failed with exit $status; see $disasm.stderr.log"
  python3 - "$disasm" <<'PY'
import pathlib
import re
import sys
text = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
for match in re.findall(r"^\.function\s+void\s+([^\s]+.ETSGLOBAL.main)\(\)", text, re.MULTILINE):
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

assert_stdout_contains() {
  python3 - "$1" "$2" "$3" "$4" <<'PY'
import pathlib
import sys
stdout = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
stderr = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8", errors="replace")
out = pathlib.Path(sys.argv[3])
expected = sys.argv[4]
if expected in stdout:
    out.write_text("PASS stdout contains expected text\n" + stdout, encoding="utf-8")
    sys.exit(0)
out.write_text(
    "FAIL stdout did not contain expected text\n"
    f"expected: {expected}\nstdout:\n{stdout}\nstderr:\n{stderr}\n",
    encoding="utf-8",
)
sys.exit(1)
PY
}

mkdir -p "$ARTIFACT_DIR" || fail "failed to create artifact directory $ARTIFACT_DIR"
require_file "$INTEROP_HELPER" "production interop helper source not found"
require_file "$PROCESS_CONFIG" "production process_build_config source not found"
require_file "$PROD_SRC/util/utils.ets" "production utils source not found"
require_file "$PROD_SRC/util/error.ets" "production error source not found"
require_file "$PROD_SRC/entry.ets" "production entry source not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"
require_file "$DEMO_CONFIG" "demo_hap build_config.json not found"
require_file "$PROCESS_TEST" "interop helper runtime test source not found"
require_file "$MISSING_MODULE_CONFIG" "missing module fixture not found"
require_file "$MISSING_SDK_CONFIG" "missing SDK fixture not found"
require_file "$EMPTY_OUTPUT" "empty output ABC fixture not found"
require_executable_or_blocker "$ES2PANDA" "es2panda not executable"
require_executable_or_blocker "$ARK" "ark runtime not executable"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm not executable"
require_executable_or_blocker "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

info "offline deterministic validation; network, external providers, and devices are disabled"

python3 - "$PROD_SRC" "$INTEROP_HELPER" "$PROCESS_CONFIG" "$DEMO_CONFIG" "$MISSING_MODULE_CONFIG" "$MISSING_SDK_CONFIG" "$EMPTY_OUTPUT" "$ARTIFACT_DIR/source_api_fixture_assertion.txt" <<'PY'
import json
import pathlib
import re
import sys
src = pathlib.Path(sys.argv[1])
helper = pathlib.Path(sys.argv[2])
process = pathlib.Path(sys.argv[3])
demo = pathlib.Path(sys.argv[4])
missing_module = pathlib.Path(sys.argv[5])
missing_sdk = pathlib.Path(sys.argv[6])
empty_output = pathlib.Path(sys.argv[7])
out = pathlib.Path(sys.argv[8])
helper_text = helper.read_text(encoding="utf-8", errors="replace")
process_text = process.read_text(encoding="utf-8", errors="replace")
required_helper_snippets = [
    "global.interop._ReadFile",
    "global.interop._FileExists",
    "global.interop._FileSize",
    "getEnvironmentVar",
    "getNativeLibraryPath",
    "missing-native-library",
    "missing-output-abc",
    "empty-output-abc",
]
missing = [snippet for snippet in required_helper_snippets if snippet not in helper_text]
if missing:
    out.write_text("FAIL interop helper missing required snippets: " + ", ".join(missing) + "\n", encoding="utf-8")
    sys.exit(1)
required_process_snippets = [
    "../util/interop_helper",
    "ensureFileExists(resolvedConfigPath, 'missing-build-config')",
    "substituteEnvVarsInJSON(rawText)",
    "resolveNativeCompilerLibraryPath()",
    "ensurePathExists(modulePath, 'missing-module-path')",
    "ensurePathExists(requiredPaths[i], 'missing-sdk-stub-path')",
]
missing = [snippet for snippet in required_process_snippets if snippet not in process_text]
if missing:
    out.write_text("FAIL process_build_config is not helper-backed: " + ", ".join(missing) + "\n", encoding="utf-8")
    sys.exit(1)
for path in (demo, missing_module, missing_sdk):
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        out.write_text(f"FAIL {path} is not a JSON object\n", encoding="utf-8")
        sys.exit(1)
if empty_output.stat().st_size != 0:
    out.write_text(f"FAIL {empty_output} must be zero bytes\n", encoding="utf-8")
    sys.exit(1)
for path in src.rglob("*.ets"):
    text = path.read_text(encoding="utf-8", errors="replace")
    if re.search(r"^\s*import\s+.*\b(child_process|fs|path|os|process)\b", text, re.MULTILINE):
        out.write_text(f"FAIL forbidden Node import in {path}\n", encoding="utf-8")
        sys.exit(1)
out.write_text("PASS helper API, fixtures, binary fixture, and no forbidden Node imports validated\n", encoding="utf-8")
PY
[ $? -eq 0 ] || fail "source/API/fixture assertion failed; see $ARTIFACT_DIR/source_api_fixture_assertion.txt"

ENTRY_ABC="$ARTIFACT_DIR/build_system_entry.abc"
compile_arkts "build_system_entry" "$ENTRY_ABC" "$ARTIFACT_DIR/entry_compile.stdout.log" "$ARTIFACT_DIR/entry_compile.stderr.log" "$ARTIFACT_DIR/entry_compile_command.txt" "$PROD_SRC/entry.ets"

PROCESS_ABC="$ARTIFACT_DIR/process_build_config_test.abc"
compile_arkts "process_build_config_test" "$PROCESS_ABC" "$ARTIFACT_DIR/process_compile.stdout.log" "$ARTIFACT_DIR/process_compile.stderr.log" "$ARTIFACT_DIR/process_compile_command.txt" "$PROCESS_TEST"

BASELINE_DIR="$ARTIFACT_DIR/console_baseline_pkg"
BASELINE_CONFIG="$ARTIFACT_DIR/console_baseline_arktsconfig.json"
BASELINE_ABC="$ARTIFACT_DIR/console_baseline.abc"
write_baseline_package "$BASELINE_DIR" "$BASELINE_CONFIG"
compile_baseline "$BASELINE_CONFIG" "$BASELINE_DIR/baseline.ets" "$BASELINE_ABC" "$ARTIFACT_DIR/console_baseline_compile.stdout.log" "$ARTIFACT_DIR/console_baseline_compile.stderr.log" "$ARTIFACT_DIR/console_baseline_compile_command.txt"
BASELINE_ENTRYPOINT=$(derive_entrypoint "$BASELINE_ABC" "$ARTIFACT_DIR/console_baseline.pa")
[ -n "$BASELINE_ENTRYPOINT" ] || blocker "could not derive console baseline entrypoint"
verify_abc "console_baseline" "$BASELINE_ABC" "$ARTIFACT_DIR/console_baseline_verify_command.txt" "$ARTIFACT_DIR/console_baseline_verify.stdout.log" "$ARTIFACT_DIR/console_baseline_verify.stderr.log"
run_abc "$BASELINE_ABC" "$BASELINE_ENTRYPOINT" "$ARTIFACT_DIR/console_baseline_runtime_command.txt" "$ARTIFACT_DIR/console_baseline_runtime.stdout.log" "$ARTIFACT_DIR/console_baseline_runtime.stderr.log"
baseline_status=$?
if [ $baseline_status -ne 0 ]; then
  blocker "console baseline runtime failed with exit $baseline_status; see $ARTIFACT_DIR/console_baseline_runtime.stderr.log"
fi
assert_stdout_contains "$ARTIFACT_DIR/console_baseline_runtime.stdout.log" "$ARTIFACT_DIR/console_baseline_runtime.stderr.log" "$ARTIFACT_DIR/console_baseline_stdout_assertion.txt" "console-baseline-ok"
[ $? -eq 0 ] || blocker "console baseline did not produce expected stdout; see $ARTIFACT_DIR/console_baseline_stdout_assertion.txt"

PROCESS_ENTRYPOINT=$(derive_entrypoint "$PROCESS_ABC" "$ARTIFACT_DIR/process_build_config_test.pa")
[ -n "$PROCESS_ENTRYPOINT" ] || fail "could not derive process build config test entrypoint"
verify_abc "process_build_config_test" "$PROCESS_ABC" "$ARTIFACT_DIR/process_verify_command.txt" "$ARTIFACT_DIR/process_verify.stdout.log" "$ARTIFACT_DIR/process_verify.stderr.log"
run_abc "$PROCESS_ABC" "$PROCESS_ENTRYPOINT" "$ARTIFACT_DIR/process_runtime_command.txt" "$ARTIFACT_DIR/process_runtime.stdout.log" "$ARTIFACT_DIR/process_runtime.stderr.log"
process_status=$?
[ $process_status -eq 0 ] || fail "interop helper validation runtime failed with exit $process_status; see $ARTIFACT_DIR/process_runtime.stderr.log"
assert_stdout_contains "$ARTIFACT_DIR/process_runtime.stdout.log" "$ARTIFACT_DIR/process_runtime.stderr.log" "$ARTIFACT_DIR/process_stdout_assertion.txt" "config-resolution-ok"
[ $? -eq 0 ] || fail "interop helper runtime assertions failed; see $ARTIFACT_DIR/process_stdout_assertion.txt"

printf 'PASS %s\n' "$TASK_ID"
