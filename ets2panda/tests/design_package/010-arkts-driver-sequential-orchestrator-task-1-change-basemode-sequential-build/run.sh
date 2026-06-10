#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="010-arkts_driver_sequential_orchestrator-task-1-change-basemode-sequential-build"
ARTIFACT_DIR="${ARTIFACT_DIR:-$SCENARIO_DIR/out}"
REPORT="$ARTIFACT_DIR/report.txt"
BLOCKED=0

ES2PANDA=${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}
ARK=${ARK:-$PANDA_ROOT/out/debug/bin/ark}
ARK_DISASM=${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}
ETSSTDLIB=${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}
BUILD_SYSTEM="$REPO_ROOT/driver/build_system"
PROD_ARKTSCONFIG="$BUILD_SYSTEM/arktsconfig.json"
PROD_SRC="$BUILD_SYSTEM/ets_src"
BASE_MODE="$PROD_SRC/build/base_mode.ets"
ENTRY="$PROD_SRC/entry.ets"
ADAPTER="$PROD_SRC/util/ets2panda.ets"
GRAPH="$PROD_SRC/util/graph.ets"
BASE_TEST="$BUILD_SYSTEM/test/ets_ut/base_mode_sequential_test.ets"
GRAPH_TEST="$BUILD_SYSTEM/test/ets_ut/graph_utils_test.ets"
ADAPTER_TEST="$BUILD_SYSTEM/test/ets_ut/ets2panda_native_adapter_contract_test.ets"
DEMO_CONFIG="$BUILD_SYSTEM/test/demo_hap/build_config.json"
TIMEOUT_SEC=60

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
  local source=$2
  local output=$3
  local stdout_log=$4
  local stderr_log=$5
  local cmd_log=$6
  local cmd=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$PROD_ARKTSCONFIG" --output "$output" "$source")
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
        "package": "@val_sequential_baseline",
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
rm -f "$ARTIFACT_DIR"/*
: > "$REPORT"
info "offline deterministic validation; network, external providers, live validation, and devices are disabled"
info "target=$REPO_ROOT"

require_file "$PROD_ARKTSCONFIG" "ArkTS config not found"
require_file "$BASE_MODE" "BaseMode source not found"
require_file "$ENTRY" "entry source not found"
require_file "$ADAPTER" "native adapter source not found"
require_file "$GRAPH" "graph source not found"
require_file "$BASE_TEST" "BaseMode sequential contract test not found"
require_file "$GRAPH_TEST" "graph utility test not found"
require_file "$ADAPTER_TEST" "native adapter contract test not found"
require_file "$DEMO_CONFIG" "demo_hap build config not found"
require_executable_or_blocker "$ES2PANDA" "es2panda executable not available"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm executable not available"

compile_arkts build_system_entry "$ENTRY" "$ARTIFACT_DIR/build_system_entry.abc" "$ARTIFACT_DIR/build_system_entry.stdout.log" "$ARTIFACT_DIR/build_system_entry.stderr.log" "$ARTIFACT_DIR/build_system_entry.cmd"
compile_arkts base_mode_sequential_test "$BASE_TEST" "$ARTIFACT_DIR/base_mode_sequential_test.abc" "$ARTIFACT_DIR/base_mode_sequential_test.stdout.log" "$ARTIFACT_DIR/base_mode_sequential_test.stderr.log" "$ARTIFACT_DIR/base_mode_sequential_test.cmd"
compile_arkts graph_utils_test "$GRAPH_TEST" "$ARTIFACT_DIR/graph_utils_test.abc" "$ARTIFACT_DIR/graph_utils_test.stdout.log" "$ARTIFACT_DIR/graph_utils_test.stderr.log" "$ARTIFACT_DIR/graph_utils_test.cmd"
compile_arkts ets2panda_native_adapter_contract_test "$ADAPTER_TEST" "$ARTIFACT_DIR/ets2panda_native_adapter_contract_test.abc" "$ARTIFACT_DIR/ets2panda_native_adapter_contract_test.stdout.log" "$ARTIFACT_DIR/ets2panda_native_adapter_contract_test.stderr.log" "$ARTIFACT_DIR/ets2panda_native_adapter_contract_test.cmd"
pass "canonical product entrypoint and ETS UT drivers compile to non-empty ABC"

"$ARK_DISASM" "$ARTIFACT_DIR/base_mode_sequential_test.abc" "$ARTIFACT_DIR/base_mode_sequential_test.pa" > "$ARTIFACT_DIR/base_mode_sequential_test.disasm.stdout.log" 2> "$ARTIFACT_DIR/base_mode_sequential_test.disasm.stderr.log"
[ $? -eq 0 ] || fail "ark_disasm failed for BaseMode sequential test"
[ -s "$ARTIFACT_DIR/base_mode_sequential_test.pa" ] || fail "BaseMode sequential test disassembly is empty"
grep -q 'base_mode_sequential_test.ETSGLOBAL.main' "$ARTIFACT_DIR/base_mode_sequential_test.pa" || fail "BaseMode sequential test ABC does not contain main entrypoint"
grep -q 'base-mode-sequential-ok' "$ARTIFACT_DIR/base_mode_sequential_test.pa" || fail "BaseMode sequential test success marker missing from ABC"
pass "BaseMode sequential contract ABC is readable and contains expected markers"

if grep -R -n -E 'compile_process_worker|compile_thread_worker|TaskManager' "$PROD_SRC" > "$ARTIFACT_DIR/decommission_refs.log" 2>&1; then
  fail "ArkTS driver still references decommissioned worker/task-manager surfaces; see $ARTIFACT_DIR/decommission_refs.log"
fi
for target in \
  "$BUILD_SYSTEM/src/build/compile_process_worker.ts" \
  "$BUILD_SYSTEM/src/build/compile_thread_worker.ts" \
  "$BUILD_SYSTEM/src/util/TaskManager.ts"; do
  [ -f "$target" ] || fail "legacy reference target is unexpectedly missing: $target"
done
pass "decommissioned worker and task-manager surfaces are not referenced by ArkTS driver sources"

python3 - "$BASE_MODE" "$BASE_TEST" "$ADAPTER" "$ARTIFACT_DIR/source_contract_assertion.txt" <<'PY'
import pathlib
import sys
base = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
test = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8", errors="replace")
adapter = pathlib.Path(sys.argv[3]).read_text(encoding="utf-8", errors="replace")
out = pathlib.Path(sys.argv[4])
def require(cond, msg):
    if not cond:
        out.write_text("FAIL " + msg + "\n", encoding="utf-8")
        sys.exit(1)
validate = base.find("this.validateGraph(units)")
generator = base.find("new ArkTSConfigGenerator")
require(validate >= 0 and generator >= 0 and validate < generator, "BaseMode.run must validate graph before ArkTSConfigGenerator construction")
require("compileExternalSourceSet" in base, "BaseMode must dispatch through in-process external source-set compiler")
require("missing-output-abc" in base, "BaseMode must preserve missing-output-abc output failure")
require("empty-output-abc" in adapter, "native adapter must preserve empty-output-abc output failure")
require("['d.ets', 'c.ets', 'b.ets', 'a.ets']" in base, "BaseMode must materialize entry source order d,c,b,a")
for marker in ["mode.collectCompileUnits", "Unresolved dependency", "Cyclic", "mode.validateGraph", "mode.makeCompileRequest"]:
    require(marker in test, "BaseMode contract test missing marker " + marker)
out.write_text("PASS BaseMode source contract checks passed\n", encoding="utf-8")
PY
[ $? -eq 0 ] || fail "BaseMode source contract assertion failed"
pass "graph-before-native, deterministic order, compiler dispatch, and graph-failure evidence are materialized"

write_baseline_package "$ARTIFACT_DIR/console_baseline_pkg" "$ARTIFACT_DIR/console_baseline_pkg/arktsconfig.json"
compile_baseline "$ARTIFACT_DIR/console_baseline_pkg/arktsconfig.json" "$ARTIFACT_DIR/console_baseline_pkg/baseline.ets" "$ARTIFACT_DIR/console_baseline.abc" "$ARTIFACT_DIR/console_baseline.stdout.log" "$ARTIFACT_DIR/console_baseline.stderr.log" "$ARTIFACT_DIR/console_baseline.cmd"
BASELINE_ENTRYPOINT=$(derive_entrypoint "$ARTIFACT_DIR/console_baseline.abc" "$ARTIFACT_DIR/console_baseline.pa") || blocker "could not derive console baseline entrypoint"
if [ -x "$ARK" ] && [ -f "$ETSSTDLIB" ]; then
  run_abc "$ARTIFACT_DIR/console_baseline.abc" "$BASELINE_ENTRYPOINT" "$ARTIFACT_DIR/console_baseline.run.cmd" "$ARTIFACT_DIR/console_baseline.run.stdout.log" "$ARTIFACT_DIR/console_baseline.run.stderr.log"
  runtime_rc=$?
  if [ $runtime_rc -eq 0 ] && grep -q 'console-baseline-ok' "$ARTIFACT_DIR/console_baseline.run.stdout.log"; then
    printf 'RUNTIME_BASELINE_OK\n' > "$ARTIFACT_DIR/runtime.status"
    BASE_TEST_ENTRYPOINT=$(derive_entrypoint "$ARTIFACT_DIR/base_mode_sequential_test.abc" "$ARTIFACT_DIR/base_mode_sequential_test.runtime.pa") || fail "could not derive BaseMode test entrypoint"
    run_abc "$ARTIFACT_DIR/base_mode_sequential_test.abc" "$BASE_TEST_ENTRYPOINT" "$ARTIFACT_DIR/base_mode_sequential_test.run.cmd" "$ARTIFACT_DIR/base_mode_sequential_test.run.stdout.log" "$ARTIFACT_DIR/base_mode_sequential_test.run.stderr.log"
    base_runtime_rc=$?
    [ $base_runtime_rc -eq 0 ] || fail "BaseMode sequential runtime contract test failed with exit $base_runtime_rc; see $ARTIFACT_DIR/base_mode_sequential_test.run.stderr.log"
    grep -q 'base-mode-sequential-ok' "$ARTIFACT_DIR/base_mode_sequential_test.run.stdout.log" || fail "BaseMode runtime test did not emit success marker"
    pass "BaseMode sequential runtime contract test passed"
  else
    printf 'RUNTIME_BLOCKED: ark console baseline failed with exit %s\n' "$runtime_rc" > "$ARTIFACT_DIR/runtime.status"
    info "runtime execution blocked by same-toolchain console baseline; compile/import/decommission checks still completed"
  fi
else
  printf 'RUNTIME_BLOCKED: ark or etsstdlib unavailable\n' > "$ARTIFACT_DIR/runtime.status"
  info "runtime execution blocked because ark or etsstdlib is unavailable"
fi

if [ "${ARK_VALIDATE_RUNTIME:-0}" = "1" ] && grep -q 'RUNTIME_BASELINE_OK' "$ARTIFACT_DIR/runtime.status"; then
  cat > "$ARTIFACT_DIR/demo_driver_smoke.ets" <<EOF_SMOKE
import { build } from '$ENTRY';
function main(): void {
  if (!build('$DEMO_CONFIG')) {
    throw new Error('demo_hap build failed');
  }
  console.log('demo-hap-driver-ok');
}
EOF_SMOKE
  compile_arkts demo_driver_smoke "$ARTIFACT_DIR/demo_driver_smoke.ets" "$ARTIFACT_DIR/demo_driver_smoke.abc" "$ARTIFACT_DIR/demo_driver_smoke.stdout.log" "$ARTIFACT_DIR/demo_driver_smoke.stderr.log" "$ARTIFACT_DIR/demo_driver_smoke.cmd"
  rm -rf "$BUILD_SYSTEM/test/demo_hap/dist"
  SMOKE_ENTRYPOINT=$(derive_entrypoint "$ARTIFACT_DIR/demo_driver_smoke.abc" "$ARTIFACT_DIR/demo_driver_smoke.pa") || fail "could not derive demo smoke entrypoint"
  run_abc "$ARTIFACT_DIR/demo_driver_smoke.abc" "$SMOKE_ENTRYPOINT" "$ARTIFACT_DIR/demo_driver_smoke.run.cmd" "$ARTIFACT_DIR/demo_driver_smoke.run.stdout.log" "$ARTIFACT_DIR/demo_driver_smoke.run.stderr.log"
  smoke_rc=$?
  [ $smoke_rc -eq 0 ] || fail "demo_hap driver smoke failed with exit $smoke_rc"
  for abc in "$BUILD_SYSTEM/test/demo_hap/dist/harB.abc" "$BUILD_SYSTEM/test/demo_hap/dist/harA.abc" "$BUILD_SYSTEM/test/demo_hap/dist/entry.abc"; do
    [ -s "$abc" ] || fail "expected non-empty demo_hap artifact missing: $abc"
    stat -c '%n %s' "$abc" | tee -a "$ARTIFACT_DIR/demo_artifacts.stat"
  done
  pass "demo_hap driver smoke produced non-empty ABC artifacts"
else
  info "demo_hap live artifact smoke skipped unless ARK_VALIDATE_RUNTIME=1 and runtime baseline is healthy"
fi

pass "VALIDATION_PASS $TASK_ID"
printf 'VALIDATION_PASS: %s; runtime_status=%s\n' "$TASK_ID" "$(cat "$ARTIFACT_DIR/runtime.status" 2>/dev/null || printf 'not-run')"
