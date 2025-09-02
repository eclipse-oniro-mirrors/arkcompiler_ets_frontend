## 不支持TS装饰器

**规则：**`arkts-no-ts-decorators`

**级别：error**

ArkTS1.2中不支持将类作为对象，不能通过装饰器中对类做动态改变。

**ArkTS1.1**

```typescript
function decorateKlass(target: Object) {
  console.log("decorateKlass")
}

@decorateKlass // 违反规则
class Person {
    age: number = 12
}
```

**ArkTS1.2**

```typescript
class Person {
    age: number = 12
}

class PersonHelper {
  static createPerson(): Person {
     console.log("decorateKlass")
     return new Person()
  }
}
```
