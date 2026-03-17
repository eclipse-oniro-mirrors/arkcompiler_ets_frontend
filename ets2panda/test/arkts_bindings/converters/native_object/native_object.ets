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

import { EtsNativePointer, TestNativeModule } from '../native';

export function testNativeObject(): void {
    const nativeModule = TestNativeModule.nativeModule;
    const pairPtr: EtsNativePointer = nativeModule._CreatePair(10, 20);
    nativeModule._ValidatePair(pairPtr, 0);
    nativeModule._ModifyPair(pairPtr, 30, 40);
    nativeModule._ValidatePair(pairPtr, 1);
    nativeModule._DestroyPair(pairPtr);
}