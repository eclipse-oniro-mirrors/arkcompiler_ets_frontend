/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function test_n(v: Object | null) { return v ? true : false }
function test_u(v: Object | undefined) { return v ? true : false }
function test_nu(v: Object | null | undefined) { return v ? true : false }

function main() {
    arktest.assertTrue(test_n({} as Object))
    arktest.assertTrue(test_u({} as Object))
    arktest.assertTrue(test_nu({} as Object))

    arktest.assertTrue(!null)
    if (null) { arktest.assertTrue(false)}
    arktest.assertTrue(!test_n(null))
    arktest.assertTrue(!test_nu(null))

    arktest.assertTrue(!undefined)
    if (undefined) { arktest.assertTrue(false) }
    arktest.assertTrue(!test_u(undefined))
    arktest.assertTrue(!test_nu(undefined))
}
