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

class A {}
class B extends A {}
class C {}

let ac: FixedArray<A> | FixedArray<C> = [new A(), new B()]
let b: FixedArray<B> = [new B(), new B()]

let cOnly: FixedArray<C> = b

/* @@? 23:28 Error TypeError: Type 'FixedArray<B>' cannot be assigned to type 'FixedArray<C>' */
