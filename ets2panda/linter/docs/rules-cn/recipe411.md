## instanceof的目标类型不能是函数

**规则：** `arkts-no-instanceof-func`

**规则解释：**

ArkTS1.2中instanceof的目标类型不能是函数。

**变更原因：**

ArkTS1.2不再基于原型实现继承，没有原型或构造函数的概念，不能通过任意函数创建对象。

**适配建议：**

请将instanceof的目标修改为类型。

**示例：**

**ArkTS1.1**

```typescript
function foo() {}
function bar(obj: Object) {
  console.info('obj instanceof foo :' ,obj instanceof foo);
}
```

**ArkTS1.2**
```typescript
function bar(obj: Object) {
console.info('obj instanceof foo :', obj instanceof string);
}
```