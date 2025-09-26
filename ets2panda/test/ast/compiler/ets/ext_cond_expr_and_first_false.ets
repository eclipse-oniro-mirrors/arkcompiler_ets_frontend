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

let arr = [555, 666]

// 'A && B' => A is false, so the result should have type of A

arr["" && 1]
arr[false && 1]
arr[0.0 && 1]

// NO CTE
arr[0 && 1]
arr[0 && ""]
arr[0 && 0.0]

/* @@? 20:1 Error Semantic error ESE0127: No matching indexing signature for $_get("") */
/* @@? 20:5 Error Semantic error ESE0251: Cannot find index access method with the required signature. */
/* @@? 21:1 Error Semantic error ESE0127: No matching indexing signature for $_get(Boolean) */
/* @@? 21:5 Error Semantic error ESE0251: Cannot find index access method with the required signature. */
/* @@? 22:1 Error Semantic error ESE0127: No matching indexing signature for $_get(Double) */
/* @@? 22:5 Error Semantic error ESE0251: Cannot find index access method with the required signature. */
