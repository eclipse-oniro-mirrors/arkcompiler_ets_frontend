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

function foo(a : (b: int) => int) {
    arktest.assertEQ(a(3), 3)
}

function bar(a : (b: int, c: string) => int) {
    arktest.assertEQ(a(4, "c"), 6)
}

function main() {
    let a = (b: int, c: string = "b"): int => {
        if (c == "b") {
            return b;
        } else if (c == "a") {
            return b + 1;
        } else if (c == "c"){
            return b + 2;
        } else {
            arktest.assertTrue(false)
        }

        return 0;
    };

    arktest.assertEQ(a(1), 1)
    arktest.assertEQ(a(2,"c"), 4)
    foo(a);
    bar(a);
}
