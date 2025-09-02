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
    export namespace NS2 {
        export function foo1(a: string): string {
            return "invoke1"
        };
        export namespace NS3 {
            export function foo2(a: number): string {
                return "invoke2"
            };
        }
    }

    export function foo3(a: boolean): string {
        return "invoke3"
    };
}


overload foo{ NS.NS2.foo1, NS.NS2.NS3.foo2, NS.foo3, /* @@ label1 */NS.NS2.NS3.foo2};

/* @@@ label1 Error TypeError: Duplicate overloaded method. */

