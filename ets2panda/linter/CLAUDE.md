# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this directory.

## Project Overview

This is `@panda/tslinter`, a ArkTS-dynamic linter designed specifically for migrating ArkTS-dynamic code to ArkTS-static. The linter identifies deprecated ArkTS-dynamic constructs and provides autofix capabilities to help migrate code to ArkTS-static compatibility.
ArkTS-dynamic is a language developed based on TypeScript, with most features aligning closely with TypeScript. However, it introduces certain static constraints to the TypeScript syntax. For specific differences, you may refer to: https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/quick-start/typescript-to-arkts-migration-guide.md. When designing test code and functional code, please consult this website for guidance.
**Key Purpose:** Find ArkTS-dynamic patterns that are incompatible with ArkTS-static and provide automated fixes for migration.

## Project documents

syntax-rules: https://gitcode.com/openharmony/docs/blob/OpenHarmony_feature_20250702/zh-cn/application-dev/quick-start/arkts-dyn-to-sta-syntax-rules.md
concurrency-rules: https://gitcode.com/openharmony/docs/blob/OpenHarmony_feature_20250702/zh-cn/application-dev/quick-start/arkts-dyn-to-sta-concurrency-rules.md
builtin-rules: https://gitcode.com/openharmony/docs/blob/OpenHarmony_feature_20250702/zh-cn/application-dev/quick-start/arkts-dyn-to-sta-builtin-rules.md
interop-rules: https://gitcode.com/openharmony/docs/blob/OpenHarmony_feature_20250702/zh-cn/application-dev/quick-start/arkts-dyn-to-sta-interop-rules.md
sdk-rules: https://gitcode.com/openharmony/docs/blob/OpenHarmony_feature_20250702/zh-cn/application-dev/quick-start/arkts-dyn-to-sta-sdk-rules.md

## Development Process

1. The task will provide a test sample file test.ets and its corresponding expected JSON file (line and column numbers may not be precise, but the count of reported errors is definitive). You must ensure this test case passes—this is the non-negotiable baseline.

2. Based on these test cases, you need to analyze the issues and code logic, and propose a solution. If you identify scenarios not covered by the provided test cases, you must supplement them and develop the corresponding detection logic.

3. Testing and verification must meet the following requirements:
   a. Ensure all test.ets test cases pass.
   b. If test cases from other files are affected (apart from test.ets), while ensuring condition a is met, analyze whether the impact on other test cases aligns with the expectations of the current task, rather than being due to newly introduced bugs.

4. If step 3 is not satisfied, repeat the modifications until step 3 is fully completed.

5. After testing and fixing all test cases, synchronize and archive the test.ets test cases into the corresponding rule's test files like xxx-positive.ets/xxx-negative.ets. At this point, the test.ets file can be deleted.

## Build Commands

### First-time Setup

```bash
npm install
npm run install-ohos-typescript
```

### Building

```bash
npm run build           # Full clean build: compile + webpack + pack
```

### Code Quality

```bash
npm run fix             # Run prettier and eslint fixes
npm run eslint-check    # Check code style with eslint
npm run prettier-fix    # Format code with prettier
```

### Distribution

```bash
npm run pack:linter     # Create distributable .tgz package in bundle/
```

## Testing

This command will use Linter to scan this file and generate a series of JSON files to compare with the expected JSON files. If they are consistent, pass the test
The a-positive.ets file will include a positive example: an example that conforms to ArkTS-static syntax 'a', and will not report an error for rule 'a'
The a-negative.ets file will contain counterexamples: examples that do not comply with ArkTS-static syntax a will result in an error for rule a.
e.g
a-positive.ets.args.json is the configuration parameter for testing
a-positive.ets.arkts2.json is the expected scan result under --arkts-2
a-positive.ets.json is the expected scan result without --arkts-2 enabled
a-positive.ets.autofix.json is the expected result of automatic repair
When performing code modifications or requirement design, it is essential to strictly adhere to the error points specified in the task description. You must not alter the generated expected files in an unintended manner simply to pass tests and achieve acceptance.

### Running Tests

```bash
npm test                # Run all tests in parallel
npm run test_all        # Run all test suites sequentially
npm run testrunner -- [args]  # Custom test runner with options
npm run testrunner -- -d test/main -p xxx.ets       #Run test on xxx.ets alone
```

### Test Categories

The project has specialized test suites for different rule categories (when executing the test suites, running them all together can lead to long wait times, so it is necessary to test each category one by one):

- `npm run test_main` - Core functionality tests
- `npm run test_rules` - Rule-specific tests
- `npm run test_regression` - Regression test cases
- `npm run test_sdk` - SDK API tests
- `npm run test_interop` - JavaScript/ETS interoperability tests
- `npm run test_concurrent` - Concurrency/parallelism tests
- `npm run test_builtin` - Built-in function tests
- `npm run test_deprecatedapi` - Deprecated API tests
- `npm run test_taskpool` - Task pool tests
- `npm run test_sdkcommonapi` - Common API tests
- `npm run test_overload` - ArkTS-dynamic overload tests

### Test Runner Usage

```bash
# Run tests in specific directory with pattern
npm run testrunner -- -d test/main -p {array,object}*

# Run with SDK mode
npm run testrunner -- -d test/rules --sdk

# Run multiple test directories
npm run testrunner -- -d test/main,test/rules
```

### Coverage

```bash
npm run coverage        # Generate test coverage reports
```

### Updating Test Results

```bash
npm run update-tests    # Update test result files (use carefully)
```

The use of this command requires confirming that the corresponding scenario necessitates modifications and aligns with the rule design. Only then should the npm run update-tests command be executed to generate new expected scanning results.

## Architecture

### Core Components

**CLI Layer** (`src/cli/`)

- `LinterCLI.ts` - Main CLI entry point and orchestration
- `CommandLineParser.ts` - Command-line argument parsing using Commander.js
- `main.ts` - Application bootstrap

**Linter Engine** (`src/lib/`)

- `TypeScriptLinter.ts` - Main linter implementation
- `BaseTypeScriptLinter.ts` - Base linter class with core logic
- `LinterRunner.ts` - Orchestrates the linting process across files
- `Problems.ts` - Problem/fault collection and management
- `ProblemInfo.ts` - Individual problem representation
- `Autofixer.ts` - Automatic code fix capabilities

**TypeScript Integration** (`src/lib/ts-compiler/`, `src/lib/ts-diagnostics/`)

- `Compiler.ts` - TypeScript compiler wrapper
- `TypeScriptDiagnosticsExtractor.ts` - Extract TSC compilation errors

**Rule System** (`src/rules/`)
Rules are organized by categories in `rule-config.json`:

- **ArkTS** (59 rules) - Core ETS language restrictions
- **Interop** (32 rules) - JavaScript/ETS interoperability
- **ArkUI** (24 rules) - UI framework restrictions
- **Builtin** (4 rules) - Built-in function restrictions
- **Concurrent** (10 rules) - Concurrency patterns
- **SDK** (11 rules) - SDK API usage

**SDK Support** (`src/sdk/`)

- `linter_1_1/` - SDK-specific linting implementation

### Build Pipeline

1. **TypeScript Compilation** (`tsc`) - Compiles `src/` to `build/`
2. **Webpack Bundling** - Bundles into `dist/tslinter.js` (Node.js target)
3. **Packaging** - Creates distributable in `bundle/` directory

### Entry Points

- **Built executable**: `dist/tslinter.js`
- **Binary symlink**: `bin/tslinter.js`
- **Package**: `bundle/panda-tslinter-*.tgz`

## Running the Linter

```bash
node dist/tslinter.js [options] [input files]

# Or use wrapper scripts:
tslinter.sh [options] [input files]      # Linux/Mac
tslinter.bat [options] [input files]     # Windows
```

### Key Options

- `-f, --project-folder <path>` - Folder to lint recursively (can be repeated)
- `-p, --project <path>` - Path to tsconfig.json
- `-E, --TSC_Errors` - Show TypeScript compilation errors
- `--deveco-plugin-mode` - IDE integration mode (do NOT use from command line)
- `@response-file.txt` - Response file for large input lists (must be last argument)

### Response Files

For large input lists to avoid command-line buffer overflow:

```
tslinter.sh @response-file.txt
```

One file path per line in the response file.

## Configuration Files

- `tsconfig.json` - TypeScript compilation settings
- `tsconfig-sdk.json` - SDK-specific TypeScript configuration
- `rule-config.json` - Rule definitions by category
- `webpack.config.js` - Bundling configuration
- `BUILD.gn` - OpenHarmony/GN build system integration
- `.prettierrc.json` - Code formatting rules
- `eslint.config.mjs` - ESLint configuration

## Test Structure

Tests use `.ets` extension for ETS-specific test files. Each test directory contains:

- Source files (`.ets`, `.ts`)
- Expected result files (JSON)
- Autofix validation data (JSON)

Key test directories:

- `test/main/` - Core functionality
- `test/rules/` - Individual rule validation
- `test/regression/` - Known issue regression tests
- `test/extended_features/` - Advanced feature tests
- `test/interop/` - Interoperability tests
- `test/sdkwhite/` - SDK API whitelist tests
- `test/concurrent/` - Concurrency tests
- `test/builtin/` - Built-in function tests

## Dependencies

**Runtime:**

- `commander` - CLI parsing
- `log4js` - Logging
- `fs-extra` - File system operations
- `homecheck` - Migration checking (local package)
- `yup` - Schema validation
- `readline-sync` - Interactive input

**Development:**

- `typescript` - TypeScript compiler
- `webpack` - Application bundling
- `eslint`, `prettier` - Code quality
- `nyc` - Code coverage
- `glob` - File pattern matching
- `rimraf` - Cross-platform delete

## Development Notes

- Always install dependencies before linting a project
- The linter processes files from tsconfig.json unless specific files are passed
- Files are processed from the intersection of command-line files and tsconfig.json includes
- IDE integration uses `--deveco-plugin-mode` flag
- Incremental linting is supported for performance
- Statistics generation available via `stats_calculator/` for DevEco plugin

## Linter Testing Workflow

### Understanding Test File Naming Convention

**Important**: The naming convention:

- **Positive tests (`xxx-positive.ets`)**: Code that **conforms to ArkTS syntax** (should NOT trigger errors)
  - Purpose: Verify valid code doesn't get falsely flagged
  - Example: `Column({space: 5} as ColumnOptions)` with explicit type assertion - correct usage

- **Negative tests (`xxx-negative.ets`)**: Code that **violates ArkTS syntax** (SHOULD trigger errors)
  - Purpose: Verify the rule correctly detects violations
  - Example: `Column({space: 5})` without type assertion - ambiguous type that needs fixing

### Test Result Files

For each test file, there are corresponding expected result files:

- `xxx-positive.ets.json` - Expected errors in default mode
- `xxx-positive.ets.arkts2.json` - Expected errors in ArkTS-2 mode
- `xxx-positive.ets.args.json` - Test configuration (modes to enable)
- `xxx-positive.ets.autofix.json` - Expected autofix results

### How Testing Works

The test runner:

1. Scans the `.ets` file using the linter
2. Generates actual result JSON files
3. Compares with expected result JSON files
4. Test passes if they match

### Creating New Tests

When adding a new rule test:

1. **Create the test files**:

   ```bash
   test/main/myrule-positive.ets      # Code that violates the rule
   test/main/myrule-negative.ets      # Code that follows the rule
   ```

2. **Generate expected results**:

   ```bash
   npm run testrunner -- -d test/main -p myrule
   # This will scan the files and generate JSON results
   ```

3. **Review and update expectations**:
   ```bash
   # If the results are correct, update the expected files:
   npm run update-tests
   ```

### Example: SDK Union Type Ambiguity Test

**Positive test** (correct usage, no error):

```ArkTS-dynamic
// sdk-union-type-ambiguity-positive.ets
Column({ space: 20 } as ColumnOptions) { }  // OK: explicit type assertion
Row({ space: 10 } as RowOptions) { }        // OK: explicit type assertion
```

**Negative test** (violation, should error):

```ArkTS-dynamic
// sdk-union-type-ambiguity-negative.ets
Column({ space: 20 }) { }  // ERROR: ambiguous union type
Row({ space: 10 }) { }     // ERROR: ambiguous union type
```

### Debugging Test Failures

If a test fails:

1. Check the actual vs expected JSON files
2. Use `npm run testrunner -- -d test/main -p myrule` to re-run
3. Add `-v` flag for verbose output
4. Verify line numbers match the source file

### Testing Different Scenarios

For comprehensive rule testing, create variations:

- `myrule-build-component.ets` - Inside @Component struct
- `myrule-build-in-function.ets` - Inside function
- `myrule-build-simple.ets` - Simple standalone case

This ensures the rule works in all contexts.
