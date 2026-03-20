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

class C {
    static foo111(a: number): string {
        return "static invoke1";
    }
}

class D extends C {
    static fooString(a: string): string {
        return "static invoke2"
    }
    static foo111FromC(a: number): string {
        return C.foo111(a)
    }
    static overload foo111 { foo111FromC, fooString };
}

function main() {
    arktest.assertEQ(D.foo111(1), "static invoke1")
    arktest.assertEQ(D.foo111("abc"), "static invoke2")
    arktest.assertEQ(C.foo111(1), "static invoke1")
}
