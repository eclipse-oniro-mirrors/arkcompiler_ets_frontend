# Documentation Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

Use this file for work under `docs/` together with the repository-level `AGENTS.md`.

## 1. Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Documentation |
| **Purpose** | Contributor-facing docs for frontend architecture, rules, workflows, and onboarding. |
| **Primary Language** | Markdown |

## 2. Scope

- Keep docs aligned with current specification-first policy.
- Keep commands, paths, and options accurate for the current tree/build flow.
- Use concise, actionable guidance with runnable command examples.
- Keep docs aligned with the latest technical preview release feed: <https://gitcode.com/igelhaus/arkcompiler_runtime_core/releases/>.

## 3. Rules

- Do not document behavior that is not in the latest technical preview spec.
- When implementation and spec diverge, call out the mismatch explicitly.
- Do not remove hard review constraints from docs (tests required, no assertion removal, no speculative features).
- Prefer repository-relative paths in links.

## 4. When Updating Docs

- Update `ets2panda/docs/README.md` when adding or renaming major docs.
- Keep onboarding guidance consistent with root `AGENTS.md`.
- Keep root `AGENTS.md`, component `AGENTS.md`, and `ets2panda/docs/frontend-onboarding.md` consistent on hard rules.
- If a change modifies component process/rules, update the closest component `AGENTS.md` in the same patch.
