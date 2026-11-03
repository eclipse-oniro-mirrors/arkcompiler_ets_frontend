# Declgen Spec Test Suite (test/spec)

A spec test suite with the case directories as the single source of truth (SSOT):
every case under `sections/` serves as both an **executable test** and **spec
documentation material**. The `framework/` (spec-kit) exports two Markdown
documents (Chinese and English) from the same cases, closing two anti-drift
loops:

- change code → tests fail → update case/golden → re-render docs → `render --check` gates doc sync;
- change doc intent → update case descriptions/goldens first → surface code deltas → then fix code.

Design details: [docs/spec-test-design.md](../../docs/spec-test-design.md).

## Directory Layout

```text
test/spec/
├── SUMMARY.md          # Single source of truth for chapter order (2-space indent = one level)
├── spec.config.json    # docId, bilingual titles, frontMatter, structure (topic-major|direction-major)
├── sections/           # SSOT: direction (dyn2sta/sta2dyn) × topic × case
│   └── <dir>/
│       ├── node.json           # Direction groups only: {"kind": "direction"}
│       ├── section.{zh,en}.md  # Narrative + front-matter title (bilingual pairs)
│       └── <case>/
│           ├── case.json       # Only {compat, tags, pending}
│           ├── desc.{zh,en}.md # Scenario descriptions; {{input}}/{{output}}/{{sample:...}} placeholders inject code blocks
│           ├── input/          # Dynamic-side inputs (.ts); the whole dir runs through the package_source generator
│           ├── output/         # Blessed goldens (carry a license header; never hand-edit)
│           ├── sample.input.ets    # Optional: single-file path input
│           └── sample.output.d.ets # Optional: single-file path golden
├── docs/               # Generated: arkts-sta-declgen-spec.{zh,en}.md (checked in; do not hand-edit)
├── framework/          # spec-kit: loader / lint / render / test / new / cli
│   └── dist/           # Build output (build:spec-kit)
├── ut/                 # spec-kit self-tests (22 cases)
└── specSuite.test.ts   # jest adapter: runs each case twice (fixture + sample) and compares
```

## Common Commands (run from declgen/)

| Command | Purpose |
|---|---|
| `npm run test:spec` | Full chain: build declgen and spec-kit, run `ut` + `specSuite` |
| `npm run test:spec-kit` | spec-kit self-tests only |
| `npm run build:spec-kit` | Compile framework → dist only |
| `npm run spec:lint` | Content lint (titles / fence alignment / placeholders / anchors / compat order) |
| `npm run spec:render` | Regenerate docs/*.md; append `-- --check` for byte comparison (CI gate) |
| `npm run spec:new -- <parent/path> <slug>` | Scaffold a new case interactively |
| `npx jest test/spec -- -t '<case-relPath>'` | Run a single case only |

Underlying CLI: `node test/spec/framework/dist/cli.js <lint|render|test|new> [options]`
(`test` supports `--update` to bless and `--filter <substr>`; `render` supports
`--lang zh|en`, `--structure <s>`, `--check`, `--all`).

## Authoring and Maintenance Rules

1. **SUMMARY.md is the single ordering source**: directories and SUMMARY entries
   must match bidirectionally (enforced by the loader as hard errors).
2. **Bilingual pairs**: `*.zh.md` / `*.en.md` are edited together with matching
   titles (topic titles must match across direction groups).
3. **compat ordering**: cases are ordered `fully-compatible → requires-transform → incompatible`.
4. **Golden discipline**: `output/` and `sample.output.d.ets` may only be written
   by `test --update`, and every bless diff must be human-reviewed before
   acceptance. Comparison inherits the cookbook convention (TypeScript printer
   strips comments before diffing), so goldens carry a license header while doc
   injection strips it.
5. **pending**: until the static side (sta2dyn) generator is wired in, mark such
   cases `"pending": true`; tests skip them while lint/render still cover them.
6. **Code gate**: nesting depth ≤ 4 and function body ≤ 50 lines for new code.
7. **Byte-stable docs**: `docs/` is a checked-in build product; after any case
   change, re-render and make sure `render --check` passes.
