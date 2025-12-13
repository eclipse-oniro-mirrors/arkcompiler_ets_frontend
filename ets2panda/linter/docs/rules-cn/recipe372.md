## 智能类型差异

**规则：** `arkts-no-ts-like-smart-type`

**规则解释：**

在ArkTS1.2中，线程共享对象在做[智能转换](#智能转换)时会表现的与ArkTS1.1不一致。

**变更原因：**

在ArkTS1.2中，由于线程共享对象在多线程中使用，编译器在做类型推导和分析时需要考虑并发场景下变量类型/值的变化。

**适配建议：**

线程共享对象要通过局部变量进行[智能转换](#智能转换)。

**示例：**

**ArkTS1.1**

```typescript
class AA {
  public static instance?: number;
  getInstance(): number {
    if (!AA.instance) {
      return 0;
    }
    return AA.instance;
  }
}
```

**ArkTS1.2**

```typescript
class AA {
  public static instance?: number;
  getInstance(): number {
    let a = AA.instance     // 需通过局部变量进行类型转换。
    if (!a) {
      return 0;
    }
    return a;
  }
}
```