# gluegen end-to-end tests

Black-box tests that spawn the real, built `gluegen` executable and check its behavior -- as
opposed to `test/native/unit/`, which links gluegen's sources directly into a gtest binary and
calls `Gluegen::Run()` in-process. Only a real subprocess run can observe `main.cpp`'s CLI parsing
and process exit code, which is what `negative-cases/` needs.

Run via `ninja gluegen_e2e_tests` from your CMake+Ninja build directory (this builds `gluegen`
first, then runs the suite -- see CMakeLists.txt), or invoke the runner directly:

```sh
node runner/run-e2e.js \
  --gluegen-binary /path/to/build/bin/gluegen \
  --default-arktsconfig /path/to/build/bin-gtests/arktsconfig.json \
  --positive-cases-dir positive-cases \
  --negative-cases-dir negative-cases \
  [--run-prefix "qemu-aarch64 -L /sysroot"]
```

`--run-prefix` mirrors CMake's `PANDA_RUN_PREFIX` (only set for cross-compiled toolchains, e.g.
`runtime_core/*/cmake/toolchain/cross-*-qemu-*.cmake`); leave it unset for a native host build.

## Directory layout

Each immediate subdirectory of `positive-cases/` or `negative-cases/` is one test case:

```
<case-name>/
├── case.json          # optional; see schema below
├── input/*.ets         # source files (default inputFiles discovery: all *.ets here, sorted)
├── arktsconfig.json     # optional, only needed to override args.arktsconfig
└── expected/
    └── output.json      # positive-cases only; compared against gluegen's --output
```

- **`positive-cases/`**: gluegen is expected to exit **0**. This includes source files that are
  syntactically invalid -- per `Gluegen::Run()`, a syntax error is a normal, terminal outcome
  (`{"status": "syntax-error"}` written to `--output`, exit code 0), not a tool failure. Compare
  the actual gluegen output against `expected/output.json`.
- **`negative-cases/`**: gluegen is expected to exit **non-zero** because of a usage/environment
  problem (bad CLI args, missing/malformed `arktsconfig.json`, a referenced source file that
  doesn't exist, ...) -- never because of anything wrong with the *source* being processed. The
  runner asserts the exit code, optionally a `stderr` substring/regex and/or a DiagnosticEngine
  code+severity (via the always-supplied `--report-path`, see `expect.diagnostics` below), and
  that `--output` was **not** created (a partial/stale glue file must never be left behind for a
  downstream step to consume).

## `case.json` schema

All fields are optional; defaults depend on whether the case lives under `positive-cases/` or
`negative-cases/`.

```jsonc
{
  "description": "human-readable summary, shown on failure",

  "args": {
    // "default" (use --default-arktsconfig), null (omit --arktsconfig entirely), or a path
    // relative to this case directory (e.g. to point at a deliberately broken/missing config).
    "arktsconfig": "default",
    // "auto" (use a fresh scratch dir, i.e. exercise --cache-path) or omitted (no --cache-path).
    "cachePath": null
  },

  // Paths relative to this case directory. Defaults to every *.ets file under input/, sorted.
  // A path that doesn't actually exist on disk is still passed to gluegen (not copied) -- used to
  // simulate a missing source file.
  "inputFiles": ["input/a.ets"],

  "expect": {
    "exitCode": 0,                        // default: 0 for positive-cases, 1 for negative-cases
    "outputFile": "expected/output.json", // positive-cases only; null skips the output comparison
    "stderrContains": "substring",        // negative-cases only; plain substring match against stderr
    "stderrMatches": "regex",             // negative-cases only; regex match against stderr (e.g. for
                                           // messages that embed an environment-dependent absolute
                                           // path, where a fixed substring can't be hardcoded here).
                                           // Note: gluegen's stderr ends with a trailing newline, and
                                           // JS's `$` anchor does NOT match before a trailing "\n"
                                           // (unlike Perl/PCRE) unless the `m` flag is set -- so avoid
                                           // anchoring patterns with a trailing `$`.
    "diagnostics": [                      // negative-cases only; asserts against DiagnosticEngine's
                                           // own structured report (written via --report-path, which
                                           // the runner always passes), rather than prose scraped out
                                           // of stderr -- so a wording-only change to a diagnostic's
                                           // description can't spuriously break this suite. Each entry
                                           // must be found among the matching severity bucket
                                           // (`diagnostics.errors`/`diagnostics.warnings`) in the
                                           // report by `code` alone (see DiagnosticCode in
                                           // native/include/diagnostic.hpp for the numeric values).
      { "code": 116001, "severity": "error" }
    ]
  }
}
```

## Path normalization

Each case runs against a private, temporary scratch copy of its `input/` files (so cases can run
in parallel and never pollute the source tree), so absolute paths inside gluegen's actual output
never match a golden file literally. Before comparing, every occurrence of the scratch directory's
absolute path is replaced with the placeholder `<CASE_DIR>` -- write your `expected/output.json`
using that same placeholder (e.g. `"<CASE_DIR>/input/a.ets"`).

## Updating golden files

Run with `--update-golden` to regenerate every `positive-cases/*/expected/output.json` from
gluegen's actual current output (already `<CASE_DIR>`-normalized). Always diff the result before
committing.
