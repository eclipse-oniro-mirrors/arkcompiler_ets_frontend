/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

// Following declarations are added to check that we don't generate new interfaces with names that already exist
import GeneratedObjectLiteralInterface_4 from 'x';
import * as GeneratedObjectLiteralInterface_7 from 'x';
import {
    GeneratedObjectLiteralInterface_8,
    foo as GeneratedObjectLiteralInterface_9
} from 'x';
interface GeneratedObjectLiteralInterface_11 {} 
class GeneratedObjectLiteralInterface_12 {}
function GeneratedObjectLiteralInterface_15() {}

class C<T> {}

function foo(x): number {
    return 1;
}

const o1 = {};
const o2 = { hello: "world" };
const o3! = {a: 1, b: 2};
const o4 = { 
    field: "hello",
    field1: 2,
    field2: "world",
    field3: 4,
    field4: true,
    field5: false
};

// Properties with various types. Nested object literals
const o5 = {
    a: 1,
    b: '2',
    c: true,
    d: new C<number>(),
    e: {},
    f: { a: 1, b: '2' },
    g: {
        q: 10,
        w: 20
    },
};
const o6 = {
    a: 1,
    b: '2',
    c: { q: 10, w: 20 },
    d: true,
    e: { q: 30, w: 40 }
};

// Object literals inside another expression
const o7 = { a:1, b:2 }.a + { a:3, b:4 }.b;
const o8 = {
    a: 1, 
    b: 2,
    c: ({x:1, y:2}),
    d: foo({q:1, w:2}.q + {q:3, w:4}.w)
};

// Object literals inside class declaration
class D {
    f1 = {a: 1, b: 2};
    f2? = {a: 1, b: 2};
    f3! = {a: 1, b: 2};
    f4 = ({c: 3, d: 4});
    f5 = {e: 5}.e + {f: 6, g: 7}.f;

    m() {
        let x = {a:1, b:2};
        let y = {c:1, d:2, e:3};
    }
}

// Object literals as function parameter initializer
function funInit(p = { a: 1, b: 2 }) {}
function funInit2({a, b} = { a: 3, b: 4 }) {} // Not fixable, as in case of destructuring parameters, the contextual type of expression is implied by the binding pattern

// Object literals inside function declaration
function bar(): void {
    let a = {a: 1, b: 2};
    let b = {c: 3, d: 4};

    if (a.b > b.c) {
        let c = {e: 5, f: 6};
    }

    let d = {g: 7, d: foo({q:1,w:2}.q + {q:3,w:4}.w)};
}

const o9 = { 1: '1', '2': 2 }; // Not fixable, property name is string/numeric literal
const o10 = { [3]: 3 }; // Not fixable, computed property value
const o11 = { [o2.hello]: 'world' }; // Not fixable, computed property value

const anyVal: any = 1;
const o12 = { a: anyVal }; // Not fixable, type of property 'a' is not supported

let val = 1;
const o13 = { val }; // Fixable
const o14 = { ...o1 }; // Not fixable, property is not 'key:value' pair
const o15 = { m() {} }; // Fixable

const o16 = { // Not fixable, property 'c' is initialized with non-fixable nested object literal, and thus will always have unsupported type (object type literal)
    a: 1,
    b: '2',
    c: {
        q: 1,
        w: 2,
        e: anyVal // Not fixable, unsupported type
    }
}

class X<K> {}
class Y<Q extends X<Q>> {}
function captureFromLocalScope<T>(t: T): void {
    let v1 = {a: 1, b: '2', c: t}; // Not fixable, `c` references local type parameter `T`
    let v2 = {a: 1, b: '2', c: new X<T>()}; // Not fixable, `c` references local type parameter `T`
    let v3 = {a: 1, b: '2', c: new Y<X<T>>()}; // Not fixable, `c` references local type parameter `T`
  
    type LocalType = {a: number, b: string};
    let localTypeVar: LocalType = {a:1, b:'2'};
    let v4 = { x: localTypeVar }; // Non-fixable, `x` references type `LocalType` declared in local scope
  
    class LocalClass {x: number = 1};
    let v5 = { y: new LocalClass() }; // Non-fixable, `y` references type `LocalClass` declared in local scope
  
    let v6 = { z: LocalClass }; // Non-fixable, `z` references type `LocalClass` declared in local scope
}

// Record object literals
let rec1: Record<string, any> = {
    a: 1,
    b: 2,
    c: 3
}

let rec2: Record<string | number, any> = {
    foo: 1,
    bar: 2,
    10: 'foo',
    20: 'bar',
    baz: 3,
    'daz': 4
}

let rec3: Record<string | number, any> = { // Not fixable
    f1: 1,
    f2: 2,
    f3: 3,
    [val]: 4    // Not fixable, key is a computed value
}

interface NullableRecord {
    params?: Record<string, string>;
}
let rec4: NullableRecord = {
    params: {
        key: '1',
        message: '2'
    }
};