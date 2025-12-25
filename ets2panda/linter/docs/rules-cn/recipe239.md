## 限定关键字

**规则：** `arkts-invalid-identifier`

**规则解释：**

ArkTS-Sta中不能使用关键字或保留字作为变量、函数或类型的名称。

**变更原因：**

ArkTS-Sta严格定义了关键字和保留字，代码中不能将其用作变量、函数或类型的名称。

以下硬关键字在所有上下文中均为保留字，不能用作标识符（包括变量名、函数名、类型名等）：
```
abstract enum let this as export native throw async extends new true await false null try break final overload typeof case for override undefined class function private while const if protected constructor implements public continue import return default in static do instanceof switch else interface super
```
预定义类型的名称和别名均属于硬关键字，不能用作标识符（包括变量名、函数名、类型名等）：
```
Any bigint BigInt boolean Boolean byte Byte char Char double Double float Float int Int long Long number Number Object object short Short string String void
```
以下软关键字在特定上下文中具有特殊含义，但在其他情况下可作为有效标识符：
```
catch namespace declare of finally out from readonly get set keyof type
```
以下标识符同样被视为软关键字，保留供将来使用或当前用于TypeScript中：
```
is struct var yield
```

**适配建议：**

请将用到关键字或保留字的变量、函数或类型重命名。

**示例：**

ArkTS-Dyn
```typescript
const abstract: string = "test";
const constructor = () => "test";
type as = string[];

const Any: string = "test";
const bigint = () => "test";
type double = string[];
```

ArkTS-Sta
```typescript
const abstract: string = "test"; // 报错，变量名需改为非限定关键字
const constructor = () => "test"; // 报错，函数名需改为非限定关键字
type as = string[]; // 报错，类型名需改为非限定关键字

const Any: string = "test"; // 报错，变量名需改为非限定关键字
const bigint = () => "test"; // 报错，函数名需改为非限定关键字
type double = string[]; // 报错，类型名需改为非限定关键字
```