/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class C {
  private p: number;

  p2: number;
  #p2: string; // not fixable

  private q?: string;
  private e!: string;
  private static s = 0;
  private readonly r = 20;
  private static readonly sr = 0;
  private static readonly srq?: string;

  private m(x: number): void { }

  m2(x: number): void {}
  #m2(x: number): void {} // not fixable

  m3: boolean;
  #m3(x: number): void {} // not fixable

  private get g1(): number { return 10; }
  private set s1(x: number) { }

  private static get g2(): number { return 10; }
  private static set s2(x: number) { }

  test() {
    console.log(this.p + this.#p2 + this.q + this.e + C.s + this.r + C.sr + C.srq); // '#p2' is not fixable
    this.m(10);
    this.#m2(20); // not fixable
    this.#m3(30); // not fixable
    let x = this.g1;
    this.s1 = x;
    let y = C.g2;
    C.s2 = y;
  }
}

class D extends C {
  private a: string;
  #p: number; // not fixable

  #m(): string { return 'foo'; } // not fixable

  private bar(): string { return 'baz'; }
  
  test() {
    console.log(this.#p + this.a); // '#p' is not fixable
    let x = this.#m(); // not fixable
    let y = this.bar();
  }
}

class E {
  private a: number;
  #b: string; // not fixable

  public b: number;
  constructor(b: number) {
    this.b = b;
  }
}