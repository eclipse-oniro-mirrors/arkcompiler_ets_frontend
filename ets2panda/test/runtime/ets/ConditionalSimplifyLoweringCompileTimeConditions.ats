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

class TruthyIndexSelector {}

enum Color {
    Red,
    Green,
    Blue
}

let sideEffectCtorCalls = 0;

class SideEffectCtor {
    constructor() {
        sideEffectCtorCalls++;
    }
}

function knownNonNullishOptionalIndexValue(): string {
    let result: string[] = ["ok"];
    return result?.[0];
}

function truthySelectorConditionalIndex(): number {
    let selector = new TruthyIndexSelector();
    let arr = [1, 2];
    return arr[selector ? 1 : ""];
}

function compileTimeTrueConditionalPreservesSideEffect(): number {
    let before = sideEffectCtorCalls;
    let value: number = (new SideEffectCtor() ? 1 : 2);
    arktest.assertEQ(sideEffectCtorCalls, before + 1);
    return value;
}

function enumConditionalInRelationalExpression(): void {
    arktest.assertTrue(Color.Red < (true ? Color.Green : Color.Red));
    arktest.assertTrue(!((true ? Color.Green : Color.Red) > Color.Blue));
}

function main(): void {
    arktest.assertEQ(knownNonNullishOptionalIndexValue(), "ok");
    arktest.assertEQ(truthySelectorConditionalIndex(), 2);
    arktest.assertEQ(compileTimeTrueConditionalPreservesSideEffect(), 1);
    enumConditionalInRelationalExpression();
}
