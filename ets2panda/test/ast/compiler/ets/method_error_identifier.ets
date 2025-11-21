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

void function () { };

class BindFuncExpr {
    let callback1 = function () { }
    callback1 = callback1 || function () { }
}


/* @@? 16:1 Error Syntax error ESY0227: Unexpected token 'void'. */
/* @@? 16:6 Error Syntax error ESY0227: Unexpected token 'function'. */
/* @@? 16:15 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 19:5 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 19:21 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */
/* @@? 20:5 Error Semantic error ESE0351: Variable 'callback1' has already been declared. */
/* @@? 20:30 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */
