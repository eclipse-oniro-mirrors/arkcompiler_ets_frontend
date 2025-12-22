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

class A {}

let x: (i: int, n: number) => A;
x = (i: int, n: number): /* @@ label */this => { return new A(); };

/* @@? 19:5 Error Semantic error ESE0318: Type '((p1: Int, p2: Double) => ETSGLOBAL)' cannot be assigned to type '((p1: Int, p2: Double) => A)' */
/* @@@ label Error Syntax error ESY0044: A 'this' type is available only as return type in a non-static method of a class or struct and extension functions. */
/* @@? 19:50 Error Semantic error ESE0031: Only extension function or a class method can return 'this' */
