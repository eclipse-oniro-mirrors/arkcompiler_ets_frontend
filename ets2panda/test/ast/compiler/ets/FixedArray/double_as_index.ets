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

let a: FixedArray<int> = new int[10.1](0)
let b: FixedArray<int> = new FixedArray<int>(10.1, 0)

function bar(): number {
    return 4.3
}

let c: FixedArray<int> = new FixedArray<int>(bar(), 0)

/* @@? 16:26 Error Semantic error ESE0127: No matching call signature for create(Double, Int) */
/* @@? 16:26 Error Semantic error ESE0046: Type 'Double' is not compatible with type 'Int' at index 1 */
/* @@? 17:46 Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */
/* @@? 23:46 Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */