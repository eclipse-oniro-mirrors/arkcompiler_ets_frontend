## 构造类型已弃用

**规则：** `sdk-constructor-funcs`


**规则解释：**

ArkTS1.2不支持构造函数类型，需要将使用构造函数类型的地方改为lambda函数。

**变更原因：**

移除构造函数类型主要基于静态类型安全和运行时性能优化考虑。

**适配建议：**

将使用构造函数类型的地方改为lambda函数。

**示例：**

**ArkTS1.1**
```typescript
// ArkTS1.1API定义
declare class User {}
declare class DatabaseQuery<T> {
    constructor(entityClass: new () => T);
}
// ArkTS1.1应用代码
const userQuery = new DatabaseQuery(User);
```

**ArkTS1.2**
```typescript
// ArkTS1.2API定义
declare class User {}
declare function createInstence<T>(): T;
declare class DatabaseQuery<T> {
  constructor(entityClass: () => T);
}

// ArkTS1.2应用代码
const userQuery = new DatabaseQuery<User>(createInstence);
```