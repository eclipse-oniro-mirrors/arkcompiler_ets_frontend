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

interface I {
    arr
}

function foo (bar: Partial<I>) {
    if (bar.var_one != undefined) {
        bar.var_one = "asddsf";
    }
}

function main() {
    let a : I = {var_one: "initial_var_one", var_two: "initial_var_two_a"};
    foo(a.var_one);
    let a : I = {var_two: "initial_var_two_b"};
    foo(break.var_one);
}

/* @@? 18:1 Error Syntax error ESY0138: Invalid Type. */
/* @@? 21:13 Error Semantic error ESE0087: Property 'var_one' does not exist on type 'I$partial' */
/* @@? 22:13 Error Semantic error ESE0087: Property 'var_one' does not exist on type 'I$partial' */
/* @@? 27:18 Error Semantic error ESE0065: type I has no property named var_one */
/* @@? 28:11 Error Semantic error ESE0087: Property 'var_one' does not exist on type 'I' */
/* @@? 29:9 Error Semantic error ESE0351: Variable 'a' has already been declared. */
/* @@? 29:18 Error Semantic error ESE0065: type I has no property named var_two */
/* @@? 30:9 Error Syntax error ESY0227: Unexpected token 'break'. */
