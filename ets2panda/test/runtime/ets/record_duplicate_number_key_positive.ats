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

function main(): void {
    let record: Record<number, string> = {
        1: "one",
        2.0: "two",
        3.5: "three point five",
    }

    arktest.assertEQ(record.get(1), "one")
    arktest.assertEQ(record.get(1.0), "one")
    arktest.assertEQ(record.get(2), "two")
    arktest.assertEQ(record.get(2.0), "two")
    arktest.assertEQ(record.get(3.5), "three point five")
}
