/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
 
// &&= basic usage
let ab: number | undefined = 5
let abc: number | undefined = 0

ab &&= 10 // &&=, ||=, ??= operators not supported.
abc &&= 5 // Unexpected token '&&='

// &&= usage in typeof
let a = 1;
typeof (a &&= 2); // Unexpected token '&&='

// &&= usage with object property
interface Person {
  name: string,
  age: number
}
let person: Person = { name: "Alice", age: 30 };

function setAge() {
  person.age &&= - 1; // Unexpected token '&&='
}
person.age &&= 5; // Unexpected token '&&='

// &&= usage inside function with union type variable
interface Obj {
  x?: number
}
function foo() {
  let obj: Obj | undefined = { x: 2 }
  obj.x &&= obj.x + 33; // Unexpected token '&&='
}

// ??= usage with nullable variable
let count: number | null = null;
count ??= 10; // Unexpected token '??='

// ??= usage with in builtin function
count = null
console.log((count ??= 5) + '') // Unexpected token '??='

// ??= usage in return statement
function getCount() {
  return (count ??= 5); // Unexpected token '??='
}

// ||= usage with nullable variable
let message: string | null = null;

message ||= "msg"; // Unexpected token '||='
message = "msg1";
message ||= "newMsg"; // Unexpected token '||='

// ||= usage in class/constructor
class Demo {
  constructor() {
    message ||= "defaultMsg"; // Unexpected token '||='
  }
  getMes() {
    return message ||= "newMsg"; // Unexpected token '||='
  }
}

// ||= usage in if statement
if (message ||= 'sdf') { // Unexpected token '||='
  console.log(message)
}