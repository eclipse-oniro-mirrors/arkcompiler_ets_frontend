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

async function foo() {
    try {
        await import("hello")
            .then((m: JSValue): null => null)
            .catch((a): never => {
                throw a as Error
            })
    } catch (e) {
        arktest.assertEQ((e as Error).message, "Dynamic import is not supported")
        return
    }
    arktest.assertTrue(!"nothing was thrown")
}

/* @@? 18:9 Error Semantic error ESE0303: Type '*ERROR_TYPE*' can not be awaited, it is not a Promise. */
/* @@? 18:15 Error Syntax error ESY0227: Unexpected token 'import'. */
