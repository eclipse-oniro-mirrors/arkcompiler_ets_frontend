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

function id<T>(v: Object): T {
    return v as T
}

function f1(a: "aa") {
    return a as "bb"
}

function f2(a: "aa"): "bb" {
    return a as "bb"
}

function main() {
    let s1 = id<"a" | "b" | "c">("d")
    let s = id<"a" | "b" | "c">("a")
    arktest.assertEQ(s1 + s, "da")

    s as "b"
    arktest.assertEQ((s as "c") == "c", false)
    f1("aa");
    f2("aa");
}
