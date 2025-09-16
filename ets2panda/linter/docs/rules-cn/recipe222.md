## 不支持副作用导入

**规则：**`arkts-no-side-effect-import`

**级别：error**

ArkTS1.2中模块加载默认支持懒加载，无法实现导入副作用的功能。

**ArkTS1.1**

```typescript
// logger.ets
console.log("Logger initialized!");

// main.ets
import "./logger";
console.log("Main program running...");
```

**ArkTS1.2**

```typescript
// main.ets
console.log("Logger initialized!");
console.log("Main program running...");
```
