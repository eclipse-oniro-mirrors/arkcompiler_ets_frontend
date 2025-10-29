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
enum Colorset {Red, Green,Blue};
const result = JSON.stringify(Colorset);
console.log("Hello",result);

/* @@? 15:1 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 16:16 Error Semantic error ESE0127: No matching call signature for stringify(Colorset) */
/* @@? 16:31 Error Semantic error ESE154289: Enum 'Colorset' cannot be used as object */