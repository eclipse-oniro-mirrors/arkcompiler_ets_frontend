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

/**
 * This test simulates a real SDK usage scenario.
 * It verifies that during the recheck phase of `declareOverloadLowering`,
 * the `this` keyword is able to resolve the correct ContainingClass.
 */

declare class A {
    on(type: 'A', cb: () => void): void
    on(type: 'B', cb: () => void): void
}

let a = new A();

class B {
    prop:string = "xxx"
    foo() {
        a.on('B', () => {
            this.prop // test `this` exprssion could be recheck in the lowering
        })
    }
}
