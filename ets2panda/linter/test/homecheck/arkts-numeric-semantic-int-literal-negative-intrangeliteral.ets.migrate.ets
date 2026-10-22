/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

let a = 2147483647          // warning1, defined as int, arkts-numeric-semantic
let b = 2147483648          // exceeds int range, no warning needed
let c = -2147483648         // warning2, defined as int, arkts-numeric-semantic
let d = -2147483649         // exceeds int range, no warning needed
let e = 0x7FFFFFFF          // warning3, defined as int, arkts-numeric-semantic
let f = 0xcafebabe          // exceeds int range, no warning needed
let g = -0x80000000         // warning4, defined as int, arkts-numeric-semantic
let h = -0x80000001         // exceeds int range, no warning needed
let j = 2.147483647e9       // number type, no warning
let k = -1e10               // exceeds int range, no warning needed