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
    a: number = -11;
    tuple: [number, string, boolean] = [1, 'hi', true]
    arrs: [[number, number, boolean], [boolean, number, boolean]] = [
        [1, 3.14, true], this.cc()
    ]
    aa(): number {
        let ss: number[] = [-1, -5]
        return ss[0];
    }
    cc(): [boolean, number, boolean] {
        return [true, 33, false];
    }
    dd(): [boolean, string, A] {
        return [true, 'hello', new A()];
    }
    gg() {
        return this.arrs[0];
    }
    ff() {
        let array = tuple;
        return array;
    }
}

let ff = new A().ff()
let hh: (boolean | number | boolean)[] =ff
