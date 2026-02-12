# AGENTS.md

## 项目概述

**es2panda** - JavaScript/TypeScript编译器，将源码编译为ARK字节码（.abc文件）

**技术栈**：C++17 | GN | ERB/Ruby | gtest | Python

**核心特性**：
- 7阶段编译流水线（Lexer→Parser→Binder→TypeChecker→Transformer→Compiler→Emitter）
- 支持JavaScript、TypeScript, abc三种输入
- 并行编译（文件级、函数级、类级三级并行）
- Arena分配器优化内存管理

**构建产物**：`es2abc` 可执行文件

---

## 路径说明

**本文档所有路径均为从 OpenHarmony 源码根目录的相对路径**

- 源码根目录：OpenHarmony 仓库根目录（包含 `arkcompiler/`、`build/` 等目录）
- es2panda 源码路径：`arkcompiler/ets_frontend/es2panda`
- 构建产物路径：`out/rk3568/clang_x64/arkcompiler/ets_frontend/`

---

## 核心模块
- AOT（Ahead-Of-Time）编译器入口：`aot/AGENTS.md`
- 中间表示（IR）：`ir/AGENTS.md`
- 词法分析器（Lexer）：`lexer/AGENTS.md`
- 语法分析器（Parser）：`parser/AGENTS.md`
- 绑定器（Binder）：`binder/AGENTS.md`
- TypeScript类型检查器：`typescript/AGENTS.md`

## 快速开始

### 构建项目

```bash
# 使用GN构建（需在源码根目录执行）
./build.sh --product-name rk3568 --build-target ets_frontend_build

# 构建产物位置
out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc

# 验证构建
out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc --help
```

**构建目标说明**：
- `ets_frontend_build` - 完整构建，包含es2abc
- 产品名可选：`rk3568` (默认)、`hi3516` 等

### 编译示例

```bash
# 使用完整路径调用（从源码根）
es2abc=out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc

# 编译JavaScript文件
$es2abc input.js -o output.abc

# 编译TypeScript文件（类型检查功能默认关闭，通常不需要启用）
$es2abc --extension ts input.ts -o output.abc

# 查看AST（调试用）
$es2abc --dump-ast input.js

# 查看生成的汇编
$es2abc --dump-assembly input.js
```

---

## 项目结构

```
es2panda/                           # 源码目录（构建产物名为es2abc）
├── aot/                            # CLI入口
│   ├── main.cpp                    # main函数，参数解析
│   └── options.cpp                 # 命令行选项定义
├── lexer/                          # 词法分析：源码→Token
│   ├── lexer.cpp                   # 主词法分析器
│   ├── templates/                  # ⚠️ ERB模板，自动生成关键字
│   └── token/                      # Token类型定义
├── parser/                         # 语法分析：Token→AST
│   ├── parserImpl.cpp              # 175KB，核心解析逻辑
│   ├── expressionParser.cpp
│   ├── statementParser.cpp
│   └── transformer/                # TypeScript→ES AST转换
├── binder/                         # 语义分析：变量绑定、作用域
│   ├── binder.cpp                  # 标识符解析
│   ├── scope.cpp                   # 作用域管理
│   └── variable.cpp                # 变量声明处理
├── typescript/                     # TypeScript类型检查
│   ├── checker.cpp                 # 主类型检查器
│   ├── types/                      # 类型定义（30+类型）
│   └── core/                       # 类型关系、推断
├── compiler/                       # 字节码编译：AST→字节码
│   ├── core/
│   │   ├── compilerImpl.cpp
│   │   ├── pandagen.cpp            # 字节码指令生成
│   │   └── emitter.cpp             # 字节码序列化
│   ├── base/                       # 编译模式（if/for/destructuring等）
│   └── function/                   # 函数类型（async/generator）
├── ir/                             # AST节点定义
│   ├── astNode.h                   # 基类和Type枚举
│   ├── expressions/                # 40+表达式节点
│   ├── statements/                 # 30+语句节点
│   └── ts/                         # 50+ TypeScript节点
├── util/                           # 工具类
├── test/                           # 测试套件
│   ├── runner.py                   # Python测试运行器
│   └── config.py                   # API版本映射（API9-20）
├── BUILD.gn                        # GN构建配置
├── es2abc_config.gni               # es2abc GN配置模板
```

---

## ⚠️ 项目关键约定

### 代码生成（不可手动编辑）

以下文件由GN构建系统自动生成，**修改模板而非生成文件**：

| 生成文件 | 模板文件 | 生成方式 |
|---------|---------|---------|
| `out/gen/isa.h` | `compiler/templates/isa.h.erb` | GN自动生成 |
| `out/gen/formats.h` | `compiler/templates/formats.h.erb` | GN自动生成 |
| `out/gen/keywords.h` | `lexer/templates/keywords.h.erb` | GN自动生成 |
| `out/gen/keywordsMap.h` | `lexer/templates/keywordsMap.h.erb` | GN自动生成 |

⚠️ **重要**：
- **不要手动执行生成脚本** - ISA数据文件(`isa.yaml`)是GN构建时动态生成的
- GN会自动处理所有依赖关系和生成流程
- 只需要修改`.erb`模板文件，然后重新构建即可

**正确修改流程**：
```bash
# 1. 编辑.erb模板
vim compiler/templates/isa.h.erb

# 2. 重新构建（GN会自动重新生成头文件，需在源码根执行）
./build.sh --product-name rk3568 --build-target ets_frontend_build

# 3. 验证生成的文件
ls out/rk3568/clang_x64/arkcompiler/ets_frontend/gen/isa.h

### ScriptExtension模式

```cpp
enum class 
 {
    JS,  // JavaScript：直接编译
    TS,  // TypeScript：类型检查 + AST转换 + 编译
    ABC  // 字节码反编译再编译
};
```

**关键差异**：
- `TS` 模式初始化 `parser::Transformer`
- `TS` 模式执行 `typescript::Checker` 进行类型推导
- 类型错误报告需要显式启用 `--enable-type-check`（默认关闭，通常不使用）

### API版本兼容性
修改字节码格式或新增字节码需同步更新API版本和字节码版本号。版本之间应严格隔离。

**重要**：某些字节码特性在不同API版本下可用性不同。

---

## 测试指南

### 单元测试（C++）

```bash
# 通过GN构建并运行单元测试（需在源码根执行）
./build.sh --product-name rk3568 --build-target arkcompiler/ets_frontend/es2panda:es2abc_tests

# 运行特定测试（在构建产物目录）
cd out/rk3568/clang_x64/tests/unittest/arkcompiler/ets_frontend
./lexer_test
./parser_test

# 运行特定测试用例
./lexer_test --gtest_filter="LexerTest.Keywords"
```

**测试位置**：`unittest/` 目录（源码）
**测试命名**：`${module}_test.cpp`
**框架**：gtest
**构建产物**：`out/rk3568/clang_x64/tests/unittest/arkcompiler/ets_frontend/`

### 集成测试（Python）

```bash
# 安装依赖
pip install tqdm

# 回归测试
python3 test/runner.py --regression $BUILD_DIR

# 编译器测试
python3 test/runner.py --compiler $BUILD_DIR

# base64测试
python3 test/runner.py --base64 $BUILD_DIR

# TypeScript测试（需单独设置）
python3 test/runner.py --tsc $BUILD_DIR

# 热补丁热重载冷补丁冷重载能力测试
python3 test/runner.py --hotfix --hotreload --coldfix --coldreload $BUILD_DIR

# 字节码测试
python3 test/runner.py --bytecode $BUILD_DIR

# debugger测试
python3 test/runner.py --debugger $BUILD_DIR

# 版本控制测试
python3 test/runner.py --no-progress --version-control $BUILD_DIR

# Test262（ECMAScript标准测试）
cd ../
python3 test262/run_test262.py --es2022 all --ark-frontend-binary=out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc --ark-frontend=es2panda --product-name=rk3568 --timeout=3000000

# 查看详细错误输出
python3 test/runner.py --regression $BUILD_DIR --error
```

---

## 编译选项速查

```bash
# 设置es2abc路径（从源码根）
alias es2abc=out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc

# 或直接使用完整路径
es2abc=out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc

# 调试输出
$es2abc --dump-ast input.js                  # 打印AST
$es2abc --dump-assembly input.js             # 打印汇编
$es2abc --dump-debug-info input.js           # 打印调试信息
$es2abc --dump-size-stat input.js            # 打印字节码统计
$es2abc --target-api-version=13 input.js     # 生成API版本为13的字节码

# abc文件作为输入
$es2abc --enable-abc-input input.abc         

# TypeScript文件作为输入
$es2abc input.ts

# 优化级别
$es2abc --opt-level 2 input.js               # O0-O2，默认O2

# 模块类型
$es2abc --module input.js                    # 解析为模块
```

---

## 常见任务

### 添加新AST节点

1. **定义节点类**（`ir/expressions/myExpression.h`）：
```cpp
class MyExpression : public Expression {
    ...
};
```

2. **添加Type枚举**（`ir/astNode.h`）：
```cpp
enum class Type { ..., MY_EXPRESSION };
```

3. **添加解析逻辑**（`parser/expressionParser.cpp`）：
```cpp
Expression *ParserImpl::ParseMyExpression() { ... }
```

4. **添加编译逻辑**（`compiler/core/pandagen.cpp`）：
```cpp
void PandaGen::CompileMyExpression(const ir::MyExpression *expr) { ... }
```

### 添加新字节码指令

1. **修改模板**（`arkcompiler/runtime_core/isa/isa.yaml`, `arkcompiler/ets_frontend/es2panda/compiler/templates/isa.h.erb`）
2. **重新生成**：
```bash
cd scripts && ./gen_isa.sh ...
```

3. **添加发射逻辑**（`compiler/core/pandagen.cpp`）

---

## 依赖关系

### 外部依赖

| 库 | 用途 | 链接方式 |
|---|------|---------|
| ICU | Unicode、国际化 | `hmicuuc.z` (静态) |
| arkbase | Panda运行时基础 | PUBLIC |
| arkassembler | 汇编器 | PRIVATE |
| abc2program | ABC文件读取 |

### 内部模块依赖

```
es2panda-lib (静态库，GN目标: //arkcompiler/ets_frontend/es2panda:es2panda_lib)
├── arkbase (公共库)
├── hmicuuc.z (ICU静态库)
└── arkassembler (汇编器)

es2abc (可执行文件，GN目标: //arkcompiler/ets_frontend/es2panda:es2panda)
└── es2panda-lib
```

---

## 故障排查

### GN构建失败

```bash
# 从es2panda目录返回源码根（如果需要）
cd ../../..

# 检查gn工具
which gn
gn --version

# 清理构建产物
rm -rf out/

# 重新构建
./build.sh --product-name rk3568 --build-target ets_frontend_build
```

---

## 性能优化

### Arena分配

所有AST节点必须在Arena上分配，自动批量释放，避免频繁malloc/free。

---

## 版本信息

- **构建系统**: GN (推荐)
- **GN版本**: 与OpenHarmony同步
- **C++标准**: C++17
- **许可证**: Apache License 2.0
- **构建产物**: `es2abc` (可执行文件) + `es2panda-lib` (静态库)
