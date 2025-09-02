/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

let a = []
a[0] = 1
a[1] = "1"
let b = a[0] + a[1]

/* @@? 16:9 Error TypeError: Can't resolve array type */
/* @@? 17:1 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 18:1 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 19:9 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 19:16 Error TypeError: Indexed access is not supported for such expression type. */
