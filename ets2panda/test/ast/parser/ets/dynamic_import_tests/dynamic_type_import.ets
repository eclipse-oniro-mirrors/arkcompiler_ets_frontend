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

<<<<<<<< HEAD:ets2panda/test/ast/compiler/ets/forof_bad_type.ets
declare class A<T> {
    a: A<T>
    foo(f: (a: FixedArray<Tadad> | A<T>[]) => void):void
    bar():A<T>
}

new A<number>().bar().a.foo((x) => {
    for (let e of x) {
        console.log(e)
    }
})

/* @@? 18:27 Error TypeError: Cannot find type 'Tadad'. */
========
/*---
flags: [dynamic-ast]
---*/

import { C, requiredC } from "dynamic_import_tests/modules/module"

class A<T> {
    f?: A<C> = new A<C>() // should be ok
}

function foo(c: C) { }

foo({ s: "" }) // should be ok
>>>>>>>> origin/OpenHarmony_feature_Release_20250728:ets2panda/test/ast/parser/ets/dynamic_import_tests/dynamic_type_import.ets
