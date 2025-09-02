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
  public readonly x: number;
protected y: number;
private z: number;
constructor(
    x: number,
    y: number,
    z: number
  ) {
    this.x = x;
    this.y = y;
    this.z = z;
}

  foo(): void {
    console.log(this.x + this.y + this.z);
  }
}

const a = new A(1, 2, 3);
console.log(a.x);

class B {
  public f: number = 10;

  public w: string;
private readonly r: number[];
constructor(q: number, w = 'default', e: boolean, r: number[] = [1, 2, 3]) {
    this.w = w;
    this.r = r;
    console.log(q, this.w, e, this.r, this.f);
}
}

const b = new B(1, '2', true, []);
console.log(b.w);

class C {
  constructor(public a: any) {} // not fixable
}

interface GeneratedTypeLiteralInterface_1 {
    x: string;
}
class D {
  public a: number;
private b: GeneratedTypeLiteralInterface_1;
constructor(a: number, b: GeneratedTypeLiteralInterface_1) {
    this.a = a;
    this.b = b;
} // not fixable
}

class E {
  b: number = 0;
  c: number = 0;
  
  readonly a: number;
constructor(a: number) {
    this.a = a;
}
}

class F extends E {
  readonly aa: number;
b: number;
public c: number;
constructor(
    aa: number,
    b: number,
    c: number
  ){
    super(aa);
    this.aa = aa;
    this.b = b;
    this.c = c;
}
}

class F2 extends E {
  readonly aa: number;
constructor(aa: number) {
    let f2: number = 1;
    console.log('before super() call');
    super(aa);
    this.aa = aa;
}
}

class F3 extends E {
  readonly aa: number;
constructor(aa: number) {
    super(aa);
    this.aa = aa;
    let f3: number = 1;
    console.log('after super() call');
}
}

class F4 extends E {
  readonly aa: number;
constructor(aa: number) {
    let f4: number = 1;
    console.log('before super() call');
    super(aa);
    this.aa = aa;
    console.log('after super() call');
    let f5: number = 1;
}
}

class G {
  constructor(a?: number) {}
}

class G1 {
  public a?: number;
public b: number;
constructor(a?: number, b: number) {
    this.a = a;
    this.b = b;
}
}

class G2 {
  public a?: number;
public b?: number;
constructor(a?: number, b?: number) {
    this.a = a;
    this.b = b;
}
}