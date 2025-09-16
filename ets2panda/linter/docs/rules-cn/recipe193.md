## 不支持void操作符

**规则：** `arkts-no-void-operator`

**级别：** error

在ArkTS1.2中，undefined作为关键字不能作为变量名称，因此不需要通过void操作符获取undefined。

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
