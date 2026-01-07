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

class A {
    p: undefined | int | (() => int)
    constructor(
        p?: int | (() => int)
    ) {
        this.p = p;
    }

    invokeP() : int {
        if (this.p instanceof Function) {
            let c = this.p as (() => int)
            return c();
        }
        return 1;
    }
}

function main() {
    let a = new A(()=>12);
    arktest.assertEQ(a.invokeP(), 12);
}
