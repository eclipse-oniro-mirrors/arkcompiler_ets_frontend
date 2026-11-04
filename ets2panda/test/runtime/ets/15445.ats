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

class A {
    public static foo<A1>(fn: (a1: A1) => void, arg: A1): void {
        fn(arg)
    }
    public static foo<A1, A2>(fn: (a1: A1, a2: A2) => void, arg1: A1, arg2: A2): void {
        fn(arg1, arg2)
    }
}

function t1(s: String): void {
}

function t2(s: String, n: int): void {
}

function main(): int {
    A.foo(t1, "lll");
    A.foo(t2, "llll", 1)
    return 0
}
