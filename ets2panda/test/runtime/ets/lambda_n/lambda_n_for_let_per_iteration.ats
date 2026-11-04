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

function case_classic_for_let_closures(): void {
    // case 1: classic for(let i...) + closures
    let out1: String = "";
    const fns1: Array<() => void> = [];
    for (let i = 0; i < 3; i++) {
        fns1.push(() => { out1 += ("" + i); });
    }
    for (const fn of fns1) fn();
    arktest.assertEQ(out1, "012");
}

function case_for_of_per_iteration(): void {
    // case 2: for-of must also be per-iteration
    let out2: String = "";
    const fns2: Array<() => void> = [];
    const arr: number[] = [10, 20, 30];
    for (let x of arr) {
        fns2.push(() => { out2 += ("" + x); });
    }
    for (const fn of fns2) fn();
    arktest.assertEQ(out2, "102030");
}

function case_continue_keeps_binding(): void {
    // case 3: continue should not break binding of this iteration
    let out3: String = "";
    const fns3: Array<() => void> = [];
    for (let i = 0; i < 3; i++) {
        if (i === 1) {
            fns3.push(() => { out3 += "M" + i; });
            continue;
        }
        fns3.push(() => { out3 += "N" + i; });
    }
    for (const fn of fns3) fn();
    arktest.assertEQ(out3, "N0M1N2");
}

function main(): void {
    case_classic_for_let_closures();
    case_for_of_per_iteration();
    case_continue_keeps_binding();
}