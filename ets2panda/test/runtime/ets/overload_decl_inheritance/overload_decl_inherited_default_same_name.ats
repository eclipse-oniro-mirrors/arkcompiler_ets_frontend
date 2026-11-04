/*
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

/*---
desc: >-
    A default same-name method declared together with the explicit overload
    remains available through an inherited alias when the implementing class
    only provides the other overload target.
---*/

interface Root {
    foo(p: short): string { return "short" }
    fooInt(p: int): string
    overload foo { fooInt, foo }
}

interface Mid extends Root {
}

class Impl implements Mid {
    fooInt(p: int): string { return "int" }
}

function main(): void {
    let m: Mid = new Impl()
    arktest.assertEQ(m.foo(1 as short), "int")
    arktest.assertEQ(m.foo(1 as int), "int")
}
