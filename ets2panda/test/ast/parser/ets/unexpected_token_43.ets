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

let v2 = `--- ${y + abc /* @@ label */${y} = ${ n*2 }!`/* @@ label1 */} ---`;`


/* @@? 16:17 Error Semantic error ESE0143: Unresolved reference y */
/* @@? 16:21 Error Semantic error ESE0143: Unresolved reference abc */
/* @@@ label Error Syntax error ESY0230: Expected '}', got 'identification literal'. */
/* @@? 16:49 Error Semantic error ESE0143: Unresolved reference n */
/* @@@ label1 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@@ label1 Error Semantic error ESE0107: Bad operand type, the types of the operands must be numeric type. */