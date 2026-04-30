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

declare class IterBox implements Iterable<int> {
    $_iterator(): Iterator<int>
}

declare let box: IterBox

function test(flag: boolean) {
    let fixed = [1, 2, 3] as FixedArray<int>
    let src: FixedArray<int> | IterBox = flag ? fixed : box
    for (let v of src) {
    }
}

/* @@? 25:19 Error Semantic error ESE0402: Unions of types with iterator and fixed/value arrays are currently unsupported in an iteration: FixedArray<Int>|IterBox. */
