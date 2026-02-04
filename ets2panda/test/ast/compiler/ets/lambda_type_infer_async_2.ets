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

async function foo() {
    const p = Promise.resolve(null);

    const result = await p.then(obj => {
        const value = obj?.prop ?? "default";
        return typeof value;
    });
}

/* @@? 19:20 Error Semantic error ESE0303: Type '*ERROR_TYPE*' can not be awaited, it is not a Promise. */
/* @@? 19:26 Error Semantic error ESE0127: No matching call signature for then(((p1: null) => void)) */
/* @@? 20:23 Error Semantic error ESE0087: Property 'prop' does not exist on type 'never' */
