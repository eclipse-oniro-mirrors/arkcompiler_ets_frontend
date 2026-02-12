# Scripts Module - AGENTS.md

## 模块职责

**代码生成脚本**：GN构建时调用的辅助脚本

**核心功能**：
- ISA指令定义生成（gen.rb + isa.yaml → isa.h）
- 关键字Token生成（keywords.rb → keywords.h）
- 字节码生成辅助（Python脚本）
- TypeScript转换辅助

⚠️ **重要**：这些脚本由GN构建系统自动调用，**不需要手动执行**

---

## 目录结构

```
scripts/
├── gen_isa.sh              # ISA生成包装脚本（GN调用）
├── gen_keywords.sh         # 关键字生成包装脚本（GN调用）
├── generate_js_bytecode.py # Python字节码生成辅助
└── ts2abc.js               # TypeScript转换辅助
```

---

## ISA指令生成

### gen_isa.sh（由GN自动调用）

**功能**：调用gen.rb生成 `isa.h` 和 `formats.h`

---

## 关键字生成

### gen_keywords.sh（由GN自动调用）

**功能**：调用keywords.rb生成 `keywords.h` 和 `keywordsMap.h`

---

## Python辅助脚本

### generate_js_bytecode.py

**功能**：GN构建时调用的字节码生成辅助

**使用场景**：
- 由GN的`es2abc_gen_abc`模板调用
- 将JS文件编译为ABC文件
- 作为构建规则的一部分

---

## TypeScript转换脚本

### ts2abc.js

**功能**：TypeScript到字节码的Node.js转换脚本

**使用场景**：
- 工具链辅助
- 开发调试
- 非构建主流程

---

## 相关模块

**上游**：
- `arkcompiler/ets_frontend/es2panda/compiler/templates/` - ISA模板
- `arkcompiler/ets_frontend/es2panda/lexer/templates/` - 关键字模板
- `arkcompiler/runtime_core/isa/` - ISA生成器和数据

**下游**：
- `compiler/core/` - 使用isa.h
- `lexer/` - 使用keywords.h

**工具**：
- Ruby (ERB模板引擎)
- Python (GN构建脚本)
- Node.js (ts2abc.js)

---

## 关键文件速查

| 文件 | 职责 | 调用方式 |
|------|------|---------|
| `gen_isa.sh` | ISA生成包装 | GN自动调用 |
| `gen_keywords.sh` | 关键字生成包装 | GN自动调用 |
| `arkcompiler/ets_frontend/es2panda/compiler/templates/*.erb` | ISA模板 | GN读取 |
| `arkcompiler/ets_frontend/es2panda/lexer/templates/*.erb` | 关键字模板 | GN读取 |
| `arkcompiler/runtime_core/isa/gen.rb` | 实际生成器 | gen_isa.sh调用 |
| `arkcompiler/ets_frontend/es2panda/lexer/scripts/keywords.rb` | 实际生成器 | gen_keywords.sh调用 |
