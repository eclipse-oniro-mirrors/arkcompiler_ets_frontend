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

function foo(arg: number|undefined) {
    arg!++;
    return arg;
}

function foo1(arg: number|undefined) {
    arg!--;
    return arg;
}

function main(): void {
    let a = foo(1);
    assertEQ(a, 2.0)
    let b = foo1(2);
    assertEQ(b, 1.0)
}