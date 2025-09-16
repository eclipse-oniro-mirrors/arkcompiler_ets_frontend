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

function testInfo() : void {
    let iniP : Promise<Object> = intFunc();
    let pThen = iniP.then((d = e : Object) : Object => { return else;});
}

/* @@? 17:34 Error TypeError: Unresolved reference intFunc */
/* @@? 18:34 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 18:42 Error SyntaxError: Expected '=>', got ')'. */
/* @@? 18:42 Error SyntaxError: Unexpected token ')'. */
/* @@? 18:44 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 18:44 Error SyntaxError: Unexpected token ':'. */
/* @@? 18:46 Error SyntaxError: Unexpected token 'Object'. */
/* @@? 18:46 Error TypeError: The type of parameter 'Object' cannot be inferred */
/* @@? 18:65 Error SyntaxError: Unexpected token 'else'. */
/* @@? 18:71 Error SyntaxError: Unexpected token ')'. */
