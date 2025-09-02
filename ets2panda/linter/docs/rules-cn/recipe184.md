## 不支持可选方法

**规则：**`arkts-optional-methods`

**级别：error**

ArkTS1.2中类的方法被所有类的实例所共享，增加可选方法的支持会增加开发者判断空值的成本，影响性能。

**ArkTS1.1**

```typescript
interface InterfaceA {
  aboutToDisappear?(): void
}
class ClassA {
  aboutToDisappear?(): void {}
}
```

**ArkTS1.2**

```typescript
interface InterfaceA {
  aboutToDisappear?: () => void
}
class ClassA {
  aboutToDisappear?: () => void = () => {}
}
```
