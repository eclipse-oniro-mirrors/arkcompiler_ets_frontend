---
name: linter-knowledge-base
description: Reference knowledge base for ets2panda/linter migration tool. Use when modifying TypeScriptLinter/HomeCheck rules, debugging autofix conflicts, investigating lint behavior, or setting up linter build/test. Use when working in ets2panda/linter directory.
---

# Linter Knowledge Base

> Document version: v1.1
> Last updated: 2026-07-31
> Scope: `arkcompiler/ets_frontend/ets2panda/linter` (repo-relative path: ets2panda/linter)

## Summary

1. Linter is migration assistance (≠ compiler), helps ArkTS-dynamic → ArkTS-static
2. TypeScriptLinter (single-file) vs HomeCheck (cross-file) → check Decision Table first
3. Rule changes need 8-point/6-point synchronization → follow Verification Checklist
4. Dependency reproducibility is part of linter correctness → lock transitive npm behavior and verify all package layers

## When to Use

- Modifying TypeScriptLinter or HomeCheck rules
- Debugging autofix conflicts or migration behavior
- Investigating false positives/negatives in lint
- Setting up linter build/test environment
- Debugging npm install/build drift, `@types/node` conflicts, HomeCheck/ArkAnalyzer packaging, or lockfile behavior

**When NOT to use:**
- Compiler semantic changes → `ets2panda/Static_Frontend_Knowledge_Base.md`
- Parser/lexer issues → parser KB (if exists in repo structure)
- LSP interaction → `ets2panda/LSP_Knowledge_Base.md`
- Obfuscation tool → `ArkGuard_Knowledge_Base.md`

## Overview

`ets2panda/linter` is an ArkTS migration-oriented static analysis subsystem independent from the main compilation pipeline. Tool: easytrans/tslinter, npm package: `@panda/tslinter`, CLI entry: `dist/tslinter.js`.

Input: `.ets/.ts/.js` files, project config, SDK paths, rule config. Output: lint diagnostics, IDE JSON, migration results, autofix reports, statistics. Core goal: help ArkTS-dynamic/TypeScript migrate to ArkTS-static constraints by detecting syntax, type, SDK, ArkUI, interop risks and providing automatic fixes.

**Naming conventions**:
- `HomeCheck`: migration checker capability; `homecheck/`: directory and npm package
- `ArkAnalyzer`: underlying analysis capability; `arkanalyzer/`: directory and npm package
- `TypeScriptLinter`: main rule implementation (TypeScript AST + type checker)
- `InteropTypescriptLinter`: TS/ETS interoperability rules

**Typical issues**: rule false positives/negatives, autofix conflicts, unexpected migration, abnormal IDE JSON, SDK resolution errors, test expectation mismatches.
Dependency issues also occur: transitive npm updates, mismatched `@types/node`, unstable `package-lock.json`, and install scripts that temporarily rewrite package metadata.

## Directory Structure And Code Map

- Upstream (external): source input, SDK paths, TypeScript/ohos-typescript dependencies
- Upstream (internal): HomeCheck/ArkAnalyzer foundation, rule config
- Current module (internal): `src/`, `homecheck/`, `arkanalyzer/`, `scripts/`, `docs/`, `test/`
- Downstream (internal): lint results, migration/statistics output
- Downstream (external): IDE/DevEco consumers, npm artifacts, GN build artifacts

**Data flow**: CLI input → `src/cli` + `src/lib` → TypeScriptLinter (AST + type checker) or HomeCheck (Scene + CFG/VFG) → merge diagnostics → output JSON/reports/statistics → IDE/DevEco consumers.

## Directory Explanations

```
ets2panda/linter/
├── src/cli/                  # CLI entry, argument parsing
├── src/lib/                  # TypeScript linter core
│   ├── data/                 # JSON allow/deny lists, SDK data
│   ├── utils/consts/         # Rule constants, helper tables
│   ├── autofixes/            # Autofix generation, conflict resolution
│   ├── statistics/           # Statistics and reporting
│   ├── ts-compiler/          # TypeScript program construction
│   └── ts-diagnostics/       # TSC diagnostics extraction
├── src/sdk/linter_1_1/       # SDK/Hvigor incremental lint
├── homecheck/                # Migration checker (Scene + checkers)
├── arkanalyzer/              # Underlying static analysis library
├── scripts/                  # Build/install scripts
├── docs/                     # Rule documentation
└── test/                     # Test cases and expectations
```

**Key files**: `TypeScriptLinter.ts` (core rules), `HomeCheck.ts` (adapter), `QuasiEditor.ts` (autofix), `Problems.ts` (FaultID), `CookBookMsg.ts` (messages), `rule-config.json` (IDE filtering), `homecheck/ruleSet.json` (16 migration rules).

## Core Files And Responsibilities

| File | Responsibility |
|------|----------------|
| `TypeScriptLinter.ts` | Core rule implementation (AST + type checker) |
| `InteropTypescriptLinter.ts` | TS/ETS interoperability rules |
| `HomeCheck.ts` | Adapter, converts `FileIssues` to `ProblemInfo` |
| `QuasiEditor.ts` | Autofix sorting, conflict handling, source replacement |
| `Problems.ts` | `FaultID` enum - stable rule index |
| `FaultAttrs.ts` | Maps `FaultID` to recipe number, severity |
| `CookBookMsg.ts` | User-visible messages, rule name in final parentheses |
| `Autofixer.ts` | Tool for rules to generate replacements |
| `LinterRunner.ts` | Main executor: file filtering, rule execution, migration |
| `rule-config.json` | IDE/migration rule classification |
| `homecheck/ruleSet.json` | HomeCheck rule set (16 rules currently) |
| `package.json` / `package-lock.json` | Reproducible npm dependency graph for linter, HomeCheck, ArkAnalyzer |
| `scripts/install-ohos-typescript-and-homecheck.mjs` | Packs and installs ohos-typescript, ArkAnalyzer, HomeCheck into the linter package |

## Responsibility Boundaries

**Responsible for**: rule checking, static analysis for migration, migration suggestions, autofix candidates, reports, statistics, IDE JSON protocol.

**NOT responsible for**: real ArkTS-static type relations → `ets2panda/checker`; AST structural changes → `ets2panda/lowering` or `ets2panda/parser`; runtime semantics → companion runtime repos (outside this KB scope).

**Do not copy complete compiler type system into linter layer.**

### Why TypeScriptLinter ≠ Compiler

**Motivation**: Linter uses TypeScript's type checker (external dependency `ohos-typescript`), while compiler owns ArkTS-static type system. Copying compiler semantics creates:
- Version mismatch risk (`ohos-typescript` ≠ ArkTS-static version)
- Dual maintenance burden (update both compiler + linter)
- False authority (linter suggestions ≠ compiler facts, designed differences exist)

**Example**: Linter may warn "conservative" for migration safety, while compiler allows. Check Decision Table first before modifying compiler.

## Code Anchors (First Files to Inspect)

By symptom, start with these files in order:

| Symptom | First files (inspect order) |
|---------|----------------------------|
| **Rule false positive** | `TypeScriptLinter.ts` → `TsUtils.ts` → `CookBookMsg.ts` → check `FaultID` and `recipe` |
| **Autofix conflict** | `QuasiEditor.ts` → `RulePriority.ts` → check offset calculation and priority |
| **HomeCheck missing** | `HomeCheck.ts` → `CheckerIndex.ts` → `ruleSet.json` → verify registration and description |
| **IDE protocol abnormal** | `LinterCLI.ts` → `ProblemInfo.ts` → check `indictor` field and JSON structure |
| **SDK resolution error** | `ResolveSdks.ts` → `src/lib/data/*.json` → verify SDK path and declarations |
| **Test expectation mismatch** | `test/**/results/*.diff` → inspect actual output → fix logic → then update |
| **Dependency/install drift** | nearest `package.json` → nearest `package-lock.json` → `scripts/install-ohos-typescript-and-homecheck.mjs` |

## Top Don'ts (Critical)

**Do NOT**:
1. **Modify compiler for linter issues** → Check Decision Table first, linter is migration assistance
2. **Update test expectations before fixing logic** → Inspect `.diff` first → fix logic → then update
3. **Skip `rule-config.json`** → IDE won't filter rule, must synchronize ALL 8/6 points
4. **Rebuild `Scene` in HomeCheck** → Exponentially slower, use `Utils.ts` helpers
5. **Calculate autofix offsets from token text** → Corrupt replacements, must use original source offsets
6. **Hardcode SDK paths** → Change across versions, use `ResolveSdks.ts` mechanisms
7. **Change IDE protocol without verifying consumer** → Break IDE/DevEco integration
8. **Copy compiler type system** → Linter uses TypeScript's checker, not ArkTS-static
9. **Forget synchronization points** → Incomplete rule changes, verify checklist
10. **Treat diagnostic differences as bugs** → Designed differences exist (IDE mode disables strict diagnostics)
11. **Ignore lockfiles in linter package layers** → Transitive npm drift can break builds without source changes
12. **Assume semver patch updates are harmless** → Type declarations can change compatibility, especially through `@types/node`

## Core Data Flow Or Control Flow

### Normal CLI Scan

1. Parse arguments → collect input files, `tsconfig`
2. Construct TypeScript programs (strict + non-strict by default)
3. Filter input files → run TypeScriptLinter or InteropTypescriptLinter
4. Merge TSC diagnostics with linter diagnostics → set exit code

**Performance note**: Dual program improves accuracy but increases memory. IDE mode disables strict diagnostics (`disableStrictDiagnostics=true`).

### IDE Interactive Mode

1. `--ide-interactive` → `followSdkSettings=true`, `disableStrictDiagnostics=true`
2. HomeCheck runs if `--arkts-2 --homecheck` enabled
3. TypeScriptLinter → merge problems → `filterLintProblems()` filters interop directions
4. Output: `scan-report.json`, per-file JSON on stdout, progress on stderr

**Progress field**: `indictor` not `indicator` (consumer dependency, do not change).

### Migration / Autofix

1. `--migrate` → `migratorMode=true`, `enableAutofix=true`
2. Merge fixable problems → `QuasiEditor.sortAndRemoveIntersections()`
3. Sort patches by offset → check intersections → handle conflicts by priority
4. Apply patches → write backups → generate reports

**Critical constraints**:
- Autofix offsets MUST be based on **original source text**
- Conflict condition: `!(lhs.end < rhs.start || rhs.end < lhs.start)` (endpoint touching = conflict)
- Single-patch conflict: resolved by `RulePriority` (higher replaces lower)
- Multi-patch conflict: marked not fixable (cannot auto-resolve)

**Autofix data structure**:
```typescript
interface Autofix {
  start: number;  // MUST be original source offset
  end: number;    // MUST be original source offset
  text: string;   // replacement
}
```

### HomeCheck Migration Flow

1. `HomeCheck.ts` constructs config → `MigrationTool.buildCheckEntry()`
2. `checkEntryBuilder()` filters files → builds `Scene` → `inferTypes()` → scopes
3. `CheckerIndex.ts` divides rules (file-level / project-level) → runs checkers
4. `transferIssues2ProblemInfo()` converts to `ProblemInfo` → `QuasiEditor` writes files

**Common failures**: empty projectPath, paths outside project, incorrect SDK list, languageTags not covering files, missing built-in SDK, no files after filtering.

## Knowledge Routing

- Lint tools, rules, migration → start with this document
- CLI arguments, modes → `src/cli/CommandLineParser.ts`, `src/cli/LinterCLI.ts`
- AST/type rules → `src/lib/TypeScriptLinter.ts`, `src/lib/BaseTypeScriptLinter.ts`
- Rule names, recipes → `src/lib/Problems.ts`, `src/lib/FaultAttrs.ts`, `src/lib/CookBookMsg.ts`
- Rule classification → `rule-config.json`, `src/lib/utils/functions/ConfiguredRulesProcess.ts`
- Autofix, migration → `src/lib/autofixes/QuasiEditor.ts`, `src/lib/autofixes/Autofixer.ts`
- HomeCheck rules → `homecheck/src/checker/migration/*`, `homecheck/src/utils/common/CheckerIndex.ts`
- SDK paths, resolution → `src/lib/ts-compiler/ResolveSdks.ts`
- Dependency drift, lockfiles, npm packaging → `package.json`, `package-lock.json`, `scripts/install-ohos-typescript-and-homecheck.mjs`
- Compiler type system → `ets2panda/Static_Frontend_Knowledge_Base.md`
- LSP interaction → `ets2panda/LSP_Knowledge_Base.md`
- Build/test → see Build section below

## Output Protocol

**IDE mode outputs**:
- stdout: one JSON per line `{ "filePath": string, "problems": ProblemInfo[] }`
- stderr: progress JSON `{ "content", "messageType": 1, "indicator": number }`
- finish marker: `{"content":"report finish","messageType":1,"indictor":1}` (field is `indictor`)

**`ProblemInfo` fields**: `line`, `column`, `endLine`, `endColumn`, `start`, `end`, `type`, `severity`, `faultId`, `problem`, `suggest`, `rule`, `ruleTag`, `autofixable`, `autofix`, `autofixTitle`.

**Reports**: `scan-report.json` (per-file problems), `scan-problems-statistics.json` (counts, timing), `autofix-report.html` (fix details, field `colum`/`endColum` intentional).

## Rule System

**TypeScriptLinter path**:
```
FaultID (Problems.ts) → FaultAttrs[FaultID].cookBookRef → cookBookTag[recipe] (CookBookMsg.ts)
  → user-visible "rule" (final parentheses) → rule-config.json (IDE filtering)
```

**HomeCheck path** (no FaultID):
```
metaData.description → findRuleTagByDesc() extracts rule name from parentheses
  → searches cookBookTag[1..N] → if match fails, ruleTag = -1 (IDE/statistics lose recipe)
```

**Critical constraint**: User-visible text must put stable rule name in **final parentheses**. Regex: `/.*\(([^)]+)\)[^(]*$/`. Other parentheses before rule name → lookup failure.

### Modification Synchronization Points (Critical)

**TypeScriptLinter rule (8 points)**:
1. `Problems.ts`: Add/reuse `FaultID`
2. `FaultAttrs.ts`: Bind recipe number and severity
3. `CookBookMsg.ts`: User-visible text with rule name in **final parentheses**
4. `TypeScriptLinter.ts`: Implement detection
5. `Autofixer.ts` / `AutofixTitles.ts`: Autofix if needed
6. `rule-config.json`: Classification for IDE filtering
7. `docs/rules-cn` / `docs/rules-en`: Documentation
8. `test/`: Test cases (`*.json`, `*.arkts2.json`, `*.autofix.json`, `*.migrate.json`, `*.migrate.ets`)

**HomeCheck rule (6 points)**:
1. `homecheck/src/checker/migration/`: Implement `BaseChecker` methods
2. `homecheck/src/utils/common/CheckerIndex.ts`: Register under `fileRules` or `projectRules`
3. `homecheck/ruleSet.json`: Add `@migration/*` rule to `plugin:@migration/all`
4. Verify `metaData.description` matches `findRuleTagByDesc()` in `src/lib/HomeCheck.ts`
5. Reuse helpers in `homecheck/src/checker/migration/Utils.ts` for cross-file data
6. Add HomeCheck vitest cases or linter-side integration tests

## Build, Run, And Package

> ⚠️ **Drift-prone content**: Commands, scripts, and versions may change. Verify against current tree before use.

### Build Modes (drift-prone, verify current version)

There are two different linter build modes. Do not mix their assumptions.

**Pipeline build**:
- Uses `build_linter.py`
- Must use the pipeline-specified Node.js and npm versions
- Known pipeline versions: Node.js `v14.21.1`, npm `6.14.17`
- Before running `build_linter.py`, run `npm install` with the specified npm executable in each package layer: `arkanalyzer`, `homecheck`, then linter root
- Pass absolute paths from the current build environment; do not copy a developer machine path into scripts or documentation

Example command shape:

```bash
cd <repo>/ets2panda/linter/arkanalyzer
<node-v14.21.1>/bin/npm install

cd <repo>/ets2panda/linter/homecheck
<node-v14.21.1>/bin/npm install

cd <repo>/ets2panda/linter
<node-v14.21.1>/bin/npm install

python3 build_linter.py \
  --source-path <repo>/ets2panda/linter \
  --output-path <output-dir> \
  --npm <node-v14.21.1>/bin/npm \
  --typescript <repo>/ets2panda/linter/<ohos-typescript-package>.tgz \
  --version <linter-version>
```

**Local developer build**:
- Follow `ets2panda/linter/README.md`
- Does not require the pipeline Node.js/npm versions unless reproducing a pipeline-only issue
- Run `npm install`, then `npm run install-ohos-typescript`
- Run `npm run build` when build/package validation is needed

### Common Commands

| Command | Purpose | When to use |
|---------|---------|-------------|
| `npm install` | Install dependencies | Local setup, or pipeline preinstall when run with the specified npm |
| `npm run install-ohos-typescript` | Install ohos-typescript, arkanalyzer, homecheck | Local developer setup after dependency update |
| `python3 build_linter.py ...` | Pipeline-style package build | Reproducing pipeline package behavior |
| `npm run build` | Local build (clean → compile → webpack → pack) | Local validation before testing |
| `npm test` | Full test suite (runs `npm run fix` first) | Regression |
| `npm run testrunner -- -d test/rules -p 'rule-name*'` | Single rule test | Debugging |
| `npm run testrunner -- -d test/main --sdk` | SDK-dependent tests | SDK scenarios |
| `npm run coverage` | Coverage report | Measurement |
| `npm run update-tests` | Update test expectations | **Only after confirming correctness** |

### Test File Conventions

**Test files**: `case-name.ets` (input), `case-name.json` (default expectation), `case-name.arkts2.json` (--arkts-2), `case-name.autofix.json` (autofix), `case-name.migrate.json` (migration diagnostics), `case-name.migrate.ets` (migrated source). Results in `test/**/results/`, failures generate `.diff`.

**Test directories**: `test/main`, `test/rules`, `test/regression` (main rules), `test/interop` (interop rules), `test/sdkwhite`, `test/sdkcommonapi` (SDK rules), `test/builtin`, `test/concurrent` (dedicated domains).

### Common Run Commands

**Normal scan**: `node dist/tslinter.js --arkts-2 path/to/file.ets`

**IDE + HomeCheck + migration**: `node dist/tslinter.js --ide-interactive --arkts-2 --autofix --homecheck --migrate --sdk-default-api-path /path/to/sdk ...`

**Critical parameters**: `--project <tsconfig>` (construct program), `--project-folder <dir>` (collect files), `--check-ts-and-js` (allow .ts/.js), `--rule-config <path>` (rule classification), `--sdk-default-api-path` (must contain `build-tools/ets-loader/declarations`).

## Dependency And Packaging Stability

`ets2panda/linter` is a multi-package npm workspace in practice, even though it is arranged as repo subdirectories instead of a formal npm workspace. The root linter package depends on local `homecheck`, and `homecheck` depends on local `arkanalyzer`. A build or packaging issue may come from any of these layers:

```
ets2panda/linter/package.json
  → homecheck/package.json
    → arkanalyzer/package.json
  → build_linter.py
  → scripts/install-ohos-typescript-and-homecheck.mjs
  → package-lock.json files generated for each package layer
```

### Dependency Drift Rules

Treat dependency reproducibility as a compatibility boundary for the migration tool. A dependency-only change can break `npm run compile`, generated package contents, IDE integration, or downstream SDK builds even when linter TypeScript source code is unchanged.

**Rules**:
- Keep `package-lock.json` tracked for linter package layers that participate in build/package reproduction; do not hide them with `.gitignore`.
- When an install script temporarily mutates a managed `package.json`, back up and restore the matching `package-lock.json` as well.
- Treat an absent, ignored, or accidentally regenerated lockfile as dependency graph drift until proven intentional.
- Prefer explicit pins or npm `overrides` for transitive packages known to affect TypeScript declaration, runtime, or package-export compatibility.
- After changing dependencies or package scripts, inspect the resolved transitive versions in `package-lock.json`, not only direct dependencies in `package.json`.
- Verify the full package chain with the relevant build mode: pipeline uses specified Node.js/npm plus `build_linter.py`; local development follows `README.md` with `npm install` and `npm run install-ohos-typescript`.

**General pattern**: If lockfiles are missing or not restored after temporary installs, transitive packages can move under the same direct dependency declarations. The failure may appear as an unrelated TypeScript compile error, changed package contents, or SDK build break. Fix the reproducibility boundary first: restore or commit the correct lockfiles, then add pins/overrides only for dependencies whose resolved versions must remain constrained.

### Install Script Safety

`build_linter.py` and `scripts/install-ohos-typescript-and-homecheck.mjs` run `npm install`, `npm pack`, and local package installation across `arkanalyzer`, `homecheck`, and `linter`. These scripts can create or rewrite package metadata as a side effect.

**When touching these scripts**:
- Treat `package.json` and `package-lock.json` as a pair.
- If a lockfile did not exist before a temporary install, restore by removing the generated lockfile after packaging.
- If a lockfile existed before a temporary install, restore the original file after packaging.
- Confirm the final worktree contains only intentional dependency metadata changes.
- For pipeline behavior, reproduce with the specified Node.js/npm versions and `build_linter.py`.
- For local developer behavior, run `npm run install-ohos-typescript` and then `npm run build` when dependency/package behavior changes.

### Dependency Debugging Checklist

For npm compile/install failures, check in this order:

1. Identify which package layer fails: linter root, `homecheck`, or `arkanalyzer`
2. Compare direct dependencies in the nearest `package.json`
3. Inspect resolved transitive versions in the nearest `package-lock.json`
4. Check whether `@types/node` or another type package is resolved through multiple incompatible paths
5. Check whether `overrides` or exact pins are needed for known fragile transitive packages
6. Re-run the package chain from the lowest failing layer upward, using the same build mode that failed

Useful commands:

```bash
npm ls <package> @types/node
npm explain <package>
npm explain @types/node
npm --prefix arkanalyzer run compile
npm --prefix homecheck run compile
npm run compile
python3 build_linter.py --source-path <repo>/ets2panda/linter --output-path <output-dir> --npm <node-v14.21.1>/bin/npm --typescript <repo>/ets2panda/linter/<ohos-typescript-package>.tgz --version <linter-version>
```

## Expert Experience

**Common rationalizations (anti-excuses)**:

| Rationalization | Reality |
|---|---|
| "I'll modify the compiler" | Linter is for migration, not compiler semantics. Check decision table first. |
| "I'll update test expectations" | Inspect `.diff` first → fix logic → only update after confirming correctness. Migration test counts are hard constraints. |
| "I'll skip `rule-config.json`" | Rule won't be filtered in IDE mode. Synchronize ALL 8 or 6 points. |
| "I'll add tests later" | Rule may not be found in test runner. Add test cases immediately. |
| "I'll reuse compiler type system" | Linter uses TypeScript's type checker, not ArkTS-static. Do not copy compiler type relations. |
| "I'll rebuild `Scene` in my checker" | Makes project-level rules exponentially slower. Use `Utils.ts` helpers. |
| "I'll calculate offsets from token text" | `QuasiEditor` applies patches by original source offsets. Shifted offsets corrupt all later replacements. |
| "I'll hardcode SDK paths" | SDK directories change across versions. Use `ResolveSdks.ts` and `src/lib/data/*.json`. |
| "It's only a transitive npm patch update" | Type declarations and package exports can change build compatibility. Check lockfile resolution and compile every package layer. |
| "The script only changes package.json temporarily" | `npm install` can also rewrite `package-lock.json`; back up and restore both files. |

## Anti-Patterns

- Treating linter issues as compiler bugs → modifying main compilation pipeline
- Copying compiler semantic implementations into linter layer
- Updating test expectations without checking rule design purpose
- Adding `FaultID` but forgetting `FaultAttrs.ts`, `CookBookMsg.ts`, `rule-config.json`, docs
- Writing HomeCheck checker but not registering in `CheckerIndex.ts` / `ruleSet.json`
- Rebuilding `Scene`, call graph, DVFG inside HomeCheck checker
- Calculating autofix offsets from token text or intermediate text
- Hardcoding SDK API allowlist paths
- Changing stdout/stderr protocol in IDE mode without verifying consumer side
- Ignoring generated or updated `package-lock.json` files after package/install scripts
- Fixing a dependency build failure only in the root linter package while `homecheck` or `arkanalyzer` still resolves a different transitive graph

## Debugging And Verification

### Verification Checklist

When modifying linter rules or documentation:

- [ ] Decision table checked: TypeScriptLinter vs HomeCheck vs Compiler
- [ ] All synchronization points updated (8 for TypeScriptLinter, 6 for HomeCheck)
- [ ] Rule name in `CookBookMsg.ts` final parentheses: `"... (rule-name)"`
- [ ] Rule registered in `rule-config.json` (IDE filtering) or `homecheck/ruleSet.json` (HomeCheck)
- [ ] Test directory exists under `test/`, test cases added
- [ ] Build commands run successfully: `npm run build` or GN build
- [ ] Test commands run successfully: `npm run testrunner -- -d test/rules -p 'rule-name*'`
- [ ] No compiler files modified (unless intentional cross-component change)
- [ ] HomeCheck rule count verified against `homecheck/ruleSet.json`
- [ ] Dependency changes verified across linter, `homecheck`, and `arkanalyzer` package layers
- [ ] `package-lock.json` changes are intentional, reviewed, and restored when produced only by temporary install steps

### Common Issues Diagnosis

| Issue | Quick diagnosis |
|-------|-----------------|
| **Migration doesn't modify source** | Check: 1) `autofix` array exists, 2) not filtered by `--autofix-rule-config`, 3) no intersection conflict, 4) written to `results/` mapped path |
| **IDE doesn't show rule** | Check: 1) `rule-config.json` contains rule, 2) final parentheses rule name matches, 3) `ruleConfigTags` not filtered |
| **HomeCheck results missing** | Check: 1) `--ide-interactive --arkts-2 --homecheck` all passed, 2) file not filtered by `removeOutOfRangeFiles()`, 3) interop direction not filtered |
| **HomeCheck rule tag -1** | Check: final parentheses rule name in `metaData.description` can be found in `cookBookTag` (regex: `desc.match(/\(([^)]+)\)/)`) |
| **Test line/column differ** | Check: 1) `TsUtils.getHighlightRange()` logic, 2) node `getStart()` method, 3) autofix range calculation, 4) TSC diagnostics merge order changed |
| **`npm run compile` fails after dependency refresh** | Check: 1) failing package layer, 2) missing/rewritten `package-lock.json`, 3) transitive version drift, 4) type package compatibility, 5) npm `overrides`/pins |
| **Package script leaves dirty lockfiles** | Check: 1) temporary `npm install` paths, 2) backup/restore of both `package.json` and `package-lock.json`, 3) final `git status --short` |
| **Unexpected `.d.ts` type errors** | Check: whether an unlocked transitive dependency changed its declarations or pulled a different `@types/*` version; restore lockfile stability before changing source code |

## FAQ

1. **Lint results differ from compiler** → Check if designed difference or preceding semantic judgment error
2. **Migration/statistics script abnormal** → Check script input format, rule set, analyzer output chain
3. **Command line can scan, but IDE doesn't show rule** → Check `rule-config.json` contains rule, final parentheses rule name matches
4. **Migration doesn't modify source** → See Common Issues table above
5. **HomeCheck results missing** → See Common Issues table above
6. **SDK API rules miss reports** → Confirm `--sdk-default-api-path` points to `openharmony/ets` and contains `build-tools/ets-loader/declarations`
7. **Test fails but line/column differ slightly** → Do not update expectations first. Check `TsUtils.getHighlightRange()`, node `getStart()`, autofix range
8. **HomeCheck rule tag -1** → See Common Issues table above
9. **Rule counts missing from statistics** → Confirm `ProblemInfo.rule` contains final parentheses rule name, `rule-config.json` contains rule
10. **Dependency install or compile suddenly fails** → Check lockfiles and transitive type packages first; semver-compatible updates can still change `.d.ts` compatibility
11. **Why track linter lockfiles?** → The linter package chain depends on transitive npm resolution. Lockfiles preserve the tested graph for root linter, HomeCheck, and ArkAnalyzer.

## Related Documents

- `AGENTS.md` -- repository-level routing and constraints
- `ets2panda/Static_Frontend_Knowledge_Base.md` -- static frontend pipeline boundaries
- `ets2panda/LSP_Knowledge_Base.md` -- LSP interaction surface and downstream consumers
- `ArkGuard_Knowledge_Base.md` -- obfuscation and migration-adjacent tooling context
