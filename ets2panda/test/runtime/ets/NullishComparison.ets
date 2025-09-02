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

const vnull = ((): Object | null | undefined => { return null })();
const vundefined = ((): Object | null | undefined => { return undefined })();
const vobject = ((): Object | null | undefined => { return new Object() })();

function main() {
    arktest.assertTrue(null === null)
    arktest.assertTrue(null !== undefined)
    arktest.assertTrue(null !== vobject)
    arktest.assertTrue(undefined === undefined)
    arktest.assertTrue(undefined !== vobject)
    arktest.assertTrue(vobject === vobject)

    arktest.assertTrue(vnull === vnull)
    arktest.assertTrue(vnull !== vundefined)
    arktest.assertTrue(vnull !== vobject)
    arktest.assertTrue(vundefined === vundefined)
    arktest.assertTrue(vundefined !== vobject)

    arktest.assertEQ(null, null)
    arktest.assertTrue(null == undefined)
    arktest.assertNE(null, vobject)
    arktest.assertEQ(undefined, undefined)
    arktest.assertNE(undefined, vobject)
    arktest.assertEQ(vobject, vobject)

    arktest.assertEQ(vnull, vnull)
    arktest.assertTrue(vnull == vundefined)
    arktest.assertNE(vnull, vobject)
    arktest.assertEQ(vundefined, vundefined)
    arktest.assertNE(vundefined, vobject)
}
