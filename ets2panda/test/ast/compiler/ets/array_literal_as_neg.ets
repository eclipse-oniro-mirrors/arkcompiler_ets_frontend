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

/**
 * Negative test: array literal with incompatible context types
 * Array literal [1, 2] cannot be cast to Array<string> or its supertypes
 */

let a = [1, 2] as Array<string>
let b = [1, 2] as ReadonlyArray<string>
let c = [1, 2] as ConcatArray<string>
let d = [1, 2] as ArrayLike<string>

/* @@? 21:10 Error Semantic error ESE0227: Array element at index 0 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 21:13 Error Semantic error ESE0227: Array element at index 1 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 22:10 Error Semantic error ESE0227: Array element at index 0 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 22:13 Error Semantic error ESE0227: Array element at index 1 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 23:10 Error Semantic error ESE0227: Array element at index 0 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 23:13 Error Semantic error ESE0227: Array element at index 1 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 24:10 Error Semantic error ESE0227: Array element at index 0 with type 'Int' is not compatible with the target array element type 'String' */
/* @@? 24:13 Error Semantic error ESE0227: Array element at index 1 with type 'Int' is not compatible with the target array element type 'String' */