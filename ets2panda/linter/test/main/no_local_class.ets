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

// VALID: Class/interface can be declared outside of functions
class GlobalClass { }
interface GlobalInterface { }

// INVALID: Class declared inside a function
function foo() {
    class Boo { }
}

// INVALID: Interface declared inside a function
function a() {
    interface B { }
}

// INVALID: Both class and interface declared inside a function
function b() {
    interface C { }
    class D { }
}

// INVALID: Interface declared inside an arrow function
const poo = () => {
    interface T { }
}

// INVALID: Both interface and class declared inside an arrow function
const yoo = () => {
    interface L { }
    class Zoo { }
}

// INVALID: Class declared inside an arrow function
const qoo = () => {
    class Moo { }
}

// INVALID: Class declared inside a function expression
const dest = function () {
    class Doo { }
}

// INVALID: Both class and interface declared inside a function expression
const test = function () {
    class Koo { }
    interface Goo { }
}

// INVALID: Interface declared inside a function expression
const cast = function () {
    interface Pop { }
}

// INVALID: Class declared inside a top-level block statement
{
    class BlockLocal { }
}

// INVALID: Interface declared inside a top-level block statement
{
    interface BlockIface { }
}

// INVALID: Class declared inside an if block
if (true) {
    class IfBlockClass { }
}

// INVALID: Interface declared inside an if block
if (false) {
    interface IfBlockIface { }
}

// INVALID: Class declared inside a for loop block
for (let i = 0; i < 1; i++) {
    class ForBlockClass { }
}

// INVALID: Interface declared inside a for loop block
for (let i = 0; i < 1; i++) {
    interface ForBlockIface { }
}

// INVALID: Class declared inside a while loop block
while (false) {
    class WhileBlockClass { }
}

// INVALID: Interface declared inside a while loop block
while (false) {
    interface WhileBlockIface { }
}

// INVALID: Class declared inside a nested block (block inside block)
{
    {
        class NestedBlockClass { }
    }
}

// INVALID: Interface declared inside a nested block (block inside block)
{
    {
        interface NestedBlockIface { }
    }
}

// VALID: Type alias at top-level (should NOT warn)
type FooType = number;

// NESTED CASE: Deeply nested class inside arrow functions
function xyc(): void {
    const a1 = () => {
        const b2 = () => {
            class Abc {

            }
        }
    }
}

