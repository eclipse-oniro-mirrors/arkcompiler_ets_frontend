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

function f1(a: int) {}
function f2(a: long) {}
function f3(a: double) {}
function f4(a: float) {}

overload foo1{ f2, f1 }
overload foo2{ f3, f2 }
overload foo3{ f3, f1 }
overload foo4{ f3, f4 }

/* @@? 21:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Int) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Long) => undefined' prevails. */
/* @@? 22:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Long) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double) => undefined' prevails. */
/* @@? 23:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Int) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double) => undefined' prevails. */
/* @@? 24:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Float) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double) => undefined' prevails. */
