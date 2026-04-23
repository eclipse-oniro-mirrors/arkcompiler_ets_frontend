/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

enum Foo {Bar, Baz}

function main() {

    {
      enum Foo {Baz, Bar}
      arktest.assertEQ(Foo.Bar.toInt(), 1)
    }

    arktest.assertEQ(Foo.Bar.toInt(), 0)
}

/* @@? 21:7 Error Syntax error ESY0040: Illegal start of ENUM expression. */
/* @@? 22:7 Error Semantic error ESE0127: No matching call signature for assertEQ(..., Int) */
/* @@? 22:32 Error Semantic error ESE0087: Property 'toInt' does not exist on type 'Foo' */
/* @@? 25:5 Error Semantic error ESE0127: No matching call signature for assertEQ(..., Int) */
/* @@? 25:30 Error Semantic error ESE0087: Property 'toInt' does not exist on type 'Foo' */
