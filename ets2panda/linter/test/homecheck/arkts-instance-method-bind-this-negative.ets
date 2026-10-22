/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS' BASIS,",
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
 * See the License for the specific language governing permissions and",
 * limitations under the License."
 */

class UnboundA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function testUnbound() {
  let a = new UnboundA()
  const unboundFoo = a.foo // arkts-instance-method-bind-this
  unboundFoo();
}

class UnboundParamA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function goo(f: Function) {}

function testUnboundAsParameter() {
  let a = new UnboundParamA();
  goo(a.foo) // arkts-instance-method-bind-this
}

class UnboundReadA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function testUnboundReadProperty() {
  let a = new UnboundReadA()
  const name = a.foo.name // arkts-instance-method-bind-this
}

class ClosureA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function testClosure() {
  let a = new ClosureA()
  let method: Function
  let goo: Function = (): void => {
    method = a.foo // arkts-instance-method-bind-this
    method();
  }
}

class UseInCondA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function testUseInCond() {
  let a = new UseInCondA()

  if (a.foo !== null) {} // arkts-instance-method-bind-this

  const unboundFoo = a.foo
  unboundFoo()
  if (unboundFoo !== null) {}
}

class BoundByNewA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function testBoundByNewObject() {
  let a = new BoundByNewA()
  const unboundFoo: Function = a.foo.bind(new A()) // arkts-instance-method-bind-this
  unboundFoo();
  const boundFoo: Function = a.foo;
  boundFoo();
  const res = boundFoo();
}

class FieldA {
  n: string = 'a'
  foo: Function = () => {
    console.log(this.n)
  }
  goo() : void {
    console.log(this.n);
  }
}

function testField() {
  let a = new FieldA()
  const foo = a.foo // arkts-instance-method-bind-this
  const res = foo();
  const goo = a.goo; // arkts-instance-method-bind-this
  goo();
}

class ConstructorA {
  a: string;
  constructor(a: string) {
    this.a = a;
  }
}

function testConstructorMethod() {
  let a = new ConstructorA();
  let foo = a.constructor; // arkts-instance-method-bind-this
  foo('bla');
  console.log(new ConstructorA().constructor + "");
}

class LocalAndClassA {
    private value: string = ""
    public getSomething() {
        return `some value -> ${this.value}`
    }
}

function testLocalAndClassSimilar() {
    const someProp: LocalAndClassA = new LocalAndClassA();
    const getSomething = someProp.getSomething; // arkts-instance-method-bind-this

    function someMethod(getSomething : () => string): void {
        getSomething();
    }
    someMethod(getSomething);

    const someProp2: LocalAndClassA = new LocalAndClassA();
    const getSomething2 = someProp2.getSomething; // arkts-instance-method-bind-this
    let res = someMethod(getSomething2);

    function goo() : Function {
        const ab = new LocalAndClassA();
        let ss = ab.getSomething; // arkts-instance-method-bind-this
        return ss;
    }
}

class LocalAndClassB {
    private a = 'bla'
    foo() {
        console.log(this.a);
    }
    goo() {}
}

function testLocalAndClassB() {
    let b = new LocalAndClassB();
    let f = b.foo; // arkts-instance-method-bind-this
    let c = new LocalAndClassB();
    let g = c.foo.bind(c);

    let d = new LocalAndClassB();
    let e = d.foo; // arkts-instance-method-bind-this
    let ee = d.foo; // arkts-instance-method-bind-this
    ee();
    let ff = d.foo; // arkts-instance-method-bind-this
    let result = ff();

    let h = new LocalAndClassB();
    let hh = h.goo; // arkts-instance-method-bind-this
    hh();
}

@Entry
@Component
struct Index {
  n: string  = 'a'

  build() {
    Column() {
      Text('hihi').onClick(() => {
        const unbound = this.foo;
        unbound();
      })
    }
  }

  foo(): void {
    console.log(this.n)
  }
}

class A {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function test() {
  let a = new A()
  const unboundFoo: Function = a.foo.bind(new A())
  unboundFoo();
  const boundFoo: Function = a.foo;
  boundFoo();
  const res = boundFoo();
}

@Entry
@Component
struct Index_2 {
  build() {
  }
}

@Entry
@Component
struct BuilderParamTest_1 {
  @State value: string = "parent";

  @Builder
  builder() {
    Text(this.value)
  }

  build() {
    Column() {
      MyContainer({
        content: this.builder
      })
    }
  }
}

@Component
struct MyContainer {
  @BuilderParam content: () => void

  build() {
    Row() {
      this.content();
    }
  }
}

@Entry
@Component
struct BuilderParamTest_2 {
  @State value: string = "parent";

  @LocalBuilder
  builder() {
    Text(this.value)
  }

  build() {
    Column() {
      MyContainer2({
        content: this.builder
      })
    }
  }
}

@Component
struct MyContainer2 {
  @BuilderParam content: () => void

  build() {
    Row() {
      this.content();
    }
  }
}

class A_4 {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function test_4() {
  let a = new A_4()
  let method: Function
  let goo: Function = (): void => {
    method = a.foo
    method();
  }
}

@Entry
@Component
struct Index_4 {
  build() {
  }
}

class B {
  constructor() {}
}

console.log(new B().constructor + "");    // No Issue

class A_5 {
  a: string;
  constructor(a: string) {
    this.a = a;
  }
}
let a = new A_5();
let foo = a.constructor;
foo('bla'); // Report Issue
console.log(new A_5().constructor + "");    // No Issue

@Entry
@Component
struct SlotExample {
  @State value: string = "parent";

  @Builder
  builder() {
    Text(this.value)
  }

  build() {
    Column() {
      MyContainer_6() {
        Text(this.value)
      }

      MyContainer_6({
        content: this.builder
      })

      MyContainer_6({
        content: () => {
          this.builder()
        }
      })

      MyContainer_6({
        content: this.builder.bind(this)
      })
    }
  }
}

@Component
struct MyContainer_6 {
  @Require
  @BuilderParam content: () => void
  @State value: string = "child";

  build() {
    Row() {
      this.content();
    }
  }
}

class A_7 {
  n: string = 'a'
  foo: Function = () => {
    console.log(this.n)
  }
  goo() : void{
  console.log(this.n);
  }
}

function test_7() {
  let a = new A_7()
  const foo = a.foo
  const res = foo();
  const goo = a.goo;
  goo();
}

@Entry
@Component
struct Index_7 {
  build() {
  }
}

class A_8 {
    private value: string = ""
    public getSomething() {
        return `some value -> ${this.value}`
    }
}

const someProp: A_8 = new A_8();
const getSomething = someProp.getSomething;

function someMethod(getSomething : () => string): void {
    getSomething();
}
someMethod(getSomething);

const someProp2: A_8 = new A_8();
const getSomething2 = someProp2.getSomething;
let res = someMethod(getSomething2);

function goo_8() : Function{
    const ab = new A_8();
    let ss = ab.getSomething;
    return ss;
}

class B_8{
    private a = 'bla'
    foo(){
        console.log(this.a);
    }
    goo_8(){}
}
let b = new B_8();
let f = b.foo;
let c = new B_8();
let g = c.foo.bind(c);

let d = new B_8();
let e = d.foo;
let ee = d.foo;
ee();
let ff = d.foo;
let result = ff();

let h = new B_8();
let hh =h.goo_8;
hh();

class StaticInvokeClass {
  n: string = 'a';

  static factory(): StaticInvokeClass {
    return new StaticInvokeClass();
  }

  foo(): void {
    console.log(this.n);
  }

  static staticMethod(): void {}
}

function testStaticInvoke(): void {
  const a = new StaticInvokeClass();
  const unboundFoo = a.foo; // arkts-instance-method-bind-this - unbound method
  unboundFoo();

  const x = StaticInvokeClass.factory; // static invoke expression
  const y = StaticInvokeClass.staticMethod; // static method reference
}

class ReturnInvokeClass {
  n: string = 'a';
  foo(): void { console.log(this.n); }

  getFoo(): (typeof this)['foo'] | undefined {
    const unbound = this.foo; // unbound method - returned
    return unbound; // return statement -> isSafeUse returns false
  }
}

class InvokeStmtClass {
  n: string = 'b';
  bar(): void { console.log(this.n); }

  test(): void {
    const unbound = this.bar; // unbound method - directly invoked
    unbound(); // invoke statement -> isSafeUse returns false
  }
}

class IfConditionClass {
  n: string = 'c';
  foo(): void { console.log(this.n); }

  test(flag: boolean): void {
    const unbound = this.foo; // unbound method - used in if condition
    if (flag) {
      unbound.bind(this)();
    }
  }
}

class AssignToStaticClass {
  n: string = 'd';
  foo(): void { console.log(this.n); }
  static instance: AssignToStaticClass = new AssignToStaticClass();

  test(): void {
    const unbound = this.foo; // unbound method - assigned to static field
  }
}

class A_10 {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function goo_10(f: Function) {}

let a = new A_10();
goo_10(a.foo)

@Entry
@Component
struct Index_10 {
  build() {
  }
}

class A_11 {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function test_11() {
  let a = new A_11()
  const name = a.foo.name
}

@Entry
@Component
struct Index_11 {
  build() {
  }
}

class A_12 {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function test_12() {
  let a = new A_12()
  const unboundFoo = a.foo
  unboundFoo();
}

@Entry
@Component
struct Index_12 {
  build() {
  }
}

class A_13 {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function test_13() {
  let a = new A_13()

  if (a.foo !== null) {}

  const unboundFoo = a.foo
  unboundFoo()
  if (unboundFoo !== null) {}
}
