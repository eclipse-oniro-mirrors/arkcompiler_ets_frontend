# CLAUDE.md

**Name**: easytrans(tslinter)
**Purpose**: a static analysis tool specifically designed for migrating ArkTS-dynamic code to ArkTS-static. The tool identifies deprecated ArkTS-dynamic constructs and provides autofix capabilities to assist in migrating code to be compatible with ArkTS-static.ArkTS-dynamic is a language developed based on TypeScript, with most features aligning closely with TypeScript. However, it introduces certain static constraints to the TypeScript syntax.
**Primary Language** TypeScript

## Directory Structure

```
linter/
├── arkanalyzer/           # ArkTS analyzer component
│   ├── config/
│   ├── src/
│   ├── script/
│   └── rules/             # Rule documentation (en/cn)
├── bin/                   # Executable symlinks
├── build/                 # Compiled TypeScript output
├── bundle/                # Distributable packages (.tgz)
├── dist/                  # Bundled application (webpack output)
├── docs/                  # Documentation
│   └── rules/             # Rule documentation (en/cn)
├── homecheck/             # Runtime Error/Consistency Checker
│   ├── config/
│   ├── lib/
│   ├── resources/
│   ├── src/
│   │   ├── checker/
│   │   │   └── migration/  # Concrete Runtime Rule Implementation
│   │   ├── codeFix/        # Auto-fix implementations
│   │   ├── matcher/        # Pattern matching utilities
│   │   ├── model/          # Data models
│   │   ├── tools/          # Checker tools
│   │   └── utils/          # Utility functions
│   └── test/               # homecheck tests
├── scripts/               # Build and utility scripts
│   ├── bundle-ts-lib-declarations.mjs
│   ├── install-ohos-typescript-and-homecheck.mjs # Local Build Packaging Script
│   ├── testRunner/         # Test Suite
│   └── update-test-results.mjs     # Test Result Updater
├── src/                   # Main source code
│   ├── cli/               # CLI layer
│   │   ├── LinterCLI.ts
│   │   ├── CommandLineParser.ts
│   │   └── main.ts
│   ├── lib/               # Linter engine
│   │   ├── autofixes/      # Auto-fix implementations
│   │   ├── data/           # Static data
│   │   ├── progress/       # Progress reporting
│   │   ├── statistics/     # Statistics collection
│   │   ├── ts-compiler/    # TypeScript compiler wrapper
│   │   ├── ts-diagnostics/ # TSC diagnostics extraction
│   │   └── utils/          # Utility functions
│   ├── sdk/               # SDK support
│   │   └── linter_1_1/     # SDK-specific linting
│   └── testRunner/        # Test runner implementation
├── stats_calculator/      # Statistics for DevEco plugin
├── test/                  # Test suites
│   ├── builtin/           # Built-in function tests
│   ├── concurrent/        # Concurrency tests
│   ├── deprecatedapi/     # Deprecated API tests
│   ├── extended_features/ # Advanced feature tests
│   ├── interop/           # Interoperability tests
│   ├── main/              # Core functionality tests
│   ├── ohmurl/            # OHM URL tests
│   ├── regression/        # Regression tests
│   ├── rules/             # Rule-specific tests
│   ├── sdkcommonapi/      # Common API tests
│   ├── sdkwhite/          # SDK whitelist tests
│   ├── taskpool/          # Task pool tests
│   ├── ts_import_ets/     # TS import ETS tests
│   └── ts_overload/       # Overload tests
├── cookbook_convertor/    # Cookbook conversion tool
├── BUILD.gn               # OpenHarmony/GN build config
├── CLAUDE.md              # This file - Claude Code guidance
├── eslint.config.mjs      # ESLint configuration
├── package.json           # NPM package definition
├── rule-config.json       # Rule definitions by category
├── tsconfig.json          # TypeScript compilation config
├── tsconfig-sdk.json      # SDK TypeScript config
├── tslinter.sh            # Linux/Mac wrapper script
├── tslinter.bat           # Windows wrapper script
└── webpack.config.js      # Webpack bundling config
```

## Build Commands

### Building

```bash
npm install
npm run install-ohos-typescript   # you need to execute this command to update the source code dependency result of homecheck in node_modules.
npm run build           #  build: after npm run install-ohos-typescript
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

## Test suite

This command will use tslinter to scan the target file and generate a series of JSON files to compare with the expected JSON files. If they are consistent, pass the test.
The a-positive.ets file will include a positive example: an example that conforms to ArkTS-static syntax 'a', and will not report an error for rule 'a'.
The a-negative.ets file will contain counterexamples: examples that do not comply with ArkTS-static syntax a will result in an error for rule a.
For the expected JSON files in testing, an example is as follows:
-- a-positive.ets.args.json is the configuration parameter for testing
-- a-positive.ets.arkts2.json is the expected scan result under --arkts-2
-- a-positive.ets.json is the expected scan result without --arkts-2 enabled
-- a-positive.ets.autofix.json is the expected result of automatic repair
When performing issue fixes or requirement design, it is essential to strictly adhere to the error points specified in the task description. You must not alter the expected JSON files of acceptance test cases in an unintended manner simply to pass tests and achieve acceptance.
After each final modification, you need to run 'npm test' to pass all test suites.

### Running Tests

```bash
npm test                # Run all tests in parallel
npm run test_all        # Run all test suites sequentially
npm run testrunner -- [args]  # Custom test runner with options
npm run testrunner -- -d test/main -p xxx.ets       #Run test on xxx.ets alone
```

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

This command allows modification of expected JSON results based on discrepancies between actual scanned JSON results and expected JSON results. However, note that this usage requires user confirmation and must be employed with caution.

## Architecture

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
e.g

`node dist/tslinter.js --ide-interactive --arkts-2 --autofix --homecheck --migrate --sdk-external-api-path /home/xxx/sdk/default/hms/ets/dynamic --sdk-default-api-path /home/xxx/sdk/default/openharmony/ets/dynamic /home/xxx/test.ets` # After building easytrans, you can directly use its scanning and fixing capabilities by executing this command.

### Key Options


`--ide-interactive` # Generate JSON files to support IDE parsing.
`--arkts-2` # Enable ArkTS-Static rules.
`--autofix` # Provide automatic fix suggestions and output them to JSON files.
`--check-ts-as-source` # Support scanning mixed projects containing both .ts and .ets files.
`--migrate` # Enable migration functionality. When enabled, original code will be modified.
`--migration-report` # Generate migration reports.
`--sdk-default-api-path` # Path to the SDK, ending with openharmony/ets.
`--sdk-external-api-path` # Path to the SDK, ending with default/hms/ets. If an incorrect path is provided, this parameter will not take effect and no error will be reported.
`--arkts-whole-project-path` # Full project path.
`--project-folder` #　Path to the folder to be scanned. If scanning a single file, this parameter is not required—simply append the single file path at the end of the command line.
`--autofix-check` ＃ Used in combination with --migrate. After scanning, prompt the user to confirm whether to proceed with the fix.
`--homecheck` # Add runtime error checks.

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

### Bug Fix Workflow
1. The task will provide a test sample file test.ets and its corresponding expected JSON file (line and column numbers may not be precise, but the count of reported errors is definitive). You must ensure this test case passes—this is the non-negotiable baseline.

2. Based on these test cases, you need to analyze the issues and code logic, and propose a solution. If you identify scenarios not covered by the provided test cases, you must supplement them and develop the corresponding detection logic.

3. Testing and verification must meet the following requirements:
   a. Ensure all test.ets test cases pass().
   b. If test cases from other files are affected (apart from test.ets), while ensuring condition a is met, analyze whether the impact on other test cases aligns with the expectations of the current task, rather than being due to newly introduced bugs.
   c. Also, ensure that the functional tests for the built migration toolkit are running normally, referring to the "Running the Linter" section.

4. If step 3 is not satisfied, repeat the modifications until step 3 is fully completed.

5. After testing and fixing all test cases, synchronize and archive the test.ets test cases into the corresponding rule's test files like xxx-positive.ets/xxx-negative.ets. At this point, the test.ets file can be deleted.

### Precautions During Development
1. When you modify the code in homecheck, you need to execute the "npm run install-ohos-typescript" command to reinstall the homecheck on which tslinter source code depends.
2. Always remember to execute npm run install-ohos-typescript before running npm run build every time! This is crucial.