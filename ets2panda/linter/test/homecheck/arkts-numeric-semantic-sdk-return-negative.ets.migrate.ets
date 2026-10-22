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

import { batteryInfo, systemDateTime, rpc } from '@ohos.int';

class AssignWithSDKReturn {
    directlyAssignWithSDKReturn(): void {
        let a1 = batteryInfo.setBatteryConfig('a1', 'a11'); // arkts-numeric-semantic
        let a2 = systemDateTime.getTime(); // arkts-numeric-semantic
        let a3 = systemDateTime.getTime(true); // arkts-numeric-semantic
    }

    returnValueUsedAsInt(): void {
        let b1 = 1; // arkts-numeric-semantic
        b1 = batteryInfo.setBatteryConfig('b1', 'b11');
        let b2 = systemDateTime.getTime(); // arkts-numeric-semantic
        let b4 = 1.1;
        b4 = b2;
    }

    returnValueUsedAsNumber(): void {
        let c1 = batteryInfo.setBatteryConfig('c1', 'c11'); // arkts-numeric-semantic
        let c2 = systemDateTime.getTime(); // arkts-numeric-semantic
        c1 = 1.2;
        let c3 = c2/7; // arkts-numeric-semantic
    }

    foo(param: number): number {
        let arr = [1, 2, 3];
        return arr[param];
    }

    goo(param: number): number {
        return param;
    }

    returnValueUsedAsParam(): void {
        let d1 = batteryInfo.setBatteryConfig('d1', 'd11'); // arkts-numeric-semantic
        let d2 = systemDateTime.getTime(); // arkts-numeric-semantic
        let d3 = this.foo(d1);
        d2 = this.goo(d2);
    }
}

class StaticSdkReturnInt {
    foo(): void {
        let a = rpc.IPCSkeleton.getCallingPid();
        let b = rpc.IPCSkeleton.getCallingTokenId();
    }
}

class Positive {
    annotationAfterFix(): void {
        let a1: int = batteryInfo.setBatteryConfig('a1', 'a11');
        let a2: long = systemDateTime.getTime();
        let a3: long = systemDateTime.getTime(true);
        let a4 = systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE)
    }
}
