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

function foo<T>(b: T | undefined = 4) {}

let a = 4;
function foo1<T>(b: T | undefined = a) {}

const a1 = 4;
function foo2<T>(b: T | undefined = a1) {}

/* @@? 16:17 Error TypeError: Type 'T|Int' cannot be assigned to type 'T|undefined' */
/* @@? 19:18 Error TypeError: Type 'T|Int' cannot be assigned to type 'T|undefined' */
/* @@? 22:18 Error TypeError: Type 'T|Int' cannot be assigned to type 'T|undefined' */
