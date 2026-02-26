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

    getValue(): number {
        return this.value;
    }
}

class NestedBox {
    child: SideEffectBox | null;

    constructor(child: SideEffectBox | null) {
        this.child = child;
    }
}

function optionalPropertyValue(): number {
    let before = sideEffectCtorCalls;
    let value = new SideEffectBox(2)?.value ?? -1;
    arktest.assertEQ(sideEffectCtorCalls, before + 1);
    return value;
}

function optionalMethodValue(): number {
    let before = sideEffectCtorCalls;
    let value = new SideEffectBox(3)?.getValue() ?? -1;
    arktest.assertEQ(sideEffectCtorCalls, before + 1);
    return value;
}

function optionalNullFallback(): number {
    let box: SideEffectBox | null = null;
    return box?.value ?? 4;
}

function optionalMethodNullFallback(): number {
    let box: SideEffectBox | null = null;
    return box?.getValue() ?? 5;
}

function optionalChainedPropertyValue(): number {
    let before = sideEffectCtorCalls;
    let value = new NestedBox(new SideEffectBox(6))?.child?.value ?? -1;
    arktest.assertEQ(sideEffectCtorCalls, before + 1);
    return value;
}

function optionalChainedNullChildFallback(): number {
    let box = new NestedBox(null);
    return box?.child?.value ?? 7;
}

function optionalChainedNullBoxFallback(): number {
    let box: NestedBox | null = null;
    return box?.child?.value ?? 8;
}

function main(): void {
    arktest.assertEQ(optionalPropertyValue(), 2);
    arktest.assertEQ(optionalMethodValue(), 3);
    arktest.assertEQ(optionalNullFallback(), 4);
    arktest.assertEQ(optionalMethodNullFallback(), 5);
    arktest.assertEQ(optionalChainedPropertyValue(), 6);
    arktest.assertEQ(optionalChainedNullChildFallback(), 7);
    arktest.assertEQ(optionalChainedNullBoxFallback(), 8);
}
