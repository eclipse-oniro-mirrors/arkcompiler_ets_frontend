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

'use static';

export declare function main(): void;
export declare class Foo {
    public static get staticProperty(): number;
    public static set staticProperty(value: number);
    public bar(): void;
    constructor();
}

export declare interface UserParm {
    bar(): void;
}

export type ID = string | number;

export declare enum Color {
    Red = 0,
    Green = 1,
    Blue = 2,
}

export declare interface Printable {
    print(): void;
    getTitle(): string;
}
export declare class Document implements Printable {
    public constructor();
    public print(): void;
    public getTitle(): string;
}

export declare function move(dir: Direction): void;
export type Direction = "up" | "down" | "left" | "right";

export declare class Service {
    public doSomething(): void;
    public oldMethod(): void;
    constructor();
}

export declare function foo1(p: Promise<Promise<string>>): void;

export declare class Base {
    public get prop1(): number;
    public set prop1(value: number);
    constructor();
}
export declare class Derived extends Base {
    public get prop2(): string;
    public set prop2(value: string);
    constructor();
}
export declare class StaticClass {
    public static staticMethod(): void;
    constructor();
}
export declare class PrivateClass {
    constructor();
}
export declare class EmptyClass {
    constructor();
}

export declare const obj;

export declare function a(): string;

export declare class Parent {
    public get name(): string;
    public set name(value: string);
    public constructor(name: string);
}
export declare class Child extends Parent {
    public get age(): number;
    public set age(value: number);
    public constructor(name: string, age: number);
}

export declare function test(): void;

export declare let user: Person;
export declare function printPerson(p: Person1): void;
export declare class Person {
    public get name(): string;
    public set name(value: string);
    public get age(): number;
    public set age(value: number);
    public constructor(name: string, age: number);
}
export declare class Person1 {
    public get name(): string;
    public set name(value: string);
    constructor();
}
export type UserList = Array<Person>;

export declare const numbers: Array<number>;
export declare const moreNumbers: Array<number>;
export declare const part1: Array<number>;
export declare const part2: Array<number>;
export declare const combined: Array<number>;
export declare const original: Array<number>;
export declare const copy: Array<number>;
export declare class Test {
    public sum(...numbers: Array<number>): number;
    public get args(): Array<number>;
    public set args(value: Array<number>);
    public get result(): number;
    public set result(value: number);
    public get testNums(): Array<number>;
    public set testNums(value: Array<number>);
    public get moreTestNums(): Array<number>;
    public set moreTestNums(value: Array<number>);
    constructor();
}
