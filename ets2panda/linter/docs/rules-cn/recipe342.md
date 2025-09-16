### ArkTS1.2调用js方法和传参

**规则：** arkts-interop-js2s-call-js-method

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
class Foo {
  bar(a) {}
}
export let foo = new Foo();
// file2.ets
import { foo } from './file1';
foo.bar(123);
```

**ArkTS1.2**
```typescript
// file1.js
class Foo {
  bar(a) {}
}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
foo.invokeMethod('bar', ESValue.wrap(123));
```
