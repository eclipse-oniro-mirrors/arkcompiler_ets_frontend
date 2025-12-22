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
  adminMethod(): void {}
}

class B {
  adminMethod(): void {}
}

function getBaseClass(isAdmin: boolean) {
  return isAdmin ? A : B;
}

class User extends getBaseClass(false) { // expression in extends
  commonMethod(): void {}
}

class Admin extends getBaseClass(true) { // expression in extends
  commonMethod(): void {}
}

const Base = A;

class C extends Base { // expression in extends
  u: number = 2;
}

class Provider {
  static getBase() {
    return A;
  }
}

class D extends Provider.getBase() { // expression in extends
  d: number = 2;
}


class ArrowIife extends (() => A)() { // expression in extends
  z2: number = 6;
}

class Y extends (function() { return A; })() { // expression in extends
  z: number = 20;
}

function provide() {
  return A;
}

const Ref = provide();

class Q extends Ref { // expression in extends
  r: number = 8;
}

function outer() {
  return () => A;
}

class Chain extends outer()() { // expression in extends
  m: number = 200;
}


interface E {
    foo(): void;
}

let e = E

class C3 implements e { // expression in implements
    foo() {}
}