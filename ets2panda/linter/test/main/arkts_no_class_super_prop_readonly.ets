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
  readonly x: number = 10;
}
class B extends A {
  readonly x: number = 20; // legal
}

class C {
  readonly y: string = "base";
}
class D extends C {
  readonly y: string = "child"; // legal
}

class E {
  z: boolean = true;
}
class F extends E {
  z: boolean = false; // legal
}

class G {
  readonly foo: number = 42;
}
class H extends G {} // legal

class M1 {
  data: number = 1;
}
class M2 extends M1 {
  readonly data: number = 2; // error
}

class M3 {
  version: string = "1.0";
}
class M4 extends M3 {
  readonly version: "2.0" = "2.0"; // error
}

class M5 {
  isReady: boolean = false;
}
class M6 extends M5 {
  readonly isReady: true = true; // error
}

class A1 {
    data: number = 1;
}

class A2 extends A1 {
}

class A3 extends A2 {
    readonly data: number= 2; // error
}

class B1 {
    readonly data: number = 1;
}

class B2 extends B1 {
}

class B3 extends B2 {
    readonly data: number= 2; // legal
}

interface NonReadonly {
    v: number
}

class B4 extends NonReadonly {
    readonly v: number = 0; // error
}

interface Readonly {
    readonly v: number;
}

class B5 extends Readonly {
    readonly v: number = 0; // legal
}
