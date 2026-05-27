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

class A {}
class B extends A {}
function main(): int {
    let t: char = c'a'
    /* @@ label1 */ t as boolean

    let b: B[] = [new B()]
    /* @@ label2 */ b as A[]

    let n: Object | null = null
    /* @@ label3 */ n as Object

    return 0
}

/* @@@ label1 Warning Warning W0326: Attempting to cast type 'Char' to 'Boolean' can throw 'ClassCastError' exception at runtime */
/* @@@ label2 Warning Warning W369037: Cast expression `as Array<A>` is type unsafe. 'ClassCastError' exception can be thrown at runtime */
/* @@@ label3 Warning Warning W0304: Possible cast of 'null' or 'undefined' to non-nullish type can throw 'ClassCastError' exception at runtime */
