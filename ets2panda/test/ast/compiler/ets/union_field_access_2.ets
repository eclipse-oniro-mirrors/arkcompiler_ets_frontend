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

/* @@? 18:1 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 18:2 Error TypeError: Unresolved reference a */
/* @@? 18:6 Error TypeError: Unresolved reference delete */
/* @@? 18:13 Error SyntaxError: Unexpected token. */
/* @@? 18:13 Error TypeError: Unresolved reference arguments */
/* @@? 18:24 Error TypeError: need to specify target type for class composite */
/* @@? 18:26 Error SyntaxError: Unexpected token. */
/* @@? 18:28 Error SyntaxError: Unexpected token. */
/* @@? 18:54 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 18:76 Error SyntaxError: Unexpected token ')'. */
