# Test262 Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Project Overview

`test262` integrates ECMAScript conformance tests with the dynamic frontend, ARK VM, host adapter, and test harness. Edition, Intl, Sendable, and other selections are defined by the current runner and configuration.

```text
Python orchestration + Node harness + selected Test262 data
    → frontend compilation → ARK VM execution → classified results
```

### 1.1 Coverage selections

| Selection | Purpose |
|---|---|
| ES5.1 / ES2015 | Earlier ECMAScript suites |
| ES2021 / ES2022 / ES2023 | Edition-filtered or cumulative selections |
| Intl | Internationalization tests |
| Sendable | Relevant concurrency extension tests |
| Other | Additional nonstandard selections |
| ABC-to-program | Mixed compilation/recompilation route |

Inspect `run_test262.py` and `config.py` rather than treating this list as a fixed latest-version guarantee.

## 2. Code Map

Paths in this table are relative to `test262/`; generated/downloaded paths may be absent until preparation.

| Path | Responsibility |
|---|---|
| `run_test262.py` | Arguments, preparation, test orchestration |
| `config.py` | Suite/tool/output paths and host defaults |
| `utils.py` | Process and test helpers |
| `mix_compile.py` | Mixed compilation support |
| `run_sunspider.py` | Separate performance helper |
| Edition/list files | Selected test inventory |
| `skip_tests.json`, `intl_skip_tests.json` | Known exclusions |
| `ignored-test262-*.txt`, `skip-test262-*.txt` | Scenario/platform exclusions |
| `data/` | Prepared external Test262 data |
| `harness/`, `eshost/` | Prepared/patched Node harness and VM adapter |

Output paths are controlled by `config.py` and selected mode; the base output directory is repository-relative `out/test262/`, not an unconditional `test262/output/` directory.

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Dynamic compiler regression | Repository-relative `es2panda/AGENTS.md` |
| Route constraints and required suites | `docs/knowledge/ES2Panda_Knowledge_Base.md` |
| Runner flag/path issue | `test262/run_test262.py`, `test262/config.py` |
| Harness/VM mismatch | Runner preparation, patches, host adapter, matching runtime |
| SDK/XTS/workload | `test/AGENTS.md` |

## 4. Constraints

### 4.1 Do not

- Do not update skip/ignored lists merely because many tests fail; trace environment failures and regressions first.
- Do not report selected/skipped tests as executed passes.
- Do not substitute static `ets2panda` semantics for this dynamic conformance route.
- Do not assume generated suite/harness/output directories are tracked source files.
- Do not confuse a disassembler/tool build with runtime conformance execution.
- Do not copy the obsolete `--ark-frontend-tool` spelling; the current binary option is `--ark-frontend-binary`.

### 4.2 Ask before

Follow repository approval gates for dependencies, serialized/bytecode compatibility, and environment-affecting actions. Inspect preparation/download/patch behavior before running a full suite in an unprepared checkout.

## 5. Usage and Verification

### 5.1 Working directory and prerequisites

Run Python commands from the ets_frontend repository root. `config.py` constructs paths such as `test262/data` relative to that root; changing into `test262/` first can duplicate the path prefix.

Prepare the selected compiler, matching runtime and shared libraries, suite data, Node harness, and any required ICU resources. Use absolute binary paths to avoid resolving an OpenHarmony output path relative to this repository.

### 5.2 Basic usage

The following are parameterized recipes. Set actual absolute binary paths before execution:

```bash
ES2ABC="<absolute_es2abc_binary>"
ARK_VM="<absolute_ark_js_vm_binary>"
python3 test262/run_test262.py --es2022 all --ark-frontend=es2panda --ark-frontend-binary="$ES2ABC" --ark-tool="$ARK_VM" --product-name=rk3568
python3 test262/run_test262.py --es2022 all --ark-frontend=es2panda --ark-frontend-binary="$ES2ABC" --ark-tool="$ARK_VM" --product-name=rk3568 --abc2program
```

For dynamic code changes, run both ordinary and mixed modes as required by `es2panda/AGENTS.md`; focused cases supplement the full required regression.

### 5.3 Select a smaller suite

```bash
python3 test262/run_test262.py --es51
python3 test262/run_test262.py --es2023 only
python3 test262/run_test262.py --file <prepared_test_file>
python3 test262/run_test262.py --dir <prepared_test_directory>
```

The last two commands are parameterized selectors; use real prepared test paths and the same explicit tool/runtime options as the full-suite recipe when defaults do not match your checkout.

### 5.4 Common options

| Option | Meaning |
|---|---|
| `--es51`, `--es2015` | Earlier edition selectors |
| `--es2021`, `--es2022`, `--es2023` | Edition selection; inspect `all`/`only` choices |
| `--intl`, `--sendable`, `--other` | Additional selection groups |
| `--file`, `--dir`, `--test-list` | Focused selection |
| `--mode 1` | Default (non-strict) mode |
| `--mode 2` | Strict mode |
| `--mode 3` | Both modes |
| `--skip-list` | Additional skip-list input |
| `--timeout`, `--threads` | Timeout and parallelism |
| `--ark-frontend`, `--ark-frontend-binary` | Frontend route and executable |
| `--ark-tool`, `--libs-dir` | Runtime and library paths |
| `--abc2program` | Mixed route |
| `--babel` | Runner's optional transformation path |

Use `python3 test262/run_test262.py --help` for exact current types/defaults. Set parallelism to the prepared environment rather than freezing a specific worker count.

### 5.5 Execution flow

1. Parse selection and tool configuration.
2. Prepare dependencies, suite checkout, and relevant patches as implemented by the runner.
3. Compile test inputs in the selected route.
4. Execute the matching VM/harness mode.
5. Classify compile failures, runtime failures, timeouts, skips, and passes.
6. Write logs/results in the configured output directory.

Check subprocess failures and preparation completion before interpreting suite totals. A missing tool can cause broad failure without indicating a language regression.

### 5.6 Skip lists

Read the actual JSON/list schema and scenario matching code before adding an entry. The filename pattern describes a scenario; it does not prove a particular mode, architecture, or test was exercised.

Every new exclusion needs a reason and reproducible evidence. Keep strict/default and ordinary/mixed results distinguishable. Do not automatically accept changed baselines after a compiler change.

### 5.7 Output and diagnostics

Inspect configured output roots for compiled ABCs, harness/process logs, and result summaries. Exact suffixes and report filenames depend on the runner path. Preserve the selected command/configuration with the report.

A report should distinguish:

| Outcome | Evidence |
|---|---|
| Pass | Actual harness/VM execution with intended mode |
| Compile failure | Frontend command and diagnostic |
| Runtime failure | VM/harness output and exit status |
| Timeout | Tool, timeout setting, and case |
| Skip | Applied list/reason; not counted as executed pass |
| Preparation failure | Missing/downloaded tool or fixture prerequisite |

### 5.8 Advanced use

For custom tools, supply `--ark-frontend-binary` and `--ark-tool` explicitly and verify dependencies. Use `--babel` only for the intended runner path, since transformation can change what is being measured. Compare single-worker and parallel failures when diagnosing nondeterminism.

SunSpider/performance helpers are separate measurements; conformance passes do not establish performance, and timing results do not replace conformance.

### 5.9 Troubleshooting

| Symptom | First action |
|---|---|
| npm/data preparation fails | Diagnose actual registry/network/tool error and preserve logs |
| Frontend cannot launch | Check binary option, file architecture, permissions, libraries |
| Widespread runtime failures | Check compiler/runtime compatibility and harness patch state |
| Timeout | Reproduce focused case before changing timeout |
| Unexpected skipped cases | Inspect selection and scenario list resolution |
| Mixed-only failure | Trace ABC-to-program inputs and matching runtime versions |

Re-run the minimal failing case, inspect generated input/ABC and diagnostics, then fix the owning layer and rerun relevant suites. Never present sample result counts as measured output.
