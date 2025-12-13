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

let s = "abc"
class A {
    $_set(idx: int, val: number) {
        s = "$_set1 " + idx.toString() + " " + val.toString();
    }

    $_set(idx: int, val: string) {
        s = "$_set2 " + idx.toString() + " " + val;
    }
}

function main() {
    let a = new A();


    a[1] = 1234
    const result1 = "$_set1 1 1234"
    arktest.assertEQ(s, result1)

    a[2] = "aaa"
    const result2 = "$_set2 2 aaa"
    arktest.assertEQ(s, result2)
}
