# Interop Toolkits Common

`@interop-toolkits/common` provides the shared contracts, configuration
utilities, file classification, diagnostics, and pipeline primitives used by
the interop toolkits.

## Public API

The package exposes most APIs through namespaces. Import the package once and
access the owning namespace:

```ts
import * as common from '@interop-toolkits/common';

const fileManager = new common.fileManager.FileManagerBuilder()
  .addModuleList(modules)
  .addStaticSdkPaths(staticSdkPaths)
  .addDynamicSdkPaths(dynamicSdkPaths)
  .build();
```

| Namespace                   | Contents                                                                            |
| --------------------------- | ----------------------------------------------------------------------------------- |
| `common.fileUtils`          | File extensions, source language definitions, and path normalization.               |
| `common.fileManager`        | `FileManager` construction, module metadata, SDK lookup, and source classification. |
| `common.arktsconfig`        | ArkTS configuration contracts and configuration generation rules.                   |
| `common.interopConfig`      | Interop configuration parsing, target types, and validation errors.                 |
| `common.framework.pipeline` | Typed artifact and stage primitives for ordered build pipelines.                    |
| `common.errors`             | Shared user-facing and internal error types.                                        |
| `common.hvigorLogger`       | Hvigor-compatible logging utilities.                                                |

The Hvigor logger types and factory are also exported directly for compatibility:

```ts
import { createHvigorLogger, type ILogger, LogData } from '@interop-toolkits/common';
```

## File Management

`FileManagerBuilder` accepts project modules and SDK declaration roots, then
builds an immutable index. `FileManager` classifies a registered file as static
or dynamic source, SDK, or interop SDK and exposes the associated `FileMeta`.

SDK lookups accept ArkTS platform module names such as `@ohos.*`, `@system.*`,
`@kit.*`, and `@arkts.*`. An unresolved lookup returns `undefined`.

## Pipeline Framework

The pipeline framework exchanges data between stages with typed artifacts:

```ts
const artifact = common.framework.pipeline.createArtifact<string>('example');

const stage = common.framework.pipeline.Stage.start<Context>('example').provides(artifact, {
  build: () => 'result',
});
```

A later stage declares the artifact in `.requires(...)` and reads it through
`scope.get(artifact)`. Within a stage, `.use(...)` registers named sequential
hook outputs that `.provides(...)` combines into the stage artifact.

## Building and Testing

The package requires Node.js 18 or later.

```sh
npm install
npm run build
npm test
npm run lint
npm run format:check
```

## Repository Layout

```text
common/
├── src/
│   ├── arktsconfig/     # ArkTS configuration generation
│   ├── framework/       # Typed artifact and pipeline primitives
│   ├── interop-config/  # Interop configuration resolution
│   ├── fileManager.ts   # Source and SDK file index
│   ├── fileUtils.ts     # Extensions, languages, and paths
│   └── hvigorLogger.ts  # Hvigor logging adapter
├── test/                # Unit tests
└── package.json
```
