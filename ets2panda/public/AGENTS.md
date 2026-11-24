# Public API Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

Use this file for work under `public/` together with the repository-level `AGENTS.md`.

## 1. Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Public |
| **Purpose** | Exposes ets2panda’s C++ API as a stable C interface (es2panda_lib) for plugins, SDKs, and external tools. |
| **Primary Language** | C++; YAML/Ruby/ERB (codegen); Python (headers_parser) |

## 2. C API Source and Generation

- **Source**: The exported AstNode and related APIs **come from the AST node classes and their methods in `ir/`** (and from other exported headers for checker, parser, varbinder). Headers to export are listed in **HEADERS_TO_BE_PARSED** in **CMakeLists.txt**; their public classes and methods are parsed and then emitted as the C API.
- **Pipeline**: headers_parser parses those headers → produces .yaml → es2panda_lib.rb + .erb + cppToCTypes.yaml and ignoredAllowed.yaml generate es2panda_lib.h/.cpp, etc. **Do not edit generated files by hand**; they are overwritten by `ninja gen_api`.

## 3. Directory Layout

```
public/
├── es2panda_lib.*, *.inc.erb, *.idl.erb   # C API and codegen templates
├── es2panda_lib.rb, enums.rb              # Codegen scripts
├── cppToCTypes.yaml, ignoredAllowed.yaml  # Type mapping and ignore list
├── CMakeLists.txt                         # HEADERS_TO_BE_PARSED defined here
└── headers_parser/                        # C++ header parser (Python)
```

## 4. Modifying the C API

| Goal | Action |
|------|--------|
| Add or change exported class/method | Edit the **C++ header** (e.g. under ir/); if the header is new, add it to **HEADERS_TO_BE_PARSED**. Then run `ninja gen_api`. |
| New type on C side | Add a mapping in **cppToCTypes.yaml** per README, then `ninja gen_api`. |
| Exclude a symbol | **ignoredAllowed.yaml**. |
| Change codegen logic | Edit **.erb** / **es2panda_lib.rb** / **cppToCTypes.yaml**, then `ninja gen_api`. |

## 5. Test entry points

Paths in this section are relative to the repository root. Run Ninja commands from the configured static frontend build directory with regression tests enabled.

| Coverage | Source / target |
|---|---|
| Public C API gtests | `ets2panda/test/unit/public/`; includes `es2panda_public_test` and API result storage/lifecycle tests. Run `ninja es2panda_gtests` for the registered frontend gtest group |
| Plugin compile/runtime regressions | `ets2panda/test/unit/plugin/`; run `ninja es2panda-plugin-test` |
| API generation and compilation | Run `ninja gen_api es2panda-public` |

Select or extend actual cases registered in the corresponding `CMakeLists.txt`. Public gtests and plugin regressions are separate coverage; neither replaces the other. Check build guards before selecting tests: unit tests require `PANDA_REGRESSION_TESTS`, and public tests have sanitizer/platform-specific exclusions. Report unavailable tests explicitly.

## 6. Plugin API Guardrails

- Keep Plugin API growth minimal; prefer existing API if it already expresses required behavior.
- Every change in `public/es2panda_lib.h` must be reflected in `public/es2panda_lib.idl.erb` in the same patch.
