/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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


class Dog {
    name: String = "";
    legs: Int = 0;
}

let dog: Dog = {
    name: "Scooby"
}

let name = dog?.name ?? "Pluto";
let legCount: int = dog?.legs ?? 4;
/* @@? 26:12 Warning Warning W0015: Operand of the optional-chaining expression is always non-nullish. */
/* @@? 26:12 Warning Warning W971874: result of the nullish-coalescing expression is equal to its left-hand-side expression value. */
/* @@? 27:21 Warning Warning W0015: Operand of the optional-chaining expression is always non-nullish. */
/* @@? 27:21 Warning Warning W971874: result of the nullish-coalescing expression is equal to its left-hand-side expression value. */
