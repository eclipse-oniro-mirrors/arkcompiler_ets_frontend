# Compiler Module - AGENTS.md

## 模块职责

**编译器（Compiler）**：将AST转换为ARK字节码（.abc文件）

**核心功能**：AST → Panda字节码指令、寄存器分配、作用域和环境管理、控制流编译、函数编译、异常处理、模块系统、并行编译

---

## 目录结构

```
compiler/
├── core/                          # 核心编译基础设施
│   ├── compilerImpl.cpp/h         # CompilerImpl主入口
│   ├── compilerContext.cpp/h      # CompilerContext上下文
│   ├── pandagen.cpp/h             # PandaGen字节码生成器（核心）
│   ├── emitter.cpp/h              # Emitter字节码序列化
│   ├── regAllocator.cpp/h         # 寄存器分配器
│   ├── regScope.cpp               # 寄存器作用域管理
│   ├── compileQueue.cpp           # 并行编译队列
│   ├── function.cpp               # 函数编译状态
│   ├── labelTarget.cpp            # 跳转标签管理
│   └── templates/isa.h.erb        # ISA模板文件
├── base/                          # 编译模式
│   ├── condition.cpp              # 条件表达式编译
│   ├── iterators.cpp              # for-in/for-of迭代
│   ├── destructuring.cpp          # 解构赋值
│   ├── hoisting.cpp               # 变量提升
│   ├── lexenv.cpp                 # 词法环境
│   └── catchTable.cpp             # 异常表生成
├── function/                      # 函数类型编译
│   ├── functionBuilder.cpp        # 普通函数
│   ├── asyncFunctionBuilder.cpp   # async函数
│   ├── generatorFunctionBuilder.cpp   # generator函数
│   └── asyncGeneratorFunctionBuilder.cpp
└── debugger/debuginfoDumper.cpp
```

---

## 核心类和API

### CompilerImpl类

```cpp
class CompilerImpl {
public:
    explicit CompilerImpl(size_t threadCount);
    panda::pandasm::Program *Compile(parser::Program *ast, const CompilerOptions &options);
    void SetFileThreadCount(size_t count);
    void SetFunctionThreadCount(size_t count);
};
```

### CompilerContext类

```cpp
class CompilerContext {
public:
    PandaGen *GetPandaGen() const;
    Emitter *GetEmitter() const;
    RegAllocator *GetRegAllocator() const;
    bool IsDumpAssembly() const;
    int OptLevel() const;
};
```

### PandaGen类（核心）

```cpp
class PandaGen {
public:
    explicit PandaGen(compiler::CompilerContext *ctx, binder::Scope *scope);
    void Compile();
    void CompileStatement(const ir::Statement *stmt);
    void CompileExpression(const ir::Expression *expr);
    void Emit(BytecodeInstruction inst);
    Reg AllocReg();
    void FreeReg(Reg reg);
};
```

### Emitter类

```cpp
class Emitter {
public:
    explicit Emitter(CompilerContext *ctx);
    panda::pandasm::Program *Finalize(bool generateArkMainFile, const std::string &packageName);
    FunctionEmitter *CreateFunctionEmitter(const ir::ScriptFunction *func);
    void Emit(BytecodeInstruction inst);
};
```

---

## 字节码指令生成

### 基本指令模式

```cpp
// 加载常量
void EmitLoadConst(VReg v0, const ir::Literal *lit) {
    switch (lit->Type()) {
        case ir::LiteralType::NUMBER: Emit(Opcode::LDAI, lit->AsNumberLiteral()->Value()); break;
        case ir::LiteralType::STRING: Emit(Opcode::LDA_STR, lit->AsStringLiteral()->Str()); break;
        case ir::LiteralType::BOOLEAN: Emit(lit->AsBooleanLiteral()->Value() ? Opcode::LDA_TRUE : Opcode::LDA_FALSE); break;
        case ir::LiteralType::NULL: Emit(Opcode::LDNULL); break;
    }
}

// 二元运算
void CompileBinary(const ir::BinaryExpression *expr) {
    CompileExpression(expr->GetLeft());
    Reg left = GetRegAllocator()->AllocReg();
    EmitStoreAccumulator(left);
    CompileExpression(expr->GetRight());
    switch (expr->GetOperator()) {
        case TokenType::ADD: Emit(Opcode::ADD, left); break;
        case TokenType::SUB: Emit(Opcode::SUB, left); break;
    }
    GetRegAllocator()->FreeReg(left);
}
```

### 控制流指令

```cpp
// If语句
void CompileIf(const ir::IfStatement *stmt) {
    CompileExpression(stmt->GetTest());
    Reg testReg = AllocReg();
    EmitStoreAccumulator(testReg);
    Label elseLabel = labelTarget_->AllocLabel();
    EmitWide(Opcode::JEQZ, elseLabel, testReg);
    CompileStatement(stmt->GetConsequent());
    Label endLabel = labelTarget_->AllocLabel();
    Emit(Opcode::JMP, endLabel);
    labelTarget_->SetLabel(elseLabel);
    if (stmt->GetAlternate()) CompileStatement(stmt->GetAlternate());
    labelTarget_->SetLabel(endLabel);
    FreeReg(testReg);
}

// While循环
void CompileWhile(const ir::WhileStatement *stmt) {
    Label loopStart = labelTarget_->AllocLabel();
    Label loopEnd = labelTarget_->AllocLabel();
    labelTarget_->SetLabel(loopStart);
    CompileExpression(stmt->GetTest());
    Reg testReg = AllocReg();
    EmitStoreAccumulator(testReg);
    EmitWide(Opcode::JEQZ, loopEnd, testReg);
    CompileStatement(stmt->GetBody());
    Emit(Opcode::JMP, loopStart);
    labelTarget_->SetLabel(loopEnd);
    FreeReg(testReg);
}
```

---

## 寄存器分配

```cpp
class RegAllocator {
public:
    explicit RegAllocator(size_t regCount);
    Reg AllocReg();
    void FreeReg(Reg reg);
private:
    std::vector<bool> regMap_;
    size_t regCount_;
};

class RegScope {
public:
    explicit RegScope(RegAllocator *allocator);
    ~RegScope();  // 自动释放所有寄存器
    Reg AllocReg();
private:
    RegAllocator *allocator_;
    std::vector<Reg> regs_;
};
```

---

## LexicalEnvironment（词法环境）

```cpp
class EnvScope {
public:
    explicit EnvScope(PandaGen *pg, binder::Scope *scope);
    void CreateBinding(const binder::Variable *var);
    void InitializeBinding(const binder::Variable *var);
    void SetBinding(const binder::Variable *var);
    Reg GetBinding(const binder::Variable *var);
    void PushEnv();
    void PopEnv();
};
```

---

## 函数编译

```cpp
class FunctionBuilder {
public:
    explicit FunctionBuilder(PandaGen *pg, const ir::ScriptFunction *func);
    void Compile();
    void CompileParameters();
    void CompileBody();
    void EmitReturn();
};

class AsyncFunctionBuilder : public FunctionBuilder {
public:
    void Compile() override;
    void EmitCreatePromise();
    void EmitResolvePromise(Reg result);
    void EmitAwait(const ir::AwaitExpression *awaitExpr);
};

class GeneratorFunctionBuilder : public FunctionBuilder {
public:
    void Compile() override;
    void EmitCreateGenerator();
    void EmitYield(const ir::YieldExpression *yieldExpr);
};
```

---

## 异常处理

```cpp
class CatchTable {
public:
    explicit CatchTable(PandaGen *pg);
    void BeginTry(const ir::TryStatement *stmt);
    void EndTry();
    void BeginCatch(const ir::CatchClause *catchClause);
    void EndCatch();
    void BeginFinally(const ir::BlockStatement *finalizer);
    void EndFinally();
private:
    PandaGen *pg_;
    Label tryStart_, tryEnd_, catchStart_, finallyStart_;
    Reg exceptionReg_;
};

void CompileTry(const ir::TryStatement *stmt) {
    CatchTable catchTable(this);
    catchTable.BeginTry(stmt);
    CompileStatement(stmt->GetBlock());
    catchTable.EndTry();
    if (stmt->GetCatchClauses().size() > 0) {
        catchTable.BeginCatch(stmt->GetCatchClauses()[0]);
        CompileStatement(stmt->GetCatchClauses()[0]->GetBody());
        catchTable.EndCatch();
    }
    if (stmt->GetFinalizer()) {
        catchTable.BeginFinally(stmt->GetFinalizer());
        CompileStatement(stmt->GetFinalizer());
        catchTable.EndFinally();
    }
}
```

---

## ISA指令系统

### 指令模板

```
compiler/core/templates/isa.h.erb    # ISA指令定义模板
```

**修改流程**：
1. 编辑ERB模板：`compiler/templates/isa.h.erb`
2. 重新构建：`./build.sh --product-name rk3568 --build-target ets_frontend_build`

### 生成文件位置

```
arkcompiler/ets_frontend/out/gen/isa.h
```

### 指令示例

```cpp
// 加载: LDA, LDAI, LDA_STR, LDUNDEFINED, LDNULL
// 运算: ADD, SUB, MUL, DIV, MOD
// 比较: EQ, NEQ, LT, GT, LE, GE
// 跳转: JMP, JEQZ, JNEZ
// 调用: CALL, CALL_THIS, CALL_NEW
// 返回: RETURN, RETURNUNDEF
```

---

## 并行编译

```
1. 文件级并行 (CompileQueue)   ──> 每个源文件独立编译
2. 函数级并行 (CompileFuncQueue) ──> 每个函数独立编译
3. 类级并行 (CompileAbcClassQueue) ──> 每个类独立编译
```

```cpp
CompilerOptions options;
options.fileThreadCount = 4;
options.functionThreadCount = 8;
options.abcClassThreadCount = 16;
```

---

## 修改Compiler的注意事项

### 添加新字节码指令

1. 修改 `compiler/templates/isa.h.erb`, `arkcompiler/runtime_core/isa/isa.yaml`
2. 重新生成 `arkcompiler/ets_frontend/out/gen/isa.h`
3. 在 `PandaGen` 添加发射方法
4. 在 `Emitter` 添加序列化逻辑

### 添加新AST节点编译

1. 在 `compiler/core/pandagen.cpp` 添加 `CompileXxx()`
2. 处理寄存器分配和释放
3. 发射正确的字节码指令
4. 处理异常边界（如果需要）

---

## 相关模块

**上游**：`parser/`、`binder/`、`typescript/`

**下游**：`arkassembler`、`arkbytecodeopt`

**依赖**：`ir/`、`binder/`、`util/arena.h`
