/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
    a: A<T> = undefined as A<T>
    foo(f: (a: FixedArray<Tadad> | A<T>[]) => void): void {}
    bar(): A<T> { return this.a }
}

new A<number>().bar().a.foo((x) => {
    for (let e of x) {
        console.log(e)
    }
})

/* @@? 17:15 Warning Warning W0304: Possible cast of 'null' or 'undefined' to non-nullish type can throw 'ClassCastError' exception at runtime */
/* @@? 18:27 Error Semantic error ESE0371: Cannot find type 'Tadad'. */
/* @@? 23:19 Error Semantic error ESE0402: Unions of types with iterator and fixed/value arrays are currently unsupported in an iteration: FixedArray<*ERROR_TYPE*>|Array<A<Double>>. */
