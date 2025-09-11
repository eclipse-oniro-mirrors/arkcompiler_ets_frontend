## 不支持void操作符

**规则：** `arkts-no-void-operator`

**规则解释：**

ArkTS1.2不支持void操作符获取undefined。

**变更原因：**
 
在ArkTS1.2中，undefined是关键字，不能用作变量名称，因此无需使用void操作符获取undefined。

**适配建议：**

使用IIFE（立即执行函数表达式）来执行运算符的表达式，并返回undefined。

**示例：**

**ArkTS1.1**
```typescript
let s = void 'hello';
console.log(s);  // output: undefined

let a = 5;
let b = void (a + 1);

function logValue(value: any) {
    console.log(value);
}
logValue(void 'data');

let fn = () => void 0;
```

**ArkTS1.2**
```typescript
(() => {
    'hello'
    return undefined;
})()

let a = 5;
let b = (() => {
    a + 1;
    return undefined;
})();  // 替换为 IIFE

logValue((() => {
    'data';
    return undefined;
})());  // 替换为 IIFE

let fn = () => undefined;  // 直接返回 `undefined`
```