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

Class MyClass {
  compare((value: number, index:number, arr: Array<number>)) {}
}

/* @@? 16:1 Error SyntaxError: Class cannot be used as object. */
/* @@? 16:7 Error SyntaxError: Unexpected token 'MyClass'. */
/* @@? 16:7 Error TypeError: Unresolved reference MyClass */
/* @@? 16:15 Error SyntaxError: Unexpected token '{'. */
/* @@? 17:3 Error TypeError: Unresolved reference compare */
/* @@? 17:17 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 17:19 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 17:19 Error SyntaxError: Unexpected token 'number'. */
/* @@? 17:19 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 17:25 Error SyntaxError: Unexpected token ','. */
/* @@? 17:27 Error SyntaxError: Unexpected token 'index'. */
/* @@? 17:33 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 17:33 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 17:39 Error SyntaxError: Unexpected token ','. */
/* @@? 17:41 Error SyntaxError: Unexpected token 'arr'. */
/* @@? 17:46 Error SyntaxError: Class cannot be used as object. */
/* @@? 17:46 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 17:59 Error SyntaxError: Unexpected token ')'. */
/* @@? 17:60 Error SyntaxError: Unexpected token ')'. */
/* @@? 17:62 Error SyntaxError: Unexpected token '{'. */
