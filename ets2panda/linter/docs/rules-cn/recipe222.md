## 不支持副作用导入

**规则：** `arkts-no-side-effect-import`

**规则解释：**

ArkTS-Sta不支持副作用导入的功能。

**变更原因：**
 
ArkTS-Sta中模块加载支持懒加载，不支持副作用导入的功能。

**适配建议：**

将导入文件中的执行逻辑移到本文件中。

**示例：**

ArkTS-Dyn

```typescript
// logger.ets
console.info("Logger initialized!");

// main.ets
import "./logger";
console.info("Main program running...");
```

ArkTS-Sta

```typescript
// main.ets
console.info("Logger initialized!");
console.info("Main program running...");
```