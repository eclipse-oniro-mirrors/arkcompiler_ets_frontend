/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

    arktest.assertTrue(objectArray instanceof FixedArray<Object|undefined>)
    arktest.assertTrue(boolArray instanceof FixedArray<boolean|undefined>)
    arktest.assertTrue(intArray instanceof FixedArray<int|undefined>)
    arktest.assertTrue(doubleArray instanceof FixedArray<number|undefined>)
    arktest.assertTrue(stringArray instanceof FixedArray<string|undefined>)
    arktest.assertTrue(classArray instanceof FixedArray<Item|undefined>)
    arktest.assertTrue(unionArray instanceof (FixedArray<boolean|number|undefined|Item|string>))
}
