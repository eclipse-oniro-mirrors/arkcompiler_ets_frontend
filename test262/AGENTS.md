# AGENTS.md

## 项目概述

**test262** 是 ECMAScript 标准测试套件（Test262）的集成测试框架，用于验证 ARK 编译器对 JavaScript/TypeScript 标准的兼容性。该框架支持 ES5.1 到 ES2023 的各个版本测试，以及国际化（Intl）测试。

### 测试覆盖

| 版本 | 说明 |
|------|------|
| **ES5.1** | ECMAScript 5.1 标准测试 |
| **ES2015** | ES6 标准测试（箭头函数、类、Promise 等） |
| **ES2021** | ES2021 新特性测试 |
| **ES2022** | ES2022 新特性测试 |
| **ES2023** | ES2023 新特性测试 |
| **Intl** | 国际化 API 测试 |
| **Sendable** | 并发特性测试 |
| **Other** | 其他非标准测试 |

### 技术栈

```
Python 3 + Node.js + Test262 Suite → es2panda → ARK VM → 测试结果
```

---

## 快速开始

### 基本用法

```bash
cd test262

# 运行完整测试套件
python3 run_test262.py

# 运行 ES5.1 测试
python3 run_test262.py --es51

# 运行 ES2022 测试（全部）
python3 run_test262.py --es2022 all

# 运行 ES2023 测试（仅 ES2023）
python3 run_test262.py --es2023 only

# 运行单个测试文件
python3 run_test262.py --file data/test/language/statements/break/12.8-1.js

# 运行目录下所有测试
python3 run_test262.py --dir data/test/language/statements
```

### 常用选项

| 选项 | 说明 |
|------|------|
| `--es51` | 运行 ES5.1 测试 |
| `--es2015` | 运行 ES2015 测试 |
| `--es2021 all\|only` | 运行 ES2021 测试 |
| `--es2022 all\|only` | 运行 ES2022 测试 |
| `--es2023 all\|only` | 运行 ES2023 测试 |
| `--intl` | 运行国际化测试 |
| `--sendable` | 运行并发特性测试 |
| `--other` | 运行其他测试 |
| `--file FILE` | 运行单个测试文件 |
| `--dir DIR` | 运行目录下所有测试 |
| `--mode 1\|2\|3` | 1: default, 2: strict, 3: both |
| `--skip-list FILE` | 指定忽略列表 |

---

## 目录结构

```
test262/
├── run_test262.py              # 主测试运行器
├── config.py                   # 测试配置
├── utils.py                    # 工具函数
├── mix_compile.py              # 混合编译工具
├── run_sunspider.py           # SunSpider 性能测试
├── es5_tests.txt              # ES5 测试列表
├── es2015_tests.txt           # ES2015 测试列表
├── es2021_tests.txt           # ES2021 测试列表
├── es2022_tests.txt           # ES2022 测试列表
├── es2023_tests.txt           # ES2023 测试列表
├── intl_tests.txt             # 国际化测试列表
├── sendable_tests.txt         # 并发特性测试列表
├── other_tests.txt            # 其他测试列表
├── skip_tests.json            # 标准跳过列表
├── intl_skip_tests.json       # 国际化跳过列表
├── ignored-test262-*.txt      # 场景特定忽略列表
├── skip-test262-*.txt         # 平台特定跳过列表
├── data/                       # Test262 测试用例
│   └── test/                   # 官方 Test262 仓库
├── harness/                    # 测试工具（npm 包）
│   ├── bin/run.js             # 测试执行器
│   └── lib/                   # 测试库
├── eshost/                     # eshost npm 包
│   └── panda/                 # ARK VM 适配器
└── output/                     # 测试输出目录
    ├── *.abc                   # 生成的字节码文件
    ├── *.err                   # 错误日志
    ├── *.fail                  # 失败测试详情
    └── result.txt             # 测试统计结果
```

---

## 测试流程

### 完整测试流程

```
1. 准备环境（安装 npm 依赖）
   ↓
2. 拉取 Test262 用例（data/test）
   ↓
3. 应用 ARK 补丁（harness.patch, eshost.patch）
   ↓
4. 编译测试用例（es2panda）
   ↓
5. 执行测试（ARK VM）
   ↓
6. 收集结果（passed/failed）
   ↓
7. 生成报告（result.txt）
```

### 测试模式

| 模式 | 说明 |
|------|------|
| `--mode 1` | 仅 default 模式（非严格模式） |
| `--mode 2` | 仅 strict 模式 |
| `--mode 3` | 同时运行两种模式 |

---

## 忽略列表

### 标准忽略列表

**skip_tests.json** - 不符合要求的测试用例：
```json
{
  "test_name": {
    "comment": "Reason for skipping"
  }
}
```

### 场景特定忽略列表

**ignored-test262-{scenario}-{mode}-{arch}.txt**

| 场景 | 文件示例 |
|------|----------|
| Release x64 | `ignored-test262-release-x64.txt` |
| Debug x64 | `ignored-test262-debug-x64.txt` |
| FastVerify QEMU | `ignored-test262-fastverify-qemu-aot-pgo.txt` |
| AOT PGO LiteCG | `ignored-test262-release-x64-aot-pgo-litecg.txt` |

### 平台特定忽略列表

**skip-test262-*.txt** - 特定平台的已知问题

---

## 测试输出

### 输出文件

测试完成后在 `output/` 目录生成：

| 文件 | 说明 |
|------|------|
| `*.abc` | 编译后的字节码文件 |
| `*.err` | 错误日志 |
| `*.fail` | 失败测试详情 |
| `*.pass` | 通过测试详情 |
| `result.txt` | 测试统计报告 |

### 结果示例

```
FAIL test/language/statements/break/12.8-1.js (strict mode)

Ran 1000 tests
950 passed
50 failed
used time is: 0:15:30
```

---

## 高级用法

### 指定编译器工具

```bash
python3 run_test262.py \
  --ark-tool=/path/to/ark_js_vm \
  --ark-frontend-tool=/path/to/es2abc \
  --ark-frontend=es2panda
```

### 使用 Babel 转换

```bash
python3 run_test262.py --babel --file test.js
```

### 并行测试

```bash
# 使用 15 个线程
python3 run_test262.py --threads 15
```

---

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| npm 依赖失败 | 网络问题 | 检查网络，使用国内镜像 |
| 测试超时 | 超时时间太短 | 使用 `--timeout` 增加时间 |
| 编译失败 | es2abc 路径错误 | 检查 `--ark-frontend-tool` |
| 大量失败 | 忽略列表未更新 | 更新对应的 ignored-*.txt |

### 调试技巧

```bash
# 查看详细日志
python3 run_test262.py --file test.js 2>&1 | tee test.log

# 查看生成的字节码
ls output/*.abc

# 查看失败详情
cat output/*.fail

# 使用 es2abc 手动编译
es2abc test.js -o test.abc
```