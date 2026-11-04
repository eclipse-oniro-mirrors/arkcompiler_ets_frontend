/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

/*---
flags: [dynamic-ast]
---*/

import { AValue } from "dynamic_import_tests"
import { ADeclared } from "dynamic_import_tests/modules/instanceof"

class A {}

function fn_object(o: Object): boolean {
    return o instanceof Object;
}

function fn_ets_object(o: A): boolean {
    return o instanceof Object;
}

function fn_jsvalue(o: JSValue): boolean {
    return o instanceof Object;
}

function fn_dyn_value(o: AValue): boolean {
    return o instanceof Object;
}

function fn_dyn_decl(o: ADeclared): boolean {
    return o instanceof Object;
}
