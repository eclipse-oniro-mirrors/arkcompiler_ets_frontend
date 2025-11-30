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

class B<BT>{}
class A<T> {
    static foo0(a0: B<T>): void { }
    static foo1(a0: T[]): void { }
    static foo3(...a0: T[]): void { }
    static foo2(a0: [T]): void { }
    static foo4<U = T>(a0: U): void { }
    static foo5(a0: (a1: T) => T): void { }
}

/* @@? 1:3 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 1:3 Error TypeError: Cannot find type 'T'. */
/* @@? 18:23 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 18:23 Error TypeError: Cannot find type 'T'. */
/* @@? 20:17 Error SyntaxError: Rest parameter should be either array or tuple type. */
/* @@? 21:22 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 21:22 Error TypeError: Cannot find type 'T'. */
/* @@? 22:21 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 22:21 Error TypeError: Cannot find type 'T'. */
/* @@? 23:26 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 23:26 Error TypeError: Cannot find type 'T'. */
/* @@? 23:32 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 23:32 Error TypeError: Cannot find type 'T'. */
