/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class A<T> {
    public value: T;
}

function main(): void {
    let a_int: A<int> = new A<int>();
    let prim_int = 5;

    switch(prim_int) {
        /* @@ label */case a_int.value: // not allowed, because type of a_int.value is Int
        default:
    }
}

/* @@@ label Error TypeError: Constant expression required  */
