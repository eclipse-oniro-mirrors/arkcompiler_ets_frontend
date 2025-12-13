## 不支持副作用导入

**规则：** `arkts-no-side-effect-import`

**规则解释：**

ArkTS1.2不支持副作用导入的功能。

**变更原因：**
 
ArkTS1.2中模块加载支持懒加载，不支持副作用导入的功能。

**适配建议：**

将导入文件中的执行逻辑移到本文件中。

**示例：**

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