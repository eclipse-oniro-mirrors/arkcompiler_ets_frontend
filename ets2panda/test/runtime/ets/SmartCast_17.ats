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

type T2 = number | string;

function test_T2_b(a: T2): void {
    arktest.assertEQ("number", typeof a);

    if (a instanceof Int) { 
        arktest.assertTrue(false);
    } else if (a instanceof Number) {
        arktest.assertTrue(true);
    }else if (a instanceof string) {
        arktest.assertTrue(false);
    }else if (a instanceof object) {
        arktest.assertTrue(false);
    }
}
test_T2_b(1)
