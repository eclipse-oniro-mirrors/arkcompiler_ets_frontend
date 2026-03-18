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

// 1. T serves as the element type
function foo<T>() {
    new T[2](new T())
}

function foo1<T, U>() {
    new T[2](new T())
    new U[2][3]
}

interface Lengthwise {
    length : number
}

function foo2<T extends Lengthwise>() {
    new T[2](new T())
}

function foo3<T = number>() {
    new T[2](new T())
}

// 2. the alias of T serves as the element type
function foo4<T>() {
    type U = T
    new T[3](new T())
    new U[2](new U())
}

// 3. T is generic of a class, and this class serves as the element type
class A<T> {
    foo(){
        new A<T>[2](new A<T>())
    }
}

// 4. T is a field type of a class, and this class serves as the element type
class B<T> {
    field : T
    foo() {
        new B<T>[2](new B<T>())
    }
}

// 5. T is a subtype of an union, and this union serves as the element type
class C<T> {
    foo() {
        new (T|undefined)[2](undefined)
        new (T|int)[2](0)
    }
}

function foo5<T>() {
    new (T|undefined)[2](undefined)
    new (T|Object)[2](new Object())
}

// 6. T is a subtype of a tuple, and this tuple serves as the element type
class D<T> {
    foo() {
        new [T|undefined][2](undefined)
        new [T|int][2](0)
    }
}

function foo6<T>() {
    new [T|undefined][2](undefined)
    new [T|Object][2](new Object())
}


/* @@? 18:5 Error Semantic error ESE0127: No matching call signature for create(Int, ...) */
/* @@? 18:5 Error Semantic error ESE0292: This expression is not constructible. */
/* @@? 22:5 Error Semantic error ESE0127: No matching call signature for create(Int, ...) */
/* @@? 22:5 Error Semantic error ESE0292: This expression is not constructible. */
/* @@? 23:5 Error Semantic error ESE0007: Cannot use array creation expression with type parameter. */
/* @@? 31:5 Error Semantic error ESE0127: No matching call signature for create(Int, ...) */
/* @@? 31:5 Error Semantic error ESE0292: This expression is not constructible. */
/* @@? 35:5 Error Semantic error ESE0127: No matching call signature for create(Int, ...) */
/* @@? 35:5 Error Semantic error ESE0292: This expression is not constructible. */
/* @@? 40:5 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 41:5 Error Semantic error ESE0127: No matching call signature for create(Int, ...) */
/* @@? 41:5 Error Semantic error ESE0292: This expression is not constructible. */
/* @@? 42:5 Error Semantic error ESE0371: Cannot find type 'U'. */
/* @@? 76:9 Error Semantic error ESE0127: No matching call signature for create(Int, undefined) */
/* @@? 76:9 Error Semantic error ESE0046: Type 'undefined' is not compatible with type '[T|undefined]' at index 2 */
/* @@? 77:9 Error Semantic error ESE0127: No matching call signature for create(Int, Int) */
/* @@? 77:9 Error Semantic error ESE0046: Type 'Int' is not compatible with type '[T|Int]' at index 2 */
/* @@? 82:5 Error Semantic error ESE0127: No matching call signature for create(Int, undefined) */
/* @@? 82:5 Error Semantic error ESE0046: Type 'undefined' is not compatible with type '[T|undefined]' at index 2 */
/* @@? 83:5 Error Semantic error ESE0127: No matching call signature for create(Int, Object) */
/* @@? 83:5 Error Semantic error ESE0046: Type 'Object' is not compatible with type '[T|Object]' at index 2 */