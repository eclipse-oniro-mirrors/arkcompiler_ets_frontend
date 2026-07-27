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

// Stdlib builtin types must still be initialized when stdlib bindings are shared (delegated) rather than
// flattened. Exercising number/string/boolean/Object/Array forces them to resolve at check and run time.

function main(): void {
    let d: number = 1.5;
    let i: int = 2;
    let sum: number = d + i;
    let s: string = "sum=" + sum;
    let b: boolean = sum > 0.0;
    let o: Object = new Object();
    let arr = new Array<number>();
    arr.push(sum);

    if (!b || s.length == 0 || arr.length != 1 || o == null) {
        throw new Error("stdlib builtins did not resolve correctly");
    }
}
