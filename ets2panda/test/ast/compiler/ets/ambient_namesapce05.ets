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

declare namespace MySpace{
    interface A{
        a:int = 1
    }
}

/* @@? 18:15 Error SyntaxError: Interface member initialization is prohibited. */
/* @@? 18:17 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 18:17 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 19:5 Error SyntaxError: Identifier expected. */
