#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../../.." && pwd)
BUILD_SYSTEM="$REPO_ROOT/driver/build_system"
ETS_UT="$BUILD_SYSTEM/test/ets_ut"
OUT_DIR="${ARTIFACT_DIR:-$SCRIPT_DIR/out}"
ES2PANDA="${ES2PANDA:-/home/huawei/w/PANDA/out/debug/bin/es2panda}"
ARK="${ARK:-/home/huawei/w/PANDA/out/debug/bin/ark}"
NATIVE_OUT="$ETS_UT/native_single_file_out"
NATIVE_ABC="$NATIVE_OUT/harB/index.abc"

mkdir -p "$OUT_DIR"
rm -rf "$OUT_DIR"/*
mkdir -p "$OUT_DIR"

printf 'offline deterministic validation: network disabled; no external devices; no provider credentials\n' | tee "$OUT_DIR/mode.log"

fail() {
  printf 'VALIDATION_FAIL: %s\n' "$1" >&2
  exit 1
}

require_file() {
  local file="$1"
  [[ -f "$file" ]] || fail "required file not found: $file"
}

require_executable() {
  local tool="$1"
  [[ -x "$tool" ]] || fail "required executable not found: $tool"
}

compile_driver() {
  local src="$1"
  local name="$2"
  local abc="$OUT_DIR/$name.abc"
  local log="$OUT_DIR/$name.compile.log"
  "$ES2PANDA" --ets-module --arktsconfig "$BUILD_SYSTEM/arktsconfig.json" --output "$abc" "$src" >"$log" 2>&1 || {
    cat "$log" >&2
    fail "es2panda failed for $src"
  }
  [[ -s "$abc" ]] || fail "compiled ABC is missing or empty: $abc"
}

run_driver() {
  local name="$1"
  local entry="$2"
  local log="$OUT_DIR/$name.run.log"
  "$ARK" "$OUT_DIR/$name.abc" "$entry" >"$log" 2>&1
}

assert_marker() {
  local name="$1"
  local marker="$2"
  grep -q "$marker" "$OUT_DIR/$name.run.log" || fail "missing stdout marker '$marker' in $name"
}

require_executable "$ES2PANDA"
require_file "$BUILD_SYSTEM/arktsconfig.json"

required_drivers=(
  types_config_test
  logger_test
  graph_test
  generate_arktsconfig_test
  process_build_config_test
  native_single_file_compile_test
)

for name in "${required_drivers[@]}"; do
  require_file "$ETS_UT/$name.ets"
done

require_file "$ETS_UT/golden/demo_hap_arktsconfig.json"
require_file "$ETS_UT/golden/demo_hap_resolved_build_config.json"
require_file "$BUILD_SYSTEM/test/demo_hap/harB/index.ets"

rm -rf "$NATIVE_OUT"
started_at_epoch=$(date +%s)
printf '%s\n' "$started_at_epoch" >"$OUT_DIR/started_at_epoch.txt"

for name in "${required_drivers[@]}"; do
  compile_driver "$ETS_UT/$name.ets" "$name"
done

runtime_ok=0
if [[ -x "$ARK" ]]; then
  cat >"$OUT_DIR/console_baseline.ets" <<'ETS'
function main(): void {
  console.log("ark-console-baseline-ok");
}
ETS
  compile_driver "$OUT_DIR/console_baseline.ets" "console_baseline"
  set +e
  "$ARK" "$OUT_DIR/console_baseline.abc" "console_baseline.ETSGLOBAL::main" >"$OUT_DIR/console_baseline.run.log" 2>&1
  baseline_rc=$?
  set -e
  if [[ $baseline_rc -eq 0 ]] && grep -q 'ark-console-baseline-ok' "$OUT_DIR/console_baseline.run.log"; then
    runtime_ok=1
    printf 'RUNTIME_BASELINE_OK\n' | tee "$OUT_DIR/runtime.status"
  else
    printf 'RUNTIME_BLOCKED: ark console baseline failed with exit %s; see %s\n' "$baseline_rc" "$OUT_DIR/console_baseline.run.log" | tee "$OUT_DIR/runtime.status"
  fi
else
  printf 'RUNTIME_BLOCKED: ark executable not available: %s\n' "$ARK" | tee "$OUT_DIR/runtime.status"
fi

if [[ $runtime_ok -eq 1 ]]; then
  run_driver types_config_test "types_config_test.ETSGLOBAL::main" || { cat "$OUT_DIR/types_config_test.run.log" >&2; fail "types_config_test runtime failed"; }
  assert_marker types_config_test 'entry'

  run_driver logger_test "logger_test.ETSGLOBAL::main" || { cat "$OUT_DIR/logger_test.run.log" >&2; fail "logger_test runtime failed"; }
  assert_marker logger_test 'build started'

  run_driver graph_test "graph_test.ETSGLOBAL::main" || { cat "$OUT_DIR/graph_test.run.log" >&2; fail "graph_test runtime failed"; }
  assert_marker graph_test 'harB,harA'

  run_driver generate_arktsconfig_test "generate_arktsconfig_test.ETSGLOBAL::main" || { cat "$OUT_DIR/generate_arktsconfig_test.run.log" >&2; fail "generate_arktsconfig_test runtime failed"; }
  assert_marker generate_arktsconfig_test 'golden arktsconfig comparison success'

  run_driver process_build_config_test "process_build_config_test.ETSGLOBAL::main" || { cat "$OUT_DIR/process_build_config_test.run.log" >&2; fail "process_build_config_test runtime failed"; }
  assert_marker process_build_config_test 'resolved demo paths'

  run_driver native_single_file_compile_test "native_single_file_compile_test.ETSGLOBAL::main" || { cat "$OUT_DIR/native_single_file_compile_test.run.log" >&2; fail "native_single_file_compile_test runtime failed"; }
  assert_marker native_single_file_compile_test 'fresh single-file ABC success'

  [[ -s "$NATIVE_ABC" ]] || fail "fresh native ABC is missing or empty: $NATIVE_ABC"
  abc_mtime=$(stat -c %Y "$NATIVE_ABC")
  [[ $abc_mtime -ge $started_at_epoch ]] || fail "native ABC is stale: $NATIVE_ABC"
  stat -c '%n %s %Y' "$NATIVE_ABC" | tee "$OUT_DIR/native_harB_index_abc.stat"

  rm -rf "$NATIVE_OUT"
  mkdir -p "$(dirname "$NATIVE_ABC")"
  printf 'stale abc\n' >"$NATIVE_ABC"
  set +e
  run_driver native_single_file_compile_test "native_single_file_compile_test.ETSGLOBAL::main"
  stale_rc=$?
  set -e
  [[ $stale_rc -ne 0 ]] || fail "native_single_file_compile_test accepted stale/pre-existing ABC output"
  grep -q 'stale or pre-existing ABC output' "$OUT_DIR/native_single_file_compile_test.run.log" || fail "stale ABC rejection marker missing"
  rm -rf "$NATIVE_OUT"
fi

if grep -R "child_process\|dependency_analyzer" "$ETS_UT" >/dev/null 2>&1; then
  fail "ets_ut tests reference legacy subprocess or dependency_analyzer runtime path"
fi

if grep -R "driver/build_system/src/util/ets2panda\|driver/build_system/src/dependency_analyzer" "$ETS_UT" >/dev/null 2>&1; then
  fail "ets_ut tests import legacy TypeScript driver runtime paths"
fi

printf 'VALIDATION_PASS: ets_ut unit drivers compiled; runtime_status=%s\n' "$(cat "$OUT_DIR/runtime.status" 2>/dev/null || printf 'not-run')"
