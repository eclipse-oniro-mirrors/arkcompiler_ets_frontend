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

interface PI1 {}
interface PI2 {}
interface PI3 extends PI1, PI2 {}
class Param implements PI3 {}

interface I1 {
    m(p: PI2): String {
        return "I1"
    }
}

interface I2 {
    m(p: PI3): String
}

class C1 implements I1, I2 {
    m(p: PI3): String {
        return "C1"
    }
}

interface I3 extends I2 {
    m(p: PI2): String {
        return "I3"
    }
}

class C2 extends C1 implements I3 {}

interface I4 extends I3 {}

interface I5 extends I4 {
    m(p: PI1): String {
        return "I5"
    }
}

class C3 extends C2 implements I5 {}

let result = new C3().m(new Param())
