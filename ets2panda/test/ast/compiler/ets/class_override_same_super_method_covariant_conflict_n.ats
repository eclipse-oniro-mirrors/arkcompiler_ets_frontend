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

class A {}
class B extends A {}
class C extends B {}
class Root<T extends B>  { foo(p: T): A { return new A() } }
class Mid<T extends B>   extends Root<T> { foo(p: B): B { return new B() } }
interface Wide<T extends B> { foo(p: A): C { return new C() } }
class Leaf<T extends B> extends Mid<T> implements Wide<T> {}

/* @@? 22:59 Error Semantic error ESE602430: Method foo(p: B): B from class or interface Mid<T>, inherited into class or interface Leaf<T>, conflicts with the previous declaration of method foo(p: A): C from class or interface Wide<T> because both declarations override method foo(p: T): A from class or interface Root<T> */
