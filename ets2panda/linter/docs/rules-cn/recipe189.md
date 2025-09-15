## 数值类型语义变化

**规则：** `arkts-numeric-semantic`

**规则解释：**

在ArkTS1.2中，整型数字字面量默认是int类型，以提高执行效率。

**变更原因：**

在ArkTS1.1中只有一个数字基础类型number，不区分整型字面量或是浮点型字面量。

在ArkTS1.2中，整型数字字面量默认是int类型，以提高执行效率。

**适配建议：**

在表达式中，若涉及除法操作或其他数值类型推导为number的情况，需将整型字面量显式修改为浮点型字面量，避免类型错误。

**示例：**

- 场景1，适用于直接给变量赋值的情况。如果该变量未曾作为number类型使用，则将传值的变量链上的声明都改为int，字面量保留为整型。否则，变量声明为number，字面量仍保留为整型。

- 场景2，对于字面量参与表达式给变量赋值的情况，需根据表达式具体情况处理。如果表达式中包含除法运算，则应将字面量转换为浮点类型，变量类型为number。用户自定义函数（async函数除外）直接赋值时，仅返回number类型，无需修改。SDK API可能返回int类型，但按第二节返回值场景处理，但由于SDK已处理，此规则无需再执行。

- 场景3，对于变量或常量被导出的情况，应将其声明为number类型。其他模块默认将变量视为number类型使用，如果将其更改为int类型，可能会导致调用该变量或常量的模块出现编译错误。

- 场景4，所有整型字面量在包含除法的表达式中，应修改为浮点数字面量。无论变量是数组、tuple还是其他容器中的一个值，表达式所赋值的变量都应保留为number类型。
   ```typescript
   // 公共代码
   function foo(a:number): number {
     return a - 1;
   }
   // ArkTS1.1
   let a = 1;
   let b = a;
   let c = 2;
   let d = 3;
   let e = foo(1); // foo的返回值为number，这里foo是开发者自定义函数，不是SDK函数。e要声明为number，因为类型推导会把e推导为number，所以不用改
   let f:number = 1; // f虽然被声明为number，但是上下文中一直作为int使用，那么应该将f的类型修改为int
   export let g:number = 1; // g被export，需要保留为number

   c = 1/2; // 变量c的赋值表达式里有除法，c需要声明为number类型，且表达式中的整型字面量也要修改为浮点型字面量
   d = 1.0; // 变量d又被赋值了浮点字面量，所以需要声明为number类型

   // ArkTS1.2
   let a:int = 1;
   let b:int = a;
   let c:number = 2;
   let d:number = 3;
   let e:number = foo(1);
   let f:int = 1;
   export let g:number = 1;

   c = 1.0/2.0;
   d = 1.0;
   ```

- 场景5，tuple的情况。<br>
   - 不要改number类型，一律保留。
   - 赋值的情况，如果是整型字面量，不需要修改；如果表达式包含整型字面量和除法操作，需将整型字面量修改为浮点数字面量。
   ```typescript
   // ArkTS1.1
   let a : [number, number, boolean] = [1, 1, true];
   a = [2, 2, false];
   a = [2/3, 3/4, false];

   // ArkTS1.2
   let a : [number, number, boolean] = [1, 1, true];
   a = [2, 2, false];
   a = [2.0/3.0, 3.0/4.0, false]; // 将整型字面量修改成浮点型字面量，否则结果和ArkTS1.1不一致
   ```

- 场景6，Array的情况。<br>
   - 不要更改number类型，一律保留。
   - 赋值的情况，如果是整型字面量，则不需要修改。但是如果是包含整型字面量和除法操作的表达式，则需要将整型字面量修改为浮点数字面量。
   ```typescript
   // ArkTS1.1
   let arr:number[] = [1, 2, 3];
   arr = [2, 3, 4];
   arr[0] = 2/3;
   arr = [1/3, 2/3, 4];

   // ArkTS1.2
   let arr:number[] = [1, 2, 3];
   arr = [2, 3, 4];
   arr[0] = 2.0/3.0;
   arr = [1.0/3.0, 2.0/3.0, 4];
   ```

- 场景7，Array及相关操作符的使用情况。
   ```typescript
   // ArkTS1.1
   let arr:Array<number> = [1, 2, 3, 4];
   let arr2:Array<number> = arr??[0];
   let arr3:Array<number> = [1, 2, 3, 4];

   // ArkTS1.2
   // 在ArkTS1.2中，字面量数组的类型会根据上下文推导。例如，下面的例子中，arr被赋值的字面量数组[1, 2, 3, 4]
   // 因为arr的声明为Array<number>，所以字面量数组推导为number数组，并可以赋值给arr。
   // 数组字面量[1, 2, 3, 4]被赋值给arr3，因为arr3的声明为Array<int>，所以该字面量数组被推导为int数组，并可以被赋值给arr3
   // 当上下文无法推导时，根据数组元素类型进行推导。例如，arr??[0]中的[0]被推导为int数组，这样在将值赋给arr2时会导致错误，因为arr是number数组
   // arr??[0]的类型是Array<number> | Array<int>的联合类型，与arr2的声明类型不符，因此会导致编译错误，需要将0改为0.0
   let arr:Array<number> = [1, 2, 3, 4];
   let arr2:Array<number> = arr??[0.0]; // 将数组中的整型字面量修改为浮点型字面量
   let arr3:Array<int> = [1, 2, 3, 4];

   // 同理三元运算符的情况
   // ArkTS1.1
   let b:boolean = true;
   let arr:number[] = [1, 2, 3];
   let arr1:number[] = b? arr : [0, 1, 2];

   //ArkTS1.2
   let b:boolean = true;
   let arr:number[] = [1, 2, 3];
   let arr1:number[] = b? arr : [0.0, 1.0, 2.0];
   let arr2:int[] = [1, 2, 3];
   ```

- 场景8，Enum中的整型字面量无需修复。
   ```typescript
   enum A {
     A1 = 1,
     A2 = 2,
     A3 = 3
   } // 都不要修复成浮点型字面量
   ```

- 场景9，字面量中的整型字面量，不论是ArkTS1.1还是ArkTS1.2，字面量在赋值时，变量的类型决定了数字字面量的修改方式。此外，SDK API的number类型转换为int已在上一章示例中展示，本章将展示开发者自定义的类型（如class或interface）。
   ```typescript
   // 场景9.1类型属性不需要修改number to int的情况
   // 公共代码
   interface A {
     a : number;
     b : number;
   }
   // ArkTS1.1用户自定义类型
   let x:A = {
     a : 1,
     b : 2
   }

   let y:A = {
     a : 1,
     b : 1.1
   }

   let z:A = {
     a : 1,
     b : 2/3
   }

   // ArkTS1.2用户自定义类型
   // 不用修改
   let x:A = {
     a : 1,
     b : 2
   }
   // 不用修改
   let y:A = {
     a : 1,
     b : 1.1
   }

   let z:A = {
     a : 1,
     b : 2.0/3 //需要修改
   }

   // 场景9.2类型属性仍保留为number，但是需要作为int使用，需要调用toInt
   // 公共代码
   interface A {
     a : number;
     b : number;
     c : number;
   }
   let arr:Array<number> = [1, 2, 3, 4, 5]

   // ArkTS1.1用户自定义类型
   let x:A = {
     a : 1.1, // a被赋值为浮点字面量，所以，属性a类型认为number
     b : 2/3, // b被含除法操作的表达式赋值，所以b还需要作为number使用，且表达式的整型字面量需要改为浮点型字面量
     c : 3
   }
   arr[x.a];
   arr[x.b];
   x.c/2; // c参与除法操作，所以仍要为number

   // 如果该属性被用作索引或其他需要整数的情况（如SDK的入参），且该属性没有在其他地方被用作数字（例如参与除法操作或被浮点数字面量赋值），则应将属性类型修改为整数。
   // ArkTS1.2用户自定义类型
   let x:A = {
     a : 1.1,
     b : 2.0/3,
     c : 3
   }

   arr[x.a.toInt()]; // Array的index必须是整数
   arr[x.b.toInt()];
   x.c/2;
   ```

- 场景10，async函数、方法和lambda表达式必须返回Promise\<T>类型的值。如果函数体返回T，编译器会自动推导返回值为Promise\<T>。因此，当async函数返回整型字面量时，ArkTS1.1版本返回Promise\<number>，而ArkTS1.2版本返回Promise\<int>，两者在ArkTS1.2中不兼容。需要将async函数中返回整型字面量的情况修改为返回浮点型字面量。
   ```typescript
   // ArkTS1.1
   async function foo() {
     return 1; // 在ArkTS1.1中返回值类型是Promise<number>，在ArkTS1.2中返回值类型是Promise<int>
   }

   async function foo1() : Promise<number>{
     return 1; // 在ArkTS1.1中返回值类型是Promise<number>，在ArkTS1.2中返回值类型是Promise<int>
   }

   let func1 = async ()=> {return 1}; // 在ArkTS1.1中返回值类型是Promise<number>，在ArkTS1.2中返回值类型是Promise<int>
   let func2 = async ():Promise<number> => {return 1}; // 在ArkTS1.1中返回值类型是Promise<number>，在ArkTS1.2中返回值类型是Promise<int>

   class A {
     async method1() {
       return 1;
     }
     async method2():Promise<number> {
       return 1;
     }
   }

   // ArkTS1.2
   async function foo() {
     return 1.0;
   }

   async function foo1() : Promise<number>{
     return 1.0;
   }

   let func1 = async ()=> {return 1.0};
   let func2 = async ():Promise<number> => {return 1.0};

   class A {
     async method1() {
       return 1.0;
     }
     async method2():Promise<number> {
       return 1.0;
     }
   }
   ```

- 场景11，在包含除法的复杂运算表达式中，表达式的结果应为number或double类型。
   ```typescript
   // ArkTS1.1
   let a = (1+1)/(2*3);

   // ArkTS1.2
   let a:number = (1.0+1)/(2*3); // 表达式的第一个字面量如果为整型字面量，修改为浮点型字面量
   ```

- 场景12，lambda表达式的返回值场景，对于lambda表达式的返回值，在ArkTS1.2中，整型字面量是int类型，int类型可赋值给number类型。lambda表达式返回int类型，与number类型协变，符合ArkTS1.2语法规则。
   ```typescript
   // 公共代码
   let func1 = () => {return 1}; // 不告警
   let func2 = () => {return 2}; // 不告警
   // ArkTS1.1
   let r1 = func1()/func2(); // 在ArkTS1.2中，参与了除法，结果为number/double，这里需要调用toDouble函数。
   // ArkTS1.2
   let r1 = func1().toDouble()/func2();
   ```

- 场景13，enum的整型值参与除法操作。
   ```typescript
   // 公共代码
   enum X {
     A = 1,
     B = 2,
     C = -1
   }

   // ArkTS1.1
   let a = X.A/X.B;// ArkTS1.1结果为0.5，ArkTS1.2结果为0
   // ArkTS1.2
   let a = X.A.valueOf().toDouble() / X.B; // 先取到X.A的值，然后再转换为number/double
   ```