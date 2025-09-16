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

class Required {
    i: int
}

class Readonly {
    i: int
}

class Partial {
    i: int
}

function main() {
    let i : Required = new Required()
    let j : Readonly = new Readonly()
    let k : Partial = new Partial()
    let l = new Required<Required>()
    let m = new Readonly<Readonly>()
    let n = new Partial<Partial>()
}

/* @@? 16:16 Error SyntaxError: 'Required' is reserved and cannot be used as a variable/type name */
/* @@? 20:16 Error SyntaxError: 'Readonly' is reserved and cannot be used as a variable/type name */
/* @@? 24:15 Error SyntaxError: 'Partial' is reserved and cannot be used as a variable/type name */
/* @@? 29:13 Error SyntaxError: 'Required' is reserved and cannot be used as a variable/type name */
/* @@? 30:13 Error SyntaxError: 'Readonly' is reserved and cannot be used as a variable/type name */
/* @@? 31:13 Error SyntaxError: 'Partial' is reserved and cannot be used as a variable/type name */
/* @@? 32:17 Error TypeError: Invalid type reference. */
/* @@? 32:26 Error SyntaxError: 'Required' is reserved and cannot be used as a variable/type name */
/* @@? 33:17 Error TypeError: Invalid type reference. */
/* @@? 33:26 Error SyntaxError: 'Readonly' is reserved and cannot be used as a variable/type name */
/* @@? 34:17 Error TypeError: Invalid type reference. */
/* @@? 34:25 Error SyntaxError: 'Partial' is reserved and cannot be used as a variable/type name */
