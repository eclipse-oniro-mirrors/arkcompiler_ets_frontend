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

// destructuring declaration + untyped object literal
interface GeneratedObjectLiteralInterface_2 {
    a: number;
    b: number;
}
let GeneratedDestructObj_1: GeneratedObjectLiteralInterface_2 = { a: 1, b: 2 };
let a = GeneratedDestructObj_1.a;
let b = GeneratedDestructObj_1.b;


// destructuring declaration + untyped object literal + type literal
interface GeneratedTypeLiteralInterface_1 {
    c2: number;
    d2: string;
}
interface GeneratedObjectLiteralInterface_3 {
    a2: number;
    b2: GeneratedTypeLiteralInterface_1;
}
let GeneratedDestructObj_2: GeneratedObjectLiteralInterface_3 = {
    a2: 1,
    b2: {
        c2: 1,
        d2: '2'
    } as GeneratedTypeLiteralInterface_1
};
let a2 = GeneratedDestructObj_2.a2;
let b2 = GeneratedDestructObj_2.b2;


// untyped object literal + 'in' operator
interface GeneratedObjectLiteralInterface_1 {
    a: number;
    b: number;
}
console.log('a' in ({ a: 1, b: 2 } as GeneratedObjectLiteralInterface_1));

// untyped object literal + var declaration + literal as property name + function expression
interface GeneratedObjectLiteralInterface_4 {
    x1: number;
    x2: number;
}
let fun = () => {
    let o = {
    'a': 1,
    'b': 2
};
    let o2 = {
    'c': 3,
    'd': 4,
    5: ({ x1: 10,
    x2: 20 } as GeneratedObjectLiteralInterface_4)
};
};

// private identifier + definite assignment
class A {
    private a!: number;
}

// type assertion + as 'const'
const t = <const> 'hello';