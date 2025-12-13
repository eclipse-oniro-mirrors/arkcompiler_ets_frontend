## enum的元素不能作为类型

**规则：** `arkts-no-enum-prop-as-type`

**规则解释：**

ArkTS1.2中enum（枚举）的元素不能作为类型使用。

**变更原因：**

ArkTS1.1中的枚举是编译时概念，在运行时仍是普通对象。

ArkTS1.2中枚举的每个元素是枚举类的实例，无法作为类型使用。

**适配建议：**

使用枚举类型/字符串字面量类型。

**示例：**

**ArkTS1.1**

```typescript
enum A { E = 'A' }
function foo(a: A.E) {}
```

**ArkTS1.2**

```typescript
enum A { E = 'A' }
function foo1(a: 'A') { }
function foo2(a: A) { }
```