## enum的元素不能作为类型

**规则：**`arkts-no-enum-prop-as-type`

**级别：error**

ArkTS1.1上的枚举是编译时概念，在运行时仍是普通对象。ArkTS1.2遵循静态类型，需要在运行时为enum提供类型。因此，ArkTS1.2上枚举的每个元素是枚举类的实例（在运行时才确定），无法成为编译时的静态类型信息。这与ArkTS1.2整体类型设计上不支持实例类型相违背。

**ArkTS1.1**

```typescript
enum A { E = 'A' }
function foo(a: A.E) {}
```

**ArkTS1.2**

```typescript
enum A { E = 'A' }
function foo(a: 'A') {}

// ...
enum A { E = 'A' }
function foo(a: A) {}
```
