## 不支持arguments对象

**规则：** `arkts-no-arguments-obj`

**规则解释：**

ArkTS1.2不支持通过arguments对象获取参数。

**变更原因：**

ArkTS1.2对函数调用进行严格参数检查，参数个数不符时编译报错，因此无需使用arguments机制。
 
**适配建议：**

请使用具体形参代替arguments对象获取参数。

**示例：**

**ArkTS1.1**
```typescript
function foo(u: string) {
  console.log(arguments[0]);
}

function bar(a: number, b?: number) {
  if (arguments.length === 1) {
    console.log("Only one argument passed");
  }
}

function sum() {
  let total = 0;
  for (let i = 0; i < arguments.length; i++) {
    total += arguments[i];
  }
  return total;
}

function test() {
  console.log(String(arguments.callee));
}
```

**ArkTS1.2**
```typescript
function foo(u: string) {
  console.log(u);
}

function bar(a: number, b?: number) {
  if (b === undefined) {
    console.log("Only one argument passed");
  }
}

function sum(...args: number[]) {  
  // 使用 `...rest` 替代 `arguments`
  return args.reduce((acc, num) => acc + num, 0);
}

function test() {
  console.log(test);  // 直接使用函数名
}
```