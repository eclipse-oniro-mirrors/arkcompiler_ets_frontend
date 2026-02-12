# Lexer Module - AGENTS.md

## 模块职责

**词法分析器（Lexer）**：将JavaScript/TypeScript/ArkScript源代码转换为Token流

**核心功能**：字符流 → Token流、关键字识别、标识符/字面量/运算符解析、正则表达式词法分析、注释和空白字符处理、SourceLocation追踪

---

## 目录结构

```
lexer/
├── lexer.cpp              # 主词法分析器实现
├── lexer.h                # Lexer类定义
├── keywordsUtil.cpp/h     # 关键字工具函数
├── keywordString.h        # 关键字字符串映射
├── templates/             # ERB模板，自动生成关键字
│   ├── keywords.h.erb     # 生成 keywords.h
│   └── keywordsMap.h.erb  # 生成 keywordsMap.h
├── scripts/keywords.rb    # 关键字生成脚本
├── token/                 # Token相关
│   ├── token.h/cpp        # Token类定义
│   └── sourceLocation.h   # 源码位置追踪
└── regexp/regexp.cpp      # RegExp词法分析
```

---

## 核心类和API

### Lexer类

```cpp
class Lexer {
public:
    Lexer(ArenaAllocator *allocator, const std::string &source,
          ScriptExtension extension, parser::Program *program);

    // 核心方法
    Token NextToken();           // 获取下一个Token
    void SetPosition(const LexerPosition &pos);
    LexerPosition SavePosition();

    // 查询方法
    const SourceFile &GetSourceFile() const;
    ArenaAllocator *Allocator() const;

private:
    // 扫描方法
    Token ScanToken();
    Token ScanIdentifier();
    Token ScanNumber();
    Token ScanString();
    Token ScanTemplate();
    Token ScanRegExp();

    // 辅助方法
    char Peek() const;
    char Advance();
    bool Match(char expected);
    void SkipWhitespace();
    void SkipLineComment();
    void SkipBlockComment();
};
```

### Token类

```cpp
class Token {
public:
    TokenType Type() const;
    const util::StringView &Ident() const;
    const SourceLocation &Loc() const;

    enum class TokenType {
        EOF_TOKEN,
        IDENTIFIER,
        KEYW_IF, KEYW_ELSE, KEYW_FUNCTION, ...  // 关键字
        L_PAREN, R_PAREN, ...                   // 运算符
        NUMBER, STRING, ...                     // 字面量
    };
};
```

---

## 关键字生成流程

⚠️ **重要**：关键字文件由GN构建系统自动生成

### 模板位置

```
lexer/templates/
├── keywords.h.erb      # Token关键字定义
└── keywordsMap.h.erb   # 关键字字符串映射
```

### 生成说明

**BUILD.gn配置**：
```gni
keywords_generator = "lexer/scripts/keywords.rb"
```

**修改流程**：
1. 编辑ERB模板：`lexer/templates/keywords.h.erb`
2. 重新构建：`./build.sh --product-name rk3568 --build-target ets_frontend_build`

### 生成文件位置

```
out/sdk/clang_x64/obj/arkcompiler/ets_frontend/es2panda/gen/
├── keywords.h       # Token::Type 枚举中的关键字
└── keywordsMap.h    # StringView 到关键字的映射
```

### 添加新关键字

1. 编辑 `lexer/templates/keywords.h.erb`
2. 重新运行生成脚本
3. **不要**直接修改 `keywords.h`

---

## Token类型系统

### TokenType枚举

```cpp
enum class TokenType : uint8_t {
    // 特殊Token
    EOF_TOKEN = 0,
    NEW_LINE,

    // 标识符和字面量
    IDENTIFIER,
    LITERAL_STRING,
    LITERAL_NUMBER,
    LITERAL_BOOL,
    LITERAL_NULL,

    // 关键字（自动生成）
    KEYW_IF, KEYW_ELSE, KEYW_FUNCTION, KEYW_CLASS,
    KEYW_CONST, KEYW_LET, KEYW_VAR,
    // ... 约100个关键字

    // 运算符和分隔符
    L_PAREN, R_PAREN,
    L_BRACE, R_BRACE,
    L_BRACKET, R_BRACKET,
    SEMICOLON, COMMA, DOT,
    // ... 约50个运算符
};
```

### TokenFlags

```cpp
enum class TokenFlags : uint8_t {
    NONE = 0,
    NEW_LINE = 1 << 0,        // Token前有换行
    STRING_VAR = 1 << 1,      // 模板字符串变量
    ERROR_RECOVERY = 1 << 2,  // 错误恢复模式
};
```

---

## 正则表达式词法分析

```cpp
class RegExpLexer {
public:
    static RegExp *ParseRegExp(const std::string &pattern);

private:
    Token ScanRegExpToken();
    void ScanRegExpPattern();
    void ScanRegExpFlags();
};
```

**特殊Token**：
- `DIV` (/) → 可能是除法或正则开始
- `ASSIGN_DIV` (/=) → 除法赋值
- `REGEXP_LITERAL` → 正则表达式字面量

---

## SourceLocation追踪

```cpp
class SourceLocation {
public:
    size_t line;      // 行号（从1开始）
    size_t column;    // 列号（从1开始）
    size_t index;     // 在源码中的索引

    const std::string &GetFileName() const;
    std::string ToString() const;
};
```

**使用**：每个Token携带位置信息，用于错误报告和调试信息，支持SourceMap生成

---

## 修改Lexer的注意事项

### 添加新Token类型

1. 在 `token/token.h` 添加 `TokenType` 枚举值
2. 在 `lexer.cpp` 的 `ScanToken()` 添加扫描逻辑
3. 在 `parser/` 更新解析逻辑

### 添加新关键字

1. 修改 `lexer/templates/keywords.h.erb`
2. 运行生成脚本
3. 测试 `keywordsUtil::IsKeyword()`

### 性能优化

- 使用 `ArenaAllocator` 分配Token字符串
- 预读取（Peek）减少回退

---

## 调试Lexer

### 查看Token流

```cpp
while (lexer.PeekToken().Type() != TokenType::EOF_TOKEN) {
    Token token = lexer.NextToken();
    std::cout << "Token: " << TokenToString(token.Type())
              << " '" << token.Ident() << "'"
              << " at " << token.Loc().ToString() << std::endl;
}
```

---

## 相关模块

**上游**：无（Lexer是编译流水线第一阶段）

**下游**：`parser/`、`parser/context/`

**依赖**：`util/ustring.h`、`util/arena.h`、`ir/astNode.h`
