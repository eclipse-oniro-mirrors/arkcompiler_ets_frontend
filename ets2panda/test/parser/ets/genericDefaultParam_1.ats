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

function alma<T = number>(param : T): T {
    return param
}

alma(1 as number)
alma<number>(1 as number)
alma<string>("alma")

class SomeType {}
interface Interface <T1 = SomeType> { }
class Base <T2 = SomeType> { }
class Derived1 extends Base implements Interface { }
class Derived2 extends Base<SomeType> implements Interface<SomeType> { }

class C2 <T1, T2 = number, T3 = string> {}
let c1 = new C2<number>
let c2 = new C2<number, string>
let c3 = new C2<number, Object, number>
