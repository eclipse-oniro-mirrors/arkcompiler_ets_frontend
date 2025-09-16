## 不支持确定赋值断言

**规则：**`arkts-no-definite-assignment`

**级别：error**

ArkTS1.2不支持确定赋值断言。改为在声明变量的同时为变量赋值。

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
