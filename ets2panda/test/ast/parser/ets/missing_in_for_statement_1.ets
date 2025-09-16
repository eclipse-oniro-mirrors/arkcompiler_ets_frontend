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

for /* @@ label */let i = 0; i < count; ++i) {
    result = result + p[i]!.awaitResolution() * /* @@ label1 */a[i];
}

/* @@@ label Error SyntaxError: Expected '(', got 'let'. */
/* @@? 16:34 Error TypeError: Unresolved reference count */
/* @@? 17:5 Error TypeError: Unresolved reference result */
/* @@? 17:23 Error TypeError: Unresolved reference p */
/* @@? 17:23 Error TypeError: Indexed access is not supported for such expression type. */
/* @@@ label1 Error TypeError: Unresolved reference a */
/* @@@ label1 Error TypeError: Indexed access is not supported for such expression type. */
