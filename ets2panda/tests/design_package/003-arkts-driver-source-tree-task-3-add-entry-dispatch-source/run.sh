#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="003-arkts_driver_source_tree-task-3-add-entry-dispatch-source"
WORK_ROOT=${TMPDIR:-/tmp/opencode}
ARTIFACT_DIR="$WORK_ROOT/${TASK_ID}-$$"
BLOCKED=0

ES2PANDA=${ES2PANDA:-$PANDA_ROOT/out/debug/bin/es2panda}
ARK=${ARK:-$PANDA_ROOT/out/debug/bin/ark}
ARK_DISASM=${ARK_DISASM:-$PANDA_ROOT/out/debug/bin/ark_disasm}
ARK_VERIFIER=${ARK_VERIFIER:-$PANDA_ROOT/out/debug/bin/verifier}
ETSSTDLIB=${ETSSTDLIB:-$PANDA_ROOT/out/debug/plugins/ets/etsstdlib.abc}
PROD_ARKTSCONFIG="$REPO_ROOT/driver/build_system/arktsconfig.json"
ENTRY_ETS="$REPO_ROOT/driver/build_system/ets_src/entry.ets"
ETS_SRC="$REPO_ROOT/driver/build_system/ets_src"
DEMO_CONFIG="$REPO_ROOT/driver/build_system/test/demo_hap/build_config.json"
DEMO_ROOT="$REPO_ROOT/driver/build_system/test/demo_hap"
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
  local cmd=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$config" --output "$output")
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
  shift 5
  local cmd=("$ARK" --boot-panda-files "$ETSSTDLIB" --load-runtimes=ets --compiler-ignore-failures=false --panda-files "$abc" "$abc" "$entrypoint" "$@")
  printf '%q ' "${cmd[@]}" > "$cmd_log"
  printf '\n' >> "$cmd_log"
  timeout "${TIMEOUT_SEC}s" "${cmd[@]}" > "$stdout_log" 2> "$stderr_log"
  return $?
}

copy_product_sources() {
  local target=$1
  python3 - "$ETS_SRC" "$target" <<'PY'
import pathlib
import shutil
import sys
src = pathlib.Path(sys.argv[1])
dst = pathlib.Path(sys.argv[2])
for path in src.rglob("*.ets"):
    rel = path.relative_to(src)
    out = dst / rel
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, out)
PY
}

assert_entry_source_contract() {
  python3 - "$ENTRY_ETS" "$ARTIFACT_DIR/entry_source_contract.txt" <<'PY'
import pathlib
import re
import sys
entry = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
text = entry.read_text(encoding="utf-8")
checks = {
    "exports build(buildConfigPath: string): boolean": re.search(r"export\s+function\s+build\s*\(\s*buildConfigPath\s*:\s*string\s*\)\s*:\s*boolean", text) is not None,
    "calls initBuildConfig(buildConfigPath)": "initBuildConfig(buildConfigPath)" in text,
    "constructs BuildMode from resolved config": re.search(r"new\s+BuildMode\s*\(\s*buildConfig\s*\)", text) is not None,
    "dispatches BuildMode.run()": re.search(r"\bbuildMode\.run\s*\(\s*\)", text) is not None,
    "catches DriverError separately": "error instanceof DriverError" in text,
    "logs DriverError through Logger": "Logger.getInstance().printError(error.logData)" in text,
    "does not import Node.js-only modules": not re.search(r"from\s+['\"](?:node:)?(?:fs|path|os|process|child_process)['\"]", text),
}
missing = [name for name, ok in checks.items() if not ok]
if missing:
    out.write_text("FAIL entry source contract missing:\n" + "\n".join(missing) + "\n", encoding="utf-8")
    sys.exit(1)
out.write_text("PASS entry source contract satisfied\n", encoding="utf-8")
PY
}

assert_demo_artifacts() {
  python3 - "$DEMO_ROOT" "$ARTIFACT_DIR/demo_artifact_assertion.txt" <<'PY'
import pathlib
import sys
root = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
abc_files = [p for p in root.rglob("*.abc") if p.is_file() and p.stat().st_size > 0]
names = {p.name for p in abc_files}
entry_abcs = [p for p in abc_files if "entry" in {part.lower() for part in p.parts}]
missing = []
if "harB.abc" not in names:
    missing.append("harB.abc")
if "harA.abc" not in names:
    missing.append("harA.abc")
if not entry_abcs:
    missing.append("non-empty entry module .abc")
if missing:
    out.write_text(
        "FAIL missing expected demo ABC artifacts: " + ", ".join(missing) + "\n" +
        "Observed non-empty ABCs:\n" + "\n".join(str(p.relative_to(root)) for p in abc_files) + "\n",
        encoding="utf-8",
    )
    sys.exit(1)
out.write_text("PASS expected demo ABC artifacts present\n" + "\n".join(str(p.relative_to(root)) for p in abc_files) + "\n", encoding="utf-8")
PY
}

remove_demo_abcs() {
  python3 - "$DEMO_ROOT" <<'PY'
import pathlib
import sys
root = pathlib.Path(sys.argv[1])
for path in root.rglob("*.abc"):
    if path.is_file():
        path.unlink()
PY
}

mkdir -p "$ARTIFACT_DIR" || fail "failed to create artifact directory $ARTIFACT_DIR"
require_file "$ENTRY_ETS" "production entry source not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"
require_file "$DEMO_CONFIG" "demo_hap build_config.json not found"
require_executable_or_blocker "$ES2PANDA" "es2panda not executable"
require_executable_or_blocker "$ARK" "ark runtime not executable"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm not executable"
require_executable_or_blocker "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

info "offline deterministic validation; network, external providers, and devices are disabled"
info "validating production entry dispatch surface at $ENTRY_ETS"

assert_entry_source_contract || fail "entry dispatch source contract failed; see $ARTIFACT_DIR/entry_source_contract.txt"

CANONICAL_ABC="$ARTIFACT_DIR/build_system.abc"
CANONICAL_CMD=("$ES2PANDA" --extension=ets --ets-module --simultaneous --arktsconfig "$PROD_ARKTSCONFIG" --output "$CANONICAL_ABC" "$ENTRY_ETS")
printf '%q ' "${CANONICAL_CMD[@]}" > "$ARTIFACT_DIR/canonical_compile_command.txt"
printf '\n' >> "$ARTIFACT_DIR/canonical_compile_command.txt"
"${CANONICAL_CMD[@]}" > "$ARTIFACT_DIR/canonical_compile.stdout.log" 2> "$ARTIFACT_DIR/canonical_compile.stderr.log"
canonical_status=$?
[ $canonical_status -eq 0 ] || fail "canonical entry compile failed with exit $canonical_status; see $ARTIFACT_DIR/canonical_compile.stderr.log"
[ -s "$CANONICAL_ABC" ] || fail "canonical compiled driver ABC missing or empty: $CANONICAL_ABC"
info "canonical entry compile succeeded, ABC size: $(stat -c '%s' "$CANONICAL_ABC")"

BASELINE_DIR="$ARTIFACT_DIR/baseline_pkg"
mkdir -p "$BASELINE_DIR" || fail "failed to create baseline package"
python3 - "$BASELINE_DIR/baseline_driver.ets" <<'PY'
import pathlib
import sys
pathlib.Path(sys.argv[1]).write_text("function main(): void {\n  console.log('baseline ok');\n}\n", encoding="utf-8")
PY
BASELINE_CONFIG="$ARTIFACT_DIR/baseline_arktsconfig.json"
BASELINE_ABC="$ARTIFACT_DIR/baseline_driver.abc"
write_config "$BASELINE_DIR" "$BASELINE_CONFIG" "@val_entry_baseline"
compile_package "baseline" "$BASELINE_CONFIG" "$BASELINE_ABC" "$ARTIFACT_DIR/baseline_compile.stdout.log" "$ARTIFACT_DIR/baseline_compile.stderr.log" "$ARTIFACT_DIR/baseline_compile_command.txt"
BASELINE_ENTRYPOINT=$(derive_entrypoint "$BASELINE_ABC" "$ARTIFACT_DIR/baseline_driver.pa" ".baseline_driver.")
[ -n "$BASELINE_ENTRYPOINT" ] || fail "could not derive baseline runtime entrypoint"
verify_abc "baseline" "$BASELINE_ABC" "$ARTIFACT_DIR/baseline_verify_command.txt" "$ARTIFACT_DIR/baseline_verify.stdout.log" "$ARTIFACT_DIR/baseline_verify.stderr.log"
run_abc "$BASELINE_ABC" "$BASELINE_ENTRYPOINT" "$ARTIFACT_DIR/baseline_runtime_command.txt" "$ARTIFACT_DIR/baseline_runtime.stdout.log" "$ARTIFACT_DIR/baseline_runtime.stderr.log"
baseline_status=$?
if [ $baseline_status -ne 0 ] || ! python3 - "$ARTIFACT_DIR/baseline_runtime.stdout.log" "$ARTIFACT_DIR/baseline_runtime.stderr.log" <<'PY'
import pathlib
import sys
stdout = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace").strip()
stderr = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8", errors="replace")
sys.exit(0 if stdout == "baseline ok" and stderr == "" else 1)
PY
then
  blocker "same-toolchain console baseline failed; runtime stdout/stderr cannot be attributed to product code"
fi
info "same-toolchain console baseline passed"

EMPTY_DIR="$ARTIFACT_DIR/empty_arg_pkg"
mkdir -p "$EMPTY_DIR" || fail "failed to create empty-arg validation package"
copy_product_sources "$EMPTY_DIR"
python3 - "$EMPTY_DIR/val_empty_arg.ets" <<'PY'
import pathlib
import sys
pathlib.Path(sys.argv[1]).write_text(
    "import { build } from './entry';\n"
    "function main(): void {\n"
    "  let ok: boolean = build('');\n"
    "  if (ok) {\n"
    "    throw new Error('build unexpectedly accepted empty build_config path');\n"
    "  }\n"
    "}\n",
    encoding="utf-8",
)
PY
EMPTY_CONFIG="$ARTIFACT_DIR/empty_arg_arktsconfig.json"
EMPTY_ABC="$ARTIFACT_DIR/empty_arg.abc"
write_config "$EMPTY_DIR" "$EMPTY_CONFIG" "@val_entry_empty"
compile_package "empty_arg" "$EMPTY_CONFIG" "$EMPTY_ABC" "$ARTIFACT_DIR/empty_arg_compile.stdout.log" "$ARTIFACT_DIR/empty_arg_compile.stderr.log" "$ARTIFACT_DIR/empty_arg_compile_command.txt"
EMPTY_ENTRYPOINT=$(derive_entrypoint "$EMPTY_ABC" "$ARTIFACT_DIR/empty_arg.pa" ".val_empty_arg.")
[ -n "$EMPTY_ENTRYPOINT" ] || fail "could not derive empty-arg validation entrypoint"
verify_abc "empty_arg" "$EMPTY_ABC" "$ARTIFACT_DIR/empty_arg_verify_command.txt" "$ARTIFACT_DIR/empty_arg_verify.stdout.log" "$ARTIFACT_DIR/empty_arg_verify.stderr.log"
run_abc "$EMPTY_ABC" "$EMPTY_ENTRYPOINT" "$ARTIFACT_DIR/empty_arg_runtime_command.txt" "$ARTIFACT_DIR/empty_arg_runtime.stdout.log" "$ARTIFACT_DIR/empty_arg_runtime.stderr.log"
empty_status=$?
[ $empty_status -eq 0 ] || fail "entry empty-argument validation failed with exit $empty_status; see $ARTIFACT_DIR/empty_arg_runtime.stderr.log"
info "empty build-config path is rejected through the entry route"

SMOKE_DIR="$ARTIFACT_DIR/demo_smoke_pkg"
mkdir -p "$SMOKE_DIR" || fail "failed to create demo smoke package"
copy_product_sources "$SMOKE_DIR"
python3 - "$SMOKE_DIR/val_demo_smoke.ets" "$DEMO_CONFIG" <<'PY'
import pathlib
import sys
path = sys.argv[2]
pathlib.Path(sys.argv[1]).write_text(
    "import { build } from './entry';\n"
    "function main(): void {\n"
    f"  let ok: boolean = build('{path}');\n"
    "  if (!ok) {\n"
    "    throw new Error('build returned false for demo_hap build_config path');\n"
    "  }\n"
    "}\n",
    encoding="utf-8",
)
PY
SMOKE_CONFIG="$ARTIFACT_DIR/demo_smoke_arktsconfig.json"
SMOKE_ABC="$ARTIFACT_DIR/demo_smoke.abc"
write_config "$SMOKE_DIR" "$SMOKE_CONFIG" "@val_entry_demo"
compile_package "demo_smoke" "$SMOKE_CONFIG" "$SMOKE_ABC" "$ARTIFACT_DIR/demo_smoke_compile.stdout.log" "$ARTIFACT_DIR/demo_smoke_compile.stderr.log" "$ARTIFACT_DIR/demo_smoke_compile_command.txt"
SMOKE_ENTRYPOINT=$(derive_entrypoint "$SMOKE_ABC" "$ARTIFACT_DIR/demo_smoke.pa" ".val_demo_smoke.")
[ -n "$SMOKE_ENTRYPOINT" ] || fail "could not derive demo smoke entrypoint"
verify_abc "demo_smoke" "$SMOKE_ABC" "$ARTIFACT_DIR/demo_smoke_verify_command.txt" "$ARTIFACT_DIR/demo_smoke_verify.stdout.log" "$ARTIFACT_DIR/demo_smoke_verify.stderr.log"
remove_demo_abcs
run_abc "$SMOKE_ABC" "$SMOKE_ENTRYPOINT" "$ARTIFACT_DIR/demo_smoke_runtime_command.txt" "$ARTIFACT_DIR/demo_smoke_runtime.stdout.log" "$ARTIFACT_DIR/demo_smoke_runtime.stderr.log" "$DEMO_CONFIG"
smoke_status=$?
[ $smoke_status -eq 0 ] || fail "demo_hap entry dispatch smoke failed with exit $smoke_status; see $ARTIFACT_DIR/demo_smoke_runtime.stderr.log"
assert_demo_artifacts || fail "demo_hap ABC artifact assertion failed; see $ARTIFACT_DIR/demo_artifact_assertion.txt"

printf 'PASS %s\n' "$TASK_ID"
