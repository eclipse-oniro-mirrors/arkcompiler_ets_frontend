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

function identity<Type>(arg: Type): Type {
  return arg;
}
let output = identity<string>/* @@ label */"hehe"

/* @@? 19:14 Error TypeError: Expected 1 arguments, got 0. */
/* @@? 19:14 Error TypeError: No matching call signature for identity() */
/* @@? 19:14 Error TypeError: Expected 1 arguments, got 0. */
/* @@? 19:14 Error TypeError: No matching call signature for identity() */
/* @@? 19:44 Error SyntaxError: Unexpected token, expected '('. */
/* @@? 26:70 Error SyntaxError: Expected ')', got 'end of stream'. */