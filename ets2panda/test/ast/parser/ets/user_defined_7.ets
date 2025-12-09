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

interface /* @@ label1 */double {
    name : string /* @@ label2 */= /* @@ label3 */""
/* @@ label4 */}


/* @@@ label1 Error Syntax error ESY0295: double is a predefined type, cannot be used as an identifier */
/* @@@ label2 Error Syntax error ESY0180: Interface member initialization is prohibited. */
/* @@@ label3 Error Syntax error ESY0228: Unexpected token, expected ','. */
/* @@@ label3 Error Syntax error ESY0224: Identifier expected, got 'string literal'. */
/* @@@ label4 Error Syntax error ESY0122: Identifier expected. */
