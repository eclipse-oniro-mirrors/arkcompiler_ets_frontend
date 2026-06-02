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

enum E: long {
    first,
    second = -2147483648,
    third = second - 1
}

function main(): void {
    arktest.assertTrue(E.first instanceof E)
    arktest.assertTrue(E.second instanceof E)
    arktest.assertTrue(E.third instanceof E)
    arktest.assertEQ(E.first.valueOf(), 0)
    arktest.assertEQ(E.second.valueOf(), -2147483648)
    arktest.assertEQ(E.third.valueOf(), -2147483649)
}
