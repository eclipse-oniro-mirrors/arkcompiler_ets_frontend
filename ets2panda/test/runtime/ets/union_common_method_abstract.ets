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

interface I1 { name() : string }
interface I2 { name() : string }

final class A implements I1 {
    name(): string {
        return "I am A"
    }
}

final class B implements I1 {
    name(): string {
        return "I am B"
    }
}

final class C implements I2 {
    name(): string {
        return "I am C"
    }
}

function f1(x: I1 | C) {
    return x.name();
}

function f2(x: I1 | I2) {
    return x.name();
}

function f3(x: A | B | I2) {
    return x.name();
}

arktest.assertEQ(f1(new A), "I am A")
arktest.assertEQ(f2(new C), "I am C")
arktest.assertEQ(f3(new B), "I am B")
