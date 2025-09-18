## 继承/实现方法时参数遵循逆变原则，返回类型遵循协变原则

**规则：** `arkts-method-inherit-rule`

**规则解释：**

ArkTS1.1与ArkTS1.2在继承/实现方法时遵循以下规则。有关逆变和协变的详细解释，请参见[逆变和协变](#逆变和协变)。
|  类型位置 &nbsp;&nbsp;  |  ArkTS1.1规则   | ArkTS1.2规则  | 
|  ----  |  ----  | ----  |
| 参数类型 | 逆变&协变  | 逆变 |
| 返回类型 | 协变  | 协变 |

**变更原因：**

参数类型逆变，可以通过编译时检查保证类型安全，提前发现潜在错误，避免运行时失败，提升执行性能。

返回类型协变，可以确保调用方按父类声明操作返回值时，所有父类声明的属性和方法必然存在。这样可以避免运行时出现属性或方法缺失的情况。

**适配建议：**

根据规则修改参数类型协变的代码。

**示例：**

**ArkTS1.1**

```typescript
class A { u = 0 }
class B { v = 0 }
class Father {
  fun1(a: A | B) { }
  fun2(a: A) { }
  fun3(): A | B { return new A() }
  fun4(x: A) { }
}
class Son extends Father {
  // 方法参数类型：协变
  override fun1(a: A) { }
  // 方法参数类型：逆变
  override fun2(a: A | B) { }
  // 方法返回类型：协变
  override fun3(): A { return new A() }
  // 父类是普通函数，子类是异步函数
  override async fun4(x: A | B) {
    await new Promise<void>(() => {});
  }
}
```

**ArkTS1.2**

```typescript
class A { u = 0 }
class B { v = 0 }
class Father {
  fun1(a: A) { }
  fun2(): A | B { return new A() }
  fun3(x: A) { }
}
class Son extends Father {
  // 方法参数类型：逆变
  override fun1(a: A | B) { }
  // 方法返回类型：协变
  override fun2(): A { return new A() }
  // 父类是普通函数，子类是异步函数，需要改为普通函数，将异步的部分抽取出来
  override fun3(x: A | B) {
    this.asyncFunc();
  }
  async asyncFunc() {
    await new Promise<void>(() => {});
  }
}
```