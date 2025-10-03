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

interface I1<IT1>{}
interface I2<in IT1, out IT2>{}
class A1<IT1>{}
class A2<in IT1, out IT2>{}

class B1<in T1, out T2, T3> implements I1<T3>{}
class B2<in T1, out T2, T3> implements /* @@ label1 */I1<T1>{}
class B3<in T1, out T2, T3> implements I2<T1,T2>{}
class B4<in T1, out T2, T3> implements I2<T1,T3>{}
class B5<in T1, out T2, T3> implements /* @@ label2 */I2<T2,T3>{}
class B6<in T1, out T2, T3> extends A1<T3>{}
class B7<in T1, out T2, T3> extends /* @@ label3 */A1<T1>{}
class B8<in T1, out T2, T3> extends A2<T1,T2>{}
class B9<in T1, out T2, T3> extends A2<T1,T3>{}
class B10<in T1, out T2, T3> extends /* @@ label4 */A2<T2,T3>{}

/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label4 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */