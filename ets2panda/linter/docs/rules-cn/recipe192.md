## 限制void类型的使用场景

**规则：** `arkts-limited-void-type`

**规则解释：**

在ArkTS-Sta中，`void`类型只能用作方法的返回类型和泛型类型，并且void类型函数的返回值不能作为值传递。

**变更原因：**

在ArkTS-Dyn中，`void`类型可用于类型声明、类型断言、函数返回类型、泛型类型等场景。

ArkTS-Sta对`void`类型的语义进行了收紧，限制其使用场景以增强类型安全性。

**适配建议：**

视场景改为undefined或不做修改，不能作为值传递。

**示例：**

- 场景1，函数返回void类型无需修改；函数返回void联合类型需要改为undefined。

  ArkTS-Dyn

  ```typescript
  function foo1(): void {};
  function foo2(): void | number {};
  ```

  ArkTS-Sta
  
  ```typescript
  function foo1(): void {};
  function foo2(): undefined | number { return undefined };
  ```

- 场景2，void类型变量和类型别名需要改为undefined。

  ArkTS-Dyn

  ```typescript
  let s1: void;  // void类型变量
  let s2: void | number;   // void联合类型
  type t1 = void;  // void类型别名
  ```

  ArkTS-Sta
  
  ```typescript
  let s1 = undefined;
  let s2: undefined | number;
  type t1 = undefined;
  ```

- 场景3，void类型断言，需要改为undefined。

  ArkTS-Dyn

  ```typescript
  let a: void | number = undefined;
  let x1 = a as void;
  let x2 = a as void | number;
  ```

  ArkTS-Sta
  
  ```typescript
  let a: undefined | number = undefined;
  let x1 = a as undefined; 
  let x2 = a as undefined | number;
  ```

- 场景4，void类型函数的返回值不能作为值传递。

  ArkTS-Dyn

  ```typescript
  function foo():void{}
  function execute(v: void) {}
  // 在参数传递过程中执行foo方法
  execute(foo());   
  ```

  ArkTS-Sta
  
  ```typescript
  function foo():void{}
  function execute(v: () => void) {
    v();
  }
  // 改为在execute内部执行foo方法 
  execute(foo);    
  ```