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

function f1(a: number, b?: number) {}
function f2(a: number) {}
function f3(a: number, b: number) {}

function g1(a: number, b?: number, c?: number) {}
function g2(a: number, b: number) {}
function g3(a: number, b: number, c: number) {}

overload foo1{ f1, f2 }
overload foo2{ f1, f3 }

overload foo3{ g1, g2 }
overload foo4{ g1, g3 }


/* @@? 24:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: Double|undefined) => undefined' prevails. */
/* @@? 25:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: Double) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: Double|undefined) => undefined' prevails. */
/* @@? 27:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: Double) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: Double|undefined, c: Double|undefined) => undefined' prevails. */
/* @@? 28:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: Double, c: Double) => undefined' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: Double|undefined, c: Double|undefined) => undefined' prevails. */
