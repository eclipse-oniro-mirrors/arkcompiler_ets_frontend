---
name: linter-knowledge-base
description: Reference knowledge base for ets2panda/linter migration tool. Use when modifying TypeScriptLinter/HomeCheck rules, debugging autofix conflicts, investigating lint behavior, or setting up linter build/test. Use when working in ets2panda/linter directory.
---

# Linter Knowledge Base

> Document version: v1.0
> Last updated: 2026-06-04
> Scope: `arkcompiler/ets_frontend/ets2panda/linter` (repo-relative path: ets2panda/linter)

## Summary

1. Linter is migration assistance (≠ compiler), helps ArkTS-dynamic → ArkTS-static
2. TypeScriptLinter (single-file) vs HomeCheck (cross-file) → check Decision Table first
3. Rule changes need 8-point/6-point synchronization → follow Verification Checklist

## When to Use

- Modifying TypeScriptLinter or HomeCheck rules
- Debugging autofix conflicts or migration behavior
- Investigating false positives/negatives in lint
- Setting up linter build/test environment

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

## Top 10 Don'ts (Critical)

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

### Build Commands (drift-prone, verify current version)

| Command | Purpose | When to use |
|---------|---------|-------------|
| `npm install` | Install dependencies | First-time setup |
| `npm run install-ohos-typescript` | Install ohos-typescript, arkanalyzer, homecheck | After dependency update |
| `npm run build` | Local build (clean → compile → webpack → pack) | Before testing |
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

### Common Issues Diagnosis

| Issue | Quick diagnosis |
|-------|-----------------|
| **Migration doesn't modify source** | Check: 1) `autofix` array exists, 2) not filtered by `--autofix-rule-config`, 3) no intersection conflict, 4) written to `results/` mapped path |
| **IDE doesn't show rule** | Check: 1) `rule-config.json` contains rule, 2) final parentheses rule name matches, 3) `ruleConfigTags` not filtered |
| **HomeCheck results missing** | Check: 1) `--ide-interactive --arkts-2 --homecheck` all passed, 2) file not filtered by `removeOutOfRangeFiles()`, 3) interop direction not filtered |
| **HomeCheck rule tag -1** | Check: final parentheses rule name in `metaData.description` can be found in `cookBookTag` (regex: `desc.match(/\(([^)]+)\)/)`) |
| **Test line/column differ** | Check: 1) `TsUtils.getHighlightRange()` logic, 2) node `getStart()` method, 3) autofix range calculation, 4) TSC diagnostics merge order changed |

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

## Related Documents

- `AGENTS.md` -- repository-level routing and constraints
- `ets2panda/Static_Frontend_Knowledge_Base.md` -- static frontend pipeline boundaries
- `ets2panda/LSP_Knowledge_Base.md` -- LSP interaction surface and downstream consumers
- `ArkGuard_Knowledge_Base.md` -- obfuscation and migration-adjacent tooling context
