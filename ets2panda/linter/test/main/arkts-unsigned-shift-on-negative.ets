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

let a = -123;
let b = 456;
let c = -789;

let r1 = -123 >>> 0;      // error
let r2 = a >>> 0;         // error
let r3 = b >>> 0;         // valid
let r4 = c >>> 0;         // error

let d = -1;
let r5 = d >>> 0;         // error
