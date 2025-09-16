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

import * as xx1 from "./file1"

export namespace AA {
    export interface AAA1 {
        mode: int
    }
    export class AAA2 implements AAA1 {
        mode: int
        constructor() {
            this.mode = 1;
        }
    }
}

function main() {
    let a = new AA.AAA2();
    arktest.assertEQ(a.mode, 1);
}
