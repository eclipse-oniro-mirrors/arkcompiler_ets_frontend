# Binder Module - AGENTS.md

## 模块职责

**绑定器（Binder）**：语义分析，变量声明绑定和作用域管理

**核心功能**：标识符解析、变量声明绑定、作用域构建、变量生命周期管理、Temp变量重绑定（闭包处理）、TypeScript类型绑定、函数作用域分析

---

## 目录结构

```
binder/
├── binder.cpp/h       # 主绑定器
├── scope.cpp/h        # 作用域实现
├── variable.cpp/h     # 变量实现
├── declaration.cpp/h   # 声明节点
├── variableFlags.h    # 变量属性标志
└── tsBinding.h        # TypeScript绑定辅助
```

---

## 核心类和API

### Binder类

```cpp
class Binder {
public:
    explicit Binder(parser::Program *program, ScriptExtension extension);

    // 主入口
    void InitTopScope();
    void IdentifierAnalysis(ResolveBindingFlags flags = ResolveBindingFlags::ALL);

    // 声明管理
    template<typename T, typename... Args>
    T *AddDecl(const lexer::SourcePosition &pos, bool isDeclare, Args&&... args);

    // 作用域操作
    binder::Scope *CurrentScope();
    void PushScope(binder::Scope *scope);
    binder::Scope *PopScope();

    // 变量查询
    binder::Variable *FindVariable(const util::StringView &name);
    binder::Variable *LookupVariable(const ir::Identifier *ident);

    // 函数相关
    util::StringView GetFunctionName(ir::ScriptFunction *func);
    void AddFunctionName(ir::ScriptFunction *func, const util::StringView &name);

private:
    parser::Program *program_;
    binder::Scope *currentScope_;
    ScriptExtension extension_;
    ArenaVector<binder::Scope *> functionScopes_;
    ArenaHashMap<util::StringView, binder::Variable *> variableNames_;
};
```

### Scope类

```cpp
class Scope {
public:
    enum class ScopeType {
        GLOBAL, FUNCTION, BLOCK, CLASS, CATCH, WITH, FOR_VAR, MODULE,
    };

    explicit Scope(ScopeType type, Scope *parent = nullptr);

    // 变量操作
    void AddBinding(binder::Variable *var);
    binder::Variable *FindLocal(const util::StringView &name);
    binder::Variable *Find(const util::StringView &name);

    // 作用域层级
    Scope *Parent() const;
    ScopeType Type() const;
    size_t Depth() const;

    // 查询
    bool IsGlobal() const;
    bool IsFunction() const;
    bool IsBlock() const;
    bool IsCatch() const;

private:
    ScopeType type_;
    Scope *parent_;
    size_t depth_;
    ArenaHashMap<util::StringView, binder::Variable *> bindings_;
};
```

### Variable类

```cpp
class Variable {
public:
    enum class VariableFlags {
        NONE = 0,
        CONST = 1 << 0,    // const声明
        LET = 1 << 1,      // let声明
        VAR = 1 << 2,      // var声明
        FUNCTION = 1 << 3, // 函数声明
        CLASS = 1 << 4,    // class声明
        PARAM = 1 << 5,    // 函数参数
        IMPORT = 1 << 6,   // import导入
        HOISTED = 1 << 7,  // 变量提升
        TDZ = 1 << 8,      // 暂时性死区
        INITED = 1 << 9,   // 已初始化
        ASSIGNED = 1 << 10,// 已赋值
        USED = 1 << 11,    // 已使用
        CAPTURED = 1 << 12,// 被闭包捕获
        INTERNAL = 1 << 13,// 内部变量
    };

    Variable(const util::StringView &name, VariableFlags flags,
             binder::Scope *scope, const lexer::SourcePosition &pos);

    const util::StringView &Name() const;
    binder::Scope *Scope() const;
    const lexer::SourcePosition &Declaration() const;

    bool IsConst() const;
    bool IsLet() const;
    bool IsVar() const;
    bool IsHoisted() const;
    bool IsCaptured() const;

    void SetFlag(VariableFlags flag);
};
```

---

## 标识符解析流程

### 解析阶段

```
Parse阶段 → Binder阶段 → Compile阶段
AST        → Variable    → 字节码
```

### 解析示例

```javascript
let x = 1;
function foo() {
    console.log(x);
    let y = x + 1;
}
```

```cpp
// 标识符分析
void Binder::IdentifierAnalysis() {
    Program()->Ast()->Transform(this, [](ir::AstNode *node) {
        if (node->IsIdentifier()) {
            ir::Identifier *ident = node->AsIdentifier();
            Variable *var = LookupVariable(ident);
            ident->SetVariable(var);
        }
    });
}

// 变量查找
Variable *Binder::LookupVariable(const ir::Identifier *ident) {
    const util::StringView &name = ident->Name();

    // 1. 当前作用域查找
    if (Variable *var = CurrentScope()->FindLocal(name)) {
        return var;
    }

    // 2. 向上遍历作用域链
    Scope *scope = CurrentScope();
    while (scope != nullptr) {
        if (Variable *var = scope->Find(name)) {
            if (var->HasFlag(VariableFlags::TDZ)) {
                ThrowReferenceError(ident);
            }
            return var;
        }
        scope = scope->Parent();
    }

    // 3. 未找到，创建全局变量
    return AddGlobalVariable(ident);
}
```

---

## 作用域类型

### 作用域类型

```cpp
enum class ScopeType {
    GLOBAL,   // 全局作用域
    FUNCTION, // 函数作用域
    BLOCK,    // 块级作用域
    CLASS,    // 类作用域
    CATCH,    // catch子句作用域
    WITH,     // with语句作用域
    FOR_VAR,  // for循环var作用域
    MODULE,   // 模块作用域
};
```

### 特点

- **全局作用域**：无父作用域，包含所有全局变量和函数声明，`var`声明绑定到全局作用域
- **函数作用域**：包含函数参数、内部变量、嵌套函数，`arguments`对象绑定到函数作用域
- **块级作用域**：`let`和`const`声明绑定到块级作用域，TDZ检查
- **类作用域**：包含实例属性、静态属性、方法，私有字段（#field）
- **Catch作用域**：catch参数绑定到catch作用域

---

## 变量提升（Hoisting）

### 提升规则

```javascript
// var声明提升
console.log(x);  // undefined
var x = 1;

// 函数声明提升
foo();  // 可以调用
function foo() {}

// let/const不提升（TDZ）
console.log(y);  // ReferenceError
let y = 2;
```

### Binder实现

```cpp
void Binder::AnalyzeHoisting(ir::Statement *stmt) {
    switch (stmt->Type()) {
        case ir::AstNodeType::VARIABLE_DECLARATION:
            // var声明提升到作用域顶部
            if (stmt->AsVariableDeclaration()->Kind() == ir::VariableDeclarationKind::VAR) {
                for (auto *decl : stmt->AsVariableDeclaration()->Declarators()) {
                    Variable *var = AddVariable(decl);
                    var->SetFlag(VariableFlags::HOISTED);
                    var->SetFlag(VariableFlags::TDZ);
                }
            }
            break;

        case ir::AstNodeType::FUNCTION_DECLARATION:
            // 函数声明提升
            Variable *var = AddFunctionVariable(stmt->AsFunctionDeclaration());
            var->SetFlag(VariableFlags::HOISTED);
            var->ClearFlag(VariableFlags::TDZ);
            break;
    }
}
```

---

## 闭包和变量捕获

### 捕获检测

```javascript
function outer() {
    let x = 1;
    return function inner() {
        console.log(x);  // x被inner捕获
    };
}
```

### Binder实现

```cpp
void Binder::DetectCaptures() {
    for (auto *funcScope : functionScopes_) {
        AnalyzeFunctionCaptures(funcScope);
    }
}

void Binder::AnalyzeFunctionCaptures(Scope *funcScope) {
    for (auto *var : funcScope->Bindings()) {
        if (var->Scope() != funcScope && !var->IsGlobal()) {
            var->SetFlag(VariableFlags::CAPTURED);

            if (NeedsRebinding(var)) {
                RebindVariable(var);
            }
        }
    }
}
```

---

## TypeScript类型绑定

```cpp
void Binder::BindTypeNode(ir::TSType *type) {
    if (type->IsTypeReference()) {
        ir::TSTypeReference *ref = type->AsTypeReference();
        Variable *typeVar = LookupType(ref->TypeName());
        ref->SetVariable(typeVar);
    } else if (type->IsTypeLiteral()) {
        BindTypeLiteral(type->AsTypeLiteral());
    }
}
```

---

## 修改Binder的注意事项

### 添加新变量类型

1. 在 `VariableFlags` 添加新标志
2. 在 `scope.cpp` 更新绑定逻辑
3. 在 `binder.cpp` 更新解析逻辑

### 修改作用域规则

1. 在 `scope.cpp` 修改 `Find()` 逻辑
2. 更新TDZ检查
3. 测试作用域嵌套

### TypeScript特性

1. 在 `tsBinding.h` 添加辅助函数
2. 在 `binder.cpp` 添加类型绑定调用
3. 确保 `extension_ == ScriptExtension::TS`

---

## 相关模块

**上游**：`parser/` - AST输入

**下游**：`typescript/`、`compiler/`

**依赖**：`ir/`、`util/arena.h`、`util/ustring.h`
