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

type Callback<T, C = void, V = void> = (data: T, data2: C) => V;

class A {
    a: Callback<string, int>

    constructor() {
        this.a = (data: string, data2: int) => {
            arktest.assertEQ(data, "Hello");
            arktest.assertEQ(data2, 1);
        };
    }

}

function main() {
    let a = new A();
    a.a("Hello", 1);
    a.a = (data: string, data2: int) => {
        arktest.assertEQ(data, "World");
        arktest.assertEQ(data2, 2);
    };
    a.a("World", 2);
}
