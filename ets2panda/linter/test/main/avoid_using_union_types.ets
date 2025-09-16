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
}

class B {
    y: string = '1';
    z: number = 2;
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
