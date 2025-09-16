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

// 1. T serves as the element type
function foo<T>() {
    new T[2]
}

function foo1<T, U>() {
    new T[2]
    new U[2][3]
}

interface Lengthwise {
    length : number
}

function foo2<T extends Lengthwise>() {
    new T[2]
}

function foo3<T = number>() {
    new T[2]
}

// 2. the alias of T serves as the element type
function foo4<T>() {
    type U = T
    new T[3]
    new U[2]
}

// 3. T is generic of a class, and this class serves as the element type
class A<T> {
    foo(){
        new A<T>[2]
    }
}

// 4. T is a field type of a class, and this class serves as the element type
class B<T> {
    field : T
    foo() {
        new B<T>[2]
    }
}

// 5. T is a subtype of an union, and this union serves as the element type
class C<T> {
    foo() {
        new (T|undefined)[2]
        new (T|int)[2]
    }
}

function foo5<T>() {
    new (T|undefined)[2]
    new (T|Object)[2]
}

// 6. T is a subtype of a tuple, and this tuple serves as the element type
class D<T> {
    foo() {
        new [T|undefined][2]
        new [T|int][2]
    }
}

function foo6<T>() {
    new [T|undefined][2]
    new [T|Object][2]
}

/* @@? 18:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 22:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 23:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 31:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 35:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 40:5 Error SyntaxError: Illegal start of Type Alias expression.  */
/* @@? 41:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 42:5 Error TypeError: Cannot use array creation expression with non-constructable element type which is non-assignable from undefined.  */
/* @@? 42:9 Error TypeError: Cannot find type 'U'.  */
/* @@? 64:9 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 70:5 Error TypeError: Cannot use array creation expression with type parameter.  */
/* @@? 76:9 Error TypeError: Cannot use array creation expression with non-constructable element type which is non-assignable from undefined.  */
/* @@? 77:9 Error TypeError: Cannot use array creation expression with non-constructable element type which is non-assignable from undefined.  */
/* @@? 82:5 Error TypeError: Cannot use array creation expression with non-constructable element type which is non-assignable from undefined.  */
/* @@? 83:5 Error TypeError: Cannot use array creation expression with non-constructable element type which is non-assignable from undefined.  */
