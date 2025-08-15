/**
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

interface I {
    f: number;
    readonly f_ro: number;

    // invalid interface property modifiers
    private f_priv: number;
    protected f_prot: number;
    public f_pub: number;
    static f_stat: number;
    default f_def: number;
}

/* @@? 21:5 Error SyntaxError: Identifier expected, got 'private'. */
/* @@? 22:5 Error SyntaxError: Identifier expected, got 'protected'. */
/* @@? 23:5 Error SyntaxError: Identifier expected, got 'public'. */
/* @@? 24:5 Error SyntaxError: Identifier expected, got 'static'. */
/* @@? 25:5 Error SyntaxError: Identifier expected, got 'default'. */
