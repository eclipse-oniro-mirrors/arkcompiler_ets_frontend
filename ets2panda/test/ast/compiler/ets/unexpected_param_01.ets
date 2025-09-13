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

let byteP: Promise<Object> = byteFunc()
let pthen = byteP.then<Object>((value ? Object): Object => {
    return value

});
async function foo() {
    await byteP;
}

/* @@? 16:30 Error TypeError: Unresolved reference byteFunc */
/* @@? 17:41 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 17:41 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 17:41 Error SyntaxError: Expected '=>', got 'identification literal'. */
/* @@? 17:41 Error SyntaxError: Class cannot be used as object. */
/* @@? 17:48 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:50 Error SyntaxError: Unexpected token 'Object'. */
/* @@? 17:50 Error TypeError: The type of parameter 'Object' cannot be inferred */
/* @@? 18:12 Error TypeError: Unresolved reference value */
/* @@? 20:2 Error SyntaxError: Unexpected token ')'. */