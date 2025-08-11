# ArkTS1.2 build system test suite

本测试套件基于 [Jest](https://jestjs.io/) 编写，用于测试 build_system 的功能。
自动化测试套件可通过命令行调用，展示测试结果（包括通过/失败的测试数量、编译时间、字节码大小、峰值内存，增量编译识别），并显示失败测试的详细信息。

---

## Jest 引入

### 1. 修改 build_system/package.json

**添加测试脚本**
在 `package.json` 的 `scripts` 项内添加：

```json
"scripts": {
  "build_system_Utest": "jest", // 单元测试，默认运行所有 test/ut 下的测试，可通过命令行传递测试文件位置运行指定测试，可修改 jest 配置灵活调整测试文件集
  "build_system_Etest": "TEST=test/e2e/demo_hap jest test/e2e/compile.test.ts", // 端到端测试，针对编译项目过程进行测试，修改 TEST 参数指定被编译项目路径
}
```

**添加测试依赖库**
在 `package.json` 的 `devDependencies` 项内添加：

```json
"devDependencies": {
  "@babel/core": "^7.27.1", // 用于翻译TS代码，UT测试时使用babel可以避免tsc编译全部代码
  "@babel/preset-env": "^7.27.2",
  "@babel/preset-typescript": "^7.27.1", // 二者一起使babel能够识别和转译TS代码
  "@types/jest": "^29.5.14", // Jest的类型定义文件，使typescript能识别jest的测试接口
  "babel-jest": "^29.7.0", // 用于转译ts代码，不需编译测试ts文件
  "jest": "^29.7.0", // 测试框架
  "ts-node": "^10.9.2" // 用于在Node.js中运行TypeScript代码
}
```

**babel的配置参数**
babel使得运行测试前不需要tsc编译测试文件。
在 `package.json` 中添加：

```json
"babel": {
  "presets": [
    ["@babel/preset-env", { "targets": { "node": "current" } }],
    "@babel/preset-typescript"
  ]
}
```

### 2. jest 的配置参数

在 `build_system/jest.config.js` 中添加：

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

## 运行测试套

测试套件分为单元测试（UT）和端到端测试（E2E）。

### UT 测试示例

在 `build_system/test/ut` 下添加 `osType.test.ts` 文件：

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

**[可选]**修改 build_system_Utest的testMatch参数为测试文件:

```json
"scripts": {
  "build_system_Utest": "jest --testMatch='**/test/ut/**/osType.test.ts' --testPathIgnorePatterns='test/e2e/'",
}
```

命令行运行：

```bash
npm run build_system_Utest
```

#### 推荐的写法

每一个测试文件在ut下新建一个同名文件夹，例如 `osType.test.ts` 在 `ut/osType` 下新建一个 `mockConsoleLog.test.ts` 文件，采用小驼峰方式命名。

#### 目前存在的UT
针对src下的文件有对应的同名Test测试，放在与src同名的文件夹内。
mock夹是一部分测试需要用到的mock文件。
默认配置下会运行这里全部测试文件，可根据实际需要调整jest配置进行筛选。

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

### E2E 测试前置步骤

E2E 测试前的配置步骤如下：

  - 进入 build_system 目录

   ```bash
   cd <path_to_build_system>
   ```

  - 执行mock_sdk操作，保证本地可以编译 build_system 本身和利用 build_system 编译 hap 包:
    * SDK可以从[每日构建](https://ci.openharmony.cn/workbench/cicd/dailybuild/dailylist)下载，或者自行编译。
    * 下载`ohos-sdk-pulib_0328`，解压获得`ohos-sdk`目录，进入`ohos-sdk/Linux`解压`ets-linux-x64-6.0.0.36-Canary1.zip`获得`ets`目录。
    * `ets/ets1.2`即为SDK目录,将其中的内容复制进 `build_system/test/mock_sdk`即可。
    * 复制完成后mock_sdk目录下应存在`api`，`arkts`，`build-tools`，`kits`四个文件夹。

   - 为mock_sdk/build-tools/ets2panda/bin下的几个可执行文件提供执行权限。

   ```bash
   chmod +x test/mock_sdk/build-tools/ets2panda/bin/*
   ```

  - 将所有端到端测试代码中的 `${absolute_path_to_build_system}` 替换为实际的目录。

   ```bash
   find test -name 'build_config*.json' -exec sed -i 's|${absolute_path_to_build_system}|'"$(pwd)"'|g' {} +
   find test -name 'decl-fileInfo.json' -exec sed -i 's|${absolute_path_to_build_system}|'"$(pwd)"'|g' {} +
   ```

  - 导出LD_LIBRARY_PATH环境变量，如果不想每次都手动导出可以写进环境变量文件，注意替换成实际的目录。

   ```bash
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<your_path_to_build_system>/test/mock_sdk/build-tools/ets2panda/lib
   ```

  - 下载依赖和编译src下所有代码。

   ```bash
   npm install
   npm run build
   ```

1. **[可选]**通过修改 jest.config.js 中的 jest 参数，自定义调整 test suite 的运行方式，例如包含、排除测试文件：

   ```js
   module.exports = {
     testEnvironment: "node",
     verbose: true,
     collectCoverage: true, // 收集代码覆盖率，打开此选项会大幅影响测试速度
     coverageDirectory: "<rootDir>/coverageReport", // 调整代码覆盖率报告产物的位置
     setupFilesAfterEnv: [
       "<rootDir>/testHook/jest.memory-usage.js"
       // "<rootDir>/testHook/jest.time-usage.js"
     ], // 只加载内存监控的hook文件，时间监控的hook文件注释掉
     testMatch: [
       "test/ut/sum.test.ts"
     ], // 只运行一个测试
     testPathIgnorePatterns: [
       "/test/ut/skip/",      // 排除 skip 目录
       "/test/ut/sometest.test.ts" // 排除指定文件
     ]
   }
   ```

---

### E2E 测试示例

端到端测试的目的通常是检查一个项目代码是否能通过编译，正常生成abc文件，并且检查编译过程中是否有错误或异常。端到端测试与单元测试的运行方式类似，只是提供了不同的接口文件。

在 `build_system/test/e2e` 下添加 `compile.test.ts` 文件，该代码已在项目文件中包含：

```typescript
// 文件过大，仅展示部分

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
// 下略

```

考虑到待测试项目可能非常复杂，可能存在多个build_config.json文件，且无法得知具体的编译先后顺序，于是强制用户编写运行脚本指定编译流程。
运行方式如下：

#### 单个测试运行

   修改 build_system_Etest 脚本的 TEST 参数为测试项目脚本:

```json
"scripts": {
 "entry1_2_external_har1_2:gen_abc": "npm run build && node ./dist/entry.js ${absolute_path_to_build_system}/test/e2e/entry1_2_external_har1_2/build_config.json",
 "build_system_Etest": "TEST=entry1_2_external_har1_2:gen_abc jest --testMatch '${absolute_path_to_build_system}/test/e2e/*.test.ts'"
}
```

在TEST=后添加脚本的完整名字。

命令行运行:

```bash
npm run build_system_Etest
```

会执行测试。

添加测试的方式：
在 `build_system/test/e2e` 下新建一个测试文件，build_system_Etest会自动包含测试，或是通过调整jest配置筛选待执行测试文件，（实际上与端到端测试类似）。

***在现有的测试中有一个较为独特，`IncrementDemo:gen_abc`这个测试与增量编译相关，于是为这个测试提供了单独的命令:***
```bash
npm run IncrementCompileTest1
npm run IncrementCompileTest2
```

#### 多个测试运行
在`test/e2e`下有`run_all.sh`脚本，该脚本会循环运行全部测试文件，通过增删scripts来控制运行的测试。

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

确保脚本有执行权限
```bash
chmod +x test/e2e/run_all.sh
```
运行脚本：

```bash
./test/e2e/run_all.sh
```
会运行scripts数组中的所有测试脚本，并输出测试结果。
考虑到输出会较为复杂，建议将输出重定向到文件中，或是对输出结果进行筛选。

---

## 如何编写测试

测试套基于Jest完成，利用Jest提供的断言和匹配器机制完成测试。

### 全局配置

- 按组组织测试：`describe(name, fn)`，多个测试包含在 fn 内，组成一个组。
- 单个测试: `it(name, fn)`，fn 内是测试的具体实现，所有 it 均可以替换为别名 test。
- 反向测试：`it.failing(name, fn, timeout)`，与 it 相反，fn 成功则失败，fn 失败则成功。
- todo：`it.todo(name)`，表示该测试未编写。
- 全部测试完成前/后执行操作：`afterAll(fn, timeout)`，`beforeAll(fn, timeout)`，fn 放具体实现，可选 timeout 设置超时时间。
- 逐个测试进行前/后执行操作：`afterEach(fn, timeout)`，`beforeEach(fn, timeout)`，fn 放具体实现，可选 timeout 设置超时时间。

### 断言

expect(expr)创建断言对象，例如expect(isLinux())

### 匹配器

匹配器是断言对象提供的方法，因此链式调用即可。Jest提供了大量匹配器，这里给出常用的几个:

- toBe(value): 检查值是否相等
- toHaveBeenCalled():检查是否被调用，常用于mock函数
- toHaveReturned()：检查函数是否正常返回，常用于mock函数
- toHaveLength(number)：检查数组长度
- toBeInstanceOf(Class)：判断对象是否为类型示例，与instanceof类似
- toContain(item)：检查包含性
- 等等，更多匹配器可以参考[官方文档](https://jestjs.io/docs/expect)

### mock 函数

在 `build_system/test/ut/osType` 下添加 `osType.test.ts` 文件：

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
    // mock整个模块，然后spyOn方法
    expect(utils.isWindows()).toBe(true);
    expect(utils.isLinux()).toBe(true);
    expect(utils.isMac()).toBe(false);
    spy.mockRestore();
  });
});
```

考虑到ES6的导入是只读的，无法修改，因此只能用import * as utils 的方式引入模块。
需要mock模块还是直接mock函数需要具体情况具体分析。
mock 函数提供大量接口用于断言和检查，参考 [官方文档](https://jestjs.io/docs/mock-function-api)

### 异步代码

返回 Promise 的函数，可以直接测试，若返回 Promise.resolve()，则测试通过，若返回 Promise.reject()，则测试失败。

### 测试结果

测试结果默认打印在命令行。
以上文的UT测试为例

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

开头的三个console.log/warn是测试hook的输出，分别是内存使用情况、测试时间和abc文件大小。
格式为`[Jest][${test_name} used ${memory} MB]`，
`[Jest][${test_name} spent ${time} ms]`和
`[Jest][No .abc files found in ${cacheDir}]`。
这里第三个输出是因为测试文件夹下没有abc文件，通常是因为测试没有进行编译工作。
表格为覆盖率的简略版本，分别显示每个文件的语句覆盖率、分支覆盖率、函数覆盖率和行覆盖率。
Test Suites的数量为执行的测试文件的数量。
Tests的数量为测试文件中test/it块的数量。
考虑到编译过程中也会有命令行输出，建议调整输出流的位置，例如将编译信息输出到临时文件中。

---

## 新增文件夹树

```
build_system
├── dist                    # 编译产物目录
│   └── coverage            # 覆盖率产物目录
├── src                     # 源代码目录
├── test                    # 测试目录
│   ├── ut                  # 单元测试
│   │   ├── mockOsType               # mock 函数测试
│   │   │   └── mockOsType.test.ts   # mock 操作系统类型测试
│   │   ├── mockConsoleLog.test.ts   # mock console.log 测试
│   │   └── ...                      # 更多单元测试
│   └── e2e                 # 端到端测试
│       ├── compile.test.ts          # 编译测试
│       ├── checkHash.test.ts        # 哈希检查测试
│       ├── abcGenerationTest        # 示例 hap 项目
│       ├── ...                      # 更多端到端测试
│       └── testHook                 # 测试 hook 文件
│           ├── jest.memory-usage.js # 内存使用监控
│           ├── jest.time-usage.js   # 测试时间监控
│           └── jest.abc-size.js     # abc 文件大小监控
├── package.json            # 项目配置文件
└── jest.config.js          # Jest 配置文件
```

---

## 覆盖率报告

简略的覆盖率信息会打印到命令行。
详细的覆盖率报告默认输出到 `dist/coverage` 目录。可以通过修改 `jest` 配置中的 `coverageDirectory` 字段来调整输出位置。
Jest使用istanbul生成覆盖率报告。
覆盖率报告包括以下内容：

```
dist/coverage
├── clover.xml                # Clover格式的覆盖率报告
├── coverage-final.json       # Json格式的覆盖率报告
├── lcov-report               # 详细的 HTML 格式覆盖率报告目录
│   ├── base.css
│   ├── block-navigation.js
│   ├── favicon.png
│   ├── index.html            # 覆盖率报告，浏览器打开可查看详细覆盖率
│   ├── prettify.css
│   ├── prettify.js
│   ├── sort-arrow-sprite.png
│   └── sorter.js
└── lcov.info                 # 标准 LCOV 格式的覆盖率数据文件
```
