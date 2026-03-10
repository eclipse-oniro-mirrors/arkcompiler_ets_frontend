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

let sideEffectCtorCalls = 0;

class SideEffectBox {
    value: number;

    constructor(value: number) {
        sideEffectCtorCalls++;
        this.value = value;
    }
}

class FlagBox {
    flag: boolean;

    constructor(flag: boolean) {
        this.flag = flag;
    }
}

function sideEffectConditionalValue(): number {
    let before = sideEffectCtorCalls;
    let value: number = new SideEffectBox(1) ? 1 : 0;
    arktest.assertEQ(sideEffectCtorCalls, before + 1);
    return value;
}

function optionalFlagConditionalIndex(): number {
    let arr = [390, 400];
    return arr[(new FlagBox(true)?.flag) ? 1 : 0];
}

function optionalFlagConditionalAssignment(): number {
    let value: number = (new FlagBox(false)?.flag) ? 9 : 10;
    return value;
}

function main(): void {
    arktest.assertEQ(sideEffectConditionalValue(), 1);
    arktest.assertEQ(optionalFlagConditionalIndex(), 400);
    arktest.assertEQ(optionalFlagConditionalAssignment(), 10);
}
