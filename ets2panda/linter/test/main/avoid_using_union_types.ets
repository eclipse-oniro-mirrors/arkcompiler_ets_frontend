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

class A {
    x: number = 1;
    y: number = 2;
    z: number = 3;
    public v2: number = 1;
}

class B {
    y: string = '1';
    z: number = 2;
    public v2: string = '';
}

class C {
    y: number = 1;
    z: number = 2;
}

function foo(a: A | B | C) {
    a.x; // No error here, since TypeScript already checks this case and reports an error. No need to duplicate this logic
    a.y; // Report error here, type of `y` is different: 'number' in A, C and 'string' in B
    a.z; // No error, the code is valid
}

let data: Uint8Array = new Uint8Array();
let reqPb = data.buffer.slice(data.byteOffset, data.byteLength + data.byteOffset);
class D {
    tt() {}
}
class E {
    tt() {}
}
let ab: D|E = new D();
ab.tt();

declare interface TextPickerResult {
  value: string | string[];
}
const result: TextPickerResult = { value: 'a' };
result.value.toString() // no error - ok

class F {
  static a2: A | B = new A();
}

let a1: A | B = new A();
console.log('' + a1.v2) //Report error here
console.log('' + a1.y)  //Report error here
console.log('' + a1.z)  //No error, the code is valid 
console.log('' + F.a2.y)  //Report error here


class Cat1 {
  meow() {
    console.log("Cat1!")
  }
  meow2() {
    console.log("Cat1!")
  }
  meow3() {
    console.log("Cat1!")
  }
  meow4 = ()=>{};
  meow5:Function = ()=>{};
}

class Dog1 {
  meow() {
    console.log("Dog1!")
  }
  meow2():string {
    console.log("Dog1!")
    return "123"
  }
  meow3(str:string) {
    console.log("Cat1!")
  }
  meow4() {
    console.log("Cat1!")
  }
  meow5() {
    console.log("Cat1!")
  }
}

function makeSound1(pet: Cat1 | Dog1) {
  pet.meow();       //  [arkts-common-union-member-access]  correct
  pet.meow2();       // (arkts-common-union-member-access)    missing report
  pet.meow3("123");       // (arkts-common-union-member-access)    missing report
  pet.meow4();       // (arkts-common-union-member-access)    missing report
  pet.meow5();       // (arkts-common-union-member-access)      missing report
}