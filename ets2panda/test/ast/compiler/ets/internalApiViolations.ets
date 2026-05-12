/**
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

import { annotation } from 'arkruntime'
import { /* @@ label1 */restrictedNs, RestrictedMembers, ConstructorRestrictedApi } from './internalApiLocalApi'

@annotation.Async
class X {
    p: number = 0
}

restrictedNs.restrictedFunction()
let a = restrictedNs.restrictedValue
let c = new RestrictedMembers()
let d = c.value
c.ping()
let e = new /* @@ label2 */ConstructorRestrictedApi()

/* @@@ label1 Error Semantic error ESE154780: Access to 'restrictedNs' is restricted by 'TestRestriction' */
/* @@? 19:2 Error Semantic error ESE154780: Access to 'Async' is restricted by 'BytecodeAPI' */
/* @@? 19:13 Error Semantic error ESE154780: Access to 'Async' is restricted by 'BytecodeAPI' */
/* @@? 24:1 Error Semantic error ESE154780: Access to 'restrictedFunction' is restricted by 'TestRestriction' */
/* @@? 24:1 Error Semantic error ESE154780: Access to 'restrictedNs' is restricted by 'TestRestriction' */
/* @@? 25:9 Error Semantic error ESE154780: Access to 'restrictedNs' is restricted by 'TestRestriction' */
/* @@? 25:9 Error Semantic error ESE154780: Access to 'restrictedValue' is restricted by 'TestRestriction' */
/* @@? 27:9 Error Semantic error ESE154780: Access to 'value' is restricted by 'TestRestriction' */
/* @@? 28:1 Error Semantic error ESE154780: Access to 'ping' is restricted by 'TestRestriction' */
/* @@@ label2 Error Semantic error ESE154780: Access to 'constructor' is restricted by 'myapi' */
