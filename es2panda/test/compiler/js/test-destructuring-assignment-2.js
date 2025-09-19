/*
 Copyright (c) 2025 Huawei Device Co., Ltd.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 *
 http://www.apache.org/licenses/LICENSE-2.0
 *
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.limitations under the License.
 */


// object-destructuring.js

// 1. Basic usage
const obj1 = { a: 1, b: 2 };
const { a, b } = obj1;
print(a, b); // 1 2

// 2. Rename variables
const obj2 = { a: 1, b: 2 };
const { a: x, b: y } = obj2;
print(x, y); // 1 2

// 3. Default values
const obj3 = { a: 1 };
const { a: a3, b: b3 = 10 } = obj3;
print(a3, b3); // 1 10

// 4. Nested destructuring
const obj4 = { a: { x: 5, y: 6 }, b: 2 };
const { a: { x: x4, y: y4 }, b: b4 } = obj4;
print(x4, y4, b4); // 5 6 2

// 5. Destructuring in function parameters
function printUser1({ name, age }) {
  print(name, age);
}
const user1 = { name: "Alice", age: 25 };
printUser1(user1); // Alice 25

// 6. Function parameter destructuring with default values
function printUser2({ name = "Unknown", age = 0 }) {
  print(name, age);
}
printUser2({}); // Unknown 0

// 7. Dynamic property names
const key = "foo";
const obj8 = { foo: 123 };
const { [key]: val } = obj8;
print(val); // 123

// 8. Multiple nesting + default values + renaming
const obj9 = { user: { name: "Bob" } };
const { user: { name: username = "Guest", age: userAge = 18 } } = obj9;
print(username, userAge); // Bob 18
