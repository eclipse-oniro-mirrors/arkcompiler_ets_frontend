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
interface Padding {
    top?: Length;
    right?: Length;
    bottom?: Length;
    left?: Length;
}

type Length = string | number | Resource;

interface Resource {
    bundleName: string;
    moduleName: string;
    id: number;
    type?: number;
}

interface LocalizedPadding {
    top?: LengthMetrics;
    end?: LengthMetrics;
    bottom?: LengthMetrics;
    start?: LengthMetrics;
}

class LengthMetrics {
    constructor(value: number, unit?: LengthUnit){}
    static px(value: number){}
    static vp(value: number){}
    static fp(value: number){}
    static percent(value: number){}
    static lpx(value: number){}
    static resource(value: Resource){}
    public unit: LengthUnit = LengthUnit.PX;
    public value: number = 1;
}
enum LengthUnit {
    PX = 0,
    VP = 1,
    FP = 2,
    PERCENT = 3,
    LPX = 4
}

let a: LengthMetrics = new LengthMetrics(1, LengthUnit.PX);
let status: string = "";
function padding(value: Padding | Length | LocalizedPadding){
    if (value instanceof Length) {
        status = "Length"
    }

    if (value instanceof Padding) {
        status = "Padding"
    }

    if (value instanceof LocalizedPadding) {
        status = "LocalizedPadding"
    }
}
padding(1);
arktest.assertTrue(status == "Length")
padding({top: 1});
arktest.assertTrue(status == "Padding")
padding({top: a});
arktest.assertTrue(status == "LocalizedPadding")
