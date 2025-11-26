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

class C {
  v1: int = 0;
  v2: string = "";
}

let x: C = {/* @@ label1 */"v1": /* @@ label2 */"zzz", /* @@ label3 */"v2"/* @@ label4 */}

/* @@@ label1 Warning Warning W98268: Key in class composite should be identifier, the use of strings as keys will be deprecated soon */
/* @@@ label2 Error Semantic error ESE0319: Type '"zzz"' is not compatible with type 'Int' at property 'v1' */
/* @@@ label3 Warning Warning W98268: Key in class composite should be identifier, the use of strings as keys will be deprecated soon */
/* @@@ label4 Error Syntax error ESY0228: Unexpected token, expected ':'. */
/* @@? 29:1 Error Syntax error ESY0227: Unexpected token 'end of stream'. */
/* @@? 29:1 Error Syntax error ESY0016: Unexpected token. */
