#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD_SYSTEM=$(cd "$SCRIPT_DIR/../.." && pwd)
OUT_DIR="${ARTIFACT_DIR:-$SCRIPT_DIR/out/demo_hap_smoke_runner}"
DEMO_DIR="$BUILD_SYSTEM/test/demo_hap"
DEMO_DIST="$DEMO_DIR/dist"
DEMO_CONFIG="$DEMO_DIR/build_config.json"
ES2PANDA="${ES2PANDA:-/home/huawei/w/PANDA/out/debug/bin/es2panda}"
ARK="${ARK:-/home/huawei/w/PANDA/out/debug/bin/ark}"
ARK_DISASM="${ARK_DISASM:-/home/huawei/w/PANDA/out/debug/bin/ark_disasm}"
ETSSTDLIB="${ETSSTDLIB:-/home/huawei/w/PANDA/out/debug/plugins/ets/etsstdlib.abc}"
DUMP_TOOL="${DEMO_HAP_DUMP_TOOL:-}"
TIMEOUT_SEC="${DEMO_HAP_SMOKE_TIMEOUT_SEC:-60}"
DRIVER_ABC="$OUT_DIR/build_system.abc"
SMOKE_SRC_DIR="$SCRIPT_DIR/out/demo_hap_smoke_runner_src"
SMOKE_SRC="$SMOKE_SRC_DIR/demo_hap_smoke_runner.ets"
SMOKE_ABC="$OUT_DIR/demo_hap_smoke_runner.abc"
RUN_MARKER="$DEMO_DIST/.demo_hap_smoke_runner.marker"
ENTRY_ABC="$DEMO_DIST/entry.abc"
DUMP_OUT="$OUT_DIR/entry.dump.txt"

fail() {
  printf 'demo_hap_smoke_runner: %s\n' "$1" >&2
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
  local cmd_log="$log.cmd"
  local cmd=("$ES2PANDA" --ets-module --arktsconfig "$BUILD_SYSTEM/arktsconfig.json" --output "$out" "$src")
  printf '%q ' "${cmd[@]}" >"$cmd_log"
  printf '\n' >>"$cmd_log"
  "${cmd[@]}" >"$log" 2>&1 || {
    cat "$log" >&2
    fail "es2panda failed for $src; command log: $cmd_log; output log: $log"
  }
  [[ -s "$out" ]] || fail "compiled ABC missing or empty: $out"
}

run_smoke_abc() {
  local stdout_log="$OUT_DIR/demo_hap_smoke_runner.run.log"
  local stderr_log="$OUT_DIR/demo_hap_smoke_runner.run.stderr.log"
  local cmd_log="$OUT_DIR/demo_hap_smoke_runner.run.cmd"
  local cmd=("$ARK" --boot-panda-files "$ETSSTDLIB" --load-runtimes=ets --compiler-ignore-failures=false --panda-files "$SMOKE_ABC" "$SMOKE_ABC" demo_hap_smoke_runner.ETSGLOBAL::main)
  printf '%q ' "${cmd[@]}" >"$cmd_log"
  printf '\n' >>"$cmd_log"
  set +e
  absolute_path_to_build_system="$BUILD_SYSTEM" timeout "${TIMEOUT_SEC}s" "${cmd[@]}" >"$stdout_log" 2>"$stderr_log"
  local status=$?
  set -e
  if [[ $status -ne 0 ]]; then
    [[ -s "$stdout_log" ]] && cat "$stdout_log" >&2
    [[ -s "$stderr_log" ]] && cat "$stderr_log" >&2
    fail "compiled ArkTS demo_hap driver run failed with exit $status; command log: $cmd_log; stdout: $stdout_log; stderr: $stderr_log"
  fi
  cat "$stderr_log" >>"$stdout_log"
}

validate_fresh_abc() {
  local abc="$1"
  [[ -s "$abc" ]] || fail "fresh artifact missing or empty: $abc"
  [[ "$abc" -nt "$RUN_MARKER" ]] || fail "stale artifact not owned by current run: $abc"
  stat -c '%n %s' "$abc" | tee -a "$OUT_DIR/demo_hap_artifacts.stat" >/dev/null
}

inspect_entry_abc() {
  if [[ -x "$ARK_DISASM" ]]; then
    "$ARK_DISASM" "$ENTRY_ABC" "$OUT_DIR/entry.pa" >"$OUT_DIR/ark_disasm.log" 2>&1 || {
      cat "$OUT_DIR/ark_disasm.log" >&2
      fail "ark_disasm failed for $ENTRY_ABC"
    }
    cp "$OUT_DIR/entry.pa" "$DUMP_OUT"
  elif [[ -n "$DUMP_TOOL" && -x "$DUMP_TOOL" ]]; then
    "$DUMP_TOOL" "$ENTRY_ABC" >"$DUMP_OUT" 2>"$OUT_DIR/dump_tool.log" || {
      cat "$OUT_DIR/dump_tool.log" >&2
      fail "configured dump tool failed for $ENTRY_ABC"
    }
  else
    fail 'missing-dump-tool'
  fi
  [[ -s "$DUMP_OUT" ]] || fail "entry ABC dump is missing or empty"
  grep -q 'strA' "$DUMP_OUT" || fail "entry ABC inspection missing strA"
  grep -q 'strB' "$DUMP_OUT" || fail "entry ABC inspection missing strB"
}

require_executable "$ES2PANDA"
require_executable "$ARK"
[[ -f "$ETSSTDLIB" ]] || fail "required ETS stdlib not found: $ETSSTDLIB"
[[ -f "$DEMO_CONFIG" ]] || fail "missing demo_hap build_config.json"
[[ -f "$BUILD_SYSTEM/ets_src/entry.ets" ]] || fail "missing ArkTS driver entry source"

mkdir -p "$OUT_DIR"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
rm -rf "$SMOKE_SRC_DIR"
mkdir -p "$SMOKE_SRC_DIR"
rm -rf "$DEMO_DIST"
mkdir -p "$DEMO_DIST"
touch "$RUN_MARKER"

compile_ets "$BUILD_SYSTEM/ets_src/entry.ets" "$DRIVER_ABC" "$OUT_DIR/build_system.compile.log"

cat >"$SMOKE_SRC" <<ETS
import { build } from '../../../../ets_src/entry';
function main(): void {
  if (!build('$DEMO_CONFIG')) {
    throw new Error('demo_hap build failed');
  }
  console.log('demo-hap-smoke-runner-driver-ok');
}
ETS

compile_ets "$SMOKE_SRC" "$SMOKE_ABC" "$OUT_DIR/demo_hap_smoke_runner.compile.log"

run_smoke_abc

grep -q 'Accepted sequential ArkTS build dispatch.' "$OUT_DIR/demo_hap_smoke_runner.run.log" || fail "ArkTS sequential dispatch marker missing"
grep -q 'demo-hap-smoke-runner-driver-ok' "$OUT_DIR/demo_hap_smoke_runner.run.log" || fail "driver success marker missing"

validate_fresh_abc "$DEMO_DIST/harB.abc"
validate_fresh_abc "$DEMO_DIST/harA.abc"
validate_fresh_abc "$ENTRY_ABC"
inspect_entry_abc

if grep -R -n -E 'compile_process_worker|compile_thread_worker|TaskManager' "$BUILD_SYSTEM/ets_src" >"$OUT_DIR/decommission_refs.log" 2>&1; then
  cat "$OUT_DIR/decommission_refs.log" >&2
  fail "ArkTS smoke references decommissioned worker/task-manager surfaces"
fi

printf 'demo_hap_smoke_runner: PASS\n'
