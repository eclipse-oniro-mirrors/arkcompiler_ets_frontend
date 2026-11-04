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

function compareCharUnion(val: int, expected: boolean) {
    const c: char = c'\uFFFF'
    const u: char | int = val
    arktest.assertEQ((c == u), expected)
    arktest.assertEQ((u == c), expected)
}

function main() {
    compareCharUnion(65535, true)
    compareCharUnion(-1, false)
    compareCharUnion(0, false)
    compareCharUnion(0xFFFF, true)

    const c: char = c'\uFFFF'
    const i: int = 65535
    arktest.assertEQ((c == i), true)
    arktest.assertEQ((i == c), true)
}
