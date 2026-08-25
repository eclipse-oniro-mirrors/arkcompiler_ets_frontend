# Dependency Resolver

Dependency Resolver builds a merged dependency graph for static and dynamic
ArkTS sources participating in interoperability. It combines TypeScript module
resolution for dynamic sources with the Panda SDK `dep_analyzer` for static
sources, then preserves cross-language references in one graph.

## Overview

```text
Project source files
  ├─ dynamic ArkTS ── DynamicResolver ─┐
  └─ static ArkTS ─── StaticResolver ──┼─ CrossLanguageResolver ── DependencyGraph
                                       │
                                       └─ graphToViewModel ── renderGraphHtml
```

Each partial resolver records references to the other language as sentinel
nodes. `CrossLanguageResolver` merges the two partial graphs, unions dependency
edges, and populates reverse dependant edges.

## Features

- Resolves dynamic ArkTS imports, exports, `import =` references, and dynamic
  `import()` calls with the TypeScript module resolver.
- Resolves static ArkTS dependencies through the Panda SDK `dep_analyzer`.
- Represents cross-language dependencies as sentinel nodes while retaining
  whether each node was later resolved by its own language resolver.
- Provides direct and transitive dependency and dependant queries.
- Generates a self-contained interactive HTML graph with zooming, panning,
  color controls, and an SDK visibility toggle.

## Programmatic Use

The package is a TypeScript library. It does not currently provide a command
line interface.

Create a `Context` with a populated `FileManager`, construct one resolver for
each language, and pass them to `CrossLanguageResolver`:

```ts
import * as ts from 'typescript';
import * as common from '@interop-toolkits/common';
import { CrossLanguageResolver, DynamicResolver, StaticResolver, type Context } from 'dependency-resolver';

const fileManager = new common.fileManager.FileManagerBuilder()
  .addStaticSdkPaths(staticSdkPaths)
  .addDynamicSdkPaths(dynamicSdkPaths)
  .addStaticInteropSdkPaths(staticInteropSdkPaths)
  .addDynamicInteropSdkPaths(dynamicInteropSdkPaths)
  .addModuleList(modules)
  .build();

const context: Context = {
  fileManager,
  cachePath,
};

const dynamicResolver = new DynamicResolver(projectRootPath, {
  module: ts.ModuleKind.NodeNext,
  moduleResolution: ts.ModuleResolutionKind.NodeNext,
  target: ts.ScriptTarget.ES2021,
});
const arktsconfigPath = '/path/to/arktsconfig.json';
const staticResolver = new StaticResolver(depAnalyzerPath, arktsconfigPath);
const graph = new CrossLanguageResolver(context, dynamicResolver, staticResolver).resolve();
```

`common.fileManager.FileManagerBuilder` must receive all project modules and all applicable SDK
paths. `StaticResolver` invokes `dep_analyzer` with project static source files
and requires the path to a generated `arktsconfig.json`.

### Querying the Graph

```ts
const directDependencies = graph.getDependencies('/project/src/main.ets');
const transitiveDependencies = graph.getDependencyChain('/project/src/main.ets');
const sameLanguageDependencies = graph.getPartialDependencyChain('/project/src/main.ets');
const directDependants = graph.getDependants('/project/src/shared.ets');
```

Graph keys and query file names are normalized paths. A node with
`isSentinel: true` was discovered across a language boundary; `isResolved`
indicates that the resolver for that node's language also produced its real
node.

## HTML Visualization

Convert a resolved graph to a view model and render a standalone HTML file:

```ts
import { writeFile } from 'node:fs/promises';
import { graphToViewModel, renderGraphHtml } from 'dependency-resolver';

const model = graphToViewModel(graph, context);
const html = renderGraphHtml(model, {
  title: 'Project Dependencies',
  colors: {
    static: '#f4c430',
    dynamic: '#3b82f6',
  },
});

await writeFile('dependency-graph.html', html, 'utf-8');
```

Open the generated file directly in a browser. Static files are displayed in
yellow and dynamic files in blue by default. Sentinel nodes are squares; other
files are circles. Edge arrows are shown at the midpoint of each dependency
line. The page can hide and restore SDK and interop SDK nodes with the toolbar.

## Build and Test

The package requires Node.js 18 or later. Install dependencies from this
directory, then run the desired package script:

```sh
npm install
npm run build
npm test
npm run lint
npm run format:check
```

`npm run install-tsc` installs the OpenHarmony TypeScript compiler used by the
package when the default dependency installation does not provide it.

## Repository Layout

```text
dependency-resolver/
├── src/
│   ├── context/       # Resolver context contract
│   ├── resolver/      # Dynamic, static, and merged graph resolvers
│   └── visualize/     # Graph view model and standalone HTML renderer
├── test/ut/           # Unit tests
├── scripts/           # Toolchain setup scripts
└── package.json
```
