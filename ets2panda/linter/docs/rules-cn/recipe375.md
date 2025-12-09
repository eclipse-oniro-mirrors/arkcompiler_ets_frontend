## catch语句中是error类型

**规则：** `arkts-no-ts-like-catch-type`

**规则解释：**

在ArkTS1.2的静态模式中，类型必须明确，同时需考虑与ArkTS1.1的兼容性。对于catch(e)的语法，默认e为Error类型。

**变更原因：**

在ArkTS1.1上catch语句中的e是any类型。编译器不会对catch语句中的异常进行编译时类型检查。当ArkTS1.1上限制throw时，只能抛出Error类型。

在ArkTS1.2中，类型必须明确。对于catch(e)的语法，默认e为Error类型，以保持与ArkTS1.1的兼容性。

**适配建议：**

开发者需要将catch(e)转换成需要处理的异常类型，例如：`(e as SomeError).prop`。

**示例：**

**ArkTS1.1**

```typescript
try {
  throw new Error();
} catch(e) {  // e是any类型
  e.message; // ArkTS1.1编译通过，运行正常
  e.prop;     // ArkTS1.1编译通过，输出undefined
}
```

**ArkTS1.2**

```typescript
try {
  throw new Error();
} catch(e:Error) {  // e是Error类型
  e.message;   // ArkTS1.2编译通过，运行正常
  e.prop;      // ArkTS1.2编译错误，需要将e转换成需要处理的异常类型，例如：(e as SomeError).prop
}
```