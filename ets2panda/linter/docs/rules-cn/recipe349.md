## 内存默认共享，不提供SharedArrayBuffer

**规则：** arkts-no-need-stdlib-sharedArrayBuffer

**级别：** error

新增对象天然共享特性，ArrayBuffer默认共享，不需要SharedArrayBuffer。

**ArkTS1.1**
```typescript
let sab: SharedArrayBuffer = new SharedArrayBuffer(20);
let int32 = new Int32Array(sab);
```

**ArkTS1.2**
```typescript
let sab: ArrayBuffer = new ArrayBuffer(20);
let int32 = new Int32Array(sab);
```
