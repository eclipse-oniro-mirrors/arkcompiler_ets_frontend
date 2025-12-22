## `Function`类型的调用方式与Typescript不同

**规则：** `arkts-no-ts-like-function-call`

**规则解释：**

ArkTS1.1中`Function`类型可以直接用括号调用。

ArkTS1.2中`Function`类型的调用方式与Typescript不同，需要使用`unsafeCall`方法调用。

**变更原因：**

ArkTS1.2对函数类型进行严格编译期检查，要求函数返回类型严格定义。`Function`对象必须通过`unsafeCall`调用后转换类型，以确保类型安全，替代ArkTS1.1中的括号调用。

**适配建议：**

使用`unsafeCall`方法代替括号调用`Function`类型。

**示例：**

**ArkTS1.1**

```typescript
let fn: Function = (): number => { return 11 };
let res: number = fn();
```

**ArkTS1.2**

```typescript
let fn: Function = (): number => { return 11 };
let res: number = fn.unsafeCall() as number;
```