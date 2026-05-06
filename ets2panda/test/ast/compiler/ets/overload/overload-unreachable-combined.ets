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

function f1(a: number, b: string, c?: boolean): string { return "a"; }
function f2(a: number, b: string): number { return 1; }
function f3(a: number, b: string, c: boolean): object { return {}; }

function g1(a: number, b: string, ...args: boolean[]): string { return "a"; }
function g2(a: number, b: string, c: boolean): number { return 1; }
function g3(a: number, b: string, c: boolean, d: boolean): object { return {}; }


function e1(a: number, b: string, ...args: boolean[]): number { return 1; }
function e2<T extends number>(a: T, b: string, ...args: boolean[]): string { return "a"; }


overload foo1{ f1, f2 }
overload foo2{ f1, f3 }
overload foo3{ g1, g2 }
overload foo4{ g1, g3 }
overload foo5{ e1, e2 }


/* @@? 29:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: String) => Double' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: String, c: Boolean|undefined) => String' prevails. */
/* @@? 30:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: String, c: Boolean) => Object' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: String, c: Boolean|undefined) => String' prevails. */
/* @@? 31:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: String, c: Boolean) => Double' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: String, ...args: Array<Boolean>) => String' prevails. */
/* @@? 32:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(a: Double, b: String, c: Boolean, d: Boolean) => Object' will never be selected for a call, as the preceding entity with the signature '(a: Double, b: String, ...args: Array<Boolean>) => String' prevails. */
