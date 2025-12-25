## 内存默认共享，不提供SharedArrayBuffer

**规则：** `arkts-no-need-stdlib-sharedArrayBuffer`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，ArrayBuffer默认共享，不需要SharedArrayBuffer。

**变更原因：**

ArkTS-Sta新增对象天然共享特性，ArrayBuffer默认支持跨线程安全共享，无需再使用SharedArrayBuffer，简化并发数据共享机制。

**适配建议：**

ArkTS-Sta使用ArrayBuffer，将ArkTS-Dyn中使用的SharedArrayBuffer改为ArrayBuffer。

**示例：**

ArkTS-Dyn

```typescript
let sab: SharedArrayBuffer = new SharedArrayBuffer(20);
let int32 = new Int32Array(sab);
```

ArkTS-Sta

```typescript
let sab: ArrayBuffer = new ArrayBuffer(20);
let int32 = new Int32Array(sab);
```