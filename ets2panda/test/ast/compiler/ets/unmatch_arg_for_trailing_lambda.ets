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

function foo(a?: number, f?: () => void) { }

foo(1, "2")
foo(1, 1)
foo(1, () => { return "1" })

/* @@? 18:1 Error Semantic error ESE0127: No matching call signature for foo(Int, "2") */
/* @@? 18:8 Error Semantic error ESE0046: Type '"2"' is not compatible with type '((() => void))|undefined' at index 2 */
/* @@? 19:1 Error Semantic error ESE0127: No matching call signature for foo(Int, Int) */
/* @@? 19:8 Error Semantic error ESE0046: Type 'Int' is not compatible with type '((() => void))|undefined' at index 2 */
/* @@? 20:1 Error Semantic error ESE0127: No matching call signature for foo(Int, (() => String)) */
/* @@? 20:23 Error Semantic error ESE0091: Type '"1"' is not compatible with the enclosing method's return type 'void' */
