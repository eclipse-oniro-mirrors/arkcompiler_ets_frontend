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

// ensure compiler doesn't crash when compiling invalid code.

(a ? delete arguments==({: false, Float64Array: []}) : [ArrayBuffer[false]])


/* @@? 18:1 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 18:2 Error Semantic error ESE0143: Unresolved reference a */
/* @@? 18:6 Error Semantic error ESE55815: Types cannot be modified at runtime with 'delete'. */
/* @@? 18:13 Error Syntax error ESY0016: Unexpected token. */
/* @@? 18:13 Error Semantic error ESE0143: Unresolved reference arguments */
/* @@? 18:24 Error Semantic error ESE0062: need to specify target type for class composite */
/* @@? 18:26 Error Syntax error ESY0016: Unexpected token. */
/* @@? 18:28 Error Syntax error ESY0016: Unexpected token. */
/* @@? 18:54 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 18:76 Error Syntax error ESY0227: Unexpected token ')'. */