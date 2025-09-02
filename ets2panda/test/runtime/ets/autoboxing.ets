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

function foo_i(value: Int): string {
    return value.toString();
}

function foo_d(value: Double): string {
    return value.toString();
}

function foo_n(value: Number): string {
    return value.toString();
}

function main(): void {
    let i: int = 4
    let str_i: string = foo_i(i);
    arktest.assertEQ(str_i, "4")

    let d: double = 3.14
    let str_d: string = foo_d(d);
    arktest.assertEQ(str_d, "3.14")

    let n: number = 5
    let str_n: string = foo_n(n);
    arktest.assertEQ(str_n, "5")
}