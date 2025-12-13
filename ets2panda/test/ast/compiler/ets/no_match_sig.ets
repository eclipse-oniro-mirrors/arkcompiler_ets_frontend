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

function main() {
    foo()
}

function foo1(a: number, b: number) {
    return "aaaa"
}

function foo2(a: number, ...args: number[]) {
    return "bbbb"
}

function foo3(a: number, b: number, c?: number) {
    return "cccc"
}

overload foo{ foo1, foo2, foo3 }

/* @@? 17:5 Error Semantic error ESE0127: No matching call signature for foo() */