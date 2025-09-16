/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

let classVal: double = 1.1;

class A {
    public static classVal: int = 42;
    private readonly instanceVal: int;

    constructor(val: int) {
        this.instanceVal = val;
    }

    public final getInstanceVar(): int {
        A.classVal++;
        classVal++;
        return this.instanceVal;
    }
}

final class B extends A {
    constructor() {
        super(69);
        B.classVal++;
    }
}

function main(): void {
    classVal = 1.1;
    A.classVal = 42;
    arktest.assertEQ(A.classVal, 42)
    const b = new B();
    arktest.assertEQ(A.classVal, 43)
    arktest.assertEQ(classVal, 1.1)
    arktest.assertEQ(b.getInstanceVar(), 69)
    arktest.assertEQ(classVal, 2.1)
    arktest.assertEQ(A.classVal, 44)
}
