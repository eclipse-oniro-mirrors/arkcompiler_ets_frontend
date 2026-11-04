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

class NestedClass {
    readonly constField: string = "123"
}

class A {
    readonly constClass: NestedClass

    constructor() {
        this.constClass = new NestedClass
    }
}

const constExpr = new A
let variable: constExpr.constClass.constField = 42


/* @@? 29:15 Error Semantic error ESE0371: Cannot find type 'constExpr'. */
/* @@? 29:25 Error Semantic error ESE0070: 'constClass' type does not exist. */
/* @@? 29:36 Error Semantic error ESE0070: 'constField' type does not exist. */