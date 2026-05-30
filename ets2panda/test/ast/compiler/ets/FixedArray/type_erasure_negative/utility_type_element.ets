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

/*---
desc: >-
  Utility types like Record<K, V> are generic type instantiations and are
  caught by the same check that handles generic objects.  Readonly<T>,
  Awaited<T>, Required<T>, Partial<T> and NonNullable<T> are simplified by
  the compiler before the check runs and do not need a separate check.
expected_out: Expected_CTE
name: fixed_array_type_erasure_utility_type_element
tags:
- compile-only
- negative
---*/

function main() {
   let a: FixedArray<Record<string, int>> = [{"key": 1}]
}

/* @@? 30:45 Error Semantic error ESE461884: Type 'Record<String,Int>' is not preserved by type erasure and cannot be used as type argument for FixedArray instantiations. */
