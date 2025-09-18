## 不支持指数操作符

**规则：** `arkts-no-exponent-op`

**规则解释：**

ArkTS1.2不支持指数运算符（`**`和`**=`）。

**变更原因：**
 
ArkTS1.2不支持指数运算符（`**`和`**=`），采用语言基础库。

**适配建议：**

使用Math库中的pow方法来代替指数运算符。

**示例：**

**ArkTS1.1**

```typescript
let x = 2 ** 5;

let y = 3;
y **= 4; // 违反规则

let result = (1 + 2) ** (3 * 2); // 违反规则

function power(base: number, exponent: number) {
  return base ** exponent; // 违反规则
}

let values = [1, 2, 3];
let squared = values.map(v => v ** 2); // 违反规则
```

**ArkTS1.2**

```typescript
let x = Math.pow(2, 5);

let y = 3;
y = Math.pow(y, 4); // 直接使用 `Math.pow()`

let result = Math.pow(1 + 2, 3 * 2); // 直接使用 `Math.pow()`

function power(base: number, exponent: number) {
  return Math.pow(base, exponent); // 使用 `Math.pow()`
}

let values = [1, 2, 3];
let squared = values.map(v => Math.pow(v, 2)); // 使用 `Math.pow()`
```