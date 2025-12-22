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

class MyClass {
  i: number = neverFunction();
  static member_: byte = neverFunction();
  
  constructor() {
    console.log(this.i);
  }
}

function neverFunction(): never {
  throw new Error("This function never returns");
}

function main() {
    let obj = new MyClass();
    let j = obj.i;
}