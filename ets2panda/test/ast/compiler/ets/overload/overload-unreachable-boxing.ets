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

function f1(a: Double) {}
function f2(a: double) {}
function f3(a: Int) {}
function f4(a: int) {}
function f5(a: Boolean) {}
function f6(a: boolean) {}

overload foo1{ f1, f2 }
overload foo2{ f3, f4 }
overload foo3{ f5, f6 }


/* @@? 23:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double) => undefined' prevails. */
/* @@? 24:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Int) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Int) => undefined' prevails. */
/* @@? 25:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Boolean) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Boolean) => undefined' prevails. */
