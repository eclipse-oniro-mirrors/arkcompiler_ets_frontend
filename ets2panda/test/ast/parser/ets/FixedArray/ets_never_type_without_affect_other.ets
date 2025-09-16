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
function bar(p1: number) {}
function foo(): number {
    let n: number|undefined = undefined
    if (n != undefined) {
        let a: Number = n
        bar(n)
        let b: FixedArray<number> = [2.7, n, 3.14]
        let c = ((p: number): number => p + 1)(n)
        let d = n * 2
        return n;
    }
    return n!
}

/* @@? 23:17 Error TypeError: Bad operand type, the types of the operands must be numeric type. */
/* @@? 26:12 Error Warning: Bad operand type, the operand of the non-nullish expression is 'null' or 'undefined'. */
