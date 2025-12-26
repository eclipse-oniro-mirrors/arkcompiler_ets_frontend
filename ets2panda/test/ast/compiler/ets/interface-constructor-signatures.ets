/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

//arkts-no-ctor-signatures-iface

interface I {
    new (s: string): I
}

function fn(i: I) {
    return new i("hello")
}

/* @@? 19:5 Error Syntax error ESY81515: Constructor signatures are not supported in interfaces, use methods instead! */
/* @@? 19:5 Error Syntax error ESY0224: Identifier expected, got 'new'. */
/* @@? 19:9 Error Semantic error ESE0371: Cannot find type 's'. */
/* @@? 19:11 Error Syntax error ESY0224: Identifier expected, got 'end of stream'. */
/* @@? 19:11 Error Syntax error ESY0228: Unexpected token, expected ','. */
/* @@? 19:19 Error Syntax error ESY0119: Interface fields must have type annotation. */
/* @@? 19:20 Error Syntax error ESY0224: Identifier expected, got 'end of stream'. */
/* @@? 19:20 Error Syntax error ESY0138: Invalid Type. */
/* @@? 19:20 Error Syntax error ESY0228: Unexpected token, expected ','. */
/* @@? 20:1 Error Syntax error ESY0138: Invalid Type. */
/* @@? 23:16 Error Semantic error ESE0371: Cannot find type 'i'. */

