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

type None = undefined
type RT<X, Y> = [X, Y, RT<X, Y> | None]

function main(): void {
    let x: RT<string, string> = ['1', '1', ['1', '2', undefined] as RT<string, string>]
    let y = x[2]!
    let z = ['1', '2', undefined]
    arktest.assertEQ(y[0], z[0]);
    arktest.assertEQ(y[1], z[1]);
    arktest.assertEQ(y[2], z[2]);
}