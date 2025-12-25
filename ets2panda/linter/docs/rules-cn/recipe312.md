## `attributeValue`方法不再需要传入泛型参数

**规则：** `sdk-no-props-by-index`

**规则解释：**

ArkTS-Sta中，`attributeValue`方法不再需要传入泛型参数。

**变更原因：**

方法签名变更。

**适配建议：**

请删除attributeValue中的泛型参数。

**示例：**

**ArkTS-Dyn**
```typescript
// a.ts ArkTS-Dyn API定义 
export declare interface ElementAttributeValues {
  description: string;
  checkable: boolean;
}
export declare function attributeValue<T extends keyof ElementAttributeValues>(p: ElementAttributeValues[T]): void;

// ArkTS-Dyn应用代码
import { ElementAttributeValues, attributeValue } from './a';
attributeValue<'checkable'>(true);
```

**ArkTS-Sta**
```typescript
// a.ets ArkTS-Sta API定义
export declare interface ElementAttributeValues {
    description: string; 
    checkable: boolean;
}
export declare function attributeValue(p: string | boolean): void;

// ArkTS-Sta应用代码
import { ElementAttributeValues, attributeValue } from './a.ets';
attributeValue(true);
```