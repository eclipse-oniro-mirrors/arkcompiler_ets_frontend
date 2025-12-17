## 构造类型已弃用

**规则：** `sdk-constructor-funcs`


**规则解释：**

ArkTS-Sta不支持构造函数类型，需要将使用构造函数类型的地方改为lambda函数。

**变更原因：**

移除构造函数类型主要基于静态类型安全和运行时性能优化考虑。

**适配建议：**

将使用构造函数类型的地方改为lambda函数。

**示例：**

**ArkTS-Dyn**
```typescript
// a.ts ArkTS-Dyn API定义
export declare class User {}
export declare class DatabaseQuery<T> {
  constructor(entityClass: new () => T) ;
}

// ArkTS-Dyn应用代码
import { User, DatabaseQuery } from './a';
const userQuery = new DatabaseQuery(User);
```

**ArkTS-Sta**
```typescript
// a.ets ArkTS-Sta API定义
export declare class User {}
export declare function createInstence<T>(): T;
export declare class DatabaseQuery<T> {
  constructor(entityClass: () => T);
}

// ArkTS-Sta应用代码
import { createInstence, User, DatabaseQuery } from './a.ets';
const userQuery = new DatabaseQuery<User>(createInstence);
```