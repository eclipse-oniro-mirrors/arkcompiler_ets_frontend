## as具有运行时语义

**规则：** `arkts-no-ts-like-as`

**规则解释：**

ArkTS-Sta中`as`具有运行时语义。

**变更原因：**

ArkTS-Dyn中的`as`只在编译时提供类型信息，如果类型断言失败，报错时机取决于后续的代码操作。

ArkTS-Sta中的`as`会在运行时进行类型检查和可能的类型转换，如果类型断言失败，会立即抛出错误。

**适配建议：**

修改异常处理逻辑。

**示例：**

ArkTS-Dyn

```typescript
interface I {}
class A implements I {
  m: number = 0;
}

class B implements I {
  n: string = 'a';
}

let a: A = new A();
let i: I = a;
let t: B = i as B; // 正常编译运行
t.n.toString();    // 编译正常，运行时报错
```

ArkTS-Sta

```typescript
interface I {}
class A implements I {
    m: number = 0;
}

class B implements I {
    n: string = 'a';
}

let a: A = new A();
let i: I = a;
let t: B = i as B; // 编译正常，运行时报错
t.n.toString();
```