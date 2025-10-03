/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

try {
    let x = 8
}

try {}
finally (x)

try {}
catch () {}

declare namespace abc {
    let i = 0
}

function f(): void {
    abstract function s(): void {}
}

function f(): void {
    module "module1" let x = 99;
}

interface A {
    a: number;
    a: char;
}

class A {
    declare [index: string]]: string;

    @decorator
    [index: string]]: MyType;

    @decorator
    private x: number;

    @decorator
    constructor() {}

    x! number;
}

class A {
    [index: number]]?: string;

    constructor?(x: number) {}
}

class A implements A. {}

const x: number;

let [x, y];

enum A
    I1,
    I2,
    123,
    123 = 55,
    I3 = ,
    I4
}

class A<123> {}

class A<T = Number, S> {}

class A<> {}

class B {
    abstract class C;

    private public x: number;
    protected protected x: number;

    constructor(static x: number) {}
}

let f = (a?: number, a: string) => {};
let f = (arguments: bool[], eval: bool) => {};
let f = ([a!, [b]?, c?]) => {};
let f = (a?: [c: int, d?: char]) => {};
let f = ({a: 123, b: bool}) => {};
let f = (a: int = await 10, a?: int = 2) => {};

declare namespace abc {
    let a = 8;
}

module {
    declare function s(): void;
}

module module2
    declare namespace abc {}
// This should be the last test to show the absent of the '}'.


/* @@? 20:1 Error Syntax error ESY0169: Missing catch or finally clause. */
/* @@? 21:9 Error Syntax error ESY0230: Expected '{', got '('. */
/* @@? 24:8 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 27:13 Error Syntax error ESY0125: Initializers are not allowed in ambient contexts. */
/* @@? 31:14 Error Syntax error ESY0184: Abstract methods can only appear within an abstract class. */
/* @@? 35:12 Error Syntax error ESY0181: Only ambient modules can use quoted names. */
/* @@? 35:22 Error Syntax error ESY0228: Unexpected token, expected ';'. */
/* @@? 40:5 Error Syntax error ESY0215: Duplicated identifier. */
/* @@? 44:37 Error Syntax error ESY0104: A 'declare' modifier cannot be used in an already ambient context. */
/* @@? 46:5 Error Syntax error ESY0183: Decorators are not valid here. */