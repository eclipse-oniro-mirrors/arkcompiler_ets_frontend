# es2panda

ArkTS/ETS frontend parser, checker, lowering pipeline, and bytecode compiler.

## Specification Source of Truth

- Latest technical preview release feed: <https://gitcode.com/igelhaus/arkcompiler_runtime_core/releases/>.
- Frontend behavior changes must follow the latest technical preview specification.
- Tests/apps/legacy behavior are not the source of truth when they conflict with spec.

## CLI Usage

```sh
es2panda [OPTIONS] [input file] -- [arguments]
```

- Option definitions: `util/options.yaml`
- Tail argument: `input` (input file)

## Build/Run Smoke Flow

Run from a build directory that contains `./bin`:

```sh
./bin/es2panda --extension=ats --opt-level=0 --output=out.abc fault.ets
./bin/verifier --boot-panda-files=./plugins/ets/etsstdlib.abc --load-runtimes=ets out.abc
./bin/ark --boot-panda-files=./plugins/ets/etsstdlib.abc --load-runtimes=ets --panda-files=out.abc out.abc fault.ETSGLOBAL::main
```

## Running Frontend Test Suites

Run from `static_core`:

```sh
static_core/tests/tests-u-runner/main.py --force-generate --ets-cts --build-dir .
```

Useful options: `--processes 6`, `--verbose short`.

## Debugging Aids

- AST source dump: `node->DumpEtsSrc()`
- Type dump: `type->ToString()`
- Signature dump: `sig->ToString()`
- Dump source after a phase: `--dump-ets-src-after-phases=<PhaseName>`

## Documentation

- Onboarding: `docs/frontend-onboarding.md`
- Docs index: `docs/README.md`
- Repository/component rules: `AGENTS.md`, `*/AGENTS.md`
