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

class A {
    value: number;
    constructor(v: number) {
        this.value = v;
    }
    equals(other: A): boolean {
        return this.value == other.value;
    }
}
final class B extends A {
    valueB: number;
    constructor(v: number) {
        super(v);
    }
    equals(other: B): boolean {
        return this.valueB == other.valueB;
    }
}

function test() {
    let str1 = "1";
    let str2 = "2";
    let d0: number = 0, d1: number = 1, d2: number = 2, d3: number = 3, d4: number = 4, d5: number = 5,
        d6: number = 6, d7: number = 7, d8: number = 8, d9: number = 9,
        d10: number = 10, d11: number = 11, d12: number = 12, d13: number = 13, d14: number = 14, d15: number = 15,
        d16: number = 16, d17: number = 17, d18: number = 18, d19: number = 19,
        d20: number = 20, d21: number = 21, d22: number = 22, d23: number = 23, d24: number = 24, d25: number = 25,
        d26: number = 26, d27: number = 27, d28: number = 28, d29: number = 29,
        d30: number = 30;
    arktest.assertEQ(str1 == str2, false);
    arktest.assertEQ(new B(1) == new B(2), false);
}

test();