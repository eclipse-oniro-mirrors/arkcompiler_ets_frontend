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

type X<T> = FixedArray<Y<T> | T>
type Y<T> = Array<X<T> | T | string>

function f(): X<number> {
    return [1.0] as X<number>
}

function g(): FixedArray<Y<number> | string> {
    return [] as FixedArray<Y<number> | string>
}

function main() {
    let a = f()
    let b = g()
    let ok = a.length == 1 && b.length == 0
    arktest.assertTrue(ok)
}

