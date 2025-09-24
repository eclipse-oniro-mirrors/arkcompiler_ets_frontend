/*
 Copyright (c) 2025 Huawei Device Co., Ltd.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 *
 http://www.apache.org/licenses/LICENSE-2.0
 *
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.limitations under the License.
 */


// 1. Basic usage
let [a, b] = [1, 2];
print(a, b); // 1 2

// 2. Skip some elements
let [a1, , c1] = [1, 2, 3];
print(a1, c1); // 1 3

// 3. Rest elements
let [a2, ...rest] = [1, 2, 3, 4];
print(a2);    // 1
print(rest);  // [2, 3, 4]

// 4. Default values
let [a3 = 10, b3 = 20] = [1];
print(a3, b3); // 1 20

// 5. Default values won't replace null
let [a4 = 10] = [null];
print(a4); // null

// 6. Nested destructuring
let [a5, [b5, c5]] = [1, [2, 3]];
print(a5, b5, c5); // 1 2 3

// 7. Destructuring assignment to existing variables (object destructuring)
let a6, b6;
({a6, b6} = {a6: 1, b6: 2});
print(a6, b6); // 1 2

// 8. Swap variables
let a7 = 1, b7 = 2;
[a7, b7] = [b7, a7];
print(a7, b7); // 2 1