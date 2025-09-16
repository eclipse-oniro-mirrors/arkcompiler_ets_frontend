## 不支持[]访问对象属性

**规则：**`arkts-no-props-by-index`

**级别：error**

在ArkTS1.2中，对象结构在编译时已确定。为避免运行时出现错误和更好地提升性能，在ArkTS1.2中不能使用[]的方式动态访问object类型对象的属性。

**ArkTS1.1**

```typescript
function foo(u: object) {
  u['key'] // 违反规则
}

const person = { name: "Alice", age: 30 };
console.log(person['name']); // 违反规则

const data = JSON.parse('{ "name": "Alice" }');
console.log(data['name']); // 违反规则
```

**ArkTS1.2**

```typescript
function foo(m: Map<string, Object>) {
  m.get('key') // 使用 `Map`
}

console.log(person.name); // 直接使用 `.` 访问

interface UserData {
  name: string;
}
const data: UserData = JSON.parse('{ "name": "Alice" }');
console.log(data.name); // 直接使用点访问符
```
