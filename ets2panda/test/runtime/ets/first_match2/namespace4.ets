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
    export function foo(a: number): string {
        return "invoke1"
    }
    export function foo(a: string): string {
        return "invoke2"
    }
    export namespace NS2 {
        export function foo(a: number): string {
            return "invoke3"
        }
        export function foo(a: string): string {
            return "invoke4"
        }
    }
}

function main() {
    arktest.assertEQ(NS.foo(1), "invoke1")
    arktest.assertEQ(NS.foo("abc"), "invoke2")
    arktest.assertEQ(NS.NS2.foo(1), "invoke3")
    arktest.assertEQ(NS.NS2.foo("abc"), "invoke4")
}
