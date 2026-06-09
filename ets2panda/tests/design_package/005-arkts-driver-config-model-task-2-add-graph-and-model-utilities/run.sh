#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="005-arkts_driver_config_model-task-2-add-graph-and-model-utilities"
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

write_config() {
  python3 - "$1" "$2" "$3" "$PROD_ARKTSCONFIG" <<'PY'
import json
import pathlib
import sys
workdir = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
package = sys.argv[3]
prod_cfg = json.loads(pathlib.Path(sys.argv[4]).read_text(encoding="utf-8"))
prod_options = prod_cfg.get("compilerOptions", {})
patterns = [str(path) for path in sorted(workdir.rglob("*.ets"))]
cfg = {
    "include": patterns,
    "compilerOptions": {
        "package": package,
        "baseUrl": str(workdir),
        "rootDir": str(workdir),
        "cacheDir": str(workdir / "__etscache"),
        "paths": prod_options.get("paths", {}),
        "dependencies": prod_options.get("dependencies", {}),
    },
    "references": prod_cfg.get("references", []),
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
  shift 6
  local cmd=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$config" --output "$output" "$@")
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
    f"expected: {expected}\n"
    f"stdout:\n{stdout}\n"
    f"stderr:\n{stderr}\n",
    encoding="utf-8",
)
sys.exit(1)
PY
}

mkdir -p "$ARTIFACT_DIR" || fail "failed to create artifact directory $ARTIFACT_DIR"
require_file "$PROD_SRC/util/graph.ets" "production graph source not found"
require_file "$PROD_SRC/util/error.ets" "production error source not found"
require_file "$PROD_SRC/util/utils.ets" "production utils source not found"
require_file "$PROD_SRC/logger.ets" "production logger source not found"
require_file "$PROD_SRC/pre_define.ets" "production constants source not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"
require_executable_or_blocker "$ES2PANDA" "es2panda not executable"
require_executable_or_blocker "$ARK" "ark runtime not executable"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm not executable"
require_executable_or_blocker "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

info "offline deterministic validation; network, external providers, and devices are disabled"
info "validating production graph/error utility surfaces under $PROD_SRC/util"

BASELINE_DIR="$ARTIFACT_DIR/console_baseline_pkg"
mkdir -p "$BASELINE_DIR" || fail "failed to create baseline package"
cat > "$BASELINE_DIR/baseline.ets" <<'EOF_BASELINE'
function main(): void {
  console.log('console-baseline-ok');
}
EOF_BASELINE
BASELINE_CONFIG="$ARTIFACT_DIR/console_baseline_arktsconfig.json"
BASELINE_ABC="$ARTIFACT_DIR/console_baseline.abc"
write_config "$BASELINE_DIR" "$BASELINE_CONFIG" "@val_graph_baseline"
compile_package "console_baseline" "$BASELINE_CONFIG" "$BASELINE_ABC" "$ARTIFACT_DIR/console_baseline_compile.stdout.log" "$ARTIFACT_DIR/console_baseline_compile.stderr.log" "$ARTIFACT_DIR/console_baseline_compile_command.txt" "$BASELINE_DIR/baseline.ets"
BASELINE_ENTRYPOINT=$(derive_entrypoint "$BASELINE_ABC" "$ARTIFACT_DIR/console_baseline.pa" ".baseline.")
[ -n "$BASELINE_ENTRYPOINT" ] || blocker "could not derive console baseline entrypoint"
verify_abc "console_baseline" "$BASELINE_ABC" "$ARTIFACT_DIR/console_baseline_verify_command.txt" "$ARTIFACT_DIR/console_baseline_verify.stdout.log" "$ARTIFACT_DIR/console_baseline_verify.stderr.log"
run_abc "$BASELINE_ABC" "$BASELINE_ENTRYPOINT" "$ARTIFACT_DIR/console_baseline_runtime_command.txt" "$ARTIFACT_DIR/console_baseline_runtime.stdout.log" "$ARTIFACT_DIR/console_baseline_runtime.stderr.log"
baseline_status=$?
if [ $baseline_status -ne 0 ]; then
  blocker "console baseline runtime failed with exit $baseline_status; see $ARTIFACT_DIR/console_baseline_runtime.stderr.log"
fi
assert_stdout_contains "$ARTIFACT_DIR/console_baseline_runtime.stdout.log" "$ARTIFACT_DIR/console_baseline_runtime.stderr.log" "$ARTIFACT_DIR/console_baseline_stdout_assertion.txt" "console-baseline-ok"
[ $? -eq 0 ] || blocker "console baseline did not produce expected stdout; see $ARTIFACT_DIR/console_baseline_stdout_assertion.txt"

VALIDATION_DIR="$ARTIFACT_DIR/graph_pkg"
mkdir -p "$VALIDATION_DIR/util" || fail "failed to create graph validation package"
cp "$PROD_SRC/logger.ets" "$VALIDATION_DIR/logger.ets" || fail "failed to copy production logger source"
cp "$PROD_SRC/pre_define.ets" "$VALIDATION_DIR/pre_define.ets" || fail "failed to copy production constants source"
cp "$PROD_SRC/util/graph.ets" "$VALIDATION_DIR/util/graph.ets" || fail "failed to copy production graph source"
cp "$PROD_SRC/util/error.ets" "$VALIDATION_DIR/util/error.ets" || fail "failed to copy production error source"
cp "$PROD_SRC/util/utils.ets" "$VALIDATION_DIR/util/utils.ets" || fail "failed to copy production utils source"
cat > "$VALIDATION_DIR/graph_driver.ets" <<'EOF_DRIVER'
import { Graph, GraphNode } from './util/graph';
import { DriverError, ErrorCode } from './util/error';

function assertCondition(condition: boolean, message: string): void {
  if (!condition) {
    throw new Error(message);
  }
}

function validateOrder(): void {
  let harB: GraphNode<string> = new GraphNode<string>('harB', 'harB');
  let harA: GraphNode<string> = new GraphNode<string>('harA', 'harA');
  harA.predecessors.add('harB');
  let graph: Graph<string> = Graph.createGraphFromNodes<string>([harA, harB]);
  let order: string[] = Graph.topologicalSort<string>(graph);
  assertCondition(order.length === 2, 'unexpected order length');
  assertCondition(order[0] === 'harB', 'harB must be emitted before harA');
  assertCondition(order[1] === 'harA', 'harA must be emitted after harB');
  console.log(order.join(','));
}

function validateCycleError(): void {
  let harB: GraphNode<string> = new GraphNode<string>('harB', 'harB');
  let harA: GraphNode<string> = new GraphNode<string>('harA', 'harA');
  harA.predecessors.add('harB');
  harB.predecessors.add('harA');
  let graph: Graph<string> = Graph.createGraphFromNodes<string>([harA, harB]);
  try {
    Graph.topologicalSort<string>(graph);
  } catch (error) {
    if (error instanceof DriverError) {
      let driverError: DriverError = error as DriverError;
      assertCondition(driverError.logData.code === ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'unexpected DriverError code');
      console.log('cycle-error:' + driverError.logData.code);
      return;
    }
    throw error;
  }
  throw new Error('cycle did not raise DriverError');
}

function main(): void {
  validateOrder();
  validateCycleError();
}
EOF_DRIVER

VALIDATION_CONFIG="$ARTIFACT_DIR/graph_arktsconfig.json"
VALIDATION_ABC="$ARTIFACT_DIR/graph_validation.abc"
write_config "$VALIDATION_DIR" "$VALIDATION_CONFIG" "@val_graph_utils"
compile_package "graph_validation" "$VALIDATION_CONFIG" "$VALIDATION_ABC" "$ARTIFACT_DIR/graph_compile.stdout.log" "$ARTIFACT_DIR/graph_compile.stderr.log" "$ARTIFACT_DIR/graph_compile_command.txt" "$VALIDATION_DIR/graph_driver.ets"
VALIDATION_ENTRYPOINT=$(derive_entrypoint "$VALIDATION_ABC" "$ARTIFACT_DIR/graph_validation.pa" ".graph_driver.")
[ -n "$VALIDATION_ENTRYPOINT" ] || fail "could not derive graph validation entrypoint"
verify_abc "graph_validation" "$VALIDATION_ABC" "$ARTIFACT_DIR/graph_verify_command.txt" "$ARTIFACT_DIR/graph_verify.stdout.log" "$ARTIFACT_DIR/graph_verify.stderr.log"
run_abc "$VALIDATION_ABC" "$VALIDATION_ENTRYPOINT" "$ARTIFACT_DIR/graph_runtime_command.txt" "$ARTIFACT_DIR/graph_runtime.stdout.log" "$ARTIFACT_DIR/graph_runtime.stderr.log"
graph_status=$?
[ $graph_status -eq 0 ] || fail "graph validation runtime failed with exit $graph_status; see $ARTIFACT_DIR/graph_runtime.stderr.log"
assert_stdout_contains "$ARTIFACT_DIR/graph_runtime.stdout.log" "$ARTIFACT_DIR/graph_runtime.stderr.log" "$ARTIFACT_DIR/graph_order_assertion.txt" "harB,harA"
[ $? -eq 0 ] || fail "graph order assertion failed; see $ARTIFACT_DIR/graph_order_assertion.txt"
assert_stdout_contains "$ARTIFACT_DIR/graph_runtime.stdout.log" "$ARTIFACT_DIR/graph_runtime.stderr.log" "$ARTIFACT_DIR/graph_cycle_assertion.txt" "cycle-error:11410030"
[ $? -eq 0 ] || fail "graph cycle DriverError assertion failed; see $ARTIFACT_DIR/graph_cycle_assertion.txt"

printf 'PASS %s\n' "$TASK_ID"
