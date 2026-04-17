/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

for await (/* @@ label1 */;/* @@ label2 */;/* @@ label3 */i < /* @@ label4 */count/* @@ label5 */; ++i/* @@ label6 */) /* @@ label7 */{
    /* @@ label8 */result = result + /* @@ label9 */p[i]!.awaitResolution() * /* @@ label10 */a[i];
}
for (let i? /* @@ label11 */: Number = 1;;)  { break; }

/* @@? 16:27 Error Syntax error ESY0227: Unexpected token ';'. */
/* @@? 16:43 Error Syntax error ESY0227: Unexpected token ';'. */
/* @@? 16:59 Error Semantic error ESE0143: Unresolved reference i */
/* @@? 16:78 Error Semantic error ESE0143: Unresolved reference count */
/* @@? 16:98 Error Syntax error ESY0230: Expected ')', got ';'. */
/* @@? 16:102 Error Semantic error ESE0143: Unresolved reference i */
/* @@? 16:118 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 16:135 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 17:20 Error Semantic error ESE0143: Unresolved reference result */
/* @@? 17:29 Error Semantic error ESE0143: Unresolved reference result */
/* @@? 17:53 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 17:53 Error Semantic error ESE0143: Unresolved reference p */
/* @@? 17:53 Warning Warning W0015: The operand of the ensure-not-nullish expression is always non-nullish. */
/* @@? 17:95 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 17:95 Error Semantic error ESE0143: Unresolved reference a */
/* @@? 19:29 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */
