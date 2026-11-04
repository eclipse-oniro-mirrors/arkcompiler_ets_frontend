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

function bar(a: int[] | string[]): void {
    arktest.assertEQ(a.length, 2)
}

function baz(a: int[] | Object[]): void {
    arktest.assertEQ(a.length, 2)
}

function qux(a: Object | int[]): void {
    arktest.assertTrue(a instanceof Array)
}

function main() {
    bar([1, 2])
    baz([1, new Object()])
    qux([1, undefined])
}
