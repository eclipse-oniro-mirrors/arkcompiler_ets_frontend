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

let z0 = 1.0 / -0.0
arktest.assertEQ(z0.toString(), "-Infinity")

const x1 = 1.0;
const y1 = -0.0;
const z1 = x1 / y1;
arktest.assertEQ(z1.toString(), "-Infinity")

let z2 = 1.0 / 0.0
arktest.assertEQ(z2.toString(), "Infinity")

const x3 = 1.0;
const y3 = 0.0;
const z3 = x3 / y3;
arktest.assertEQ(z3.toString(), "Infinity")

