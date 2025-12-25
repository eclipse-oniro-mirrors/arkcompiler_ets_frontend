## 不支持arguments对象

**规则：** `arkts-no-arguments-obj`

**规则解释：**

ArkTS-Sta不支持通过arguments对象获取参数。

**变更原因：**

ArkTS-Sta对函数调用进行严格参数检查，参数个数不符时编译报错，因此无需使用arguments机制。
 
**适配建议：**

请使用具体形参代替arguments对象获取参数。

**示例：**

ArkTS-Dyn
```typescript
function foo(u: string) {
  const args: object[] = Array.from(arguments);
  console.info(args[0].toString());
}

function bar(a: number, b?: number) {
  if (arguments.length === 1) {
    console.info("Only one argument passed");
  }
}

function sum() {
  let total = 0;
  const args: object[] = Array.from(arguments);
  for (let i = 0; i < args.length; i++) {
    total += Number(args[i]);
  }
  return total;
}

function test() {
  console.info(String(arguments.callee));
}
```

ArkTS-Sta
```typescript
function foo(u: string) {
  console.info(u);
}

function bar(a: number, b?: number) {
  if (b === undefined) {
    console.info("Only one argument passed");
  }
}

function sum(...args: number[]) {  
  // 使用 `...rest` 替代 `arguments`
  const res = args.reduce((acc: number, num: number) => acc + num);
  return res;
}

function test() {
  console.info(test);  // 直接使用函数名
}
```