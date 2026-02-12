# AGENTS.md

## 项目概述

**test** 目录包含了 ets_frontend 项目的完整测试框架，涵盖 SDK 测试、XTS 测试、性能测试和工作负载测试。测试框架使用 Python 编写，支持自动化测试调度和结果报告。

### 测试覆盖范围

| 测试类型 | 说明 |
|----------|------|
| **SDK 测试** | 基于 OpenHarmony SDK 的集成测试 |
| **XTS 测试** | OpenHarmony 兼容性测试套件 |
| **性能测试** | JSPerf、ArkJS-VM 性能基准测试 |
| **工作负载测试** | 真实场景的工作负载验证 |
| **单元测试** | 位于 `es2panda/unittest/` |

### 技术栈

```
Python 3 + YAML + Batch/Shell Scripts → OpenHarmony SDK → 测试执行
```

---

## 快速开始

### 运行完整测试套件

```bash
cd test/scripts

# 运行所有测试（自动下载 SDK、镜像）
python entry.py

# 跳过 SDK 下载
python entry.py --skipDownloadSdk

# 跳过Dayu镜像下载
python entry.py --skipDownloadDayu
```

### 运行特定测试

```bash
# XTS 测试
cd auto_xts_test
./run.bat

# SDK 测试
cd sdk_test
python entry.py

# 性能测试
cd performance_test
python performance_entry.py
```

---

## 目录结构

```
test/
├── scripts/                      # 测试脚本
│   ├── entry.py                 # 主入口（调度所有测试）
│   ├── auto_xts_test/           # XTS 自动化测试
│   │   ├── run.bat              # XTS 测试执行脚本
│   │   ├── config.yaml          # 测试配置
│   │   └── running_modules.txt  # 运行模块列表
│   ├── sdk_test/                # SDK 测试
│   │   ├── entry.py             # SDK 测试入口
│   │   ├── run_ohos_sdk_test.sh # Shell 执行脚本
│   │   └── config.yaml          # 测试配置
│   ├── performance_test/        # 性能测试
│   │   ├── performance_entry.py # 性能测试入口
│   │   └── config.yaml          # 测试配置
│   └── utils/                   # 测试工具
│       ├── download_sdk_and_image/  # SDK/镜像下载
│       ├── flash_image/         # 镜像烧录
│       ├── commit_message/      # 提交信息获取
│       └── send_email/          # 结果邮件发送
├── sdk_test_projects/           # SDK 测试项目
│   ├── bytecodehar_test/        # 字节码 HAR 测试项目
│   ├── bytecodehar_increace_compile/  # 增量编译测试
│   └── bytecodehar_out_project/       # 输出项目测试
├── workload/                    # 工作负载测试用例
│   └── ignored-*.txt            # 各场景的忽略列表
└── ignorelist/                  # 单元测试忽略列表
    ├── ignored-ut-debug-x64-frontend.txt
    └── ignored-ut-release-x64-frontend.txt
```

---

## 测试类型

### 1. SDK 测试

**目的**：验证 ets_frontend 在真实 OpenHarmony SDK 环境中的功能

**测试内容**：
- 字节码编译（.js/.ts → .abc）
- 字节码合并（merge_abc）
- 代码混淆（arkguard）
- HAR 包构建
- 增量编译

**运行方式**：
```bash
cd scripts/sdk_test
python entry.py
# 或
./run_ohos_sdk_test.sh
```

### 2. XTS 测试

**目的**：验证与 OpenHarmony XTS (eXtreme Test Suite) 的兼容性

**测试内容**：
- XTS 框架集成测试
- API 兼容性验证
- 真机/模拟器测试

**运行方式**：
```bash
cd scripts/auto_xts_test
./run.bat
```

### 3. 性能测试

**目的**：评估编译性能和运行时性能

**测试内容**：
- JSPerf 基准测试
- ArkJS-VM 性能测试
- AOT/JIT 模式对比
- 内存使用分析

**运行方式**：
```bash
cd scripts/performance_test
python performance_entry.py
```

### 4. 工作负载测试

**目的**：使用真实场景验证编译器稳定性

**测试场景**：
| 场景 | 说明 |
|------|------|
| jsperf-jit-x64 | JIT 模式 x64 性能测试 |
| jsperf-release-qemu-aot | QEMU AOT 模式测试 |
| arkjs-vm-jsperf | ArkJS VM 性能测试 |
| app-workload | 应用工作负载测试 |

---

## 使用指南

### 测试配置

**SDK 测试配置** (`scripts/sdk_test/config.yaml`):
```yaml
test_projects:
  - path: "../sdk_test_projects/bytecodehar_test"
  - path: "../sdk_test_projects/bytecodehar_increace_compile"

sdk_path: "/path/to/ohos-sdk"
output_path: "./test_results"
```

### 定时测试

**entry.py** 支持定时执行：
```python
# 每天凌晨 2:10 执行
schedule.every().day.at("02:10").do(run)
```

### 测试流程

```
1. 下载 SDK 和镜像（可选）
   ↓
2. 烧录镜像到设备（可选）
   ↓
3. 运行 XTS 测试
   ↓
4. 运行 SDK 测试
   ↓
5. 运行性能测试
   ↓
6. 获取提交日志
   ↓
7. 发送测试结果邮件
```

### 自定义测试项目

添加新的 SDK 测试项目：
1. 在 `sdk_test_projects/` 创建项目
2. 在 `scripts/sdk_test/config.yaml` 添加配置
3. 运行测试

---

## 忽略列表

### 工作负载忽略列表

**文件格式**：`ignored-{scenario}-{mode}.txt`

**用途**：列出在特定测试场景中应该忽略的测试用例

**示例**：
```
# workload/ignored-jsperf-jit-x64.txt
# 忽略 Promise 构造函数测试
#23382
Promise_promiseconstructor
```

### 单元测试忽略列表

**文件**：`ignorelist/ignored-ut-{mode}-{arch}.txt`

**用途**：单元测试中需要跳过的测试

---

## 测试结果位置

```
scripts/sdk_test/test_results/
scripts/performance_test/results/
scripts/auto_xts_test/results/
```
