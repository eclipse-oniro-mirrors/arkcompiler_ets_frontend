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

const iterable = [10, 20, 30];

for (const value = 50 /* @@ label1 */of iterable) {
  console.log(value);
}

function foo(): int {
  console.log("aaa")
}

const iterable2 = [10, 20, 30];

for (let value = 40 /* @@ label2 */of iterable2) {
  value += 50;
  console.log(value);
}


/* @@? 18:1 Error Semantic error ESE0069: Source element type 'Double' is not assignable to the loop iterator type 'Int'. */
/* @@@ label1 Error Syntax error ESY0168: for-of loop variable declaration may not have an initializer. */
/* @@? 28:1 Error Semantic error ESE0069: Source element type 'Double' is not assignable to the loop iterator type 'Int'. */
/* @@@ label2 Error Syntax error ESY0168: for-of loop variable declaration may not have an initializer. */