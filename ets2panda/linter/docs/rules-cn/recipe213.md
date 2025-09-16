## class的懒加载

**规则：**`arkts-class-lazy-import`

**级别：error**

ArkTS1.2的类在使用时进行加载或初始化，以提升启动性能，减少内存占用。

**ArkTS1.1**

```typescript
class C {
  static {
    console.info('init');  // ArkTS1.2上不会立即执行
  }
}
```

**ArkTS1.2**

```typescript
// ArkTS1.2  如果依赖没有被使用的class执行逻辑，那么将该段逻辑移出class
class C {
  static {}
}
console.info('init');
```
