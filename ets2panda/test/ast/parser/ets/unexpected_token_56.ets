/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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


/* @@@ label1 Error Syntax error ESY0227: Unexpected token ';'. */
/* @@@ label2 Error Syntax error ESY0227: Unexpected token ';'. */
/* @@@ label3 Error Semantic error ESE0143: Unresolved reference i */
/* @@@ label4 Error Semantic error ESE0143: Unresolved reference count */
/* @@@ label5 Error Syntax error ESY0230: Expected ')', got ';'. */
/* @@@ label6 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@@ label7 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@@ label8 Error Semantic error ESE0143: Unresolved reference result */
/* @@@ label9 Error Semantic error ESE0143: Unresolved reference p */
/* @@@ label9 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@@ label10 Error Semantic error ESE0143: Unresolved reference a */
/* @@@ label10 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@@ label11 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */