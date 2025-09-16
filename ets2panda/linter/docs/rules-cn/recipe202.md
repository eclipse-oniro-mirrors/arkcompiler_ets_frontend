## 限定使用字面量类型

**规则：** `arkts-limited-literal-types`

**级别：** error

ArkTS1.2不支持数字字面量类型，布尔字面量类型。

ArkTS1.2提供了更多细化的数值类型供开发者选择，更关注数值的范围而非某个特定的数字值，同时，为了更好的代码简洁性和避免引入歧义，不引入复杂的数值字面量类型语法。

**ArkTS1.1**
```typescript
let n1: 1 = 1;
let n2: 0.1 = 0.1;
let f: true = true;

function getOne(): 1 {
  return 1; 
}
function isAvailable(): true {
  return true;
}

function setFlag(flag: true) {
  console.log(flag);
}
function setPrecision(precision: 0.1) {
  console.log(precision);
}

interface Config {
  readonly enable: true;
  readonly threshold: 100;
}
```

**ArkTS1.2**
```typescript
let n1: int = 1;
let n2: number = 0.1;
let f: boolean = true;

function getOne(): int {
  return 1;
}
function isAvailable(): boolean {
  return true;
}

function setFlag(flag: boolean) {
  console.log(flag);
}
function setPrecision(precision: number) {
  console.log(precision);
}

interface Config {
  readonly enable: boolean;
  readonly threshold: int;
}
```
