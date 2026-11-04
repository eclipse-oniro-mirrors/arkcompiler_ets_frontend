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

class Fruit<T> {
    data1: T;
    data2: T;

    constructor(d1: T, d2: T) {
        this.data1 = d1;
        this.data2 = d2;
    }

    name(): T {
        return this.data2
    }
}

function name<T>(this:Fruit<T>, p:T) : T{
    return this.name();
}

function main() {
    let f = new Fruit<Int>(1, 2);
    arktest.assertEQ(f.name(1), 2)
    arktest.assertEQ(new Fruit<Int>(2, 3).name(1), 3)
    arktest.assertEQ(name(f, 1), 2)
    arktest.assertEQ(name(new Fruit<Int>(2, 3), 1), 3)
}