## 限制void类型的使用场景

**规则：** `sdk-limited-void-type`

**规则解释：**

在ArkTS1.1中，`void`类型可用于类型声明、类型断言、函数返回类型、泛型类型等场景。

在ArkTS1.2中，`void`类型只能用作方法的返回类型和泛型类型，并且void类型函数的返回值不能作为值传递。

**变更原因：**

ArkTS1.2对`void`类型的语义进行了收紧，限制其使用场景以增强类型安全性。

**适配建议：**

请使用undefined代替void。

**示例：**
 
**ArkTS1.1**
```typescript
// ArkTS1.1API定义
type AsyncOrVoidMethod = () => Promise<void> | void;

// ArkTS1.1应用代码
const syncFunction: AsyncOrVoidMethod = () => {
  console.log("This is a sync function");
  // 隐式返回void
};
```

**ArkTS1.2**
```typescript
// ArkTS1.2API定义
type AsyncOrVoidMethod = () => Promise<void> | undefined;

// ArkTS1.2应用代码
const syncFunction: AsyncOrVoidMethod = () => {
  console.log("This is a sync function");
  return undefined;
};
```