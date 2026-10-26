/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

interface ClockInterface {
  tick(): void;
}

class DigitalClock implements ClockInterface {
  private h: number;
  private m: number;
  constructor(h: number, m: number) {
    this.h = h;
    this.m = m;
  }
  tick() {
    console.log("beep beep");
  }
}

class AnalogClock implements ClockInterface {
  private h: number;
  private m: number;
  constructor(h: number, m: number) {
    this.h = h;
    this.m = m;
  }
  tick() {
    console.log("tick tock");
  }
}

function constructorSignature(): void {
  type ClockConstructor = new (hour: number, minute: number) => ClockInterface;

  function createClock(
    ctor: ClockConstructor,
    hour: number,
    minute: number
  ): ClockInterface {
    return new ctor(hour, minute);
  }

  const digital = createClock(DigitalClock, 12, 17);
  const analog = createClock(AnalogClock, 7, 32);
}

function constructorType(): void {
  function createClock(
    ctor: new (hour: number, minute: number) => ClockInterface,
    h: number,
    m: number
  ): ClockInterface {
    return new ctor(h, m);
  }

  const digital = createClock(DigitalClock, 16, 30);
  const analog = createClock(AnalogClock, 23, 45);
}

interface I {
  new (s: string): I;
}

function fn1(i: I) {
  return new i("hello");
}

class SomeObject {}

type SomeConstructor = {
  new (s: string): SomeObject;
};

function fn2(ctor: SomeConstructor) {
  return new ctor("hello");
}

class Person {
  constructor(name: string, age: number) {}
}

type PersonCtor = new (name: string, age: number) => Person;

function createPerson1(Ctor: PersonCtor, name: string, age: number): Person {
  return new Ctor(name, age);
}
function createPerson2(
  Ctor: new (name: string, age: number) => Person,
  name: string,
  age: number
): Person {
  return new Ctor(name, age);
}

const person1 = createPerson1(Person, "John", 30);
const person2 = createPerson2(Person, "John", 30);

// Test: property access not in new expression - should NOT trigger sdk-constructor-funcs
class Util {
  static factory(): Person {
    return new Person("name", 1);
  }
}
const ref = Util.factory;

// Test: qualified name not in new expression
const propRef = Person.prototype;

// Test: new expression with property access from class
const created = new Util.factory();

// Test: property access in new expression context
class Container {
  inner: Person;
  constructor() {
    this.inner = new Person("inner", 0);
  }
  static create(): Person {
    return new Person("static", 99);
  }
}
const fromStatic = new Container.create();
