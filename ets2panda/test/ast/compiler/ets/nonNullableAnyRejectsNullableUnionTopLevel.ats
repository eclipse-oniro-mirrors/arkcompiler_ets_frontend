/**
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

class A {}

let a: A | undefined = undefined
let b: NonNullable<Any> = a
let anyValue: Any = undefined
let anyNonNull: NonNullable<Any> = anyValue

function main(): void {}

/* @@? 19:27 Error Semantic error ESE0318: Type 'A|undefined' cannot be assigned to type 'NonNullable<Any>' */
/* @@? 21:36 Error Semantic error ESE0318: Type 'Any' cannot be assigned to type 'NonNullable<Any>' */
