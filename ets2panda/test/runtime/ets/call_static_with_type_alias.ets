/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class C1 {
    static test(p: number): number {
        return p + 1;
    }
}

class C2 {
    static test(p: number): number {
        return p + 2;
    } 
}

type TC1 = C1;
type TTC1 = TC1;

type TC2 = C2;
type TTC2 = TC2;

function main(): void {
    arktest.assertEQ(C1.test(20), 21)
    arktest.assertEQ(TC1.test(20), 21)
    arktest.assertEQ(TTC1.test(20), 21)

    arktest.assertEQ(C2.test(20), 22)
    arktest.assertEQ(TC2.test(20), 22)
    arktest.assertEQ(TTC2.test(20), 22)
}
