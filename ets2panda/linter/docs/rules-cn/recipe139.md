## 不支持为函数增加属性

**规则：** `arkts-no-func-props`

**规则解释：**

ArkTS-Sta不支持在函数上动态添加属性。

**变更原因：**
 
ArkTS-Sta是静态类型语言，不支持在函数，方法上动态增加属性；name属性可访问，不可修改。

**适配建议：**

使用类来封装函数和属性。

**示例：**

ArkTS-Dyn

```typescript
// 场景1，通过function关键字定义的函数
function foo(path: string): void {
  console.info(path);
}
foo.baz = 1; // ArkTS-Sta不支持function增加属性

// 场景2，箭头函数
const arrowFunc = (path: string) => {
  console.info(path);
}
arrowFunc.bar = 2; // ArkTS-Sta不支持function增加属性
```

ArkTS-Sta
```typescript
// ArkTS-Sta只能访问name属性
console.info(foo.name);
console.info(arrowFunc.name);

// ArkTS-Sta使用类封装属性
class T1 {	 
  static foo(path: string): void {	 
    console.info(path);	 
  } 

  static baz: number = 2; 
} 

T1.foo("example"); 
console.info(T1.baz);
```