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
desc: FixedArray element type must be preserved by type erasure; function types
  are not preserved because parameter types erase to Any and return type to never.
expected_out: Expected_CTE
name: fixed_array_type_erasure_function_element_type
tags:
- compile-only
- negative
---*/

function main() {
   let arr: FixedArray<(p: number) => void> = [(x: number) => {}]
}

/* @@? 27:47 Error Semantic error ESE461884: Type '((p1: Double) => undefined)' is not preserved by type erasure and cannot be used as type argument for FixedArray instantiations. */
