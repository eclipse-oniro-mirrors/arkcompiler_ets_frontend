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

class PositiveNoThisA {
  n: string = 'a'
  foo() {
  }
}

function testFuncNotUseThis() {
  let a = new PositiveNoThisA()
  const unboundFoo = a.foo
}

class PositiveBoundByAPIA {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function testBoundByAPI() {
  let a = new PositiveBoundByAPIA()
  const foo: Function = a.foo.bind(a)
  foo();
}

class PositiveBoundByArrowA {
  n: string = 'a'
  foo() {
    let m = () => {
      console.log(this.n)
    }
    m()
  }
}

function testBoundByArrowFunction() {
  let a = new PositiveBoundByArrowA()
  const foo = a.foo
}

class A {
  n: string = 'a'
  foo() {
    console.log(this.n)
  }
}

function test() {
  let a = new A()
  const foo: Function = a.foo.bind(a)
  foo();
}

@Entry
@Component
struct Index {
  build() {
  }
}

class A_2 {
  n: string = 'a'
  foo() {
    let m = () => {
      console.log(this.n)
    }
    m()
  }
}

function test_2() {
  let a = new A_2()
  const foo = a.foo
}

@Entry
@Component
struct Index_2 {
  build() {
  }
}

class TestExcludedClass {
  n: string = 'a';
  foo(): void {
    console.log(this.n);
  }
}

function testFileExclusion(): void {
  const a = new TestExcludedClass();
  const unboundFoo = a.foo; // positive - test file excluded
}

class A_4 {
  n: string = 'a'
  foo() {
  }
}

function test_4() {
  let a = new A_4()
  const unboundFoo = a.foo
}

@Entry
@Component
struct Index_4 {
  build() {
  }
}
