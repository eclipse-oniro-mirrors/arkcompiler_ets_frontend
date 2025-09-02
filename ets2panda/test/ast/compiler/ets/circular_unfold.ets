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

function foo() {
    const a = a
    const b = b
    return b - a
}

/* @@? 17:11 Error TypeError: Circular dependency detected for identifier: a */
/* @@? 17:15 Error TypeError: Variable 'a' is accessed before it's initialization. */
/* @@? 18:11 Error TypeError: Circular dependency detected for identifier: b */
/* @@? 18:15 Error TypeError: Variable 'b' is accessed before it's initialization. */
