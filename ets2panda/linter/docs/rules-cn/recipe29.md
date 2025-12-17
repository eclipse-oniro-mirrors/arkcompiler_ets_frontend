## 不支持[]访问对象属性

**规则：** `arkts-no-props-by-index`

**规则解释：**

不能使用[]的方式动态访问object类型对象的属性。

**变更原因：**
 
在ArkTS-Sta中，对象结构在编译时已确定。为避免运行时错误并提升性能，不能使用[]方式动态访问object类型对象的属性。

**适配建议：**

使用点访问符代替[]。

**示例：**

ArkTS-Dyn

```typescript
interface Person {
  name: string;
  age: number;
}

function foo(u: object) {
  u['key'];
}

const person: Person = { name: "Alice", age: 30 };
console.info((person as object)['name']);

const data: object = JSON.parse('{ "name": "Alice" }');
console.info(data['name']);
```

ArkTS-Sta

```typescript
interface Person {
  name: string;
  age: number;
}

function foo(m: Map<string, Object>) {
  m.get('key'); // 使用 `Map`
}

const person: Person = {name: 'John',age: 30};
console.info(person.name); // 直接使用 `.` 访问

class UserData {
  name?: string;
}
const data =  JSON.parse<UserData>('{ "name": "Alice" }', Type.from<UserData>())!;
console.info(data.name); // 直接使用点访问符
```