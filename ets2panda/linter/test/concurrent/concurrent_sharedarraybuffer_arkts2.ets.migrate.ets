/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

let typeName: ArrayBuffer; // ERROR
let sab: ArrayBuffer = new ArrayBuffer(0) // 2 ERROR

type NewTypeName = ArrayBuffer // ERROR
let newTypeName: NewTypeName
// disable use new NewTypeName()
let ntn: NewTypeName = new ArrayBuffer(0) // ERROR

function foo(atmo: Atomics) {} // NOT ERROR

class A extends ArrayBuffer {
    constructor() {
        supper(1)
    }
}
