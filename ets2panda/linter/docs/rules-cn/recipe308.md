## 限制void类型的使用场景

**规则：** `sdk-limited-void-type`

**规则解释：**

在ArkTS-Dyn中，`void`类型可用于类型声明、类型断言、函数返回类型、泛型类型等场景。

在ArkTS-Sta中，`void`类型只能用作方法的返回类型和泛型类型，并且void类型函数的返回值不能作为值传递。

**变更原因：**

ArkTS-Sta对`void`类型的语义进行了收紧，限制其使用场景以增强类型安全性。

**适配建议：**

请使用undefined代替void。

**示例：**
 
**ArkTS-Dyn**
```typescript
// ArkTS-Dyn API定义
type AsyncOrVoidMethod = () => Promise<void> | void;

// ArkTS-Dyn应用代码
const syncFunction: AsyncOrVoidMethod = () => {
  console.info("This is a sync function");
  // 隐式返回void
};

const asyncFunction: AsyncOrVoidMethod = async () => {
  console.info("This is an async function");
  // 隐式返回void
};

async function test() {
  syncFunction();
  await asyncFunction();
}
```

**ArkTS-Sta**
```typescript
// ArkTS-Sta API定义
type SyncOrVoidMethod = () => undefined; 
type AsyncOrVoidMethod = () => Promise<void>; 

// ArkTS-Sta应用代码
const syncFunction: SyncOrVoidMethod = () => {
  console.info("This is a sync function");
  return undefined; // 必须明确返回undefined
};

const asyncFunction: AsyncOrVoidMethod = async () => {
  console.info("This is an async function");
  return undefined; // 必须明确返回undefined
};

async function test() {
  syncFunction();
  await asyncFunction();
}
```