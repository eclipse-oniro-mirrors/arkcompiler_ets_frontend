/**
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

function nestedNonNull<S extends Object | null | undefined, T extends S>(x: NonNullable<T>): NonNullable<T> {
    return x;
}

let obj = new Object();
nestedNonNull<Object | null | undefined, Object>(obj);
nestedNonNull<Object | null | undefined, Object | null>(obj);
nestedNonNull<Object | null | undefined, Object | null>(null);
nestedNonNull<Object | null | undefined, Object | undefined>(undefined);

function main(): void {}

/* @@? 23:1 Error Semantic error ESE0127: No matching call signature for nestedNonNull(null) */
/* @@? 23:57 Error Semantic error ESE0046: Type 'null' is not compatible with type 'Object' at index 1 */
/* @@? 24:1 Error Semantic error ESE0127: No matching call signature for nestedNonNull(undefined) */
/* @@? 24:62 Error Semantic error ESE0046: Type 'undefined' is not compatible with type 'Object' at index 1 */
