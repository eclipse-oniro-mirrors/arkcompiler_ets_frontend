/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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


function force(thunk:() => number) {
    return thunk();
}

function testConst(): void {
    const c1 = 1;
    const f1 = () => {
        const c2 = c1;
        return force(() => {
            const c3 = c2;
            return force(() => {
                const c4 = c3;
                return c1 + c2 + c3 + c4;
            });
        });
    };
    arktest.assertEQ(4, force(f1))
}

function testConstShadow(): void {
    const a = 1;
    arktest.assertEQ(3, force(() => {
        const a = 2;
        return force(() => {
            const a = 3;
            return force(() => a);
        })
    }))
}

function main(): void {
    testConst();
    testConstShadow();
}
