## Object Property Access via [] Not Supported

**Rule:** `arkts-no-props-by-index`

**Severity: error**

In ArkTS1.2, object structures are determined at compile time. To avoid runtime errors and improve performance, dynamic property access via [] is not allowed for objects of type

**ArkTS1.1**

```typescript
function foo(u: object) {
  u['key'] // Violates the rule
}

const person = { name: "Alice", age: 30 };
console.log(person['name']); // Violates the rule

const data = JSON.parse('{ "name": "Alice" }');
console.log(data['name']); // Violates the rule
```

**ArkTS1.2**

```typescript
function foo(m: Map<string, Object>) {
  m.get('key') // Use `Map`
}

console.log(person.name); // Use `.` for direct access

interface UserData {
  name: string;
}
const data: UserData = JSON.parse('{ "name": "Alice" }');
console.log(data.name); // Use dot notation
```