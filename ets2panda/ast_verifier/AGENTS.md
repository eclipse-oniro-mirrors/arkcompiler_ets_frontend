# AST Verifier Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

Before planning or editing, read `docs/knowledge/ets2panda/AST_Verifier_Knowledge_Base.md` (repository-relative) and follow the root verification/evidence rules.

Use this file for work under `ast_verifier/` together with the repository-level `AGENTS.md`.

## 1. Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | AST Verifier |
| **Purpose** | Encodes AST invariants and validates that parser/lowering output remains structurally correct. |
| **Primary Language** | C++ |

## 2. Responsibilities

- Define and enforce AST integrity checks.
- Catch invalid AST states before code generation.
- Guard lowering output with explicit verifier rules.

## 3. Hard Rules

- Never disable existing AST verifier checks in a patch.
- If new AST node kinds are introduced, add verifier checks for them.
- If a lowering creates new structural patterns, add verifier checks for those patterns.
- If a parser/lowering bug produced invalid AST that passed verification, extend AST verifier accordingly.
- Treat temporarily disabled checks as still normative for new code; do not add patches that rely on bypassing invariants.

## 4. Running

- In debug builds, AST verifier runs after lowering and before code generation.
- Useful options:
  - `--ast-verifier:phases=each`
  - `--ast-verifier:full-program`
  - `--ast-verifier:errors=...`
  - `--ast-verifier:warnings=...`

## 5. References

- Component README: `ets2panda/ast_verifier/README.md`
- Repository policy: `AGENTS.md`
