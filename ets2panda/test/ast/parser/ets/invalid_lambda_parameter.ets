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

interface I {}
class A {
    constructor(callback: (entries: I[]) => void) {}
}

function foo(entry: I) {
    const x = new A(/* @@ label1 */([entry]) /* @@ label2 */=> {
    });
}

/* @@? 22:15 Error TypeError: No matching construct signature for invalid_lambda_parameter.A(...) */
/* @@? 22:36 Error TypeError: Invalid lambda parameter. Expected: 'identifier(: type)?', 'identifier?(: type)?' or '...identifier(: type)?'. */
/* @@? 22:61 Error SyntaxError: Unexpected token '=>'. */