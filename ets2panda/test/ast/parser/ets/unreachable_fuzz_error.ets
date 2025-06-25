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

    let x = z.meth(foo)

{
        arktest.assert(y[0] == 1)
        arktest.assert(y[1] == 2)
    
classtext: string, reviver: ((key: string, value: Any) => Any) | undefined, type: Type

/* @@? 16:13 Error TypeError: Unresolved reference z */
/* @@? 22:12 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 22:18 Error SyntaxError: Unexpected token ','. */
/* @@? 22:20 Error SyntaxError: Unexpected token 'reviver'. */
/* @@? 22:29 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 22:75 Error SyntaxError: Unexpected token ','. */
/* @@? 22:77 Error SyntaxError: Unexpected token 'type'. */
/* @@? 22:83 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 33:1 Error SyntaxError: Expected '}', got 'end of stream'. */
