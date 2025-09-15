## enum中当前语法不支持浮点数值

**规则：** `arkts-no-enum-mixed-types`

**规则解释：**

ArkTS1.2中enum当前语法不支持浮点数值。

**变更原因：**
 
enum表示一组离散的数据，使用浮点数据不符合设计理念，可能造成精度损失。因此，ArkTS1.2中enum的值必须为整型。

**适配建议：**

定义enum类型时，需显式声明number类型，以支持浮点数值。

**示例：**

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
enum Size: number{ 
  UP = 1.5,
  MIDDLE = 1,
  DOWN = 0.75
}
```