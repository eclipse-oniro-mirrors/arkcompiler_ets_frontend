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
  static foo() {
    return 123;
  }
}

class B extends A {
  static foo() {
    return A.foo() + 456;
  }
  static {
    console.log('asd');
    A.foo();
  }
}

class Animal1 {
  name: string;
  constructor(name: string) {
    this.name = name;
  }
}
class Dog1 extends Animal1 {
  breed: string;
  constructor(name: string, breed: string) {
    super(name);
    this.breed = breed;
  }
}

class Animal2 {
  move(): void {
    console.log("Moving...");
  }
}
class Dog2 extends Animal2 {
  move(): void {
    super.move();
    console.log("Running...");
  }
}

class Base1 {
  greet(): void {
    console.log("Hello, world!");
  }
}
class Derived1 extends Base1 {
  greet(): void {
    super.greet();
    console.log("Hello from Derived class!");
  }
}

class Base2 {
  value: number;
  constructor() {
    this.value = 100;
  }
}
class Derived2 extends Base2 {
  constructor() {
    super();
    this.value += 50;
  }
}