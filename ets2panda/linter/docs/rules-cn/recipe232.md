## 不支持lazy关键字

**规则：** `arkts-no-lazy-import`

**规则解释：**

ArkTS1.2不需要lazy关键字。

**变更原因：**
 
ArkTS1.2默认支持懒加载，无需使用lazy关键字。

**适配建议：**

移除lazy关键字。

**示例：**

**ArkTS1.1**

```typescript
// file1.ets
let a='a';
let b='b';
let c='c';
export {a,b,c};

// file2.ets
import lazy { a } from './file1';
import lazy { b, c } from './file1'; // 违反规则
```

**ArkTS1.2**

```typescript
// file1.ets
let a='a';
let b='b';
let c='c';
export {a,b,c};

// file2.ets
import { a } from './file1';
import { b, c } from './file1'; // 移除lazy
```