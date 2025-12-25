/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class A {
  constructor(
    public readonly x: number, // Declaring fields in constructor is not supported
    protected y: number, // Declaring fields in constructor is not supported
    private z: number // Declaring fields in constructor is not supported
  ) {}

  foo(): void {
    console.log(this.x + this.y + this.z);
  }
}

const a = new A(1, 2, 3);
console.log(a.x);

class B {
  public f: number = 10;

  constructor(q: number, public w = 'default', e: boolean, private readonly r: number[] = [1, 2, 3]) { // Declaring fields in constructor is not supported
    console.log(q, this.w, e, this.r, this.f);
  }
}

const b = new B(1, '2', true, []);
console.log(b.w);

class C {
  constructor(public a: any) {} // not fixable
}

class D {
  constructor(public a: number, private b: {x: string}) {} // not fixable
}

class E {
  b: number = 0;
  c: number = 0;
  
  constructor(readonly a: number) {} // Declaring fields in constructor is not supported
}

class F extends E {
  constructor(
    readonly aa: number, // Declaring fields in constructor is not supported
    override b: number, // Declaring fields in constructor is not supported
    public override c: number // Declaring fields in constructor is not supported
  ){
    super(aa);
  }
}

class F2 extends E {
  constructor(readonly aa: number) { // Declaring fields in constructor is not supported
    let f2: number = 1;
    console.log('before super() call');
    super(aa);
  }
}

class F3 extends E {
  constructor(readonly aa: number) { // Declaring fields in constructor is not supported
    super(aa);
    let f3: number = 1;
    console.log('after super() call');
  }
}

class F4 extends E {
  constructor(readonly aa: number) { // Declaring fields in constructor is not supported
    let f4: number = 1;
    console.log('before super() call');
    super(aa);
    console.log('after super() call');
    let f5: number = 1;
  }
}

class G {
  constructor(a?: number) {}
}

class G1 {
  constructor(public a?: number, public b: number) {} // Declaring fields in constructor is not supported
}

class G2 {
  constructor(public a?: number, public b?: number) {} // Declaring fields in constructor is not supported
}

// empty body, braces on same line
class H1 {
  constructor(
    readonly x: number, // Declaring fields in constructor is not supported
    readonly y: number, // Declaring fields in constructor is not supported
  ) {}
}

// empty body with comment, braces on same line
class H2 {
  constructor(
    readonly x: number, // Declaring fields in constructor is not supported
    readonly y: number, // Declaring fields in constructor is not supported
  ) { /* Comment */ }
}

// empty body, braces on different lines
class H3 {
  constructor(
    readonly x: number, // Declaring fields in constructor is not supported
    readonly y: number, // Declaring fields in constructor is not supported
  ) {
  }
}

// empty body with comments on multiple lines
class H4 {
  constructor(
    readonly x: number, // Declaring fields in constructor is not supported
    readonly y: number, // Declaring fields in constructor is not supported
  ) {
    // comment 1
    // comment 2
  }
}

// body with statements and comments
class H5 {
  constructor(
    readonly x: number, // Declaring fields in constructor is not supported
    readonly y: number, // Declaring fields in constructor is not supported
  ) {
    // comment at the beginning
    console.log('hello');
    // comment at the end
  }
}