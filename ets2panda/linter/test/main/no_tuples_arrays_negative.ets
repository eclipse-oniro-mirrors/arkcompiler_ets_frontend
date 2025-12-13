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
    tuple: [number, string, boolean] = [1, 'hi', true]

    val: (number|string|boolean) [] = new A().tuple // error tuple is not an array
    A() {
        const tuple2: [string, number, boolean] = ['s', 3.14, true]
        let array2: (string | number | boolean)[] = tuple2 //error tuple is not an array

    }
    cc(): [boolean, number, boolean] {
        return [true, 33, false];
    }

    ee() {
        let ccVal: (boolean | number | boolean)[] = this.cc() //error cc return an tuple which is not a same type as ccVal array
        return ccVal;
    }
}

const tuple: [number, number, boolean] = [1, 3.14, true]
const array: (number | boolean)[] = tuple //error tuple is not an array
