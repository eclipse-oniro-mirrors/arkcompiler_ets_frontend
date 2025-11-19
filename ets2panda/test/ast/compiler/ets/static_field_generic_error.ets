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
    static a0: B<T>;
    static a1: T[];
    static a2: [T];
    static a3: (a1: T) => T;
    static a4: T;
}

/* @@? 1:3 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 1:3 Error TypeError: Cannot find type 'T'. */
/* @@? 18:18 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 18:18 Error TypeError: Cannot find type 'T'. */
/* @@? 20:17 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 20:17 Error TypeError: Cannot find type 'T'. */
/* @@? 21:21 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 21:21 Error TypeError: Cannot find type 'T'. */
/* @@? 21:27 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 21:27 Error TypeError: Cannot find type 'T'. */
/* @@? 22:16 Error TypeError: Static members cannot reference class type parameters 'T' */
/* @@? 22:16 Error TypeError: Cannot find type 'T'. */
