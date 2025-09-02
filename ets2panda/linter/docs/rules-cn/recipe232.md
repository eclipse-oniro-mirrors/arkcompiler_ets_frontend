## 不支持lazy关键字

**规则：**`arkts-no-lazy-import`

**级别：error**

ArkTS1.2支持默认懒加载，无需lazy关键字。

**ArkTS1.1**

```typescript
import lazy { m } from 'module'
import lazy { a, b } from 'module1'; // 违反规则
import { c } from 'module2';
```

**ArkTS1.2**

```typescript
import { m } from 'module'
import { a, b } from 'module1'; // 移除 lazy
import { c } from 'module2';
```
