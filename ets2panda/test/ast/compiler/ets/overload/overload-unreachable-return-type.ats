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

function f1(a: number): string { return "a"; }
function f2(a: number): number { return 1; }
function f3(a: string|number): boolean { return true; }
function f4(a: string): object { return {}; }


// Check that return type do not participate in check
overload foo1{ f3, f1 }
overload foo2{ f3, f2 }
overload foo3{ f3, f4 }


/* @@? 23:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double) => String' will never be selected for a call, as the preceding entity with the signature '(a: String|Double) => Boolean' prevails. */
/* @@? 24:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double) => Double' will never be selected for a call, as the preceding entity with the signature '(a: String|Double) => Boolean' prevails. */
/* @@? 25:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: String) => Object' will never be selected for a call, as the preceding entity with the signature '(a: String|Double) => Boolean' prevails. */
