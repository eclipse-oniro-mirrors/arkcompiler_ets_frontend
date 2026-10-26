/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

const test1 = {}

const test2 = { hello: "world" };

let test3:{} = {};

const test4 = { 
        field: "world",
        field1: "hello2",
        field2: "hello3",
        field3: "hello4",
        field4: "hello5",
        field5: "hello6",
        field6: "hello7",
    };

const test5 = { 
        field: 1,
        field1: 2,
        field2: 3,
        field3: 4,
        field4: 5,
        field5: 6,
        field6: 7,
    };

const test6 = { 
        field: "world",
        field1: 2,
        field2: "hello3",
        field3: 4,
        field4: "hello5",
        field5: 6,
        field6: "hello7",
        field7: true,
        field8: false
    };

interface test7 { 
        field: "world",
        field1: "hello2",
        field2: "hello3",
        field3: "hello4",
        field4: "hello5",
        field5: "hello6",
        field6: "hello7"
    };

interface test8 { 
        field: 1,
        field1: 2,
        field2: 3,
        field3: 4,
        field4: 5,
        field5: 6,
        field6: 7,
    };

interface test9 { 
        field: "world",
        field1: 2,
        field2: "hello3",
        field3: 4,
        field4: "hello5",
        field5: 6,
        field6: "hello7",
        field7: true,
        field8: false
    };

const test10:object = { 
        field: "world",
        field1: "hello2",
        field2: "hello3",
        field3: "hello4",
        field4: "hello5",
        field5: "hello6",
        field6: "hello7",
    };

const test11:object = { 
        field: 1,
        field1: 2,
        field2: 3,
        field3: 4,
        field4: 5,
        field5: 6,
        field6: 7,
    };

const test12:object = { 
        field: "world",
        field1: 2,
        field2: "hello3",
        field3: 4,
        field4: "hello5",
        field5: 6,
        field6: "hello7",
        field7: true,
        field8: false
    };

const test13:object = { 
        field: "world",
        field1: "hello2",
        field2: "hello3",
        field3: "hello4",
        field4: "hello5",
        field5: "hello6",
        field6: "hello7",
    };

const test14:object = { 
        field: 1,
        field1: 2,
        field2: 3,
        field3: 4,
        field4: 5,
        field5: 6,
        field6: 7,
    };

const test15:object = { 
        field: "world",
        field1: 2,
        field2: "hello3",
        field3: 4,
        field4: "hello5",
        field5: 6,
        field6: "hello7",
        field7: true,
        field8: false
    };

let test16:{
    field:String,
    field1:String,
    field2:String,
    field3:String,
    field4:String,
    field5:String,
    field6:String} = { 
        field: "world",
        field1: "hello2",
        field2: "hello3",
        field3: "hello4",
        field4: "hello5",
        field5: "hello6",
        field6: "hello7",
    };

let test17:{
    field:number,
    field1:number,
    field2:number,
    field3:number,
    field4:number,
    field5:number,
    field6:number} = { 
        field: 1,
        field1: 2,
        field2: 3,
        field3: 4,
        field4: 5,
        field5: 6,
        field6: 7,
    };

let test18:{
    field:String,
    field1:number,
    field2:String,
    field3:number,
    field4:String,
    field5:number,
    field6:String,
    field7:boolean,
    field8:boolean} = { 
        field: "world",
        field1: 2,
        field2: "hello3",
        field3: 4,
        field4: "hello5",
        field5: 6,
        field6: "hello7",
        field7: true,
        field8: false
    };

const test19:Record<string, string> = { 
        field: "world",
        field1: "hello2",
        field2: "hello3",
        field3: "hello4",
        field4: "hello5",
        field5: "hello6",
        field6: "hello7",
    };

const test20:Record<string, number> = { 
        field: 1,
        field1: 2,
        field2: 3,
        field3: 4,
        field4: 5,
        field5: 6,
        field6: 7,
    };

const test21:Record<string, string | number | boolean> = {
        field: "world",
        field1: 2,
        field2: "hello3",
        field3: 4,
        field4: "hello5",
        field5: 6,
        field6: "hello7",
        field7: true,
        field8: false
    };

// Test: object literal as function argument
function acceptObject(param: { name: string, value: number }): void {
  console.log(param.name, param.value);
}
acceptObject({ name: "test", value: 42 });

// Test: object literal as function argument with extra args
function multiParams(a: number, b: { x: string }): void {
  console.log(a, b.x);
}
multiParams(1, { x: "hello" }, { y: "extra" });

// Test: object literal in return statement
function returnObject(): { name: string, value: number } {
  return { name: "result", value: 100 };
}

// Test: nested object literal assignment
interface Inner { x: number; }
interface Outer { inner: Inner; }
const nested: Outer = { inner: { x: 1 } };

// Test: object literal without type annotation
const noType = { a: 1, b: "hello" };

// Test: object literal with nested property as function call arg
function acceptOuter(param: Outer): void {
  console.log(param.inner.x);
}
acceptOuter({ inner: { x: 2 } });

// Test: object literal in arrow function return
const arrowReturn = (): { x: number } => {
  return { x: 5 };
};

// Test: object literal in method return
class ObjHandler {
  getData(): { name: string, value: number } {
    return { name: "method", value: 200 };
  }
}