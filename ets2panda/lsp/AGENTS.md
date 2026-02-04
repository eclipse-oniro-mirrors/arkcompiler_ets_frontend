# ets2panda LSP Module

This file provides guidance for working with the Language Server Protocol implementation in the ets2panda compiler.

## Overview

The LSP module provides language services for ArkTS/ETS/TypeScript, including:
- Code completion
- Go to definition
- Find references
- Rename
- Code actions/refactors
- Signature help
- Quick info (hover)
- Diagnostics
- Formatting

The LSP functionality is split across two directories:
- **`lsp/`** - C++ implementation with LSP API
- **`bindings/`** - TypeScript/JavaScript Node.js bindings

## Directory Structure

```
ets2panda/
├── lsp/                          # C++ LSP implementation
│   ├── include/                  # Public headers
│   │   ├── formatting/           # Formatting support
│   │   ├── refactors/            # Code refactor actions
│   │   ├── register_code_fix/    # Auto-generated code fixes
│   │   ├── code_fixes/           # Code fix types
│   │   └── services/             # Service utilities
│   ├── src/                      # Implementation files (.cpp)
│   │   ├── formatting/
│   │   ├── refactors/
│   │   ├── register_code_fix/
│   │   └── services/
│   ├── CMakeLists.txt            # CMake build configuration
│   ├── BUILD.gn                  # GN build configuration
│   ├── code_fix_register.h.erb   # Code fix template
│   └── code_fix_register.rb      # Code fix generator script
│
├── bindings/                     # Node.js bindings
│   ├── src/                      # TypeScript source
│   │   ├── common/               # Common utilities
│   │   ├── generated/            # Generated bindings
│   │   └── lsp/                  # LSP-specific implementations
│   │       ├── lsp_helper.ts     # Main LSP helper class
│   │       ├── lspNode.ts        # LSP node types
│   │       ├── generateArkTSConfig.ts
│   │       ├── generateBuildConfig.ts
│   │       └── index.ts
│   ├── native/                   # Native C++ bindings
│   │   └── src/
│   │       └── lsp.cpp           # Node.js API implementation
│   ├── test/                     # Binding tests
│   │   ├── testcases/            # Test case files
│   │   └── e2e/                  # Expected results and run tests
│   ├── package.json
│   └── tsconfig.json
│
└── test/unit/lsp/                # C++ unit tests (GTest)
    ├── CMakeLists.txt            # Test suite definitions
    ├── get_completions.cpp       # Completion tests
    ├── get_definition_at_position_test.cpp
    ├── find_references_test.cpp
    ├── formatting_test.cpp
    ├── quick_info_test.cpp
    ├── refactor_extract_symbol_test.cpp
    ├── lsp_rename_test.cpp
    └── ... (100+ test files)
```
## Development Process

### Adding a New LSP Method
1. Add C++ implementation in `lsp/src/` with header in `lsp/include/`
2. Add C wrapper in `lsp/src/api.cpp`
3. Update `lsp/include/api.h` if external access needed
4. Update `lsp/CMakeLists.txt` with new source file
5. Update `lsp/BUILD.gn` with new source file
6. Update `test/unit/lsp/CMakeLists.txt` with new test file
7. Add native binding in `bindings/native/src/lsp.cpp`
8. Add TypeScript node type in `bindings/src/lsp/lspNode.ts` if needed
9. Add TypeScript declaration in `bindings/src/common/Es2pandaNativeModule.ts`
10. Add TypeScript implement in `bindings/src/lsp/lsp_helper.ts`
11. Add bindings test in `bindings/test/e2e` and `bindings/test/testcases`


## Build

### Build the LSP module

```bash
# Full build
cmake --build {cmake-build-dir}
```

### Build Bindings
```bash
cd bindings

npm install

npm run run
```

### Code Generation

Code fix registration is auto-generated during build:

```bash
# Input files (YAML diagnostics)
util/diagnostic/syntax.yaml
util/diagnostic/semantic.yaml
util/diagnostic/warning.yaml
util/diagnostic/fatal.yaml

# Template
code_fix_register.h.erb

# Ruby script
code_fix_register.rb

# Output (generated during build)
${GENERATED_DIR}/code_fix_register.h
```

## Code Fix Registration

Code fixes are auto-generated from diagnostic YAML files via `code_fix_register.rb`:
- Input: YAML files in `util/diagnostic/`
- Template: `code_fix_register.h.erb`
- Output: Generated registration code in `${GENERATED_DIR}`

To add a new code fix:
1. Add entry to appropriate diagnostic YAML (syntax.yaml, semantic.yaml, etc.)
2. Implement fix in `src/register_code_fix/`
3. Rebuild to generate registration code

## Test

### GTest
`test/unit/lsp`
The map from test suite to test files could refer to ets_frontend/ets2panda/test/unit/lsp/CMakeLists.txt
```bash
cd {cmake-build-dir}

ninja {test_suite_name}

./bin-gtest/{test_suite_name}
```

### Bindings Test
`bindings/test`
Need to have ets folder under `bindings/`
```bash
cd bindings

npm install

npm run test
```

## Code Check

Aim to check the code style and detect potential bugs.
```bash
# Check modified files
python {arkcompiler-dir}/runtime_core/static_core/scripts/clang-tidy/clang_tidy_check.py {arkcompiler-dir}/runtime_core/static_core {cmake-build-dir} --filename-filter '{modified-file1}|{modified-file2}|...'

# Format modified files
bash {arkcompiler-dir}/runtime_core/static_core/scripts/code_style/run_code_style_tools.sh {modified-file}
```

## Key Dependencies

C++ LSP module:
- `ir/astNode.h` - AST node hierarchy
- `checker/types/type.h` - Type system
- `public/es2panda_lib.h` - Public compiler API
- `util/eheap.h` - Arena allocator

TypeScript bindings:
- Node.js N-API
- TypeScript compiler API
- @types/node