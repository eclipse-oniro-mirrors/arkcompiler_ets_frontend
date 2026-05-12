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

import { TeamOnly, RestrictedClass, RestrictedCtor, RestrictedIface, restrictedGlobal, customNs } from './internalApiLocalApi'

@TeamOnly
class UseRestrictedAnnotation {
}

customNs.customFunction()
let x = customNs.customValue
let a: RestrictedIface | null = null
let b = new /* @@ label1 */RestrictedCtor()
let c = new RestrictedClass()
let d = c.field
c.method()
let e = restrictedGlobal

/* @@? 16:53 Error Semantic error ESE154780: Access to 'RestrictedIface' is restricted by 'TeamOnly' */
/* @@? 16:70 Error Semantic error ESE154780: Access to 'restrictedGlobal' is restricted by 'TeamOnly' */
/* @@? 16:88 Error Semantic error ESE154780: Access to 'customNs' is restricted by 'TeamOnly' */
/* @@? 22:1 Error Semantic error ESE154780: Access to 'customFunction' is restricted by 'TeamOnly' */
/* @@? 22:1 Error Semantic error ESE154780: Access to 'customNs' is restricted by 'TeamOnly' */
/* @@? 23:9 Error Semantic error ESE154780: Access to 'customNs' is restricted by 'TeamOnly' */
/* @@? 23:9 Error Semantic error ESE154780: Access to 'customValue' is restricted by 'TeamOnly' */
/* @@? 24:8 Error Semantic error ESE154780: Access to 'RestrictedIface' is restricted by 'TeamOnly' */
/* @@@ label1 Error Semantic error ESE154780: Access to 'constructor' is restricted by 'TeamOnly' */
/* @@? 27:9 Error Semantic error ESE154780: Access to 'field' is restricted by 'TeamOnly' */
/* @@? 28:1 Error Semantic error ESE154780: Access to 'method' is restricted by 'TeamOnly' */
/* @@? 29:9 Error Semantic error ESE154780: Access to 'restrictedGlobal' is restricted by 'TeamOnly' */
