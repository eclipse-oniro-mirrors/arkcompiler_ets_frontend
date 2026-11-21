# ES2Panda Testing Procedure

For dynamic compiler code changes, build successfully before running unit tests, integration suites, both Test262 modes, runtime_core UT, verifier checks, disassembler build, fuzz build, and applicable version checks. Focused tests help diagnose a defect and do not establish completion of the full required regression.

Use Release unless Debug is needed to investigate a failure. Record host/product/build configuration; do not claim an untested host branch passed.

## 1. Prepare paths and build

Read `docs/knowledge/Build_Environment.md` first. Set and validate `OH_ROOT` and `FRONTEND_BUILD_DIR` in the current Bash session. The sample below is for Linux x64/rk3568; set the runtime/library paths for the actual matching build and adapt the product consistently.

All document/source paths are repository-relative unless an absolute variable is used.

```bash
: "${OH_ROOT:?Set OH_ROOT as documented in Build_Environment.md}"
: "${FRONTEND_BUILD_DIR:?Set FRONTEND_BUILD_DIR for this host/product}"
OUT_DIR="$OH_ROOT/out/rk3568/clang_x64"
RUNTIME_DIR="$OUT_DIR/arkcompiler/ets_runtime"
TEST_LD_LIBRARY_PATH="$RUNTIME_DIR:$OUT_DIR/thirdparty/icu:$OUT_DIR/thirdparty/zlib:$OH_ROOT/prebuilts/clang/ohos/linux-x86_64/llvm/lib"

cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build --no-prebuilt-sdk
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:ark_packages --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages --no-prebuilt-sdk
"$FRONTEND_BUILD_DIR/es2abc" --help
```

For a Debug frontend/runtime build, replace the corresponding Release build with:

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build --gn-args is_debug=true --no-prebuilt-sdk
```

## 2. C++ unit tests

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target arkcompiler/ets_frontend/es2panda:es2abc_tests --no-prebuilt-sdk
```

Sources are under `es2panda/unittest/`, grouped by component. Inspect `es2panda/unittest/BUILD.gn` for actual executable names and test registration; do not assume generic `lexer_test`/`parser_test` names exist.

When running a built test binary directly, use the real binary path and an actual gtest filter from its registered tests:

Set `UNIT_TEST_BINARY` to an absolute executable selected from the registered targets and `GTEST_FILTER` to a real suite/case from its listing. Then run:

```bash
: "${UNIT_TEST_BINARY:?Set a registered executable path}"
test -x "$UNIT_TEST_BINARY"
"$UNIT_TEST_BINARY" --gtest_list_tests
: "${GTEST_FILTER:?Set a filter from the test listing}"
"$UNIT_TEST_BINARY" --gtest_filter="$GTEST_FILTER"
```

Verify the changed case and nearby cases. A target build is not automatically evidence that every binary was executed.

## 3. Python integration tests

```bash
cd "$OH_ROOT/arkcompiler/ets_frontend/es2panda"

# Install only if the test dependency is not already present.
python3 -m pip install tqdm

# Parser regression
python3 test/runner.py --regression "$FRONTEND_BUILD_DIR"

# Compiler execution, using the matching JS runtime and library path
python3 test/runner.py --no-progress --compiler --js-runtime "$RUNTIME_DIR" --LD_LIBRARY_PATH "$TEST_LD_LIBRARY_PATH" "$FRONTEND_BUILD_DIR"

# External TypeScript conformance input
python3 test/runner.py --tsc "$FRONTEND_BUILD_DIR" --tsc-path "$OH_ROOT/third_party/typescript"

# Hot/cold patch and reload suites
python3 test/runner.py --hotfix --hotreload --coldfix --coldreload "$FRONTEND_BUILD_DIR"

# Encoding, bytecode, debugger, and version coverage
python3 test/runner.py --base64 "$FRONTEND_BUILD_DIR"
python3 test/runner.py --bytecode "$FRONTEND_BUILD_DIR"
python3 test/runner.py --debugger "$FRONTEND_BUILD_DIR"
python3 test/runner.py --version-control --no-progress "$FRONTEND_BUILD_DIR"
```

For parser diagnostic detail:

```bash
python3 test/runner.py --regression "$FRONTEND_BUILD_DIR" --error
```

`--regression` is the parser regression selector. `--parser`, `--optimizer`, and `--patch` are not supported runner options in this checkout. Use the explicit patch/reload suite switches above.

## 4. Test262: both modes are required

Run from the repository root with the absolute frontend path:

```bash
cd "$OH_ROOT/arkcompiler/ets_frontend"

# Full source suite
python3 test262/run_test262.py --es2022 all --ark-frontend-binary="$FRONTEND_BUILD_DIR/es2abc" --ark-frontend=es2panda --product-name=rk3568 --timeout=3000000

# Mixed / ABC-to-program route
python3 test262/run_test262.py --es2022 all --ark-frontend-binary="$FRONTEND_BUILD_DIR/es2abc" --ark-frontend=es2panda --product-name=rk3568 --timeout=3000000 --abc2program
```

Prepare the suite and runtime prerequisites before execution. Distinguish selected, skipped, passed, failed, and timed-out cases in the result record.

## 5. runtime_core unit tests

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target runtime_core_host_unittest --no-prebuilt-sdk
```

## 6. Verifier tests

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:verifier_host_unittest --no-prebuilt-sdk
```

## 7. Disassembler build check

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages --no-prebuilt-sdk
```

This builds host tools including the disassembler. A successful build alone does not establish disassembly behavior; record any actual disassembly comparison separately.

## 8. Fuzz build

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core/tests/fuzztest:fuzztest --no-prebuilt-sdk
```

Report build coverage separately from fuzz execution and corpus coverage.

## 9. Instruction / ABC version self-check

```bash
cd "$OH_ROOT"
python3 arkcompiler/runtime_core/isa/check_version.py
```

- `No change in version!`: attach the required evidence to the corresponding checklist item.
- `[IMPORTANT] Version change detected ...`: contact the relevant domain owner and call out instruction/ABC compatibility impact in the checklist and final report.
- An error is a failed/unavailable check, never a pass.

The script checks version changes; it does not replace bytecode compatibility regressions.

## 10. Legacy TestTs coverage

The actual repository directory is `testTs/`. Its runner is separate from `es2panda/test/runner.py --tsc` and expects a legacy Node frontend `index.js`, not native `es2abc` or the `scripts/ts2abc.js` native wrapper.

Inspect `testTs/config.py`, `skip_tests.json`, `import_tests.json`, and `expect/`. Prepare the configured external TS fixtures and legacy frontend. Set `LEGACY_FRONTEND_JS` to the actual legacy index.js, `LEGACY_TS_CASE` to a prepared case, and `LEGACY_TS_DIRECTORY` to the selected fixture directory before running from the repository root:

```bash
python3 testTs/run_testTs.py --ark_frontend_tool "$LEGACY_FRONTEND_JS" --file "$LEGACY_TS_CASE"
python3 testTs/run_testTs.py --ark_frontend_tool "$LEGACY_FRONTEND_JS" --dir "$LEGACY_TS_DIRECTORY"
```

The runner also supports `--testinstype`. Inspect that mode before selecting it. Compare generated output under `out/testTs/` with the relevant expectations, and account for skip lists and import prerequisites. Report an unavailable legacy frontend as a coverage gap; do not substitute a different frontend contract.

## 11. ECMAScript bytecode optimizer fixture

`test_ecma_bcopt/` contains `test_bytecode_optimizer_for_js.js` and a historical assembly reference `test_bytecode_optimizer_for_js.txt`. The directory has no standalone runner or GN target.

Before claiming coverage, locate a compatible optimizer/disassembler consumer and matching bytecode version in the runtime toolchain. Compile/optimize/disassemble the fixture through that supported consumer, compare intended optimization behavior and assembly, and check execution behavior. If no matching consumer is available, record the gap. Merely retaining the fixture is not a passing test.

## 12. Test result record

Record build mode, host/product/toolchain, executed commands, pass/fail results, skipped checks, logs, and version-check evidence. Explain failures and missing prerequisites. Partial success must not be presented as complete regression success.
