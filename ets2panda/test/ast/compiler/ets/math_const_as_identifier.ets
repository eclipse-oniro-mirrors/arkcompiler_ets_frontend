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

function foo1() {
    const NaN = 10;
    const nan = 20;
    const Infinity = "Inf";

    let NaN = "";
    let Infinity = test();
    let undefined
};

/* @@? 21:9 Error TypeError: Variable 'NaN' has already been declared. */
/* @@? 22:9 Error TypeError: Variable 'Infinity' has already been declared. */
/* @@? 22:20 Error TypeError: Unresolved reference test */
/* @@? 23:9 Error SyntaxError: Hard keyword 'undefined' cannot be used as identifier */
/* @@? 23:9 Error SyntaxError: Identifier expected, got 'undefined'. */
/* @@? 24:1 Error SyntaxError: Variable must be initialized or it's type must be declared. */
