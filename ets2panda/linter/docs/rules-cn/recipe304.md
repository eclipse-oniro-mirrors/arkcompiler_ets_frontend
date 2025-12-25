## namespace内方法不能重名

**规则：** `arkts-no-duplicate-function-name`

**规则解释：**

在ArkTS-Sta中，相同namespace中的方法不能重名。

**变更原因：**

由于ArkTS-Sta中会将名称相同的namespace合并成一个namespace，同名方法会导致冲突。

**适配建议：**

相同namespace中的方法不能重名。

**示例：**

ArkTS-Dyn

```typescript
namespace A {
  function foo() {
    console.info('test1');
  }
}

namespace A {
  function foo() {
    console.info('test2');
  }
}
```

ArkTS-Sta

```typescript
namespace A {
  function foo1() {
    console.info('test1');
  }
}

namespace A {
  function foo2() {
    console.info('test2'); // 修改函数名称，避免命名冲突
  }
}
```