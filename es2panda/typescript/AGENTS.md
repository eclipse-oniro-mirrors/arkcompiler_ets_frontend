# TypeScript Module - AGENTS.md

## 模块职责

**TypeScript类型检查器**：TypeScript类型推断和检查

**核心功能**：类型推断、类型赋值检查、泛型实例化、类型兼容性检查、联合类型和交叉类型、条件类型、映射类型、类型守卫、装饰器类型检查

---

## 目录结构

```
typescript/
├── checker.cpp/h            # 主类型检查器
├── core/                    # 核心类型系统
│   ├── checkerContext.cpp/h # 检查上下文
│   ├── typeRelation.cpp/h   # 类型关系和兼容性
│   ├── typeCreation.cpp/h   # 类型创建
│   ├── function.cpp         # 函数类型
│   └── object.cpp           # 对象类型
└── types/                   # 类型定义
    ├── type.cpp/h           # Type基类
    ├── anyType.cpp/h        # any类型
    ├── unionType.cpp/h      # 联合类型 A | B
    ├── intersectionType.cpp/h # 交叉类型 A & B
    ├── functionType.cpp/h   # 函数类型
    ├── objectType.cpp/h     # 对象类型
    ├── classType.cpp/h      # 类类型
    └── typeParameter.cpp/h  # 类型参数（泛型）
```

---

## 核心类和API

### Checker类

```cpp
class Checker {
public:
    explicit Checker(binder::Binder *binder, ScriptExtension extension);

    // 主入口
    void Check(ir::ScriptFunction *func);

    // 表达式类型检查
    Type *GetType(const ir::Expression *expr);
    Type *CheckExpression(const ir::Expression *expr);
    Type *CheckBinaryExpression(const ir::BinaryExpression *expr);
    Type *CheckCallExpression(const ir::CallExpression *expr);

    // 语句类型检查
    void CheckStatement(const ir::Statement *stmt);
    void CheckVariableStatement(const ir::VariableStatement *stmt);

    // 类型注解
    Type *ResolveTypeAnnotation(const ir::TypeNode *annotation);

    // 类型兼容性
    bool IsTypeAssignableTo(Type *source, Type *target);
    bool AreTypesEqual(Type *left, Type *right);

private:
    CheckerContext *context_;
    binder::Binder *binder_;
};
```

### Type类（基类）

```cpp
class Type {
public:
    enum class TypeId {
        ANY, BOOLEAN, NUMBER, STRING, VOID, UNDEFINED, NULL_TYPE, NEVER,
        FUNCTION, OBJECT, INTERFACE, CLASS, TUPLE, ARRAY, ENUM,
        UNION, INTERSECTION, TYPE_PARAMETER, TYPE_REFERENCE,
    };

    // 类型查询
    virtual TypeId GetId() const = 0;
    virtual bool IsAnyType() const;
    virtual bool IsUnionType() const;
    virtual bool IsFunctionType() const;
    virtual bool IsObjectType() const;

    // 类型关系
    virtual bool IsSupertypeOf(const Type *other) const;
    virtual bool IsSubtypeOf(const Type *other) const;
};
```

---

## 基础类型

### 原始类型

```cpp
class BooleanType : public Type {
    TypeId GetId() const override { return TypeId::BOOLEAN; }
};

class NumberType : public Type {
    TypeId GetId() const override { return TypeId::NUMBER; }
};

class StringType : public Type {
    TypeId GetId() const override { return TypeId::STRING; }
};
```

### 特殊类型

```cpp
class AnyType : public Type {
    // any是所有类型的超类型和子类型
    bool IsSupertypeOf(const Type *other) const override { return true; }
    bool IsSubtypeOf(const Type *other) const override { return true; }
};

class NeverType : public Type {
    // never是所有类型的底部类型（子类型）
    bool IsSubtypeOf(const Type *other) const override { return true; }
};
```

---

## 联合类型和交叉类型

### UnionType（A | B）

```cpp
class UnionType : public Type {
public:
    explicit UnionType(ArenaVector<Type *> &&types);
    const ArenaVector<Type *> &Types() const;

    // 联合类型是成员的子类型
    bool IsSubtypeOf(const Type *other) const override {
        for (Type *type : types_) {
            if (!type->IsSubtypeOf(other)) return false;
        }
        return true;
    }
};
```

### IntersectionType（A & B）

```cpp
class IntersectionType : public Type {
public:
    explicit IntersectionType(ArenaVector<Type *> &&types);
    const ArenaVector<Type *> &Types() const;

    // 交叉类型是成员的超类型
    bool IsSupertypeOf(const Type *other) const override {
        for (Type *type : types_) {
            if (!type->IsSupertypeOf(other)) return false;
        }
        return true;
    }
};
```

---

## 函数类型

```cpp
class FunctionType : public Type {
public:
    FunctionType(ArenaVector<Type *> &&params, Type *returnType,
                 ArenaVector<ir::TSTypeParameterDeclaration *> *typeParams = nullptr);

    const ArenaVector<Type *> &GetParameters() const;
    Type *GetReturnType() const;

    // 函数类型兼容性（协变和逆变）
    bool IsSubtypeOf(const Type *other) const override {
        FunctionType *otherFunc = other->AsFunctionType();

        // 参数类型：逆变
        for (size_t i = 0; i < params_.size(); i++) {
            if (!otherFunc->params_[i]->IsSubtypeOf(params_[i])) return false;
        }

        // 返回类型：协变
        return returnType_->IsSubtypeOf(otherFunc->returnType_);
    }
};
```

---

## 对象类型和接口

### ObjectType

```cpp
class ObjectType : public Type {
public:
    explicit ObjectType(ArenaVector<Signature *> *properties);

    Signature *GetProperty(const util::StringView &name);
    bool HasProperty(const util::StringView &name) const;
};
```

### InterfaceType

```cpp
class InterfaceType : public ObjectType {
public:
    InterfaceType(const util::StringView &name,
                  ArenaVector<ir::TSInterfaceHeritage *> *heritage);

    // 接口继承
    void AddBaseInterface(InterfaceType *base);
    bool ExtendsInterface(InterfaceType *other) const;
};
```

### ClassType

```cpp
class ClassType : public ObjectType {
public:
    ClassType(const util::StringView &name,
              ArenaVector<ir::TSInterfaceHeritage *> *heritage);

    // 类继承
    void SetBaseClass(ClassType *base);
    ClassType *GetBaseClass() const;

    // 实现的接口
    void AddImplementedInterface(InterfaceType *interface);
};
```

---

## 泛型

### TypeParameter

```cpp
class TypeParameter : public Type {
public:
    TypeParameter(const util::StringView &name,
                  Type *constraint = nullptr,
                  Type *defaultType = nullptr);

    Type *GetConstraint() const;      // 约束 T extends Number
    Type *GetDefaultType() const;     // 默认类型 T = string
};
```

### TypeReference（泛型引用）

```cpp
class TypeReference : public Type {
public:
    TypeReference(Type *base, ArenaVector<Type *> *typeArgs = nullptr);

    Type *GetBaseType() const;
    const ArenaVector<Type *> *GetTypeArguments() const;

    // 泛型实例化
    Type *Instantiate(ArenaVector<Type *> *typeArgs);
};
```

---

## 类型推断

### 推断规则

```typescript
let x = 1;              // x: number
let y = "hello";        // y: string

function foo() {
    return 42;          // 返回类型: number
}

let z: number[] = [];   // z推断为number[]
```

### Checker实现

```cpp
Type *Checker::InferType(const ir::Expression *expr) {
    switch (expr->Type()) {
        case ir::AstNodeType::NUMBER_LITERAL:
            return GlobalTypesHolder::GetNumberType();
        case ir::AstNodeType::STRING_LITERAL:
            return GlobalTypesHolder::GetStringType();
        case ir::AstNodeType::IDENTIFIER:
            return expr->AsIdentifier()->Variable()->GetType();
        case ir::AstNodeType::ARRAY_EXPRESSION:
            // 推断数组元素类型
            return CreateUnionType(elementTypes);
    }
}
```

---

## 类型守卫（Type Guards）

### typeof守卫

```typescript
function foo(x: string | number) {
    if (typeof x === "string") {
        console.log(x.toUpperCase());  // x: string
    } else {
        console.log(x.toFixed());      // x: number
    }
}
```

### Checker实现

```cpp
void Checker::CheckTypeGuard(const ir::IfStatement *stmt) {
    if (IsTypeofCheck(stmt->GetTest())) {
        Type *guardedType = ExtractGuardedType(stmt->GetTest());

        // 细化then分支的类型
        Context()->NarrowType(stmt->GetTest()->AsBinaryExpression()->GetLeft(),
                             guardedType);
        CheckStatement(stmt->GetConsequent());
    }
}
```

---

## 修改TypeScript模块的注意事项

### 添加新类型

1. 在 `types/` 创建新的Type子类
2. 实现 `IsSubtypeOf()` 和 `IsSupertypeOf()`
3. 在 `typeRelation.cpp` 添加类型关系检查
4. 在 `checker.cpp` 添加类型检查逻辑

### 添加新类型特性

1. 理解类型系统的语义
2. 实现类型推断规则
3. 实现类型兼容性检查
4. 添加错误诊断

---

## 调试TypeScript检查器

### 类型检查说明

TypeScript 类型检查功能默认**关闭**，通常不需要启用。TypeScript 代码会被正确解析和转换，但类型错误不会报错。

```bash
es2abc --extension ts --enable-type-check input.ts
```

---

## 相关模块

**上游**：`parser/`、`binder/`

**下游**：`compiler/`

**依赖**：`ir/ts/`、`binder/`、`typescript/types/`
