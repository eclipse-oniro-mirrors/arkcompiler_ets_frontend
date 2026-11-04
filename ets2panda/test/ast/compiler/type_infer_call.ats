/**
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

let arr = [1.1, 2.2, 3.3]
let res = arr.reduce((a, b) => a + b, 0) // CTE now, type inference is not smart enough

let arr2 = [1, 2, 3]
let res2 = arr2.reduce((a, b) => a + b, 0.)

/* @@? 17:11 Error Semantic error ESE0127: No matching call signature for reduce(((p1: Int, p2: Double) => Int), Int) */
/* @@? 17:32 Error Semantic error ESE0091: Type 'Double' is not compatible with the enclosing method's return type 'Int' */
