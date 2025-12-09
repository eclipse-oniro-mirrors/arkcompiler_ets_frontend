/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function foo<T>(a0: T): T { return a0; }

function main() {
    let fooInt1: (a0: Int) => Int = /* @@ label1 */foo<Double>
    let fooInt2 = foo<Double>
    let fooInt3: (a0: Int) => Int = /* @@ label2 */fooInt2

    let fooInt4 = foo<Double>
    let fooInt5: (a0: Int) => Int = /* @@ label3 */fooInt4

    let fooInt6 = foo<Double>
    let fooInt7: (a0: Int) => Int = /* @@ label4 */fooInt6
}

/* @@@ label1 Error Semantic error ESE0318: Type '((p1: Double) => Double)' cannot be assigned to type '((p1: Int) => Int)' */
/* @@@ label2 Error Semantic error ESE0318: Type '((p1: Double) => Double)' cannot be assigned to type '((p1: Int) => Int)' */
/* @@@ label3 Error Semantic error ESE0318: Type '((p1: Double) => Double)' cannot be assigned to type '((p1: Int) => Int)' */
/* @@@ label4 Error Semantic error ESE0318: Type '((p1: Double) => Double)' cannot be assigned to type '((p1: Int) => Int)' */
