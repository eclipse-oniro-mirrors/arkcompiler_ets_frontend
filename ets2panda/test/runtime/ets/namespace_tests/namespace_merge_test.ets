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
namespace A{
    export let a = 1
    export function foo1():string {return "foo1"}
}

namespace A{
    export let b = 2
    export function foo2():string {return "foo2"}
}

namespace A{
    export let c = 3
    export function foo3():string {return "foo3"}
}

function main(){
    arktest.assertEQ(A.a, 1)
    arktest.assertEQ(A.b, 2)
    arktest.assertEQ(A.c, 3)
    arktest.assertEQ(A.foo1(), "foo1")
    arktest.assertEQ(A.foo2(), "foo2")
    arktest.assertEQ(A.foo3(), "foo3")
}