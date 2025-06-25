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

class A<T> {
    x: FixedArray<Any>
    constructor(x: T) {
        this.x = new Any[1]
        this.x[0] = x as Any
    }

    static $_invoke<T>(first: T, ...items: T[]): A<T> {
        return new A<T>(first)
    }

    public get(): T {
        return this.x[0] as T
    }
}

let x = A<number>(1)

arktest.assertEQ(x.get(), 1)
