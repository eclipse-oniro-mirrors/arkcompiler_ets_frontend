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
let a =1;
a &&=2;
a ||=2;
a ??=2;
typeof  (a ||=2);
let person = { name: "Alice", age: 30 };

function  setAge(){
  person.age &&= - 1;
}
person.age = 0;
person.age &&= 5;

let count: number | null = null;
count ??= 10;

count = 0;
count ??= 5;
console.log((count ??= 5)+'')
function getCount(){
  return (count ??= 5);
}
let message: string | null = null;

message ||= "msg";
console.log(message);

message = "msg1";
message ||= "newMsg";
console.log(message);
class Demo{
  constructor() {
    message ||= "defaultMsg";
  }
  setMes(){
    message ||= "defaultMsg";
  }
  getMes(){
    return message ??= "newMsg";
  }
}