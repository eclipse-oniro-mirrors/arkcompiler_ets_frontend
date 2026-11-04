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

let b1 = true
let b2 = false
let /* @@ label */b2? /* @@ label1 */= b1 || b2

/* @@? 18:19 Error Semantic error ESE0351: Variable 'b2' has already been declared. */
/* @@? 18:38 Error Syntax error ESY0306: Optional variable is deprecated and no longer supported. */
