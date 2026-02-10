# IR Module - AGENTS.md

## 模块职责

**中间表示（IR）**：抽象语法树（AST）节点定义

**核心功能**：所有AST节点类型定义、AST节点基类和辅助方法、AST遍历和转换、AST转储（Dump）、节点类型映射

---

## 目录结构

```
ir/
├── astNode.h               # AstNode基类和Type枚举
├── expression.h/cpp        # Expression基类
├── statement.h/cpp         # Statement基类
├── astDump.cpp/h           # AST转储
├── base/                   # 基础节点
├── expressions/            # 表达式节点（40+）
├── statements/             # 语句节点（30+）
├── ts/                     # TypeScript节点（50+）
└── module/                 # 模块节点
```

---

## AstNode基类

```cpp
class AstNode {
public:
    enum class Type {
        // 语句：BLOCK_STATEMENT, IF_STATEMENT, WHILE_STATEMENT,
        //       FOR_UPDATE_STATEMENT, VARIABLE_DECLARATION, FUNCTION_DECLARATION
        // 表达式：IDENTIFIER, LITERAL, BINARY_EXPRESSION, CALL_EXPRESSION
        // TypeScript：TS_TYPE_REFERENCE, TS_INTERFACE_DECLARATION, TS_ENUM_DECLARATION
    };

    explicit AstNode(AstNode::Type type);

    // 类型查询
    AstNode::Type Type() const;
    virtual bool IsExpression() const;
    virtual bool IsStatement() const;

    // 类型转换
    virtual Expression *AsExpression();
    virtual Statement *AsStatement();
    virtual ir::Identifier *AsIdentifier();

    // 遍历和转换
    virtual void Transform(const ArenaVector<AstNode *> &parents,
                          std::function<void(AstNode *)> callback);
    virtual AstNode *Clone(ArenaAllocator *allocator) const;

    // 位置信息
    const lexer::SourceLocation &GetRange() const;
    AstNode *Parent() const;
};
```

---

## Expression基类

### Expression类

```cpp
class Expression : public AstNode {
public:
    explicit Expression(AstNode::Type type);
    bool IsExpression() const override;

    // 类型信息（TypeScript）
    typescript::Type *GetType() const;
    void SetType(typescript::Type *type);

    // 变量绑定
    binder::Variable *GetVariable() const;
    void SetVariable(binder::Variable *var);

    // 常量折叠
    virtual bool IsConstant() const;
    virtual Expression *GetConstantValue();
};
```

### 具体Expression类

#### Identifier

```cpp
class Identifier : public Expression {
public:
    explicit Identifier(const util::StringView &name);
    const util::StringView &Name() const;
    binder::Variable *Variable() const;
    void SetVariable(binder::Variable *var);
};
```

#### Literal

```cpp
class Literal : public Expression {
public:
    enum class LiteralType { NUMBER, STRING, BOOLEAN, NULL_TYPE, UNDEFINED, BIGINT, REGEXP };
    explicit Literal(LiteralType type);
    LiteralType GetLiteralType() const;
    bool IsConstant() const override;
};
```

#### BinaryExpression

```cpp
class BinaryExpression : public Expression {
public:
    BinaryExpression(Expression *left, lexer::TokenType op, Expression *right);
    Expression *GetLeft() const;
    Expression *GetRight() const;
    lexer::TokenType GetOperator() const;
    bool IsConstant() const override;  // 支持常量折叠
};
```

#### CallExpression

```cpp
class CallExpression : public Expression {
public:
    CallExpression(Expression *callee, ArenaVector<Expression *> &&arguments, bool optional = false);
    Expression *GetCallee() const;
    const ArenaVector<Expression *> &GetArguments() const;
    bool IsOptional() const;  // 可选链调用 callee?.()
    const ArenaVector<ir::TSTypeParameterInstantiation *> *GetTypeArgs() const;  // 泛型
};
```

---

## Statement基类

### Statement类

```cpp
class Statement : public AstNode {
public:
    explicit Statement(AstNode::Type type);
    bool IsStatement() const override;
};
```

### 具体Statement类

#### BlockStatement

```cpp
class BlockStatement : public Statement {
public:
    explicit BlockStatement(ArenaVector<Statement *> &&statements);
    const ArenaVector<Statement *> &Statements() const;
    void AddStatement(Statement *stmt);
};
```

#### IfStatement

```cpp
class IfStatement : public Statement {
public:
    IfStatement(Expression *test, Statement *consequent, Statement *alternate = nullptr);
    Expression *GetTest() const;
    Statement *GetConsequent() const;
    Statement *GetAlternate() const;  // else分支
};
```

#### VariableDeclaration

```cpp
class VariableDeclaration : public Statement {
public:
    enum class VariableDeclarationKind { VAR, LET, CONST };
    VariableDeclaration(VariableDeclarationKind kind, ArenaVector<VariableDeclarator *> &&declarators);
    VariableDeclarationKind GetKind() const;
    const ArenaVector<VariableDeclarator *> &GetDeclarators() const;
};
```

#### FunctionDeclaration

```cpp
class FunctionDeclaration : public Statement {
public:
    FunctionDeclaration(const util::StringView &name, ScriptFunction *function);
    const util::StringView &GetName() const;
    ScriptFunction *GetFunction() const;
};
```

---

## TypeScript节点

### TSTypeReference

```cpp
class TSTypeReference : public TypeNode {
public:
    explicit TSTypeReference(Expression *typeName, ArenaVector<TypeNode *> *typeArgs = nullptr);
    Expression *GetTypeName() const;
    const ArenaVector<TypeNode *> *GetTypeArgs() const;  // 泛型参数 <T, U>
};
```

### TSInterfaceDeclaration

```cpp
class TSInterfaceDeclaration : public Statement {
public:
    TSInterfaceDeclaration(const util::StringView &name, TSInterfaceBody *body,
                          ArenaVector<TSInterfaceHeritage *> *heritage = nullptr);
    const util::StringView &GetName() const;
    TSInterfaceBody *GetBody() const;
    const ArenaVector<TSInterfaceHeritage *> *GetHeritage() const;  // 继承的接口
};
```

### TSEnumDeclaration

```cpp
class TSEnumDeclaration : public Statement {
public:
    explicit TSEnumDeclaration(const util::StringView &name, ArenaVector<TSEnumMember *> &&members);
    const util::StringView &GetName() const;
    const ArenaVector<TSEnumMember *> &GetMembers() const;
};
```

---

## AST遍历和转换

```cpp
void AstNode::Transform(const ArenaVector<AstNode *> &parents,
                       std::function<void(AstNode *)> callback) {
    parents.push_back(this);
    callback(this);

    // 遍历子节点
    switch (Type()) {
        case AstNode::Type::BLOCK_STATEMENT:
            for (auto *stmt : AsBlockStatement()->Statements()) {
                stmt->Transform(parents, callback);
            }
            break;
        case AstNode::Type::BINARY_EXPRESSION:
            AsBinaryExpression()->GetLeft()->Transform(parents, callback);
            AsBinaryExpression()->GetRight()->Transform(parents, callback);
            break;
    }

    parents.pop_back();
}
```

---

## 添加新AST节点

### 步骤

1. **在 `astNode.h` 添加类型枚举**
```cpp
enum class Type { MY_NEW_STATEMENT };
```

2. **创建节点类文件**（如 `ir/statements/myStatement.h/cpp`）
```cpp
class MyStatement : public Statement {
public:
    explicit MyStatement(...);
};
```

3. **在 `parser/` 添加解析逻辑**
```cpp
Statement *ParserImpl::ParseMyStatement() {
    return AllocNode<MyStatement>(...);
}
```

4. **在 `compiler/` 添加编译逻辑**
```cpp
void PandaGen::CompileMyStatement(const ir::MyStatement *stmt) {}
```

5. **在 `astDump.cpp` 添加转储逻辑**
```cpp
void AstDumper::Visit(const ir::MyStatement *stmt) {}
```

---

## AST转储

```cpp
class AstDumper {
public:
    explicit AstDumper(const parser::Program *program);
    void Dump();  // 转储整个AST
private:
    void Visit(const ir::AstNode *node);
    void Visit(const ir::Identifier *ident);
    void Visit(const ir::Literal *lit);
};
```

```bash
es2abc --dump-ast input.js
```

---

## 相关模块

**上游**：无（IR是数据结构定义）

**下游**：`parser/`、`binder/`、`typescript/`、`compiler/`

**依赖**：`util/arena.h`、`lexer/token/`
