/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function makeDate(timestamp: number): Date;  //error
function makeDate(m: number, d: number, y: number): Date;  //error
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
  abstract foo(): void  //error
  abstract foo(x: string): void  //error
  abstract foo(x?: string): void {  //error
    /body/
  }

  public fun(): void  //error
  public fun(x: string): void  //error
  public fun(x?: string): void {  //error
    /body/
  }
}

abstract class absClass {
  abstract foo(): void
  abstract foo(x: string): void
  abstract foo(x?: string): void {
    /body/
  }

  constructor(x: number, y: number);  //error

  constructor(magnitude: number);  //error

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
  constructor(x:number)  //error
}

class A{
  constructor() {
  }
}
class D{
  constructor() {  //error
  }
  constructor(x:number)  //error
}

// Test: ambient class with method overloads - should NOT report (ambient context)
declare class AmbientClass {
  foo(): void;
  foo(x: string): void;
}

// Test: ambient namespace with function overloads - should NOT report (ambient context)
declare namespace AmbientNS {
  function bar(): void;
  function bar(x: number): void;
}

// Test: internal function with 2 declarations - should NOT report (needs >2 for internal)
function animateTo(): void {}
function animateTo(x: number): void {}

// Test: internal function with 3 declarations - should report (>=3 for internal)
function vp2px(): void;  //error
function vp2px(x: number): void;  //error
function vp2px(x?: number): void {}

// Test: struct with constructor without text (empty body)
struct StructE {
  constructor()  //error
  constructor(x: number)  //error
}

// Test: function with name but single declaration - should NOT report
function singleFunc(x: string): void {}

// Test: method overload in ambient class with declare keyword
declare class AmbientDecl {
  methodA(): void;
  methodA(x: string): void;
  constructor();
  constructor(x: number);
}