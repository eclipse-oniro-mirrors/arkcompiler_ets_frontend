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

interface I{
    i: number;
    j: string;
    k: number;
}

interface I2{
    i?: number;
    j: string;
    k?: number;
}

function main() {
    let x: Required<I> = {i: 1.9, j :"some str", k:10};
    arktest.assertEQ(x.i, 1.9)
    arktest.assertEQ(x.j, "some str")
    arktest.assertEQ(x.k, 10)

    let x2: Required<I2> = {i: 1.9, j :"some str", k:10};
    arktest.assertEQ(x2.i, 1.9)
    arktest.assertEQ(x2.j, "some str")
    arktest.assertEQ(x2.k, 10)
}
