## class的懒加载

**规则：** `arkts-class-lazy-import`

**规则解释：**

ArkTS1.2的类默认是懒加载的。

**变更原因：**

ArkTS1.2的类默认是懒加载的，这可以提升启动性能并减少内存占用。

**适配建议：**

将类中未执行的初始化逻辑移到外部。

**示例：**

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