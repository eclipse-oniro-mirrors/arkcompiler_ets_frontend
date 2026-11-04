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

function f1(a: A) {}
function f2(a: B) {}
function f3(a: C) {}
function f4(a: A, b?: B) {}
function f5(a: C, b: C)  {}

overload foo1{ f1, f2 }
overload foo3{ f1, f3 }
overload foo4{ f4, f5 }

/* @@? 26:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: B) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: A) => undefined' prevails. */
/* @@? 27:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: C) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: A) => undefined' prevails. */
/* @@? 28:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: C, b: C) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: A, b: B|undefined) => undefined' prevails. */
