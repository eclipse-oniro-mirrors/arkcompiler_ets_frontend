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

function main() {
    let arg1: string[] = ["a", "b"]
    let arg2: FixedArray<string> = ["c", "d"]
    let arg3: [string, string] = ["e", "f"]
    let fn = (...args: string[]) => {
        let result = "";
        for (let e of args) {
            result += e;
        }
        return result;
    }
    arktest.assertEQ(fn("a", "b"), "ab");
    arktest.assertEQ(fn(...arg1), "ab");
    arktest.assertEQ(fn(...[...arg1]), "ab");
    arktest.assertEQ(fn("a", "b", ...arg1), "abab");
    arktest.assertEQ(fn(...arg1, ...arg2, "g", ...arg3, "h"), "abcdgefh");
}

