# driver Module

**Name**: driver
**Purpose**: Orchestrates ArkTS/ETS compilation pipeline: build configuration, dependency analysis, compilation execution, and output linking. Bridges the compiler (ets2panda) with build systems (hvigor).
**Primary Language**: TypeScript (build_system) + C++ (dependency_analyzer)

## Directory Structure

```
driver/
├── build_system/                    # TypeScript build orchestration
│   ├── src/
│   │   ├── entry.ts                 # Entry point, build orchestration
│   │   ├── types.ts                 # TypeScript type definitions
│   │   ├── pre_define.ts            # Constants and configuration
│   │   ├── dependency_analyzer.ts   # TypeScript wrapper for dependency analysis
│   │   ├── logger.ts                # Logging utility
│   │   ├── build/                   # Build modes and workers
│   │   │   ├── build_mode.ts        # Standard build mode
│   │   │   ├── base_mode.ts         # Base build mode
│   │   │   ├── build_framework_mode.ts # Framework build mode
│   │   │   ├── generate_arktsconfig.ts # ArkTS config generation
│   │   │   ├── compile_process_worker.ts # Process-based compilation
│   │   │   ├── compile_thread_worker.ts  # Thread-based compilation
│   │   │   └── declgen_process_worker.ts # Declaration generation
│   │   ├── init/                   # Initialization
│   │   │   ├── process_build_config.ts  # Process build config JSON
│   │   │   └── init_koala_modules.ts   # Initialize Koala modules
│   │   ├── util/                   # Utilities
│   │   │   ├── ets2panda.ts         # ets2panda wrapper
│   │   │   ├── graph.ts              # Graph data structure
│   │   │   ├── TaskManager.ts        # Worker task management
│   │   │   ├── statsRecorder.ts       # Performance statistics
│   │   │   └── [other utilities]
│   │   └── plugins/                # Plugin system
│   │       ├── plugins_driver.ts     # Plugin driver
│   │       ├── KitImportTransformer.ts # @kit import transform
│   │       └── FileManager.ts        # File operations
│   ├── test/                       # Test suites
│   │   ├── ut/                      # Unit tests
│   │   ├── e2e/                     # End-to-end tests
│   │   └── plugin/                  # Plugin tests
│   ├── docs/                       # Documentation
│   ├── package.json
│   ├── tsconfig.json
│   └── eslint.config.mjs
└── dependency_analyzer/              # C++ dependency analysis tool
    ├── dep_analyzer.h
    ├── dep_analyzer.cpp
    ├── main
    ├── CMakeLists.txt
    └── BUILD.gn
```

## Build Commands

### Building

```bash
npm install
npm run build            # Compile TypeScript to dist/
npm run build_debug      # Build with source maps
```

## Architecture

### Build Pipeline

1. **Configuration Processing** - Parse and validate build config JSON
2. **Module Collection** - Build module dependency graph
3. **arktsconfig Generation** - Generate arktsconfig.json for each module
4. **Dependency Analysis** - Run C++ analyzer to build file dependency graph
5. **Compilation** - Execute compilation (parallel/simultaneous/sequential)
6. **Linking** - Merge ABC files into final output

### Key Components

- **Build Modes**: BaseMode (parallel/concurrent/simultaneous), BuildMode, BuildFrameworkMode
- **Dependency Analysis**: TypeScript wrapper + C++ analyzer tool
- **arktsconfig Generation**: Singleton generator with SDK dependency resolution
- **Plugin System**: PARSED, CHECKED, CLEAN hooks
- **Task Management**: Worker pool for parallel compilation

## Dependencies

****Used by**: hvigor (build system)
**Uses**: ets2panda (C++ compiler), Koala bindings

## Development Notes

### Build Configuration

Build config is passed as JSON from hvigor. Key fields include:
- buildType, buildMode, es2pandaMode
- moduleType (HAP/FEATURE/SHARED/HAR)
- sourceRoots, compileFiles
- dependencyModuleList
- paths, plugins, SDK paths

### Incremental Builds

File hash caching (hash_cache.json) enables incremental compilation. Unchanged files are filtered from the dependency graph.

### Graph Clustering

Optional clustering merges files into batches when file count exceeds CLUSTER_FILES_TRESHOLD (460) for improved parallelism.

### Worker Communication

Workers communicate via message passing. Message types include DECL_GENERATED, ABC_COMPILED, ERROR_OCCURED, ASSIGN_TASK.

### Plugin Development

Plugins can hook into PARSED (after parsing), CHECKED (after type checking), and CLEAN (after compilation) stages. Use PluginDriver to register and execute plugins.
