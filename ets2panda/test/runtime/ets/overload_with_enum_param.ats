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

enum Myenum{
    A,
    B
}

function foo(a: Myenum): string {
    return "Enum";
}

function foo(a: number): number {
    return 42;
}

export default function function_overload_with_enum(): number {

    let result1 = foo(Myenum.A);
    let result2 = foo(10);

    let s1 = (result1 === "Enum");
    let s2 = (result2 === 42);

    return s1 && s2 ? 0 : 1;
}

function main() {
    arktest.assertEQ(function_overload_with_enum(), 0)
}
