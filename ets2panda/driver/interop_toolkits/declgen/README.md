# Declgen

Declgen generates interoperability declaration files for static and dynamic
ArkTS sources. It resolves configured interop entries, builds a cross-language
dependency graph, and emits declarations for each language closure.

Declgen is part of the interop toolkits and is invoked by Hvigor during a
project build.

## Entry Point

Hvigor calls the programmatic entry point with its build configuration:

```ts
import { runDeclgen } from 'declgen';

await runDeclgen(buildConfig);
```

`runDeclgen` initializes the build configuration and logger, stores a
`buildConfig.json` snapshot in `cachePath`, then runs `DeclgenRunner`.

## Pipeline

`DeclgenRunner` executes these stages in order:

| Stage                          | Purpose                                                                            |
| ------------------------------ | ---------------------------------------------------------------------------------- |
| `resolve-interop-entries`      | Resolves configured static and dynamic interop entry files.                        |
| `arktsconfig`                  | Generates the ArkTS configuration consumed by static dependency analysis.          |
| `resolve-dependency-graph`     | Merges static Panda analysis with dynamic TypeScript resolution.                   |
| `compute-interop-closure`      | Validates cross-language sentinels and computes per-language declaration closures. |
| `generate-interop-declaration` | Runs static and dynamic declaration generation for those closures.                 |

```text
BuildConfig
    │
    ▼
resolve interop entries ──► generate ArkTSConfig
    │                              │
    └────► resolve dependency graph ┘
                   │
                   ▼
          compute interop closures
                   │
                   ▼
       generate static and dynamic declarations
```

The dependency graph preserves cross-language references as sentinel nodes.
When a source file is reached across a language boundary but is absent from the
interop configuration, Declgen reports an invalid interop configuration error
before declaration generation begins.

## Dependencies

Declgen uses:

- `@interop-toolkits/common` for file indexing, ArkTS configuration generation,
  interop configuration parsing, diagnostics, and pipeline infrastructure.
- `dependency-resolver` to combine static and dynamic source dependencies.
- `@es2panda/bindings` for the static declaration generator.
- OpenHarmony TypeScript for dynamic declaration generation.

## Building and Testing

The package requires Node.js 18 or later and the local bindings package used by
the static generator.

```sh
npm install
npm run build
npm test
npm run lint
npm run format:check
```

Targeted test commands are available for each generator:

```sh
npm run test:static
npm run test:dynamic
npm run test:dynamic:cookbook
```

## Repository Layout

```text
declgen/
├── src/
│   ├── adapter/    # Static and dynamic generator adapters
│   ├── dynamic/    # Dynamic declaration transformation pipeline
│   ├── runner/     # Cross-language declgen pipeline and stages
│   ├── static/     # Static declaration generation and worker scheduling
│   ├── entry.ts    # Public runDeclgen entry point
│   └── buildConfig.ts
├── test/
│   ├── dynamic/    # Dynamic unit and cookbook tests
│   └── static/     # Static declaration generation tests
├── scripts/
└── package.json
```
