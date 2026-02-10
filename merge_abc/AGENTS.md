# AGENTS.md

## 项目概述

**merge_abc** 是一个专门用于合并多个 ARK 字节码（.abc）文件的工具。它使用 Protocol Buffers 进行序列化和反序列化，将多个独立的 .abc 文件合并为一个单一的输出文件。

### 核心特性

| 维度 | 说明 |
|------|------|
| **用途** | 合并多个 .abc 文件为一个 |
| **输入** | .abc 文件或目录 |
| **输出** | 合并后的 .abc 文件 |
| **序列化** | Protocol Buffers (protobuf) |
| **支持平台** | Linux、Windows、macOS |

### 技术栈

```
merge_abc → Protocol Buffers → Pandasm Program → 合并后的 .abc 文件
```

---

## 快速开始

### 基本使用

```bash
# 合并单个文件
merge_abc --input module1.abc --output merged.abc

# 合并目录中的所有 .abc 文件
merge_abc --input ./modules/ --suffix abc --output merged.abc

# 从文件列表合并（@前缀表示列表文件）
merge_abc --input @file_list.txt --output merged.abc

# 指定输出路径
merge_abc --input ./modules/ --suffix abc --output merged.abc --outputFilePath /tmp/
```

### 命令行选项

| 选项 | 说明 |
|------|------|
| `--input` | 输入路径（文件/目录/@列表文件） |
| `--suffix` | 文件后缀过滤（如 "abc"） |
| `--output` | 输出文件名 |
| `--outputFilePath` | 输出目录路径 |
| `--help` | 显示帮助信息 |

---

## 架构设计

### 目录结构

```
merge_abc/
├── protos/                        # Protocol Buffer 定义
│   ├── assemblyProgram.proto     # 程序结构
│   ├── assemblyFunction.proto    # 函数定义
│   ├── assemblyRecord.proto      # 记录定义
│   └── ...                       # 其他定义（14个文件）
├── src/                          # C++ 实现
│   ├── main.cpp                  # 程序入口
│   ├── options.cpp/h             # 命令行解析
│   ├── mergeProgram.cpp/h        # 文件收集与合并
│   ├── protobufSnapshotGenerator.cpp/h  # 序列化引擎
│   └── *Proto.cpp/h              # 各类序列化器
├── script/build_proto.sh         # Protobuf 生成脚本
└── BUILD.gn                      # GN 构建配置
```

### 合并流程

```
输入源（文件/目录/列表）
    ↓
文件收集（递归遍历、后缀过滤）
    ↓
Protobuf 反序列化（.abc → Program）
    ↓
程序合并（函数表、记录表、字面量）
    ↓
输出（merged.abc）
```

### 核心类

| 类 | 职责 |
|------|------|
| `Options` | 命令行参数解析 |
| `MergeProgram` | 文件收集与合并逻辑 |
| `ProtobufSnapshotGenerator` | Protobuf 序列化引擎 |
| `*Proto` | 各类消息序列化器（14种） |

---

## 使用指南

### 常用场景

#### 场景 1：应用打包

```bash
# 编译模块
es2abc module1.js -o build/module1.abc
es2abc module2.js -o build/module2.abc

# 合并所有模块
merge_abc --input ./build/ --suffix abc --output app.abc
```

#### 场景 2：从文件列表合并

```bash
# 创建列表文件
cat > files.txt <<EOF
./modules/core.abc
./modules/utils.abc
./modules/ui.abc
EOF

# 合并
merge_abc --input @files.txt --output app.abc
```

### 输出文件结构

合并后的 .abc 文件包含：
- Header（文件标识）
- String Table（去重后的字符串）
- Record Table（类定义）
- Function Table（函数）
- Literal Arrays（字面量）
- Type Infos（类型信息）
- Debug Info（调试信息）

---

## Protocol Buffers 规范

### .proto 文件语法

```protobuf
syntax = "proto3";
package protoPanda;

message Program {
    uint32 lang = 1;
    repeated FunctionTable functionTable = 2;
    repeated RecordTable recordTable = 3;
}
```

### 数据类型映射

| Protobuf | C++ | 说明 |
|----------|-----|------|
| `uint32` | `uint32_t` | 32位无符号整数 |
| `string` | `std::string` | 字符串 |
| `bytes` | `std::string` | 字节数组 |
| `repeated` | `std::vector` | 数组 |
| `oneof` | `std::variant` | 联合类型 |

### 编号标签规范

- **1-15**: 常用字段（1字节编码）
- **16-2048**: 较少使用的字段
- **19000-19999**: 保留（不可使用）

### C++ 映射规则

**基本类型**：
```protobuf
uint32 count = 1;  // → uint32_t count
```

**嵌套消息**：
```protobuf
message Function { repeated uint32 params = 2; }  // → std::vector<uint32_t>
```

**std::variant**：
```cpp
// C++: std::variant<uint32_t, double, std::string> value_;
// Proto: oneof value { uint32 value_u32 = 1; double value_f64 = 2; }
```

---

## 开发指南

### 添加新的 Proto 定义

1. **定义 .proto 文件**：
```bash
vim protos/myNewProto.proto
```

2. **更新 BUILD.gn**：
```gni
proto_file_defines = ["myNewProto", ...]
```

3. **实现 C++ 序列化器**（src/myNewProtoProto.cpp/h）

4. **重新构建**：
```bash
./build.sh --product-name rk3568 --build-target ets_frontend_build
```
---

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 无法找到文件 | 路径错误 | 验证路径和后缀匹配 |
| Protobuf 解析失败 | 版本不匹配 | 重新生成 Protobuf 代码 |
| 内存不足 | 文件过大 | 增加内存限制或分批合并 |
| 权限拒绝 | 文件不可读 | 检查文件权限 |
---

## 参考资料

### 相关组件

| 组件 | 路径 | 用途 |
|------|------|------|
| **es2panda** | `../es2panda/` | 编译 JS/TS 为 .abc |
| **arkguard** | `../arkguard/` | 代码混淆 |
| **legacy_bin** | `../legacy_bin/` | API8 兼容工具 |

### 构建产物

| 产物 | 位置 |
|------|------|
| merge_abc | `out/rk3568/clang_x64/arkcompiler/ets_frontend/merge_abc` |