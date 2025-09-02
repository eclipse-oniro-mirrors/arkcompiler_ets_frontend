/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function conversion1(x: int, y: int): Object {
    return x+y;
}

function conversion2(x: int): long {
    let sum: long = 0;
    sum += x;
    return sum;
}

function conversion3(x: int): long {
    let sum: long = 0;
    sum = x;
    return sum;
}

function main() : void {
    arktest.assertEQ(conversion1(1,2) as Int, 3)
    arktest.assertEQ(conversion2(5), (5 as long))
    arktest.assertEQ(conversion3(7), (7 as long))
}
