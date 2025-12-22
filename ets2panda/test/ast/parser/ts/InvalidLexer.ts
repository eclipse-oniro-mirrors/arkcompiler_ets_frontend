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

/foo/bar ;

/foo/gx ;

/foo
/g ;

yield

funct\u{0069}on

`string ${expr


/* @@? 16:2 Error Syntax error ESY0246: Invalid RegExp flag. */
/* @@? 16:7 Error Syntax error ESY0227: Unexpected token 'ar'. */
/* @@? 18:2 Error Syntax error ESY0246: Invalid RegExp flag. */
/* @@? 20:2 Error Syntax error ESY0260: Unterminated RegExp. */
/* @@? 20:5 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 20:5 Error Syntax error ESY0227: Unexpected token 'end of stream'. */
/* @@? 22:1 Error Syntax error ESY0264: Unexpected strict mode reserved keyword. */
/* @@? 24:1 Error Syntax error ESY0271: Escape sequences are not allowed in keyword. */
/* @@? 40:83 Error Syntax error ESY0230: Expected '}', got 'end of stream'. */
/* @@? 40:83 Error Syntax error ESY0253: Unterminated string. */
/* @@? 40:83 Error Syntax error ESY0259: Unexpected token, expected '${' or '`' */
/* @@? 40:83 Error Syntax error ESY0228: Unexpected token, expected '`'. */