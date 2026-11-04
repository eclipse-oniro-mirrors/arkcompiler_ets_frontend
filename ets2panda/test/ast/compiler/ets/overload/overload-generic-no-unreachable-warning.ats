/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function f1(a: string) {
    return 1;
}
function f2<T extends string>(a: T) {
    return 2;
}

function f3(a: number) {
    return 3;
}
function f4<T extends number>(a: T) {
    return 4;
}

overload foo1{ f1, f2 }
overload foo2{ f3, f4 }

function main(): void {
    arktest.assertEQ(foo1("aa"), 1)
    arktest.assertEQ(foo1<string>("aa"), 2)
    arktest.assertEQ(foo2(1), 3)
    arktest.assertEQ(foo2<number>(1), 4)
}
