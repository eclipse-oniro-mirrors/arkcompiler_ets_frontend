# Gluegen

Gluegen generates the symbol map required by static ArkTS sources that
participate in interoperability. It is part of the interop toolkits and is
invoked by Hvigor during a project build.

## Overview

Gluegen has two components:

| Component          | Location  | Responsibility                                                                                                 |
| ------------------ | --------- | -------------------------------------------------------------------------------------------------------------- |
| TypeScript wrapper | `src/`    | Resolves project inputs, prepares native input files, invokes the native executable, and forwards diagnostics. |
| Native generator   | `native/` | Generates the symbol map and diagnostic report.                                                                |

```text
Hvigor BuildConfig
        │
        ▼
TypeScript wrapper
  ├─ fileInfo.txt
  └─ arktsconfig.json
        │
        ▼
Native generator
  ├─ declgen-bridageconfig.json # decided by hvigor
  └─ report.json
```

## TypeScript wrapper

### Entry points

Hvigor uses the programmatic entry point:

```ts
export async function runGluegen(buildConfig: BuildConfig): Promise<void>;
```

The packaged command-line adapter accepts the serialized build configuration:

```sh
gluegen <build-config.json>
```

### Pipeline

Each invocation creates an isolated runner and executes four stages:

| Stage           | Purpose                                                                                               |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| `configuration` | Validates the build configuration, resolves the module graph, and reads module interop configuration. |
| `prepare`       | Collects participating static sources and writes `fileInfo.txt`.                                      |
| `arktsconfig`   | Generates the main module's `arktsconfig.json`.                                                       |
| `generation`    | Invokes the native generator and handles its diagnostic report.                                       |

### Native contract

The wrapper invokes the native executable with the generated inputs and the configured output path:

```text
gluegen[.exe]
  --input-file-list <fileInfo.txt>
  --arktsconfig <arktsconfig.json>
  --output <declgenBridgeConfigPath>
  --cache-path <cachePath>/gluegen
  --report-path <cachePath>/gluegen/report.json
```

The main intermediate files are:

| File                                         | Purpose                                             |
| -------------------------------------------- | --------------------------------------------------- |
| `<cachePath>/projectionConfig.json`          | Snapshot of the input build configuration.          |
| `<cachePath>/gluegen/fileInfo.txt`           | UTF-8 list of static source files passed to native. |
| `<cachePath>/<packageName>/arktsconfig.json` | ArkTS configuration for the main module.            |
| `<cachePath>/gluegen/report.json`            | Errors and warnings returned by native.             |

An empty `fileInfo.txt` is a successful no-op. Native exit codes are `0` for success, `1` for a generation diagnostic, and `2` for an internal failure. Calls that share project or cache paths must be serialized by the caller.

## Native generator

<!-- Native documentation will be added with the sources under native/. -->

## Building and testing

The TypeScript wrapper requires Node.js 18 or later.

```sh
npm run build
```

## Repository layout

```text
gluegen/
├── src/
│   ├── contracts/    # External build and report contracts
│   ├── native/       # TypeScript-side process and report adapters
│   ├── pipeline/     # Stage and artifact infrastructure
│   ├── stages/       # Gluegen processing stages
│   └── utils/
├── native/           # Native generator sources
├── test/             # Wrapper tests
├── BUILD.gn
└── package.json
```
