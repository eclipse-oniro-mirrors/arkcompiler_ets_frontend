## Promise\<void>构造器中只支持使用resolve(undefined)

**规则：** `arkts-promise-with-void-type-need-undefined-as-resolve-arg`

ArkTS1.2中void不能作为变量类型声明；作为泛型时，在类实例化时会自动转换为undefined。
**ArkTS1.1**
```typescript
let p1 = new Promise<void>((resolve, reject) => {
    resolve();
})
let p2 = new Promise<void>((resolve, reject) => {
    setTimeout(resolve, 10);
})
```

**ArkTS1.2**
```typescript
let p1 = new Promise<void>((resolve, reject) => {
    resolve();
})
let p2 = new Promise<void>((resolve, reject) => {
    setTimeout(() => {resolve(undefined)}, 10);
})    
```