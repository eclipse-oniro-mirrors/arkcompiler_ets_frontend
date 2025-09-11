## 创建泛型实例需要类型实参

**规则：** `arkts-no-inferred-generic-params`

**规则解释：**

ArkTS1.2中，创建泛型实例时需要指定类型实参。

**变更原因：**
 
ArkTS1.2遵循空安全，未指定泛型类型实参时，创建实例时无法明确元素或属性类型。

**适配建议：**

创建泛型实例时指定类型实参。

**示例：**

```typescript
// 类型定义
class A<T> {
  constructor(value: T) {
  }
}
class B {
  static get<T>(value:T): string {
    return 'res';
  }
}
```

**ArkTS1.1**

```typescript
let a = new A(42); // 可省略泛型类型
let b = B.get('param');  // 可省略泛型类型
```

**ArkTS1.2**

```typescript
let a = new A<number>(42); // 需要显式指定类型
let b = B.get<string>('param');  // 需要显式指定类型
```