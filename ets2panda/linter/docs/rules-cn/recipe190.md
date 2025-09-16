## 函数类型

**规则：**`arkts-incompatible-function-types`

**级别：error**

TypeScript允许对函数类型的变量进行更宽松的赋值，而在ArkTS1.2中，将对函数类型的赋值进行更严格的检查。函数类型转换时，参数遵循逆变(Contravariance)规则，返回类型遵循协变(Covariance)规则。

**ArkTS1.1**

```typescript
type FuncType = (p: string) => void;
let f1: FuncType =
    (p: string): number => {
        return 0
    }
let f2: FuncType = (p: any): void => {};

class Animal {}
class Dog extends Animal {}
type FuncType = () => Animal;
let f: FuncType = (): Dog => new Dog(); // 在 TypeScript 允许，但在 ArkTS 可能不允许

type FuncType2 = (dog: Dog) => void;
let f: FuncType2 = (animal: Animal) => {}; // 违反规则
```

**ArkTS1.2**

```typescript
type FuncType = (p: string) => void
let f1: FuncType =
  	(p: string) => {
        ((p: string): number => {
            return 0
        })(p) 
    }
let f2: FuncType = (p: string): void => {};

class Animal {}
class Dog extends Animal {}
type FuncType = () => Animal;
let f: FuncType = (): Animal => new Animal();// 返回 `Animal`

type FuncType2 = (dog: Dog) => void;
let f: FuncType = (dog: Dog) => {}; // 参数类型严格匹配
```
