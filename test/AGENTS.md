# Repository Test Orchestration Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Project Overview

`test/` contains SDK, XTS, performance, and workload orchestration, configuration, example projects, and exclusion lists. It complements route-specific compiler unit/regression suites; it is not a single universal frontend test runner.

```text
Python + YAML + host/device scripts
    → prepared OpenHarmony SDK/runtime/device
    → scenario execution and artifact comparison
    → result reporting
```

### 1.1 Coverage

| Kind | Purpose |
|---|---|
| SDK | HAP/HAR builds, bytecode/obfuscation integration, incremental behavior |
| XTS | Compatibility suites on the intended device/platform |
| Performance | JSPerf/VM and build measurements |
| Workload | Representative application/scenario validation |
| Compiler unit tests | Separate sources under `es2panda/unittest/` and `ets2panda/test/` |

## 2. Code Map

Paths below are repository-relative.

| Path | Responsibility |
|---|---|
| `test/scripts/entry.py` | Preparation, scheduling, orchestration |
| `test/scripts/auto_xts_test/` | XTS batch runner/configuration/results |
| `test/scripts/sdk_test/` | SDK preparation, execution, and result comparison |
| `test/scripts/performance_test/` | Performance configuration/build/run/report |
| `test/scripts/utils/download_sdk_and_image/` | SDK/image preparation |
| `test/scripts/utils/flash_image/` | Device flashing support |
| `test/scripts/utils/commit_message/` | Revision metadata collection |
| `test/scripts/utils/send_email/` | External result delivery |
| `test/sdk_test_projects/` | SDK project fixtures |
| `test/workload/` | Workload exclusion lists |
| `test/ignorelist/` | Unit-test exclusions |

### 2.1 Fixture projects

Inspect actual project names and configuration rather than reproducing a misspelled directory. Existing fixtures include bytecode HAR, incremental compile, and out-of-project dependency scenarios. Changes to project selection must update the owning SDK config and preserve baseline/new SDK comparisons.

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Dynamic compiler validation | `es2panda/AGENTS.md`, `docs/knowledge/ES2Panda_Knowledge_Base.md` |
| Static compiler validation | `ets2panda/test/AGENTS.md`, `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |
| ECMAScript conformance | `test262/AGENTS.md` |
| SDK setup/run | `test/scripts/sdk_test/readme.md` |
| XTS setup/run | `test/scripts/auto_xts_test/readme.md` |
| Performance setup/run | `test/scripts/performance_test/readme.md` |
| Preparation/reporting helpers | `test/scripts/utils/readme.md` |

## 4. Constraints and Side Effects

### 4.1 Do not

- Do not run the top-level scheduler as passive validation: it can download SDK/images, flash a device, run external tools, and send email.
- Do not send reports/messages without user authorization for that delivery.
- Do not infer every scenario is supported on the current host from the presence of a batch/shell script.
- Do not update ignored lists or expected results to conceal a regression.
- Do not confuse build success, artifact comparison, runtime execution, and performance measurement.
- Do not hardcode developer SDK paths into tracked configuration.

### 4.2 Ask before

Apply repository approval boundaries to destructive/environment-affecting operations, device changes, and compatibility-sensitive build changes. Confirm intended device/SDK configuration before flashing. Routine source inspection and document checks do not require running the orchestration stack.

## 5. Usage and Verification

### 5.1 Full orchestration

The following commands run from the repository root and then enter the script directory:

```bash
cd test/scripts
python entry.py
```

Inspect `entry.py` first. It invokes preparation and reporting helpers, starts a scheduled loop, and may continue running after the initial execution. Use the scenario-specific runner when only one scenario is authorized.

| Option | Effect |
|---|---|
| `--skipDownloadSdk` | Skip the SDK download branch |
| `--skipDownloadDayu` | Skip the Dayu image download/flashing branch |

Skipping downloads does not disable the other orchestration/reporting behavior. Use the interpreter required by these scripts and installed dependencies; `entry.py` invokes child commands using `python`.

### 5.2 Scenario-specific commands

Each block starts from the ets_frontend repository root; do not execute the blocks as cumulative directory changes.

```bash
# SDK scenario
cd test/scripts/sdk_test
python entry.py
# Alternative shell entry, after inspecting its behavior:
# ./run_ohos_sdk_test.sh
```

```text
XTS on its supported host:
working directory: test/scripts/auto_xts_test/
entry: run.bat
```

```bash
# Performance scenario
cd test/scripts/performance_test
python performance_entry.py
```

### 5.3 SDK tests

Prepare baseline and candidate SDKs, the selected projects, build tools, and required dependencies. Cover bytecode compilation/merge, enabled obfuscation, HAR output, and incremental builds according to the changed feature.

Compare exit status, diagnostics, packaged artifacts, and actual behavior. A difference caused by missing SDK/tooling is an environment failure to diagnose, not an expected-output update.

### 5.4 XTS tests

Select modules in the current configuration and `running_modules.txt`. Verify SDK/device/image alignment and the supported runner host. Preserve device logs and module results; do not infer complete compatibility from one passing module.

### 5.5 Performance tests

Inspect `performance_config.py`, build/run code, and report schemas. Keep machine, toolchain, mode (interpreter/JIT/AOT), corpus, warm-up, and measurement conditions comparable. Compare baseline/candidate distributions rather than treating one timing as a regression conclusion.

### 5.6 Workloads

Inspect scenario-specific lists under `test/workload/`. Scenarios can distinguish JIT, QEMU/AOT, VM benchmarks, and application workloads. Choose the matching mode/architecture and retain the effective exclusion set with results.

### 5.7 Configuration

Use the actual schema in `test/scripts/sdk_test/config.yaml`, XTS config, and performance configuration. Fill local SDK/output/device settings in the intended local configuration mechanism. Do not introduce invented keys such as a hypothetical `test_projects` list without checking its consumer.

### 5.8 Scheduling and execution flow

`entry.py` registers a daily job with `schedule`, runs once immediately, then services pending jobs. Inspect the actual schedule and subprocess dispatch before changing it; the schedule is operational configuration, not a fixed requirement for every test.

```text
Authorized preparation/download
    → optional device preparation
    → XTS / SDK / performance execution
    → revision metadata and result collection
    → explicitly authorized delivery
```

Keep execution failures visible and stop/report at the actual failing stage. Do not count a downstream report-generation attempt as a completed test.

### 5.9 Add a fixture project

1. Add the project under `test/sdk_test_projects/` with reproducible inputs.
2. Register it using the actual owning config schema.
3. Run the focused scenario with prepared SDKs.
4. Compare intended artifacts/diagnostics and nearby related cases.
5. Record dependencies and any device/runtime limitations.

### 5.10 Exclusion lists

| List family | Purpose |
|---|---|
| `test/workload/ignored-*.txt` | Scenario/mode exclusions |
| `test/ignorelist/ignored-ut-*.txt` | Unit-test mode/architecture exclusions |

Use existing list syntax and the selector's actual matching rules. Add a reason and evidence for each new exclusion; do not change lists merely to lower the failure count.

### 5.11 Result locations

Check each scenario's configuration and result writer for actual output paths. SDK results, performance JSON/reports, XTS results, downloaded artifacts, and compiler outputs are separate artifacts. Do not assume all reports are under a fixed `results/` directory.

### 5.12 Independent repository-level suites

- `testTs/run_testTs.py`: legacy Node frontend TS tests. Requires the external TS cases and matching legacy `index.js`; it is separate from dynamic `runner.py --tsc` and from the native-binary Node wrapper.
- `test_ecma_bcopt/`: JS optimizer input and historical disassembly reference. There is no standalone runner; identify a compatible optimizer/disassembler and runtime before comparing output/behavior.
- Detailed prerequisites and comparisons: `docs/knowledge/ES2Panda_Knowledge_Base.md`.

### 5.13 Done definition

Record changed fixtures/configuration, effective commands and environment, baseline/candidate results, diagnostics/artifact comparisons, skips, and unexecuted checks. Keep device, SDK, runtime, network, and delivery limitations explicit. A scheduler launch or fixture's presence is not a passing regression.
