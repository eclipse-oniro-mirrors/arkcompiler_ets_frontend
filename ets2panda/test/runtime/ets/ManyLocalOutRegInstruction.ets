/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

// Main goal is to test that bytecode is valid
function main(): int {
    let x0 = 0;
    let x1 = 1;
    let x2 = 2;
    let x3 = 3;
    let x4 = 4;
    let x5 = 5;
    let x6 = 6;
    let x7 = 7;
    let x8 = 8;
    let x9 = 9;
    let x10 = 10;
    let x11 = 11;
    let x12 = 12;
    let x13 = 13;
    let x14 = 14;
    let x15 = 15;
    let x16 = 16;
    let x: FixedArray<int> = new FixedArray<int>(10);
    return x[0];
}
