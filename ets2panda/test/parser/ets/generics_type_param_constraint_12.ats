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

interface Unpackable<T> {
    getValue() : T;
}

class Boxx<T> implements Unpackable<T> {
    value_ : T;

    constructor(value : T) {
        this.value_ = value;
    }

    public getValue() : T {
        return this.value_;
    }
}

function printBoxedValue<T>(x : Boxx<T>) {
    console.log("Value is ", x.getValue())
}

function doSomethingUseful() {
    let box1 = new Boxx<number>(123)
    printBoxedValue(box1)
    let box2 = new Boxx<string>("hello")
    printBoxedValue(box2)
}
