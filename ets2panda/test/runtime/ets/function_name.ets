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

let global_f1: Function = (): void => {}
let global_f2: Function;
global_f2 = global_f1;
let global_f3: Function;
global_f3 = (): void => {}

function main(): void {
    arktest.assertEQ(global_f1.name, "global_f1")
    arktest.assertEQ(global_f2.name, "global_f1")
    arktest.assertEQ(global_f3.name, "global_f3")

    let f1: Function = (): void => {}
    arktest.assertEQ(f1.name, "f1")

    let f2: Function;
    f2 = f1;
    arktest.assertEQ(f2.name, "f1")

    let f3: Function;
    f3 = (): void => {}
    arktest.assertEQ(f3.name, "f3")
}
