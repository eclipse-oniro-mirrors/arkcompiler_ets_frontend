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

class A {}
class B {}

function foo<T>() {
    let x1: Partial<void> = {}
    let x2: Partial<A | B> = {}
    let x3: Partial<T> = {}
    let x4: Partial<undefined> = {}
    let x5: Partial<null> = {}
}


/* @@? 20:20 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 21:20 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 22:20 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 23:20 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 24:20 Error TypeError: T in Partial<T> must be a class or an interface type. */
