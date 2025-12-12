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

type Lazy<T> = T | () => T

function useLazy(p: Lazy<string>): void {
    if (p instanceof Function && p() == 'From param') {
        console.log('param callable:', p())
    }
}

function main() {
    const v: Lazy<string> = () => 'From param'
    useLazy(v)
}

/* @@? 19:34 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 19:34 Error Semantic error ESE0002: Type 'never' has no call signatures. */
/* @@? 20:9 Error Semantic error ESE0127: No matching call signature for log("param callable:", ...) */
/* @@? 20:40 Error Semantic error ESE0002: Type 'never' has no call signatures. */