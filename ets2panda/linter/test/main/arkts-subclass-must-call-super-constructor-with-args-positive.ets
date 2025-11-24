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

class A {
  constructor(a: number) {
  }
}

class B {
  constructor() {
  }
}

class D extends B {} // NO ERROR

class E extends A { // NO ERROR
  constructor() {
    super(1);
  }
}

class F extends E {} // NO ERROR

class G extends A { // NO ERROR
  constructor(a:number) {
    super(a);
  }
}

class EpicFoo extends Error { // NO ERROR
  constructor() {
    super()
  }
}

class EpicBar extends Error { // NO ERROR
  constructor() {
    super("foo")
  }
}

export enum EEE {
  E1,
  E2,
}

export class AA {
  protected _e:EEE
  constructor(e: EEE) {
    this._e = e;
  }
}

class BB extends AA { // NO ERROR
  constructor() {
    super(EEE.E1)
  }
}

class A3 {}

class C3 extends A3 { // NO ERROR
  constructor() {
    super()
  }
}

class C4 extends A3 { // NO ERROR
}


class BaseWithGeneric<T> {
  item: T;
  constructor(param: T) {
    this.item = param;
  }
}

class Test {
  public a: string = "";
}

class X extends BaseWithGeneric<Test> { // NO ERROR
  constructor() {
    super(new Test());
  };
}

interface I{

}

class A9{
  constructor(i:I) {

  }
}

class B9 extends A9 { // NO ERROR
  constructor() {
    super({ })
  }
}

class C9 extends A9 { // NO ERROR
  constructor() {
    super(1)
  }
}

// Builtin and SDK Object should not be considered
class MyError extends Error {}

class MyPromise<T> extends Promise<T> {}

class MyMap<K, V> extends Map<K, V> {}

class MyDate extends Date {}

class MyRegExp extends RegExp {}

class MyFunction extends Function {}

class MyNumber extends Number {}

class MyString extends String {}

class AB extends ArrayBuffer {}

class AB9{
  constructor(i?:number) {

  }
}

class BB9 extends AB9 { // no err
}

class AB10{
  constructor(i:number = 1) {

  }
}

class BB10 extends AB10 { // no err
}
