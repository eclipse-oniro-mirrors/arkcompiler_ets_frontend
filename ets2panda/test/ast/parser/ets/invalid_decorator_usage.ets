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

@ q.M function i ( this : ( this : @ e ( ) [ ] ) => ( @ V ( ) "" ) ) { }

/* @@? 16:3 Error TypeError: Cannot find type 'q'. */
/* @@? 16:5 Error TypeError: 'M' is not an annotation. */
/* @@? 16:5 Error TypeError: 'M' type does not exist. */
/* @@? 16:16 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 16:20 Error SyntaxError: Unexpected 'this' keyword in non-receiver context. */
/* @@? 16:40 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 16:40 Error SyntaxError: Annotation usage must have parentheses when used in front of a parenthesized type. */
/* @@? 16:40 Error SyntaxError: Unexpected token, expected '=>'. */
/* @@? 16:40 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 16:40 Error SyntaxError: Unexpected token '=>'. */
/* @@? 16:42 Error SyntaxError: Unexpected token ')'. */
/* @@? 16:46 Error SyntaxError: Unexpected token ']'. */
/* @@? 16:48 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 16:50 Error SyntaxError: Unexpected token '=>'. */
/* @@? 16:50 Error SyntaxError: Unexpected token. */
/* @@? 16:50 Error SyntaxError: Unexpected token '=>'. */
/* @@? 16:59 Error SyntaxError: Annotations are not allowed on this type of declaration. */
/* @@? 16:61 Error SyntaxError: Unexpected token ')'. */
/* @@? 16:63 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 16:68 Error SyntaxError: Unexpected token ')'. */
/* @@? 16:70 Error SyntaxError: Unexpected token '{'. */