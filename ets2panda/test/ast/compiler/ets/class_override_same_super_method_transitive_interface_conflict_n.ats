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
class Root<T extends B>  { foo(p: T): void {} }
class Mid<T extends B>   extends Root<T> { foo(p: T): void {} }
interface WideBase<T extends B> { foo(p: A): void {} }
interface Wide<T extends B> extends WideBase<T> {}
class Leaf<T extends B> extends Mid<T> implements Wide<T> {}

/* @@? 22:59 Error Semantic error ESE602430: Method foo(p: T): undefined from class or interface Mid<T>, inherited into class or interface Leaf<T>, conflicts with the previous declaration of method foo(p: A): undefined from class or interface WideBase<T> because both declarations override method foo(p: T): undefined from class or interface Root<T> */
