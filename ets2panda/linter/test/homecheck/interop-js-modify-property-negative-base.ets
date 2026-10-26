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

// 1.2

'use static'


import { foo1 } from './fixtures/homecheck-interop-js-modify-property/ets/normal'
import { foo2 } from './fixtures/homecheck-interop-js-modify-property/ets/interprocedual'
import { foo3 } from './fixtures/homecheck-interop-js-modify-property/ets/cycle'
import { foo4 } from './fixtures/homecheck-interop-js-modify-property/ets/exceed_limit'
import { foo5 } from './fixtures/homecheck-interop-js-modify-property/ets/cycle2'
import { foo6, JSType } from './fixtures/homecheck-interop-js-modify-property/ets/jstype'
import { foo7 } from './fixtures/homecheck-interop-js-modify-property/ets/field'

class X {
    name: string = ""
    data: number = 0
}

foo1(new X())   // 在此报错

const obj = new X()
foo1(obj)           // 在此报错

const obj2 = new X()
function test() {
    foo1(obj2)      // 在此报错
}

foo2(new X())   // 在此报错
foo3(new X())   // 在此报错
foo4(new X())   // 在此报错
foo5(new X())   // 在此报错
foo6(new JSType())
foo7(new X())