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

let enum ELetInsteadOfConst {
  Member1,
  Member2,
  Member3
}

enum EWrongLeftBrace (
  Member1 = 1,
  Member2 = 2,
  Member3 = 3
}

enum EWrongRightBrace {
  Member1 = 10,
  Member2 = 100,
  Member3 = 1000
]

enum EBadNum {
  Num1 = 1,
  Num2 = 2,
  Num3 = !
}

enum EBadString {
  Str1 = "str1",
  Str2 = "str2",
  Str3 = ?
}

enum EMissingComma {
  Member1
  Member2
}

enum ETrailingComma {
  Str1 = "str1",
}

enum MissingLeftBrace
  Member1,
  Member2
}



/* @@? 16:5 Error Syntax error ESY0126: Variable declaration expected. */
/* @@? 22:22 Error Syntax error ESY0228: Unexpected token, expected '{'. */
/* @@? 32:1 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 32:1 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 37:10 Error Semantic error ESE0342: Enumeration members can be initialized only by compile-time expressions and initializers must be of the same type. */
/* @@? 38:1 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 40:1 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 43:10 Error Syntax error ESY0227: Unexpected token '?'. */
/* @@? 43:10 Error Semantic error ESE0342: Enumeration members can be initialized only by compile-time expressions and initializers must be of the same type. */
/* @@? 48:3 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 48:3 Error Semantic error ESE0143: Unresolved reference Member2 */
/* @@? 49:1 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 56:3 Error Syntax error ESY0228: Unexpected token, expected '{'. */
/* @@? 56:10 Error Syntax error ESY0224: Identifier expected, got ','. */
/* @@? 57:3 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 58:1 Error Syntax error ESY0227: Unexpected token '}'. */