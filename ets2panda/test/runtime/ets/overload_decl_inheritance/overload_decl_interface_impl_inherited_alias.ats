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

interface Base {
    foo(p: short): string { return "base-short" }
    fooInt(p: int): string
    overload foo { fooInt, foo }
}

class Derived implements Base {
    override foo(p: short): string { return "derived-short" }
    fooInt(p: int): string { return "derived-int" }
}

let base: Base = new Derived()
let derived = new Derived()

arktest.assertEQ(base.foo(1 as int), "derived-int")
arktest.assertEQ(derived.foo(1 as short), "derived-short")
arktest.assertEQ(derived.foo(1 as int), "derived-int")
