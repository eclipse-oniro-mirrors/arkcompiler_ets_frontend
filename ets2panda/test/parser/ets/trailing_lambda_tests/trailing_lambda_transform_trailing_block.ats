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

function f(c: ()=>void): int {
    c();
    return 1;
}

function main() {
    // Case 1: transform trailing lambda for expression statement
    f() {
        console.println("I am call back!")
    }

    f()
    {
        console.println("I am call back!")
    }

    // Case 2: transform trailing lambda for variable declaration
    let a: int = f() {}

    let b: int = f()
    {}

    // Case 3: transform trailing lambda for arktest.assert statement
    arktest.assertEQ(1, f(){});
    arktest.assertEQ(1, f(()=>{}));
    {
    }
}
