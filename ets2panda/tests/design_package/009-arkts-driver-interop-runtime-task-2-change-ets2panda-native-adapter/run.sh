#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="009-arkts_driver_interop_runtime-task-2-change-ets2panda-native-adapter"
WORK_ROOT=${TMPDIR:-/tmp/opencode}
ARTIFACT_DIR="$WORK_ROOT/${TASK_ID}-$$"
REPORT="$ARTIFACT_DIR/report.txt"
BLOCKED=0

ES2PANDA=${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}
ARK=${ARK:-$PANDA_ROOT/out/debug/bin/ark}
ARK_DISASM=${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}
ARK_VERIFIER=${ARK_VERIFIER:-$PANDA_ROOT/out/debug/bin/verifier}
ETSSTDLIB=${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}
PROD_ARKTSCONFIG="$REPO_ROOT/driver/build_system/arktsconfig.json"
PROD_SRC="$REPO_ROOT/driver/build_system/ets_src"
ADAPTER="$PROD_SRC/util/ets2panda.ets"
INTEROP_HELPER="$PROD_SRC/util/interop_helper.ets"
BASE_MODE="$PROD_SRC/build/base_mode.ets"
ENTRY="$PROD_SRC/entry.ets"
CONTRACT_TEST="$REPO_ROOT/driver/build_system/test/ets_ut/ets2panda_native_adapter_contract_test.ets"
DEMO_HARB="$REPO_ROOT/driver/build_system/test/demo_hap/harB/index.ets"
TIMEOUT_SEC=60

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
  printf 'FAIL: %s\n' "$1" >> "$REPORT"
  exit 1
}

blocker() {
  BLOCKED=1
  printf 'ENVIRONMENT BLOCKER: %s\n' "$1" >&2
  printf 'BLOCKER: %s\n' "$1" >> "$REPORT"
  printf 'BLOCKED %s\n' "$TASK_ID"
  exit 0
}

info() {
  printf 'INFO: %s\n' "$1"
  printf 'INFO: %s\n' "$1" >> "$REPORT"
}

pass() {
  printf 'PASS: %s\n' "$1"
  printf 'PASS: %s\n' "$1" >> "$REPORT"
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
        "package": "@val_native_adapter_baseline",
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
: > "$REPORT"
info "offline deterministic validation; network, external providers, live validation, and devices are disabled"
info "target=$REPO_ROOT"

require_file "$ADAPTER" "native adapter source not found"
require_file "$INTEROP_HELPER" "interop helper source not found"
require_file "$BASE_MODE" "base mode source not found"
require_file "$ENTRY" "entry source not found"
require_file "$CONTRACT_TEST" "native adapter contract test source not found"
require_file "$DEMO_HARB" "demo harB source not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"

python3 - "$PROD_SRC" "$ADAPTER" "$INTEROP_HELPER" "$BASE_MODE" "$CONTRACT_TEST" "$ARTIFACT_DIR/source_contract_assertion.txt" <<'PY'
import pathlib
import re
import sys
src = pathlib.Path(sys.argv[1])
adapter = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8", errors="replace")
interop = pathlib.Path(sys.argv[3]).read_text(encoding="utf-8", errors="replace")
base = pathlib.Path(sys.argv[4]).read_text(encoding="utf-8", errors="replace")
contract = pathlib.Path(sys.argv[5]).read_text(encoding="utf-8", errors="replace")
out = pathlib.Path(sys.argv[6])
failures = []
required_adapter = [
    "NativeCompilerConfigRequest",
    "argv: string[]",
    "configText: string",
    "compileFile(request: CompileUnitRequest)",
    "compileExternalSourceSet(request: CompileUnitRequest)",
    "_MemInitialize()",
    "_CreateConfig(argv.length, argv)",
    "_CreateContextFromFile",
    "_CreateContextGenerateAbcForExternalSourceFiles",
    "_ProceedToState",
    "_ContextState",
    "_ContextErrorMessage",
    "_GetAllErrorMessages",
    "_DestroyContext",
    "_DestroyConfig",
    "_MemFinalize",
    "ensureNonEmptyBinaryFile(request.outputPath",
    "ES2PANDA_STATE_PARSED",
    "ES2PANDA_STATE_BOUND",
    "ES2PANDA_STATE_CHECKED",
    "ES2PANDA_STATE_ASM_GENERATED",
    "ES2PANDA_STATE_BIN_GENERATED",
    "requestedState=",
    "observedState=",
    "contextErrorMessage=",
    "allErrorMessages=",
    "arkts-bindings-config-text-api-gap",
]
for token in required_adapter:
    if token not in adapter:
        failures.append(f"adapter missing {token}")
required_interop = [
    "global.interop._FileExists",
    "global.interop._FileSize",
    "fileExists(path: string)",
    "fileSize(path: string)",
    "ensureNonEmptyBinaryFile",
    "missing-output-abc",
    "empty-output-abc",
]
for token in required_interop:
    if token not in interop:
        failures.append(f"interop helper missing {token}")
if "new Ets2panda" not in base or ".compileExternalSourceSet(request)" not in base:
    failures.append("BaseMode does not consume native Ets2panda.compileExternalSourceSet")
run_match = re.search(r"public\s+run\s*\(\)\s*:\s*void\s*\{(?P<body>.*?)\n\s*\}", base, re.S)
if not run_match or "compileModule" not in run_match.group("body"):
    failures.append("BaseMode.run does not schedule module compilation")
required_contract = [
    "new Ets2panda().compileFile(request)",
    "driver/build_system/test/demo_hap/harB/index.ets",
    "driver/build_system/test/demo_hap/harB/index.abc",
    "NativeCompilerConfigRequest",
    "configText",
    "ets2panda-native-adapter-contract-ok",
]
for token in required_contract:
    if token not in contract:
        failures.append(f"adapter contract test missing {token}")
for forbidden in ["child_process", "spawn(", "exec(", "dependency_analyzer", "/bin/sh", "bash -c", "readTextFile(request.outputPath"]:
    offenders = []
    for path in src.rglob("*.ets"):
        if forbidden in path.read_text(encoding="utf-8", errors="replace"):
            offenders.append(str(path.relative_to(src.parent.parent)))
    if offenders:
        failures.append(f"forbidden token {forbidden!r} in {offenders}")
if failures:
    out.write_text("FAIL\n" + "\n".join(failures) + "\n", encoding="utf-8")
    print(out.read_text(encoding="utf-8"), end="")
    sys.exit(1)
out.write_text("PASS source contract validated\n", encoding="utf-8")
print(out.read_text(encoding="utf-8"), end="")
PY
[ $? -eq 0 ] || fail "source/native-adapter contract assertion failed; see $ARTIFACT_DIR/source_contract_assertion.txt"
pass "source/native-adapter contract validated"

require_executable_or_blocker "$ES2PANDA" "es2panda not executable"
require_executable_or_blocker "$ARK" "ark runtime not executable"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm not executable"
require_executable_or_blocker "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

ENTRY_ABC="$ARTIFACT_DIR/build_system_entry.abc"
compile_arkts "build_system_entry" "$ENTRY_ABC" "$ARTIFACT_DIR/entry_compile.stdout.log" "$ARTIFACT_DIR/entry_compile.stderr.log" "$ARTIFACT_DIR/entry_compile_command.txt" "$ENTRY"
pass "canonical ArkTS build-system entrypoint compiles"

CONTRACT_ABC="$ARTIFACT_DIR/ets2panda_native_adapter_contract_test.abc"
compile_arkts "ets2panda_native_adapter_contract_test" "$CONTRACT_ABC" "$ARTIFACT_DIR/contract_compile.stdout.log" "$ARTIFACT_DIR/contract_compile.stderr.log" "$ARTIFACT_DIR/contract_compile_command.txt" "$CONTRACT_TEST"
pass "native adapter contract test compiles"

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
pass "same-toolchain console baseline runs"

CONTRACT_ENTRYPOINT=$(derive_entrypoint "$CONTRACT_ABC" "$ARTIFACT_DIR/ets2panda_native_adapter_contract_test.pa")
[ -n "$CONTRACT_ENTRYPOINT" ] || fail "could not derive adapter contract test entrypoint"
verify_abc "ets2panda_native_adapter_contract_test" "$CONTRACT_ABC" "$ARTIFACT_DIR/contract_verify_command.txt" "$ARTIFACT_DIR/contract_verify.stdout.log" "$ARTIFACT_DIR/contract_verify.stderr.log"
run_abc "$CONTRACT_ABC" "$CONTRACT_ENTRYPOINT" "$ARTIFACT_DIR/contract_runtime_command.txt" "$ARTIFACT_DIR/contract_runtime.stdout.log" "$ARTIFACT_DIR/contract_runtime.stderr.log"
contract_status=$?
[ $contract_status -eq 0 ] || fail "native adapter contract runtime failed with exit $contract_status; see $ARTIFACT_DIR/contract_runtime.stderr.log"
assert_stdout_contains "$ARTIFACT_DIR/contract_runtime.stdout.log" "$ARTIFACT_DIR/contract_runtime.stderr.log" "$ARTIFACT_DIR/contract_stdout_assertion.txt" "ets2panda-native-adapter-contract-ok"
[ $? -eq 0 ] || fail "native adapter contract stdout assertion failed; see $ARTIFACT_DIR/contract_stdout_assertion.txt"
pass "native adapter contract runtime exercises harB compileFile"

printf 'PASS %s\n' "$TASK_ID"
