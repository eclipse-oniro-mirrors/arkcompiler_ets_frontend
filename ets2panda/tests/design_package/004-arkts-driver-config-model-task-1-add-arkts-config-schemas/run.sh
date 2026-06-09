#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="004-arkts_driver_config_model-task-1-add-arkts-config-schemas"
WORK_ROOT=${TMPDIR:-/tmp/opencode}
ARTIFACT_DIR="$WORK_ROOT/${TASK_ID}-$$"
BLOCKED=0

ES2PANDA=${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}
ARK=${ARK:-$PANDA_ROOT/out/debug/bin/ark}
ARK_DISASM=${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}
ARK_VERIFIER=${ARK_VERIFIER:-$PANDA_ROOT/out/debug/bin/verifier}
ETSSTDLIB=${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}
PROD_ARKTSCONFIG="$REPO_ROOT/driver/build_system/arktsconfig.json"
TYPES_ETS="$REPO_ROOT/driver/build_system/ets_src/types.ets"
PRE_DEFINE_ETS="$REPO_ROOT/driver/build_system/ets_src/pre_define.ets"
TIMEOUT_SEC=45
EXPECTED_PACKAGE="entry.pkg"

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
normalized_lines = [line.strip() for line in stdout.replace("\r\n", "\n").split("\n") if line.strip()]
if expected in normalized_lines or expected in stdout:
    out.write_text("PASS stdout contains expected package name\n" + stdout, encoding="utf-8")
    sys.exit(0)
out.write_text(
    "FAIL stdout did not contain expected package name\n"
    f"expected: {expected}\n"
    f"stdout:\n{stdout}\n"
    f"stderr:\n{stderr}\n",
    encoding="utf-8",
)
sys.exit(1)
PY
}

mkdir -p "$ARTIFACT_DIR" || fail "failed to create artifact directory $ARTIFACT_DIR"
require_file "$TYPES_ETS" "production types source not found"
require_file "$PRE_DEFINE_ETS" "production constants source not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"
require_executable_or_blocker "$ES2PANDA" "es2panda not executable"
require_executable_or_blocker "$ARK" "ark runtime not executable"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm not executable"
require_executable_or_blocker "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

info "offline deterministic validation; network, external providers, and devices are disabled"
info "validating production config schema surfaces at $TYPES_ETS and $PRE_DEFINE_ETS"

BASELINE_DIR="$ARTIFACT_DIR/console_baseline_pkg"
mkdir -p "$BASELINE_DIR" || fail "failed to create baseline package"
cat > "$BASELINE_DIR/baseline.ets" <<'EOF_BASELINE'
function main(): void {
  console.log('console-baseline-ok');
}
EOF_BASELINE
BASELINE_CONFIG="$ARTIFACT_DIR/console_baseline_arktsconfig.json"
BASELINE_ABC="$ARTIFACT_DIR/console_baseline.abc"
write_config "$BASELINE_DIR" "$BASELINE_CONFIG" "@val_schema_baseline"
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

VALIDATION_DIR="$ARTIFACT_DIR/schema_pkg"
mkdir -p "$VALIDATION_DIR" || fail "failed to create schema validation package"
cp "$TYPES_ETS" "$VALIDATION_DIR/types.ets" || fail "failed to copy production types source"
cp "$PRE_DEFINE_ETS" "$VALIDATION_DIR/pre_define.ets" || fail "failed to copy production constants source"
cat > "$VALIDATION_DIR/schema_driver.ets" <<'EOF_DRIVER'
import { BuildConfig, DependencyModuleConfig } from './types';
import { ABC_SUFFIX } from './pre_define';

function main(): void {
  let dependency: DependencyModuleConfig = {
    packageName: 'har.pkg',
    moduleName: 'harA',
    moduleType: 'har',
    modulePath: '/tmp/harA',
    sourceRoots: ['src/main/ets'],
    entryFile: 'index.ets',
    language: '1.2',
  };
  let buildConfig: BuildConfig = {
    packageName: 'entry.pkg',
    moduleType: 'entry',
    moduleRootPath: '/tmp/entry',
    sourceRoots: ['src/main/ets'],
    entryFile: 'index.ets',
    dependencyModuleList: [dependency],
  };
  let suffix: string = ABC_SUFFIX;
  if (suffix.length == 0 || buildConfig.dependencyModuleList == undefined) {
    console.log('schema-construction-failed');
    return;
  }
  console.log(buildConfig.packageName);
}
EOF_DRIVER
VALIDATION_CONFIG="$ARTIFACT_DIR/schema_arktsconfig.json"
VALIDATION_ABC="$ARTIFACT_DIR/schema_driver.abc"
write_config "$VALIDATION_DIR" "$VALIDATION_CONFIG" "@val_schema"
compile_package "schema_driver" "$VALIDATION_CONFIG" "$VALIDATION_ABC" "$ARTIFACT_DIR/schema_compile.stdout.log" "$ARTIFACT_DIR/schema_compile.stderr.log" "$ARTIFACT_DIR/schema_compile_command.txt" "$VALIDATION_DIR/schema_driver.ets"
info "schema validation compilation succeeded, ABC size: $(stat -c '%s' "$VALIDATION_ABC")"
VALIDATION_ENTRYPOINT=$(derive_entrypoint "$VALIDATION_ABC" "$ARTIFACT_DIR/schema_driver.pa" ".schema_driver.")
[ -n "$VALIDATION_ENTRYPOINT" ] || fail "could not derive schema validation runtime entrypoint"
verify_abc "schema_driver" "$VALIDATION_ABC" "$ARTIFACT_DIR/schema_verify_command.txt" "$ARTIFACT_DIR/schema_verify.stdout.log" "$ARTIFACT_DIR/schema_verify.stderr.log"
run_abc "$VALIDATION_ABC" "$VALIDATION_ENTRYPOINT" "$ARTIFACT_DIR/schema_runtime_command.txt" "$ARTIFACT_DIR/schema_runtime.stdout.log" "$ARTIFACT_DIR/schema_runtime.stderr.log"
runtime_status=$?
[ $runtime_status -eq 0 ] || fail "schema validation runtime failed with exit $runtime_status; see $ARTIFACT_DIR/schema_runtime.stderr.log"
assert_stdout_contains "$ARTIFACT_DIR/schema_runtime.stdout.log" "$ARTIFACT_DIR/schema_runtime.stderr.log" "$ARTIFACT_DIR/schema_stdout_assertion.txt" "$EXPECTED_PACKAGE"
[ $? -eq 0 ] || fail "schema validation stdout assertion failed; see $ARTIFACT_DIR/schema_stdout_assertion.txt"

printf 'PASS %s\n' "$TASK_ID"
