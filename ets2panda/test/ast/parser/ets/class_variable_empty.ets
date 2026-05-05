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

function main(): void {
  {
    /* @@ label1 */class A {}
    let a: /* @@ label2 */A = new /* @@ label3 */A;
    arktest.assertTrue(/* @@ label4 */a != null)
  }
  arktest.assertTrue(a != null)
}

/* @@@ label1 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@@ label2 Error Semantic error ESE0371: Cannot find type 'A'. */
/* @@@ label3 Error Semantic error ESE0371: Cannot find type 'A'. */
/* @@@ label4 Warning Warning W65001: The equality expression is always false. */
/* @@? 22:3 Error Semantic error ESE0127: No matching call signature for assertTrue(Boolean) */
/* @@? 22:22 Error Semantic error ESE0143: Unresolved reference a */
/* @@? 22:22 Warning Warning W65001: The equality expression is always false. */
