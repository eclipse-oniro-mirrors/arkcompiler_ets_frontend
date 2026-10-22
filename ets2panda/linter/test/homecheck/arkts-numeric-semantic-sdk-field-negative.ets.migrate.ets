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

import { image, outerNS, rpc } from '@ohos.int';

class AssignWithSDKField {
    directlyAssignWithSDKField(): void {
        let a1 = image.Size.width; // arkts-numeric-semantic
        let a2 = image.Size.height; // arkts-numeric-semantic
        let a3 = image.Image.timestamp; // arkts-numeric-semantic
        let a4 = outerNS.innerNS.MAX_LENGTH; // arkts-numeric-semantic
        let a5 = outerNS.innerNS1.innerNS2.MIN; // arkts-numeric-semantic
    }

    fieldValueUsedAsInt(): void {
        let b1 = 1; // arkts-numeric-semantic
        b1 = image.Size.height;
        let b2 = image.Image.timestamp; // arkts-numeric-semantic
        let b4 = 1.1;
        b4 = b2;
    }

    fieldValueUsedAsNumber(): void {
        let c1 = image.Size.width; // arkts-numeric-semantic
        let c2 = outerNS.innerNS.MAX_LENGTH; // arkts-numeric-semantic
        c1 = 1.2;
        let c3 = c2/7; // arkts-numeric-semantic
    }

    foo(param: number): number {
        return param;
    }

    goo(param: number): number {
        return param;
    }

    fieldValueUsedAsParam(): void {
        let d1 = image.Size.width; // arkts-numeric-semantic
        let d2 = this.foo(d1);
        let d3 = this.goo(image.Image.timestamp);
    }

    fieldFromInstance(): void {
        let aso = new rpc.AtomicServiceOptions();
        let aso1 = aso.flags;
    }
}

class Positive {
    annotationAfterFix(): void {
        let a1: int = image.Size.width;
        let a2: int = image.Size.height;
        let a3: long = image.Image.timestamp;
        let a4: long = outerNS.innerNS.MAX_LENGTH;
        let a5: int = outerNS.innerNS1.innerNS2.MIN;
    }
}
