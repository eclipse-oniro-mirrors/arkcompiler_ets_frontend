## namespace内方法不能重名

**规则：**`arkts-no-duplicate-function-name`

**级别：error**

由于ArkTS1.2中会将多个名称相同的namespace合并成一个namespace，所以namespace内方法不能重名，否则会导致冲突。

**ArkTS1.1**

```typescript
namespace A {
  export function foo() {  // 错误：命名空间 'A' 中重复导出函数 'foo'.
    console.log('test1');
  }
}

namespace A {
  export function foo() {  // 错误：命名空间 'A' 中重复导出函数 'foo'.
    console.log('test2');
  }
}

```

**ArkTS1.2**

```typescript
namespace A {
  export function foo1() {  // 重命名导出函数
    console.log('test1');
  }
}

namespace A {
  export function foo2() {
    console.log('test2');
  }
}
```
