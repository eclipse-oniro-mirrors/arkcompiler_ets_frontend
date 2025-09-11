## void Operator Not Supported

**Rule:** `arkts-no-void-operator`

**Severity:** error

In ArkTS1.2, undefined is a keyword and cannot be used as a variable name, so the void operator is unnecessary for obtaining undefined.

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
})();  // Replaced with IIFE

logValue((() => {
    'data';
    return undefined;
})());  // Replaced with IIFE

let fn = () => undefined;  // Directly return `undefined`
```