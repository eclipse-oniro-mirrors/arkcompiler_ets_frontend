/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { objA } from './fixtures/homecheck-interop-assign/Field/1.1import1.2obj/file1'

interface Y { s: string }

function goo() {
    let y: Y = { s: "Hi" }
    objA.data = y      // 在此报错

    const obj2 = objA
    obj2.data = y      // 在此报错
}