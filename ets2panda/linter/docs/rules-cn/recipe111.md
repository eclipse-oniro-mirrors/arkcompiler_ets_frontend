## enum中不支持成员为不同类型数据

**规则：**`arkts-no-enum-mixed-types`

**级别：error**

enum用来表示一组离散的数据，使用浮点数据不符合enum的设计理念。使用浮点数据可能造成精度损失的问题。因此，ArkTS1.2中enum的值必须为整型数据。

**ArkTS1.1**

```typescript
enum Size {
  UP = 1.5,
  MIDDLE = 1,
  DOWN = 0.75
}
```

**ArkTS1.2**

```typescript
enum Size{ 
  UP = 1,
  MIDDLE = 2,
  DOWN = 3
}
```
