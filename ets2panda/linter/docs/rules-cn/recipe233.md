## 不支持动态import

**规则：**`arkts-no-dynamic-import`

**级别：error**

ArkTS1.2中模块加载默认支持懒加载。

**ArkTS1.1**

```typescript
function main(): void {
  import('./file').then((m) => {
    console.log(m.Data.name)
  })
}

document.getElementById("btn")?.addEventListener("click", async () => {
  const module = await import('./utils');  // 错误: 在ArkTS中动态`import()`是不支持的.
  module.doSomething();
});

function getModule() {
  return import('./heavyModule')  // 错误: 在ArkTS中动态`import()`是不支持的.
    .then((m) => m.default);
}
```

**ArkTS1.2**

```typescript
import { Data } from './file'
import { doSomething } from './utils';  // 静态import是可以的.
import heavyModule from './heavyModule';  // 静态import是可以的.

function main(): void {
  console.log(Data.name)
}

document.getElementById("btn")?.addEventListener("click", () => {
  doSomething();
});

function getModule() {
  return heavyModule;
}
```
