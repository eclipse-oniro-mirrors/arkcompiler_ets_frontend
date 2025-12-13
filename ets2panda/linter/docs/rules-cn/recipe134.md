## 不支持确定赋值断言

**规则：** `arkts-no-definite-assignment`

**规则解释：**

ArkTS1.2不支持确定赋值断言，例如：let v!: T。

**变更原因：**

ArkTS1.2语法层面不支持。

**适配建议：**

修改声明方式。

**示例：**

**ArkTS1.1**

```typescript
let x!: number // 提示：在使用前将x初始化

initialize();

function initialize() {
  x = 10;
}

console.log('x = ' + x);
```

**ArkTS1.2**

```typescript
function initialize(): number {
  return 10;
}

let x: number = initialize();

console.log('x = ' + x);
```