# Parser Module - AGENTS.md

## 模块职责

**语法分析器（Parser）**：将Token流转换为抽象语法树（AST）

**核心功能**：Token流 → AST、表达式解析（优先级、结合性）、语句解析（控制流、声明）、TypeScript类型解析、模块系统、错误恢复、AST转换

---

## 目录结构

```
parser/
├── parserImpl.cpp         # 核心解析逻辑
├── parserImpl.h           # ParserImpl类定义
├── expressionParser.cpp   # 表达式解析
├── statementParser.cpp    # 语句解析
├── commonjs.cpp           # CommonJS支持
├── parserFlags.h          # 解析标志位
├── context/parserContext.h # 状态管理
└── transformer/transformer.cpp # AST转换（TS→ES）
```

---

## 核心类和API

### ParserImpl类

```cpp
class ParserImpl {
public:
    ParserImpl(ArenaAllocator *allocator, std::string source, ScriptExtension extension);
    parser::Program *ParseProgram();

    Token PeekToken();
    Token NextToken();
    bool CheckToken(TokenType type);
    bool ConsumeToken(TokenType type);

    parser::ParserContext *Context();
    ArenaAllocator *Allocator();

private:
    Lexer *lexer_;
    ParserContext *context_;
    ScriptExtension extension_;
};
```

### ParserContext类

```cpp
class ParserContext {
public:
    void PushScope(ScopeType type);
    void PopScope();
    binder::Scope *CurrentScope();

    bool InFunction() const;
    bool InLoop() const;
    bool InClass() const;

    void AddLabel(const util::StringView &label);
    binder::Variable *FindLabel(const util::StringView &label);
};
```

---

## 表达式解析

### 优先级层次（从低到高）

```
1. Assignment   = += -= *= /= %=
2. Conditional  ? :
3. Logical OR   || ??
4. Logical AND  &&
5. Bitwise OR   |
6. Bitwise XOR  ^
7. Bitwise AND  &
8. Equality     == != === !==
9. Relational   < > <= >= instanceof in
10. Shift        << >> >>>
11. Additive     + -
12. Multiplicative * / %
13. Exponential  **
14. Unary        + - ~ ! typeof void delete
15. Postfix      () [] . ?.
16. Primary      Identifier, Literal, this
```

### 解析方法

```cpp
Expression *ParseExpression();              // 任何表达式
Expression *ParseAssignmentExpression();     // 赋值
Expression *ParseConditionalExpression();    // 三元
Expression *ParseLogicalOrExpression();      // ||
Expression *ParseLogicalAndExpression();     // &&
Expression *ParseEqualityExpression();      // == !=
Expression *ParseRelationalExpression();     // < >
Expression *ParseAdditiveExpression();       // + -
Expression *ParseMultiplicativeExpression(); // * /
Expression *ParseUnaryExpression();          // + - ~ !
Expression *ParsePrimaryExpression();        // 基础
```

### 示例：二元表达式

```cpp
Expression *ParseBinaryExpression(ParseFunc parseOperand, TokenType stopType) {
    Expression *left = parseOperand();
    while (MatchToken(stopType)) {
        Token op = PeekToken();
        NextToken();
        Expression *right = parseOperand();
        left = AllocNode<BinaryExpression>(left, op, right);
    }
    return left;
}
```

---

## 语句解析

```cpp
Statement *ParseStatement();               // 任何语句
Statement *ParseBlockStatement();          // {}
Statement *ParseIfStatement();             // if-else
Statement *ParseSwitchStatement();         // switch
Statement *ParseWhileStatement();          // while
Statement *ParseForStatement();            // for
Statement *ParseForInStatement();          // for..in
Statement *ParseForOfStatement();          // for..of
Statement *ParseBreakStatement();          // break
Statement *ParseContinueStatement();       // continue
Statement *ParseReturnStatement();         // return
Statement *ParseTryStatement();            // try-catch
Statement *ParseVariableStatement();       // var/let/const
Statement *ParseFunctionDeclaration();     // function
Statement *ParseClassDeclaration();        // class
Statement *ParseImportDeclaration();       // import
Statement *ParseExportDeclaration();       // export
```

### 示例：If语句

```cpp
IfStatement *ParseIfStatement() {
    ConsumeToken(TokenType::KEYW_IF);
    ConsumeToken(TokenType::L_PAREN);
    Expression *test = ParseExpression();
    ConsumeToken(TokenType::R_PAREN);
    Statement *consequent = ParseStatement();

    Statement *alternate = nullptr;
    if (MatchToken(TokenType::KEYW_ELSE)) {
        NextToken();
        alternate = ParseStatement();
    }

    return AllocNode<IfStatement>(test, consequent, alternate);
}
```

---

## TypeScript特定解析

### 类型注解

```cpp
ir::TypeNode *ParseType();
ir::TSTypeKeyword *ParsePrimitiveType();         // string, number
ir::TSArrayType *ParseArrayType();               // string[]
ir::TSTypeReference *ParseTypeReference();       // MyClass<string>
ir::TSUnionType *ParseUnionType();               // string | number
ir::TSFunctionType *ParseFunctionType();         // (x: string) => number
```

### 装饰器

```cpp
Expression *ParseDecorator();  // @Decorator
void ParseDecorators(ir::AstNode *node);
```

---

## AST转换（Transformer）

```cpp
if (extension_ == ScriptExtension::TS) {
    transformer_ = std::make_unique<Transformer>(allocator_);
    transformer_->Transform(program);
}
```

**转换内容**：
1. 类型擦除：移除TypeScript类型注解
2. 枚举处理：`enum` → IIFE + 对象
3. 命名空间：`namespace` → IIFE
4. 参数属性：构造函数参数 → 类属性
5. 装饰器：转换为运行时调用

```cpp
class Transformer {
public:
    explicit Transformer(ArenaAllocator *allocator);
    void Transform(parser::Program *program);

private:
    ir::Statement *TransformStatement(ir::Statement *stmt);
    ir::Expression *TransformExpression(ir::Expression *expr);
    ir::ClassDefinition *TransformEnum(ir::TSEnumDeclaration *enumDecl);
};
```

---

## 错误恢复

### 策略

1. **同步点**：语句结束（`;`, `}`）、块结束（`}`）、声明开始
2. **错误标记**：标记错误但不停止解析
3. **AST错误节点**：返回错误节点继续解析

```cpp
if (PeekToken().Type() == TokenType::KEYW_ASYNC) {
    ErrorIncompatibleAsync();
    NextToken();  // 跳过async关键字
}
```

---

## ParseFlags

```cpp
enum ParseFlags : uint32_t {
    NONE = 0,
    STMT = 1 << 0,           // 语句模式
    EXPRESSION = 1 << 1,      // 表达式模式
    ALLOW_SUPER = 1 << 3,     // 允许super
    ALLOW_NEW_TARGET = 1 << 4,// 允许new.target
    IN_GENERATOR = 1 << 5,    // 在generator中
    IN_ASYNC = 1 << 6,        // 在async中
    IN_CLASS = 1 << 8,        // 在class中
    TYPE_ANNOTATION = 1 << 10,// 允许类型注解（TS）
};
```

---

## 模块系统

```cpp
// ES Modules
ir::ImportDeclaration *ParseImportDeclaration();
ir::ExportNamedDeclaration *ParseExportNamedDeclaration();
ir::ExportDefaultDeclaration *ParseExportDefaultDeclaration();

// CommonJS
ir::Expression *ParseRequireCall();      // require("fs")
ir::Expression *ParseModuleExports();    // module.exports
```

---

## 修改Parser的注意事项

### 添加新语句

1. 在 `ir/statements/` 创建AST节点类
2. 在 `ir/astNode.h` 添加 `Type` 枚举值
3. 在 `statementParser.cpp` 实现 `ParseXxxStatement()`
4. 在 `compiler/` 实现编译逻辑

### 添加新表达式

1. 在 `ir/expressions/` 创建AST节点类
2. 在 `expressionParser.cpp` 实现 `ParseXxxExpression()`
3. 考虑优先级和结合性
4. 更新 `compiler/` 编译逻辑

---

## 调试Parser

```bash
es2abc --dump-ast input.js
```

---

## 相关模块

**上游**：`lexer/` - Token流输入

**下游**：`binder/`、`typescript/`、`compiler/`

**依赖**：`ir/`、`binder/scope.h`、`util/arena.h`
