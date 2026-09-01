/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

class A {
    field = ""
}

let a: A = {
    field: "1",
    "field": '2',
}

let b: A = {
    field: "1",
    field: "2",
    "field": '3',
}

let c: A = {
    "field": '2',
    "field": '3',
}

interface I {
    field: string
}

let d: I = {
    field: "1",
    field: "2",
}

/* @@? 22:5 Error Semantic error ESE0100: An object literal cannot have multiple properties with the same name. */
/* @@? 22:5 Warning Warning W98268: Key in class composite should be identifier, the use of strings as keys will be deprecated soon */
/* @@? 27:5 Error Semantic error ESE0100: An object literal cannot have multiple properties with the same name. */
/* @@? 28:5 Error Semantic error ESE0100: An object literal cannot have multiple properties with the same name. */
/* @@? 28:5 Warning Warning W98268: Key in class composite should be identifier, the use of strings as keys will be deprecated soon */
/* @@? 32:5 Warning Warning W98268: Key in class composite should be identifier, the use of strings as keys will be deprecated soon */
/* @@? 33:5 Error Semantic error ESE0100: An object literal cannot have multiple properties with the same name. */
/* @@? 33:5 Warning Warning W98268: Key in class composite should be identifier, the use of strings as keys will be deprecated soon */
/* @@? 42:5 Error Semantic error ESE0100: An object literal cannot have multiple properties with the same name. */
