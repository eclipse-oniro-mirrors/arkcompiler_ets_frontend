/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

function assertEntries(entries: [string, string][], expectedKey: string, expectedValue: string): void {
    arktest.assertEQ(entries.length, 1)
    arktest.assertEQ(entries[0][0], expectedKey)
    arktest.assertEQ(entries[0][1], expectedValue)
}

function assertSpreadEntries(expectedKey: string, expectedValue: string, ...entries: [string, string][]): void {
    assertEntries(entries, expectedKey, expectedValue)
}

function main(): void {
    let r1: Record<string, string> = { 'one': '1' }
    assertSpreadEntries('one', '1', ...r1)

    let r2: Record<string, string> = { 'two': '2' }
    let r3: Record<string, string> = { ...r1, ...r2 }

    arktest.assertEQ(r3['one'], '1')
    arktest.assertEQ(r3['two'], '2')

    let copied = [...r3]
    arktest.assertEQ(copied.length, 2)
    arktest.assertEQ(copied[0][0], 'one')
    arktest.assertEQ(copied[0][1], '1')
    arktest.assertEQ(copied[1][0], 'two')
    arktest.assertEQ(copied[1][1], '2')
}
