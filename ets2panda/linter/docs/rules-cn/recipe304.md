## namespace内方法不能重名

**规则：** `arkts-no-duplicate-function-name`

**规则解释：**

在ArkTS1.2中，相同namespace中的方法不能重名。

**变更原因：**

由于ArkTS1.2中会将名称相同的namespace合并成一个namespace，同名方法会导致冲突。

**适配建议：**

相同namespace中的方法不能重名。

**示例：**

**ArkTS1.1**

```typescript
namespace A {
  export function foo() {  // 错误：命名空间 'A' 中重复导出函数 'foo'
    console.log('test1');
  }
}

namespace A {
  export function foo() {  // 错误：命名空间 'A' 中重复导出函数 'foo'
    console.log('test2');
  }
}

```

**ArkTS1.2**

```typescript
namespace A {
  export function foo1() {  // 修改函数名称，避免命名冲突
    console.log('test1');
  }
}

namespace A {
  export function foo2() {
    console.log('test2');
  }
}
```