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

import { Something, SomethingFactory, SomethingBar, Bar, Select } from "./oh_modules/ohos_factory";

class C {
    static a = 5
    static b = 8
}

namespace H {
    export class G {}
}

let c = C //error
c = C // error
let g = H.G //error
g = H.G //error


function foo1(arg: typeof C) {}
foo1(C) //error

function foo2(arg: C) {
    return C //error
}

function foo3(arg: typeof H.G) {}
foo3(H.G) //error

function foo4(arg: H.G) {
    return H.G //error
}

class D {
    constructor(arg: typeof C) {}
}
new D(C) //error


enum Color { WHITE, BLUE, RED };
let color: Color = Color.RED;

for (let item = 0; item < Object.keys(Color).length; item++) { //error
    console.log(item);
}

foo2(() => C); //error

namespace NS {
    export enum E {
        A = 'A',
        B = 'B',
        C = 'C'
    }
}

for (let item = 0; item < Object.keys(NS.E).length; item++) { //error
    console.log(item);
}


function classAsObject(x: any): void {}

classAsObject(Object); // error
classAsObject(Function); // error
classAsObject(Math); //error
classAsObject(Date); // error
classAsObject(RegExp); // error
classAsObject(Error); // error
classAsObject(EvalError); // error
classAsObject(RangeError); // error
classAsObject(ReferenceError); // error
classAsObject(SyntaxError); // error
classAsObject(TypeError); // error
classAsObject(URIError); // error
classAsObject(AggregateError); // error
classAsObject(JSON); // error
classAsObject(Array); // error
classAsObject(ArrayBuffer); // error
classAsObject(DataView); // error
classAsObject(Int8Array); // error
classAsObject(Uint8Array); // error
classAsObject(Uint8ClampedArray); // error
classAsObject(Int16Array); // error
classAsObject(Uint16Array); // error
classAsObject(Int32Array); // error
classAsObject(Uint32Array); // error
classAsObject(Float32Array); // error
classAsObject(Float64Array); // error
classAsObject(Map); // error
classAsObject(WeakMap); // error
classAsObject(Set); // error
classAsObject(WeakSet); // error
classAsObject(Promise); // error
classAsObject(Proxy); // error
classAsObject(Symbol); // error
classAsObject(SharedArrayBuffer); // error
classAsObject(Atomics); // error
classAsObject(BigInt); // error
classAsObject(BigInt64Array); // error
classAsObject(BigUint64Array); // error
classAsObject(WeakRef); // error
classAsObject(FinalizationRegistry); // error


let aaa4 = typeof DataView; // error

let obj = new Object();
switch (obj) {
  case Object: // error
    console.log('A');
    break;
  default:
    console.log('F');
}
