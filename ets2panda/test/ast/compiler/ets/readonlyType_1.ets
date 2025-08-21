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
 
class A {
    fld: Number
    constructor (fld : Number) {
        this.fld = fld
    }
}

function foo(a0: A) {
    let a: Readonly<A> = new A(2)
    /* @@ label */a.fld = 5
}


/* @@@ label Error TypeError: Cannot assign to a readonly field fld  */
/* @@@ label Error TypeError: The 'Readonly' property cannot be reassigned.  */
