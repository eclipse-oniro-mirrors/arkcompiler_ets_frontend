/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
function bar(p1: number) {}
function foo(): number {
    let n: number|undefined = undefined
    if (n != undefined) {
        let a: Number = n
        bar(n)
        let b: number[] = [2.7, n, 3.14]
        let c = ((p: number): number => p + 1)(n)
        let d = n * 2
        return n;
    }
    return n!
}


/* @@? 18:9 Warning Warning W65001: The equality expression is always false. */
/* @@? 23:17 Error Semantic error ESE0107: Bad operand type, the types of the operands must be numeric type. */
/* @@? 26:12 Warning Warning W0014: Operand of the ensure-not-nullish expression is 'null' or 'undefined'. */
