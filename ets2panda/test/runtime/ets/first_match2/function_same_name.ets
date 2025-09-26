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

namespace NS {
    export function foo(a: number) { return "namespace.invoke1" }
    export function foo(a: string) { return "namespace.invoke2" }
}

function foo(a: number) { return "invoke1" }
function foo(a: string) { return "invoke2" }

function main() {
    arktest.assertEQ(foo(123), "invoke1")
    arktest.assertEQ(foo("123"), "invoke2")
    arktest.assertEQ(NS.foo(123), "namespace.invoke1")
    arktest.assertEQ(NS.foo("123"), "namespace.invoke2")
}
