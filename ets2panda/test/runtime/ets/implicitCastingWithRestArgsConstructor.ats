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

class A {
    public a: number | undefined;
    constructor (...args: number[]) {
        if (args.length == 0) {
            this.a = undefined;
        }

        let retVal = args[0];
        for (let i = 0; i < args.length; i++) {
            if (args[i] > retVal) {
                retVal = args[i];
            }
        }
        this.a = retVal;
    }
}

function main() {
    let a = 1;
    let val = new A(0, 1 + a);
    arktest.assertEQ(val.a, 2)
}
