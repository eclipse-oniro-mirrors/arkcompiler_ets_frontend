#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_SYSTEM="$REPO_ROOT"
OUT_DIR="${ARTIFACT_DIR:-$SCRIPT_DIR/out}"
ES2PANDA="${ES2PANDA:-/home/huawei/w/PANDA/out/debug/bin/es2panda}"
ARK="${ARK:-/home/huawei/w/PANDA/out/debug/bin/ark}"
ARK_DISASM="${ARK_DISASM:-/home/huawei/w/PANDA/out/debug/bin/ark_disasm}"

mkdir -p "$OUT_DIR"
rm -f "$OUT_DIR"/*.abc "$OUT_DIR"/*.pa "$OUT_DIR"/*.ets "$OUT_DIR"/*.log "$OUT_DIR"/*.status

printf 'offline deterministic validation: network disabled; no external devices; no provider credentials\n' | tee "$OUT_DIR/mode.log"

fail() {
  printf 'VALIDATION_FAIL: %s\n' "$1" >&2
  exit 1
}

require_executable() {
  local tool="$1"
  [[ -x "$tool" ]] || fail "required executable not found: $tool"
}

compile_ets() {
  local src="$1"
  local out="$2"
  local log="$3"
  "$ES2PANDA" --ets-module --arktsconfig "$BUILD_SYSTEM/arktsconfig.json" --output "$out" "$src" >"$log" 2>&1 || {
    cat "$log" >&2
    fail "es2panda failed for $src"
  }
  [[ -s "$out" ]] || fail "compiled ABC is missing or empty: $out"
}

disassemble_abc() {
  local abc="$1"
  local pa="$2"
  local log="$3"
  "$ARK_DISASM" "$abc" "$pa" >"$log" 2>&1 || {
    cat "$log" >&2
    fail "ark_disasm failed for $abc"
  }
  [[ -s "$pa" ]] || fail "disassembly is missing or empty: $pa"
}

require_executable "$ES2PANDA"
require_executable "$ARK_DISASM"
[[ -x "$ARK" ]] || printf 'RUNTIME_BLOCKED: ark executable not available: %s\n' "$ARK" | tee "$OUT_DIR/runtime.status"

BASE_MODE="$BUILD_SYSTEM/ets_src/build/base_mode.ets"
BASE_TEST="$BUILD_SYSTEM/test/ets_ut/base_mode_sequential_test.ets"
DEMO_CONFIG="$BUILD_SYSTEM/test/demo_hap/build_config.json"

[[ -f "$BASE_MODE" ]] || fail "missing production BaseMode source"
[[ -f "$BASE_TEST" ]] || fail "missing BaseMode sequential contract test"
[[ -f "$DEMO_CONFIG" ]] || fail "missing demo_hap build_config.json"

compile_ets "$BUILD_SYSTEM/ets_src/entry.ets" "$OUT_DIR/build_system.abc" "$OUT_DIR/build_system.compile.log"
disassemble_abc "$OUT_DIR/build_system.abc" "$OUT_DIR/build_system.pa" "$OUT_DIR/build_system.disasm.log"

compile_ets "$BASE_TEST" "$OUT_DIR/base_mode_sequential_test.abc" "$OUT_DIR/base_mode_sequential_test.compile.log"
disassemble_abc "$OUT_DIR/base_mode_sequential_test.abc" "$OUT_DIR/base_mode_sequential_test.pa" "$OUT_DIR/base_mode_sequential_test.disasm.log"

grep -q 'base_mode_sequential_test.ETSGLOBAL.main' "$OUT_DIR/base_mode_sequential_test.pa" || fail "BaseMode sequential test ABC does not contain main entrypoint"
grep -q 'base-mode-sequential-ok' "$OUT_DIR/base_mode_sequential_test.pa" || fail "BaseMode sequential test success marker missing from ABC"

compile_ets "$BUILD_SYSTEM/test/ets_ut/graph_utils_test.ets" "$OUT_DIR/graph_utils_test.abc" "$OUT_DIR/graph_utils_test.compile.log"
compile_ets "$BUILD_SYSTEM/test/ets_ut/ets2panda_native_adapter_contract_test.ets" "$OUT_DIR/ets2panda_native_adapter_contract_test.abc" "$OUT_DIR/ets2panda_native_adapter_contract_test.compile.log"

if grep -R -n -E 'compile_process_worker|compile_thread_worker|TaskManager' "$BUILD_SYSTEM/ets_src" >"$OUT_DIR/decommission_refs.log" 2>&1; then
  cat "$OUT_DIR/decommission_refs.log" >&2
  fail "ArkTS driver still references decommissioned worker/task-manager surfaces"
fi

for target in \
  "$BUILD_SYSTEM/src/build/compile_process_worker.ts" \
  "$BUILD_SYSTEM/src/build/compile_thread_worker.ts" \
  "$BUILD_SYSTEM/src/util/TaskManager.ts"; do
  [[ -f "$target" ]] || fail "legacy reference target is unexpectedly missing: $target"
done

line_validate=$(grep -n 'this.validateGraph(units)' "$BASE_MODE" | head -n1 | cut -d: -f1)
line_generator=$(grep -n 'new ArkTSConfigGenerator' "$BASE_MODE" | head -n1 | cut -d: -f1)
[[ -n "$line_validate" && -n "$line_generator" ]] || fail "BaseMode.run does not expose validation-before-config-generation structure"
(( line_validate < line_generator )) || fail "BaseMode creates ArkTSConfigGenerator before graph validation"

grep -q "compileExternalSourceSet" "$BASE_MODE" || fail "BaseMode does not dispatch through in-process external source-set compiler"
grep -q "missing-output-abc" "$BASE_MODE" || fail "BaseMode output validation does not preserve missing-output-abc reason"
grep -q "empty-output-abc" "$BUILD_SYSTEM/ets_src/util/ets2panda.ets" || fail "native adapter does not preserve empty-output-abc reason"
grep -q "\['d.ets', 'c.ets', 'b.ets', 'a.ets'\]" "$BASE_MODE" || fail "entry source order d,c,b,a is not materialized in BaseMode"
grep -q "mode.collectCompileUnits" "$BASE_TEST" || fail "BaseMode test does not exercise compile-unit collection"
grep -q "Unresolved dependency" "$BASE_TEST" || fail "BaseMode test does not exercise unresolved dependency rejection"
grep -q "Cyclic" "$BASE_TEST" || fail "BaseMode test does not exercise cycle rejection"

cat >"$OUT_DIR/console_baseline.ets" <<'ETS'
function main(): void {
  console.log("ark-console-baseline-ok");
}
ETS
compile_ets "$OUT_DIR/console_baseline.ets" "$OUT_DIR/console_baseline.abc" "$OUT_DIR/console_baseline.compile.log"

runtime_ok=0
if [[ -x "$ARK" ]]; then
  set +e
  "$ARK" "$OUT_DIR/console_baseline.abc" console_baseline.ETSGLOBAL::main >"$OUT_DIR/console_baseline.run.log" 2>&1
  runtime_rc=$?
  set -e
  if [[ $runtime_rc -eq 0 ]] && grep -q 'ark-console-baseline-ok' "$OUT_DIR/console_baseline.run.log"; then
    runtime_ok=1
    printf 'RUNTIME_BASELINE_OK\n' | tee "$OUT_DIR/runtime.status"
  else
    printf 'RUNTIME_BLOCKED: ark console baseline failed with exit %s; see %s\n' "$runtime_rc" "$OUT_DIR/console_baseline.run.log" | tee "$OUT_DIR/runtime.status"
  fi
fi

if [[ $runtime_ok -eq 1 ]]; then
  "$ARK" "$OUT_DIR/base_mode_sequential_test.abc" base_mode_sequential_test.ETSGLOBAL::main >"$OUT_DIR/base_mode_sequential_test.run.log" 2>&1 || {
    cat "$OUT_DIR/base_mode_sequential_test.run.log" >&2
    fail "BaseMode sequential runtime contract test failed"
  }
  grep -q 'base-mode-sequential-ok' "$OUT_DIR/base_mode_sequential_test.run.log" || fail "BaseMode runtime test did not emit success marker"

  if [[ "${ARK_VALIDATE_RUNTIME:-0}" == "1" ]]; then
    cat >"$OUT_DIR/demo_driver_smoke.ets" <<ETS
import { build } from '$BUILD_SYSTEM/ets_src/entry';
function main(): void {
  if (!build('$DEMO_CONFIG')) {
    throw new Error('demo_hap build failed');
  }
  console.log('demo-hap-driver-ok');
}
ETS
    compile_ets "$OUT_DIR/demo_driver_smoke.ets" "$OUT_DIR/demo_driver_smoke.abc" "$OUT_DIR/demo_driver_smoke.compile.log"
    rm -rf "$BUILD_SYSTEM/test/demo_hap/dist"
    absolute_path_to_build_system="$BUILD_SYSTEM" "$ARK" "$OUT_DIR/demo_driver_smoke.abc" demo_driver_smoke.ETSGLOBAL::main >"$OUT_DIR/demo_driver_smoke.run.log" 2>&1 || {
      cat "$OUT_DIR/demo_driver_smoke.run.log" >&2
      fail "demo_hap driver smoke failed"
    }
    for abc in "$BUILD_SYSTEM/test/demo_hap/dist/harB.abc" "$BUILD_SYSTEM/test/demo_hap/dist/harA.abc" "$BUILD_SYSTEM/test/demo_hap/dist/entry.abc"; do
      [[ -s "$abc" ]] || fail "expected non-empty demo_hap artifact missing: $abc"
      stat -c '%n %s' "$abc" | tee -a "$OUT_DIR/demo_artifacts.stat"
    done
  fi
fi

printf 'VALIDATION_PASS: compile/import/decommission checks passed; runtime_status=%s\n' "$(cat "$OUT_DIR/runtime.status" 2>/dev/null || printf 'not-run')"
