# ArkTS1.2 build system test suite

The test suite bases on [Jest](https://jestjs.io/) and aims to test the `build_system`.

---

## Import Jest && Test Suite

### 1. Edit the build_system/package.json file

**Add test scripts**  
Add the following to the `scripts` section in `package.json` (already in the code repository):

```json
"scripts": {
  "build_system_Utest": "jest --testMatch='**/test/ut/**/utils.test.ts' --testPathIgnorePatterns='test/e2e/'",
  "build_system_Etest": "TEST=${test_script_name} jest test/e2e/compile.test.ts",
}
```

Build_system_Utest is for unit tests while build_system_Etest is for E2E tests.
The `TEST` environment variable is used to specify the test script to run for E2E tests.

**Add dependencies**  
Add the following to the `devDependencies` section in `package.json` (already in the code repository):

```json
"devDependencies": {
  "@babel/core": "^7.27.1",
  "@babel/preset-env": "^7.27.2",
  "@babel/preset-typescript": "^7.27.1",
  "@types/jest": "^29.5.14",
  "babel-jest": "^29.7.0",
  "jest": "^29.7.0",
  "ts-node": "^10.9.2"
}
```

**Babel configuration**

Babel allows running tests without compiling test files with tsc first.
Add the following to `package.json` (already in the code repository):

```json
"babel": {
  "presets": [
    ["@babel/preset-env", { "targets": { "node": "current" } }],
    "@babel/preset-typescript"
  ]
}
```

### 2. Jest configuration

Add the following to `build_system/jest.config.js` (already in the code repository):

```js
module.exports = {
  testEnvironment: "node",
  verbose: true,
  collectCoverage: true,
  coverageDirectory: "<rootDir>/dist/coverage",
  setupFilesAfterEnv: [
    "<rootDir>/testHook/jest.memory-usage.js",
    "<rootDir>/testHook/jest.time-usage.js",
    "<rootDir>/testHook/jest.abc-size.js"
  ],
  testMatch: [
    "<rootDir>/test/ut/**/*.test.[jt]s"
  ],
  testPathIgnorePatterns: []
};
```

---

## Running the Test Suite

This test suite is divided into two parts: unit tests and end-to-end (E2E) tests.

### Unit Test Example

Add an `osType.test.ts` file under `build_system/test/ut`:

```typescript
import { isWindows, isLinux, isMac } from '../../src/utils';

describe('osType', () => {
  it('should detect OS type correctly', () => {
    expect(isWindows()).toBe(false);
    expect(isLinux()).toBe(true);
    expect(isMac()).toBe(false);
  });
});
```

**[Optional]** Modify the testMatch parameter of build_system_Utest to specify the test file:

```json
"scripts": {
  "build_system_Utest": "jest --testMatch='**/test/ut/**/osType.test.ts' --testPathIgnorePatterns='test/e2e/'",
}
```

Run from the command line:

```bash
npm run build_system_Utest
```

#### Recommended Practice

For each test file, create a folder with the same name under `ut`.
For example, for `osType.test.ts`, create a `ut/osType` folder, using camelCase naming convention.

#### Existing UT

There are corresponding test files for each file under `src`, placed in folders with the same name as in `src`.
The `mock` folder contains mock files needed for some tests.
By default, all test files in this directory will be run. You can filter them by adjusting the Jest configuration.

```
test/ut
├── base_modeTest
│   └── base_mode.test.ts
├── build_framework_modeTest
│   └── build_framework_mode.test.ts
├── compile_WorkerTest
│   └── compile_worker.test.ts
├── compile_thread_workerTest
│   └── compile_thread_worker.test.ts
├── declgen_workerTest
│   └── declgen_worker.test.ts
├── entryTest
│   └── entry.test.ts
├── fileManagerTest
│   └── filemanager.test.ts
├── generate_arktsconfigTest
│   └── generate_arktsconfig.test.ts
├── loggerTest
│   └── logger.test.ts
├── mock
│   ├── a.ets
│   └── mockData.ts
├── plugins_driverTest
│   └── plugins_driver.test.ts
├── process_build_configTest
│   └── process_build_config.test.ts
├── safeRealpath.test.ts
└── utilsTest
    └── utils.test.ts
```

---

### Before E2E

The configuration steps before E2E testing are as follows:
  - Enter the `build_system` directory

   ```bash
   cd ${absolute_path_to_build_system}
   ```

  - Perform mock_sdk operations to ensure that the build_system itself and hap packages can be compiled locally:
    * You can get the SDK for Linux from the [DailyBuild](https://ci.openharmony.cn/workbench/cicd/dailybuild/dailylist).
    [*Build the SDK from scratch is also an option.*]
    * Download the `ohos-sdk-public_0328` package and extract it, get `ohos-sdk`.
    * Extract `ohos-sdk/linux/ets-linux-x64-6.0.0.36-Canary1.zip`, get `ets` and move `ets/ets1.2` to the `build_system/test/mock_sdk` directory.
    * After that, the mock_sdk directory should contain the folders `api`, `arkts`, `build-tools`, and `kits`.

  - Grant execute permissions to the executables under `mock_sdk/build-tools/ets2panda/bin`:

   ```bash
   chmod +x test/mock_sdk/build-tools/ets2panda/bin/*
   ```

  - Replace all `${absolute_path_to_build_system}` in e2e test code with the actual directory:

   ```bash
   find test -name 'build_config*.json' -exec sed -i 's|${absolute_path_to_build_system}|'"$(pwd)"'|g' {} +
   find test -name 'decl-fileInfo.json' -exec sed -i 's|${absolute_path_to_build_system}|'"$(pwd)"'|g' {} +
   ```

  - Export the `LD_LIBRARY_PATH` environment variable.
   To avoid exporting it every time, you can add it to your environment file.
   Be sure to replace with the actual directory:

   ```bash
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${absolute_path_to_build_system}/test/mock_sdk/build-tools/ets2panda/lib
   ```

  - Install dependencies and compile all files under `src`:

   ```bash
   npm install
   npm run build
   ```
  - **[optional]** Customize the test suite by modifying Jest parameters in `jest.config.js`, such as including or excluding test files:

   ```js
   module.exports = {
     testEnvironment: "node",
     verbose: true,
     collectCoverage: true,
     coverageDirectory: "<rootDir>/coverageReport",
     setupFilesAfterEnv: [
       "<rootDir>/testHook/jest.memory-usage.js"
       // "<rootDir>/testHook/jest.time-usage.js"
     ],
     testMatch: [
       "test/ut/sum.test.ts"
     ],
     testPathIgnorePatterns: [
       "/test/ut/skip/",
       "/test/ut/sometest.test.ts"
     ]
   }
   ```

---

### E2E Test Example

The purpose of end-to-end tests is usually to check whether a project can be compiled, whether the `abc file` is generated correctly, and whether there are errors or exceptions during compilation.
End-to-end tests are run similarly to unit tests, but with different interface files.

Add a `compile.test.ts` file under `build_system/test/e2e` (The latest version is already in the code repository):

```typescript
// this file is too long, so only the first part is shown here

import { execFile } from 'child_process';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';

const execFileAsync = promisify(execFile);

function getAllFilesWithExt(dir: string, exts: string[]): string[] {
  if (!fs.existsSync(dir)) return [];
  let result: string[] = [];
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      result = result.concat(getAllFilesWithExt(fullPath, exts));
    } else if (exts.some(ext => entry.name.endsWith(ext))) {
      result.push(fullPath);
    }
  }
  return result;
}
// more code below...
```

Since the project under test may be very complex, with multiple `build_config.json` files and unknown compilation order, users are required to write scripts to specify the compilation process.
Usage:

#### Single test

Modify the `build_system_Etest` script's TEST parameter to the test project script:

```json
"scripts": {
  "entry1_2_external_har1_2:gen_abc": "npm run build && node ./dist/entry.js ${absolute_path_to_build_system}/test/e2e/entry1_2_external_har1_2/build_config.json",
  "build_system_Etest": "TEST=entry1_2_external_har1_2:gen_abc jest --testMatch '${absolute_path_to_build_system}/test/e2e/*.test.ts'"
}
```

Specify the full script name after TEST=.

Run from the command line:

```bash
npm run build_system_Etest
```

This will execute the test.

To add tests, create a new test file/folder under `build_system/test/e2e`.
The `build_system_Etest` script will automatically include the test.
If you want to run a specific test, you can modify the `testMatch` parameter in `jest.config.js` to include only that test file.

***Among the existing tests, there is a relatively unique one: IncrementDemo:gen_abc, which is related to incremental compilation. Therefore, to run this test, an special script is provided:***
```bash
npm run IncrementCompileTest
```

#### Multiple tests

A `run_all.sh` script is provided in the `test/e2e` directory to run all tests.

```bash
scripts1=(
"demo_entry1.1_har1.1_hsp1.2:gen_abc"
"demo_entry1.1_har1.2_hsp1.1:gen_abc"
"demo_entry1.1_har1.2_hsp1.2:gen_abc"
"demo_entry1.1_hsp1.2:gen_abc"
"demo_entry1.2_har1.1_hsp1.1:gen_abc"
"demo_entry1.2_har1.1_hsp1.2:gen_abc"
"demo_entry1.2_har1.2_hsp1.1:gen_abc"
"demo_entry1.2_har1.2_hsp1.2:gen_abc"
"demo_entry1.2_hsp1.1:gen_abc"
"demo_entry1.2_hsp1.2:gen_abc"
"demo_entry1_2:gen_abc"
"demo_har1_2:gen_decl"
"demo_hsp1_2:gen_decl"
"entry1_1_external_har1_2:gen_abc"
"entry1_1_external_har1_2:gen_decl"
"entry1_1_external_hsp1_2:gen_abc"
"entry1_1_external_hsp1_2:gen_decl"
"entry1_2_external_har1_1:gen_abc"
"entry1_2_external_har1_2:gen_abc"
"entry1_2_external_hsp1_1:gen_abc"
"entry1_2_external_hsp1_2:gen_abc"
"entry1_2_external_hsp1_2:gen_decl"
)

scripts2=(
  "IncrementCompileTest1"
  "IncrementCompileTest2"
)

passed=()
failed=()

for script in "${scripts1[@]}"; do
  echo "Running E2E test: $script"
  TEST=$script npx jest --testMatch='**/test/e2e/*.test.ts' --testPathIgnorePatterns='test/ut/'
  #npm run "$script"
  if [ $? -eq 0 ]; then
    passed+=("$script")
  else
    failed+=("$script")
  fi
done

for script in "${scripts2[@]}"; do
  echo "Running IncrementalCompile test: $script"
  npm run "$script"
  if [ $? -eq 0 ]; then
    passed+=("$script")
  else
    failed+=("$script")
  fi
done

echo
echo "================== E2E Test Summary =================="
total=$(( ${#scripts1[@]} + ${#scripts2[@]} ))
echo "Total: $total"
echo "Passed: ${#passed[@]}"
echo "Failed: ${#failed[@]}"
if [ ${#passed[@]} -gt 0 ]; then
  echo "Passed tests:"
  for s in "${passed[@]}"; do
    echo "  $s"
  done
fi
if [ ${#failed[@]} -gt 0 ]; then
  echo "Failed tests:"
  for s in "${failed[@]}"; do
    echo "  $s"
  done
fi
echo "======================================================"
```

Make the script executable:

```bash
chmod +x test/e2e/run_all.sh
```

Run the script from the command line:

```bash
./test/e2e/run_all.sh
```

It will run all the test scripts in the scripts array and output the test results.
Since the output may be quite complex, it is recommended to redirect the output to a file or filter the output results.

---

## How to Write Tests

The test suite is based on Jest, using Jest's assertions and matcher mechanisms.

### Global Configuration

- Group tests: `describe(name, fn)`, multiple tests are included in fn as a group.
- Single test: `it(name, fn)`, fn contains the test implementation. All `it` can be replaced with the alias `test`.
- Failing test: `it.failing(name, fn, timeout)`, opposite of `it`: if fn succeeds, the test fails; if fn fails, the test passes.
- Todo: `it.todo(name)`, indicates the test is not yet written.
- Run before/after all tests: `afterAll(fn, timeout)`, `beforeAll(fn, timeout)`, fn is the implementation, optional timeout.
- Run before/after each test: `afterEach(fn, timeout)`, `beforeEach(fn, timeout)`, fn is the implementation, optional timeout.

### Assertions

`expect(expr)` creates an assertion object, e.g. `expect(isLinux())`

### Matchers

Matchers are methods provided by assertion objects and can be chained (method chaining). Jest provides many matchers; here are some common ones:

- `toBe(value)`: Checks if values are equal
- `toHaveBeenCalled()`: Checks if a function was called, often used with mock functions
- `toHaveReturned()`: Checks if a function returned normally, often used with mock functions
- `toHaveLength(number)`: Checks array length
- `toBeInstanceOf(Class)`: Checks if an object is an instance of a class (like `instanceof`)
- `toContain(item)`: Checks for containment
- etc. See [official documentation](https://jestjs.io/docs/expect) for more matchers.

### Mock Functions

Example: Add an `osType.test.ts` file under `build_system/test/ut/osType`:

```typescript
import * as utils from '../../src/utils';

describe('osTypeCheck', () => {
  it('should detect OS type correctly', () => {
    expect(utils.isWindows()).toBe(false);
    expect(utils.isLinux()).toBe(true);
    expect(utils.isMac()).toBe(false);
  });
  it('mocked isWindows always return true', () => {
    const spy = jest.spyOn(utils, 'isWindows').mockImplementation(() => true);
    expect(utils.isWindows()).toBe(true);
    expect(utils.isLinux()).toBe(true);
    expect(utils.isMac()).toBe(false);
    spy.mockRestore();
  });
});
```

Note: ES6 imports are read-only and cannot be modified, so you must use `import * as utils` to import the module.
Whether to mock the whole module or just a function depends on the specific situation.
Mock functions provide many interfaces for assertions and checks. See [official documentation](https://jestjs.io/docs/mock-function-api).

### Asynchronous Code

Functions that return a Promise can be tested directly. If they return `Promise.resolve()`, the test passes; if they return `Promise.reject()`, the test fails.

### Results

Test results are printed to the command line by default.

Example (may change in the future):

```
> ArkTS2.0_build_system@1.0.0 build_system_Utest
> jest --testMatch='**/test/ut/**/osType.test.ts' --testPathIgnorePatterns='test/e2e/'

  console.log
    [Jest][osType should detect OS type correctly used 0.28 MB]

      at Object.<anonymous> (test/testHook/jest.memory-usage.js:27:11)

  console.log
    [Jest][osType should detect OS type correctly spent 6 ms]

      at Object.<anonymous> (test/testHook/jest.time-usage.js:26:11)

  console.warn
    [Jest][No .abc files found in ${absolute_path_to_build_system}/dist/cache]

      42 |   const abcFiles = getAllAbcFiles(cacheDir);
      43 |   if (abcFiles.length === 0) {
    > 44 |     console.warn(`[Jest][No .abc files found in ${cacheDir}]`);
         |             ^
      45 |     return;
      46 |   }
      47 |   abcFiles.forEach(file => {

      at Object.<anonymous> (test/testHook/jest.abc-size.js:44:13)

 PASS  test/ut/osType.test.ts
  osType
    ✓ should detect OS type correctly (8 ms)

---------------|---------|----------|---------|---------|---------------------------------------------------------
File           | % Stmts | % Branch | % Funcs | % Lines | Uncovered Line #s
---------------|---------|----------|---------|---------|---------------------------------------------------------
All files      |   41.05 |    13.95 |   14.63 |   41.26 |
 error.ts |     100 |      100 |     100 |     100 |
 logger.ts     |    6.75 |        0 |       0 |    6.84 | 25-103,125-126,147-179,189-218
 pre_define.ts |     100 |      100 |     100 |     100 |
 utils.ts      |   36.76 |        0 |   23.07 |   36.76 | 51-53,57-60,64-71,77-78,82,87-95,99-109,114-120,128-143
---------------|---------|----------|---------|---------|---------------------------------------------------------
Test Suites: 1 passed, 1 total
Tests:       1 passed, 1 total
Snapshots:   0 total
Time:        0.903 s
Ran all test suites.
```

The first three console.log/warn statements at the beginning are outputs for testing hooks, showing memory usage, test duration, and the size of abc files, respectively.
The formats are `[Jest][${test_name} used ${memory} MB]`,
`[Jest][${test_name} spent ${time} ms]`, and
`[Jest][No .abc files found in ${cacheDir}]`.
The table is a summary of coverage, showing statement, branch, function, and line coverage for each file.
Test Suites is the number of test files executed.
Tests is the number of test/it blocks in the test files.
Since there may be command line output during compilation, it is recommended to redirect the output stream, for example, by outputting compilation information to a temporary file.

---

## New Folder Structure

```
build_system
├── dist                    # Build output directory
│   └── coverage            # Coverage output directory
├── src                     # Source code directory
├── test                    # Test directory
│   ├── ut                  # Unit tests
│   │   ├── mockOsType               # Mock function tests
│   │   │   └── mockOsType.test.ts   # Mock OS type test
│   │   ├── mockConsoleLog.test.ts   # Mock console.log test
│   │   └── ...                      # More unit tests
│   └── e2e                 # End-to-end tests
│       ├── compile.test.ts          # Compile test
│       ├── checkHash.test.ts        # Hash check test
│       ├── abcGenerationTest        # Example hap project
│       ├── ...                      # More end-to-end tests
│       └── testHook                 # Test hook files
│           ├── jest.memory-usage.js # Memory usage monitor
│           ├── jest.time-usage.js   # Test time monitor
│           └── jest.abc-size.js     # abc file size monitor
├── package.json            # Project configuration file
└── jest.config.js          # Jest configuration
```

---

## Coverage Report

A summary of coverage information is printed to the command line.
A detailed coverage report is output to the `dist/coverage` directory by default.
You can change the output path by modifying the `coverageDirectory` field in `jest.config.js`.
Jest uses `Istanbul` to generate coverage reports.
The coverage report folder includes the following:

```
dist/coverage
├── clover.xml                # Clover format coverage report
├── coverage-final.json       # JSON format coverage report
├── lcov-report               # Detailed HTML coverage report directory
│   ├── base.css
│   ├── block-navigation.js
│   ├── favicon.png
│   ├── index.html            # Coverage report, open in browser for details
│   ├── prettify.css
│   ├── prettify.js
│   ├── sort-arrow-sprite.png
│   └── sorter.js
└── lcov.info                 # Standard LCOV format coverage data file
```
