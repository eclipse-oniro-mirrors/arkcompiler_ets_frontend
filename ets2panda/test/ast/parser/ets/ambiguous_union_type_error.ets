/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

type AliasAmbiguous = string | /* @@ label1 */() => string | number;

function foo<T>(): void {}

function main(): void {
    let a: string | /* @@ label2 */() => string | number = () => {
        return 1;
    };

    foo<string | /* @@ label3 */() => string | number>();
}

/* @@@ label1 Error Syntax error ESY113981: Using '() => string | number' function type notation in a union causes ambiguity. Parenthesize the function type or its return type to make the meaning explicit. */
/* @@@ label2 Error Syntax error ESY113981: Using '() => string | number ' function type notation in a union causes ambiguity. Parenthesize the function type or its return type to make the meaning explicit. */
/* @@@ label3 Error Syntax error ESY113981: Using '() => string | number' function type notation in a union causes ambiguity. Parenthesize the function type or its return type to make the meaning explicit. */
