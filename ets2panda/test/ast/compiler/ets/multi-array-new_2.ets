/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function baz(): number {
    return 5.1
}

function main(): void {
   let A: Array<number> = new Array<number>();
   let a: FixedArray<FixedArray<FixedArray<Number>>>;
   a = new Number[/* @@ label */baz()][4][/* @@ label1 */A.length + 3.0000];
}

/* @@@ label Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */
/* @@@ label1 Error Semantic error ESE0236: Type 'Double' cannot be used as an index type. Only primitive or unboxable integral types can be used as index. */
