## Promise\<void>构造器中只支持使用resolve(undefined)

**规则：** `arkts-promise-with-void-type-need-undefined-as-resolve-arg`

**规则解释：**

ArkTS-Sta中void不能作为变量类型声明；作为泛型时，在类实例化时会自动转换为undefined。

**变更原因：**

ArkTS-Sta中void不能作为变量类型，Promise\<void>在实例化时转换为Promise\<undefined>，因此需使用resolve(undefined)确保类型一致。reject则不需要，会与Error类型冲突。

**适配建议：**

所有Promise\<void>的resolve调用需要显式传入undefined，在异步回调中须通过箭头函数包装并明确参数。

**示例：**

ArkTS-Dyn

```typescript
let p1 = new Promise<void>((resolve, reject) => {
    resolve();
})
let p2 = new Promise<void>((resolve, reject) => {
    setTimeout(resolve, 10);
})
```

ArkTS-Sta

```typescript
let p1 = new Promise<void>((resolve, reject) => {
    resolve(undefined);
})
let p2 = new Promise<void>((resolve, reject) => {
    setTimeout(() => {resolve(undefined)}, 10);
})    
```