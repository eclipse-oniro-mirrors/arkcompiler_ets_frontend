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

let tup : [['arkTs' | boolean, 1 | true]] = [['arkTs', 1]];

/* @@? 16:32 Error SyntaxError: Invalid Type. */
/* @@? 16:32 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 16:32 Error SyntaxError: Unexpected token '1'. */
/* @@? 16:32 Error TypeError: Wrong type of operands for binary expression */
/* @@? 16:32 Error TypeError: Bad operand type, the types of the operands must be numeric type. */
/* @@? 16:40 Error SyntaxError: Unexpected token ']'. */
/* @@? 16:41 Error SyntaxError: Unexpected token ']'. */
