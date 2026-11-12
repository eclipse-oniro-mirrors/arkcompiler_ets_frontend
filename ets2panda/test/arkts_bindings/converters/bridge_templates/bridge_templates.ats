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

import { TestNativeModule } from '../native';

function testVoidBridges(): void {
    const nativeModule = TestNativeModule.nativeModule;
    nativeModule._Bridge_V1(1);
    nativeModule._Bridge_V2(1, 2);
    nativeModule._Bridge_V3(1, 2, 3);
    nativeModule._Bridge_V4(1, 2, 3, 4);
    nativeModule._Bridge_V5(1, 2, 3, 4, 5);
    nativeModule._Bridge_V6(1, 2, 3, 4, 5, 6);
    nativeModule._Bridge_V7(1, 2, 3, 4, 5, 6, 7);
    nativeModule._Bridge_V8(1, 2, 3, 4, 5, 6, 7, 8);
    nativeModule._Bridge_V9(1, 2, 3, 4, 5, 6, 7, 8, 9);
    nativeModule._Bridge_V10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    nativeModule._Bridge_V11(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
    nativeModule._Bridge_V12(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    nativeModule._Bridge_V13(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    nativeModule._Bridge_V14(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
    nativeModule._Bridge_V15(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
}

function testNonVoidBridges(): void {
    const nativeModule = TestNativeModule.nativeModule;
    arktest.assertEQ(nativeModule._Bridge_0(), 0);
    arktest.assertEQ(nativeModule._Bridge_1(1), 1);
    arktest.assertEQ(nativeModule._Bridge_2(1, 2), 1 + 2);
    arktest.assertEQ(nativeModule._Bridge_3(1, 2, 3), 1 + 2 + 3);
    arktest.assertEQ(nativeModule._Bridge_4(1, 2, 3, 4), 1 + 2 + 3 + 4);
    arktest.assertEQ(nativeModule._Bridge_5(1, 2, 3, 4, 5), 1 + 2 + 3 + 4 + 5);
    arktest.assertEQ(nativeModule._Bridge_6(1, 2, 3, 4, 5, 6), 1 + 2 + 3 + 4 + 5 + 6);
    arktest.assertEQ(nativeModule._Bridge_7(1, 2, 3, 4, 5, 6, 7), 1 + 2 + 3 + 4 + 5 + 6 + 7);
    arktest.assertEQ(nativeModule._Bridge_8(1, 2, 3, 4, 5, 6, 7, 8), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8);
    arktest.assertEQ(nativeModule._Bridge_9(1, 2, 3, 4, 5, 6, 7, 8, 9), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9);
    arktest.assertEQ(nativeModule._Bridge_10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10);
    arktest.assertEQ(nativeModule._Bridge_11(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11);
    arktest.assertEQ(nativeModule._Bridge_12(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12);
    arktest.assertEQ(nativeModule._Bridge_13(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13);
    arktest.assertEQ(nativeModule._Bridge_14(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14);
    arktest.assertEQ(nativeModule._Bridge_15(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15);
}

export function testBridgeTemplates(): void {
    testVoidBridges();
    testNonVoidBridges();
}