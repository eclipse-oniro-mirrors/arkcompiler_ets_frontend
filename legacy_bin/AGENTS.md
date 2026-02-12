# AGENTS.md

## 项目概述

**legacy_bin** 为 OpenHarmony API8 应用程序提供向后兼容的编译器工具链。它包含预构建的二进制文件和 Node.js 包，用于将 JavaScript/TypeScript 源码编译为 ARK 字节码，确保与 OpenHarmony 3.1 Release 遗留应用程序的兼容性。

### 核心特性

| 维度 | 说明 |
|------|------|
| **用途** | API8 向后兼容层 |
| **目标** | OpenHarmony 3.1 Release 应用程序 |
| **语言支持** | JavaScript、TypeScript (ES6+) |
| **输出格式** | ARK 字节码 (.abc 文件) |
| **支持平台** | Linux x64、Windows x64、macOS x64/arm64 |
| **打包方式** | Node.js npm 包，附带预构建二进制文件 |

### 技术栈

```
┌─────────────────────────────────────────────────────────┐
│                   legacy_bin                            │
├─────────────────────────────────────────────────────────┤
│  Node.js 运行时   │  TypeScript 4.2.3  │  Commander    │
├─────────────────────────────────────────────────────────┤
│      js2abc 二进制文件 (C++ 编译为 ELF/PE/Mach-O)       │
├─────────────────────────────────────────────────────────┤
│         ARK 字节码 (.abc) → ARK 运行时                 │
└─────────────────────────────────────────────────────────┘
```

### 为什么叫 "legacy"？

该组件命名为 **legacy_bin** 的原因：
1. 维护与 **API8** (OpenHarmony 3.1 Release) 的兼容性
2. 新版本使用现代化的 `es2panda` 编译器
3. 包含预构建的二进制文件，不再活跃开发
4. 为现有应用程序提供向后兼容性支持

---

## 快速开始

### 前置条件

- **Node.js** >= 12.0.0
- **npm** 或 **yarn**
- 操作系统：Linux、Windows 或 macOS

### 安装步骤

```bash
# 进入 legacy_bin 目录
cd arkcompiler/ets_frontend/legacy_bin/api8

# 安装依赖
npm install

# 验证安装
npm ls
```

### 基本使用

```bash
# 编译 JavaScript 为 ARK 字节码
node src/index.js input.js -o output.abc

# 编译 TypeScript 为 ARK 字节码
node src/index.js input.ts -o output.abc

# 显示帮助信息
node src/index.js --help
```

---

## 架构设计

### 目录结构

```
legacy_bin/
├── api8/                          # API8 兼容包
│   ├── bin/                       # 预构建的 js2abc 二进制文件
│   │   ├── linux/
│   │   │   └── js2abc            # ELF 64 位可执行文件
│   │   ├── win/
│   │   │   └── js2abc.exe        # PE 可执行文件
│   │   └── mac/
│   │       └── js2abc            # Mach-O 可执行文件
│   ├── src/
│   │   └── index.js              # Webpack 打包的编译器 (3.5MB)
│   ├── node_modules/             # 依赖包
│   │   ├── commander/            # CLI 参数解析器
│   │   └── typescript/           # TypeScript 4.2.3
│   ├── package.json              # NPM 包清单
│   ├── package-lock.json         # 依赖锁定文件
│   └── manifest_tag.xml          # OpenHarmony 清单标签
├── BUILD.gn                       # GN 构建配置
└── prebuilts-readme-legacy.md    # 构建说明文档
```

### 编译流水线

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   源码文件    │     │  TypeScript  │     │    js2abc    │
│  (.js/.ts)   │────▶│   编译器     │────▶│   二进制     │
└──────────────┘     └──────────────┘     └──────────────┘
                                                 │
                                                 ▼
                                          ┌──────────────┐
                                          │  .abc 文件   │
                                          │ (ARK字节码)  │
                                          └──────────────┘
```

### 组件关系图

```
┌─────────────────────────────────────────────────────────────┐
│                    OpenHarmony 构建系统                     │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐         ┌──────────────┐                  │
│  │  legacy_bin  │────────▶│   js-loader  │                  │
│  │   (api8)     │         │  ets-loader  │                  │
│  └──────────────┘         └──────────────┘                  │
│         │                                                      │
│         │ 提供预构建二进制文件用于                              │
│         │ 向后兼容                                              │
│         ▼                                                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              OpenHarmony SDK / 运行时                │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 组件详解

### js2abc 二进制文件

**用途**：核心编译器，将 JavaScript/TypeScript 转换为 ARK 字节码

**二进制文件信息**：

| 平台 | 文件 | 大小 | 格式 |
|------|------|------|------|
| Linux x64 | `js2abc` | ~3.3 MB | ELF 64 位 |
| Windows x64 | `js2abc.exe` | ~2.4 MB | PE |
| macOS x64/arm64 | `js2abc` | ~1.1 MB | Mach-O |

**依赖项**：
- 动态链接（需要系统库）
- 平台相关：`ld-linux-x86-64.so.2` (Linux)

### TypeScript 编译器包

**文件**：`src/index.js` (3.5 MB webpack 打包文件)

**包含内容**：
- TypeScript 4.2.3 编译器
- Source map 支持
- Buffer 工具类
- Commander.js CLI 框架

**注意**：这是一个**预构建的 webpack 打包文件**。源代码位于仓库的其他位置。

### Node.js 依赖

```json
{
  "commander": "9.4.0",    // CLI 参数解析
  "typescript": "4.2.3"     // TypeScript 语言服务
}
```
---

## 使用指南

### 命令行接口

```bash
# 显示所有选项
node src/index.js --help

# 基本编译
node src/index.js input.js -o output.abc

# TypeScript 编译
node src/index.js --extension ts input.ts -o output.abc

# 指定输出目录
node src/index.js input.js --out-dir ./output

# 启用 source maps
node src/index.js input.js --source-map -o output.abc

# 设置模块类型
node src/index.js input.js --module

# 优化级别
node src/index.js input.js --opt-level 2
```

### 编程式调用

```javascript
const { compile } = require('./src/index.js');

// 编译 JavaScript
const result = compile('./input.js', {
  outputFile: './output.abc',
  extension: 'js'
});

// 编译 TypeScript
const result = compile('./input.ts', {
  outputFile: './output.abc',
  extension: 'ts',
  sourceMap: true
});
```
---

## 兼容性说明

### API 版本支持

| API 级别 | OpenHarmony 版本 | 编译器 | 状态 |
|----------|------------------|--------|------|
| API8 | 3.1 Release | legacy_bin (js2abc) | ✅ 维护中 |
| API9+ | 3.2+ | es2panda | ✅ 活跃开发 |

### 功能兼容性

| 功能特性 | API8 (legacy_bin) | API9+ (es2panda) |
|----------|-------------------|------------------|
| ES5 | ✅ 完整支持 | ✅ 完整支持 |
| ES6+ | ⚠️ 部分支持 | ✅ 完整支持 |
| TypeScript | ⚠️ 4.2.3 有限支持 | ✅ 完整支持 |
| Async/await | ✅ | ✅ |
| 装饰器 | ❌ | ✅ |
| TS 类型检查 | ❌ 仅运行时 | ✅ 完整支持 |

### 平台支持

| 平台 | 架构 | 测试状态 |
|------|------|----------|
| Linux | x86_64 | ✅ 已测试 |
| Windows | x86_64 | ✅ 已测试 |
| macOS | x86_64, arm64 | ✅ 已测试 |

---

## 维护指南

### 何时更新 legacy_bin

**更新场景**：
1. API8 兼容性的关键 bug 修复
2. 捆绑依赖的安全漏洞
3. 平台特定的构建失败

**何时不应更新**：
1. 新的 ES/TS 特性（应使用 es2panda）
2. 性能优化（专注于现代化工具链）
3. API9+ 应用程序（应使用 es2panda）

### 版本号管理

更新 `package.json` 时：

```json
{
  "name": "ts2panda",
  "version": "1.0.1",  // 递增补丁版本
  "description": "API8 编译器工具链",
  "main": "src/index.js"
}
```

### 获取帮助

- **OpenHarmony 文档**：https://docs.openharmony.cn
- **问题追踪**：https://gitee.com/openharmony/arkcompiler_ets_frontend/issues
- **社区**：https://gitee.com/openharmony/community

---

## 参考资料

### 相关组件

| 组件 | 路径 | 用途 |
|------|------|------|
| **es2panda** | `../es2panda/` | 现代化编译器 (API9+) |
| **merge_abc** | `../merge_abc/` | 字节码合并工具 |
| **arkguard** | `../arkguard/` | 代码混淆工具 |
| **ets2panda** | `../ets2panda/` | ETS 语言编译器 |

### 文件位置

| 产物 | 构建输出 | 运行时位置 |
|------|----------|------------|
| js2abc (Linux) | `out/sdk/.../js_linux/.../js2abc` | `/usr/local/bin/js2abc` |
| ts2panda 包 | `out/sdk/.../js_linux/.../index.js` | SDK 工具目录 |

### 外部依赖

```
legacy_bin
├── Node.js 生态系统
│   ├── commander: https://www.npmjs.com/package/commander
│   └── typescript: https://www.npmjs.com/package/typescript
└── OpenHarmony 运行时
    ├── arkcompiler_runtime_core
    └── arkcompiler_ets_runtime
```