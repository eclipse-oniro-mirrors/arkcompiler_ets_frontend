/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use static';

import myhilog from './getReferencesAtPosition8';

import * as All from './getReferencesAtPosition8';

export class Foo {
    bar() {};
    static staticProperty: number = 42;
}
let foo = new Foo();
foo.bar();
export { PD } from "./getReferencesAtPosition8";

export interface User {
  bar(): void;
}

export type ID = string | number;

export enum Color {
  Red,
  Green,
  Blue
}

export interface Printable {
    print(): void;
    getTitle(): string;
}

export class Document implements Printable {
    constructor() {}

    print(): void {
    }

    getTitle(): string {
        return "";
    }
}

export type Direction = 'up' | 'down' | 'left' | 'right';
export function move(dir: Direction): void {
  let a = 'aaa';
  const b: Direction = 'up';
  let c = true;
}

@interface Validate {

}

@interface Log {
  level: string;
}

@interface Component1 {
    name: string;
    version: number;
}

@interface Deprecated {

}

@Component1({name: 'Service', version: 1})
export class Service {
    @Validate
    @Log({level: 'info'})
    doSomething() {
    }
    
    @Deprecated
    oldMethod() {
    }
}

export function foo1(p: Promise<Promise<string>>): void {
    let result: string = await p;
}

export class Base {
    prop1: number = 1;
}

export class Derived extends Base {
    prop2: string = "test";
}

export { State } from './getReferencesAtPosition8';

const obj = {
    prop: 'value'
};

export function a() {
    return 'hello';
}
a();

export class Parent {
  name: string;
  constructor(name: string) {
    this.name = name;
  }
}
export class Child extends Parent {
  age: number;
  constructor(name: string, age: number) {
    super(name);
    this.age = age;
  }
}

export { PI } from "std/math";

export function test(){}

export class Person {
    name: string;
    age: number;

    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }
}
let user: Person = new Person("Alice", 30);
export class Person1 {
    name = "Jonn"
}
export function printPerson(p: Person1): void {
}
export type UserList = Array<Person>;

const numbers = [1, 2, 3];
const moreNumbers = [0, ...numbers, 4, 5]; 
const part1 = [1, 2];
const part2 = [3, 4];
const combined = [...part1, ...part2, 5]; 
const original = [10, 20, 30];
const copy = [...original];

export class Test {
   sum(...numbers: number[]): number {
      return 0;
   }
   args = [1.5, 2.5, 3.0];
   result = this.sum(...this.args);
   testNums = [1, 2, 3];
   moreTestNums  = [0, ...this.testNums, 4, 5]; 
}