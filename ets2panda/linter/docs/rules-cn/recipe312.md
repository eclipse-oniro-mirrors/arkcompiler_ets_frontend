## `attributeValue`方法不再需要传入泛型参数

**规则：** `sdk-no-props-by-index`

**规则解释：**

ArkTS1.2中，`attributeValue`方法不再需要传入泛型参数。

**变更原因：**

方法签名变更。

**适配建议：**

请删除attributeValue中的泛型参数。

**示例：**

**ArkTS1.1**
```typescript
// ArkTS1.1API定义
declare interface ElementAttributeValues {
    description: string;
    checkable: boolean;
}
declare function attributeValue<T extends keyof ElementAttributeValues>(p: ElementAttributeValues[T]): void;

// ArkTS1.1应用代码
attributeValue<'checkable'>(true);
```

**ArkTS1.2**
```typescript
// ArkTS1.2API定义
declare interface ElementAttributeValues {
    description: string;
    checkable: boolean;
}
declare function attributeValue(p: string | boolean): void;

// ArkTS1.2应用代码
attributeValue(true);
```