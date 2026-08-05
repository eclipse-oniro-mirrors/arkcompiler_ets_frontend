/**
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

// AC: mixed multi-param — param 1 is nullish-partial (string|null vs object) but param 2 is
// non-nullish-incompatible (number vs string). The nullish difference is NOT the sole
// incompatibility, so partial-override CTE must NOT fire (Approach A "solely" check).
// Falls to Implicit Method Overloading; x.foo("abc","abc") resolves to B.foo.
class A {
    foo(a: string | null, b: number) {}
}

class B extends A {
    foo(a: object, b: string) {}
}

let x = new B()
x.foo("abc", "abc")
