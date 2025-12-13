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

/* @@? 17:34 Error Semantic error ESE0143: Unresolved reference intFunc */
/* @@? 18:28 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 18:34 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 18:42 Error Syntax error ESY0230: Expected '=>', got ')'. */
/* @@? 18:42 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 18:44 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 18:44 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 18:46 Error Syntax error ESY0227: Unexpected token 'Object'. */
/* @@? 18:46 Error Semantic error ESE0132: The type of parameter 'Object' cannot be inferred */
/* @@? 18:65 Error Syntax error ESY0227: Unexpected token 'else'. */
/* @@? 18:71 Error Syntax error ESY0227: Unexpected token ')'. */
