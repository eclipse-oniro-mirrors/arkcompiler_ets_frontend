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

/*---
flags: [dynamic-ast]
---*/

import { A, I, foo } from "dynamic_import_tests/modules/module"

class B implements I {
    override f1(p: string): double {
        return 10;
    }
    override f2(p: double): string {
        return "abc"
    }
}

function main(): void {
    let obj: A = new A(10.0, "abc")
    let f1: string = obj.foo(20)
    let f2: double = A.bar()

    obj.f1 = f1
    A.f2 = f2

    foo(obj)
}
