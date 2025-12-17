## enum语法不支持混合使用不同类型的值

**规则：** `arkts-no-enum-mixed-types`

**规则解释：**

ArkTS-Sta中enum语法不支持混合使用不同类型的值，不支持浮点数类型的值。

**变更原因：**
 
为提高代码可读性和性能，禁止在enum中混用类型，禁止使用浮点数类型。

**适配建议：**

ArkTS-Sta中定义的enum，需要修改为同一类型，比如统一int、long或string。同时不建议在enum中使用复杂表达式（如加减乘除、条件表达式、特殊边界值等），不建议使用其他enum类型进行值传递。

**示例：**

ArkTS-Dyn

```typescript
enum E {
  UP = 1.5,
  MIDDLE = 1,
  DOWN = 0.75
}
```

ArkTS-Sta

```typescript
// 统一int
enum E1 {
  UP = 15,
  MIDDLE = 10,
  DOWN = 75
}

// 统一string
enum E3 {
  UP = "up",
  MIDDLE = "middle",
  DOWN = "down"
}
```