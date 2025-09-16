### ArkTS1.2处理TS非常规异常

**规则：** arkts-interop-ts2s-ts-exception

**级别：** error

**ArkTS1.1**
```typescript
// file1.ts
export function foo() {
  throw 123;
}

// file2.ets
import { foo } from './file1';

try {
  foo();
} catch (e) {
  console.log("result is " + (e as number)); // 123
}
```

**ArkTS1.2**
```typescript
// file1.ts
export function foo() {
  throw 123;
}

// file2.ets  // ArkTS1.2
'use static'
import { foo } from './file1';

try {
  foo();
} catch (e) {
  let err: ESValue = (e as ESError).getValue();
  err.toNumber(); // 123
}
```
