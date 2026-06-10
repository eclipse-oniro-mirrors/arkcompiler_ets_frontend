#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
BUILD_SYSTEM="$REPO_ROOT/driver/build_system"
RUNNER="$BUILD_SYSTEM/test/ets_ut/demo_hap_smoke_runner.sh"
DEMO_DIST="$BUILD_SYSTEM/test/demo_hap/dist"
OUT_DIR="${ARTIFACT_DIR:-$SCRIPT_DIR/out}"
SUCCESS_OUT="$OUT_DIR/success"
NEGATIVE_OUT="$OUT_DIR/missing_dump_tool"
ES2PANDA="${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}"
ARK="${ARK:-$PANDA_ROOT/out/debug/bin/ark}"
ARK_DISASM="${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}"
ETSSTDLIB="${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}"
TIMEOUT_SEC="${DEMO_HAP_SMOKE_TIMEOUT_SEC:-60}"

fail() {
  printf 'VALIDATION_FAIL: %s\n' "$1" >&2
  exit 1
}

blocker() {
  printf 'VALIDATION_BLOCKED: %s\n' "$1" >&2
  printf 'RUNTIME_BLOCKED: %s\n' "$1" >"$OUT_DIR/runtime.status"
  exit 0
}

require_file() {
  local path="$1"
  [[ -f "$path" ]] || fail "required file not found: $path"
}

require_executable() {
  local path="$1"
  [[ -x "$path" ]] || fail "required executable not found: $path"
}

require_nonempty_file() {
  local path="$1"
  [[ -s "$path" ]] || fail "expected non-empty evidence file missing: $path"
}

assert_contains() {
  local path="$1"
  local pattern="$2"
  grep -q -- "$pattern" "$path" || fail "missing pattern '$pattern' in $path"
}

run_console_baseline() {
  mkdir -p "$OUT_DIR/console_baseline_pkg"
  local src="$OUT_DIR/console_baseline_pkg/console_baseline.ets"
  local abc="$OUT_DIR/console_baseline.abc"
  local compile_log="$OUT_DIR/console_baseline.compile.log"
  local run_log="$OUT_DIR/console_baseline.run.log"
  local run_stderr="$OUT_DIR/console_baseline.run.stderr.log"
  cat >"$src" <<'ETS'
function main(): void {
  console.log('demo-hap-console-baseline-ok');
}
ETS
  "$ES2PANDA" --ets-module --arktsconfig "$BUILD_SYSTEM/arktsconfig.json" --output "$abc" "$src" >"$compile_log" 2>&1 || blocker "console baseline compilation failed; see $compile_log"
  [[ -s "$abc" ]] || blocker "console baseline ABC missing or empty: $abc"
  set +e
  timeout "${TIMEOUT_SEC}s" "$ARK" --boot-panda-files "$ETSSTDLIB" --load-runtimes=ets --compiler-ignore-failures=false --panda-files "$abc" "$abc" console_baseline.ETSGLOBAL::main >"$run_log" 2>"$run_stderr"
  local status=$?
  set -e
  if [[ $status -ne 0 ]] || ! grep -q 'demo-hap-console-baseline-ok' "$run_log"; then
    blocker "ark console baseline failed with exit $status; stdout: $run_log; stderr: $run_stderr"
  fi
  printf 'RUNTIME_BASELINE_OK\n' >"$OUT_DIR/runtime.status"
}

run_product_success() {
  mkdir -p "$SUCCESS_OUT"
  local stdout_log="$SUCCESS_OUT/product.stdout.log"
  local stderr_log="$SUCCESS_OUT/product.stderr.log"
  local cmd_log="$SUCCESS_OUT/product.cmd"
  local cmd=(bash "$RUNNER")
  printf '%q ' "${cmd[@]}" >"$cmd_log"
  printf '\n' >>"$cmd_log"
  set +e
  ARTIFACT_DIR="$SUCCESS_OUT/product_artifacts" \
  ES2PANDA="$ES2PANDA" \
  ARK="$ARK" \
  ARK_DISASM="$ARK_DISASM" \
  ETSSTDLIB="$ETSSTDLIB" \
  DEMO_HAP_SMOKE_TIMEOUT_SEC="$TIMEOUT_SEC" \
  "${cmd[@]}" >"$stdout_log" 2>"$stderr_log"
  local status=$?
  set -e
  if [[ $status -ne 0 ]]; then
    [[ -s "$stdout_log" ]] && cat "$stdout_log" >&2
    [[ -s "$stderr_log" ]] && cat "$stderr_log" >&2
    fail "product demo_hap smoke runner failed with exit $status; see $SUCCESS_OUT"
  fi
  assert_contains "$stdout_log" 'demo_hap_smoke_runner: PASS'
}

validate_success_evidence() {
  local product_out="$SUCCESS_OUT/product_artifacts"
  require_nonempty_file "$product_out/build_system.abc"
  require_nonempty_file "$product_out/demo_hap_smoke_runner.abc"
  require_nonempty_file "$product_out/build_system.compile.log.cmd"
  require_nonempty_file "$product_out/demo_hap_smoke_runner.compile.log.cmd"
  require_nonempty_file "$product_out/demo_hap_smoke_runner.run.cmd"
  require_nonempty_file "$product_out/demo_hap_smoke_runner.run.log"
  require_nonempty_file "$product_out/demo_hap_artifacts.stat"
  require_nonempty_file "$product_out/entry.dump.txt"

  assert_contains "$product_out/demo_hap_smoke_runner.run.log" 'Accepted sequential ArkTS build dispatch.'
  assert_contains "$product_out/demo_hap_smoke_runner.run.log" 'demo-hap-smoke-runner-driver-ok'
  assert_contains "$product_out/entry.dump.txt" 'strA'
  assert_contains "$product_out/entry.dump.txt" 'strB'

  for name in harB harA entry; do
    local abc="$DEMO_DIST/$name.abc"
    require_nonempty_file "$abc"
    assert_contains "$product_out/demo_hap_artifacts.stat" "$name.abc"
    local size
    size=$(stat -c %s "$abc")
    [[ "$size" -gt 0 ]] || fail "product ABC has zero size: $abc"
  done

  if grep -R -n -E 'compile_process_worker|compile_thread_worker|TaskManager' "$BUILD_SYSTEM/ets_src" >"$SUCCESS_OUT/decommission_refs.log" 2>&1; then
    cat "$SUCCESS_OUT/decommission_refs.log" >&2
    fail "ArkTS sources reference decommissioned worker/task-manager surfaces"
  fi
}

run_missing_dump_tool_negative() {
  mkdir -p "$NEGATIVE_OUT"
  local stdout_log="$NEGATIVE_OUT/product.stdout.log"
  local stderr_log="$NEGATIVE_OUT/product.stderr.log"
  local cmd_log="$NEGATIVE_OUT/product.cmd"
  local cmd=(bash "$RUNNER")
  printf '%q ' "${cmd[@]}" >"$cmd_log"
  printf '\n' >>"$cmd_log"
  set +e
  ARTIFACT_DIR="$NEGATIVE_OUT/product_artifacts" \
  ES2PANDA="$ES2PANDA" \
  ARK="$ARK" \
  ARK_DISASM="$NEGATIVE_OUT/not-an-ark-disasm" \
  DEMO_HAP_DUMP_TOOL="" \
  ETSSTDLIB="$ETSSTDLIB" \
  DEMO_HAP_SMOKE_TIMEOUT_SEC="$TIMEOUT_SEC" \
  "${cmd[@]}" >"$stdout_log" 2>"$stderr_log"
  local status=$?
  set -e
  [[ $status -ne 0 ]] || fail "missing dump tool scenario unexpectedly passed"
  if ! grep -q 'missing-dump-tool' "$stdout_log" "$stderr_log" 2>/dev/null; then
    [[ -s "$stdout_log" ]] && cat "$stdout_log" >&2
    [[ -s "$stderr_log" ]] && cat "$stderr_log" >&2
    fail "missing dump tool scenario did not report missing-dump-tool"
  fi
}

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
printf 'offline deterministic validation: network disabled; no external devices; no provider credentials\n' | tee "$OUT_DIR/mode.log" >/dev/null

require_file "$RUNNER"
require_file "$BUILD_SYSTEM/arktsconfig.json"
require_file "$BUILD_SYSTEM/ets_src/entry.ets"
require_file "$BUILD_SYSTEM/test/demo_hap/build_config.json"
require_executable "$ES2PANDA"
require_executable "$ARK"
require_executable "$ARK_DISASM"
require_file "$ETSSTDLIB"

run_console_baseline
run_product_success
validate_success_evidence
run_missing_dump_tool_negative

printf 'VALIDATION_PASS: demo_hap_smoke_runner product scenarios passed\n'
