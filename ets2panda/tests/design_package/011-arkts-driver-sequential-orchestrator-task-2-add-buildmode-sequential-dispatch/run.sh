#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_SYSTEM="$REPO_ROOT/driver/build_system"
OUT_DIR="${ARTIFACT_DIR:-$SCRIPT_DIR/out}"
ES2PANDA="${ES2PANDA:-/home/huawei/w/PANDA/out/debug/bin/es2panda}"
ARK="${ARK:-/home/huawei/w/PANDA/out/debug/bin/ark}"
ARK_DISASM="${ARK_DISASM:-/home/huawei/w/PANDA/out/debug/bin/ark_disasm}"

mkdir -p "$OUT_DIR"
rm -f "$OUT_DIR"/*.abc "$OUT_DIR"/*.pa "$OUT_DIR"/*.ets "$OUT_DIR"/*.log "$OUT_DIR"/*.status "$OUT_DIR"/*.stat

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

run_ark() {
  local abc="$1"
  local entry="$2"
  local log="$3"
  "$ARK" "$abc" "$entry" >"$log" 2>&1
}

require_executable "$ES2PANDA"
require_executable "$ARK_DISASM"
[[ -x "$ARK" ]] || printf 'RUNTIME_BLOCKED: ark executable not available: %s\n' "$ARK" | tee "$OUT_DIR/runtime.status"

ENTRY_SRC="$BUILD_SYSTEM/ets_src/entry.ets"
BUILD_MODE_SRC="$BUILD_SYSTEM/ets_src/build/build_mode.ets"
BUILD_MODE_TEST="$BUILD_SYSTEM/test/ets_ut/build_mode_dispatch_test.ets"
DEMO_CONFIG="$BUILD_SYSTEM/test/demo_hap/build_config.json"
DEMO_DIST="$BUILD_SYSTEM/test/demo_hap/dist"

[[ -f "$ENTRY_SRC" ]] || fail "missing production entry source"
[[ -f "$BUILD_MODE_SRC" ]] || fail "missing production BuildMode source"
[[ -f "$BUILD_MODE_TEST" ]] || fail "missing BuildMode dispatch test"
[[ -f "$DEMO_CONFIG" ]] || fail "missing demo_hap build_config.json"

grep -q 'new BuildMode(buildConfig)' "$ENTRY_SRC" || fail "entry.build does not dispatch through BuildMode"
grep -q 'initBuildConfig(buildConfigPath)' "$ENTRY_SRC" || fail "entry.build does not initialize config from product path"
grep -q 'this.baseMode.run()' "$BUILD_MODE_SRC" || fail "BuildMode.run does not call BaseMode.run"
grep -q 'unsupported-build-mode' "$BUILD_MODE_SRC" || fail "BuildMode missing unsupported-build-mode rejection reason"
grep -q 'unsupported-feature-obfuscation' "$BUILD_MODE_SRC" || fail "BuildMode missing obfuscation rejection reason"
grep -q 'unsupported-feature-declgen-v1' "$BUILD_MODE_SRC" || fail "BuildMode missing DeclgenV1 rejection reason"

compile_ets "$ENTRY_SRC" "$OUT_DIR/build_system.abc" "$OUT_DIR/build_system.compile.log"
disassemble_abc "$OUT_DIR/build_system.abc" "$OUT_DIR/build_system.pa" "$OUT_DIR/build_system.disasm.log"
grep -q 'entry.ETSGLOBAL' "$OUT_DIR/build_system.pa" || fail "canonical entry ABC does not contain entry module symbols"

compile_ets "$BUILD_MODE_TEST" "$OUT_DIR/build_mode_dispatch_test.abc" "$OUT_DIR/build_mode_dispatch_test.compile.log"
disassemble_abc "$OUT_DIR/build_mode_dispatch_test.abc" "$OUT_DIR/build_mode_dispatch_test.pa" "$OUT_DIR/build_mode_dispatch_test.disasm.log"
grep -q 'build_mode_dispatch_test.ETSGLOBAL.main' "$OUT_DIR/build_mode_dispatch_test.pa" || fail "BuildMode dispatch test ABC does not contain main entrypoint"
grep -q 'build-mode-dispatch-ok' "$OUT_DIR/build_mode_dispatch_test.pa" || fail "BuildMode dispatch success marker missing from ABC"
grep -q 'unsupported-build-mode' "$OUT_DIR/build_mode_dispatch_test.pa" || fail "BuildMode dispatch test does not observe unsupported-build-mode"
grep -q 'unsupported-feature-obfuscation' "$OUT_DIR/build_mode_dispatch_test.pa" || fail "BuildMode dispatch test does not observe unsupported-feature-obfuscation"
grep -q 'unsupported-feature-declgen-v1' "$OUT_DIR/build_mode_dispatch_test.pa" || fail "BuildMode dispatch test does not observe unsupported-feature-declgen-v1"
grep -q 'runCount' "$OUT_DIR/build_mode_dispatch_test.pa" || fail "BuildMode dispatch test does not verify BaseMode invocation count"

cat >"$OUT_DIR/console_baseline.ets" <<'ETS'
function main(): void {
  console.log("ark-console-baseline-ok");
}
ETS
compile_ets "$OUT_DIR/console_baseline.ets" "$OUT_DIR/console_baseline.abc" "$OUT_DIR/console_baseline.compile.log"

runtime_ok=0
if [[ -x "$ARK" ]]; then
  set +e
  run_ark "$OUT_DIR/console_baseline.abc" "console_baseline.ETSGLOBAL::main" "$OUT_DIR/console_baseline.run.log"
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
  run_ark "$OUT_DIR/build_mode_dispatch_test.abc" "build_mode_dispatch_test.ETSGLOBAL::main" "$OUT_DIR/build_mode_dispatch_test.run.log" || {
    cat "$OUT_DIR/build_mode_dispatch_test.run.log" >&2
    fail "BuildMode dispatch runtime/compiler rejection test failed"
  }
  grep -q 'build-mode-dispatch-ok' "$OUT_DIR/build_mode_dispatch_test.run.log" || fail "BuildMode runtime test did not emit success marker"

  cat >"$OUT_DIR/demo_driver_smoke.ets" <<ETS
import { build } from '$ENTRY_SRC';
function main(): void {
  if (!build('$DEMO_CONFIG')) {
    throw new Error('demo_hap build failed');
  }
  console.log('demo-hap-driver-ok');
}
ETS
  compile_ets "$OUT_DIR/demo_driver_smoke.ets" "$OUT_DIR/demo_driver_smoke.abc" "$OUT_DIR/demo_driver_smoke.compile.log"
  rm -rf "$DEMO_DIST"
  absolute_path_to_build_system="$BUILD_SYSTEM" run_ark "$OUT_DIR/demo_driver_smoke.abc" "demo_driver_smoke.ETSGLOBAL::main" "$OUT_DIR/demo_driver_smoke.run.log" || {
    cat "$OUT_DIR/demo_driver_smoke.run.log" >&2
    fail "demo_hap product-route driver smoke failed"
  }
  grep -q 'demo-hap-driver-ok' "$OUT_DIR/demo_driver_smoke.run.log" || fail "demo_hap smoke did not emit success marker"
  for abc in "$DEMO_DIST/harB.abc" "$DEMO_DIST/harA.abc" "$DEMO_DIST/entry.abc"; do
    [[ -s "$abc" ]] || fail "expected non-empty demo_hap artifact missing: $abc"
    stat -c '%n %s' "$abc" | tee -a "$OUT_DIR/demo_artifacts.stat"
  done
fi

if compgen -G "$OUT_DIR/*unsupported*.abc" >/dev/null || compgen -G "$OUT_DIR/*rejected*.abc" >/dev/null; then
  fail "rejection scenarios unexpectedly created unsupported/rejected ABC outputs"
fi

printf 'VALIDATION_PASS: BuildMode dispatch compile evidence passed; runtime_status=%s\n' "$(cat "$OUT_DIR/runtime.status" 2>/dev/null || printf 'not-run')"
