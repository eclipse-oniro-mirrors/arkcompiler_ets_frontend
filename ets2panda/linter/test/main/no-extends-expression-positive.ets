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

class UserBase {
  adminMethod(): void {
    console.log("user");
  }
}

class AdminBase {
  adminMethod(): void {
    console.log("admin");
  }
}

class User extends UserBase { // OK
  commonMethod(): void {
    console.log("common");
  }
}

class Admin extends AdminBase { // OK
  commonMethod(): void {
    console.log("common");
  }
}


class A {
  v: number = 1;
}

class B extends A { // OK
  u: number = 2;
}


interface E {
    foo(): void;
}

class C implements E { // expression in implements
    foo() {}
}


interface IFirst {
  first(): void;
}

interface ISecond{
  second(): void;
}

class TwoImplement implements IFirst, ISecond { // OK
  first(): void {
  }

  second(): void {
  }
}


class Base {
  id: number = 1;
}

class Mid extends Base {
  name: string = "mid";
}

class Final extends Mid { // OK
  active: boolean = true;
}


class Advanced extends Base implements ISecond { // OK
  second(): void {
  }
}
