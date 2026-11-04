/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

export namespace NSOuter {
    export namespace NSInner {
        export enum XXXType {
            TAP = 0,
            PAT = 1
        }
    }
}
class A {}
class B {}

type XXXType = A | B

function foo(a : boolean) : NSOuter.NSInner.XXXType | int | string {
    if (a) {
        return NSOuter.NSInner.XXXType.TAP
    }
    return 0
}

function main() {
    const res : NSOuter.NSInner.XXXType = foo(true) as NSOuter.NSInner.XXXType
    arktest.assertEQ(res, NSOuter.NSInner.XXXType.TAP)
    const res1 : NSOuter.NSInner.XXXType = NSOuter.NSInner.XXXType.fromValue(1)
    arktest.assertEQ(res1, NSOuter.NSInner.XXXType.PAT)
}
