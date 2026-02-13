# declgen_ets2ts Module

**Name**: declgen_ets2ts
**Purpose**: Generates TypeScript declaration files (.d.ets) and glue code (.ets) from ArkTS/ETS source, enabling TypeScript ecosystem interoperability. Also provides isolated validation mode for high-performance, non-global type checking.
**Primary Language**: C++

## Directory Structure

```
declgen_ets2ts/
├── declgenEts2Ts.h               # Core generation engine (TSDeclGen class)
├── declgenEts2Ts.cpp             # Implementation
├── isolatedDeclgenChecker.h        # Isolated mode validator header
├── isolatedDeclgenChecker.cpp      # Isolated mode validation implementation
├── main.cpp                       # CLI entry point
├── CMakeLists.txt                 # CMake build configuration
├── BUILD.gn                       # GN build configuration
├── declgen_ets2ts_error.yaml      # Error definitions
├── declgen_ets2ts_warning.yaml    # Warning definitions
└── isolated_declgen.yaml           # Isolated mode config
```

## Architecture

### Generation Pipeline

The declaration generator operates on a Program AST after it reaches the CHECKED state:

1. **Global Descriptor Initialization** - Initialize ETSGLOBAL for runtime interoperability
2. **Dependency Collection** - Traverse AST to collect all type dependencies
3. **Glue Code Import Collection** - Collect runtime imports needed for glue code
4. **Declaration Generation** - Generate class, interface, and type alias declarations
5. **Export Generation** - Generate export statements
6. **Module Initialization** - Generate init module calls for glue code
7. **Import Generation** - Generate import statements (last for proper ordering)

### Key Components

**TSDeclGen Class**
- Main engine for generating TypeScript declarations and glue code
- Handles AST traversal and TypeScript syntax emission
- Maintains dependency tracking (dependencySet_, importSet_, exportSet_)
- Supports UI and local annotations for ArkTS components

**IsolatedDeclgenChecker Class**
- Validates code is "declaration-ready" without full global symbol table
- Enables high-performance isolated validation mode
- Enforces explicit type annotations for properties, parameters, and functions

**CLI Entry Point**
- Command-line tool for standalone declaration generation
- Integrates with es2panda library for parsing and checking

## Running the Tool

```bash
declgen_ets2ts [OPTIONS] [input]

Options:
  --export-all              Treat all top-level statements as exported
  --output-dets=[FILE]      Path to output .d.ets declaration file
  --output-ets=[FILE]       Path to output .ets glue code file
  --help                    Print usage information
```

## Dependencies

**Runtime:**
- es2panda-public - Public API
- es2panda-lib - Core compiler library
- arkassembler - Bytecode assembler
- arkbytecodeopt - Bytecode optimizer

**Used by**: es2panda compiler (declaration generation phase)

**Depends on**:
- checker (ETSChecker) - Type checking
- parser (Program AST) - AST structure
- util (diagnostics, options) - Utilities
- ir - Intermediate representation

## Development Notes

### Isolated Mode Validation

Isolated mode validates that code is ready for declaration generation without requiring a full global symbol table. This enables:
- High-performance validation for large codebases
- Incremental checking without full recompilation
- Early error detection before full type checking

**Validation Rules:**
- Public properties must have explicit type annotations
- Function parameters must have explicit type annotations
- Functions must have explicit return type annotations (inference from return statements supported)
- Only const arrays (literals) can be inferred
- Default exports cannot be inferred, must have explicit types

### Error Handling

Errors and warnings are defined in YAML files:
- `declgen_ets2ts_error.yaml` - Declaration generation errors
- `declgen_ets2ts_warning.yaml` - Declaration generation warnings
- `isolated_declgen.yaml` - Isolated mode validation errors
