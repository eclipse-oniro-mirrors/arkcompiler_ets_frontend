# Public Component

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Public |
| **Purpose** | Exposes ets2panda’s C++ API as a stable C interface (es2panda_lib) for plugins, SDKs, and external tools. |
| **Primary Language** | C++; YAML/Ruby/ERB (codegen); Python (headers_parser) |

## C API Source and Generation

- **Source**: The exported AstNode and related APIs **come from the AST node classes and their methods in `ir/`** (and from other exported headers for checker, parser, varbinder). Headers to export are listed in **HEADERS_TO_BE_PARSED** in **CMakeLists.txt**; their public classes and methods are parsed and then emitted as the C API.
- **Pipeline**: headers_parser parses those headers → produces .yaml → es2panda_lib.rb + .erb + cppToCTypes.yaml and ignoredAllowed.yaml generate es2panda_lib.h/.cpp, etc. **Do not edit generated files by hand**; they are overwritten by `ninja gen_api`.

## Directory Layout

```
public/
├── es2panda_lib.*, *.inc.erb, *.idl.erb   # C API and codegen templates
├── es2panda_lib.rb, enums.rb              # Codegen scripts
├── cppToCTypes.yaml, ignoredAllowed.yaml  # Type mapping and ignore list
├── CMakeLists.txt                         # HEADERS_TO_BE_PARSED defined here
└── headers_parser/                        # C++ header parser (Python)
```

## Modifying the C API

| Goal | Action |
|------|--------|
| Add or change exported class/method | Edit the **C++ header** (e.g. under ir/); if the header is new, add it to **HEADERS_TO_BE_PARSED**. Then run `ninja gen_api`. |
| New type on C side | Add a mapping in **cppToCTypes.yaml** per README, then `ninja gen_api`. |
| Exclude a symbol | **ignoredAllowed.yaml**. |
| Change codegen logic | Edit **.erb** / **es2panda_lib.rb** / **cppToCTypes.yaml**, then `ninja gen_api`. |

Tests: `test/unit/public` (e2p_test_plugin*); run `ninja es2panda-plugin-test`.
