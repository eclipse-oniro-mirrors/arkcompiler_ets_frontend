## number类型转换为具体的int/long/double类型

**规则：** `sdk-api-num2int`

**规则解释：**

在ArkTS-Dyn中，只有一种数字类型number（与JS/TS相同），该类型实际上是双精度浮点数double，整型字面量会被转化为双精度浮点数来存储。虚拟机会对整型字面量进行优化处理，但总体而言，整型操作时使用独立类型性能更优。

在ArkTS-Sta中，数字类型被区分为了多种，包括byte、char、short、int、long、float、double等；number类型因为等价于double也被保留。

ArkTS-Sta和ArkTS-Dyn在数字类型系统上存在差异。

* ArkTS-Dyn中整型字面量默认为number(double)类型，而ArkTS-Sta中为int类型。这种语义上的变化可能会导致某些表达式或者操作的结果不同。例如，ArkTS-Dyn中1/2表达式结果为0.5，ArkTS-Sta中因为是int类型字面量相除，结果是0。
* Array和Tuple等类型的index在ArkTS-Dyn中可以是任意类型，ArkTS-Sta中必须是非负的整数类型。
* 由于性能原因，ArkTS-Dyn的SDK API中使用的是number类型，但是实际上处理的是int类型操作的API。在ArkTS-Sta的SDK API中，将其入参和返回值修改为了int或者long类型。这样能大大提高SDK API的执行性能。

**变更原因：**

在ArkTS-Sta中，为提升性能与类型安全，将单一的number类型细化为多种具体数字类型。

**适配建议：**

将仅用于整型操作的变量类型改为int或long，否则在API调用处使用.toInt()等方法进行显式转换。

**示例：**

**例一：直接调用SDK API的函数或方法**

ArkTS-Dyn

```typescript
// ArkTS-Dyn API定义
class A {
  foo(a: number): number {
    return 1;
  }
}

// 应用代码
const test0: A = new A()

function test() {
  let a: number = 1;
  let b: number = 2;
  let c: number = 3;
  let d: number = 101 / b;
  c = 1.2;
  let e: number = 1.2;

  let a1: number = test0.foo(a);
  let b1: number = test0.foo(b);
  let c1: number = test0.foo(c);
  let e1: number = test0.foo(e);

  test0.foo(1);
  test0.foo(1.0);
  test0.foo(1.1);

  let x: number = a1;
  let y: number = b1 / 7;
  let z: number = c1;
  z = 1.1;
}
```

**参数场景解析：**

1. 从API foo的调用触发，变量a在其生命周期内始终作为int类型使用，因此可以将a声明为int。
2. 在处理变量b时，发现b用于除法运算并赋值给d，因此b不能声明为int。在调用foo时，需要将b转换为int。
3. 变量c被重新赋值了浮点数字面量，需要在调用处转换为int。
4. 变量e在声明处就被赋值了浮点数字面量，需要在调用处转换为int。
5. foo直接传入字面量，如果是整型字面量，无需修改，如果是1.0这样的浮点数字面量，修改为foo(1); 如果是1.1这样的字面量，修改为(1.1).toInt。
6. 对于参数的场景，不建议对number类型的参数进行修改；建议在参数传入时调用toInt或toLong。

**返回值场景解析：**

1. 变量a1用于接收返回值，始终作为int使用，然后将其赋值给变量x，x始终作为int使用，那么a1和x都应该声明int。
2. b1参与了除法操作，y的赋值表达式中有除法，它们都需要声明为number。
3. 将c1赋值给z后，z又被赋值为浮点字面量。由于int类型可以赋值给number类型变量，c1可以声明为int，但z必须保留为number。
4. 因为e1当做int使用，所以声明改为int。
5. 对于返回值的场景，因为int/long可以直接转换为number/double用，建议对返回值类型不做变化。

ArkTS-Sta

```typescript
// ArkTS-Sta API定义
class A {
  foo(a: int): int {
    return 1;
  }
}

// 应用代码
const test0: A = new A();

function test() {
  let a: int = 1;
  let b: number = 2;
  let c: number = 3;
  let d: number = 101 / b;
  c = 1.2;
  let e: number = 1.2;

  let a1: int = test0.foo(a);
  let b1: number = test0.foo(b.toInt());
  let c1: int = test0.foo(c.toInt());
  let e1: int = test0.foo(e.toInt());

  test0.foo(1);
  test0.foo(1);
  test0.foo(1.1.toInt());

  let x: int = a1;
  let y: number = b1 / 7;
  let z: number = c1;
  z = 1.1;
}
```

**例二：调用API是属性和变量的情况**

ArkTS-Dyn

```typescript
// ArkTS-Dyn API定义
class A {
    static x: number = 1;
}

// 应用代码
let a: number = A.x;
let b: number = A.x;
let c: number = A.x;
let d: number = 101 / b;
c = 1.2;
```

ArkTS-Sta

```typescript
// ArkTS-Sta API定义
class A {
    static x: int = 1;
}

// 应用代码
let a: int = A.x; 
let b: number = A.x;  // b参与除法，依然保留为number类型
let c: number = A.x;  // c被再次赋值了浮点数字面量，依然保留为number类型
let d: number = 101 / b;
c = 1.2;
```

**例三：API是interface/class属性，并且interface/class的对象使用字面量赋值**

ArkTS-Dyn

```typescript
// ArkTS-Dyn SDK API
interface A {
    a: number;
    b: number;
}

function foo(a: number): number{
    return 1;
}

// 场景1
let x0: A = {
    a: 1,
    b: 2
}

foo(x0.a)
foo(x0.b)

// 场景2
let x1: A = {
    a: 1,
    b: 1.1
}

foo(x1.a)
foo(x1.b)

// 场景3
let x2: A = {
    a: 1,
    b: 2/3
}

foo(x2.a)
foo(x2.b)

// 场景4
let x3: A = {
    a: 3 / 4,
    b: 2 / 3
}

foo(x3.a)
foo(x3.b)
```

ArkTS-Sta

```typescript
// ArkTS-Sta SDK API
interface A {
    a: number;
    b: int;
}

function foo(a: int): int{
    return 1;
}

// 场景1
let x0: A = {
    a: 1,
    b: 2
}

foo(x0.a.toInt()) // x.a为number类型，赋值给int类型的参数，需要调用toInt()
foo(x0.b) // x.b为int类型

// 场景2
let x1: A = {
    a: 1,
    b: (1.1).toInt() // 需要转化为int，但是实际上如果有这种情况，SDK API的类型不应该改为int
}

foo(x1.a.toInt()) // x.a为number类型，赋值给int类型的参数，需要调用toInt()
foo(x1.b) // x.b为int类型

// 场景3
let x2: A = {
    a: 1,
    b: 2/3 // 不用修改，因为2/3在ArkTS-Sta会自动截取为int
}

foo(x2.a.toInt()) // x.a为number类型，赋值给int类型的参数，需要调用toInt()
foo(x2.b) // x.b为int类型

// 场景4
let x3: A = {
    a: 3.0 / 4, // a的类型为number，这里需要将除数修改为浮点数字面量，否则结果会丢失精度
    b: 2 / 3 // 不用修改，因为2/3在ArkTS-Sta会自动截取为int
}

foo(x3.a.toInt()); // x.a为number类型，赋值给int类型的参数，需要调用toInt()
foo(x3.b); // x.b为int类型

// 以上是interface的场景，如果是class是一样的
```
