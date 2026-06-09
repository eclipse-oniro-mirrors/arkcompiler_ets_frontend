#!/usr/bin/env bash
set -u -o pipefail

SCENARIO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCENARIO_DIR/../../.." && pwd)
PANDA_ROOT=$(cd "$REPO_ROOT/../../.." && pwd)
TASK_ID="006-arkts_driver_config_model-task-3-add-arktsconfiggenerator-model"
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
DEMO_CONFIG="$REPO_ROOT/driver/build_system/test/demo_hap/build_config.json"
BUILD_SYSTEM_ROOT="$REPO_ROOT/driver/build_system"
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
require_file "$PROD_SRC/build/generate_arktsconfig.ets" "production ArkTSConfigGenerator source not found"
require_file "$PROD_SRC/types.ets" "production types source not found"
require_file "$PROD_SRC/pre_define.ets" "production constants source not found"
require_file "$PROD_SRC/logger.ets" "production logger source not found"
require_file "$PROD_SRC/util/error.ets" "production error source not found"
require_file "$PROD_SRC/util/utils.ets" "production utils source not found"
require_file "$PROD_ARKTSCONFIG" "production arktsconfig not found"
require_file "$DEMO_CONFIG" "demo_hap build_config.json not found"
require_executable_or_blocker "$ES2PANDA" "es2panda not executable"
require_executable_or_blocker "$ARK" "ark runtime not executable"
require_executable_or_blocker "$ARK_DISASM" "ark_disasm not executable"
require_executable_or_blocker "$ARK_VERIFIER" "verifier not executable"
require_file "$ETSSTDLIB" "ETS stdlib ABC not found"

python3 - "$DEMO_CONFIG" "$ARTIFACT_DIR/demo_config_assertion.txt" <<'PY'
import json
import pathlib
import sys
path = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
data = json.loads(path.read_text(encoding="utf-8"))
if not isinstance(data, dict):
    out.write_text("FAIL demo config is not an object\n", encoding="utf-8")
    sys.exit(1)
out.write_text("PASS demo config is valid JSON object\n", encoding="utf-8")
PY
[ $? -eq 0 ] || fail "demo_hap build_config.json is not readable JSON"

info "offline deterministic validation; network, external providers, and devices are disabled"
info "validating production ArkTSConfigGenerator through local compiler/runtime commands"

BASELINE_DIR="$ARTIFACT_DIR/console_baseline_pkg"
mkdir -p "$BASELINE_DIR" || fail "failed to create baseline package"
cat > "$BASELINE_DIR/baseline.ets" <<'EOF_BASELINE'
function main(): void {
  console.log('console-baseline-ok');
}
EOF_BASELINE
BASELINE_CONFIG="$ARTIFACT_DIR/console_baseline_arktsconfig.json"
BASELINE_ABC="$ARTIFACT_DIR/console_baseline.abc"
write_config "$BASELINE_DIR" "$BASELINE_CONFIG" "@val_arktsconfig_baseline"
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

VALIDATION_DIR="$ARTIFACT_DIR/arktsconfig_pkg"
mkdir -p "$VALIDATION_DIR/build" "$VALIDATION_DIR/util" || fail "failed to create ArkTSConfigGenerator validation package"
cp "$PROD_SRC/build/generate_arktsconfig.ets" "$VALIDATION_DIR/build/generate_arktsconfig.ets" || fail "failed to copy production generator source"
cp "$PROD_SRC/types.ets" "$VALIDATION_DIR/types.ets" || fail "failed to copy production types source"
cp "$PROD_SRC/pre_define.ets" "$VALIDATION_DIR/pre_define.ets" || fail "failed to copy production constants source"
cp "$PROD_SRC/logger.ets" "$VALIDATION_DIR/logger.ets" || fail "failed to copy production logger source"
cp "$PROD_SRC/util/error.ets" "$VALIDATION_DIR/util/error.ets" || fail "failed to copy production error source"
cp "$PROD_SRC/util/utils.ets" "$VALIDATION_DIR/util/utils.ets" || fail "failed to copy production utils source"
cat > "$VALIDATION_DIR/arktsconfig_golden_driver.ets" <<EOF_DRIVER
import { ArkTSConfig, ArkTSConfigGenerator } from './build/generate_arktsconfig';
import { AliasConfig, BuildConfig, ModuleInfo, OHOS_MODULE_TYPE } from './types';
import { LANGUAGE_VERSION } from './pre_define';

function assertCondition(condition: boolean, message: string): void {
  if (!condition) {
    throw new Error(message);
  }
}

function assertStringEquals(actual: string | undefined, expected: string, message: string): void {
  if (actual !== expected) {
    throw new Error(message + ': expected ' + expected + ', got ' + actual);
  }
}

function assertArrayEquals(actual: string[] | undefined, expected: string[], message: string): void {
  if (actual === undefined) {
    throw new Error(message + ': got undefined');
  }
  if (actual.length !== expected.length) {
    throw new Error(message + ': expected length ' + expected.length + ', got ' + actual.length + ' values ' + actual.join('|'));
  }
  for (let i: int = 0; i < expected.length; i++) {
    if (actual[i] !== expected[i]) {
      throw new Error(message + ': expected ' + expected.join('|') + ', got ' + actual.join('|'));
    }
  }
}

function createModuleInfo(packageName: string, moduleRootPath: string, staticFiles: string[], dependencies: string[]): ModuleInfo {
  return {
    isMainModule: packageName === 'entry',
    packageName: packageName,
    moduleRootPath: moduleRootPath,
    moduleType: packageName === 'entry' ? OHOS_MODULE_TYPE.ENTRY : OHOS_MODULE_TYPE.HAR,
    sourceRoots: packageName === 'entry' ? ['./', 'src/main1/ets'] : ['./'],
    entryFile: packageName === 'entry' ? 'a.ets' : 'index.ets',
    arktsConfigFile: moduleRootPath + '/arktsconfig.json',
    dependencies: dependencies,
    staticDependencyModules: new Map<string, ModuleInfo>(),
    dynamicDependencyModules: new Map<string, ModuleInfo>(),
    language: LANGUAGE_VERSION.ARKTS_1_2,
    staticFiles: staticFiles,
    moduleName: packageName,
  };
}

function createBuildConfig(root: string): BuildConfig {
  return {
    packageName: 'entry',
    moduleType: OHOS_MODULE_TYPE.SHARED,
    moduleRootPath: root + '/test/demo_hap/entry',
    sourceRoots: ['./', 'src/main1/ets'],
    compileFiles: [
      root + '/test/demo_hap/entry/a.ets',
      root + '/test/demo_hap/entry/b.ets',
      root + '/test/demo_hap/entry/c.ets',
      root + '/test/demo_hap/entry/d.ets',
      root + '/test/demo_hap/harA/index.ets',
      root + '/test/demo_hap/harB/index.ets',
    ],
    buildSdkPath: root + '/test/mock_sdk',
    pandaSdkPath: '${PANDA_ROOT}/out/debug',
    pandaStdlibPath: '${PANDA_ROOT}/out/debug/plugins/ets/etsstdlib.abc',
    cachePath: root + '/test/demo_hap/entry/dist/cache',
    projectRootPath: root + '/test/demo_hap',
    externalApiPaths: [],
    interopSDKPaths: new Set<string>(),
    paths: new Map<string, string[]>(),
    aliasConfig: new Map<string, Map<string, AliasConfig>>(),
  };
}

function assertGoldenConfig(config: ArkTSConfig, packageName: string, baseUrl: string, files: string[], packagePaths: string[]): void {
  assertStringEquals(config.packageName, packageName, packageName + ' packageName');
  assertStringEquals(config.compilerOptions.baseUrl, baseUrl, packageName + ' baseUrl');
  assertStringEquals(config.compilerOptions.rootDir, '${BUILD_SYSTEM_ROOT}/test/demo_hap', packageName + ' rootDir');
  assertStringEquals(config.compilerOptions.cacheDir, '${BUILD_SYSTEM_ROOT}/test/demo_hap/entry/dist/cache', packageName + ' cacheDir');
  assertArrayEquals(config.configObject.files, files, packageName + ' files');
  assertArrayEquals(config.pathSection.get(packageName), packagePaths, packageName + ' source root paths');
  assertCondition(config.dependencies.has('std/core'), packageName + ' std/core dependency missing');
  assertCondition(config.toJSONText().indexOf('"package":"' + packageName + '"') >= 0, packageName + ' serialized package missing');
  assertCondition(config.toJSONText().indexOf('"files"') >= 0, packageName + ' serialized files missing');
  assertCondition(config.toJSONText().indexOf('"paths"') >= 0, packageName + ' serialized paths missing');
}

function main(): void {
  let root: string = '${BUILD_SYSTEM_ROOT}';
  let harB: ModuleInfo = createModuleInfo('harB', root + '/test/demo_hap/harB', [root + '/test/demo_hap/harB/index.ets'], []);
  let harA: ModuleInfo = createModuleInfo('harA', root + '/test/demo_hap/harA', [root + '/test/demo_hap/harA/index.ets'], ['harB']);
  let entry: ModuleInfo = createModuleInfo('entry', root + '/test/demo_hap/entry', [
    root + '/test/demo_hap/entry/a.ets',
    root + '/test/demo_hap/entry/b.ets',
    root + '/test/demo_hap/entry/c.ets',
    root + '/test/demo_hap/entry/d.ets',
  ], ['harA']);
  harA.staticDependencyModules.set('harB', harB);
  entry.staticDependencyModules.set('harA', harA);

  let generator: ArkTSConfigGenerator = new ArkTSConfigGenerator(createBuildConfig(root));
  let harBConfig: ArkTSConfig = generator.generateConfigForModule(harB);
  let harAConfig: ArkTSConfig = generator.generateConfigForModule(harA);
  let entryConfig: ArkTSConfig = generator.generateConfigForModule(entry);

  assertGoldenConfig(harBConfig, 'harB', root + '/test/demo_hap/harB', [root + '/test/demo_hap/harB/index.ets'], [root + '/test/demo_hap/harB']);
  assertGoldenConfig(harAConfig, 'harA', root + '/test/demo_hap/harA', [root + '/test/demo_hap/harA/index.ets'], [root + '/test/demo_hap/harA']);
  assertGoldenConfig(entryConfig, 'entry', root + '/test/demo_hap/entry', [
    root + '/test/demo_hap/entry/a.ets',
    root + '/test/demo_hap/entry/b.ets',
    root + '/test/demo_hap/entry/c.ets',
    root + '/test/demo_hap/entry/d.ets',
  ], [root + '/test/demo_hap/entry/src/main1/ets', root + '/test/demo_hap/entry']);

  assertArrayEquals(entryConfig.pathSection.get('entry/a'), [root + '/test/demo_hap/entry/a.ets'], 'entry/a path');
  assertArrayEquals(entryConfig.pathSection.get('entry/b'), [root + '/test/demo_hap/entry/b.ets'], 'entry/b path');
  assertArrayEquals(entryConfig.pathSection.get('entry/c'), [root + '/test/demo_hap/entry/c.ets'], 'entry/c path');
  assertArrayEquals(entryConfig.pathSection.get('entry/d'), [root + '/test/demo_hap/entry/d.ets'], 'entry/d path');
  assertArrayEquals(harAConfig.pathSection.get('harA/index'), [root + '/test/demo_hap/harA/index.ets'], 'harA/index path');
  assertArrayEquals(harBConfig.pathSection.get('harB/index'), [root + '/test/demo_hap/harB/index.ets'], 'harB/index path');
  assertCondition(generator.serializeConfig('entry').indexOf('"package":"entry"') >= 0, 'entry serialized config missing package');
  console.log('arktsconfig-generator-golden-ok');
}
EOF_DRIVER

VALIDATION_CONFIG="$ARTIFACT_DIR/arktsconfig_arktsconfig.json"
VALIDATION_ABC="$ARTIFACT_DIR/arktsconfig_generator_validation.abc"
write_config "$VALIDATION_DIR" "$VALIDATION_CONFIG" "@val_arktsconfig_generator"
compile_package "arktsconfig_generator_validation" "$VALIDATION_CONFIG" "$VALIDATION_ABC" "$ARTIFACT_DIR/arktsconfig_compile.stdout.log" "$ARTIFACT_DIR/arktsconfig_compile.stderr.log" "$ARTIFACT_DIR/arktsconfig_compile_command.txt" "$VALIDATION_DIR/arktsconfig_golden_driver.ets"
VALIDATION_ENTRYPOINT=$(derive_entrypoint "$VALIDATION_ABC" "$ARTIFACT_DIR/arktsconfig_validation.pa" ".arktsconfig_golden_driver.")
[ -n "$VALIDATION_ENTRYPOINT" ] || fail "could not derive ArkTSConfigGenerator validation entrypoint"
verify_abc "arktsconfig_generator_validation" "$VALIDATION_ABC" "$ARTIFACT_DIR/arktsconfig_verify_command.txt" "$ARTIFACT_DIR/arktsconfig_verify.stdout.log" "$ARTIFACT_DIR/arktsconfig_verify.stderr.log"
run_abc "$VALIDATION_ABC" "$VALIDATION_ENTRYPOINT" "$ARTIFACT_DIR/arktsconfig_runtime_command.txt" "$ARTIFACT_DIR/arktsconfig_runtime.stdout.log" "$ARTIFACT_DIR/arktsconfig_runtime.stderr.log"
validation_status=$?
[ $validation_status -eq 0 ] || fail "ArkTSConfigGenerator validation runtime failed with exit $validation_status; see $ARTIFACT_DIR/arktsconfig_runtime.stderr.log"
assert_stdout_contains "$ARTIFACT_DIR/arktsconfig_runtime.stdout.log" "$ARTIFACT_DIR/arktsconfig_runtime.stderr.log" "$ARTIFACT_DIR/arktsconfig_stdout_assertion.txt" "arktsconfig-generator-golden-ok"
[ $? -eq 0 ] || fail "ArkTSConfigGenerator golden assertion failed; see $ARTIFACT_DIR/arktsconfig_stdout_assertion.txt"

printf 'PASS %s\n' "$TASK_ID"
