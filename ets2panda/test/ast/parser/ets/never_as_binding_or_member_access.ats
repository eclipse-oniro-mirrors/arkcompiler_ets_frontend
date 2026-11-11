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

// 'never' cannot be used as a for-of binding, a catch parameter or a member-access name.

function bindings() {
    let arr: int[] = [1, 2];
    for (let never of arr) {
    }
    try {
    } catch (never) {
    }
}

interface Holder {
    never: int
}

function access(h: Holder) {
    let v = h.never;
}

/* @@? 20:14 Error Syntax error ESY0295: never is a predefined type, cannot be used as an identifier */
/* @@? 23:14 Error Syntax error ESY0295: never is a predefined type, cannot be used as an identifier */
/* @@? 32:15 Error Syntax error ESY0295: never is a predefined type, cannot be used as an identifier */
