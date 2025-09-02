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

class Item {
    public value: int;

    constructor(v: int)
    {
        this.value = v;
    }
}

function main() {
    let objectArray: FixedArray<Object> = [];
    let boolArray: FixedArray<boolean> = [true, false, true];
    let intArray: FixedArray<int> = [1, 2, 99, 100];
    let doubleArray: FixedArray<number> = [1, -2, 1.2, 9.9];
    let stringArray: FixedArray<string> = ["one", "two", "three"];
    let classArray: FixedArray<Item> = [new Item(1), new Item(2), new Item(3)];
    let unionArray: FixedArray<boolean|number|Item|string> = [true, 9.99, new Item(9), "success"];

    arktest.assertTrue(objectArray instanceof FixedArray<Object>)
    arktest.assertTrue(boolArray instanceof FixedArray<boolean>)
    arktest.assertTrue(intArray instanceof FixedArray<int>)
    arktest.assertTrue(doubleArray instanceof FixedArray<number>)
    arktest.assertTrue(stringArray instanceof FixedArray<string>)
    arktest.assertTrue(classArray instanceof FixedArray<Item>)
    arktest.assertTrue(unionArray instanceof (FixedArray<boolean|number|Item|string>))
}
