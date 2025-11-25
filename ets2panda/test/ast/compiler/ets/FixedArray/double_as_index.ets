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

let a: FixedArray<int> = new int[10.1]
let b: FixedArray<int> = new FixedArray<int>(10.1)

function bar(): number {
    return 4.3
}

let c: FixedArray<int> = new FixedArray<int>(bar())

/* @@? 16:26 Error Semantic error ESE0318: Type 'Array<Int>' cannot be assigned to type 'FixedArray<Int>' */
/* @@? 16:34 Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */
/* @@? 17:46 Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */
/* @@? 23:46 Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */