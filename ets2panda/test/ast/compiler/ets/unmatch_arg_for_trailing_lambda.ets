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

/* @@? 18:1 Error TypeError: No matching call signature for foo(Double, "2") */
/* @@? 18:8 Error TypeError: Type '"2"' is not compatible with type '((() => void))|undefined' at index 2 */
/* @@? 19:1 Error TypeError: No matching call signature for foo(Double, Int) */
/* @@? 19:8 Error TypeError: Type 'Int' is not compatible with type '((() => void))|undefined' at index 2 */
/* @@? 20:23 Error TypeError: Unexpected return value, enclosing method return type is void. */
