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

import {I} from "./optionalClassProperty2.ets"

// 1. multiple classes implements single interface
interface I1 {
    a ?: Array<number>
}
class C11 implements I1 {
    a ?: Array<number>
}
class C12 implements I1 {
    a ?: Array<number>
}

// 2. multiple classes implements multiple interfaces
interface I21 {
    a ?: Array<number>
}
interface I22 {
    b ?: Array<number>
}
class C21 implements I21, I22 {
    a ?: Array<number>
    b ?: Array<number>
}
class C22 implements I21, I22 {
    a ?: Array<number>
    b ?: Array<number>
}

// 3. add readonly attr for interface property
interface I3 {
    readonly a ?: Array<number>
    readonly b ?: Array<number>
}
class C3 implements I3 {
    readonly a ?: Array<number>
    b ?: Array<number>
}

// 4. add private qualifier for interface property
interface I4 {
    private a ?: Array<number>
}
class C4 implements I4 {
    a ?: Array<number>
}

// 5. class has owner property which is not included by interface
interface I5 {
    a ?: Array<number>
}
class C5 implements I5 {
    a ?: Array<number>
    b : string = ""
    c ?: int
}

// 6. Getter or Setter instead of field
interface I6 {
    get a() : Array<number> | undefined
    set a(arr : Array<number> | undefined)
}
class C6 implements I6 {
    a ?: Array<number>
}

// 7. super interface
interface I71 {
    a ?: Array<number>
}
interface I72 extends I71 {
    b ?: Array<number>
}
class C7 implements I72 {
    a ?: Array<number>
    b ?: Array<number>
}

// 8. cross script file
class C8 implements I {
    a ?: Array<number>
}

/* @@? 57:5 Error SyntaxError: Identifier expected, got 'private'. */
