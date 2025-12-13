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

// Example 1
let ab: number | undefined = 5
ab && 10

let a = 1;
typeof (a && 2);


interface Person {
  name: string,
  age: number
}
let person: Person = { name: "Alice", age: 30 };

function setAge() {
  person.age && 1;
}

// Example 2
interface Obj {
  x?: number
}
function foo() {
  let obj: Obj | undefined = { x: 2 }
  obj.x && 33; // Unexpected token '&&='
}

// Example 3
let count: number | null = null;
count ?? 10;

count = null
console.log((count ?? 5) + '')

function getCount() {
  return (count ?? 5);
}

// Example 4
let message: string | null = null;

message || "msg";
message = "msg1";

class Demo {
  getMes() {
    return message || "newMsg";
  }
}

if (message || 'sdf') {
  console.log(message)
}