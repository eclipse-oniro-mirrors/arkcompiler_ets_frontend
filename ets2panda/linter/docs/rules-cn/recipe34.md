## 创建泛型实例需要类型实参

**规则：**`arkts-no-inferred-generic-params`

**级别：error**

ArkTS1.2中，创建泛型实例时需要类型实参。

**ArkTS1.1**

```typescript
new Array() // 违反规则

new Map(); // 违反规则

class Box<T> {
  value: T;
  constructor(value: T) {
    this.value = value;
  }
}

let box = new Box(42); // 违反规则
```

**ArkTS1.2**

```typescript
new Array<SomeType>() // 指定类型

new Map<string, number>(); // 显式指定键值类型

let box = new Box<number>(42); // 明确指定类型
```
