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

// Getters with cyclically inferred return types cannot be resolved and must be
// reported as a circular reference instead of recursing in the checker.
class C {
    constructor() {
        let v = this.y;
    }

    get y() {
        return this.y;
    }
}

/* @@? 23:10 Error Semantic error ESE0381: Circular call function */
/* @@? 24:21 Warning Warning W0023: Reading the value of the property inside its getter may lead to an endless loop. */
