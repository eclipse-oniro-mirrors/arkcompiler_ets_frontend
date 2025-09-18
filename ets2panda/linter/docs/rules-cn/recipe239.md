## 限定关键字

**规则：** `arkts-invalid-identifier`

**规则解释：**

ArkTS1.2中不能使用关键字或保留字作为变量、函数或类型的名称。

**变更原因：**

ArkTS1.2严格定义了关键字和保留字，代码中不能将其用作变量、函数或类型的名称。

以下关键字不能用作变量或函数的名称：
```
abstract else internal static as enum launch switch async export let super await extends native this break false new throw case final null true class for override try const function package undefined constructor if private while continue implements protected default import public do interface return boolean double number Boolean Double Number byte float object Byte Float Object bigint int short Bigint Int Short char long string Char Long String void
```
以下关键字不能用作类型的名称：
```
Awaited NoInfer Pick ConstructorParameters NonNullable ReturnType Exclude Omit ThisParameterType Extract OmitThisParameter ThisType InstanceType Parameters Capitalize Uncapitalize Lowercase Uppercase ArrayBufferTypes Function Proxy AsyncGenerator Generator ProxyHandler AsyncGeneratorFunction GeneratorFunction Symbol AsyncIterable IArguments TemplateStringsArray AsyncIterableIterator IteratorYieldResult TypedPropertyDescriptor AsyncIterator NewableFunction CallableFunction PropertyDescriptor
```

**适配建议：**

请将用到关键字或保留字的变量、函数或类型重命名。

**示例：**

**ArkTS1.1**
```typescript
let as: number = 1;
const abstract: string = "abstract";
```

**ArkTS1.2**
```typescript
let a = 1;
const abstract1: string = "abstract";
```