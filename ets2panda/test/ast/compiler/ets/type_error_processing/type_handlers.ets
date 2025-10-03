/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

interface A {
    a: int
    b?: string
}


type T1 = Partial<>
type T2 = Partial<A>
type T3 = Partial<A, A>
type T4 = Partial<A, int>
type T5 = Partial<int>

type T6 = Readonly<>
type T7 = Readonly<A>
type T8 = Readonly<A, A>
type T9 = Readonly<A, int>
type T10 = Readonly<int>

type T11 = Required<>
type T12 = Required<A>
type T13 = Required<A, A>
type T14 = Required<A, int>
type T15 = Required<int>


/* @@? 22:18 Error Semantic error ESE0223: Invalid number of type parameters for Partial type, should be 1. */
/* @@? 24:18 Error Semantic error ESE0223: Invalid number of type parameters for Partial type, should be 1. */
/* @@? 25:18 Error Semantic error ESE0223: Invalid number of type parameters for Partial type, should be 1. */
/* @@? 28:19 Error Semantic error ESE0223: Invalid number of type parameters for Readonly type, should be 1. */
/* @@? 30:19 Error Semantic error ESE0223: Invalid number of type parameters for Readonly type, should be 1. */
/* @@? 31:19 Error Semantic error ESE0223: Invalid number of type parameters for Readonly type, should be 1. */
/* @@? 34:20 Error Semantic error ESE0223: Invalid number of type parameters for Required type, should be 1. */
/* @@? 36:20 Error Semantic error ESE0223: Invalid number of type parameters for Required type, should be 1. */
/* @@? 37:20 Error Semantic error ESE0223: Invalid number of type parameters for Required type, should be 1. */