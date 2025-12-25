## 不支持类TS重载

**规则：** `arkts-no-ts-overload`

**规则解释：**

ArkTS-Sta不支持TS-like的重载。

**变更原因：**
 
重载时使用不同的函数体可以提高执行效率。

**适配建议：**

重载时分别使用不同的函数体。

**示例：**

ArkTS-Dyn

```typescript
function foo(): void;

function foo(x: string): void;

function foo(x?: string): void {
  console.info(x);
}

class A {
  public foo(): void;

  public foo(x: string): void;  

  public foo(x?: string): void {  
    console.info(x);
  }
}
```

ArkTS-Sta

```typescript
function foo(x?: string): void {  // ArkTS-Sta中声明与实现一一对应，删除多余声明
  console.info(x);
}

class A {
  public foo(x?: string): void {  // ArkTS-Sta中声明与实现一一对应，删除多余声明
    console.info(x);
  }
}
```