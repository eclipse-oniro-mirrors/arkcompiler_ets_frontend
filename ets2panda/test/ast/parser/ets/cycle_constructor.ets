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

class A {
    private _parentField: int;
    get parentField(): int {
        return this._parentField;
    }
    public constructor() {
    }
}

class B extends A {
    childField1: int
    public constructor() {
        this(super.parentField)
    }
}

/* @@? 28:9 Error Semantic error ESE0124: Expected 0 arguments, got 1. */
/* @@? 28:9 Error Semantic error ESE0127: No matching call signature for cycle_constructor.B(Int) */
/* @@? 28:9 Error Semantic error ESE0204: No matching call signature for constructor */
/* @@? 28:14 Error Semantic error ESE0235: Using super is not allowed in constructor */