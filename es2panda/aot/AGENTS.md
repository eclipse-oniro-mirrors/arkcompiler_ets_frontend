# AOT Module - AGENTS.md

## 模块职责

**AOT（Ahead-Of-Time）编译器入口**：命令行接口和编译驱动

**核心功能**：命令行参数解析、多文件编译协调、并行编译调度、输出文件生成、热重载支持、模块依赖解析

---

## 目录结构

```
aot/
├── main.cpp                 # 主入口
├── options.cpp/h            # 命令行选项解析
├── emitFiles.cpp/h          # 多文件发射
└── resolveDepsRelation.cpp  # 依赖关系解析
```

---

## 主入口（main.cpp）

```cpp
int main(int argc, char **argv)
{
    Options options;
    if (!ParseOptions(argc, argv, options)) return 1;

    std::map<std::string, util::ProgramCache*> progCache;
    int result = es2panda::Compiler::CompileFiles(options, progCache);

    if (result == 0) EmitOutputFiles(options, progCache);
    CleanupProgCache(progCache);
    return result;
}
```

---

## 命令行选项

### Options类

```cpp
struct Options {
    std::vector<std::string> inputFiles;
    std::string outputFile;
    ScriptExtension extension = ScriptExtension::JS;

    bool parseOnly = false;
    bool dumpAst = false;
    bool dumpAssembly = false;
    int optLevel = 0;
    bool enableTypeCheck = false;
    bool module = false;
    bool strict = false;

    int fileThreadCount = 2;
    int functionThreadCount = 0;

    std::string transformLib;
    bool mergeAbc = false;
    PatchFixOptions patchFixOptions;
};
```

### 命令行帮助

```bash
Usage: es2abc [OPTIONS] [input files]

Options:
  -o, --output <file>           Output ABC file
  --extension <js|ts|as>        Input file extension
  --parse-only                  Parse only, don't compile
  --dump-ast                    Dump AST to stdout
  --dump-assembly               Dump assembly to stdout
  --opt-level <0|1|2>           Optimization level
  --module                      Parse as module
  --file-thread-count <n>       File-level parallel threads
  --merge-abc                   Merge multiple ABC files
```

---

## 多文件发射

```cpp
class EmitFiles {
public:
    bool Emit(const std::map<std::string, util::ProgramCache*> &progs);
    bool MergeAbcFiles(const std::vector<panda::pandasm::Program*> &programs);

private:
    bool WriteAbcFile(const panda::pandasm::Program *program, const std::string &filename);
    bool WriteDebugInfo(const panda::pandasm::Program *program, const std::string &filename);
    bool WriteAsmFile(const panda::pandasm::Program *program, const std::string &filename);
};

bool EmitFiles::Emit(const std::map<std::string, util::ProgramCache*> &progs)
{
    if (options_.output.empty()) {
        for (auto &entry : progs) {
            if (!WriteAbcFile(entry.second->program, GetOutputFileName(entry.first)))
                return false;
        }
        return true;
    }

    if (options_.mergeAbc && progs.size() > 1) {
        std::vector<panda::pandasm::Program*> programs;
        for (auto &entry : progs) programs.push_back(entry.second->program);
        return WriteAbcFile(MergeAbcFiles(programs), options_.output);
    }

    return WriteAbcFile(progs.begin()->second->program, options_.output);
}
```

---

## 依赖关系解析

```cpp
class DepsGraph {
public:
    void AddNode(const std::string &fileName);
    void AddEdge(const std::string &from, const std::string &to);
    std::vector<std::string> TopologicalSort();
    bool DetectCircularDeps();
};

void ResolveImports(const ir::Statement *stmt, DepsGraph &graph)
{
    if (stmt->Type() == ir::AstNodeType::IMPORT_DECLARATION) {
        graph.AddEdge(currentFile_, stmt->AsImportDeclaration()->GetSource()->AsString());
    }
}
```

---

## 并行编译调度

```cpp
// 文件级并行
for (size_t i = 0; i < options.fileThreadCount; i++) {
    threads.emplace_back([&, i]() {
        for (size_t j = i; j < files.size(); j += options.fileThreadCount)
            CompileFile(files[j]);
    });
}

// 函数级并行
CompileFuncQueue queue(options.functionThreadCount, options);
queue.Schedule();
queue.Consume();
queue.Wait();
```

---

## 热重载支持

```cpp
struct PatchFixOptions {
    std::string dumpSymbolTable;
    std::string symbolTable;
    bool generatePatch = false;
    bool hotReload = false;
    bool coldReload = false;
};
```

```bash
es2abc --dump-symbol-table symbols.json input.js
es2abc --symbol-table symbols.json input.js -o output.abc
```

---

## 使用示例

```bash
# 基础编译
es2abc input.js -o output.abc
es2abc --extension ts input.ts -o output.abc

# 调试
es2abc --dump-ast input.js
es2abc --dump-assembly input.js

# 优化
es2abc --opt-level 2 input.js -o output.abc

# 多文件
es2abc file1.js file2.js -o output.abc
es2abc --merge-abc file1.abc file2.abc -o merged.abc
```

---

## 修改AOT的注意事项

### 添加新命令行选项

1. 在 `Options` 添加字段
2. 在 `ParseOptions()` 添加解析逻辑
3. 在 `PrintHelp()` 添加帮助文本
4. 传递到 `CompilerOptions`

### 添加新的输出格式

1. 在 `EmitFiles` 添加新方法
2. 在 `CompilerOptions` 添加标志
3. 实现写入逻辑

---

## 相关模块

**上游**：无（AOT是程序入口）

**下游**：`es2panda.h`、`compiler/`、`util/programCache.h`

**依赖**：`ir/`、`binder/`、`typescript/`
