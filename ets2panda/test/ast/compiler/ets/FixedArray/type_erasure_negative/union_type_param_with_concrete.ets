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
desc: FixedArray element type must be preserved by type erasure; a union of a
  type parameter and a concrete type is not preserved because the type parameter
  erases to its constraint, altering the union structure.
expected_out: Expected_CTE
name: fixed_array_type_erasure_union_type_param_with_concrete
tags:
- compile-only
- negative
---*/

function foo<T>(v: T): FixedArray<T | number> {
    return [v]
}

/* @@? 28:12 Error Semantic error ESE461884: Type 'T|Double' is not preserved by type erasure and cannot be used as type argument for FixedArray instantiations. */
