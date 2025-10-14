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

import { Component } from '@kit.ArkUI';

  //error
  //error
function makeDate(mOrTimestamp: number, d?: number, y?: number): Date {  //error
  if (d !== undefined && y !== undefined) {
    return new Date(y, mOrTimestamp, d);
  } else {
    return new Date(mOrTimestamp);
  }
}
const d1 = makeDate(12345678);
const d2 = makeDate(5, 5, 5);
const d3 = makeDate(1, 3);

class Vector {
    //error
    //error
  abstract foo(x?: string): void {  //error
    new RegExp("body")
  }

    //error
    //error
  public fun(x?: string): void {  //error
    new RegExp("body")
  }
}

abstract class absClass {
  
  
  abstract foo(x?: string): void {
    new RegExp("body")
  }

    //error

    //error

  constructor(...args: number[]) {  //error
      /* ... */
    }
}
function func(){
  console.log("ArkTs foo4")
}

func.val = "0xff";

@Component
struct B{
  constructor() {
    super()
  }
  build() {
  }
}

struct C{
  constructor() {  //error
    super()
  }
    //error
}

class A{
  constructor() {
  }
}
class D{
  constructor() {  //error
  }
    //error
}